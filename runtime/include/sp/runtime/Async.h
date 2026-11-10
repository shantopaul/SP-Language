#pragma once

// Phase 2 slice B2 (pro-4.0.0): real async runtime.
//
// What this is: a single-threaded, timer-capable scheduler built on top of
// the existing poll()-based EventLoop, plus a small Task (future) type.
//
// What it is NOT: a coroutine / state-machine runtime. There is no heap
// frame splitting here. An async SP function "suspends" at an await point
// by pumping this scheduler (see awaitTask()): its own frame stays on the
// stack, but it makes no progress and the loop keeps doing useful work —
// other timers and queued tasks fire while the await is outstanding.
// A true state-machine split of async functions (locals spilled to a heap
// frame, caller not blocked) is future work; see the B2 doc section.
//
// Threading model: the Scheduler is single-threaded by design — it must
// only be driven from one thread (the event-loop thread). Task state
// itself is mutex/condition-variable protected, so a worker thread may
// complete a Task, but scheduling (scheduleAfter/post/runOnce) is not
// thread-safe and must happen on the loop thread.

#include "sp/runtime/EventLoop.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <deque>

namespace sp::rt::async {

class Scheduler;
class Task;
using TaskPtr = std::shared_ptr<Task>;
TaskPtr sleepTask(Scheduler& sched, int64_t delayMs);
TaskPtr delayedValueTask(Scheduler& sched, int64_t delayMs, int64_t value);

// ---------------------------------------------------------------------------
// Task: a type-erased future. The payload is an int64 (0 for void tasks),
// an optional string payload (Task<string>), a double payload
// (Task<double>), or a pointer payload (Task<T[]> carries an SP array;
// Task<struct> carries a heap-boxed struct the Task owns). A Task is
// complete when done() is true; it may also be cancelled, in which case
// awaiting it returns promptly with the default value (0 / "" / 0.0 /
// null) and wasCancelled() true.
// ---------------------------------------------------------------------------
class Task {
public:
    Task() = default;
    // Frees an owned pointer payload (completePtrOwned box); non-owned
    // payloads (arrays via completePtr) are never freed here.
    ~Task();
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    bool isReady() const;
    bool wasCancelled() const;
    // Value of a completed task (0 when cancelled or for void tasks).
    int64_t value() const;
    // String payload of a completed task (empty when cancelled, for void
    // tasks, or when the task was completed with an int64 value).
    std::string strValue() const;
    // Double payload of a completed task (0.0 when cancelled or when the
    // task has no double payload).
    double dblValue() const;
    // Pointer payload of a completed task (null when cancelled or when
    // the task has no pointer payload). The pointer stays valid while
    // the Task lives; for owned payloads it is freed with the Task.
    void* ptrValue() const;

    // Block the calling thread until the task completes. Only for tests
    // and for code that is NOT running on the loop thread; generated SP
    // code must use awaitTask() (which pumps the loop) instead.
    void wait() const;
    bool waitFor(int64_t timeoutMs) const;

    // Register a continuation, invoked (at most once) when the task
    // completes or is cancelled. If already complete, invoked immediately
    // on the calling thread.
    void onComplete(std::function<void(Task&)> cb);

