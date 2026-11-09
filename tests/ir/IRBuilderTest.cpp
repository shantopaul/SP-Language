#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRPrinter.h"
#include "sp/ir/IRVerifier.h"

using namespace sp;

namespace {

std::unique_ptr<ir::IRModule> compileToIR(const std::string& source, const std::string& filename = "test.sp") {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, filename, &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return nullptr;

    parser::Parser parser(tokens, filename, &diags);
    auto program = parser.parseProgram();
    if (diags.hasErrors() || !program) return nullptr;

    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) return nullptr;

    ir::IRBuilder builder(sema);
    return builder.build(*program, "test");
}

} // namespace

TEST(IRBuilderTest, HelloSPGeneratesMainAndPrintlnCall) {
    const std::string source = R"(
        fn main() {
            println("Hello, SP!");
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @main() {"), std::string::npos);
    EXPECT_NE(irStr.find("call void @sp_rt_println_string(ptr @.str.0)"), std::string::npos);
    EXPECT_NE(irStr.find("ret i32 0"), std::string::npos);
}

TEST(IRBuilderTest, ArithmeticAndVariables) {
    const std::string source = R"(
        fn main() -> int {
            var a = 10;
            var b = 20;
            var c = a + b * 2;
            return c;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @main() {"), std::string::npos);
    EXPECT_NE(irStr.find("alloca i32"), std::string::npos);
    EXPECT_NE(irStr.find("mul i32"), std::string::npos);
    EXPECT_NE(irStr.find("add i32"), std::string::npos);
    EXPECT_NE(irStr.find("ret i32"), std::string::npos);
}

TEST(IRBuilderTest, IfElseControlFlow) {
    const std::string source = R"(
        fn test_if(x: int) -> int {
            if (x > 0) {
                return 1;
            } else {
                return -1;
            }
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @test_if(i32 %x) {"), std::string::npos);
    EXPECT_NE(irStr.find("icmp sgt i32"), std::string::npos);
    EXPECT_NE(irStr.find("br i1"), std::string::npos);
    EXPECT_NE(irStr.find("then_"), std::string::npos);
    EXPECT_NE(irStr.find("else_"), std::string::npos);
}

TEST(IRBuilderTest, WhileLoopControlFlow) {
    const std::string source = R"(
        fn count_down(n: int) {
            var i = n;
            while (i > 0) {
                i = i - 1;
            }
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define void @count_down(i32 %n) {"), std::string::npos);
    EXPECT_NE(irStr.find("while_cond_"), std::string::npos);
    EXPECT_NE(irStr.find("while_body_"), std::string::npos);
    EXPECT_NE(irStr.find("while_end_"), std::string::npos);
    EXPECT_NE(irStr.find("br i1"), std::string::npos);
}

TEST(IRBuilderTest, FibonacciRecursiveFunction) {
    const std::string source = R"(
        fn fib(n: int) -> int {
            if (n <= 1) {
                return n;
            }
            return fib(n - 1) + fib(n - 2);
        }

        fn main() {
            println(fib(10));
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @fib(i32 %n) {"), std::string::npos);
    EXPECT_NE(irStr.find("call i32 @fib("), std::string::npos);
    EXPECT_NE(irStr.find("call void @sp_rt_println_int("), std::string::npos);
}

TEST(IRBuilderTest, ARCObjectAllocationAndScopeDropEmission) {
    const std::string source = R"(
        class Connection {
            int port;
        }

        fn runSession() {
            var conn = new Connection();
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("sp_rt_arc_alloc"), std::string::npos);
    EXPECT_NE(irStr.find("sp_rt_release"), std::string::npos);
}

TEST(IRBuilderTest, ARCClassAssignmentEmitsRetainAndRelease) {
    const std::string source = R"(
        class Resource {
            int id;
        }

        fn transfer() {
            var r1 = new Resource();
            var r2 = new Resource();
            r2 = r1;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("sp_rt_arc_alloc"), std::string::npos);
    EXPECT_NE(irStr.find("sp_rt_retain"), std::string::npos);
    EXPECT_NE(irStr.find("sp_rt_release"), std::string::npos);
}

// Phase 1 (pro-4.0.0): && must short-circuit. The RHS lives in its own
// block, reached only when the LHS is true, and no eager "and i1" is
// emitted. The finished module must also pass IR verification.
TEST(IRBuilderTest, ShortCircuitAndSkipsRHS) {
    const std::string source = R"(
        fn check(a: bool, b: bool) -> bool {
            return a && b;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("and_rhs_"), std::string::npos);
    EXPECT_NE(irStr.find("and_merge_"), std::string::npos);
    EXPECT_NE(irStr.find("br i1"), std::string::npos);
    EXPECT_EQ(irStr.find("and i1"), std::string::npos);
}

// Phase 1 (pro-4.0.0): || must short-circuit the same way: the RHS block
// is reached only when the LHS is false.
TEST(IRBuilderTest, ShortCircuitOrSkipsRHS) {
    const std::string source = R"(
        fn check(a: bool, b: bool) -> bool {
            return a || b;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("or_rhs_"), std::string::npos);
    EXPECT_NE(irStr.find("or_merge_"), std::string::npos);
    EXPECT_NE(irStr.find("br i1"), std::string::npos);
    EXPECT_EQ(irStr.find("or i1"), std::string::npos);
}

// Phase 1 (pro-4.0.0): constructs with no lowering must fail compilation
// loudly (nullptr from build()) instead of being silently dropped. These
// programs all pass lexing/parsing/semantic analysis; the rejection comes
// from IRBuilder.
TEST(IRBuilderTest, ThrowStatementFailsIRGeneration) {
    const std::string source = R"(
        fn main() {
            throw "boom";
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

TEST(IRBuilderTest, TryCatchStatementFailsIRGeneration) {
    const std::string source = R"(
        fn main() {
            try {
                println("try");
            } catch (Exception e) {
                println("caught");
            }
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

TEST(IRBuilderTest, MapLiteralFailsIRGeneration) {
    const std::string source = R"(
        fn main() {
            var m = {"a": 1};
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

TEST(IRBuilderTest, AwaitExpressionFailsIRGeneration) {
    const std::string source = R"(
        async fn worker() {
            await 1;
        }

        fn main() {
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Phase 3 slice A (pro-4.0.0): a bare free function used as a value now
// lowers to its real address (this replaces the Phase 2 loud error, which
// fired because function pointers were not lowered yet). The IR must pass
// `@handler` itself, not a null constant, to the extern registration
// function — a null callback is what silently sent queue jobs to the DLQ.
TEST(IRBuilderTest, BareFunctionNameAsValueLowersToRealAddress) {
    const std::string source = R"(
        extern "C" {
            fn fake_register(ptr handler);
        }

        fn handler() {
            println("hi");
        }

        fn main() {
            fake_register(handler);
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define void @handler()"), std::string::npos);
    EXPECT_NE(irStr.find("call void @fake_register(ptr @handler)"), std::string::npos);
}

// Phase 3 slice A: a bare function assigned to a `ptr` variable also
// lowers to its address, and calling through the variable works.
TEST(IRBuilderTest, FunctionPointerAssignedToPtrVariable) {
    const std::string source = R"(
        fn doubleIt(int x) -> int {
            return x * 2;
        }

        fn main() -> int {
            let f = doubleIt;
            return f(21);
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @doubleIt(i32 %x)"), std::string::npos);
    // The variable holds the address, not a null constant.
    EXPECT_NE(irStr.find("@doubleIt"), std::string::npos);
}

// Phase 3 slice A: same-name free functions with different signatures
// coexist (mangled by signature) and each call site resolves to the
// right one. Single-definition functions keep their plain names (see
// the fib test above), so only overloaded names gain a suffix.
TEST(IRBuilderTest, OverloadedFreeFunctionsBothCompileAndResolve) {
    const std::string source = R"(
        fn add(int a, int b) -> int {
            return a + b;
        }

        fn add(int a, int b, int c) -> int {
            return a + b + c;
        }

        fn main() -> int {
            let x = add(1, 2);
            let y = add(1, 2, 3);
            return x + y;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @add__int_int(i32 %a, i32 %b)"), std::string::npos);
    EXPECT_NE(irStr.find("define i32 @add__int_int_int(i32 %a, i32 %b, i32 %c)"), std::string::npos);
    EXPECT_NE(irStr.find("call i32 @add__int_int("), std::string::npos);
    EXPECT_NE(irStr.find("call i32 @add__int_int_int("), std::string::npos);
    // No plain-`@add(` definition or call may remain.
    EXPECT_EQ(irStr.find("define i32 @add("), std::string::npos);
    EXPECT_EQ(irStr.find("call i32 @add("), std::string::npos);
}

// Phase 3 slice A: same-name overloads differing by parameter TYPE
// (not just arity) also mangle and resolve correctly.
TEST(IRBuilderTest, OverloadedFreeFunctionsByParamType) {
    const std::string source = R"(
        fn describe(int x) -> int {
            return x;
        }

        fn describe(string s) -> int {
            return 42;
        }

        fn main() -> int {
            let a = describe(7);
            let b = describe("hi");
            return a + b;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @describe__int(i32 %x)"), std::string::npos);
    EXPECT_NE(irStr.find("define i32 @describe__string(ptr %s)"), std::string::npos);
    EXPECT_NE(irStr.find("call i32 @describe__int("), std::string::npos);
    EXPECT_NE(irStr.find("call i32 @describe__string("), std::string::npos);
}

// Phase 3 slice A: same-name methods with different signatures coexist
// on one class and virtual dispatch picks the right slot.
TEST(IRBuilderTest, OverloadedMethodsBothCompileAndResolve) {
    const std::string source = R"(
        class Greeter {
            public fn greet(int times) -> int {
                return times;
            }

            public fn greet(string name) -> int {
                return 1;
            }
        }

        fn main() -> int {
            let g = new Greeter();
            let a = g.greet(3);
            let b = g.greet("shanto");
            return a + b;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("define i32 @Greeter_greet__int("), std::string::npos);
    EXPECT_NE(irStr.find("define i32 @Greeter_greet__string("), std::string::npos);
    // Both overloads occupy vtable slots (two distinct function pointers).
    EXPECT_NE(irStr.find("@Greeter_greet__int"), std::string::npos);
    EXPECT_NE(irStr.find("@Greeter_greet__string"), std::string::npos);
}

// Phase 3 slice A: a bare OVERLOADED name as a value is still ambiguous
// (SP has only untyped `ptr`, so no overload can be picked) and must
// fail loudly instead of silently becoming null.
TEST(IRBuilderTest, OverloadedBareNameAsValueStillFailsIRGeneration) {
    const std::string source = R"(
        extern "C" {
            fn fake_register(ptr handler);
        }

        fn handler(int x) -> int {
            return x;
        }

        fn handler(string s) -> int {
            return 0;
        }

        fn main() {
            fake_register(handler);
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Phase 3 slice A: a bare method name as a value (needs an instance)
// still fails loudly.
TEST(IRBuilderTest, BareMethodNameAsValueStillFailsIRGeneration) {
    const std::string source = R"(
        extern "C" {
            fn fake_register(ptr handler);
        }

        class Worker {
            public fn run() {
                fake_register(run);
            }
        }

        fn main() {
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Phase 3 slice A: a lambda that captures an outer variable still fails
// loudly (capturing lambdas are not lowered; previously the captured
// variable silently became 0 inside the lambda body).
TEST(IRBuilderTest, CapturingLambdaAsValueStillFailsIRGeneration) {
    const std::string source = R"(
        fn main() -> int {
            let base = 10;
            let f = (int x) -> int { return x + base; };
            return f(5);
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Phase 3 slice A: a non-capturing lambda still compiles (control case
// for the capture check above).
TEST(IRBuilderTest, NonCapturingLambdaStillCompiles) {
    // Parent fix (Phase 3): slice A wrote the lambda with `-> int`
    // syntax, which the parser rejects — lambdas use a fat arrow:
    // `(int x) => { ... }` (see Parser::parseGroupedOrLambda).
    const std::string source = R"(
        fn main() -> int {
            let f = (int x) => { return x * 2; };
            return f(21);
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
}

// Phase 2 (pro-4.0.0, parent fix): a `char` passed where a string method
// expects a string must fail loudly. The old lowering passed the raw char
// value as a `ptr` to strstr etc. — `"abc".indexOf('b')` segfaulted the
// compiled program (char value 0x62 used as a needle pointer). Sema does
// not check these intrinsic string-method arguments, so IRBuilder guards
// them with error[E1000].
TEST(IRBuilderTest, StringIndexOfWithCharArgFailsIRGeneration) {
    const std::string source = R"(
        fn main() {
            let s = "abc";
            let i = s.indexOf('b');
            println(i);
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

TEST(IRBuilderTest, StringContainsWithCharArgFailsIRGeneration) {
    const std::string source = R"(
        fn main() {
            let s = "abc";
            if (s.contains('b')) {
                println("yes");
            }
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Positive control for the guard above: the same calls with real strings
// must still compile.
TEST(IRBuilderTest, StringIndexOfWithStringArgStillCompiles) {
    const std::string source = R"(
        fn main() {
            let s = "abc";
            let i = s.indexOf("b");
            println(i);
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
}

// Phase 2 slice B2 (pro-4.0.0): `await sleep_async(ms)` inside an async
// fn now lowers instead of failing. The emitted IR must call the
// non-blocking async runtime (create task, pump-await it, destroy it)
// and must NOT call the blocking sleep.
TEST(IRBuilderTest, AwaitSleepAsyncLowersToAsyncRuntime) {
    const std::string source = R"(
        async fn worker() {
            await sleep_async(50);
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call ptr @sp_rt_async_default_scheduler()"), std::string::npos);
    EXPECT_NE(irStr.find("call ptr @sp_rt_async_sleep_ms(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call void @sp_rt_async_task_await_void(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call void @sp_rt_async_task_destroy(ptr"), std::string::npos);
    // The module still *declares* the blocking sleeps (they are part of
    // the runtime declarations), but the await path must not *call* them.
    EXPECT_EQ(irStr.find("call void @sp_rt_time_sleep_ms"), std::string::npos);
    EXPECT_EQ(irStr.find("call void @sp_rt_thread_sleep_ms"), std::string::npos);
}

// Phase 2 slice B2: `await delayed_value(ms, value)` lowers and yields
// the int value, so it can initialize a variable and be returned.
TEST(IRBuilderTest, AwaitDelayedValueLowersAndYieldsInt) {
    const std::string source = R"(
        async fn worker() -> int {
            var x = await delayed_value(50, 42);
            return x;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call ptr @sp_rt_async_delayed_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("ret i32"), std::string::npos); // SP `int` is i32
    EXPECT_EQ(irStr.find("call void @sp_rt_time_sleep_ms"), std::string::npos);
}

// Phase 2 slice B2: pump-based lowering works inside control flow too —
// an await in a loop body is still just calls in the loop's block.
TEST(IRBuilderTest, AwaitSleepAsyncInsideLoopLowers) {
    const std::string source = R"(
        async fn worker() {
            var i = 0;
            while (i < 3) {
                await sleep_async(10);
                i = i + 1;
            }
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);
    EXPECT_NE(irStr.find("sp_rt_async_sleep_ms"), std::string::npos);
    EXPECT_NE(irStr.find("while_body_"), std::string::npos);
}

// Phase 2 slice B2: awaiting anything outside the supported subset still
// fails loudly (nullptr) instead of miscompiling: a sync call, an
// identifier, and a call to another (user) async fn.
TEST(IRBuilderTest, AwaitOnSyncCallStillFailsIRGeneration) {
    const std::string source = R"(
        async fn worker() {
            await now();
        }

        fn main() {
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

TEST(IRBuilderTest, AwaitOnIdentifierStillFailsIRGeneration) {
    const std::string source = R"(
        async fn worker() {
            var x = 1;
            await x;
        }

        fn main() {
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Phase 3 slice C (pro-4.0.0): `await` on a user async fn now LOWERS
// (the Phase 2 test below asserted it fails). Calling the async fn
// spawns its body as a Task (thunk + heap ctx); the await pumps until
// the task completes and destroys the temporary task.
TEST(IRBuilderTest, AwaitOnUserAsyncFnCallLowers) {
    const std::string source = R"(
        async fn inner() -> int {
            return 42;
        }

        async fn worker() -> int {
            var v = await inner();
            return v;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    // The call spawns (create_task + spawn_i64 with the thunk), the
    // await pumps and destroys the temporary task.
    EXPECT_NE(irStr.find("call ptr @sp_rt_async_create_task()"), std::string::npos);
    EXPECT_NE(irStr.find("call void @sp_rt_async_spawn_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("@__sp_async_thunk_inner"), std::string::npos);
    EXPECT_NE(irStr.find("define i64 @__sp_async_thunk_inner(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call void @sp_rt_async_task_destroy(ptr"), std::string::npos);
    EXPECT_EQ(irStr.find("call void @sp_rt_time_sleep_ms"), std::string::npos);
}

// Phase 3 slice C: calling an async fn WITHOUT awaiting returns the
// Task handle (no await is emitted); main's exit drains it.
TEST(IRBuilderTest, CallToAsyncFnReturnsTaskWithoutAwait) {
    const std::string source = R"(
        async fn inner() -> int {
            return 7;
        }

        fn main() {
            var t = inner();
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call ptr @sp_rt_async_create_task()"), std::string::npos);
    EXPECT_NE(irStr.find("call void @sp_rt_async_spawn_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("define i64 @__sp_async_thunk_inner(ptr"), std::string::npos);
    EXPECT_EQ(irStr.find("call i64 @sp_rt_async_task_await_i64"), std::string::npos);
    // main drains fire-and-forget tasks before exiting.
    EXPECT_NE(irStr.find("call void @sp_rt_async_scheduler_run_tasks_until_idle(ptr"), std::string::npos);
}

// Phase 3 slice C: awaiting a Task held in a variable pumps until it
// completes but does NOT destroy it — the variable owns the task, so
// awaiting it twice must return the same value.
TEST(IRBuilderTest, AwaitOnTaskVariableLowersWithoutDestroy) {
    const std::string source = R"(
        async fn inner() -> int {
            return 5;
        }

        async fn worker() -> int {
            var t = inner();
            var v = await t;
            return v;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call void @sp_rt_async_spawn_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
    EXPECT_EQ(irStr.find("call void @sp_rt_async_task_destroy"), std::string::npos);
}

// Phase 3 slice C: string payloads — spawn_str / await_str, and the
// thunk returns the string pointer.
TEST(IRBuilderTest, AwaitOnUserAsyncFnWithStringPayloadLowers) {
    const std::string source = R"(
        async fn greeter() -> string {
            return "hello";
        }

        async fn worker() -> string {
            var s = await greeter();
            return s;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call void @sp_rt_async_spawn_str(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call ptr @sp_rt_async_task_await_str(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("define ptr @__sp_async_thunk_greeter(ptr"), std::string::npos);
}

// Phase 3 slice C: bool payloads ride the i64 task with a trunc to i1.
TEST(IRBuilderTest, AwaitOnUserAsyncFnWithBoolPayloadLowers) {
    const std::string source = R"(
        async fn flag() -> bool {
            return true;
        }

        async fn worker() -> bool {
            var b = await flag();
            return b;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call void @sp_rt_async_spawn_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("trunc i64"), std::string::npos);
}

// Phase 3 slice C: delayed_value string/bool overloads.
TEST(IRBuilderTest, AwaitDelayedValueStringLowers) {
    const std::string source = R"(
        async fn worker() -> string {
            var s = await delayed_value(10, "hi");
            return s;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call ptr @sp_rt_async_delayed_str(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call ptr @sp_rt_async_task_await_str(ptr"), std::string::npos);
}

TEST(IRBuilderTest, AwaitDelayedValueBoolLowers) {
    const std::string source = R"(
        async fn worker() -> bool {
            var b = await delayed_value(10, true);
            return b;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call ptr @sp_rt_async_delayed_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("trunc i64"), std::string::npos);
}

// Phase 3 slice C: an async method called through a virtual dispatch
// spawns through the INDIRECT thunk (vtable fn pointer in ctx slot 0).
TEST(IRBuilderTest, AsyncMethodCallThroughVirtualDispatchLowers) {
    const std::string source = R"(
        class Greeter {
            public async fn greet() -> int {
                return 3;
            }
        }

        async fn worker() -> int {
            let g = new Greeter();
            var v = await g.greet();
            return v;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call void @sp_rt_async_spawn_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("@__sp_async_thunk_indirect_Greeter_greet"), std::string::npos);
    EXPECT_NE(irStr.find("define i64 @__sp_async_thunk_indirect_Greeter_greet(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
}

// Phase 3 slice C: an async method can await inside its own body.
TEST(IRBuilderTest, AwaitInsideAsyncMethodLowers) {
    const std::string source = R"(
        class Worker {
            public async fn run() -> int {
                var v = await delayed_value(10, 9);
                return v;
            }
        }

        fn main() {
            let w = new Worker();
            var t = w.run();
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call ptr @sp_rt_async_delayed_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
    // The method body itself is compiled (not just the thunk).
    EXPECT_NE(irStr.find("define i32 @Worker_run("), std::string::npos);
}

// Phase 3 slice C: generic async fns monomorphize + spawn + await.
TEST(IRBuilderTest, GenericAsyncFnCallLowers) {
    const std::string source = R"(
        async fn identity<T>(item: T) -> T {
            return item;
        }

        async fn worker() -> int {
            var v = await identity<int>(999);
            return v;
        }

        fn main() {
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));

    ir::IRPrinter printer;
    std::string irStr = printer.print(*module);

    EXPECT_NE(irStr.find("call void @sp_rt_async_spawn_i64(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("@__sp_async_thunk_identity_int"), std::string::npos);
    EXPECT_NE(irStr.find("define i64 @__sp_async_thunk_identity_int(ptr"), std::string::npos);
    EXPECT_NE(irStr.find("call i64 @sp_rt_async_task_await_i64(ptr"), std::string::npos);
}

// Phase 3 slice C: still fails loudly — Task<double> cannot be carried.
TEST(IRBuilderTest, AwaitOnAsyncFnWithDoublePayloadStillFailsIRGeneration) {
    const std::string source = R"(
        async fn dbl() -> double {
            return 1.5;
        }

        async fn worker() -> double {
            var v = await dbl();
            return v;
        }

        fn main() {
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

TEST(IRBuilderTest, ComptimeExpressionFailsIRGeneration) {
    const std::string source = R"(
        fn main() {
            var x = comptime 1 + 2;
            println(x);
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Phase 1 (pro-4.0.0): two free functions with the same name but different
// signatures used to collapse onto the first definition silently.
TEST(IRBuilderTest, OverloadedFunctionSignatureClashFailsIRGeneration) {
    const std::string source = R"(
        fn add(a: int) -> int {
            return a;
        }

        fn add(a: string) -> string {
            return a;
        }

        fn main() {
        }
    )";

    EXPECT_EQ(compileToIR(source), nullptr);
}

// Phase 1 (pro-4.0.0): the verifier rejects a block with no terminator.
TEST(IRVerifierTest, UnterminatedBlockIsRejected) {
    ir::IRModule module("test");
    auto* func = module.createFunction("f", ir::IRType::getVoid(), {});
    auto* entry = func->createBasicBlock("entry");
    entry->addInstruction(std::make_shared<ir::AllocaInst>(ir::IRType::getInt32(), "%x"));

    ir::IRVerifier verifier;
    EXPECT_FALSE(verifier.verify(module));
    EXPECT_FALSE(verifier.getErrors().empty());
}

// Phase 1 (pro-4.0.0): the verifier accepts a well-formed module produced
// by the real pipeline.
TEST(IRVerifierTest, WellFormedModulePasses) {
    const std::string source = R"(
        fn main() -> int {
            var a = 10;
            return a + 1;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRVerifier verifier;
    EXPECT_TRUE(verifier.verify(*module));
    EXPECT_TRUE(verifier.getErrors().empty());
}
