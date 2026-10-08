#include <gtest/gtest.h>
#include "sp/sema/ComptimeEvaluator.h"
#include "sp/ast/ASTNode.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/diagnostics/DiagnosticEngine.h"

using namespace sp;
using namespace sp::sema;

TEST(ComptimeEvaluatorTest, BasicValueTypes) {
    auto vInt = ComptimeValue::fromInt(42);
    EXPECT_EQ(vInt.kind, ComptimeKind::Int);
    EXPECT_EQ(vInt.intVal, 42);
    EXPECT_EQ(vInt.toString(), "42");
    EXPECT_TRUE(vInt.isTruthy());

    auto vFloat = ComptimeValue::fromFloat(3.1415);
    EXPECT_EQ(vFloat.kind, ComptimeKind::Float);
    EXPECT_NEAR(vFloat.floatVal, 3.1415, 0.0001);

    auto vBool = ComptimeValue::fromBool(true);
    EXPECT_EQ(vBool.kind, ComptimeKind::Bool);
    EXPECT_TRUE(vBool.boolVal);
    EXPECT_EQ(vBool.toString(), "true");

    auto vStr = ComptimeValue::fromString("hello comptime");
    EXPECT_EQ(vStr.kind, ComptimeKind::String);
    EXPECT_EQ(vStr.stringVal, "hello comptime");
    EXPECT_EQ(vStr.toString(), "hello comptime");

    auto vVoid = ComptimeValue::voidVal();
    EXPECT_EQ(vVoid.kind, ComptimeKind::Void);
    EXPECT_FALSE(vVoid.isTruthy());
}

TEST(ComptimeEvaluatorTest, ArithmeticAndLogicEvaluation) {
    diagnostics::DiagnosticEngine diag;
    ComptimeEvaluator eval(&diag);

    // Literal 10 + 20
    diagnostics::SourceLocation loc;
    auto left = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "10", loc);
    auto right = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "20", loc);
    ast::BinaryExpr addExpr(std::move(left), lexer::TokenType::Plus, std::move(right), loc);

    auto res = eval.evaluate(addExpr);
    EXPECT_EQ(res.kind, ComptimeKind::Int);
    EXPECT_EQ(res.intVal, 30);
    EXPECT_FALSE(eval.hasError());
}

TEST(ComptimeEvaluatorTest, StringConcatenation) {
    diagnostics::DiagnosticEngine diag;
    ComptimeEvaluator eval(&diag);

    diagnostics::SourceLocation loc;
    auto left = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, "SP ", loc);
    auto right = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::String, "2.0", loc);
    ast::BinaryExpr concatExpr(std::move(left), lexer::TokenType::Plus, std::move(right), loc);

    auto res = eval.evaluate(concatExpr);
    EXPECT_EQ(res.kind, ComptimeKind::String);
    EXPECT_EQ(res.stringVal, "SP 2.0");
}

TEST(ComptimeEvaluatorTest, RelationalAndEqualityOperators) {
    diagnostics::DiagnosticEngine diag;
    ComptimeEvaluator eval(&diag);

    diagnostics::SourceLocation loc;
    auto l5 = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "5", loc);
    auto r10 = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "10", loc);
    ast::BinaryExpr ltExpr(std::move(l5), lexer::TokenType::Less, std::move(r10), loc);

    auto res = eval.evaluate(ltExpr);
    EXPECT_EQ(res.kind, ComptimeKind::Bool);
    EXPECT_TRUE(res.boolVal);
}

TEST(ComptimeEvaluatorTest, ParseAndFoldComptimeProgram) {
    std::string source = 
        "fn calculate(): int {\n"
        "    let x: int = comptime (100 * 2 + 50);\n"
        "    return x;\n"
        "}\n";

    diagnostics::DiagnosticEngine diag;
    lexer::Lexer lexer(source, "<test>", &diag);
    auto tokens = lexer.tokenizeAll();

    parser::Parser parser(tokens, "<test>", &diag);
    auto program = parser.parseProgram();
    ASSERT_NE(program, nullptr);

    ComptimeEvaluator eval(&diag);
    eval.processProgram(*program);
    EXPECT_FALSE(eval.hasError());
}

TEST(ComptimeEvaluatorTest, DivisionByZeroReportsError) {
    diagnostics::DiagnosticEngine diag;
    ComptimeEvaluator eval(&diag);

    diagnostics::SourceLocation loc;
    auto l10 = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "10", loc);
    auto r0 = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "0", loc);
    ast::BinaryExpr divExpr(std::move(l10), lexer::TokenType::Slash, std::move(r0), loc);

    eval.evaluate(divExpr);
    EXPECT_TRUE(eval.hasError());
}
