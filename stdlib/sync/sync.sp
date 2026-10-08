module std.sync;

// ============================================================================
// Standard Library Concurrency, Threading & Lock-Free Channels (SP 1.5)
// ============================================================================

extern "C" {
    // OS Threads
    fn sp_rt_thread_spawn(int fnPtr, int arg) -> int;
    fn sp_rt_thread_join(int handle);
    fn sp_rt_thread_detach(int handle);
    fn sp_rt_thread_id() -> int;
    fn sp_rt_thread_hardware_concurrency() -> int;
    fn sp_rt_thread_yield();
    fn sp_rt_thread_sleep_ms(int ms);

    // Mutex
    fn sp_rt_mutex_create() -> int;
    fn sp_rt_mutex_lock(int mtx);
    fn sp_rt_mutex_try_lock(int mtx) -> bool;
    fn sp_rt_mutex_unlock(int mtx);
    fn sp_rt_mutex_destroy(int mtx);

    // RwLock
    fn sp_rt_rwlock_create() -> int;
    fn sp_rt_rwlock_read_lock(int rw);
    fn sp_rt_rwlock_try_read_lock(int rw) -> bool;
    fn sp_rt_rwlock_read_unlock(int rw);
    fn sp_rt_rwlock_write_lock(int rw);
    fn sp_rt_rwlock_try_write_lock(int rw) -> bool;
    fn sp_rt_rwlock_write_unlock(int rw);
    fn sp_rt_rwlock_destroy(int rw);

    // Semaphore
    fn sp_rt_semaphore_create(int count) -> int;
    fn sp_rt_semaphore_acquire(int sem);
    fn sp_rt_semaphore_try_acquire(int sem) -> bool;
    fn sp_rt_semaphore_release(int sem);
    fn sp_rt_semaphore_destroy(int sem);

    // ConditionVariable
    fn sp_rt_condvar_create() -> int;
    fn sp_rt_condvar_wait(int cv, int mtx);
    fn sp_rt_condvar_wait_timeout(int cv, int mtx, int ms) -> bool;
    fn sp_rt_condvar_notify_one(int cv);
    fn sp_rt_condvar_notify_all(int cv);
    fn sp_rt_condvar_destroy(int cv);

    // Atomics: AtomicInt
    fn sp_rt_atomic_int_create(int initialVal) -> int;
    fn sp_rt_atomic_int_load(int atom) -> int;
    fn sp_rt_atomic_int_store(int atom, int val);
    fn sp_rt_atomic_int_add(int atom, int val) -> int;
    fn sp_rt_atomic_int_sub(int atom, int val) -> int;
    fn sp_rt_atomic_int_exchange(int atom, int val) -> int;
    fn sp_rt_atomic_int_cas(int atom, int expected, int desired) -> bool;
    fn sp_rt_atomic_int_destroy(int atom);

    // Atomics: AtomicBool
    fn sp_rt_atomic_bool_create(bool initialVal) -> int;
    fn sp_rt_atomic_bool_load(int atom) -> bool;
    fn sp_rt_atomic_bool_store(int atom, bool val);
    fn sp_rt_atomic_bool_exchange(int atom, bool val) -> bool;
    fn sp_rt_atomic_bool_cas(int atom, bool expected, bool desired) -> bool;
    fn sp_rt_atomic_bool_destroy(int atom);

    // Channels
    fn sp_rt_channel_create(int capacity) -> int;
    fn sp_rt_channel_send_i64(int chan, int val) -> bool;
    fn sp_rt_channel_recv_i64(int chan) -> int;
    fn sp_rt_channel_try_send_i64(int chan, int val) -> bool;
    fn sp_rt_channel_close(int chan);
    fn sp_rt_channel_is_closed(int chan) -> bool;
    fn sp_rt_channel_size(int chan) -> int;
    fn sp_rt_channel_capacity(int chan) -> int;
    fn sp_rt_channel_destroy(int chan);

    // ThreadPool
    fn sp_rt_threadpool_create(int numThreads) -> int;
    fn sp_rt_threadpool_submit(int pool, int fnPtr, int arg);
    fn sp_rt_threadpool_wait_idle(int pool);
    fn sp_rt_threadpool_thread_count(int pool) -> int;
    fn sp_rt_threadpool_destroy(int pool);
}

