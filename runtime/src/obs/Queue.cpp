#include "sp/runtime/Queue.h"
#include "sp/runtime/PersistentQueue.h"
#include "sp/runtime/Crypto.h"
#include "sp/runtime/Log.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <ctime>

namespace sp::rt::obs {

namespace {

std::string escapeJson(const std::string& s) {
    std::ostringstream o;
    for (char c : s) {
        switch (c) {
            case '"':  o << "\\\""; break;
            case '\\': o << "\\\\"; break;
            case '\b': o << "\\b";  break;
            case '\f': o << "\\f";  break;
            case '\n': o << "\\n";  break;
            case '\r': o << "\\r";  break;
            case '\t': o << "\\t";  break;
            default:
                if (static_cast<unsigned char>(c) <= 0x1f) {
                    o << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                } else {
                    o << c;
                }
        }
    }
    return o.str();
}

bool matchCronField(const std::string& field, int value, int minVal, int maxVal) {
    (void)minVal;
    (void)maxVal;
    if (field == "*") return true;

    // Check step: */N
    if (field.rfind("*/", 0) == 0) {
        int step = std::stoi(field.substr(2));
        if (step <= 0) return true;
        return (value % step) == 0;
    }

    // Check comma separated: e.g. 1,5,10
    std::stringstream ss(field);
    std::string item;
    while (std::getline(ss, item, ',')) {
        if (!item.empty()) {
            try {
                int expected = std::stoi(item);
                if (expected == value) return true;
            } catch (...) {}
        }
    }
    return false;
}

} // namespace

std::string Job::toJson() const {
    std::ostringstream ss;
    ss << "{\"id\":\"" << escapeJson(id) << "\","
       << "\"name\":\"" << escapeJson(name) << "\","
       << "\"payload\":\"" << escapeJson(payload) << "\","
       << "\"attempts\":" << attempts << ","
       << "\"maxRetries\":" << maxRetries << ","
       << "\"backoffMs\":" << backoffMs << ","
       << "\"status\":\"" << escapeJson(status) << "\","
       << "\"lastError\":\"" << escapeJson(lastError) << "\","
       << "\"createdAt\":\"" << escapeJson(createdAt) << "\"}";
    return ss.str();
}

int64_t JobQueue::nowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

JobQueue::JobQueue(size_t workerCount)
    : m_workerCount(workerCount == 0 ? 1 : workerCount) {
    start();
}

JobQueue::~JobQueue() {
    stop();
}

void JobQueue::start() {
    if (m_running.exchange(true)) return;
    m_stopping = false;

    m_workers.reserve(m_workerCount);
    for (size_t i = 0; i < m_workerCount; ++i) {
        m_workers.emplace_back(&JobQueue::workerLoop, this, i);
    }
}

void JobQueue::stop() {
    if (!m_running) return;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopping = true;
    }
    m_cvWork.notify_all();

    for (auto& t : m_workers) {
        if (t.joinable()) {
            t.join();
        }
    }
    m_workers.clear();
    m_running = false;
}

void JobQueue::waitIdle() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cvIdle.wait(lock, [this]() {
        return m_pendingJobs.empty() && m_activeWorkers == 0;
    });
}

void JobQueue::registerHandler(const std::string& name, JobHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[name] = std::move(handler);
}

std::string JobQueue::enqueue(const std::string& name, const std::string& payload,
                              int64_t maxRetries, int64_t backoffMs, int64_t delayMs) {
    Job job;
    job.id = "job_" + std::to_string(m_nextJobId.fetch_add(1));
    job.name = name;
    job.payload = payload;
    job.attempts = 0;
    job.maxRetries = maxRetries;
    job.backoffMs = backoffMs;
    job.runAtMs = nowMs() + delayMs;
    job.status = "pending";
    job.createdAt = currentIsoTimestamp();

    std::string id = job.id;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = std::lower_bound(m_pendingJobs.begin(), m_pendingJobs.end(), job,
            [](const Job& a, const Job& b) { return a.runAtMs < b.runAtMs; });
        m_pendingJobs.insert(it, std::move(job));
    }
    m_cvWork.notify_one();
    return id;
}

