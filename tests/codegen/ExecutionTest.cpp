#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace sp;

namespace {

fs::path getOutputDir() {
#ifdef SP_PROJECT_ROOT
    fs::path p = fs::path(SP_PROJECT_ROOT) / "build";
    if (fs::exists(p)) return p;
#endif
    return fs::temp_directory_path();
}

std::string runSPProgram(const std::string& source, const std::string& testName) {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, testName + ".sp", &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return "LEXER_ERROR";

    parser::Parser parser(tokens, testName + ".sp", &diags);
    auto program = parser.parseProgram();
    if (diags.hasErrors() || !program) return "PARSER_ERROR";

    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) return "SEMA_ERROR";

    ir::IRBuilder builder(sema);
    auto module = builder.build(*program, testName);
    if (!module) return "IR_ERROR";

    fs::path exePath = getOutputDir() / (testName + ".exe");
    codegen::CodegenOptions opts;
    codegen::LLVMGenerator generator(opts);

    if (!generator.compileToExecutable(*module, exePath.string())) {
        return "CODEGEN_ERROR: " + generator.getLastError();
    }

    std::string outPath = exePath.string() + ".out.txt";
    std::string cmd = "\"" + exePath.string() + "\" > \"" + outPath + "\" 2>&1";
#if defined(_WIN32)
    std::string fullCmd = "\"" + cmd + "\"";
    int code = std::system(fullCmd.c_str());
#else
    int code = std::system(cmd.c_str());
#endif
    (void)code;

    std::ifstream in(outPath);
    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();

    std::error_code ec;
    fs::remove(outPath, ec);
    fs::remove(exePath, ec);
    return ss.str();
}

} // namespace

TEST(ExecutionTest, HelloNativeExecutionMilestone) {
    const std::string source = R"(
        fn main() {
            println("Hello, SP!");
        }
    )";

    std::string output = runSPProgram(source, "exec_hello");
    EXPECT_NE(output.find("Hello, SP!"), std::string::npos);
}

TEST(ExecutionTest, BasicArithmeticAndVariables) {
    const std::string source = R"(
        fn main() {
            var a = 15;
            var b = 7;
            println(a + b);
            println(a - b);
            println(a * b);
            println(a / b);
            println(a % b);
        }
    )";

    std::string output = runSPProgram(source, "exec_arith");
    EXPECT_NE(output.find("22"), std::string::npos);
    EXPECT_NE(output.find("8"), std::string::npos);
    EXPECT_NE(output.find("105"), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("1"), std::string::npos);
}

TEST(ExecutionTest, BooleanAndRelationalLogic) {
    const std::string source = R"(
        fn main() {
            var t = true;
            var f = false;
            println(t && f);
            println(t || f);
            println(!f);
            println(10 > 5);
            println(10 == 20);
        }
    )";

    std::string output = runSPProgram(source, "exec_bool");
    EXPECT_NE(output.find("false"), std::string::npos);
    EXPECT_NE(output.find("true"), std::string::npos);
}

TEST(ExecutionTest, ConditionalBranchingAndWhileLoop) {
    const std::string source = R"(
        fn sum_to(n: int) -> int {
            var s = 0;
            var i = 1;
            while (i <= n) {
                s = s + i;
                i = i + 1;
            }
            return s;
        }

        fn main() {
            println(sum_to(10));
        }
    )";

    std::string output = runSPProgram(source, "exec_loop");
    // sum from 1 to 10 is 55
    EXPECT_NE(output.find("55"), std::string::npos);
}

TEST(ExecutionTest, RecursiveFibonacciExecution) {
    const std::string source = R"(
        fn fibonacci(n: int) -> int {
            if (n <= 0) return 0;
            if (n == 1) return 1;
            return fibonacci(n - 1) + fibonacci(n - 2);
        }

        fn main() {
            println(fibonacci(10));
        }
    )";

    std::string output = runSPProgram(source, "exec_fib");
    // fib(10) = 55
    EXPECT_NE(output.find("55"), std::string::npos);
}

TEST(ExecutionTest, ShortFormFunctionExecution) {
    const std::string source = R"(
        fn square(x: int) -> int => x * x;

        fn main() {
            println(square(6));
        }
    )";

    std::string output = runSPProgram(source, "exec_short_fn");
    EXPECT_NE(output.find("36"), std::string::npos);
}

TEST(ExecutionTest, FloatingPointOperations) {
    const std::string source = R"(
        fn main() {
            var x = 3.5;
            var y = 2.5;
            println(x * y);
        }
    )";

    std::string output = runSPProgram(source, "exec_float");
    EXPECT_NE(output.find("8.75"), std::string::npos);
}

TEST(ExecutionTest, RuntimeAssertSuccess) {
    const std::string source = R"(
        fn main() {
            assert(true, "Condition should hold");
            println("Assert passed");
        }
    )";

    std::string output = runSPProgram(source, "exec_assert");
    EXPECT_NE(output.find("Assert passed"), std::string::npos);
}
