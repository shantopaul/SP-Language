module std.async;

// ============================================================================
// Standard Library Async / Await (SP 3.1, Phase 5 slice A — pro-4.0.0)
// ============================================================================
//
// Structured async/await with a non-blocking event loop: awaiting never
// blocks the loop — other timers and tasks keep firing while an await is
// outstanding (see the `await` contract in the compiler).
//
// The primitives `sleep_async` and `delayed_value` are builtins and are
// available without importing this module. This module wraps them as
// `async fn`s under the `Async` namespace so programs can write
// `await Async.sleep(ms)` / `await Async.delayInt(ms, value)`:
//
//     import std.async;
//
//     async fn fetch() -> string {
//         await Async.sleep(50);
//         var greeting = await Async.delayString(10, "hello");
//         return greeting;
//     }
//
// Calling an `async fn` returns a Task<T> immediately — the body runs
// on the event loop when something pumps it, and the task completes
// with the body's return value:
//
//     async fn worker() -> int {
//         var t = Async.delayInt(10, 42);  // Task<int>, body not run yet
//         var v = await t;                 // suspends until it completes
//         return v;
//     }
//
// `fn main` drains any tasks still pending when it returns, so a
// fire-and-forget call from main still finishes before the process
// exits (timers/queued tasks only — a registered socket does not keep
// the drain going).
//
// Payloads a Task can carry: void, int, bool, string, double, arrays
// (`int[]`, `string[]`, ...), and structs. A class-typed return still
// fails loudly at compile time with error[E1000] (ARC ownership across
// the task boundary is not implemented).
//
// Awaiting SUSPENDS through a heap-frame state machine (Phase 5 slice
// A): the async fn's locals live in a heap frame, the current segment
// returns at each await, and the continuation resumes in a fresh
// invocation. The stack stays flat (a 5000-deep chain of awaiting
// async fns does not grow the C++ stack), and a task cancelled while
// suspended stops at its next await point instead of resuming.
//
// Combinators (this module, over the runtime `sp_rt_async_task_all_*` /
// `sp_rt_async_task_race_*` C ABI):
//   - `Async.allInt(tasks)` waits for every Task<int> and collects the
//     results into an `int[]` in input order. If any input was
//     cancelled, the combined task is cancelled (and awaiting it yields
//     the default value). An empty input completes immediately with an
//     empty array. `allString` / `allDouble` are the same for string /
//     double payloads; `allVoid` waits for every Task<void>.
//   - `Async.raceInt(tasks)` completes with the FIRST input's value
//     (the first finisher wins). If the first finisher was cancelled,
//     the combined task is cancelled. An empty input completes
//     cancelled. Losing inputs keep running (they are not cancelled).
//     `raceString` / `raceDouble` are the same for string / double.
//
// Cancellation (cooperative — a running segment is never interrupted
// mid-segment):
//   - `task.cancel()` marks a task cancelled and fires its
//     continuations. Returns false when the task already completed.
//   - A task cancelled before its body starts never runs its body.
//   - A body suspended on an await stops at its next resume (it
//     branches to its finish block and frees its frame).
//   - Cancelling the AWAITED task does NOT stop the awaiting body:
//     the await returns promptly with the default value (0 / "" /
//     false / 0.0 / null) and the body continues.
//   - `task.isReady()` / `task.wasCancelled()` report the state.
//
// NOT in this module yet (future work, deliberately not faked here):
// Task timeouts, and async I/O primitives (async HTTP/fetch).
// Awaiting across a function that returns before completion is not
// supported either: an async fn cannot return a Task that outlives
// its own frame — `let t = worker();` without await only runs when
// something pumps, and a task whose body never returns never
// completes.

extern "C" {
    // Task combinators (runtime/src/net/NetBridge.cpp): `tasks` is the
    // SP array object (a Task<T>[] value) and `count` its element
    // count; the element storage pointer is extracted inside the
    // runtime via sp_rt_array_data. Each returns a new task handle.
    // Declared with a Task<T> return type so the result can be
    // awaited directly in SP. (Declared per payload kind with distinct
    // names — not as overloads of one name — because an overloaded
    // extern is name-mangled with a `__<paramTypes>` suffix, so the
    // call would target a name the runtime does not define.)
    fn sp_rt_async_task_all_i64(Task<int>[] tasks, int64 count) -> Task<int[]>;
    fn sp_rt_async_task_all_str(Task<string>[] tasks, int64 count) -> Task<string[]>;
    fn sp_rt_async_task_all_dbl(Task<double>[] tasks, int64 count) -> Task<double[]>;
    fn sp_rt_async_task_all_void(Task<void>[] tasks, int64 count) -> Task<void>;
    fn sp_rt_async_task_race_i64(Task<int>[] tasks, int64 count) -> Task<int>;
    fn sp_rt_async_task_race_str(Task<string>[] tasks, int64 count) -> Task<string>;
    fn sp_rt_async_task_race_dbl(Task<double>[] tasks, int64 count) -> Task<double>;
}

// Sleep without blocking the loop: yields for `ms`, then continues.
public class Async {
    public static async fn sleep(int ms) {
        await sleep_async(ms);
    }

    // Complete with `value` after `ms` (int payload).
    public static async fn delayInt(int ms, int value) -> int {
        var v = await delayed_value(ms, value);
        return v;
    }

    // Complete with `value` after `ms` (string payload).
    public static async fn delayString(int ms, string value) -> string {
        var s = await delayed_value(ms, value);
        return s;
    }

    // Complete with `value` after `ms` (bool payload).
    public static async fn delayBool(int ms, bool value) -> bool {
        var b = await delayed_value(ms, value);
        return b;
    }

    // Complete with `value` after `ms` (double payload).
    public static async fn delayDouble(int ms, double value) -> double {
        var d = await delayed_value(ms, value);
        return d;
    }

    // Wait for every Task<int> and collect the results in input order.
    // Cancelled when any input was cancelled; empty input completes
    // immediately with an empty array.
    public static async fn allInt(Task<int>[] tasks) -> int[] {
        var t = sp_rt_async_task_all_i64(tasks, tasks.length);
        var results = await t;
        return results;
    }

    // First finisher wins. Cancelled when the first finisher was
    // cancelled; empty input completes cancelled. Losers keep running.
    public static async fn raceInt(Task<int>[] tasks) -> int {
        var t = sp_rt_async_task_race_i64(tasks, tasks.length);
        var v = await t;
        return v;
    }

    // Wait for every Task<string> and collect the results in input order.
    public static async fn allString(Task<string>[] tasks) -> string[] {
        var t = sp_rt_async_task_all_str(tasks, tasks.length);
        var results = await t;
        return results;
    }

    // First finisher wins (string payload).
    public static async fn raceString(Task<string>[] tasks) -> string {
        var t = sp_rt_async_task_race_str(tasks, tasks.length);
        var v = await t;
        return v;
    }

    // Wait for every Task<double> and collect the results in input order.
    public static async fn allDouble(Task<double>[] tasks) -> double[] {
        var t = sp_rt_async_task_all_dbl(tasks, tasks.length);
        var results = await t;
        return results;
    }

    // First finisher wins (double payload).
    public static async fn raceDouble(Task<double>[] tasks) -> double {
        var t = sp_rt_async_task_race_dbl(tasks, tasks.length);
        var v = await t;
        return v;
    }

    // Wait for every Task<void> to complete.
    public static async fn allVoid(Task<void>[] tasks) {
        var t = sp_rt_async_task_all_void(tasks, tasks.length);
        await t;
    }
}
