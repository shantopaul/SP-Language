#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <fstream>
#include <sstream>
#include <filesystem>

namespace sp::lexer::test {

namespace {

std::string readFileContent(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void assertFileTokenizesWithoutErrors(const std::filesystem::path& filePath) {
    ASSERT_TRUE(std::filesystem::exists(filePath)) << "File does not exist: " << filePath;

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
    assertFileTokenizesWithoutErrors("K:/AndroidDev/tools/Project/Own Programming Language/SP-Language/examples/hello.sp");
}

TEST(GoldenLexerTest, TokenizeFibonacciExample) {
    assertFileTokenizesWithoutErrors("K:/AndroidDev/tools/Project/Own Programming Language/SP-Language/examples/fibonacci.sp");
}

TEST(GoldenLexerTest, TokenizeClassesExample) {
    assertFileTokenizesWithoutErrors("K:/AndroidDev/tools/Project/Own Programming Language/SP-Language/examples/classes.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibIO) {
    assertFileTokenizesWithoutErrors("K:/AndroidDev/tools/Project/Own Programming Language/SP-Language/stdlib/io/io.sp");
}

TEST(GoldenLexerTest, TokenizeStdlibMath) {
    assertFileTokenizesWithoutErrors("K:/AndroidDev/tools/Project/Own Programming Language/SP-Language/stdlib/math/math.sp");
}

} // namespace sp::lexer::test
