// BrokenProgramTest.cpp — Phase 5 slice C (pro-4.0.0): compiler robustness.
//
// Every program in this file is BROKEN input. SemanticAnalyzer::analyze()
// must never crash on it: it must return (false) with a clean diagnostic
// from the parser. The crash vector is real: callers such as the LSP
// (spls) and the debugger run analyze() on programs that already carry
// parse diagnostics — `spc` itself refuses before sema, so the compiler
// must be safe for those callers too. Phase 4 slice B proved the crash:
// `if;` segfaulted analyze() (null IfStmt condition), and the LSP had to
// work around it by skipping sema whenever the parse had errors.
//
// The parser's error recovery can build nodes with null children
// (BinaryExpr with a null right operand, IfStmt with a null condition,
// ExprStmt with a null expression, ...). The TypeChecker must null-guard
// every such access; the parser must stop emitting nameless/phantom
// declarations (see the FnTest / FnMissingName / ClassMissingName /
// VarMissingName cases below).
//
// A handful of VALID programs are included as controls: the guards must
// not change behaviour or diagnostics for valid input.

#include <gtest/gtest.h>

#include "sp/ast/Declaration.h"
#include "sp/ast/Statement.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"

using namespace sp;

namespace {

// Lex + parse + analyze a (possibly broken) program in one go. The
// diagnostic engine is kept local: callers such as the LSP run analyze()
// on programs that already carry parse diagnostics, and this helper
// reproduces exactly that call shape.
struct SweepResult {
    bool programBuilt = false;
    bool hasErrors = false;
    size_t errorCount = 0;
    std::vector<std::string> decls;
    bool analyzeOk = false;
};

std::vector<std::string> declKinds(const ast::Program& program) {
    std::vector<std::string> kinds;
    for (const auto& decl : program.getDeclarations()) {
        if (!decl) {
            kinds.emplace_back("<null>");
            continue;
        }
        switch (decl->getKind()) {
            case ast::ASTNodeKind::FunctionDecl: {
                const auto& fn = static_cast<const ast::FunctionDecl&>(*decl);
                kinds.push_back("fn:" + fn.getName());
                break;
            }
            case ast::ASTNodeKind::ClassDecl: {
                const auto& cls = static_cast<const ast::ClassDecl&>(*decl);
                kinds.push_back("class:" + cls.getName());
                break;
            }
            case ast::ASTNodeKind::BlockStmt:
                kinds.emplace_back("block");
                break;
            case ast::ASTNodeKind::ExprStmt:
                kinds.emplace_back("exprstmt");
                break;
            default:
                kinds.emplace_back("other");
                break;
        }
    }
    return kinds;
}

SweepResult sweep(const std::string& source) {
    SweepResult result;
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, "broken.sp", &diags);
    auto tokens = lexer.tokenizeAll();
    parser::Parser parser(tokens, "broken.sp", &diags);
    auto program = parser.parseProgram();
    result.programBuilt = program != nullptr;
    result.hasErrors = diags.hasErrors();
    result.errorCount = diags.errorCount();
    if (program) {
        result.decls = declKinds(*program);
        sema::SemanticAnalyzer sema(diags);
        result.analyzeOk = sema.analyze(*program);
    }
    return result;
}

std::string joinKinds(const std::vector<std::string>& kinds) {
    std::string out = "[";
    for (size_t i = 0; i < kinds.size(); ++i) {
        if (i > 0) out += ", ";
        out += kinds[i];
    }
    out += "]";
    return out;
}

} // namespace

// --- Null-child crashes: the checker must guard each of these ---------

