#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace sp;

static std::string readSourceFile(const fs::path& path) {
    std::ifstream file(path);
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

static void assertLexAndParse(const fs::path& path) {
    std::string content = readSourceFile(path);
    ASSERT_FALSE(content.empty()) << "File is empty: " << path.string();

    diagnostics::DiagnosticEngine diag;
    lexer::Lexer lexer(content, path.string(), &diag);
    auto tokens = lexer.tokenizeAll();
    EXPECT_FALSE(tokens.empty());
    EXPECT_EQ(diag.errorCount(), 0);

    parser::Parser parser(tokens, path.string(), &diag);
    auto program = parser.parseProgram();
    EXPECT_NE(program, nullptr);
    EXPECT_EQ(diag.errorCount(), 0);
}

TEST(SelfHostedTest, SourceFilesExist) {
    fs::path baseDir = SP_PROJECT_ROOT;
    fs::path selfHostedDir = baseDir / "compiler" / "self_hosted";

    EXPECT_TRUE(fs::exists(selfHostedDir / "tokens.sp"));
    EXPECT_TRUE(fs::exists(selfHostedDir / "lexer.sp"));
    EXPECT_TRUE(fs::exists(selfHostedDir / "ast.sp"));
    EXPECT_TRUE(fs::exists(selfHostedDir / "parser.sp"));
    EXPECT_TRUE(fs::exists(selfHostedDir / "sema.sp"));
    EXPECT_TRUE(fs::exists(selfHostedDir / "main.sp"));
}

TEST(SelfHostedTest, LexAndParseTokensSp) {
    assertLexAndParse(fs::path(SP_PROJECT_ROOT) / "compiler" / "self_hosted" / "tokens.sp");
}

TEST(SelfHostedTest, LexAndParseLexerSp) {
    assertLexAndParse(fs::path(SP_PROJECT_ROOT) / "compiler" / "self_hosted" / "lexer.sp");
}

TEST(SelfHostedTest, LexAndParseAstSp) {
    assertLexAndParse(fs::path(SP_PROJECT_ROOT) / "compiler" / "self_hosted" / "ast.sp");
}

TEST(SelfHostedTest, LexAndParseParserSp) {
    assertLexAndParse(fs::path(SP_PROJECT_ROOT) / "compiler" / "self_hosted" / "parser.sp");
}

TEST(SelfHostedTest, LexAndParseSemaSp) {
    assertLexAndParse(fs::path(SP_PROJECT_ROOT) / "compiler" / "self_hosted" / "sema.sp");
}

TEST(SelfHostedTest, LexAndParseMainSp) {
    assertLexAndParse(fs::path(SP_PROJECT_ROOT) / "compiler" / "self_hosted" / "main.sp");
}
