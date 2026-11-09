#pragma once

#include <string>
#include <map>
#include <functional>
#include <memory>
#include <mutex>
#include <utility>

namespace sp::rt::obs {

struct HealthCheckResult {
    bool isUp{true};
    std::string message;
};

using HealthCheckCallback = std::function<HealthCheckResult()>;

class HealthRegistry {
public:
    HealthRegistry() = default;

    void registerCheck(const std::string& name, HealthCheckCallback callback);
    void unregisterCheck(const std::string& name);
    void setStatus(const std::string& name, bool isUp, const std::string& message = "");

    [[nodiscard]] std::string checkLiveness();
    [[nodiscard]] std::pair<bool, std::string> checkReadiness();

    void clear();

    static HealthRegistry& getGlobal();

private:
    // Runs every registered callback (exception-safe) and merges static
    // statuses. Shared by liveness and readiness so both report the same
    // truth; liveness must never hard-code UP.
    std::pair<bool, std::map<std::string, HealthCheckResult>> evaluateChecks();

    std::mutex m_mutex;
    std::map<std::string, HealthCheckCallback> m_callbacks;
    std::map<std::string, HealthCheckResult> m_staticStatuses;
};

} // namespace sp::rt::obs
