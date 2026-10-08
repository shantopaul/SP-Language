#include "sp/runtime/ThreadPool.h"
#include <algorithm>

namespace sp::rt {

static thread_local size_t t_worker_id = static_cast<size_t>(-1);

ThreadPool::ThreadPool(int64_t num_threads) {
    if (num_threads <= 0) {
        num_threads = static_cast<int64_t>(std::thread::hardware_concurrency());
        if (num_threads <= 0) {
            num_threads = 4;
        }
    }

    size_t count = static_cast<size_t>(num_threads);
    m_workers.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        m_workers.push_back(std::make_unique<Worker>());
    }

    for (size_t i = 0; i < count; ++i) {
        m_workers[i]->thread = std::thread(&ThreadPool::worker_loop, this, i);
    }
}

ThreadPool::~ThreadPool() {
    destroy();
}

void ThreadPool::submit(void (*fn)(void*), void* arg) {
    if (!fn || m_shutdown.load()) return;

    ThreadPoolTask task{fn, arg};
    m_active_tasks.fetch_add(1, std::memory_order_seq_cst);

    if (t_worker_id < m_workers.size()) {
        // Internal worker submitting work: push to its own deque (LIFO)
        auto& worker = m_workers[t_worker_id];
        {
            std::lock_guard<std::mutex> lock(worker->deque_mtx);
            worker->deque.push_back(task);
        }
    } else {
        // External thread submitting work: round-robin distribute to workers
        size_t idx = m_next_worker.fetch_add(1, std::memory_order_relaxed) % m_workers.size();
        auto& worker = m_workers[idx];
        {
            std::lock_guard<std::mutex> lock(worker->deque_mtx);
            worker->deque.push_back(task);
        }
    }

    m_cv_work.notify_one();
}

bool ThreadPool::try_steal(size_t thief_id, ThreadPoolTask& out_task) {
    size_t n = m_workers.size();
    if (n <= 1) return false;

    // Steal from other workers round-robin from top (FIFO)
    for (size_t i = 1; i < n; ++i) {
        size_t victim_id = (thief_id + i) % n;
        auto& victim = m_workers[victim_id];
        std::unique_lock<std::mutex> lock(victim->deque_mtx, std::try_to_lock);
        if (lock.owns_lock() && !victim->deque.empty()) {
            out_task = victim->deque.front();
            victim->deque.pop_front();
            return true;
        }
    }
    return false;
}

void ThreadPool::worker_loop(size_t worker_id) {
    t_worker_id = worker_id;
    auto& self = m_workers[worker_id];

    while (!m_shutdown.load()) {
        ThreadPoolTask task{nullptr, nullptr};
        bool has_task = false;

        // 1. Check local deque (LIFO: pop back for cache locality)
        {
            std::lock_guard<std::mutex> lock(self->deque_mtx);
            if (!self->deque.empty()) {
                task = self->deque.back();
                self->deque.pop_back();
                has_task = true;
            }
        }

        // 2. Check global queue
        if (!has_task) {
            std::lock_guard<std::mutex> lock(m_global_mtx);
            if (!m_global_queue.empty()) {
                task = m_global_queue.front();
                m_global_queue.pop_front();
                has_task = true;
            }
        }

        // 3. Try to steal from other workers (FIFO: pop front)
        if (!has_task) {
            has_task = try_steal(worker_id, task);
        }

        // 4. If work found, execute it
        if (has_task && task.fn) {
            task.fn(task.arg);
            if (m_active_tasks.fetch_sub(1, std::memory_order_seq_cst) == 1) {
                std::lock_guard<std::mutex> lock(m_global_mtx);
                m_cv_idle.notify_all();
            }
            continue;
        }

        // 5. If no work, wait on condition variable
        std::unique_lock<std::mutex> lock(m_global_mtx);
        m_cv_work.wait_for(lock, std::chrono::milliseconds(2), [this, &self]() {
            if (m_shutdown.load()) return true;
            if (!m_global_queue.empty()) return true;
            std::lock_guard<std::mutex> lk(self->deque_mtx);
            return !self->deque.empty();
        });
    }
}

void ThreadPool::wait_idle() {
    std::unique_lock<std::mutex> lock(m_global_mtx);
    m_cv_idle.wait(lock, [this]() {
        return m_active_tasks.load(std::memory_order_seq_cst) == 0;
    });
}

int64_t ThreadPool::thread_count() const {
    return static_cast<int64_t>(m_workers.size());
}

void ThreadPool::destroy() {
    if (m_shutdown.exchange(true)) {
        return;
    }

    m_cv_work.notify_all();

    for (auto& worker : m_workers) {
        if (worker->thread.joinable()) {
            worker->thread.join();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_global_mtx);
        m_global_queue.clear();
        m_active_tasks.store(0);
        m_cv_idle.notify_all();
    }
}

} // namespace sp::rt
