#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <fstream>
#include <sstream>
#include <filesystem>

#ifndef SP_PROJECT_ROOT
#define SP_PROJECT_ROOT "."
#endif

namespace sp::lexer::test {

namespace {

std::filesystem::path resolveTestFilePath(const std::string& relPath) {
    // 1. Check from SP_PROJECT_ROOT definition
    std::filesystem::path fromRoot = std::filesystem::path(SP_PROJECT_ROOT) / relPath;
    if (std::filesystem::exists(fromRoot)) {
        return fromRoot;
    }

    // 2. Check from current working directory
    std::filesystem::path fromCwd = std::filesystem::current_path() / relPath;
    if (std::filesystem::exists(fromCwd)) {
        return fromCwd;
    }

    // 3. Check one directory up (if running from build/)
    std::filesystem::path oneUp = std::filesystem::current_path() / ".." / relPath;
    if (std::filesystem::exists(oneUp)) {
        return oneUp;
    }

    // 4. Check two directories up (if running from build/bin/ or build/tests/)
    std::filesystem::path twoUp = std::filesystem::current_path() / ".." / ".." / relPath;
    if (std::filesystem::exists(twoUp)) {
        return twoUp;
    }

    return fromRoot;
}

std::string readFileContent(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void assertFileTokenizesWithoutErrors(const std::string& relPath) {
    std::filesystem::path filePath = resolveTestFilePath(relPath);
    ASSERT_TRUE(std::filesystem::exists(filePath)) << "File does not exist: " << filePath << " (resolved from " << relPath << ")";

    std::string content = readFileContent(filePath);
    ASSERT_FALSE(content.empty()) << "File is empty: " << filePath;

    std::ostringstream errStream;
    diagnostics::DiagnosticEngine diag(errStream);

    Lexer lexer(content, filePath.filename().string(), &diag);
    std::vector<Token> tokens = lexer.tokenizeAll();

    EXPECT_FALSE(tokens.empty());
    EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);

    // Verify no Invalid tokens were emitted
    for (const auto& tok : tokens) {
        EXPECT_NE(tok.type, TokenType::Invalid) << "Invalid token encountered: " << tok.toString() << " in " << filePath;
    }

    EXPECT_FALSE(diag.hasErrors()) << "Diagnostics error in " << filePath << ": " << errStream.str();
}

} // namespace

TEST(GoldenLexerTest, TokenizeHelloExample) {
    assertFileTokenizesWithoutErrors("examples/hello.sp");
}

TEST(GoldenLexerTest, TokenizeFibonacciExample) {
    assertFileTokenizesWithoutErrors("examples/fibonacci.sp");
}

TEST(GoldenLexerTest, TokenizeClassesExample) {
    assertFileTokenizesWithoutErrors("examples/classes.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibIO) {
    assertFileTokenizesWithoutErrors("stdlib/io/io.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibMath) {
    assertFileTokenizesWithoutErrors("stdlib/math/math.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibString) {
    assertFileTokenizesWithoutErrors("stdlib/string/string.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibCollections) {
    assertFileTokenizesWithoutErrors("stdlib/collections/collections.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibFS) {
    assertFileTokenizesWithoutErrors("stdlib/fs/fs.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibTime) {
    assertFileTokenizesWithoutErrors("stdlib/time/time.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibOS) {
    assertFileTokenizesWithoutErrors("stdlib/os/os.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibProcess) {
    assertFileTokenizesWithoutErrors("stdlib/process/process.sp");
}

} // namespace sp::lexer::test
