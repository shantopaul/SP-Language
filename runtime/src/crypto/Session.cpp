#include "sp/runtime/Session.h"
#include "sp/runtime/PersistentSession.h"
#include "sp/runtime/Crypto.h"
#include <chrono>

namespace sp::rt::crypto {

namespace {

int64_t currentNowMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t b = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(b);
    }
    return bytes;
}

} // anonymous namespace

// ============================================================================
// Cookie Implementation
// ============================================================================

std::string Cookie::serialize() const {
    std::string res = name + "=" + value;
    if (!path.empty()) {
        res += "; Path=" + path;
    }
    if (!domain.empty()) {
        res += "; Domain=" + domain;
    }
    if (maxAge > 0) {
        res += "; Max-Age=" + std::to_string(maxAge);
    }
    if (httpOnly) {
        res += "; HttpOnly";
    }
    if (secure) {
        res += "; Secure";
    }
    if (!sameSite.empty()) {
        res += "; SameSite=" + sameSite;
    }
    return res;
}

std::string Cookie::signValue(const std::string& val, const std::string& secret) {
    std::string hexSig = hmacSha256(secret, val);
    auto rawSig = hexToBytes(hexSig);
    std::string sigB64 = base64UrlEncodeBytes(rawSig.data(), rawSig.size());
    return "s:" + val + "." + sigB64;
}

std::string Cookie::unsignValue(const std::string& signedVal, const std::string& secret) {
    if (signedVal.rfind("s:", 0) != 0) {
        return "";
    }
    size_t lastDot = signedVal.rfind('.');
    if (lastDot == std::string::npos || lastDot <= 2) {
        return "";
    }

    std::string val = signedVal.substr(2, lastDot - 2);
    std::string sigB64 = signedVal.substr(lastDot + 1);

    std::string hexSig = hmacSha256(secret, val);
    auto rawSig = hexToBytes(hexSig);
    std::string expectedSigB64 = base64UrlEncodeBytes(rawSig.data(), rawSig.size());

    if (timingSafeEqual(sigB64, expectedSigB64)) {
        return val;
    }
    return "";
}

// ============================================================================
// SessionStore Implementation
// ============================================================================

SessionStore::SessionStore(int64_t defaultTtlSeconds)
    : m_defaultTtlSeconds(defaultTtlSeconds > 0 ? defaultTtlSeconds : 3600) {}

std::string SessionStore::create(const std::string& dataJson, int64_t ttlSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    int64_t nowMs = currentNowMs();
    int64_t ttl = (ttlSeconds > 0) ? ttlSeconds : m_defaultTtlSeconds;

    std::string sid = randomUuid();
    SessionEntry entry;
    entry.sessionId = sid;
    entry.dataJson = dataJson.empty() ? "{}" : dataJson;
    entry.createdAtMs = nowMs;
    entry.expiresAtMs = nowMs + (ttl * 1000);

    m_sessions[sid] = entry;
    return sid;
}

std::string SessionStore::get(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return "";
    }

    int64_t nowMs = currentNowMs();
    if (it->second.isExpired(nowMs)) {
        m_sessions.erase(it);
        return "";
    }

    return it->second.dataJson;
}

bool SessionStore::set(const std::string& sessionId, const std::string& dataJson, int64_t ttlSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(sessionId);
    if (it == m_sessions.end()) {
        return false;
    }

    int64_t nowMs = currentNowMs();
    if (it->second.isExpired(nowMs)) {
        m_sessions.erase(it);
        return false;
    }

    int64_t ttl = (ttlSeconds > 0) ? ttlSeconds : m_defaultTtlSeconds;
    it->second.dataJson = dataJson;
    it->second.expiresAtMs = nowMs + (ttl * 1000);
    return true;
}

bool SessionStore::destroy(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.erase(sessionId) > 0;
}