    // Complete the task with a value. No-op if already complete/cancelled.
    void complete(int64_t v);
    // Complete the task with a string payload. No-op if already done.
    void completeStr(const char* v);
    // Complete the task with a double payload. No-op if already done.
    void completeDbl(double v);
    // Complete the task with a pointer payload (e.g. an SP array). The
    // Task does NOT own the pointer. No-op if already done.
    void completePtr(void* p);
    // Complete the task with a pointer payload the Task OWNS (e.g. a
    // heap-boxed struct return value): the box is freed when the Task
    // is destroyed. No-op if already done.
    void completePtrOwned(void* p);
    // Cancel the task. Returns false if it was already complete.
    // A timer-backed task (sleepTask / delayedValueTask / ...) also
    // cancels its scheduler-side timer (Phase 6 slice C), so no stale
    // timer is left behind. The task must not be cancelled after its
    // scheduler is destroyed.
    bool cancel();

private:
    friend class Scheduler;
    friend TaskPtr sleepTask(Scheduler& sched, int64_t delayMs);
    friend TaskPtr delayedValueTask(Scheduler& sched, int64_t delayMs, int64_t value);
    friend TaskPtr delayedValueStrTask(Scheduler& sched, int64_t delayMs, const char* value);
    friend TaskPtr delayedValueDblTask(Scheduler& sched, int64_t delayMs, double value);
    mutable std::mutex m_mtx;
    mutable std::condition_variable m_cv;
    bool m_done{false};
    bool m_cancelled{false};
    int64_t m_value{0};
    std::string m_strValue;
    double m_dblValue{0.0};
    void* m_ptrValue{nullptr};
    bool m_ownsPtr{false};
    std::vector<std::function<void(Task&)>> m_continuations;
    // Timer id backing this task, if scheduled through a Scheduler.
    uint64_t m_timerId{0};
};

// ---------------------------------------------------------------------------
// Scheduler: owns an EventLoop and adds what it lacks — a timer heap and a
// ready-task queue. runOnce() is one loop iteration: fire due timers,
// run queued callbacks, then poll() sockets with a timeout capped at the
// time of the next timer (or sleep that long when no sockets are watched,
// since EventLoop::poll() returns immediately with no sockets).
// ---------------------------------------------------------------------------
class Scheduler {
public:
    Scheduler();
    ~Scheduler();

    Scheduler(const Scheduler&) = delete;
    Scheduler& operator=(const Scheduler&) = delete;

    // Schedule cb to run after delayMs (>= 0; 0 runs on the next pump).
    // Returns a timer id usable with cancel().
    uint64_t scheduleAfter(int64_t delayMs, std::function<void()> cb);
    // Queue cb to run on the next pump, before any not-yet-due timer.
    void post(std::function<void()> cb);
    // Cancel a pending timer. Returns false if already fired/cancelled.
    bool cancel(uint64_t timerId);

    // Socket I/O through the underlying EventLoop. cb is invoked from
    // runOnce() when the socket becomes ready (events: SP_EVENT_READ etc).
    bool addSocket(sp_socket_handle_t sock, int64_t events, std::function<void(int64_t)> cb);
    bool removeSocket(sp_socket_handle_t sock);

    // One loop iteration. Returns true if any timer/task/socket callback ran.
    bool runOnce();
    // Pump until no timers, queued tasks or watched sockets remain.
    // (A watched socket never drains on its own; callers that register
    // sockets should use runUntil()/runFor() instead.)
    void runUntilIdle();
    // Pump until done() is true or timeoutMs elapses (< 0: no timeout,
    // but returns false if there is no pending work left). Returns true
    // iff done() became true.
    bool runUntil(const std::function<bool()>& done, int64_t timeoutMs = -1);
    // Pump for at most durationMs of wall time.
    void runFor(int64_t durationMs);
    // Pump until no timers or queued tasks remain, IGNORING watched
    // sockets. Unlike runUntilIdle() this cannot block forever on a
    // registered socket: it is the drain the compiler emits at the end
    // of `main` so fire-and-forget async tasks finish before the
    // process exits. A task that reschedules itself forever still
    // never drains; that is a program bug, not a scheduler bug.
    void runUntilTasksIdle();

    bool hasPending() const;
    size_t pendingCount() const;
    int64_t nowMs() const;

    net::EventLoop& eventLoop() noexcept { return m_loop; }

private:
    struct Timer {
        std::chrono::steady_clock::time_point due;
        uint64_t id;
        std::function<void()> cb;
        // Min-heap by due time (earliest first), then id for determinism.
        bool operator<(const Timer& other) const {
            if (due != other.due) return due > other.due;
            return id > other.id;
        }
    };

    void fireDueTimers();
    // Drop cancelled timers sitting at the head of the heap. A cancelled
    // timer never fires, so removing it early changes nothing observable;
    // it keeps msUntilNextTimer() from seeing a dead timer as the next
    // deadline. Loop thread only (the heap is never touched off-thread).
    void pruneCancelledHead();
    int64_t msUntilNextTimer(); // -1 when no timers pending
    // One iteration, waiting at most maxWaitMs (< 0: no cap). runOnce()
    // is runOnceCapped(-1); runUntil()/runFor() pass their deadline so
    // a long timer cannot overshoot the caller's bound.
    bool runOnceCapped(int64_t maxWaitMs);