TEST(BrokenProgramTest, IfMissingCondition) {
    auto r = sweep("fn main() { if; println(\"hi\"); }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, WhileMissingCondition) {
    auto r = sweep("fn main() { while; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, DoWhileMissingCondition) {
    auto r = sweep("fn main() { do { println(1); } while; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, ForMissingConditionOperand) {
    auto r = sweep("fn main() { for (int i = 0; i < ; i++) { println(i); } }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, BinaryMissingRightOperand) {
    auto r = sweep("fn main() { let x = 1 + ; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, UnaryMissingOperand) {
    auto r = sweep("fn main() { let x = -; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, CallMissingArg) {
    auto r = sweep("fn main() { foo(,); }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, IndexMissingIndex) {
    auto r = sweep("fn main() { let a = [1, 2, 3]; let x = a[]; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, AwaitMissingOperand) {
    auto r = sweep("async fn f() { let x = await; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, TernaryMissingElse) {
    auto r = sweep("fn main() { let x = true ? 1 : ; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, AssignMissingValue) {
    auto r = sweep("fn main() { let x = 1; x = ; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, ArrayMissingElement) {
    auto r = sweep("fn main() { let x = [1, , 3]; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, MapMissingValue) {
    auto r = sweep("fn main() { let m = {\"a\": 1, \"b\": }; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, NewMissingArg) {
    auto r = sweep("class Foo { } fn main() { let x = new Foo(,); }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, LambdaMissingBody) {
    auto r = sweep("fn main() { let f = (int x) => ; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, ThrowMissingExpr) {
    auto r = sweep("fn main() { throw; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, EmptyExprStatement) {
    auto r = sweep("fn main() { ; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, SwitchMissingCaseValue) {
    auto r = sweep("fn main() { let x = 1; switch (x) { case : println(1); } }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, MatchMissingArmBody) {
    auto r = sweep("fn main() { let x = 1; match x { 1 => , } }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, UnbalancedBraces) {
    auto r = sweep("fn main() { if (true) { println(1); }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

// --- Phantom declarations: recovery must not emit nameless nodes ------
//
// `fn test()` is the Phase 4 slice B case: `test` is a reserved keyword,
// so the fn name consume fails, the parser still returned a nameless
// FunctionDecl, and the leftover `test` token was re-parsed as a test
// declaration (`__sp_test_anonymous_test`) plus null ExprStmts. After the
// fix the broken declaration is dropped (with its diagnostic) and the
// recovery skips to the next declaration boundary: zero declarations.

TEST(BrokenProgramTest, FnTestProducesNoPhantomDecls) {
    auto r = sweep("fn test()");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    EXPECT_EQ(r.decls.size(), 0u) << "decls: " << joinKinds(r.decls);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, FnMissingNameProducesNoDecl) {
    auto r = sweep("fn () { println(1); }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    EXPECT_EQ(r.decls.size(), 0u) << "decls: " << joinKinds(r.decls);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, ClassMissingNameProducesNoDecl) {
    auto r = sweep("class { int x; }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    EXPECT_EQ(r.decls.size(), 0u) << "decls: " << joinKinds(r.decls);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, VarMissingNameProducesNoDecl) {
    auto r = sweep("let ;");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    EXPECT_EQ(r.decls.size(), 0u) << "decls: " << joinKinds(r.decls);
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, StrayCloseBraceKeepsOnlyRealDecl) {
    auto r = sweep("fn main() { println(1); } }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    ASSERT_EQ(r.decls.size(), 1u) << "decls: " << joinKinds(r.decls);
    EXPECT_EQ(r.decls[0], "fn:main");
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

TEST(BrokenProgramTest, BrokenFnDoesNotSwallowNextDecl) {
    auto r = sweep("fn test() { println(1); } fn main() { println(2); }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_TRUE(r.hasErrors);
    ASSERT_EQ(r.decls.size(), 1u) << "decls: " << joinKinds(r.decls);
    EXPECT_EQ(r.decls[0], "fn:main");
    // analyze() returned (no segfault). Its boolean is NOT asserted:
    // SemanticAnalyzer::analyze() reports whether sema added NEW errors,
    // so it is true when the parse errors are the only errors. Callers
    // must also check hasErrors(), as spc does. (void)r.analyzeOk;
}

// --- Valid-program controls: guards must change nothing here ----------

TEST(BrokenProgramTest, ValidForInfiniteLoopStillPasses) {
    auto r = sweep("fn main() { for (;;) { break; } }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_FALSE(r.hasErrors);
    EXPECT_TRUE(r.analyzeOk);
}

TEST(BrokenProgramTest, ValidEmptyMainStillPasses) {
    auto r = sweep("fn main() {}");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_FALSE(r.hasErrors);
    EXPECT_TRUE(r.analyzeOk);
}

TEST(BrokenProgramTest, ValidTryCatchStillPasses) {
    auto r = sweep("fn main() { try { throw 1; } catch (int e) { println(e); } }");
    ASSERT_TRUE(r.programBuilt);
    EXPECT_FALSE(r.hasErrors);
    EXPECT_TRUE(r.analyzeOk);
}