/**
 * Cross-platform native OS thread abstraction.
 */
public class Thread {
    public int handle;

    public Thread() {
        this.handle = 0;
    }

    public static fn spawn(fnPtr: int, arg: int) -> Thread {
        Thread th = new Thread();
        th.handle = sp_rt_thread_spawn(fnPtr, arg);
        return th;
    }

    public static fn id() -> int {
        return sp_rt_thread_id();
    }

    public static fn hardwareConcurrency() -> int {
        return sp_rt_thread_hardware_concurrency();
    }

    public static fn yield() {
        sp_rt_thread_yield();
    }

    public static fn sleep(ms: int) {
        sp_rt_thread_sleep_ms(ms);
    }

    public fn join() {
        if (this.handle != 0) {
            sp_rt_thread_join(this.handle);
            this.handle = 0;
        }
    }

    public fn detach() {
        if (this.handle != 0) {
            sp_rt_thread_detach(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * Mutual exclusion lock for thread synchronization.
 */
public class Mutex {
    public int handle;

    public Mutex() {
        this.handle = sp_rt_mutex_create();
    }

    public fn lock() {
        sp_rt_mutex_lock(this.handle);
    }

    public fn tryLock() -> bool {
        return sp_rt_mutex_try_lock(this.handle);
    }

    public fn unlock() {
        sp_rt_mutex_unlock(this.handle);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_mutex_destroy(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * Reader-writer lock allowing multiple concurrent readers or one exclusive writer.
 */
public class RwLock {
    public int handle;

    public RwLock() {
        this.handle = sp_rt_rwlock_create();
    }

    public fn readLock() {
        sp_rt_rwlock_read_lock(this.handle);
    }

    public fn tryReadLock() -> bool {
        return sp_rt_rwlock_try_read_lock(this.handle);
    }

    public fn readUnlock() {
        sp_rt_rwlock_read_unlock(this.handle);
    }

    public fn writeLock() {
        sp_rt_rwlock_write_lock(this.handle);
    }

    public fn tryWriteLock() -> bool {
        return sp_rt_rwlock_try_write_lock(this.handle);
    }

    public fn writeUnlock() {
        sp_rt_rwlock_write_unlock(this.handle);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_rwlock_destroy(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * Counting semaphore for resource limiting and synchronization.
 */
public class Semaphore {
    public int handle;

    public Semaphore(initialCount: int) {
        this.handle = sp_rt_semaphore_create(initialCount);
    }

    public fn acquire() {
        sp_rt_semaphore_acquire(this.handle);
    }

    public fn tryAcquire() -> bool {
        return sp_rt_semaphore_try_acquire(this.handle);
    }

    public fn release() {
        sp_rt_semaphore_release(this.handle);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_semaphore_destroy(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * Condition variable for thread signaling.
 */
public class ConditionVariable {
    public int handle;

    public ConditionVariable() {
        this.handle = sp_rt_condvar_create();
    }

    public fn wait(mtx: Mutex) {
        sp_rt_condvar_wait(this.handle, mtx.handle);
    }

    public fn waitTimeout(mtx: Mutex, timeoutMs: int) -> bool {
        return sp_rt_condvar_wait_timeout(this.handle, mtx.handle, timeoutMs);
    }

    public fn notifyOne() {
        sp_rt_condvar_notify_one(this.handle);
    }

    public fn notifyAll() {
        sp_rt_condvar_notify_all(this.handle);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_condvar_destroy(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * Lock-free 64-bit atomic integer.
 */
public class AtomicInt {
    public int handle;

    public AtomicInt() {
        this.handle = sp_rt_atomic_int_create(0);
    }

    public AtomicInt(initialVal: int) {
        this.handle = sp_rt_atomic_int_create(initialVal);
    }

    public fn get() -> int {
        return sp_rt_atomic_int_load(this.handle);
    }

    public fn load() -> int {
        return sp_rt_atomic_int_load(this.handle);
    }

    public fn set(val: int) {
        sp_rt_atomic_int_store(this.handle, val);
    }

    public fn store(val: int) {
        sp_rt_atomic_int_store(this.handle, val);
    }

    public fn add(val: int) -> int {
        return sp_rt_atomic_int_add(this.handle, val);
    }

    public fn sub(val: int) -> int {
        return sp_rt_atomic_int_sub(this.handle, val);
    }

    public fn increment() -> int {
        return sp_rt_atomic_int_add(this.handle, 1);
    }

    public fn decrement() -> int {
        return sp_rt_atomic_int_sub(this.handle, 1);
    }

    public fn exchange(val: int) -> int {
        return sp_rt_atomic_int_exchange(this.handle, val);
    }

    public fn compareAndSwap(expected: int, desired: int) -> bool {
        return sp_rt_atomic_int_cas(this.handle, expected, desired);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_atomic_int_destroy(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * Lock-free atomic boolean flag.
 */
public class AtomicBool {
    public int handle;

    public AtomicBool() {
        this.handle = sp_rt_atomic_bool_create(false);
    }

    public AtomicBool(initialVal: bool) {
        this.handle = sp_rt_atomic_bool_create(initialVal);
    }

    public fn get() -> bool {
        return sp_rt_atomic_bool_load(this.handle);
    }

    public fn load() -> bool {
        return sp_rt_atomic_bool_load(this.handle);
    }

    public fn set(val: bool) {
        sp_rt_atomic_bool_store(this.handle, val);
    }

    public fn store(val: bool) {
        sp_rt_atomic_bool_store(this.handle, val);
    }

    public fn exchange(val: bool) -> bool {
        return sp_rt_atomic_bool_exchange(this.handle, val);
    }

    public fn compareAndSwap(expected: bool, desired: bool) -> bool {
        return sp_rt_atomic_bool_cas(this.handle, expected, desired);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_atomic_bool_destroy(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * High-performance, lock-free typed message-passing channel.
 * Supports bounded capacity with backpressure or unbounded queues.
 */
public class Channel<T> {
    public int handle;

    public Channel() {
        this.handle = sp_rt_channel_create(0);
    }

    public Channel(capacity: int) {
        this.handle = sp_rt_channel_create(capacity);
    }

    public fn send(val: int) -> bool {
        return sp_rt_channel_send_i64(this.handle, val);
    }

    public fn recv() -> int {
        return sp_rt_channel_recv_i64(this.handle);
    }

    public fn trySend(val: int) -> bool {
        return sp_rt_channel_try_send_i64(this.handle, val);
    }

    public fn close() {
        sp_rt_channel_close(this.handle);
    }

    public fn isClosed() -> bool {
        return sp_rt_channel_is_closed(this.handle);
    }

    public fn size() -> int {
        return sp_rt_channel_size(this.handle);
    }

    public fn capacity() -> int {
        return sp_rt_channel_capacity(this.handle);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_channel_destroy(this.handle);
            this.handle = 0;
        }
    }
}

/**
 * High-performance work-stealing thread pool for scalable task parallelism.
 */
public class ThreadPool {
    public int handle;

    public ThreadPool() {
        this.handle = sp_rt_threadpool_create(0);
    }

    public ThreadPool(numThreads: int) {
        this.handle = sp_rt_threadpool_create(numThreads);
    }

    public fn submit(fnPtr: int, arg: int) {
        sp_rt_threadpool_submit(this.handle, fnPtr, arg);
    }

    public fn waitIdle() {
        sp_rt_threadpool_wait_idle(this.handle);
    }

    public fn threadCount() -> int {
        return sp_rt_threadpool_thread_count(this.handle);
    }

    public fn destroy() {
        if (this.handle != 0) {
            sp_rt_threadpool_destroy(this.handle);
            this.handle = 0;
        }
    }
}