void JobQueue::workerLoop(size_t workerId) {
    (void)workerId;
    while (!m_stopping) {
        Job job;
        JobHandler handler = nullptr;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            while (!m_stopping && (m_pendingJobs.empty() || m_pendingJobs.front().runAtMs > nowMs())) {
                if (m_pendingJobs.empty()) {
                    m_cvWork.wait(lock);
                } else {
                    int64_t waitTime = m_pendingJobs.front().runAtMs - nowMs();
                    if (waitTime > 0) {
                        m_cvWork.wait_for(lock, std::chrono::milliseconds(waitTime));
                    }
                }
            }
            if (m_stopping) break;
            if (m_pendingJobs.empty() || m_pendingJobs.front().runAtMs > nowMs()) continue;

            job = std::move(m_pendingJobs.front());
            m_pendingJobs.pop_front();
            m_activeWorkers++;

            auto hit = m_handlers.find(job.name);
            if (hit != m_handlers.end()) {
                handler = hit->second;
            }
        }

        job.status = "processing";
        bool success = false;
        std::string err;

        if (handler) {
            try {
                success = handler(job, err);
            } catch (const std::exception& e) {
                err = e.what();
                success = false;
            } catch (...) {
                err = "job execution failed with unknown exception";
                success = false;
            }
        } else {
            err = "no handler registered for job: " + job.name;
            success = false;
        }

        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (success) {
                job.status = "completed";
                m_completedJobs++;
            } else {
                job.attempts++;
                job.lastError = err;
                if (job.attempts < job.maxRetries) {
                    int64_t factor = (job.attempts <= 30) ? (1LL << (job.attempts - 1)) : (1LL << 30);
                    int64_t delay = job.backoffMs * factor;
                    job.runAtMs = nowMs() + delay;
                    job.status = "pending";

                    auto it = std::lower_bound(m_pendingJobs.begin(), m_pendingJobs.end(), job,
                        [](const Job& a, const Job& b) { return a.runAtMs < b.runAtMs; });
                    m_pendingJobs.insert(it, std::move(job));
                    m_cvWork.notify_all();
                } else {
                    job.status = "dead";
                    m_dlq.push_back(std::move(job));
                }
            }
            m_activeWorkers--;
            if (m_pendingJobs.empty() && m_activeWorkers == 0) {
                m_cvIdle.notify_all();
            }
        }
    }
}

size_t JobQueue::pendingCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_pendingJobs.size();
}

size_t JobQueue::activeCount() {
    return m_activeWorkers.load();
}

size_t JobQueue::completedCount() {
    return m_completedJobs.load();
}

size_t JobQueue::dlqCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dlq.size();
}

std::vector<Job> JobQueue::getDlqJobs() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dlq;
}

std::string JobQueue::getDlqJobJson(size_t index) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (index < m_dlq.size()) {
        return m_dlq[index].toJson();
    }
    return "{}";
}

bool JobQueue::retryDlqJob(const std::string& jobId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    for (auto it = m_dlq.begin(); it != m_dlq.end(); ++it) {
        if (it->id == jobId) {
            Job job = std::move(*it);
            m_dlq.erase(it);

            job.attempts = 0;
            job.lastError.clear();
            job.status = "pending";
            job.runAtMs = nowMs();

            auto pit = std::lower_bound(m_pendingJobs.begin(), m_pendingJobs.end(), job,
                [](const Job& a, const Job& b) { return a.runAtMs < b.runAtMs; });
            m_pendingJobs.insert(pit, std::move(job));
            m_cvWork.notify_one();
            return true;
        }
    }
    return false;
}

void JobQueue::clearDlq() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_dlq.clear();
}

static JobQueue s_globalQueue;

JobQueue& JobQueue::getGlobal() {
    return s_globalQueue;
}

// ----------------------------------------------------------------------------
// CronScheduler
// ----------------------------------------------------------------------------
CronScheduler::CronScheduler() = default;

CronScheduler::~CronScheduler() {
    stop();
}

void CronScheduler::scheduleInterval(const std::string& name, int64_t intervalMs, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    CronTask task;
    task.name = name;
    task.intervalMs = (intervalMs <= 0 ? 1000 : intervalMs);
    task.callback = std::move(callback);
    task.nextRunMs = JobQueue::nowMs() + task.intervalMs;
    m_tasks.push_back(std::move(task));
    m_cv.notify_one();
}

void CronScheduler::scheduleCron(const std::string& name, const std::string& cronExpr, std::function<void()> callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    CronTask task;
    task.name = name;
    task.scheduleExpr = cronExpr;
    task.callback = std::move(callback);
    task.nextRunMs = computeNextCronRunMs(cronExpr, JobQueue::nowMs());
    m_tasks.push_back(std::move(task));
    m_cv.notify_one();
}

