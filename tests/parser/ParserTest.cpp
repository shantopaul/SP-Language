#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/ast/Expression.h"
#include "sp/ast/Statement.h"
#include "sp/ast/Declaration.h"
#include "sp/ast/ASTPrinter.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include <sstream>

namespace sp::parser::test {

namespace {

std::unique_ptr<ast::Program> parseSource(std::string_view src, diagnostics::DiagnosticEngine* diag = nullptr) {
    lexer::Lexer lexer(src, "test.sp", diag);
    std::vector<lexer::Token> tokens = lexer.tokenizeAll();
    Parser parser(tokens, "test.sp", diag);
    return parser.parseProgram();
}

ast::ExprPtr parseExpr(std::string_view src, diagnostics::DiagnosticEngine* diag = nullptr) {
    lexer::Lexer lexer(src, "test.sp", diag);
    std::vector<lexer::Token> tokens = lexer.tokenizeAll();
    Parser parser(tokens, "test.sp", diag);
    return parser.parseExpression();
}

} // namespace

TEST(ParserTest, OperatorPrecedenceArithmetic) {
    // 2 + 3 * 4 should parse as 2 + (3 * 4)
    ast::ExprPtr expr = parseExpr("2 + 3 * 4");
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getKind(), ast::ASTNodeKind::BinaryExpr);

    auto* binExpr = dynamic_cast<ast::BinaryExpr*>(expr.get());
    EXPECT_EQ(binExpr->getOp(), lexer::TokenType::Plus);
    EXPECT_EQ(binExpr->getLeft().getKind(), ast::ASTNodeKind::LiteralExpr);
    EXPECT_EQ(binExpr->getRight().getKind(), ast::ASTNodeKind::BinaryExpr);

    auto* rightBin = dynamic_cast<const ast::BinaryExpr*>(&binExpr->getRight());
    EXPECT_EQ(rightBin->getOp(), lexer::TokenType::Star);
}

TEST(ParserTest, GroupedExpressionsOverridePrecedence) {
    // (2 + 3) * 4 should parse as (2 + 3) * 4
    ast::ExprPtr expr = parseExpr("(2 + 3) * 4");
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getKind(), ast::ASTNodeKind::BinaryExpr);

    auto* binExpr = dynamic_cast<ast::BinaryExpr*>(expr.get());
    EXPECT_EQ(binExpr->getOp(), lexer::TokenType::Star);
    EXPECT_EQ(binExpr->getLeft().getKind(), ast::ASTNodeKind::BinaryExpr);

    auto* leftBin = dynamic_cast<const ast::BinaryExpr*>(&binExpr->getLeft());
    EXPECT_EQ(leftBin->getOp(), lexer::TokenType::Plus);
}

TEST(ParserTest, LogicalAndRelationalPrecedence) {
    // a > 10 && b <= 20 || !c
    ast::ExprPtr expr = parseExpr("a > 10 && b <= 20 || !c");
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getKind(), ast::ASTNodeKind::BinaryExpr);

    auto* bin = dynamic_cast<ast::BinaryExpr*>(expr.get());
    EXPECT_EQ(bin->getOp(), lexer::TokenType::PipePipe);
}

TEST(ParserTest, SafeNavigationAndNullCoalescing) {
    // user?.name ?? "Anonymous"
    ast::ExprPtr expr = parseExpr("user?.name ?? \"Anonymous\"");
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getKind(), ast::ASTNodeKind::BinaryExpr);

    auto* bin = dynamic_cast<ast::BinaryExpr*>(expr.get());
    EXPECT_EQ(bin->getOp(), lexer::TokenType::QuestionQuestion);
    EXPECT_EQ(bin->getLeft().getKind(), ast::ASTNodeKind::MemberExpr);

    auto* member = dynamic_cast<const ast::MemberExpr*>(&bin->getLeft());
    EXPECT_TRUE(member->isSafeNav());
    EXPECT_EQ(member->getMember(), "name");
}

TEST(ParserTest, TernaryOperator) {
    // age >= 18 ? "Adult" : "Minor"
    ast::ExprPtr expr = parseExpr("age >= 18 ? \"Adult\" : \"Minor\"");
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getKind(), ast::ASTNodeKind::TernaryExpr);

    auto* ternary = dynamic_cast<ast::TernaryExpr*>(expr.get());
    EXPECT_EQ(ternary->getCondition().getKind(), ast::ASTNodeKind::BinaryExpr);
    EXPECT_EQ(ternary->getThenExpr().getKind(), ast::ASTNodeKind::LiteralExpr);
    EXPECT_EQ(ternary->getElseExpr().getKind(), ast::ASTNodeKind::LiteralExpr);
}

