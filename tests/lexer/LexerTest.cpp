#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <sstream>

namespace sp::lexer::test {

TEST(LexerTest, TokenizeEmptyAndWhitespace) {
    Lexer lexer("   \t\r\n   \n  ", "test.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    ASSERT_EQ(tokens.size(), 1);
    EXPECT_EQ(tokens[0].type, TokenType::EndOfFile);
    EXPECT_EQ(tokens[0].location.line, 3);
}

TEST(LexerTest, TokenizeSingleAndMultiCharOperators) {
    std::string_view src = "+ - * / % ++ -- == != < > <= >= && || ! & | ^ ~ << >> = += -= *= /= %= -> => ?. ?? .. @";
    Lexer lexer(src, "operators.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    std::vector<TokenType> expected = {
        TokenType::Plus, TokenType::Minus, TokenType::Star, TokenType::Slash, TokenType::Percent,
        TokenType::PlusPlus, TokenType::MinusMinus,
        TokenType::EqualEqual, TokenType::BangEqual,
        TokenType::Less, TokenType::Greater, TokenType::LessEqual, TokenType::GreaterEqual,
        TokenType::AmpAmp, TokenType::PipePipe, TokenType::Bang,
        TokenType::Amp, TokenType::Pipe, TokenType::Caret, TokenType::Tilde,
        TokenType::LessLess, TokenType::GreaterGreater,
        TokenType::Equal, TokenType::PlusEqual, TokenType::MinusEqual, TokenType::StarEqual, TokenType::SlashEqual, TokenType::PercentEqual,
        TokenType::Arrow, TokenType::FatArrow, TokenType::QuestionDot, TokenType::QuestionQuestion, TokenType::DotDot,
        TokenType::At,
        TokenType::EndOfFile
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]) << "Failed at index " << i << " with lexeme: " << tokens[i].lexeme;
    }
}

TEST(LexerTest, TokenizeKeywordsAndIdentifiers) {
    std::string_view src = "fn class interface struct enum if else for while return var let const int string bool myVar User123";
    Lexer lexer(src, "keywords.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    std::vector<TokenType> expected = {
        TokenType::KeywordFn, TokenType::KeywordClass, TokenType::KeywordInterface,
        TokenType::KeywordStruct, TokenType::KeywordEnum,
        TokenType::KeywordIf, TokenType::KeywordElse, TokenType::KeywordFor, TokenType::KeywordWhile,
        TokenType::KeywordReturn,
        TokenType::KeywordVar, TokenType::KeywordLet, TokenType::KeywordConst,
        TokenType::KeywordInt, TokenType::KeywordString, TokenType::KeywordBool,
        TokenType::Identifier, TokenType::Identifier,
        TokenType::EndOfFile
    };

    ASSERT_EQ(tokens.size(), expected.size());
    for (size_t i = 0; i < expected.size(); ++i) {
        EXPECT_EQ(tokens[i].type, expected[i]);
    }

    EXPECT_EQ(tokens[16].lexeme, "myVar");
    EXPECT_EQ(tokens[17].lexeme, "User123");
}