std::string SessionStore::regenerate(const std::string& oldSessionId, int64_t ttlSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_sessions.find(oldSessionId);
    if (it == m_sessions.end()) {
        return "";
    }

    int64_t nowMs = currentNowMs();
    if (it->second.isExpired(nowMs)) {
        m_sessions.erase(it);
        return "";
    }

    std::string savedData = it->second.dataJson;
    m_sessions.erase(it);

    int64_t ttl = (ttlSeconds > 0) ? ttlSeconds : m_defaultTtlSeconds;
    std::string newSid = randomUuid();
    SessionEntry entry;
    entry.sessionId = newSid;
    entry.dataJson = savedData;
    entry.createdAtMs = nowMs;
    entry.expiresAtMs = nowMs + (ttl * 1000);

    m_sessions[newSid] = entry;
    return newSid;
}

void SessionStore::pruneExpired() {
    std::lock_guard<std::mutex> lock(m_mutex);
    int64_t nowMs = currentNowMs();
    for (auto it = m_sessions.begin(); it != m_sessions.end(); ) {
        if (it->second.isExpired(nowMs)) {
            it = m_sessions.erase(it);
        } else {
            ++it;
        }
    }
}

size_t SessionStore::count() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_sessions.size();
}

// ----------------------------------------------------------------------------
// PersistentSessionStore (SQLite-backed durable sessions)
// ----------------------------------------------------------------------------

PersistentSessionStore::PersistentSessionStore(int64_t defaultTtlSeconds)
    : m_defaultTtlSeconds(defaultTtlSeconds > 0 ? defaultTtlSeconds : 3600) {}

PersistentSessionStore::~PersistentSessionStore() {
    close();
}

bool PersistentSessionStore::open(const std::string& path) {
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
    if (!ensureSchema()) {
        std::string err = m_lastError;
        close();
        m_lastError = err;
        return false;
    }
    return true;
}

void PersistentSessionStore::close() {
    if (m_dbOwned) {
        m_dbOwned->close();
        m_dbOwned.reset();
    }
    m_open = false;
}

bool PersistentSessionStore::isOpen() const {
    return m_open;
}

std::string PersistentSessionStore::lastError() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastError;
}

bool PersistentSessionStore::ensureSchema() {
    int rc = m_dbOwned->execute(
        "CREATE TABLE IF NOT EXISTS sp_sessions ("
        "  id TEXT PRIMARY KEY,"
        "  data TEXT NOT NULL DEFAULT '{}',"
        "  created_at_ms INTEGER NOT NULL,"
        "  expires_at_ms INTEGER NOT NULL DEFAULT 0"
        ");");
    if (rc < 0) {
        m_lastError = m_dbOwned->lastError();
        return false;
    }
    return true;
}

int64_t PersistentSessionStore::resolveExpiryMs(int64_t ttlSeconds) const {
    int64_t ttl = (ttlSeconds > 0) ? ttlSeconds : m_defaultTtlSeconds;
    if (ttl <= 0) return 0; // never expires
    return currentNowMs() + ttl * 1000;
}

std::string PersistentSessionStore::create(const std::string& dataJson, int64_t ttlSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) {
        m_lastError = "persistent session store is not open";
        return "";
    }
    std::string sid = randomUuid();
    auto stmt = m_dbOwned->prepare(
        "INSERT INTO sp_sessions (id, data, created_at_ms, expires_at_ms) VALUES (?, ?, ?, ?);");
    if (!stmt) {
        m_lastError = m_dbOwned->lastError();
        return "";
    }
    stmt->bindString(1, sid);
    stmt->bindString(2, dataJson);
    stmt->bindInt(3, currentNowMs());
    stmt->bindInt(4, resolveExpiryMs(ttlSeconds));
    if (stmt->step() != 101) { // SQLITE_DONE
        m_lastError = m_dbOwned->lastError();
        return "";
    }
    return sid;
}