TEST(ParserTest, CallAndIndexExpressions) {
    // users[0].getName("arg")
    ast::ExprPtr expr = parseExpr("users[0].getName(\"arg\")");
    ASSERT_NE(expr, nullptr);
    ASSERT_EQ(expr->getKind(), ast::ASTNodeKind::CallExpr);

    auto* call = dynamic_cast<ast::CallExpr*>(expr.get());
    EXPECT_EQ(call->getArgs().size(), 1);
    EXPECT_EQ(call->getCallee().getKind(), ast::ASTNodeKind::MemberExpr);
}

TEST(ParserTest, ArrayAndMapLiterals) {
    ast::ExprPtr arr = parseExpr("[1, 2, 3, 4]");
    ASSERT_NE(arr, nullptr);
    ASSERT_EQ(arr->getKind(), ast::ASTNodeKind::ArrayExpr);
    auto* arrNode = dynamic_cast<ast::ArrayExpr*>(arr.get());
    EXPECT_EQ(arrNode->getElements().size(), 4);

    ast::ExprPtr map = parseExpr("{\"key\": \"value\", \"count\": 42}");
    ASSERT_NE(map, nullptr);
    ASSERT_EQ(map->getKind(), ast::ASTNodeKind::MapExpr);
    auto* mapNode = dynamic_cast<ast::MapExpr*>(map.get());
    EXPECT_EQ(mapNode->getEntries().size(), 2);
}

TEST(ParserTest, NewObjectExpression) {
    ast::ExprPtr newExpr = parseExpr("new Box<int>(42)");
    ASSERT_NE(newExpr, nullptr);
    ASSERT_EQ(newExpr->getKind(), ast::ASTNodeKind::NewExpr);

    auto* node = dynamic_cast<ast::NewExpr*>(newExpr.get());
    EXPECT_EQ(node->getTypeName(), "Box");
    ASSERT_EQ(node->getTypeArgs().size(), 1);
    EXPECT_EQ(node->getTypeArgs()[0], "int");
    EXPECT_EQ(node->getArgs().size(), 1);
}

TEST(ParserTest, VariableDeclarations) {
    std::string_view src = "int age = 21; var name = \"Shanto\"; const PI = 3.14;";
    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 3);

    auto* v1 = dynamic_cast<ast::VarDeclStmt*>(program->getDeclarations()[0].get());
    EXPECT_EQ(v1->getType(), "int");
    EXPECT_EQ(v1->getName(), "age");

    auto* v2 = dynamic_cast<ast::VarDeclStmt*>(program->getDeclarations()[1].get());
    EXPECT_TRUE(v2->isVar());
    EXPECT_EQ(v2->getName(), "name");

    auto* v3 = dynamic_cast<ast::VarDeclStmt*>(program->getDeclarations()[2].get());
    EXPECT_TRUE(v3->isConst());
    EXPECT_EQ(v3->getName(), "PI");
}

TEST(ParserTest, ColonTypeAnnotations) {
    std::string_view src = "let a: int = 10; var b: string = \"hello\"; const c: float = 3.14;";
    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 3);

    auto* v1 = dynamic_cast<ast::VarDeclStmt*>(program->getDeclarations()[0].get());
    ASSERT_NE(v1, nullptr);
    EXPECT_EQ(v1->getType(), "int");
    EXPECT_EQ(v1->getName(), "a");

    auto* v2 = dynamic_cast<ast::VarDeclStmt*>(program->getDeclarations()[1].get());
    ASSERT_NE(v2, nullptr);
    EXPECT_EQ(v2->getType(), "string");
    EXPECT_EQ(v2->getName(), "b");

    auto* v3 = dynamic_cast<ast::VarDeclStmt*>(program->getDeclarations()[2].get());
    ASSERT_NE(v3, nullptr);
    EXPECT_EQ(v3->getType(), "float");
    EXPECT_EQ(v3->getName(), "c");
}

