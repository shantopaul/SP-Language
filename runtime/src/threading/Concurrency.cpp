#include "sp/runtime/Concurrency.h"
#include "sp/runtime/Channel.h"
#include "sp/runtime/ThreadPool.h"
#include "sp/runtime/Runtime.h"
#include <chrono>

using namespace sp::rt;

extern "C" {

// ============================================================================
// OS Threads
// ============================================================================

int64_t sp_rt_thread_spawn(void (*fn)(void*), void* arg) {
    if (!fn) return 0;
    auto* handle = new ThreadHandle();
    handle->thread = std::thread([fn, arg]() {
        fn(arg);
    });
    return reinterpret_cast<int64_t>(handle);
}

void sp_rt_thread_join(int64_t handle) {
    if (!handle) return;
    auto* th = reinterpret_cast<ThreadHandle*>(handle);
    if (!th->joined.exchange(true) && !th->detached.load()) {
        if (th->thread.joinable()) {
            th->thread.join();
        }
    }
    delete th;
}

void sp_rt_thread_detach(int64_t handle) {
    if (!handle) return;
    auto* th = reinterpret_cast<ThreadHandle*>(handle);
    if (!th->detached.exchange(true) && !th->joined.load()) {
        if (th->thread.joinable()) {
            th->thread.detach();
        }
    }
    delete th;
}

int64_t sp_rt_thread_id() {
    return static_cast<int64_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()));
}

int64_t sp_rt_thread_hardware_concurrency() {
    int64_t c = static_cast<int64_t>(std::thread::hardware_concurrency());
    return c > 0 ? c : 1;
}

void sp_rt_thread_yield() {
    std::this_thread::yield();
}