void CronScheduler::start() {
    if (m_running.exchange(true)) return;
    m_thread = std::thread(&CronScheduler::schedulerLoop, this);
}

void CronScheduler::stop() {
    if (!m_running) return;
    m_running = false;
    m_cv.notify_all();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

size_t CronScheduler::taskCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_tasks.size();
}

int64_t CronScheduler::computeNextCronRunMs(const std::string& expr, int64_t fromMs) {
    // Parse standard 5-part cron: min hour dom month dow
    std::vector<std::string> parts;
    std::stringstream ss(expr);
    std::string p;
    while (ss >> p) {
        parts.push_back(p);
    }
    if (parts.size() != 5) {
        // Fallback: 60s
        return fromMs + 60000;
    }

    std::time_t t = static_cast<std::time_t>((fromMs / 1000) + 60); // start from next minute
    for (int attempts = 0; attempts < 60 * 24 * 7; ++attempts) { // up to 7 days ahead
        std::tm gmt{};
#if defined(_WIN32)
        gmtime_s(&gmt, &t);
#else
        gmtime_r(&t, &gmt);
#endif
        if (matchCronField(parts[0], gmt.tm_min, 0, 59) &&
            matchCronField(parts[1], gmt.tm_hour, 0, 23) &&
            matchCronField(parts[2], gmt.tm_mday, 1, 31) &&
            matchCronField(parts[3], gmt.tm_mon + 1, 1, 12) &&
            matchCronField(parts[4], gmt.tm_wday, 0, 6)) {
            gmt.tm_sec = 0;
#if defined(_WIN32)
            std::time_t target = _mkgmtime(&gmt);
#else
            std::time_t target = timegm(&gmt);
#endif
            return static_cast<int64_t>(target) * 1000;
        }
        t += 60; // advance by one minute
    }

    return fromMs + 60000;
}

void CronScheduler::schedulerLoop() {
    while (m_running) {
        int64_t nextWakeMs = JobQueue::nowMs() + 1000;
        std::vector<std::function<void()>> toRun;

        {
            std::unique_lock<std::mutex> lock(m_mutex);
            int64_t now = JobQueue::nowMs();
            for (auto& task : m_tasks) {
                if (now >= task.nextRunMs) {
                    if (task.callback) {
                        toRun.push_back(task.callback);
                    }
                    if (task.intervalMs > 0) {
                        task.nextRunMs = now + task.intervalMs;
                    } else if (!task.scheduleExpr.empty()) {
                        task.nextRunMs = computeNextCronRunMs(task.scheduleExpr, now);
                    }
                }
                if (task.nextRunMs < nextWakeMs) {
                    nextWakeMs = task.nextRunMs;
                }
            }

            int64_t sleepMs = nextWakeMs - now;
            if (sleepMs < 10) sleepMs = 10;
            if (sleepMs > 500) sleepMs = 500;
            m_cv.wait_for(lock, std::chrono::milliseconds(sleepMs), [this]() { return !m_running; });
        }

        for (auto& cb : toRun) {
            try {
                cb();
            } catch (...) {}
        }
    }
}

static CronScheduler s_globalCron;

CronScheduler& CronScheduler::getGlobal() {
    return s_globalCron;
}

// ----------------------------------------------------------------------------
// PersistentQueue (SQLite-backed durable jobs)
// ----------------------------------------------------------------------------

std::string PersistentJob::toJson() const {
    std::ostringstream ss;
    ss << "{\"id\":\"" << escapeJson(id) << "\","
       << "\"name\":\"" << escapeJson(name) << "\","
       << "\"payload\":\"" << escapeJson(payload) << "\","
       << "\"attempts\":" << attempts << ","
       << "\"maxRetries\":" << maxRetries << ","
       << "\"backoffMs\":" << backoffMs << ","
       << "\"status\":\"" << escapeJson(status) << "\","
       << "\"lastError\":\"" << escapeJson(lastError) << "\","
       << "\"createdAt\":\"" << escapeJson(createdAt) << "\"}";
    return ss.str();
}

PersistentQueue::~PersistentQueue() {
    close();
}

