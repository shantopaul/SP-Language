#include "sp/runtime/DbPool.h"

namespace sp::runtime {

DbPool::DbPool(DbPoolConfig config, FactoryFunc factory, DestroyFunc destroyer, PingFunc pinger)
    : m_config(std::move(config)),
      m_factory(std::move(factory)),
      m_destroyer(std::move(destroyer)),
      m_pinger(std::move(pinger)) {}

DbPool::~DbPool() {
    closeAll();
}

bool DbPool::initialize() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_closed) return false;

    while (m_totalCreated < m_config.minConnections) {
        void* conn = m_factory ? m_factory() : nullptr;
        if (!conn) break;

        auto now = std::chrono::steady_clock::now();
        m_available.push_back({conn, now, now});
        m_totalCreated++;
    }

    return m_totalCreated >= m_config.minConnections;
}

void* DbPool::createRawConnection() {
    if (!m_factory) return nullptr;
    return m_factory();
}

void DbPool::destroyRawConnection(void* handle) {
    if (handle && m_destroyer) {
        m_destroyer(handle);
    }
}

bool DbPool::pingRawConnection(void* handle) {
    if (!handle) return false;
    if (!m_pinger) return true;
    return m_pinger(handle);
}

void* DbPool::acquire() {
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_closed) return nullptr;

    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_config.acquireTimeoutMs);

    while (true) {
        // 1. Try to get an available connection
        while (!m_available.empty()) {
            PooledConnection pc = m_available.front();
            m_available.pop_front();

            auto now = std::chrono::steady_clock::now();
            auto idleMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - pc.lastUsed).count();

            // If idle too long or testOnAcquire fails, evict and destroy
            bool isStale = (m_config.idleTimeoutMs > 0 && idleMs > m_config.idleTimeoutMs);
            if (isStale || (m_config.testOnAcquire && !pingRawConnection(pc.handle))) {
                destroyRawConnection(pc.handle);
                m_totalCreated--;
                continue;
            }

            return pc.handle;
        }

        // 2. Can we create a new connection?
        if (m_totalCreated < m_config.maxConnections) {
            void* newConn = createRawConnection();
            if (newConn) {
                m_totalCreated++;
                return newConn;
            }
        }

        // 3. Wait for a released connection
        if (m_cv.wait_until(lock, deadline) == std::cv_status::timeout) {
            // Timeout reached
            return nullptr;
        }

        if (m_closed) return nullptr;
    }
}

void DbPool::release(void* handle) {
    if (!handle) return;
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_closed) {
        destroyRawConnection(handle);
        m_totalCreated--;
        return;
    }

    auto now = std::chrono::steady_clock::now();
    m_available.push_back({handle, now, now});
    m_cv.notify_one();
}

void DbPool::invalidate(void* handle) {
    if (!handle) return;
    std::unique_lock<std::mutex> lock(m_mutex);
    destroyRawConnection(handle);
    if (m_totalCreated > 0) {
        m_totalCreated--;
    }
    m_cv.notify_one();
}

void DbPool::closeAll() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_closed = true;

    while (!m_available.empty()) {
        auto pc = m_available.front();
        m_available.pop_front();
        destroyRawConnection(pc.handle);
    }
    m_totalCreated = 0;
    m_cv.notify_all();
}

size_t DbPool::totalCount() const {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_totalCreated;
}

size_t DbPool::availableCount() const {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_available.size();
}

size_t DbPool::inUseCount() const {
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_totalCreated > m_available.size() ? (m_totalCreated - m_available.size()) : 0;
}

} // namespace sp::runtime
