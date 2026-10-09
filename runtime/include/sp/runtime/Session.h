#pragma once

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdint>

namespace sp::rt::crypto {

class Cookie {
public:
    std::string name;
    std::string value;
    std::string domain;
    std::string path = "/";
    int64_t maxAge = 0; // seconds, 0 = session
    bool secure = false;
    bool httpOnly = true;
    std::string sameSite = "Lax"; // Strict, Lax, None

    std::string serialize() const;

    /// Sign a cookie value with HMAC-SHA256: "s:<value>.<sig>"
    static std::string signValue(const std::string& val, const std::string& secret);

    /// Verify and unsign a cookie value: returns original value if signature matches, empty string otherwise
    static std::string unsignValue(const std::string& signedVal, const std::string& secret);
};

struct SessionEntry {
    std::string sessionId;
    std::string dataJson;
    int64_t createdAtMs = 0;
    int64_t expiresAtMs = 0;

    bool isExpired(int64_t nowMs) const {
        return expiresAtMs > 0 && nowMs > expiresAtMs;
    }
};

class SessionStore {
public:
    explicit SessionStore(int64_t defaultTtlSeconds = 3600);
    ~SessionStore() = default;

    /// Create a new session, returning a cryptographically secure random session ID
    std::string create(const std::string& dataJson = "{}", int64_t ttlSeconds = -1);

    /// Retrieve session data by session ID (returns empty string if missing or expired)
    std::string get(const std::string& sessionId);

    /// Update session data and renew its expiration TTL
    bool set(const std::string& sessionId, const std::string& dataJson, int64_t ttlSeconds = -1);

    /// Destroy a session
    bool destroy(const std::string& sessionId);

    /// Regenerate session ID (protects against session fixation attacks while preserving data)
    std::string regenerate(const std::string& oldSessionId, int64_t ttlSeconds = -1);

    /// Remove all expired sessions
    void pruneExpired();

    /// Get current active session count
    size_t count();

private:
    int64_t m_defaultTtlSeconds;
    std::unordered_map<std::string, SessionEntry> m_sessions;
    mutable std::mutex m_mutex;
};

} // namespace sp::rt::crypto