TEST(LexerTest, TokenizeNumbers) {
    std::string_view src = "42 1_000_000 3.14159 0.5 1.5e-3 2e10 0xFF 0b1010 0o755";
    Lexer lexer(src, "numbers.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    ASSERT_EQ(tokens.size(), 10);
    EXPECT_EQ(tokens[0].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[0].lexeme, "42");

    EXPECT_EQ(tokens[1].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[1].lexeme, "1_000_000");

    EXPECT_EQ(tokens[2].type, TokenType::FloatLiteral);
    EXPECT_EQ(tokens[2].lexeme, "3.14159");

    EXPECT_EQ(tokens[3].type, TokenType::FloatLiteral);
    EXPECT_EQ(tokens[3].lexeme, "0.5");

    EXPECT_EQ(tokens[4].type, TokenType::FloatLiteral);
    EXPECT_EQ(tokens[4].lexeme, "1.5e-3");

    EXPECT_EQ(tokens[5].type, TokenType::FloatLiteral);
    EXPECT_EQ(tokens[5].lexeme, "2e10");

    EXPECT_EQ(tokens[6].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[6].lexeme, "0xFF");

    EXPECT_EQ(tokens[7].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[7].lexeme, "0b1010");

    EXPECT_EQ(tokens[8].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[8].lexeme, "0o755");
}

TEST(LexerTest, TokenizeStringsAndCharacters) {
    std::string_view src = "\"Hello, SP!\" \"Line1\\nLine2\" \"\" 'A' '\\n' '\\''";
    Lexer lexer(src, "strings.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    ASSERT_EQ(tokens.size(), 7);
    EXPECT_EQ(tokens[0].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[0].lexeme, "Hello, SP!");

    EXPECT_EQ(tokens[1].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[1].lexeme, "Line1\nLine2");

    EXPECT_EQ(tokens[2].type, TokenType::StringLiteral);
    EXPECT_EQ(tokens[2].lexeme, "");

    EXPECT_EQ(tokens[3].type, TokenType::CharLiteral);
    EXPECT_EQ(tokens[3].lexeme, "A");

    EXPECT_EQ(tokens[4].type, TokenType::CharLiteral);
    EXPECT_EQ(tokens[4].lexeme, "\n");

    EXPECT_EQ(tokens[5].type, TokenType::CharLiteral);
    EXPECT_EQ(tokens[5].lexeme, "'");
}

TEST(LexerTest, CommentsSkippedByDefault) {
    std::string_view src = "int x = 10; // This is a comment\n/* Multi-line\n   comment */\nint y = 20;";
    Lexer lexer(src, "comments.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    // 10 tokens + EndOfFile = 11 tokens
    ASSERT_EQ(tokens.size(), 11);
    EXPECT_EQ(tokens[0].type, TokenType::KeywordInt);
    EXPECT_EQ(tokens[1].type, TokenType::Identifier);
    EXPECT_EQ(tokens[2].type, TokenType::Equal);
    EXPECT_EQ(tokens[3].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[4].type, TokenType::Semicolon);

    EXPECT_EQ(tokens[5].type, TokenType::KeywordInt);
    EXPECT_EQ(tokens[6].type, TokenType::Identifier);
    EXPECT_EQ(tokens[7].type, TokenType::Equal);
    EXPECT_EQ(tokens[8].type, TokenType::IntegerLiteral);
    EXPECT_EQ(tokens[9].type, TokenType::Semicolon);
    EXPECT_EQ(tokens[10].type, TokenType::EndOfFile);
}

TEST(LexerTest, PreservedCommentsAndDocComments) {
    std::string_view src = "/// Docs for add\n// regular comment\n/* block */\nfn add()";
    Lexer lexer(src, "docs.sp");
    lexer.setPreserveComments(true);
    std::vector<Token> tokens = lexer.tokenizeAll();

    // DocComment, Comment, Comment, fn, add, (, ), EndOfFile = 8 tokens
    ASSERT_EQ(tokens.size(), 8);
    EXPECT_EQ(tokens[0].type, TokenType::DocComment);
    EXPECT_EQ(tokens[1].type, TokenType::Comment);
    EXPECT_EQ(tokens[2].type, TokenType::Comment);
    EXPECT_EQ(tokens[3].type, TokenType::KeywordFn);
    EXPECT_EQ(tokens[4].type, TokenType::Identifier);
    EXPECT_EQ(tokens[5].type, TokenType::OpenParen);
    EXPECT_EQ(tokens[6].type, TokenType::CloseParen);
    EXPECT_EQ(tokens[7].type, TokenType::EndOfFile);
}

TEST(LexerTest, SourceLocationAccuracyAcrossLines) {
    std::string_view src = "fn main() {\n    int a = 10;\n    return a;\n}";
    Lexer lexer(src, "main.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    // fn on line 1, column 1
    EXPECT_EQ(tokens[0].type, TokenType::KeywordFn);
    EXPECT_EQ(tokens[0].location.line, 1);
    EXPECT_EQ(tokens[0].location.column, 1);

    // { on line 1, column 11
    EXPECT_EQ(tokens[4].type, TokenType::OpenBrace);
    EXPECT_EQ(tokens[4].location.line, 1);
    EXPECT_EQ(tokens[4].location.column, 11);

    // "int" on line 2, column 5
    EXPECT_EQ(tokens[5].type, TokenType::KeywordInt);
    EXPECT_EQ(tokens[5].location.line, 2);
    EXPECT_EQ(tokens[5].location.column, 5);

    // "return" on line 3, column 5 (tokens: 5=int, 6=a, 7==, 8=10, 9=;, 10=return)
    EXPECT_EQ(tokens[10].type, TokenType::KeywordReturn);
    EXPECT_EQ(tokens[10].location.line, 3);
    EXPECT_EQ(tokens[10].location.column, 5);
}

TEST(LexerTest, LexicalErrorHandling) {
    std::ostringstream errStream;
    diagnostics::DiagnosticEngine diag(errStream);

    std::string_view src = "\"unclosed string\n$ @@@";
    Lexer lexer(src, "error.sp", &diag);
    std::vector<Token> tokens = lexer.tokenizeAll();

    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GT(diag.errorCount(), 0);

    std::string errOutput = errStream.str();
    EXPECT_NE(errOutput.find("Unclosed string literal"), std::string::npos);
    EXPECT_NE(errOutput.find("Unexpected character"), std::string::npos);
}

TEST(LexerTest, TokenizeRealSPProgram) {
    std::string_view src = R"(
        import std.io;

        class User {
            private string name;
            public User(string name) {
                this.name = name;
            }
            public fn getName() -> string {
                return this.name;
            }
        }

        fn main() {
            User? user = new User("Shanto");
            println(user?.getName() ?? "Unknown");
        }
    )";

    Lexer lexer(src, "user.sp");
    std::vector<Token> tokens = lexer.tokenizeAll();

    // Verify key elements from the code are tokenized correctly
    EXPECT_FALSE(tokens.empty());
    EXPECT_EQ(tokens.back().type, TokenType::EndOfFile);

    bool foundImport = false;
    bool foundClass = false;
    bool foundQuestionDot = false;
    bool foundQuestionQuestion = false;

    for (const auto& tok : tokens) {
        if (tok.is(TokenType::KeywordImport)) foundImport = true;
        if (tok.is(TokenType::KeywordClass)) foundClass = true;
        if (tok.is(TokenType::QuestionDot)) foundQuestionDot = true;
        if (tok.is(TokenType::QuestionQuestion)) foundQuestionQuestion = true;
    }

    EXPECT_TRUE(foundImport);
    EXPECT_TRUE(foundClass);
    EXPECT_TRUE(foundQuestionDot);
    EXPECT_TRUE(foundQuestionQuestion);
}

} // namespace sp::lexer::test
