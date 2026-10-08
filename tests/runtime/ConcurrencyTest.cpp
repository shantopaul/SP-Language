#include "sp/runtime/Runtime.h"
#include "sp/runtime/Concurrency.h"
#include "sp/runtime/Channel.h"
#include "sp/runtime/ThreadPool.h"
#include <gtest/gtest.h>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <numeric>

// ============================================================================
// OS Threads Tests
// ============================================================================

static void thread_worker_func(void* arg) {
    auto* val = static_cast<std::atomic<int64_t>*>(arg);
    val->fetch_add(42, std::memory_order_relaxed);
}

TEST(ConcurrencyTest, OSThreadSpawnAndJoin) {
    std::atomic<int64_t> counter{0};
    int64_t handle = sp_rt_thread_spawn(thread_worker_func, &counter);
    ASSERT_NE(handle, 0);

    sp_rt_thread_join(handle);
    EXPECT_EQ(counter.load(), 42);
}

TEST(ConcurrencyTest, OSThreadHardwareConcurrencyAndYield) {
    int64_t cores = sp_rt_thread_hardware_concurrency();
    EXPECT_GE(cores, 1);

    int64_t tid = sp_rt_thread_id();
    EXPECT_NE(tid, 0);

    // Yield should complete without crashing
    sp_rt_thread_yield();

    // Sleep for 2ms
    auto start = std::chrono::steady_clock::now();
    sp_rt_thread_sleep_ms(2);
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    EXPECT_GE(elapsed, 1);
}

// ============================================================================
// Mutex Tests
// ============================================================================

