#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"
#include "sp/runtime/Runtime.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace sp;

namespace {

std::unique_ptr<ir::IRModule> parseAndBuildIR(const std::string& source) {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, "test.sp", &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return nullptr;

    parser::Parser parser(tokens, "test.sp", &diags);
    auto program = parser.parseProgram();
    if (diags.hasErrors() || !program) return nullptr;

    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) return nullptr;

    ir::IRBuilder builder(sema);
    return builder.build(*program, "test");
}

std::string runExecutable(const std::string& exePath) {
    std::string outPath = exePath + ".out.txt";
    std::string cmd = "\"" + exePath + "\" > \"" + outPath + "\" 2>&1";
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
    return ss.str();
}

fs::path getOutputDir() {
#ifdef SP_PROJECT_ROOT
    fs::path p = fs::path(SP_PROJECT_ROOT) / "build";
    if (fs::exists(p)) return p;
#endif
    return fs::temp_directory_path();
}

} // namespace

TEST(LLVMGeneratorTest, GenerateLLVMIRHeaderAndBody) {
    const std::string source = R"(
        fn main() {
            println("LLVM Generator Test");
        }
    )";

    auto module = parseAndBuildIR(source);
    ASSERT_NE(module, nullptr);

    codegen::LLVMGenerator generator;
    std::string llvmIR = generator.generateLLVMIR(*module);

    EXPECT_NE(llvmIR.find("; ModuleID = 'test'"), std::string::npos);
    EXPECT_NE(llvmIR.find("target triple = "), std::string::npos);
    EXPECT_NE(llvmIR.find("declare void @sp_rt_println_string(ptr)"), std::string::npos);
    // Phase 6 slice B: main now takes (argc, argv) so OS.args() can
    // return the real arguments.
    EXPECT_NE(llvmIR.find("define i32 @main(i32 %argc, ptr %argv) {"), std::string::npos);
}

TEST(LLVMGeneratorTest, EmitLLVMIRToFile) {
    const std::string source = R"(
        fn main() {
            println("Emit to file");
        }
    )";

    auto module = parseAndBuildIR(source);
    ASSERT_NE(module, nullptr);

    fs::path tempFile = getOutputDir() / "test_emit.ll";
    codegen::LLVMGenerator generator;
    bool success = generator.emitLLVMIRToFile(*module, tempFile.string());
    EXPECT_TRUE(success);
    EXPECT_TRUE(fs::exists(tempFile));

    std::ifstream in(tempFile);
    std::string content((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    // Phase 6 slice B: main now takes (argc, argv) so OS.args() can
    // return the real arguments.
    EXPECT_NE(content.find("define i32 @main(i32 %argc, ptr %argv)"), std::string::npos);

    std::error_code ec;
    fs::remove(tempFile, ec);
}

TEST(LLVMGeneratorTest, EndToEndCompileAndRunNativeExecutable) {
    const std::string source = R"(
        fn main() {
            println("Hello from Native Executable!");
        }
    )";

    auto module = parseAndBuildIR(source);
    ASSERT_NE(module, nullptr);

    fs::path exePath = getOutputDir() / "test_native_hello.exe";
    codegen::CodegenOptions opts;
    codegen::LLVMGenerator generator(opts);

    bool compiled = generator.compileToExecutable(*module, exePath.string());
    ASSERT_TRUE(compiled) << "Compilation failed: " << generator.getLastError();
    ASSERT_TRUE(fs::exists(exePath));

    std::string output = runExecutable(exePath.string());
    EXPECT_NE(output.find("Hello from Native Executable!"), std::string::npos);

    std::error_code ec;
    fs::remove(exePath, ec);
}

TEST(LLVMGeneratorTest, EndToEndNativeFibonacciCalculation) {
    const std::string source = R"(
        fn fib(n: int) -> int {
            if (n <= 1) {
                return n;
            }
            return fib(n - 1) + fib(n - 2);
        }

        fn main() {
            var res = fib(7);
            println(res);
        }
    )";

    auto module = parseAndBuildIR(source);
    ASSERT_NE(module, nullptr);

    fs::path exePath = getOutputDir() / "test_native_fib.exe";
    codegen::CodegenOptions opts;
    codegen::LLVMGenerator generator(opts);

    bool compiled = generator.compileToExecutable(*module, exePath.string());
    ASSERT_TRUE(compiled) << "Compilation failed: " << generator.getLastError();
    ASSERT_TRUE(fs::exists(exePath));

    std::string output = runExecutable(exePath.string());
    // fib(7) = 13
    EXPECT_NE(output.find("13"), std::string::npos);

    std::error_code ec;
    fs::remove(exePath, ec);
}

TEST(LLVMGeneratorTest, RuntimeMemoryAllocationAndFree) {
    void* ptr = sp_rt_alloc(64);
    ASSERT_NE(ptr, nullptr);
    auto* bytes = static_cast<unsigned char*>(ptr);
    for (size_t i = 0; i < 64; ++i) {
        EXPECT_EQ(bytes[i], 0);
    }
    sp_rt_free(ptr);
}