bool PersistentQueue::open(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    close();
    m_lastError.clear();
    if (path.empty()) {
        m_lastError = "empty database path";
        return false;
    }
    m_dbOwned = std::make_unique<sp::runtime::SqliteDatabase>();
    if (!m_dbOwned->open(path)) {
        m_lastError = m_dbOwned->lastError();
        if (m_lastError.empty()) m_lastError = "failed to open sqlite database: " + path;
        m_dbOwned.reset();
        return false;
    }
    m_open = true;
    if (!ensureSchema() || !recoverCrashedJobs()) {
        std::string err = m_lastError;
        close();
        m_lastError = err;
        return false;
    }
    return true;
}

void PersistentQueue::close() {
    if (m_dbOwned) {
        m_dbOwned->close();
        m_dbOwned.reset();
    }
    m_open = false;
}

bool PersistentQueue::isOpen() const {
    return m_open;
}

std::string PersistentQueue::lastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

bool PersistentQueue::ensureSchema() {
    int rc = m_dbOwned->execute(
        "CREATE TABLE IF NOT EXISTS sp_jobs ("
        "  id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  payload TEXT NOT NULL DEFAULT '',"
        "  attempts INTEGER NOT NULL DEFAULT 0,"
        "  max_retries INTEGER NOT NULL DEFAULT 3,"
        "  backoff_ms INTEGER NOT NULL DEFAULT 100,"
        "  run_at_ms INTEGER NOT NULL,"
        "  status TEXT NOT NULL DEFAULT 'pending',"
        "  last_error TEXT NOT NULL DEFAULT '',"
        "  created_at TEXT NOT NULL DEFAULT ''"
        ");");
    if (rc < 0) {
        m_lastError = m_dbOwned->lastError();
        return false;
    }
    rc = m_dbOwned->execute(
        "CREATE INDEX IF NOT EXISTS sp_jobs_pending_idx ON sp_jobs(status, run_at_ms);");
    if (rc < 0) {
        m_lastError = m_dbOwned->lastError();
        return false;
    }
    return true;
}

bool PersistentQueue::recoverCrashedJobs() {
    // Jobs left 'processing' by a crashed process are retried, not lost.
    int rc = m_dbOwned->execute(
        "UPDATE sp_jobs SET status='pending', run_at_ms=" +
        std::to_string(JobQueue::nowMs()) +
        " WHERE status='processing';");
    if (rc < 0) {
        m_lastError = m_dbOwned->lastError();
        return false;
    }
    return true;
}

std::string PersistentQueue::enqueue(const std::string& name, const std::string& payload,
                                     int64_t maxRetries, int64_t backoffMs, int64_t delayMs) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) {
        m_lastError = "persistent queue is not open";
        return "";
    }
    std::string id = "pjob_" + sp::rt::crypto::randomUuid();
    auto stmt = m_dbOwned->prepare(
        "INSERT INTO sp_jobs (id, name, payload, attempts, max_retries, backoff_ms, run_at_ms, status, last_error, created_at)"
        " VALUES (?, ?, ?, 0, ?, ?, ?, 'pending', '', ?);");
    if (!stmt) {
        m_lastError = m_dbOwned->lastError();
        return "";
    }
    stmt->bindString(1, id);
    stmt->bindString(2, name);
    stmt->bindString(3, payload);
    stmt->bindInt(4, maxRetries < 0 ? 0 : maxRetries);
    stmt->bindInt(5, backoffMs < 0 ? 0 : backoffMs);
    stmt->bindInt(6, JobQueue::nowMs() + delayMs);
    stmt->bindString(7, currentIsoTimestamp());
    if (stmt->step() != 101) { // SQLITE_DONE
        m_lastError = m_dbOwned->lastError();
        return "";
    }
    return id;
}

