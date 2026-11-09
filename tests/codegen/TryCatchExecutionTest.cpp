// TryCatchExecutionTest.cpp — end-to-end tests for Phase 3 slice B
// (pro-4.0.0): real try/catch/finally + throw, the `?` operator, and
// map literals, executed as real native programs via spc (Clang).
// IR-level coverage lives in tests/ir/TryCatchTest.cpp.
//
// The helper below is a local copy of the runSPProgram pattern from
// ExecutionTest.cpp: parse -> sema -> IR -> LLVM -> link -> run the
// executable and return its combined stdout+stderr.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "sp/codegen/LLVMGenerator.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/ir/IRBuilder.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"

namespace fs = std::filesystem;
using namespace sp;

namespace {

fs::path getOutputDir() {
#ifdef SP_PROJECT_ROOT
    fs::path dir = fs::path(SP_PROJECT_ROOT) / "build" / "test_output";
    if (fs::exists(fs::path(SP_PROJECT_ROOT) / "build")) return dir;
#endif
    return fs::temp_directory_path() / "sp_trycatch_exec_tests";
}

std::string runSPProgram(const std::string& source, const std::string& testName, const std::string& input = "") {
    fs::create_directories(getOutputDir());
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, testName + ".sp", &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return "LEXER_ERROR";
    parser::Parser parser(tokens, testName + ".sp", &diags);
    auto program = parser.parseProgram();
    if (!program || diags.hasErrors()) return "PARSER_ERROR";
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
    std::string inPath = exePath.string() + ".in.txt";
    std::string cmd;
    if (!input.empty()) {
        std::ofstream inFile(inPath);
        inFile << input;
        inFile.close();
        cmd = "\"" + exePath.string() + "\" < \"" + inPath + "\" > \"" + outPath + "\" 2>&1";
    } else {
        cmd = "\"" + exePath.string() + "\" > \"" + outPath + "\" 2>&1";
    }
    int code = std::system(cmd.c_str());
    (void)code;
    std::ifstream in(outPath);
    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();

    std::error_code ec;
    fs::remove(outPath, ec);
    fs::remove(inPath, ec);
    fs::remove(exePath, ec);
    return ss.str();
}

} // namespace

// --- throw / catch / finally ----------------------------------------------

