#pragma once

#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <cstdint>

namespace sp::runtime {

struct DbPoolConfig {
    size_t minConnections{1};
    size_t maxConnections{10};
    int acquireTimeoutMs{5000};
    int idleTimeoutMs{30000};
    bool testOnAcquire{true};
};

struct PooledConnection {
    void* handle{nullptr};
    std::chrono::steady_clock::time_point lastUsed;
    std::chrono::steady_clock::time_point createdAt;
};

class DbPool {
public:
    using FactoryFunc = std::function<void*()>;
    using DestroyFunc = std::function<void(void*)>;
    using PingFunc = std::function<bool(void*)>;

    DbPool(DbPoolConfig config, FactoryFunc factory, DestroyFunc destroyer, PingFunc pinger = nullptr);
    ~DbPool();

    DbPool(const DbPool&) = delete;
    DbPool& operator=(const DbPool&) = delete;

    bool initialize();
    void* acquire();
    void release(void* handle);
    void invalidate(void* handle);
    void closeAll();

    size_t totalCount() const;
    size_t availableCount() const;
    size_t inUseCount() const;

    const DbPoolConfig& config() const { return m_config; }

private:
    void* createRawConnection();
    void destroyRawConnection(void* handle);
    bool pingRawConnection(void* handle);

    DbPoolConfig m_config;
    FactoryFunc m_factory;
    DestroyFunc m_destroyer;
    PingFunc m_pinger;

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::deque<PooledConnection> m_available;
    size_t m_totalCreated{0};
    bool m_closed{false};
};

} // namespace sp::runtime
