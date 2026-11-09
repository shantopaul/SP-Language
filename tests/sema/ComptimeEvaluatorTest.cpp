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

// Phase 3 slice D: a recursive `comptime fn` registered via
// processProgram evaluates through the evaluator (spec §101 example).
TEST(ComptimeEvaluatorTest, ComptimeFnRecursionViaProcessProgram) {
    std::string source =
        "comptime fn factorial(int n) -> int {\n"
        "    if (n <= 1) {\n"
        "        return 1;\n"
        "    }\n"
        "    return n * factorial(n - 1);\n"
        "}\n"
        "fn main() {\n"
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

    diagnostics::SourceLocation loc;
    auto callee = std::make_unique<ast::IdentifierExpr>("factorial", loc);
    std::vector<ast::ExprPtr> args;
    args.push_back(std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "5", loc));
    ast::CallExpr call(std::move(callee), std::move(args), loc);

    auto res = eval.evaluate(call);
    EXPECT_FALSE(eval.hasError());
    EXPECT_EQ(res.kind, ComptimeKind::Int);
    EXPECT_EQ(res.intVal, 120);
}

// Phase 3 slice D: processProgram registers ONLY `comptime fn`
// functions — a plain runtime function is not callable from comptime,
// and calling it reports an error instead of silently returning void.
TEST(ComptimeEvaluatorTest, NonComptimeFunctionNotRegistered) {
    std::string source =
        "fn double(int n) -> int {\n"
        "    return n * 2;\n"
        "}\n"
        "fn main() {\n"
        "}\n";

    diagnostics::DiagnosticEngine diag;
    lexer::Lexer lexer(source, "<test>", &diag);
    auto tokens = lexer.tokenizeAll();
    parser::Parser parser(tokens, "<test>", &diag);
    auto program = parser.parseProgram();
    ASSERT_NE(program, nullptr);

    ComptimeEvaluator eval(&diag);
    eval.processProgram(*program);

    diagnostics::SourceLocation loc;
    auto callee = std::make_unique<ast::IdentifierExpr>("double", loc);
    std::vector<ast::ExprPtr> args;
    args.push_back(std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Integer, "21", loc));
    ast::CallExpr call(std::move(callee), std::move(args), loc);

    auto res = eval.evaluate(call);
    EXPECT_TRUE(eval.hasError());
    EXPECT_EQ(res.kind, ComptimeKind::Void);
}

// Phase 3 slice D: calling an unknown function from comptime is a loud
// error, never a silent void.
TEST(ComptimeEvaluatorTest, UnknownCallReportsError) {
    diagnostics::DiagnosticEngine diag;
    ComptimeEvaluator eval(&diag);

    diagnostics::SourceLocation loc;
    auto callee = std::make_unique<ast::IdentifierExpr>("no_such_fn", loc);
    std::vector<ast::ExprPtr> args;
    ast::CallExpr call(std::move(callee), std::move(args), loc);

    auto res = eval.evaluate(call);
    EXPECT_TRUE(eval.hasError());
    EXPECT_EQ(res.kind, ComptimeKind::Void);
}

// Phase 3 slice D: char equality compares VALUES (the old fallback
// returned true for any two chars, since it only compared kinds).
TEST(ComptimeEvaluatorTest, CharEqualityComparesValues) {
    diagnostics::DiagnosticEngine diag;
    ComptimeEvaluator eval(&diag);

    diagnostics::SourceLocation loc;
    auto la = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Char, "a", loc);
    auto la2 = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Char, "a", loc);
    ast::BinaryExpr eqExpr(std::move(la), lexer::TokenType::EqualEqual, std::move(la2), loc);
    auto eqRes = eval.evaluate(eqExpr);
    EXPECT_EQ(eqRes.kind, ComptimeKind::Bool);
    EXPECT_TRUE(eqRes.boolVal);

    auto lb = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Char, "a", loc);
    auto lb2 = std::make_unique<ast::LiteralExpr>(ast::LiteralKind::Char, "b", loc);
    ast::BinaryExpr neExpr(std::move(lb), lexer::TokenType::EqualEqual, std::move(lb2), loc);
    auto neRes = eval.evaluate(neExpr);
    EXPECT_EQ(neRes.kind, ComptimeKind::Bool);
    EXPECT_FALSE(neRes.boolVal);
}
