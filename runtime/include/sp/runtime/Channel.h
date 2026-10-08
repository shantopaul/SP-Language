#pragma once

#include <cstdint>
#include <cstddef>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <deque>
#include <atomic>

namespace sp::rt {

class Channel {
public:
    explicit Channel(int64_t capacity = 0);
    ~Channel();

    // Disable copy
    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    // Send item (blocking if bounded buffer is full)
    // Returns true on success, false if channel is closed
    bool send(void* item);

    // Recv item (blocking if buffer is empty)
    // Returns item pointer, or nullptr if channel is closed and empty
    void* recv();

    // Non-blocking try send
    // Returns true if successfully enqueued, false if full or closed
    bool try_send(void* item);

    // Non-blocking try recv
    // Returns true and writes to out_item if an item was popped, false if empty or closed
    bool try_recv(void** out_item);

    // Close channel: wakes all waiting senders and receivers
    void close();

    // Check if channel is closed
    bool is_closed() const;

    // Get current number of items buffered
    int64_t size() const;

    // Get maximum capacity (0 = unbounded)
    int64_t capacity() const;

private:
    const int64_t m_capacity; // 0 = unbounded
    mutable std::mutex m_mtx;
    std::condition_variable m_cv_not_empty;
    std::condition_variable m_cv_not_full;
    bool m_closed{false};

    // For bounded channel: pre-allocated circular ring buffer
    std::vector<void*> m_ring_buffer;
    size_t m_head{0};
    size_t m_tail{0};
    size_t m_count{0};

    // For unbounded channel: dynamic deque
    std::deque<void*> m_unbounded_queue;
};

} // namespace sp::rt
