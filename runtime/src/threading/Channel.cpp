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

// Phase 37 slice B: flag the queued items as retained references and
// drain them for destroy(). The flag is set from the compiler's
// Channel.send retain site (before the first reference-typed send);
// drain_items() is only called from sp_rt_channel_destroy, when the
// channel is no longer shared — a concurrent send/recv at destroy
// time is already undefined behaviour for this channel.
void Channel::mark_ref_items() {
    std::lock_guard<std::mutex> lock(m_mtx);
    m_ref_items = true;
}

bool Channel::ref_items() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_ref_items;
}

std::vector<void*> Channel::drain_items() {
    std::lock_guard<std::mutex> lock(m_mtx);
    std::vector<void*> items;
    if (m_capacity > 0) {
        items.reserve(m_count);
        for (size_t i = 0; i < m_count; ++i) {
            items.push_back(m_ring_buffer[(m_head + i) % m_capacity]);
        }
        m_head = 0;
        m_tail = 0;
        m_count = 0;
    } else {
        items.reserve(m_unbounded_queue.size());
        while (!m_unbounded_queue.empty()) {
            items.push_back(m_unbounded_queue.front());
            m_unbounded_queue.pop_front();
        }
    }
    return items;
}

int64_t Channel::size() const {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_capacity > 0 ? static_cast<int64_t>(m_count) : static_cast<int64_t>(m_unbounded_queue.size());
}

int64_t Channel::capacity() const {
    return m_capacity;
}

} // namespace sp::rt