TEST(TryCatchExecutionTest, TryCatchFinallyNoThrow) {
    const std::string source = R"(
        fn main() {
            try {
                println("in try");
            } catch (Exception e) {
                println("in catch");
            } finally {
                println("in finally");
            }
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_try_no_throw");
    EXPECT_NE(output.find("in try"), std::string::npos);
    EXPECT_EQ(output.find("in catch"), std::string::npos);
    EXPECT_NE(output.find("in finally"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowCaughtWithValue) {
    const std::string source = R"(
        fn main() {
            try {
                throw "boom";
                println("after throw");
            } catch (Exception e) {
                println("caught: " + e);
            } finally {
                println("fin");
            }
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_caught");
    EXPECT_NE(output.find("caught: boom"), std::string::npos);
    EXPECT_EQ(output.find("after throw"), std::string::npos);
    EXPECT_NE(output.find("fin"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowIntCaughtAsString) {
    const std::string source = R"(
        fn main() {
            try {
                throw 42;
            } catch (Exception e) {
                println("code " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_int");
    EXPECT_NE(output.find("code 42"), std::string::npos);
}

TEST(TryCatchExecutionTest, NestedTryCatch) {
    const std::string source = R"(
        fn main() {
            try {
                try {
                    throw "inner";
                } catch (Exception e) {
                    println("inner catch: " + e);
                }
                println("between");
                throw "outer";
            } catch (Exception e) {
                println("outer catch: " + e);
            }
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_nested_try");
    EXPECT_NE(output.find("inner catch: inner"), std::string::npos);
    EXPECT_NE(output.find("between"), std::string::npos);
    EXPECT_NE(output.find("outer catch: outer"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowInsideCatchPropagatesOutward) {
    const std::string source = R"(
        fn main() {
            try {
                try {
                    throw "first";
                } catch (Exception e) {
                    println("first catch");
                    throw "second";
                }
            } catch (Exception e) {
                println("second catch: " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_rethrow");
    EXPECT_NE(output.find("first catch"), std::string::npos);
    EXPECT_NE(output.find("second catch: second"), std::string::npos);
}

TEST(TryCatchExecutionTest, ReturnInsideTryWithFinallyRunsFinally) {
    const std::string source = R"(
        fn answer() -> int {
            try {
                return 42;
            } finally {
                println("fin");
            }
        }
        fn main() {
            println(answer());
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_return_finally");
    EXPECT_NE(output.find("fin"), std::string::npos);
    EXPECT_NE(output.find("42"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowInResultFunctionBecomesErr) {
    const std::string source = R"(
        fn fail() -> Result<int, string> {
            throw "nope";
        }
        fn main() {
            var r = fail();
            println(r.isErr());
            println(r.unwrapErr());
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_result");
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("nope"), std::string::npos);
}

TEST(TryCatchExecutionTest, UncaughtThrowPanics) {
    const std::string source = R"(
        fn main() {
            throw "fatal";
            println("never");
        }
    )";
    std::string output = runSPProgram(source, "exec_uncaught_throw");
    EXPECT_NE(output.find("SP Panic: fatal"), std::string::npos);
    EXPECT_EQ(output.find("never"), std::string::npos);
}

TEST(TryCatchExecutionTest, Example27TryCatchRuns) {
    // examples/27_try_catch.sp (the headline proof): two try/catch/
    // finally blocks that never throw — try runs, catch skipped,
    // finally runs.
    const std::string source = R"(
        fn divide(int a, int b) -> int {
            if (b == 0) {
                println("Error: Division by zero");
                return 0;
            }
            return a / b;
        }
        fn main() {
            try {
                var result = divide(10, 2);
                println("Result: " + result);
            } catch (Exception e) {
                println("Caught exception: " + e);
            } finally {
                println("Finally block executed");
            }
            try {
                var result2 = divide(10, 0);
                println("Result: " + result2);
            } catch (Exception e) {
                println("Caught exception: " + e);
            } finally {
                println("Finally block executed");
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_example27");
    EXPECT_NE(output.find("Result: 5"), std::string::npos);
    EXPECT_NE(output.find("Error: Division by zero"), std::string::npos);
    EXPECT_NE(output.find("Result: 0"), std::string::npos);
    EXPECT_EQ(output.find("Caught exception"), std::string::npos);
    // "Finally block executed" must appear exactly twice.
    size_t first = output.find("Finally block executed");
    ASSERT_NE(first, std::string::npos);
    size_t second = output.find("Finally block executed", first + 1);
    EXPECT_NE(second, std::string::npos);
    EXPECT_EQ(output.find("Finally block executed", second + 1), std::string::npos);
}

// --- `?` operator (already implemented; end-to-end regression cover) -----

TEST(TryCatchExecutionTest, TryOperatorInReturnPositionAndOption) {
    const std::string source = R"(
        fn maybe() -> Option<string> {
            return Some("hi");
        }
        fn greet() -> Option<string> {
            var name = maybe()?;
            return Some("hello " + name);
        }
        fn main() {
            var g = greet();
            println(g.unwrap());
        }
    )";
    std::string output = runSPProgram(source, "exec_try_opt");
    EXPECT_NE(output.find("hello hi"), std::string::npos);
}

TEST(TryCatchExecutionTest, TryOperatorPropagatesErr) {
    const std::string source = R"(
        fn fail() -> Result<int, string> {
            return Err("bad");
        }
        fn run() -> Result<int, string> {
            var v = fail()?;
            return Ok(v + 1);
        }
        fn main() {
            var r = run();
            println(r.isErr());
            println(r.unwrapErr());
        }
    )";
    std::string output = runSPProgram(source, "exec_try_err");
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("bad"), std::string::npos);
}

// --- map literals ----------------------------------------------------------

TEST(TryCatchExecutionTest, MapLiteralReadsBack) {
    const std::string source = R"(
        fn main() {
            var m = {"a": 1, "b": 2};
            println(m["a"]);
            println(m["b"]);
            println(m.size());
            println(m.contains("a"));
        }
    )";
    std::string output = runSPProgram(source, "exec_map_literal");
    EXPECT_NE(output.find("1"), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("true"), std::string::npos);
}

TEST(TryCatchExecutionTest, MapLiteralIntKeysReadBack) {
    const std::string source = R"(
        fn main() {
            var m = {1: 10, 2: 20};
            println(m[1]);
            println(m[2]);
            println(m.size());
        }
    )";
    std::string output = runSPProgram(source, "exec_map_int_keys");
    EXPECT_NE(output.find("10"), std::string::npos);
    EXPECT_NE(output.find("20"), std::string::npos);
}

TEST(TryCatchExecutionTest, EmptyMapLiteralWorks) {
    const std::string source = R"(
        fn main() {
            var m = {};
            println(m.isEmpty());
            println(m.size());
        }
    )";
    std::string output = runSPProgram(source, "exec_map_empty");
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("0"), std::string::npos);
}
