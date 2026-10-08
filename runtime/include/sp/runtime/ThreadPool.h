#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

namespace sp::rt {

struct ThreadPoolTask {
    void (*fn)(void*){nullptr};
    void* arg{nullptr};
};

class ThreadPool {
public:
    explicit ThreadPool(int64_t num_threads = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    // Submit a C-style task to the pool
    void submit(void (*fn)(void*), void* arg);

    // Wait until all queued and executing tasks are completed
    void wait_idle();

    // Number of worker threads
    int64_t thread_count() const;

    // Shut down workers and wait for threads to exit
    void destroy();

private:
    struct Worker {
        std::thread thread;
        std::deque<ThreadPoolTask> deque;
        std::mutex deque_mtx;
    };

    void worker_loop(size_t worker_id);
    bool try_steal(size_t thief_id, ThreadPoolTask& out_task);

    std::vector<std::unique_ptr<Worker>> m_workers;
    std::deque<ThreadPoolTask> m_global_queue;
    std::mutex m_global_mtx;
    std::condition_variable m_cv_work;
    std::condition_variable m_cv_idle;

    std::atomic<bool> m_shutdown{false};
    std::atomic<int64_t> m_active_tasks{0};
    std::atomic<size_t> m_next_worker{0};
};

} // namespace sp::rt