TEST(ParserTest, ControlFlowStatements) {
    std::string_view src = R"(
        if (x > 0) {
            y = 1;
        } else {
            y = -1;
        }

        while (y < 10) {
            y++;
        }

        for (int i = 0; i < 5; i++) {
            break;
        }
    )";

    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 3);

    EXPECT_EQ(program->getDeclarations()[0]->getKind(), ast::ASTNodeKind::IfStmt);
    EXPECT_EQ(program->getDeclarations()[1]->getKind(), ast::ASTNodeKind::WhileStmt);
    EXPECT_EQ(program->getDeclarations()[2]->getKind(), ast::ASTNodeKind::ForStmt);
}

TEST(ParserTest, MatchStatement) {
    std::string_view src = R"(
        match day {
            1 => "Monday",
            2 => "Tuesday",
            _ => "Other"
        }
    )";

    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 1);
    EXPECT_EQ(program->getDeclarations()[0]->getKind(), ast::ASTNodeKind::MatchStmt);

    auto* matchNode = dynamic_cast<ast::MatchStmt*>(program->getDeclarations()[0].get());
    EXPECT_EQ(matchNode->getArms().size(), 3);
    EXPECT_FALSE(matchNode->getArms()[0].isWildcard());
    EXPECT_TRUE(matchNode->getArms()[2].isWildcard());
}

TEST(ParserTest, SyntaxCompatibilityFunctionSignatures) {
    // Both SP-native and Java/C++ style functions normalize to FunctionDecl!
    std::string_view src = R"(
        fn add(int a, int b) -> int {
            return a + b;
        }

        int multiply(int a, int b) {
            return a * b;
        }

        fn square(int x) => x * x;
    )";

    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 3);

    auto* fn1 = dynamic_cast<ast::FunctionDecl*>(program->getDeclarations()[0].get());
    EXPECT_EQ(fn1->getName(), "add");
    EXPECT_EQ(fn1->getReturnType(), "int");
    EXPECT_EQ(fn1->getParams().size(), 2);
    EXPECT_FALSE(fn1->isShortForm());

    auto* fn2 = dynamic_cast<ast::FunctionDecl*>(program->getDeclarations()[1].get());
    EXPECT_EQ(fn2->getName(), "multiply");
    EXPECT_EQ(fn2->getReturnType(), "int");
    EXPECT_EQ(fn2->getParams().size(), 2);
    EXPECT_FALSE(fn2->isShortForm());

    auto* fn3 = dynamic_cast<ast::FunctionDecl*>(program->getDeclarations()[2].get());
    EXPECT_EQ(fn3->getName(), "square");
    EXPECT_TRUE(fn3->isShortForm());
}

TEST(ParserTest, SyntaxCompatibilityClassInheritance) {
    // Normalization of both 'extends' and ':' syntax
    std::string_view src = R"(
        class Dog extends Animal {
            Dog(string name) {
                this.name = name;
            }
        }

        class Cat : Animal {
            Cat(string name) {
                this.name = name;
            }
        }
    )";

    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 2);

    auto* c1 = dynamic_cast<ast::ClassDecl*>(program->getDeclarations()[0].get());
    EXPECT_EQ(c1->getName(), "Dog");
    EXPECT_EQ(c1->getBaseClass(), "Animal");

    auto* c2 = dynamic_cast<ast::ClassDecl*>(program->getDeclarations()[1].get());
    EXPECT_EQ(c2->getName(), "Cat");
    EXPECT_EQ(c2->getBaseClass(), "Animal");
}

TEST(ParserTest, InterfaceAndStructDeclarations) {
    std::string_view src = R"(
        interface Printable {
            fn print();
        }

        struct Point {
            int x;
            int y;
        }

        enum Color {
            Red,
            Green,
            Blue = 100
        }
    )";

    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 3);

    EXPECT_EQ(program->getDeclarations()[0]->getKind(), ast::ASTNodeKind::InterfaceDecl);
    EXPECT_EQ(program->getDeclarations()[1]->getKind(), ast::ASTNodeKind::StructDecl);
    EXPECT_EQ(program->getDeclarations()[2]->getKind(), ast::ASTNodeKind::EnumDecl);

    auto* en = dynamic_cast<ast::EnumDecl*>(program->getDeclarations()[2].get());
    EXPECT_EQ(en->getVariants().size(), 3);
    EXPECT_TRUE(en->getVariants()[2].hasValue());
}

