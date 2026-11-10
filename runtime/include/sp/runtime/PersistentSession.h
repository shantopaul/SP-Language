#pragma once

#include "sp/runtime/Sqlite.h"
#include <string>
#include <cstdint>
#include <mutex>
#include <memory>

namespace sp::rt::crypto {

// Durable sessions: the same key/value + expiry semantics as the
// in-memory SessionStore, but rows live in a SQLite `sp_sessions` table
// so sessions survive a process restart. There is no in-memory fallback:
// open() fails with lastError() set when the database cannot be opened,
// and create()/set() return failure (""/false) on any write error.
// Expired sessions are never returned by get() (and are deleted lazily).

class PersistentSessionStore {
public:
    explicit PersistentSessionStore(int64_t defaultTtlSeconds = 3600);
    ~PersistentSessionStore();

    PersistentSessionStore(const PersistentSessionStore&) = delete;
    PersistentSessionStore& operator=(const PersistentSessionStore&) = delete;

    // Opens (creating if needed) the SQLite file and its schema.
    bool open(const std::string& path);
    void close();
    [[nodiscard]] bool isOpen() const;
    [[nodiscard]] std::string lastError() const;

    // Create a session, returning a random session ID, or "" on failure.
    // ttlSeconds > 0 uses that TTL; otherwise the store default applies.
    // A resolved TTL <= 0 means the session never expires.
    std::string create(const std::string& dataJson = "{}", int64_t ttlSeconds = -1);

    // Returns "" when the session is missing or expired.
    std::string get(const std::string& sessionId);

    // Update data and renew the TTL. Returns false when missing/expired.
    bool set(const std::string& sessionId, const std::string& dataJson, int64_t ttlSeconds = -1);

    bool destroy(const std::string& sessionId);

    // New random ID for the same data (session-fixation protection).
    std::string regenerate(const std::string& oldSessionId, int64_t ttlSeconds = -1);

    void pruneExpired();
    [[nodiscard]] size_t count();

private:
    bool ensureSchema();
    int64_t resolveExpiryMs(int64_t ttlSeconds) const;

    int64_t m_defaultTtlSeconds;
    std::unique_ptr<sp::runtime::SqliteDatabase> m_dbOwned;
    mutable std::mutex m_mutex;
    std::string m_lastError;
    bool m_open{false};
};

} // namespace sp::rt::crypto
