#pragma once

#include <string>
#include <vector>
#include <deque>
#include <map>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>
#include <chrono>

namespace sp::rt::obs {

struct Job {
    std::string id;
    std::string name;
    std::string payload;
    int64_t attempts{0};
    int64_t maxRetries{3};
    int64_t backoffMs{100};
    int64_t runAtMs{0};
    std::string status{"pending"}; // pending, processing, completed, failed, dead
    std::string lastError;
    std::string createdAt;

    [[nodiscard]] std::string toJson() const;
};

using JobHandler = std::function<bool(const Job& job, std::string& outError)>;

class JobQueue {
public:
    explicit JobQueue(size_t workerCount = 2);
    ~JobQueue();

    JobQueue(const JobQueue&) = delete;
    JobQueue& operator=(const JobQueue&) = delete;

    void registerHandler(const std::string& name, JobHandler handler);

    std::string enqueue(const std::string& name, const std::string& payload,
                        int64_t maxRetries = 3, int64_t backoffMs = 100, int64_t delayMs = 0);

    void start();
    void stop();
    void waitIdle();

    [[nodiscard]] size_t pendingCount();
    [[nodiscard]] size_t activeCount();
    [[nodiscard]] size_t completedCount();
    [[nodiscard]] size_t dlqCount();

    [[nodiscard]] std::vector<Job> getDlqJobs();
    [[nodiscard]] std::string getDlqJobJson(size_t index);
    bool retryDlqJob(const std::string& jobId);
    void clearDlq();

    static JobQueue& getGlobal();
    static int64_t nowMs();

private:
    void workerLoop(size_t workerId);

    size_t m_workerCount;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_stopping{false};
    std::atomic<int64_t> m_nextJobId{1};

    std::vector<std::thread> m_workers;
    std::map<std::string, JobHandler> m_handlers;

    std::deque<Job> m_pendingJobs; // kept sorted by runAtMs
    std::vector<Job> m_dlq;
    std::mutex m_mutex;
    std::condition_variable m_cvWork;
    std::condition_variable m_cvIdle;

    std::atomic<size_t> m_activeWorkers{0};
    std::atomic<size_t> m_completedJobs{0};
};

struct CronTask {
    std::string name;
    std::string scheduleExpr;
    int64_t intervalMs{0};
    std::function<void()> callback;
    int64_t nextRunMs{0};
};

class CronScheduler {
public:
    CronScheduler();
    ~CronScheduler();

    CronScheduler(const CronScheduler&) = delete;
    CronScheduler& operator=(const CronScheduler&) = delete;

    void scheduleInterval(const std::string& name, int64_t intervalMs, std::function<void()> callback);
    void scheduleCron(const std::string& name, const std::string& cronExpr, std::function<void()> callback);

    void start();
    void stop();
    [[nodiscard]] bool isRunning() const noexcept { return m_running; }
    [[nodiscard]] size_t taskCount();

    static CronScheduler& getGlobal();

private:
    void schedulerLoop();
    static int64_t computeNextCronRunMs(const std::string& expr, int64_t fromMs);

    std::atomic<bool> m_running{false};
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::vector<CronTask> m_tasks;
};

} // namespace sp::rt::obs