bool PersistentQueue::tryDequeue(PersistentJob& out) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) {
        m_lastError = "persistent queue is not open";
        return false;
    }
    if (!m_dbOwned->beginTransaction()) {
        m_lastError = m_dbOwned->lastError();
        return false;
    }
    bool ok = false;
    std::string jobId;
    {
        auto pick = m_dbOwned->prepare(
            "SELECT id FROM sp_jobs WHERE status='pending' AND run_at_ms <= ?"
            " ORDER BY run_at_ms ASC, rowid ASC LIMIT 1;");
        if (pick) {
            pick->bindInt(1, JobQueue::nowMs());
            if (pick->step() == 100) { // SQLITE_ROW
                jobId = pick->columnString(0);
            }
        }
    }
    if (!jobId.empty()) {
        auto claim = m_dbOwned->prepare(
            "UPDATE sp_jobs SET status='processing' WHERE id=? AND status='pending';");
        if (claim) {
            claim->bindString(1, jobId);
            claim->step();
            ok = m_dbOwned->changes() == 1;
        }
    }
    if (ok) {
        auto row = m_dbOwned->prepare(
            "SELECT id, name, payload, attempts, max_retries, backoff_ms, run_at_ms, status, last_error, created_at"
            " FROM sp_jobs WHERE id=?;");
        if (row) {
            row->bindString(1, jobId);
            if (row->step() == 100) {
                out.id = row->columnString(0);
                out.name = row->columnString(1);
                out.payload = row->columnString(2);
                out.attempts = row->columnInt(3);
                out.maxRetries = row->columnInt(4);
                out.backoffMs = row->columnInt(5);
                out.runAtMs = row->columnInt(6);
                out.status = row->columnString(7);
                out.lastError = row->columnString(8);
                out.createdAt = row->columnString(9);
            } else {
                ok = false;
            }
        } else {
            ok = false;
        }
    }
    if (ok) {
        if (!m_dbOwned->commit()) {
            m_lastError = m_dbOwned->lastError();
            return false;
        }
        return true;
    }
    m_dbOwned->rollback();
    return false;
}

bool PersistentQueue::complete(const std::string& jobId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return false;
    auto stmt = m_dbOwned->prepare(
        "UPDATE sp_jobs SET status='completed' WHERE id=? AND status='processing';");
    if (!stmt) return false;
    stmt->bindString(1, jobId);
    if (stmt->step() != 101) return false;
    return m_dbOwned->changes() == 1;
}

bool PersistentQueue::fail(const std::string& jobId, const std::string& error) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return false;
    auto row = m_dbOwned->prepare(
        "SELECT attempts, max_retries, backoff_ms FROM sp_jobs WHERE id=?;");
    if (!row) return false;
    row->bindString(1, jobId);
    if (row->step() != 100) return false;
    int64_t attempts = row->columnInt(0) + 1;
    int64_t maxRetries = row->columnInt(1);
    int64_t backoffMs = row->columnInt(2);
    bool dead = attempts >= maxRetries;
    int64_t factor = (attempts <= 30) ? (1LL << (attempts - 1)) : (1LL << 30);
    int64_t nextRun = JobQueue::nowMs() + backoffMs * factor;
    auto upd = m_dbOwned->prepare(
        dead ? "UPDATE sp_jobs SET attempts=?, status='dead', last_error=?, run_at_ms=? WHERE id=?;"
             : "UPDATE sp_jobs SET attempts=?, status='pending', last_error=?, run_at_ms=? WHERE id=?;");
    if (!upd) return false;
    upd->bindInt(1, attempts);
    upd->bindString(2, error);
    upd->bindInt(3, nextRun);
    upd->bindString(4, jobId);
    if (upd->step() != 101) return false;
    return m_dbOwned->changes() == 1;
}

size_t PersistentQueue::pendingCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return 0;
    auto stmt = m_dbOwned->prepare("SELECT COUNT(*) FROM sp_jobs WHERE status='pending';");
    if (!stmt || stmt->step() != 100) return 0;
    return static_cast<size_t>(stmt->columnInt(0));
}

size_t PersistentQueue::dlqCount() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return 0;
    auto stmt = m_dbOwned->prepare("SELECT COUNT(*) FROM sp_jobs WHERE status='dead';");
    if (!stmt || stmt->step() != 100) return 0;
    return static_cast<size_t>(stmt->columnInt(0));
}

bool PersistentQueue::retryDlqJob(const std::string& jobId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return false;
    auto stmt = m_dbOwned->prepare(
        "UPDATE sp_jobs SET attempts=0, status='pending', last_error='', run_at_ms=? WHERE id=? AND status='dead';");
    if (!stmt) return false;
    stmt->bindInt(1, JobQueue::nowMs());
    stmt->bindString(2, jobId);
    if (stmt->step() != 101) return false;
    return m_dbOwned->changes() == 1;
}

void PersistentQueue::clearDlq() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return;
    m_dbOwned->execute("DELETE FROM sp_jobs WHERE status='dead';");
}

} // namespace sp::rt::obs
