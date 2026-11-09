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
