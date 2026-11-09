#include <gtest/gtest.h>
#include "sp/runtime/Async.h"
#include "sp/runtime/Runtime.h"

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

using namespace sp::rt::async;
using Clock = std::chrono::steady_clock;

namespace {
int64_t elapsedMs(Clock::time_point start) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count();
}
} // namespace

// Phase 2 slice B2 (pro-4.0.0): the scheduler fires timers, tasks
// complete with the right value, and — the point of the slice — awaiting
// a task does NOT block the loop: other timers fire while the await is
// outstanding. Timing bounds below are deliberately loose (loaded CI
// machines) and assert ordering / lower bounds, not exact durations.

// A delayed-value task completes with its value after (at least) the delay.
TEST(AsyncTest, DelayedValueTaskCompletesWithValue) {
    Scheduler sched;
    auto start = Clock::now();
    auto task = delayedValueTask(sched, 50, 42);
    ASSERT_NE(task, nullptr);
    EXPECT_FALSE(task->isReady());

    sched.runUntilIdle();

    EXPECT_TRUE(task->isReady());
    EXPECT_EQ(task->value(), 42);
    EXPECT_FALSE(task->wasCancelled());
    EXPECT_GE(elapsedMs(start), 40); // waited out (at least most of) the delay
    EXPECT_FALSE(sched.hasPending());
}

// A sleep task completes (value 0) and the scheduler drains.
TEST(AsyncTest, SleepTaskCompletes) {
    Scheduler sched;
    auto task = sleepTask(sched, 20);
    ASSERT_NE(task, nullptr);
    sched.runUntilIdle();
    EXPECT_TRUE(task->isReady());
    EXPECT_EQ(task->value(), 0);
    EXPECT_FALSE(sched.hasPending());
}

// Timers fire in due order, not in registration order.
TEST(AsyncTest, TimersFireInDueOrder) {
    Scheduler sched;
    std::vector<std::string> order;
    sched.scheduleAfter(30, [&] { order.push_back("thirty"); });
    sched.scheduleAfter(10, [&] { order.push_back("ten"); });
    sched.scheduleAfter(20, [&] { order.push_back("twenty"); });
    sched.runUntilIdle();
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], "ten");
    EXPECT_EQ(order[1], "twenty");
    EXPECT_EQ(order[2], "thirty");
}

// HEADLINE: while one task is being awaited, the loop is NOT blocked —
// a second, shorter timer fires DURING the await, before the awaited
// task completes. Asserted on ordering and timestamps, not just the
// final value: if await blocked the loop (e.g. a raw sleep), the early
// timer could only fire after the await returned.
TEST(AsyncTest, LoopNotBlockedWhileAwaiting) {
    Scheduler sched;
    auto start = Clock::now();
    std::vector<std::string> order;
    int64_t earlyFiredAtMs = -1;
    int64_t awaitReturnedAtMs = -1;

    sched.scheduleAfter(20, [&] {
        earlyFiredAtMs = elapsedMs(start);
        order.push_back("early");
    });
    auto task = delayedValueTask(sched, 150, 7);
    ASSERT_NE(task, nullptr);

    // This is what generated `await` lowers to: pump the loop until the
    // task completes.
    int64_t value = awaitTask(sched, *task);
    awaitReturnedAtMs = elapsedMs(start);
    order.push_back("awaited");

    EXPECT_EQ(value, 7);
    ASSERT_EQ(order.size(), 2u);
    EXPECT_EQ(order[0], "early");   // early timer fired first ...
    EXPECT_EQ(order[1], "awaited"); // ... and the await returned after it
    EXPECT_GE(earlyFiredAtMs, 0);
    EXPECT_LT(earlyFiredAtMs, 120); // early fired well before the 150 ms task
    EXPECT_LT(earlyFiredAtMs, awaitReturnedAtMs);
    EXPECT_GE(awaitReturnedAtMs, 120); // the await really waited out the delay
}

// A posted callback runs on the next pump, before a not-yet-due timer.
TEST(AsyncTest, PostedTaskRunsBeforeLongTimer) {
    Scheduler sched;
    bool postedRan = false;
    bool timerRan = false;
    sched.post([&] { postedRan = true; });
    sched.scheduleAfter(200, [&] { timerRan = true; });

    EXPECT_TRUE(sched.runOnce());
    EXPECT_TRUE(postedRan);
    EXPECT_FALSE(timerRan);
    EXPECT_TRUE(sched.hasPending()); // the 200 ms timer is still pending
}

