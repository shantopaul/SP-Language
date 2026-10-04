#include <gtest/gtest.h>
#include "sp/lexer/TokenType.h"
#include "sp/lexer/Token.h"
#include "sp/diagnostics/SourceLocation.h"

namespace sp::lexer::test {

TEST(TokenTypeTest, KeywordStringRepresentations) {
    EXPECT_EQ(tokenTypeToString(TokenType::KeywordFn), "fn");
    EXPECT_EQ(tokenTypeToString(TokenType::KeywordClass), "class");
    EXPECT_EQ(tokenTypeToString(TokenType::KeywordInterface), "interface");
    EXPECT_EQ(tokenTypeToString(TokenType::KeywordLet), "let");
    EXPECT_EQ(tokenTypeToString(TokenType::KeywordVar), "var");
    EXPECT_EQ(tokenTypeToString(TokenType::KeywordConst), "const");
    EXPECT_EQ(tokenTypeToString(TokenType::KeywordReturn), "return");
}

TEST(TokenTypeTest, OperatorStringRepresentations) {
    EXPECT_EQ(tokenTypeToString(TokenType::Plus), "+");
    EXPECT_EQ(tokenTypeToString(TokenType::Minus), "-");
    EXPECT_EQ(tokenTypeToString(TokenType::EqualEqual), "==");
    EXPECT_EQ(tokenTypeToString(TokenType::Arrow), "->");
    EXPECT_EQ(tokenTypeToString(TokenType::FatArrow), "=>");
    EXPECT_EQ(tokenTypeToString(TokenType::QuestionDot), "?.");
    EXPECT_EQ(tokenTypeToString(TokenType::QuestionQuestion), "??");
}

TEST(TokenTest, TokenConstructionAndMatching) {
    diagnostics::SourceLocation loc("test.sp", 10, 5);
    Token tok(TokenType::KeywordFn, "fn", loc);

    EXPECT_TRUE(tok.is(TokenType::KeywordFn));
    EXPECT_FALSE(tok.is(TokenType::KeywordClass));
    EXPECT_TRUE(tok.isNot(TokenType::KeywordClass));
    EXPECT_EQ(tok.lexeme, "fn");
    EXPECT_EQ(tok.location.line, 10);
    EXPECT_EQ(tok.location.column, 5);
}

} // namespace sp::lexer::test
