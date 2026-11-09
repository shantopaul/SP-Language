module std.async;

// ============================================================================
// Standard Library Async / Await (SP 3.1, Phase 3 slice C — pro-4.0.0)
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
//         var v = await t;                 // pumps until it completes
//         return v;
//     }
//
// `fn main` drains any tasks still pending when it returns, so a
// fire-and-forget call from main still finishes before the process
// exits (timers/queued tasks only — a registered socket does not keep
// the drain going).
//
// Payloads a Task can carry: void, int, bool, string. Anything else
// (double, arrays, ...) fails loudly at compile time with error[E1000].
//
// NOT in this module yet (future work, deliberately not faked here):
// Task combinators (all / race / timeout), cancellation handles, and
// async I/O primitives (async HTTP/fetch). Awaiting across a function
// that returns before completion is not supported either: the awaiting
// frame stays on the stack while pumping (pump-based await, not a
// heap-frame state machine).

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
}
