#include "sp/runtime/EventLoop.h"
#include "sp/runtime/Async.h"
#include "sp/runtime/Runtime.h"
#include <algorithm>
#include <cstring>
#include <thread>
#include <utility>

namespace sp::rt::net {

EventLoop::EventLoop() {
    initNetwork();
}

EventLoop::~EventLoop() {
    m_watched.clear();
    m_readyEvents.clear();
}

bool EventLoop::addSocket(sp_socket_handle_t sock, int64_t events, void* userData) {
    if (sock == SP_INVALID_SOCKET) return false;
    m_watched[sock] = WatchedSocket{sock, events, userData};
    return true;
}

bool EventLoop::modifySocket(sp_socket_handle_t sock, int64_t events) {
    auto it = m_watched.find(sock);
    if (it == m_watched.end()) return false;
    it->second.events = events;
    return true;
}

bool EventLoop::removeSocket(sp_socket_handle_t sock) {
    return m_watched.erase(sock) > 0;
}

int64_t EventLoop::poll(int timeoutMs) {
    m_readyEvents.clear();
    if (m_watched.empty()) return 0;

    std::vector<struct pollfd> pollfds;
    pollfds.reserve(m_watched.size());

    for (const auto& [sock, info] : m_watched) {
        struct pollfd pfd{};
        pfd.fd = sock;
        pfd.events = 0;
        if (info.events & SP_EVENT_READ) {
            pfd.events |= POLLIN;
        }
        if (info.events & SP_EVENT_WRITE) {
            pfd.events |= POLLOUT;
        }
        pfd.revents = 0;
        pollfds.push_back(pfd);
    }

#if defined(_WIN32)
    int ret = WSAPoll(pollfds.data(), static_cast<ULONG>(pollfds.size()), timeoutMs);
#else
    int ret = ::poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), timeoutMs);
#endif

    if (ret <= 0) {
        return ret; // 0 for timeout, <0 for error
    }

    for (const auto& pfd : pollfds) {
        if (pfd.revents != 0) {
            auto it = m_watched.find(pfd.fd);
            if (it != m_watched.end()) {
                int64_t revents = 0;
                if (pfd.revents & (POLLIN | POLLPRI)) {
                    revents |= SP_EVENT_READ;
                }
                if (pfd.revents & POLLOUT) {
                    revents |= SP_EVENT_WRITE;
                }
                if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    revents |= SP_EVENT_ERROR;
                }

                m_readyEvents.push_back(ReadyEvent{
                    it->first,
                    revents,
                    it->second.userData
                });
            }
        }
    }

    return static_cast<int64_t>(m_readyEvents.size());
}

} // namespace sp::rt::net

// ============================================================================
// Phase 2 slice B2: async scheduler + Task (see Async.h for the design and
// the honest limits). Implemented here so no CMakeLists.txt change is
// needed: EventLoop.cpp is already part of sp_runtime.
// ============================================================================

namespace sp::rt::async {

namespace {
std::chrono::steady_clock::time_point nowTp() {
    return std::chrono::steady_clock::now();
}
// Owned copy of a string via the SP allocator (never freed; matches how
// SP strings are owned by their holders).
const char* copyStrOwned(const std::string& s) {
    char* p = static_cast<char*>(::sp_rt_alloc(s.size() + 1));
    std::memcpy(p, s.data(), s.size());
    p[s.size()] = '\0';
    return p;
}
} // namespace

// --- Task -------------------------------------------------------------------

Task::~Task() {
    // Free an owned pointer payload (completePtrOwned box). The Task is
    // held in a shared_ptr, so this runs when the last reference drops.
    // Non-owned payloads (arrays via completePtr) are never freed here.
    void* owned = nullptr;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_ownsPtr) owned = m_ptrValue;
        m_ptrValue = nullptr;
        m_ownsPtr = false;
    }
    if (owned) ::sp_rt_free(owned);
}

bool Task::isReady() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_done;
}

bool Task::wasCancelled() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_cancelled;
}

int64_t Task::value() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_value;
}

std::string Task::strValue() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_strValue;
}

double Task::dblValue() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_dblValue;
}

void* Task::ptrValue() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_ptrValue;
}

void Task::wait() const {
    std::unique_lock<std::mutex> lock(m_mtx);
    m_cv.wait(lock, [this] { return m_done; });
}

