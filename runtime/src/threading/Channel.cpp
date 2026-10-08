#include "sp/runtime/Channel.h"

namespace sp::rt {

Channel::Channel(int64_t capacity)
    : m_capacity(capacity > 0 ? capacity : 0) {
    if (m_capacity > 0) {
        m_ring_buffer.resize(static_cast<size_t>(m_capacity));
    }
}

Channel::~Channel() {
    close();
}

bool Channel::send(void* item) {
    std::unique_lock<std::mutex> lock(m_mtx);

    if (m_capacity > 0) {
        // Bounded channel: block while full and not closed
        m_cv_not_full.wait(lock, [this]() {
            return m_closed || m_count < static_cast<size_t>(m_capacity);
        });

        if (m_closed) {
            return false;
        }

        m_ring_buffer[m_tail] = item;
        m_tail = (m_tail + 1) % m_capacity;
        m_count++;
    } else {
        // Unbounded channel: never block on send unless closed
        if (m_closed) {
            return false;
        }
        m_unbounded_queue.push_back(item);
    }

    lock.unlock();
    m_cv_not_empty.notify_one();
    return true;
}

void* Channel::recv() {
    std::unique_lock<std::mutex> lock(m_mtx);

    if (m_capacity > 0) {
        // Wait while empty and not closed
        m_cv_not_empty.wait(lock, [this]() {
            return m_closed || m_count > 0;
        });

        if (m_count > 0) {
            void* item = m_ring_buffer[m_head];
            m_head = (m_head + 1) % m_capacity;
            m_count--;
            lock.unlock();
            m_cv_not_full.notify_one();
            return item;
        }

        // Closed and empty
        return nullptr;
    } else {
        // Unbounded channel
        m_cv_not_empty.wait(lock, [this]() {
            return m_closed || !m_unbounded_queue.empty();
        });

        if (!m_unbounded_queue.empty()) {
            void* item = m_unbounded_queue.front();
            m_unbounded_queue.pop_front();
            return item;
        }

        // Closed and empty
        return nullptr;
    }
}

bool Channel::try_send(void* item) {
    std::unique_lock<std::mutex> lock(m_mtx);
    if (m_closed) {
        return false;
    }

    if (m_capacity > 0) {
        if (m_count >= static_cast<size_t>(m_capacity)) {
            return false; // Full
        }
        m_ring_buffer[m_tail] = item;
        m_tail = (m_tail + 1) % m_capacity;
        m_count++;
    } else {
        m_unbounded_queue.push_back(item);
    }

    lock.unlock();
    m_cv_not_empty.notify_one();
    return true;
}

bool Channel::try_recv(void** out_item) {
    std::unique_lock<std::mutex> lock(m_mtx);

    if (m_capacity > 0) {
        if (m_count > 0) {
            void* item = m_ring_buffer[m_head];
            m_head = (m_head + 1) % m_capacity;
            m_count--;
            lock.unlock();
            m_cv_not_full.notify_one();
            if (out_item) *out_item = item;
            return true;
        }
        return false;
    } else {
        if (!m_unbounded_queue.empty()) {
            void* item = m_unbounded_queue.front();
            m_unbounded_queue.pop_front();
            if (out_item) *out_item = item;
            return true;
        }
        return false;
    }
}

void Channel::close() {
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        m_closed = true;
    }
    m_cv_not_empty.notify_all();
    m_cv_not_full.notify_all();
}

bool Channel::is_closed() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_closed;
}

int64_t Channel::size() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_capacity > 0 ? static_cast<int64_t>(m_count) : static_cast<int64_t>(m_unbounded_queue.size());
}

int64_t Channel::capacity() const {
    return m_capacity;
}

} // namespace sp::rt
