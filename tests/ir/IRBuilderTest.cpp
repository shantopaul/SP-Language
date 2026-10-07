#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRPrinter.h"

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