void sp_rt_thread_sleep_ms(int64_t ms) {
    if (ms > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}

// ============================================================================
// Mutex
// ============================================================================

void* sp_rt_mutex_create() {
    return new MutexHandle();
}

void sp_rt_mutex_lock(void* mtx) {
    if (mtx) {
        static_cast<MutexHandle*>(mtx)->mtx.lock();
    }
}

bool sp_rt_mutex_try_lock(void* mtx) {
    if (!mtx) return false;
    return static_cast<MutexHandle*>(mtx)->mtx.try_lock();
}

void sp_rt_mutex_unlock(void* mtx) {
    if (mtx) {
        static_cast<MutexHandle*>(mtx)->mtx.unlock();
    }
}

void sp_rt_mutex_destroy(void* mtx) {
    delete static_cast<MutexHandle*>(mtx);
}

// ============================================================================
// RwLock
// ============================================================================

void* sp_rt_rwlock_create() {
    return new RwLockHandle();
}

void sp_rt_rwlock_read_lock(void* rw) {
    if (rw) {
        static_cast<RwLockHandle*>(rw)->rwlock.lock_shared();
    }
}

bool sp_rt_rwlock_try_read_lock(void* rw) {
    if (!rw) return false;
    return static_cast<RwLockHandle*>(rw)->rwlock.try_lock_shared();
}

void sp_rt_rwlock_read_unlock(void* rw) {
    if (rw) {
        static_cast<RwLockHandle*>(rw)->rwlock.unlock_shared();
    }
}

void sp_rt_rwlock_write_lock(void* rw) {
    if (rw) {
        static_cast<RwLockHandle*>(rw)->rwlock.lock();
    }
}

bool sp_rt_rwlock_try_write_lock(void* rw) {
    if (!rw) return false;
    return static_cast<RwLockHandle*>(rw)->rwlock.try_lock();
}

void sp_rt_rwlock_write_unlock(void* rw) {
    if (rw) {
        static_cast<RwLockHandle*>(rw)->rwlock.unlock();
    }
}

void sp_rt_rwlock_destroy(void* rw) {
    delete static_cast<RwLockHandle*>(rw);
}

// ============================================================================
// Semaphore
// ============================================================================

void* sp_rt_semaphore_create(int64_t initial_count) {
    auto* sem = new SemaphoreHandle();
    sem->count = initial_count > 0 ? initial_count : 0;
    return sem;
}

void sp_rt_semaphore_acquire(void* sem) {
    if (!sem) return;
    auto* s = static_cast<SemaphoreHandle*>(sem);
    std::unique_lock<std::mutex> lock(s->mtx);
    s->cv.wait(lock, [s]() { return s->count > 0; });
    s->count--;
}

bool sp_rt_semaphore_try_acquire(void* sem) {
    if (!sem) return false;
    auto* s = static_cast<SemaphoreHandle*>(sem);
    std::unique_lock<std::mutex> lock(s->mtx);
    if (s->count > 0) {
        s->count--;
        return true;
    }
    return false;
}

void sp_rt_semaphore_release(void* sem) {
    if (!sem) return;
    auto* s = static_cast<SemaphoreHandle*>(sem);
    {
        std::lock_guard<std::mutex> lock(s->mtx);
        s->count++;
    }
    s->cv.notify_one();
}

void sp_rt_semaphore_destroy(void* sem) {
    delete static_cast<SemaphoreHandle*>(sem);
}

// ============================================================================
// ConditionVariable
// ============================================================================

void* sp_rt_condvar_create() {
    return new CondVarHandle();
}

void sp_rt_condvar_wait(void* cv, void* mtx) {
    if (!cv || !mtx) return;
    auto* c = static_cast<CondVarHandle*>(cv);
    auto* m = static_cast<MutexHandle*>(mtx);
    c->cv.wait(m->mtx);
}

bool sp_rt_condvar_wait_timeout(void* cv, void* mtx, int64_t timeout_ms) {
    if (!cv || !mtx) return false;
    auto* c = static_cast<CondVarHandle*>(cv);
    auto* m = static_cast<MutexHandle*>(mtx);
    auto status = c->cv.wait_for(m->mtx, std::chrono::milliseconds(timeout_ms));
    return status == std::cv_status::no_timeout;
}

void sp_rt_condvar_notify_one(void* cv) {
    if (cv) {
        static_cast<CondVarHandle*>(cv)->cv.notify_one();
    }
}

void sp_rt_condvar_notify_all(void* cv) {
    if (cv) {
        static_cast<CondVarHandle*>(cv)->cv.notify_all();
    }
}

void sp_rt_condvar_destroy(void* cv) {
    delete static_cast<CondVarHandle*>(cv);
}

// ============================================================================
// Atomics: AtomicInt
// ============================================================================

void* sp_rt_atomic_int_create(int64_t initial_val) {
    auto* a = new AtomicIntHandle();
    a->val.store(initial_val, std::memory_order_relaxed);
    return a;
}

int64_t sp_rt_atomic_int_load(void* atom) {
    if (!atom) return 0;
    return static_cast<AtomicIntHandle*>(atom)->val.load(std::memory_order_seq_cst);
}

void sp_rt_atomic_int_store(void* atom, int64_t val) {
    if (atom) {
        static_cast<AtomicIntHandle*>(atom)->val.store(val, std::memory_order_seq_cst);
    }
}

int64_t sp_rt_atomic_int_add(void* atom, int64_t val) {
    if (!atom) return 0;
    return static_cast<AtomicIntHandle*>(atom)->val.fetch_add(val, std::memory_order_seq_cst);
}

int64_t sp_rt_atomic_int_sub(void* atom, int64_t val) {
    if (!atom) return 0;
    return static_cast<AtomicIntHandle*>(atom)->val.fetch_sub(val, std::memory_order_seq_cst);
}

int64_t sp_rt_atomic_int_exchange(void* atom, int64_t val) {
    if (!atom) return 0;
    return static_cast<AtomicIntHandle*>(atom)->val.exchange(val, std::memory_order_seq_cst);
}

bool sp_rt_atomic_int_cas(void* atom, int64_t expected, int64_t desired) {
    if (!atom) return false;
    return static_cast<AtomicIntHandle*>(atom)->val.compare_exchange_strong(
        expected, desired, std::memory_order_seq_cst, std::memory_order_seq_cst);
}

void sp_rt_atomic_int_destroy(void* atom) {
    delete static_cast<AtomicIntHandle*>(atom);
}

// ============================================================================
// Atomics: AtomicBool
// ============================================================================

void* sp_rt_atomic_bool_create(bool initial_val) {
    auto* a = new AtomicBoolHandle();
    a->val.store(initial_val, std::memory_order_relaxed);
    return a;
}

bool sp_rt_atomic_bool_load(void* atom) {
    if (!atom) return false;
    return static_cast<AtomicBoolHandle*>(atom)->val.load(std::memory_order_seq_cst);
}

void sp_rt_atomic_bool_store(void* atom, bool val) {
    if (atom) {
        static_cast<AtomicBoolHandle*>(atom)->val.store(val, std::memory_order_seq_cst);
    }
}

bool sp_rt_atomic_bool_exchange(void* atom, bool val) {
    if (!atom) return false;
    return static_cast<AtomicBoolHandle*>(atom)->val.exchange(val, std::memory_order_seq_cst);
}

bool sp_rt_atomic_bool_cas(void* atom, bool expected, bool desired) {
    if (!atom) return false;
    return static_cast<AtomicBoolHandle*>(atom)->val.compare_exchange_strong(
        expected, desired, std::memory_order_seq_cst, std::memory_order_seq_cst);
}

void sp_rt_atomic_bool_destroy(void* atom) {
    delete static_cast<AtomicBoolHandle*>(atom);
}

// ============================================================================
// Atomics: AtomicPtr
// ============================================================================

void* sp_rt_atomic_ptr_create(void* initial_val) {
    auto* a = new AtomicPtrHandle();
    a->val.store(initial_val, std::memory_order_relaxed);
    return a;
}

void* sp_rt_atomic_ptr_load(void* atom) {
    if (!atom) return nullptr;
    return static_cast<AtomicPtrHandle*>(atom)->val.load(std::memory_order_seq_cst);
}

void sp_rt_atomic_ptr_store(void* atom, void* val) {
    if (atom) {
        static_cast<AtomicPtrHandle*>(atom)->val.store(val, std::memory_order_seq_cst);
    }
}

void* sp_rt_atomic_ptr_exchange(void* atom, void* val) {
    if (!atom) return nullptr;
    return static_cast<AtomicPtrHandle*>(atom)->val.exchange(val, std::memory_order_seq_cst);
}

bool sp_rt_atomic_ptr_cas(void* atom, void* expected, void* desired) {
    if (!atom) return false;
    return static_cast<AtomicPtrHandle*>(atom)->val.compare_exchange_strong(
        expected, desired, std::memory_order_seq_cst, std::memory_order_seq_cst);
}

void sp_rt_atomic_ptr_destroy(void* atom) {
    delete static_cast<AtomicPtrHandle*>(atom);
}

// ============================================================================
// Channel Wrappers
// ============================================================================

void* sp_rt_channel_create(int64_t capacity) {
    return new Channel(capacity);
}

bool sp_rt_channel_send(void* chan, void* item) {
    if (!chan) return false;
    return static_cast<Channel*>(chan)->send(item);
}

void* sp_rt_channel_recv(void* chan) {
    if (!chan) return nullptr;
    return static_cast<Channel*>(chan)->recv();
}

bool sp_rt_channel_try_send(void* chan, void* item) {
    if (!chan) return false;
    return static_cast<Channel*>(chan)->try_send(item);
}

bool sp_rt_channel_try_recv(void* chan, void** out_item) {
    if (!chan) return false;
    return static_cast<Channel*>(chan)->try_recv(out_item);
}

bool sp_rt_channel_send_i64(void* chan, int64_t val) {
    if (!chan) return false;
    return static_cast<Channel*>(chan)->send(reinterpret_cast<void*>(static_cast<uintptr_t>(val)));
}

int64_t sp_rt_channel_recv_i64(void* chan) {
    if (!chan) return 0;
    void* item = static_cast<Channel*>(chan)->recv();
    return static_cast<int64_t>(reinterpret_cast<uintptr_t>(item));
}

bool sp_rt_channel_try_send_i64(void* chan, int64_t val) {
    if (!chan) return false;
    return static_cast<Channel*>(chan)->try_send(reinterpret_cast<void*>(static_cast<uintptr_t>(val)));
}

bool sp_rt_channel_try_recv_i64(void* chan, int64_t* out_val) {
    if (!chan) return false;
    void* item = nullptr;
    bool ok = static_cast<Channel*>(chan)->try_recv(&item);
    if (ok && out_val) {
        *out_val = static_cast<int64_t>(reinterpret_cast<uintptr_t>(item));
    }
    return ok;
}

void sp_rt_channel_close(void* chan) {
    if (chan) {
        static_cast<Channel*>(chan)->close();
    }
}

bool sp_rt_channel_is_closed(void* chan) {
    if (!chan) return true;
    return static_cast<Channel*>(chan)->is_closed();
}

int64_t sp_rt_channel_size(void* chan) {
    if (!chan) return 0;
    return static_cast<Channel*>(chan)->size();
}

int64_t sp_rt_channel_capacity(void* chan) {
    if (!chan) return 0;
    return static_cast<Channel*>(chan)->capacity();
}

void sp_rt_channel_destroy(void* chan) {
    if (!chan) return;
    auto* channel = static_cast<Channel*>(chan);
    // Phase 37 slice B: a destroyed channel gives up the shares of
    // its still-queued items (only when flagged as reference items
    // by sp_rt_channel_mark_ref_items). Items already received keep
    // their shares — recv never releases. The drain happens before
    // the delete, and the releases happen outside the channel lock.
    if (channel->ref_items()) {
        std::vector<void*> items = channel->drain_items();
        for (void* item : items) {
            sp_rt_release(item);
        }
    }
    delete channel;
}

void sp_rt_channel_mark_ref_items(void* chan) {
    if (chan) {
        static_cast<Channel*>(chan)->mark_ref_items();
    }
}

// ============================================================================
// ThreadPool Wrappers
// ============================================================================

void* sp_rt_threadpool_create(int64_t num_threads) {
    return new ThreadPool(num_threads);
}

void sp_rt_threadpool_submit(void* pool, void (*fn)(void*), void* arg) {
    if (pool && fn) {
        static_cast<ThreadPool*>(pool)->submit(fn, arg);
    }
}

void sp_rt_threadpool_wait_idle(void* pool) {
    if (pool) {
        static_cast<ThreadPool*>(pool)->wait_idle();
    }
}

int64_t sp_rt_threadpool_thread_count(void* pool) {
    if (!pool) return 0;
    return static_cast<ThreadPool*>(pool)->thread_count();
}

void sp_rt_threadpool_destroy(void* pool) {
    delete static_cast<ThreadPool*>(pool);
}

} // extern "C"
