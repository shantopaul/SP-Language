#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#ifndef SP_PROJECT_ROOT
#define SP_PROJECT_ROOT "."
#endif

namespace sp::sema::test {

namespace {

std::filesystem::path resolveTestFilePath(const std::string& relPath) {
    std::filesystem::path fromRoot = std::filesystem::path(SP_PROJECT_ROOT) / relPath;
    if (std::filesystem::exists(fromRoot)) return fromRoot;

    std::filesystem::path fromCwd = std::filesystem::current_path() / relPath;
    if (std::filesystem::exists(fromCwd)) return fromCwd;

    std::filesystem::path oneUp = std::filesystem::current_path() / ".." / relPath;
    if (std::filesystem::exists(oneUp)) return oneUp;

    std::filesystem::path twoUp = std::filesystem::current_path() / ".." / ".." / relPath;
    if (std::filesystem::exists(twoUp)) return twoUp;

    return fromRoot;
}

std::string readFileContent(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void assertFileAnalyzesCleanly(const std::string& relPath) {
    std::filesystem::path filePath = resolveTestFilePath(relPath);
    ASSERT_TRUE(std::filesystem::exists(filePath)) << "File does not exist: " << filePath;

    std::string content = readFileContent(filePath);
    ASSERT_FALSE(content.empty()) << "File is empty: " << filePath;

    std::ostringstream errStream;
    diagnostics::DiagnosticEngine diag(errStream);

    lexer::Lexer lexer(content, filePath.string(), &diag);
    auto tokens = lexer.tokenizeAll();
    ASSERT_FALSE(diag.hasErrors()) << "Lexing failed for " << relPath << ":\n" << errStream.str();

    parser::Parser parser(tokens, filePath.string(), &diag);
    auto program = parser.parseProgram();
    ASSERT_FALSE(diag.hasErrors()) << "Parsing failed for " << relPath << ":\n" << errStream.str();
    ASSERT_NE(program, nullptr) << "Parsed program AST is null for " << relPath;

    SemanticAnalyzer sema(diag);
    bool success = sema.analyze(*program);
    EXPECT_TRUE(success) << "Semantic analysis failed for " << relPath << ":\n" << errStream.str();
    EXPECT_FALSE(diag.hasErrors()) << "Semantic errors reported for " << relPath << ":\n" << errStream.str();
}

} // namespace

TEST(GoldenSemanticTest, AnalyzeHelloExample) {
    assertFileAnalyzesCleanly("examples/hello.sp");
}

TEST(GoldenSemanticTest, AnalyzeFibonacciExample) {
    assertFileAnalyzesCleanly("examples/fibonacci.sp");
}

TEST(GoldenSemanticTest, AnalyzeClassesExample) {
    assertFileAnalyzesCleanly("examples/classes.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibIO) {
    assertFileAnalyzesCleanly("stdlib/io/io.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibMath) {
    assertFileAnalyzesCleanly("stdlib/math/math.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibString) {
    assertFileAnalyzesCleanly("stdlib/string/string.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibCollections) {
    assertFileAnalyzesCleanly("stdlib/collections/collections.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibFS) {
    assertFileAnalyzesCleanly("stdlib/fs/fs.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibTime) {
    assertFileAnalyzesCleanly("stdlib/time/time.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibOS) {
    assertFileAnalyzesCleanly("stdlib/os/os.sp");
}

TEST(GoldenSemanticTest, AnalyzeStdlibProcess) {
    assertFileAnalyzesCleanly("stdlib/process/process.sp");
}

} // namespace sp::sema::test
