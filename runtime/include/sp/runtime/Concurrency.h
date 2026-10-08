#pragma once

#include <cstdint>
#include <cstddef>
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <memory>

namespace sp::rt {

struct ThreadHandle {
    std::thread thread;
    std::atomic<bool> joined{false};
    std::atomic<bool> detached{false};
};

struct MutexHandle {
    std::mutex mtx;
};

struct RwLockHandle {
    std::shared_mutex rwlock;
};

struct SemaphoreHandle {
    std::mutex mtx;
    std::condition_variable cv;
    int64_t count{0};
};

struct CondVarHandle {
    std::condition_variable_any cv;
};

struct AtomicIntHandle {
    std::atomic<int64_t> val{0};
};

struct AtomicBoolHandle {
    std::atomic<bool> val{false};
};

struct AtomicPtrHandle {
    std::atomic<void*> val{nullptr};
};

} // namespace sp::rt