std::string PersistentSessionStore::get(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned || sessionId.empty()) return "";
    auto stmt = m_dbOwned->prepare(
        "SELECT data, expires_at_ms FROM sp_sessions WHERE id=?;");
    if (!stmt) return "";
    stmt->bindString(1, sessionId);
    if (stmt->step() != 100) return ""; // SQLITE_ROW
    std::string data = stmt->columnString(0);
    int64_t expiresAt = stmt->columnInt(1);
    if (expiresAt > 0 && currentNowMs() > expiresAt) {
        // Expired: delete lazily and report missing.
        auto del = m_dbOwned->prepare("DELETE FROM sp_sessions WHERE id=?;");
        if (del) {
            del->bindString(1, sessionId);
            del->step();
        }
        return "";
    }
    return data;
}

bool PersistentSessionStore::set(const std::string& sessionId, const std::string& dataJson, int64_t ttlSeconds) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned || sessionId.empty()) return false;
    // Refuse to resurrect an expired session: set() only renews live ones.
    auto check = m_dbOwned->prepare("SELECT expires_at_ms FROM sp_sessions WHERE id=?;");
    if (!check) return false;
    check->bindString(1, sessionId);
    if (check->step() != 100) return false;
    int64_t expiresAt = check->columnInt(0);
    if (expiresAt > 0 && currentNowMs() > expiresAt) return false;
    auto upd = m_dbOwned->prepare(
        "UPDATE sp_sessions SET data=?, expires_at_ms=? WHERE id=?;");
    if (!upd) return false;
    upd->bindString(1, dataJson);
    upd->bindInt(2, resolveExpiryMs(ttlSeconds));
    upd->bindString(3, sessionId);
    if (upd->step() != 101) return false;
    return m_dbOwned->changes() == 1;
}

bool PersistentSessionStore::destroy(const std::string& sessionId) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned || sessionId.empty()) return false;
    auto stmt = m_dbOwned->prepare("DELETE FROM sp_sessions WHERE id=?;");
    if (!stmt) return false;
    stmt->bindString(1, sessionId);
    if (stmt->step() != 101) return false;
    return m_dbOwned->changes() == 1;
}

std::string PersistentSessionStore::regenerate(const std::string& oldSessionId, int64_t ttlSeconds) {
    // Single lock for the whole read-delete-insert: create() takes the
    // same mutex, so it must not be called from here.
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned || oldSessionId.empty()) return "";
    auto stmt = m_dbOwned->prepare("SELECT data, expires_at_ms FROM sp_sessions WHERE id=?;");
    if (!stmt) return "";
    stmt->bindString(1, oldSessionId);
    if (stmt->step() != 100) return ""; // missing
    std::string data = stmt->columnString(0);
    int64_t expiresAt = stmt->columnInt(1);
    if (expiresAt > 0 && currentNowMs() > expiresAt) return ""; // expired
    auto del = m_dbOwned->prepare("DELETE FROM sp_sessions WHERE id=?;");
    if (!del) return "";
    del->bindString(1, oldSessionId);
    if (del->step() != 101) return "";
    std::string newSid = randomUuid();
    auto ins = m_dbOwned->prepare(
        "INSERT INTO sp_sessions (id, data, created_at_ms, expires_at_ms) VALUES (?, ?, ?, ?);");
    if (!ins) {
        m_lastError = m_dbOwned->lastError();
        return "";
    }
    ins->bindString(1, newSid);
    ins->bindString(2, data);
    ins->bindInt(3, currentNowMs());
    ins->bindInt(4, resolveExpiryMs(ttlSeconds));
    if (ins->step() != 101) {
        m_lastError = m_dbOwned->lastError();
        return "";
    }
    return newSid;
}

void PersistentSessionStore::pruneExpired() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return;
    auto stmt = m_dbOwned->prepare(
        "DELETE FROM sp_sessions WHERE expires_at_ms > 0 AND expires_at_ms < ?;");
    if (!stmt) return;
    stmt->bindInt(1, currentNowMs());
    stmt->step();
}

size_t PersistentSessionStore::count() {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_dbOwned) return 0;
    auto stmt = m_dbOwned->prepare("SELECT COUNT(*) FROM sp_sessions;");
    if (!stmt || stmt->step() != 100) return 0;
    return static_cast<size_t>(stmt->columnInt(0));
}

} // namespace sp::rt::crypto
