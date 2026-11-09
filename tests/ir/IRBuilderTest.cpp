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

// Phase 2 (pro-4.0.0, parent fix): a bare function name used as a value
// (e.g. passed as a callback) must fail loudly instead of silently
// lowering to null. The old IdentifierExpr fallback turned ANY unresolved
// identifier into ConstantInt 0, so a queue/cron/ws callback silently
// became a null pointer the runtime bridge ignored (job in DLQ, no error
// anywhere). Function pointers are not lowered yet; until they are, this
// is error[E1000].
TEST(IRBuilderTest, BareFunctionNameAsValueFailsIRGeneration) {
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

    EXPECT_EQ(compileToIR(source), nullptr);
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

TEST(IRBuilderTest, AwaitOnUserAsyncFnCallStillFailsIRGeneration) {
    const std::string source = R"(
        async fn inner() {
        }

        async fn worker() {
            await inner();
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
