#include "sp/runtime/Health.h"
#include "sp/runtime/Log.h" // for currentIsoTimestamp
#include <sstream>
#include <iomanip>

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

} // namespace

void HealthRegistry::registerCheck(const std::string& name, HealthCheckCallback callback) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks[name] = std::move(callback);
}

void HealthRegistry::unregisterCheck(const std::string& name) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.erase(name);
    m_staticStatuses.erase(name);
}

void HealthRegistry::setStatus(const std::string& name, bool isUp, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_staticStatuses[name] = HealthCheckResult{isUp, message};
}

std::pair<bool, std::map<std::string, HealthCheckResult>> HealthRegistry::evaluateChecks() {
    std::lock_guard<std::mutex> lock(m_mutex);

    bool overallUp = true;
    std::map<std::string, HealthCheckResult> results;
    for (const auto& [name, check] : m_callbacks) {
        try {
            HealthCheckResult res = check();
            if (!res.isUp) overallUp = false;
            results[name] = res;
        } catch (...) {
            overallUp = false;
            results[name] = HealthCheckResult{false, "check threw exception"};
        }
    }

    for (const auto& [name, staticRes] : m_staticStatuses) {
        if (!staticRes.isUp) overallUp = false;
        results[name] = staticRes;
    }

    return {overallUp, results};
}

std::string HealthRegistry::checkLiveness() {
    std::string ts = currentIsoTimestamp();
    auto [overallUp, results] = evaluateChecks();

    // Semantics: liveness answers "is this process alive and are its
    // registered checks passing?" With no checks registered there is
    // nothing failing, so the result is UP — but the empty "checks"
    // object is always emitted so callers can see that no check ran,
    // instead of mistaking an unchecked UP for a verified one.
    std::ostringstream ss;
    ss << "{\"status\":" << (overallUp ? "\"UP\"" : "\"DOWN\"")
       << ",\"timestamp\":\"" << ts << "\""
       << ",\"checks\":{";
    bool first = true;
    for (const auto& [name, res] : results) {
        if (!first) ss << ",";
        first = false;
        ss << "\"" << escapeJson(name) << "\":{"
           << "\"status\":\"" << (res.isUp ? "UP" : "DOWN") << "\"";
        if (!res.message.empty()) {
            ss << ",\"message\":\"" << escapeJson(res.message) << "\"";
        }
        ss << "}";
    }
    ss << "}}";
    return ss.str();
}

std::pair<bool, std::string> HealthRegistry::checkReadiness() {
    std::string ts = currentIsoTimestamp();
    auto [overallUp, results] = evaluateChecks();

    std::ostringstream ss;
    ss << "{\"status\":";

    ss << (overallUp ? "\"UP\"" : "\"DOWN\"")
       << ",\"timestamp\":\"" << ts << "\"";

    if (!results.empty()) {
        ss << ",\"checks\":{";
        bool first = true;
        for (const auto& [name, res] : results) {
            if (!first) ss << ",";
            first = false;
            ss << "\"" << escapeJson(name) << "\":{"
               << "\"status\":\"" << (res.isUp ? "UP" : "DOWN") << "\"";
            if (!res.message.empty()) {
                ss << ",\"message\":\"" << escapeJson(res.message) << "\"";
            }
            ss << "}";
        }
        ss << "}";
    }

    ss << "}";
    return {overallUp, ss.str()};
}

void HealthRegistry::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_callbacks.clear();
    m_staticStatuses.clear();
}

static HealthRegistry s_globalHealth;

HealthRegistry& HealthRegistry::getGlobal() {
    return s_globalHealth;
}

} // namespace sp::rt::obs
