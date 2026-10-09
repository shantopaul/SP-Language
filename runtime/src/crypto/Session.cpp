#include "sp/runtime/Session.h"
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

} // namespace sp::rt::crypto