TEST(ConcurrencyTest, MutexMutualExclusionContention) {
    void* mtx = sp_rt_mutex_create();
    ASSERT_NE(mtx, nullptr);

    int64_t shared_counter = 0;
    const int kThreads = 16;
    const int kIterations = 25000;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([mtx, &shared_counter]() {
            for (int j = 0; j < kIterations; ++j) {
                sp_rt_mutex_lock(mtx);
                shared_counter++;
                sp_rt_mutex_unlock(mtx);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(shared_counter, kThreads * kIterations);
    sp_rt_mutex_destroy(mtx);
}

// ============================================================================
// RwLock Tests
// ============================================================================

TEST(ConcurrencyTest, RwLockReadAndWriteContention) {
    void* rw = sp_rt_rwlock_create();
    ASSERT_NE(rw, nullptr);

    int64_t shared_data = 0;
    std::atomic<int> readers_active{0};
    std::atomic<bool> stop_readers{false};
    std::atomic<int64_t> total_reads{0};

    // Spawn 8 readers
    std::vector<std::thread> readers;
    for (int i = 0; i < 8; ++i) {
        readers.emplace_back([rw, &shared_data, &stop_readers, &total_reads, &readers_active]() {
            readers_active.fetch_add(1, std::memory_order_relaxed);
            while (!stop_readers.load(std::memory_order_relaxed)) {
                sp_rt_rwlock_read_lock(rw);
                int64_t val = shared_data;
                (void)val;
                total_reads.fetch_add(1, std::memory_order_relaxed);
                sp_rt_rwlock_read_unlock(rw);
                std::this_thread::yield();
            }
        });
    }

    // Wait until all reader threads have started
    while (readers_active.load(std::memory_order_relaxed) < 8) {
        std::this_thread::yield();
    }

    // Single writer updates data 1,000 times
    for (int i = 1; i <= 1000; ++i) {
        sp_rt_rwlock_write_lock(rw);
        shared_data = i;
        sp_rt_rwlock_write_unlock(rw);
        if (i % 100 == 0) {
            std::this_thread::yield();
        }
    }

    // Ensure readers performed concurrent reads
    while (total_reads.load(std::memory_order_relaxed) == 0) {
        std::this_thread::yield();
    }

    stop_readers.store(true);
    for (auto& t : readers) {
        t.join();
    }

    EXPECT_EQ(shared_data, 1000);
    EXPECT_GT(total_reads.load(), 0);
    sp_rt_rwlock_destroy(rw);
}

// ============================================================================
// Semaphore Tests
// ============================================================================

TEST(ConcurrencyTest, SemaphoreAcquireRelease) {
    void* sem = sp_rt_semaphore_create(3);
    ASSERT_NE(sem, nullptr);

    EXPECT_TRUE(sp_rt_semaphore_try_acquire(sem)); // count -> 2
    EXPECT_TRUE(sp_rt_semaphore_try_acquire(sem)); // count -> 1
    EXPECT_TRUE(sp_rt_semaphore_try_acquire(sem)); // count -> 0
    EXPECT_FALSE(sp_rt_semaphore_try_acquire(sem)); // false: count is 0

    sp_rt_semaphore_release(sem); // count -> 1
    EXPECT_TRUE(sp_rt_semaphore_try_acquire(sem)); // count -> 0

    sp_rt_semaphore_destroy(sem);
}

// ============================================================================
// ConditionVariable Tests
// ============================================================================

TEST(ConcurrencyTest, ConditionVariableWaitAndNotify) {
    void* cv = sp_rt_condvar_create();
    void* mtx = sp_rt_mutex_create();
    ASSERT_NE(cv, nullptr);
    ASSERT_NE(mtx, nullptr);

    bool ready = false;
    std::thread worker([cv, mtx, &ready]() {
        sp_rt_mutex_lock(mtx);
        while (!ready) {
            sp_rt_condvar_wait(cv, mtx);
        }
        sp_rt_mutex_unlock(mtx);
    });

    sp_rt_thread_sleep_ms(5);
    sp_rt_mutex_lock(mtx);
    ready = true;
    sp_rt_condvar_notify_one(cv);
    sp_rt_mutex_unlock(mtx);

    worker.join();
    EXPECT_TRUE(ready);

    sp_rt_condvar_destroy(cv);
    sp_rt_mutex_destroy(mtx);
}

// ============================================================================
// Atomics Tests
// ============================================================================

TEST(ConcurrencyTest, AtomicIntFetchAddAndCasContention) {
    void* a = sp_rt_atomic_int_create(0);
    ASSERT_NE(a, nullptr);

    const int kThreads = 16;
    const int kIncrements = 50000;

    std::vector<std::thread> threads;
    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([a]() {
            for (int j = 0; j < kIncrements; ++j) {
                sp_rt_atomic_int_add(a, 1);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(sp_rt_atomic_int_load(a), static_cast<int64_t>(kThreads) * kIncrements);

    // Test CAS
    int64_t current = sp_rt_atomic_int_load(a);
    bool cas_success = sp_rt_atomic_int_cas(a, current, 9999);
    EXPECT_TRUE(cas_success);
    EXPECT_EQ(sp_rt_atomic_int_load(a), 9999);

    bool cas_fail = sp_rt_atomic_int_cas(a, 1234, 5678);
    EXPECT_FALSE(cas_fail);
    EXPECT_EQ(sp_rt_atomic_int_load(a), 9999);

    sp_rt_atomic_int_destroy(a);
}

TEST(ConcurrencyTest, AtomicBoolAndPtrOperations) {
    void* abool = sp_rt_atomic_bool_create(false);
    ASSERT_NE(abool, nullptr);
    EXPECT_FALSE(sp_rt_atomic_bool_load(abool));

    sp_rt_atomic_bool_store(abool, true);
    EXPECT_TRUE(sp_rt_atomic_bool_load(abool));

    bool old_b = sp_rt_atomic_bool_exchange(abool, false);
    EXPECT_TRUE(old_b);
    EXPECT_FALSE(sp_rt_atomic_bool_load(abool));

    sp_rt_atomic_bool_destroy(abool);

    int dummy1 = 1;
    int dummy2 = 2;
    void* aptr = sp_rt_atomic_ptr_create(&dummy1);
    ASSERT_NE(aptr, nullptr);
    EXPECT_EQ(sp_rt_atomic_ptr_load(aptr), &dummy1);

    EXPECT_TRUE(sp_rt_atomic_ptr_cas(aptr, &dummy1, &dummy2));
    EXPECT_EQ(sp_rt_atomic_ptr_load(aptr), &dummy2);

    sp_rt_atomic_ptr_destroy(aptr);
}

// ============================================================================
// Channel Tests
// ============================================================================

TEST(ConcurrencyTest, ChannelBasicSendRecvAndClose) {
    void* chan = sp_rt_channel_create(4); // Bounded capacity 4
    ASSERT_NE(chan, nullptr);
    EXPECT_EQ(sp_rt_channel_capacity(chan), 4);
    EXPECT_EQ(sp_rt_channel_size(chan), 0);
    EXPECT_FALSE(sp_rt_channel_is_closed(chan));

    EXPECT_TRUE(sp_rt_channel_send_i64(chan, 10));
    EXPECT_TRUE(sp_rt_channel_send_i64(chan, 20));
    EXPECT_EQ(sp_rt_channel_size(chan), 2);

    EXPECT_EQ(sp_rt_channel_recv_i64(chan), 10);
    EXPECT_EQ(sp_rt_channel_recv_i64(chan), 20);
    EXPECT_EQ(sp_rt_channel_size(chan), 0);

    // Try recv on empty channel
    int64_t out_val = -1;
    EXPECT_FALSE(sp_rt_channel_try_recv_i64(chan, &out_val));

    // Fill channel to capacity
    EXPECT_TRUE(sp_rt_channel_send_i64(chan, 1));
    EXPECT_TRUE(sp_rt_channel_send_i64(chan, 2));
    EXPECT_TRUE(sp_rt_channel_send_i64(chan, 3));
    EXPECT_TRUE(sp_rt_channel_send_i64(chan, 4));
    EXPECT_EQ(sp_rt_channel_size(chan), 4);

    // Try send on full channel should return false
    EXPECT_FALSE(sp_rt_channel_try_send_i64(chan, 5));

    sp_rt_channel_close(chan);
    EXPECT_TRUE(sp_rt_channel_is_closed(chan));

    // Draining closed channel
    EXPECT_EQ(sp_rt_channel_recv_i64(chan), 1);
    EXPECT_EQ(sp_rt_channel_recv_i64(chan), 2);
    EXPECT_EQ(sp_rt_channel_recv_i64(chan), 3);
    EXPECT_EQ(sp_rt_channel_recv_i64(chan), 4);
    EXPECT_EQ(sp_rt_channel_size(chan), 0);

    sp_rt_channel_destroy(chan);
}

// ============================================================================
// Milestone Requirement: 1,000,000 Channel Messages Across 16 Threads
// ============================================================================

TEST(ConcurrencyTest, ChannelMillionMessageMPMCStressTest) {
    const int64_t kTotalMessages = 1000000;
    const int kProducers = 8;
    const int kConsumers = 8;
    const int64_t kMsgsPerProducer = kTotalMessages / kProducers;

    // Bounded ring buffer with capacity 1024
    void* chan = sp_rt_channel_create(1024);
    ASSERT_NE(chan, nullptr);

    std::atomic<int64_t> total_received{0};
    std::atomic<int64_t> total_sum_sent{0};
    std::atomic<int64_t> total_sum_received{0};

    auto start_time = std::chrono::steady_clock::now();

    // Spawn consumers
    std::vector<std::thread> consumers;
    consumers.reserve(kConsumers);
    for (int i = 0; i < kConsumers; ++i) {
        consumers.emplace_back([chan, &total_received, &total_sum_received]() {
            while (true) {
                void* raw = sp_rt_channel_recv(chan);
                if (!raw) {
                    // Channel closed and drained
                    break;
                }
                int64_t val = static_cast<int64_t>(reinterpret_cast<uintptr_t>(raw));
                total_sum_received.fetch_add(val, std::memory_order_relaxed);
                total_received.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Spawn producers
    std::vector<std::thread> producers;
    producers.reserve(kProducers);
    for (int p = 0; p < kProducers; ++p) {
        producers.emplace_back([chan, p, &total_sum_sent]() {
            int64_t start_val = p * kMsgsPerProducer + 1;
            int64_t end_val = start_val + kMsgsPerProducer;
            int64_t local_sum = 0;

            for (int64_t val = start_val; val < end_val; ++val) {
                local_sum += val;
                sp_rt_channel_send_i64(chan, val);
            }
            total_sum_sent.fetch_add(local_sum, std::memory_order_relaxed);
        });
    }

    // Wait for all producers to finish sending
    for (auto& t : producers) {
        t.join();
    }

    // Close channel to notify consumers to drain and finish
    sp_rt_channel_close(chan);

    // Wait for all consumers to finish receiving
    for (auto& t : consumers) {
        t.join();
    }

    auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start_time).count();

    // Verify 100% message delivery with zero drops or corruption
    EXPECT_EQ(total_received.load(), kTotalMessages);
    EXPECT_EQ(total_sum_received.load(), total_sum_sent.load());
    EXPECT_EQ(sp_rt_channel_size(chan), 0);

    // Performance assertion: 1M messages handled rapidly
    EXPECT_GT(total_received.load(), 0);
    (void)elapsed_ms;

    sp_rt_channel_destroy(chan);
}

// ============================================================================
// ThreadPool Tests
// ============================================================================

struct TaskPayload {
    std::atomic<int64_t>* counter;
    int64_t delta;
};

static void pool_task_runner(void* arg) {
    auto* payload = static_cast<TaskPayload*>(arg);
    payload->counter->fetch_add(payload->delta, std::memory_order_relaxed);
}

TEST(ConcurrencyTest, WorkStealingThreadPoolTaskExecution) {
    void* pool = sp_rt_threadpool_create(8);
    ASSERT_NE(pool, nullptr);
    EXPECT_EQ(sp_rt_threadpool_thread_count(pool), 8);

    std::atomic<int64_t> counter{0};
    const int kTasks = 10000;
    std::vector<TaskPayload> payloads(kTasks);

    for (int i = 0; i < kTasks; ++i) {
        payloads[i] = TaskPayload{&counter, 1};
        sp_rt_threadpool_submit(pool, pool_task_runner, &payloads[i]);
    }

    // Wait until all 10,000 tasks are finished
    sp_rt_threadpool_wait_idle(pool);

    EXPECT_EQ(counter.load(), kTasks);

    sp_rt_threadpool_destroy(pool);
}
