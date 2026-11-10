#pragma once

#include "sp/runtime/Sqlite.h"
#include <string>
#include <cstdint>
#include <mutex>
#include <memory>

namespace sp::rt::obs {

// Durable job: a row in the `sp_jobs` table of a SQLite database.
// Unlike JobQueue (in-memory only), a PersistentQueue survives a process
// restart: pending jobs are still there when the database is reopened.
// Jobs claimed (status 'processing') when the process died are reset to
// 'pending' on open, so they are retried instead of silently lost.
// There is no in-memory fallback: open() fails with lastError() set when
// the database cannot be opened (bad path, read-only directory, ...),
// and enqueue() returns "" on any write failure.

struct PersistentJob {
    std::string id;
    std::string name;
    std::string payload;
    int64_t attempts{0};
    int64_t maxRetries{3};
    int64_t backoffMs{100};
    int64_t runAtMs{0};
    std::string status{"pending"}; // pending, processing, completed, dead
    std::string lastError;
    std::string createdAt;

    [[nodiscard]] std::string toJson() const;
};

class PersistentQueue {
public:
    PersistentQueue() = default;
    ~PersistentQueue();

    PersistentQueue(const PersistentQueue&) = delete;
    PersistentQueue& operator=(const PersistentQueue&) = delete;

    // Opens (creating if needed) the SQLite file and its schema.
    // Returns false and sets lastError() when the DB cannot be opened
    // or the schema cannot be created.
    bool open(const std::string& path);
    void close();
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] std::string lastError() const;

    // Enqueue durably (committed before returning). Returns the job id,
    // or "" on failure (check lastError()).
    std::string enqueue(const std::string& name, const std::string& payload,
                        int64_t maxRetries = 3, int64_t backoffMs = 100, int64_t delayMs = 0);

    // Claim the oldest due pending job (FIFO by run_at). Returns false
    // when no job is due or on error.
    bool tryDequeue(PersistentJob& out);

    // Mark a claimed job done / failed (failed jobs retry with
    // exponential backoff until maxRetries, then move to the DLQ).
    bool complete(const std::string& jobId);
    bool fail(const std::string& jobId, const std::string& error);

    [[nodiscard]] size_t pendingCount();
    [[nodiscard]] size_t dlqCount();
    bool retryDlqJob(const std::string& jobId);
    void clearDlq();

private:
    bool ensureSchema();
    bool recoverCrashedJobs();

    std::unique_ptr<sp::runtime::SqliteDatabase> m_dbOwned;
    mutable std::mutex m_mutex;
    std::string m_lastError;
    bool m_open{false};
};

} // namespace sp::rt::obs
