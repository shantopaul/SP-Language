#include <gtest/gtest.h>
#include "sp/ast/Edition.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/diagnostics/DiagnosticEngine.h"

using namespace sp;
using namespace sp::ast;

TEST(EditionTest, ParseAndStringRepresentations) {
    EXPECT_EQ(parseEdition("2026"), Edition::Edition2026);
    EXPECT_EQ(parseEdition("2028"), Edition::Edition2028);
    EXPECT_EQ(parseEdition("2.0"), Edition::Edition2028);
    EXPECT_EQ(parseEdition("unknown"), Edition::Edition2026);

    EXPECT_EQ(editionToString(Edition::Edition2026), "2026");
    EXPECT_EQ(editionToString(Edition::Edition2028), "2028");
}

TEST(EditionTest, CapabilityChecks) {
    EXPECT_FALSE(editionSupportsComptime(Edition::Edition2026));
    EXPECT_TRUE(editionSupportsComptime(Edition::Edition2028));

    EXPECT_FALSE(editionSupportsCoroutines(Edition::Edition2026));
    EXPECT_TRUE(editionSupportsCoroutines(Edition::Edition2028));

    EXPECT_FALSE(editionEnforcesStrictUnsafe(Edition::Edition2026));
    EXPECT_TRUE(editionEnforcesStrictUnsafe(Edition::Edition2028));

    EXPECT_TRUE(isEditionAtLeast(Edition::Edition2028, Edition::Edition2026));
    EXPECT_FALSE(isEditionAtLeast(Edition::Edition2026, Edition::Edition2028));
}

TEST(EditionTest, ComptimeWarningOnEdition2026) {
    std::string source = 
        "fn compute(): int {\n"
        "    let val: int = comptime (2 + 2);\n"
        "    return val;\n"
        "}\n";

    diagnostics::DiagnosticEngine diag;
    lexer::Lexer lexer(source, "<test>", &diag);
    auto tokens = lexer.tokenizeAll();

    parser::Parser parser(tokens, "<test>", &diag);
    parser.setEdition(Edition::Edition2026);
    auto program = parser.parseProgram();
    ASSERT_NE(program, nullptr);

    sema::SemanticAnalyzer sema(diag);
    sema.setEdition(Edition::Edition2026);
    sema.analyze(*program);

    // Should generate warning W1801 about experimental comptime in Edition 2026
    EXPECT_GT(diag.warningCount(), 0);
}

TEST(EditionTest, CleanComptimeOnEdition2028) {
    std::string source = 
        "fn compute(): int {\n"
        "    let val: int = comptime (2 + 2);\n"
        "    return val;\n"
        "}\n";

    diagnostics::DiagnosticEngine diag;
    lexer::Lexer lexer(source, "<test>", &diag);
    auto tokens = lexer.tokenizeAll();

    parser::Parser parser(tokens, "<test>", &diag);
    parser.setEdition(Edition::Edition2028);
    auto program = parser.parseProgram();
    ASSERT_NE(program, nullptr);

    sema::SemanticAnalyzer sema(diag);
    sema.setEdition(Edition::Edition2028);
    sema.analyze(*program);

    // In Edition 2028, comptime is stabilized, so no experimental warning
    EXPECT_EQ(diag.warningCount(), 0);
    EXPECT_EQ(diag.errorCount(), 0);
}