    net::EventLoop m_loop;
    std::priority_queue<Timer> m_timers;
    std::deque<std::function<void()>> m_ready;
    std::unordered_set<uint64_t> m_cancelledTimers;
    // Ids of timers currently in the heap that are still live (not
    // cancelled, not yet fired). Lets hasPending()/pendingCount() answer
    // in O(1) without copying the heap. Guarded by m_cancelMtx together
    // with m_cancelledTimers: cancel() may run off the loop thread.
    std::unordered_set<uint64_t> m_pendingTimerIds;
    // Phase 6 slice C: guards m_cancelledTimers. cancel() may run off
    // the loop thread (Task::cancel() invokes the timer-cancel hook
    // from whichever thread cancels the task), while fireDueTimers() /
    // msUntilNextTimer() read the set on the loop thread.
    mutable std::mutex m_cancelMtx;
    std::unordered_map<sp_socket_handle_t, std::function<void(int64_t)>> m_socketCallbacks;
    uint64_t m_nextTimerId{1};
};

// Process-wide scheduler used by generated async code
// (sp_rt_async_default_scheduler()). Intentionally never destroyed:
// generated code may await during shutdown paths.
Scheduler& defaultScheduler();

// Create a task that completes (value 0) after delayMs on this scheduler.
// Non-blocking: returns immediately; the timer fires on the loop.
TaskPtr sleepTask(Scheduler& sched, int64_t delayMs);
// Create a task that completes with `value` after delayMs.
TaskPtr delayedValueTask(Scheduler& sched, int64_t delayMs, int64_t value);
// Create a task that completes with a string `value` after delayMs.
// The value is copied into the Task; the caller's pointer need not
// outlive the call.
TaskPtr delayedValueStrTask(Scheduler& sched, int64_t delayMs, const char* value);
// Create a task that completes with a double `value` after delayMs.
TaskPtr delayedValueDblTask(Scheduler& sched, int64_t delayMs, double value);

// Pump `sched` until `task` completes or is cancelled, then return its
// value (0 when cancelled). This is the await primitive the compiler
// lowers `await` to: the awaiting function makes no progress while the
// scheduler runs other timers/tasks in the meantime.
int64_t awaitTask(Scheduler& sched, Task& task);
// As awaitTask(), but returns false when the task was cancelled.
bool awaitTaskVoid(Scheduler& sched, Task& task);
// As awaitTask(), returning the task's string payload (empty when
// cancelled or when the task has no string payload).
std::string awaitTaskStr(Scheduler& sched, Task& task);
// As awaitTask(), returning the task's double payload (0.0 when
// cancelled or when the task has no double payload).
double awaitTaskDbl(Scheduler& sched, Task& task);
// As awaitTask(), returning the task's pointer payload (null when
// cancelled or when the task has no pointer payload).
void* awaitTaskPtr(Scheduler& sched, Task& task);

// ---------------------------------------------------------------------------
// Combinators (Phase 5 slice A). Payload kind of the collected / winning
// value. `all` waits for every input and completes with the collected
// values (an SP array for Int/Str/Dbl, void for Void); it completes
// CANCELLED when any input was cancelled (or when a null handle is
// passed). `race` completes with the FIRST finisher's value and completes
// CANCELLED when the first finisher was cancelled; losing inputs keep
// running (they are NOT cancelled). An empty input completes immediately
// (all: empty array / void; race: cancelled — no winner can arrive).
// The returned Task is owned by the caller.
// ---------------------------------------------------------------------------
enum class TaskPayload { Int, Str, Dbl, Void };
TaskPtr allTasks(Scheduler& sched, const std::vector<TaskPtr>& tasks, TaskPayload kind);
TaskPtr raceTasks(Scheduler& sched, const std::vector<TaskPtr>& tasks, TaskPayload kind);

} // namespace sp::rt::async