bool Task::waitFor(int64_t timeoutMs) const {
    std::unique_lock<std::mutex> lock(m_mtx);
    if (timeoutMs < 0) {
        m_cv.wait(lock, [this] { return m_done; });
        return true;
    }
    return m_cv.wait_for(lock, std::chrono::milliseconds(timeoutMs),
                         [this] { return m_done; });
}

void Task::onComplete(std::function<void(Task&)> cb) {
    if (!cb) return;
    bool alreadyDone = false;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_done) {
            alreadyDone = true;
        } else {
            m_continuations.push_back(std::move(cb));
        }
    }
    if (alreadyDone) cb(*this);
}

void Task::complete(int64_t v) {
    std::vector<std::function<void(Task&)>> cbs;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_done) return;
        m_done = true;
        m_value = v;
        cbs = std::move(m_continuations);
        m_continuations.clear();
    }
    m_cv.notify_all();
    for (auto& cb : cbs) cb(*this);
}

void Task::completeStr(const char* v) {
    std::vector<std::function<void(Task&)>> cbs;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_done) return;
        m_done = true;
        m_value = 0;
        m_strValue = v ? v : "";
        cbs = std::move(m_continuations);
        m_continuations.clear();
    }
    m_cv.notify_all();
    for (auto& cb : cbs) cb(*this);
}

void Task::completeDbl(double v) {
    std::vector<std::function<void(Task&)>> cbs;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_done) return;
        m_done = true;
        m_value = 0;
        m_dblValue = v;
        cbs = std::move(m_continuations);
        m_continuations.clear();
    }
    m_cv.notify_all();
    for (auto& cb : cbs) cb(*this);
}

void Task::completePtr(void* p) {
    std::vector<std::function<void(Task&)>> cbs;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_done) return;
        m_done = true;
        m_value = 0;
        m_ptrValue = p;
        m_ownsPtr = false;
        cbs = std::move(m_continuations);
        m_continuations.clear();
    }
    m_cv.notify_all();
    for (auto& cb : cbs) cb(*this);
}

void Task::completePtrOwned(void* p) {
    std::vector<std::function<void(Task&)>> cbs;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_done) return;
        m_done = true;
        m_value = 0;
        m_ptrValue = p;
        m_ownsPtr = (p != nullptr);
        cbs = std::move(m_continuations);
        m_continuations.clear();
    }
    m_cv.notify_all();
    for (auto& cb : cbs) cb(*this);
}

bool Task::cancel() {
    std::vector<std::function<void(Task&)>> cbs;
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if (m_done) return false;
        m_done = true;
        m_cancelled = true;
        m_value = 0;
        cbs = std::move(m_continuations);
        m_continuations.clear();
    }
    m_cv.notify_all();
    for (auto& cb : cbs) cb(*this);
    return true;
}

// --- Scheduler ---------------------------------------------------------------

Scheduler::Scheduler() = default;
Scheduler::~Scheduler() = default;

int64_t Scheduler::nowMs() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               nowTp().time_since_epoch())
        .count();
}

uint64_t Scheduler::scheduleAfter(int64_t delayMs, std::function<void()> cb) {
    if (!cb) return 0;
    if (delayMs < 0) delayMs = 0;
    uint64_t id = m_nextTimerId++;
    m_timers.push(Timer{nowTp() + std::chrono::milliseconds(delayMs), id, std::move(cb)});
    return id;
}

void Scheduler::post(std::function<void()> cb) {
    if (cb) m_ready.push_back(std::move(cb));
}

bool Scheduler::cancel(uint64_t timerId) {
    if (timerId == 0) return false;
    // Lazily cancelled: the timer stays in the heap until due, then is
    // skipped. This keeps cancel() O(1) and avoids heap surgery.
    // Phase 6 slice C: the set is guarded — cancel() may run off the
    // loop thread (from Task::cancel()).
    std::lock_guard<std::mutex> lock(m_cancelMtx);
    auto [it, inserted] = m_cancelledTimers.insert(timerId);
    return inserted;
}

bool Scheduler::addSocket(sp_socket_handle_t sock, int64_t events,
                          std::function<void(int64_t)> cb) {
    if (!cb) return false;
    if (!m_loop.addSocket(sock, events, nullptr)) return false;
    m_socketCallbacks[sock] = std::move(cb);
    return true;
}