TEST(ParserTest, TryCatchFinallyStatements) {
    std::string_view src = R"(
        try {
            risky();
        } catch (Exception e) {
            handle(e);
        } finally {
            cleanup();
        }
    )";

    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 1);
    EXPECT_EQ(program->getDeclarations()[0]->getKind(), ast::ASTNodeKind::TryCatchStmt);

    auto* tc = dynamic_cast<ast::TryCatchStmt*>(program->getDeclarations()[0].get());
    EXPECT_EQ(tc->getCatches().size(), 1);
    EXPECT_EQ(tc->getCatches()[0].getType(), "Exception");
    EXPECT_EQ(tc->getCatches()[0].getName(), "e");
    EXPECT_TRUE(tc->hasFinally());
}

TEST(ParserTest, ASTPrinterOutputsStructuredTree) {
    std::string_view src = "fn main() { println(\"Hello, SP!\"); }";
    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);

    ast::ASTPrinter printer;
    std::string printed = printer.print(*program);

    EXPECT_NE(printed.find("Program"), std::string::npos);
    EXPECT_NE(printed.find("FunctionDecl(main"), std::string::npos);
    EXPECT_NE(printed.find("CallExpr:"), std::string::npos);
    EXPECT_NE(printed.find("LiteralExpr(Hello, SP!)"), std::string::npos);
}

TEST(ParserTest, GenericClassAndFunctionDeclarations) {
    std::string_view src = R"(
        class Box<T> {
            T value;
            Box(T val) {
                this.value = val;
            }
        }

        class Pair<A, B> {
            A first;
            B second;
        }

        fn max<T>(T a, T b) -> T {
            return a > b ? a : b;
        }

        fn runTest() {
            let m = max<int>(10, 20);
        }
    )";

    auto program = parseSource(src);
    ASSERT_NE(program, nullptr);
    ASSERT_EQ(program->getDeclarations().size(), 4);

    // Box<T>
    auto* boxClass = dynamic_cast<ast::ClassDecl*>(program->getDeclarations()[0].get());
    ASSERT_NE(boxClass, nullptr);
    EXPECT_EQ(boxClass->getName(), "Box");
    EXPECT_TRUE(boxClass->isGeneric());
    ASSERT_EQ(boxClass->getTypeParams().size(), 1);
    EXPECT_EQ(boxClass->getTypeParams()[0], "T");

    // Pair<A, B>
    auto* pairClass = dynamic_cast<ast::ClassDecl*>(program->getDeclarations()[1].get());
    ASSERT_NE(pairClass, nullptr);
    EXPECT_EQ(pairClass->getName(), "Pair");
    EXPECT_TRUE(pairClass->isGeneric());
    ASSERT_EQ(pairClass->getTypeParams().size(), 2);
    EXPECT_EQ(pairClass->getTypeParams()[0], "A");
    EXPECT_EQ(pairClass->getTypeParams()[1], "B");

    // fn max<T>
    auto* maxFn = dynamic_cast<ast::FunctionDecl*>(program->getDeclarations()[2].get());
    ASSERT_NE(maxFn, nullptr);
    EXPECT_EQ(maxFn->getName(), "max");
    EXPECT_TRUE(maxFn->isGeneric());
    ASSERT_EQ(maxFn->getTypeParams().size(), 1);
    EXPECT_EQ(maxFn->getTypeParams()[0], "T");

    // max<int>(10, 20) call
    auto* testFn = dynamic_cast<ast::FunctionDecl*>(program->getDeclarations()[3].get());
    ASSERT_NE(testFn, nullptr);
    ASSERT_NE(testFn->getBody(), nullptr);
    ASSERT_EQ(testFn->getBody()->getStatements().size(), 1);
    auto* varDecl = dynamic_cast<ast::VarDeclStmt*>(testFn->getBody()->getStatements()[0].get());
    ASSERT_NE(varDecl, nullptr);
    auto* callExpr = dynamic_cast<const ast::CallExpr*>(varDecl->getInitializer());
    ASSERT_NE(callExpr, nullptr);
    EXPECT_TRUE(callExpr->isGeneric());
    ASSERT_EQ(callExpr->getTypeArgs().size(), 1);
    EXPECT_EQ(callExpr->getTypeArgs()[0], "int");
}

} // namespace sp::parser::test