// Cancelling a pending timer means its callback never runs and the
// scheduler drains immediately instead of waiting out the delay.
TEST(AsyncTest, CancelledTimerNeverFires) {
    Scheduler sched;
    bool ran = false;
    uint64_t id = sched.scheduleAfter(5000, [&] { ran = true; });
    EXPECT_TRUE(sched.cancel(id));
    EXPECT_FALSE(sched.cancel(id)); // second cancel: already cancelled

    auto start = Clock::now();
    sched.runUntilIdle();
    EXPECT_FALSE(ran);
    EXPECT_FALSE(sched.hasPending());
    EXPECT_LT(elapsedMs(start), 1000);
}

// Cancelling a task makes an outstanding await return promptly (value 0,
// wasCancelled) instead of waiting out the timer.
TEST(AsyncTest, CancelledTaskAwaitReturnsPromptly) {
    Scheduler sched;
    auto task = delayedValueTask(sched, 5000, 99);
    ASSERT_NE(task, nullptr);
    EXPECT_TRUE(task->cancel());
    EXPECT_TRUE(task->isReady());
    EXPECT_TRUE(task->wasCancelled());

    auto start = Clock::now();
    int64_t value = awaitTask(sched, *task);
    EXPECT_EQ(value, 0);
    EXPECT_LT(elapsedMs(start), 1000);
}

// runFor() stops at its deadline: a long timer has not fired yet and is
// still pending afterwards. The timer (2000 ms) is far beyond the runFor
// bound (30 ms) on purpose: on a heavily loaded machine a 30 ms sleep
// can overshoot by hundreds of ms, so the margin must dwarf the overshoot.
TEST(AsyncTest, RunForStopsAtDeadline) {
    Scheduler sched;
    bool ran = false;
    sched.scheduleAfter(2000, [&] { ran = true; });
    auto start = Clock::now();
    sched.runFor(30);
    EXPECT_FALSE(ran);
    EXPECT_TRUE(sched.hasPending());
    EXPECT_GE(elapsedMs(start), 0);
    EXPECT_LT(elapsedMs(start), 1500);
}

// A continuation registered on a task runs when the task completes, with
// the task's value; one registered after completion runs immediately.
TEST(AsyncTest, TaskContinuationRunsOnComplete) {
    Scheduler sched;
    auto task = delayedValueTask(sched, 10, 5);
    int64_t seen = -1;
    int calls = 0;
    task->onComplete([&](Task& t) {
        seen = t.value();
        ++calls;
    });
    sched.runUntilIdle();
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(seen, 5);

    task->onComplete([&](Task&) { ++calls; });
    EXPECT_EQ(calls, 2);
}

// C ABI (what generated SP code calls): default scheduler, sleep +
// await_i64, delayed_i64, is_ready, destroy. No blocking sleep is used
// on this path — the await pumps the default scheduler.
TEST(AsyncTest, CAbiSleepAndDelayedValue) {
    void* sched = sp_rt_async_default_scheduler();
    ASSERT_NE(sched, nullptr);

    void* sleepTaskHandle = sp_rt_async_sleep_ms(sched, 30);
    ASSERT_NE(sleepTaskHandle, nullptr);
    EXPECT_FALSE(sp_rt_async_task_is_ready(sleepTaskHandle));
    auto start = Clock::now();
    sp_rt_async_task_await_void(sched, sleepTaskHandle);
    EXPECT_TRUE(sp_rt_async_task_is_ready(sleepTaskHandle));
    EXPECT_FALSE(sp_rt_async_task_was_cancelled(sleepTaskHandle));
    EXPECT_GE(elapsedMs(start), 20);
    sp_rt_async_task_destroy(sleepTaskHandle);

    void* valueTaskHandle = sp_rt_async_delayed_i64(sched, 50, 1234);
    ASSERT_NE(valueTaskHandle, nullptr);
    start = Clock::now();
    int64_t value = sp_rt_async_task_await_i64(sched, valueTaskHandle);
    EXPECT_EQ(value, 1234);
    EXPECT_GE(elapsedMs(start), 40);
    sp_rt_async_task_destroy(valueTaskHandle);

    // Destroying a null handle and awaiting a null task are safe no-ops.
    sp_rt_async_task_destroy(nullptr);
    EXPECT_EQ(sp_rt_async_task_await_i64(sched, nullptr), 0);
}

// C ABI: cancelling a task makes the await return promptly.
TEST(AsyncTest, CAbiCancelReturnsPromptly) {
    void* sched = sp_rt_async_default_scheduler();
    void* handle = sp_rt_async_delayed_i64(sched, 5000, 1);
    ASSERT_NE(handle, nullptr);
    EXPECT_TRUE(sp_rt_async_task_cancel(handle));
    EXPECT_TRUE(sp_rt_async_task_was_cancelled(handle));
    auto start = Clock::now();
    EXPECT_EQ(sp_rt_async_task_await_i64(sched, handle), 0);
    EXPECT_LT(elapsedMs(start), 1000);
    sp_rt_async_task_destroy(handle);
}