bool Scheduler::removeSocket(sp_socket_handle_t sock) {
    m_socketCallbacks.erase(sock);
    return m_loop.removeSocket(sock);
}

void Scheduler::fireDueTimers() {
    auto now = nowTp();
    while (!m_timers.empty() && m_timers.top().due <= now) {
        Timer t = m_timers.top();
        m_timers.pop();
        bool wasCancelled = false;
        {
            std::lock_guard<std::mutex> lock(m_cancelMtx);
            wasCancelled = m_cancelledTimers.erase(t.id) > 0;
        }
        if (wasCancelled) continue; // was cancelled
        m_ready.push_back(std::move(t.cb));
    }
}

int64_t Scheduler::msUntilNextTimer() const {
    if (m_timers.empty()) return -1;
    // Peek past lazily-cancelled timers at the head: they will be skipped
    // when due, so they must not extend the poll timeout. Copy the heap
    // (timers are cheap) to find the first live one. Round UP to a whole
    // millisecond: truncating (e.g. 9.6 ms -> 9 ms) would wake before the
    // timer is due and the caller would conclude no work was done.
    auto copy = m_timers;
    while (!copy.empty()) {
        bool wasCancelled = false;
        {
            std::lock_guard<std::mutex> lock(m_cancelMtx);
            wasCancelled = m_cancelledTimers.count(copy.top().id) > 0;
        }
        if (wasCancelled) {
            copy.pop();
            continue;
        }
        auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                      copy.top().due - nowTp())
                      .count();
        if (us <= 0) return 0;
        return (us + 999) / 1000;
    }
    return -1;
}

bool Scheduler::runOnce() {
    return runOnceCapped(-1);
}

bool Scheduler::runOnceCapped(int64_t maxWaitMs) {
    fireDueTimers();

    bool didWork = false;
    while (!m_ready.empty()) {
        auto cb = std::move(m_ready.front());
        m_ready.pop_front();
        didWork = true;
        cb();
        // New timers may have become due while callbacks ran.
        fireDueTimers();
    }
    if (didWork) return true;

    // Nothing ready: wait for socket activity or the next timer, capped
    // at maxWaitMs when the caller gave a deadline (runUntil/runFor).
    int64_t timeout = msUntilNextTimer();
    if (maxWaitMs >= 0 && (timeout < 0 || timeout > maxWaitMs)) timeout = maxWaitMs;
    if (timeout < 0 && m_socketCallbacks.empty()) return false; // idle
    if (timeout < 0) timeout = -1; // sockets only: block until activity

    if (m_socketCallbacks.empty()) {
        // EventLoop::poll() returns immediately with no watched sockets,
        // so sleep directly until the next timer is due.
        if (timeout > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        }
    } else {
        int64_t ready = m_loop.poll(static_cast<int>(timeout));
        if (ready > 0) {
            didWork = true;
            // Copy: callbacks may add/remove sockets.
            auto events = m_loop.getReadyEvents();
            for (const auto& ev : events) {
                auto it = m_socketCallbacks.find(ev.socketFd);
                if (it != m_socketCallbacks.end()) it->second(ev.events);
            }
        } else if (ready < 0) {
            // poll() error with watched sockets: do not spin.
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    fireDueTimers();
    while (!m_ready.empty()) {
        auto cb = std::move(m_ready.front());
        m_ready.pop_front();
        didWork = true;
        cb();
        fireDueTimers();
    }
    return didWork;
}

void Scheduler::runUntilIdle() {
    // Keep pumping while work remains. runOnce() blocks until the next
    // timer/socket event, so this cannot busy-spin; a single iteration
    // that finds nothing ready does NOT mean the loop is drained (a
    // timer may still be pending), so there is no break-on-false here.
    while (hasPending()) {
        runOnce();
    }
}

void Scheduler::runUntilTasksIdle() {
    // Like runUntilIdle() but a registered socket does NOT count as
    // pending work: only timers and queued callbacks keep the drain
    // going. runOnce() still blocks until the next timer when there is
    // nothing ready, so this cannot busy-spin either.
    for (;;) {
        if (!m_ready.empty()) {
            runOnce();
            continue;
        }
        // Any live (non-cancelled) timer?
        bool timerPending = false;
        auto copy = m_timers;
        while (!copy.empty()) {
            if (m_cancelledTimers.count(copy.top().id) == 0) {
                timerPending = true;
                break;
            }
            copy.pop();
        }
        if (!timerPending) return;
        runOnce();
    }
}

bool Scheduler::runUntil(const std::function<bool()>& done, int64_t timeoutMs) {
    auto deadline = timeoutMs < 0
        ? std::chrono::steady_clock::time_point::max()
        : nowTp() + std::chrono::milliseconds(timeoutMs);
    while (!done()) {
        if (!hasPending()) return false;
        if (nowTp() >= deadline) return false;
        int64_t remaining = timeoutMs < 0
            ? -1
            : std::chrono::duration_cast<std::chrono::milliseconds>(deadline - nowTp()).count();
        runOnceCapped(remaining);
        if (nowTp() >= deadline && !done()) return false;
    }
    return true;
}

void Scheduler::runFor(int64_t durationMs) {
    if (durationMs <= 0) {
        runOnce();
        return;
    }
    auto deadline = nowTp() + std::chrono::milliseconds(durationMs);
    while (nowTp() < deadline && hasPending()) {
        int64_t remaining =
            std::chrono::duration_cast<std::chrono::milliseconds>(deadline - nowTp()).count();
        runOnceCapped(remaining);
    }
}

bool Scheduler::hasPending() const {
    if (!m_ready.empty()) return true;
    // A live (non-cancelled) timer counts as pending.
    auto copy = m_timers;
    while (!copy.empty()) {
        if (m_cancelledTimers.count(copy.top().id) == 0) return true;
        copy.pop();
    }
    return !m_socketCallbacks.empty();
}

size_t Scheduler::pendingCount() const {
    size_t n = m_ready.size() + m_socketCallbacks.size();
    auto copy = m_timers;
    while (!copy.empty()) {
        if (m_cancelledTimers.count(copy.top().id) == 0) ++n;
        copy.pop();
    }
    return n;
}

Scheduler& defaultScheduler() {
    static Scheduler sched;
    return sched;
}

TaskPtr sleepTask(Scheduler& sched, int64_t delayMs) {
    auto task = std::make_shared<Task>();
    std::weak_ptr<Task> weak = task;
    uint64_t id = sched.scheduleAfter(delayMs, [weak] {
        if (auto t = weak.lock()) t->complete(0);
    });
    {
        std::lock_guard<std::mutex> lock(task->m_mtx);
        task->m_timerId = id;
    }
    // Phase 6 slice C: cancelling the task also cancels its timer, so a
    // cancelled task leaves no stale timer behind. Before this, the
    // timer stayed pending until its deadline and then completed the
    // already-cancelled task (a no-op) — but the scheduler still had
    // work until then, so a later runUntilTasksIdle waited it out (in
    // Phase 5-A one test took 4719 ms on a stale 5000 ms timer). The
    // hook only cancels when the task was cancelled: a normally
    // completed task's timer already fired, so there is nothing to
    // cancel. The task must not be cancelled after its scheduler is
    // destroyed (the hook captures the scheduler by reference).
    task->onComplete([&sched, id](Task& t) {
        if (t.wasCancelled()) sched.cancel(id);
    });
    return task;
}

TaskPtr delayedValueTask(Scheduler& sched, int64_t delayMs, int64_t value) {
    auto task = std::make_shared<Task>();
    std::weak_ptr<Task> weak = task;
    uint64_t id = sched.scheduleAfter(delayMs, [weak, value] {
        if (auto t = weak.lock()) t->complete(value);
    });
    {
        std::lock_guard<std::mutex> lock(task->m_mtx);
        task->m_timerId = id;
    }
    // Phase 6 slice C: see sleepTask — cancelling the task cancels its
    // timer, so no stale timer is left behind.
    task->onComplete([&sched, id](Task& t) {
        if (t.wasCancelled()) sched.cancel(id);
    });
    return task;
}

TaskPtr delayedValueStrTask(Scheduler& sched, int64_t delayMs, const char* value) {
    auto task = std::make_shared<Task>();
    std::weak_ptr<Task> weak = task;
    // Copy now: the caller's pointer need not outlive this call.
    std::string owned = value ? value : "";
    uint64_t id = sched.scheduleAfter(delayMs, [weak, owned] {
        if (auto t = weak.lock()) t->completeStr(owned.c_str());
    });
    {
        std::lock_guard<std::mutex> lock(task->m_mtx);
        task->m_timerId = id;
    }
    // Phase 6 slice C: see sleepTask — cancelling the task cancels its
    // timer, so no stale timer is left behind.
    task->onComplete([&sched, id](Task& t) {
        if (t.wasCancelled()) sched.cancel(id);
    });
    return task;
}

TaskPtr delayedValueDblTask(Scheduler& sched, int64_t delayMs, double value) {
    auto task = std::make_shared<Task>();
    std::weak_ptr<Task> weak = task;
    uint64_t id = sched.scheduleAfter(delayMs, [weak, value] {
        if (auto t = weak.lock()) t->completeDbl(value);
    });
    {
        std::lock_guard<std::mutex> lock(task->m_mtx);
        task->m_timerId = id;
    }
    // Phase 6 slice C: see sleepTask — cancelling the task cancels its
    // timer, so no stale timer is left behind.
    task->onComplete([&sched, id](Task& t) {
        if (t.wasCancelled()) sched.cancel(id);
    });
    return task;
}

int64_t awaitTask(Scheduler& sched, Task& task) {
    // Pump the loop until this task completes. Other timers/tasks fire
    // while this await is outstanding — that is the whole point: the
    // awaiting function makes no progress, but the loop is not blocked.
    // No timeout here: a task that never completes (e.g. its timer was
    // cancelled without cancelling the task — which sleepTask /
    // delayedValueTask never do, since cancel() marks the task done)
    // would pump forever, so runUntil() also bails when no work remains.
    sched.runUntil([&task] { return task.isReady(); });
    return task.value();
}

bool awaitTaskVoid(Scheduler& sched, Task& task) {
    sched.runUntil([&task] { return task.isReady(); });
    return !task.wasCancelled();
}

std::string awaitTaskStr(Scheduler& sched, Task& task) {
    sched.runUntil([&task] { return task.isReady(); });
    return task.strValue();
}

double awaitTaskDbl(Scheduler& sched, Task& task) {
    sched.runUntil([&task] { return task.isReady(); });
    return task.dblValue();
}

void* awaitTaskPtr(Scheduler& sched, Task& task) {
    sched.runUntil([&task] { return task.isReady(); });
    return task.ptrValue();
}

// --- Combinators ------------------------------------------------------------
// Shared state for all/race. Guarded by its own mutex: continuations may
// fire on the pump thread or on a thread completing a task.
namespace {
struct CombinatorState {
    std::mutex mtx;
    TaskPtr result;
    TaskPayload kind{TaskPayload::Void};
    // all(): results collected in input order; remaining inputs.
    void* resultsArray{nullptr};
    size_t remaining{0};
    // all(): input count, for freeing the results array on the cancel
    // path (Phase 6 slice C).
    size_t inputCount{0};
    bool anyCancelled{false};
    // race(): first finisher wins; later finishers are ignored.
    bool settled{false};
};

void completeAllResult(const std::shared_ptr<CombinatorState>& st) {
    // Caller holds st->mtx. Completes (or cancels) the result task.
    if (st->anyCancelled) {
        // Phase 6 slice C: the results array belongs to the combinator
        // until it is handed to the result task via completePtr — on
        // the cancel path it is never handed over, so free it here
        // (plus any string slots already copied in), or every
        // cancelled all() leaks one array. The array is calloc-zeroed
        // by sp_rt_array_alloc, so unfilled slots are null. Note
        // sp_rt_free does NOT decrement g_allocated_bytes (only ARC
        // release paths do), so sp_rt_mem_allocated_bytes() cannot
        // observe this free — it is verified by inspection + the
        // behaviour test below.
        if (st->resultsArray) {
            if (st->kind == TaskPayload::Str) {
                for (size_t i = 0; i < st->inputCount; ++i) {
                    void* slot = ::sp_rt_array_get_ptr(st->resultsArray, static_cast<int64_t>(i), 8);
                    const char* s = *static_cast<const char**>(slot);
                    if (s) ::sp_rt_free(const_cast<char*>(s));
                }
            }
            ::sp_rt_free(st->resultsArray);
            st->resultsArray = nullptr;
        }
        st->result->cancel();
        return;
    }
    switch (st->kind) {
        case TaskPayload::Int:
        case TaskPayload::Str:
        case TaskPayload::Dbl:
            st->result->completePtr(st->resultsArray);
            break;
        case TaskPayload::Void:
            st->result->complete(0);
            break;
    }
}

void storeAllValue(const std::shared_ptr<CombinatorState>& st, size_t index, Task& input) {
    // Caller holds st->mtx. Stores one input's value into the results
    // array (string values are copied: the input task may be destroyed
    // while the array lives on).
    switch (st->kind) {
        case TaskPayload::Int:
            ::sp_rt_array_set_i64(st->resultsArray, static_cast<int64_t>(index), input.value());
            break;
        case TaskPayload::Str: {
            void* slot = ::sp_rt_array_get_ptr(st->resultsArray, static_cast<int64_t>(index), 8);
            *static_cast<const char**>(slot) = copyStrOwned(input.strValue());
            break;
        }
        case TaskPayload::Dbl:
            ::sp_rt_array_set_f64(st->resultsArray, static_cast<int64_t>(index), input.dblValue());
            break;
        case TaskPayload::Void:
            break;
    }
}

void completeRaceResult(const std::shared_ptr<CombinatorState>& st, Task& winner) {
    // Caller holds st->mtx and st->settled is already true.
    if (winner.wasCancelled()) {
        st->result->cancel();
        return;
    }
    switch (st->kind) {
        case TaskPayload::Int:
            st->result->complete(winner.value());
            break;
        case TaskPayload::Str:
            st->result->completeStr(winner.strValue().c_str());
            break;
        case TaskPayload::Dbl:
            st->result->completeDbl(winner.dblValue());
            break;
        case TaskPayload::Void:
            st->result->complete(0);
            break;
    }
}
} // namespace

TaskPtr allTasks(Scheduler& sched, const std::vector<TaskPtr>& tasks, TaskPayload kind) {
    (void)sched; // all() is driven by the inputs' own completions.
    auto result = std::make_shared<Task>();
    auto st = std::make_shared<CombinatorState>();
    st->result = result;
    st->kind = kind;
    st->remaining = tasks.size();
    st->inputCount = tasks.size();
    if (kind == TaskPayload::Int) {
        st->resultsArray = ::sp_rt_array_alloc(static_cast<int64_t>(tasks.size()), 8);
    } else if (kind == TaskPayload::Str) {
        st->resultsArray = ::sp_rt_array_alloc(static_cast<int64_t>(tasks.size()), 8);
    } else if (kind == TaskPayload::Dbl) {
        st->resultsArray = ::sp_rt_array_alloc(static_cast<int64_t>(tasks.size()), 8);
    }
    if (tasks.empty()) {
        completeAllResult(st);
        return result;
    }
    for (size_t i = 0; i < tasks.size(); ++i) {
        TaskPtr input = tasks[i];
        if (!input) {
            // A null handle counts as a cancelled input.
            std::lock_guard<std::mutex> lock(st->mtx);
            st->anyCancelled = true;
            if (--st->remaining == 0) completeAllResult(st);
            continue;
        }
        input->onComplete([st, i](Task& t) {
            std::lock_guard<std::mutex> lock(st->mtx);
            if (t.wasCancelled()) st->anyCancelled = true;
            else storeAllValue(st, i, t);
            if (--st->remaining == 0) completeAllResult(st);
        });
    }
    return result;
}

TaskPtr raceTasks(Scheduler& sched, const std::vector<TaskPtr>& tasks, TaskPayload kind) {
    (void)sched; // race() is driven by the inputs' own completions.
    auto result = std::make_shared<Task>();
    auto st = std::make_shared<CombinatorState>();
    st->result = result;
    st->kind = kind;
    if (tasks.empty()) {
        // No winner can ever arrive: complete cancelled so an await
        // returns promptly with the default value instead of hanging.
        result->cancel();
        return result;
    }
    for (const auto& input : tasks) {
        if (!input) {
            // A null handle counts as a cancelled finisher.
            std::lock_guard<std::mutex> lock(st->mtx);
            if (!st->settled) {
                st->settled = true;
                st->result->cancel();
            }
            continue;
        }
        input->onComplete([st](Task& t) {
            std::lock_guard<std::mutex> lock(st->mtx);
            if (st->settled) return;
            st->settled = true;
            completeRaceResult(st, t);
        });
    }
    return result;
}

} // namespace sp::rt::async
