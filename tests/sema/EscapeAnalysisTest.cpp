#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/EscapeAnalysis.h"
#include "sp/sema/SemanticAnalyzer.h"

using namespace sp::diagnostics;
using namespace sp::lexer;
using namespace sp::parser;
using namespace sp::sema;
using namespace sp::ast;

namespace {

std::unique_ptr<sp::ast::Program> parseSource(const std::string& source, DiagnosticEngine& diag) {
    Lexer lexer(source, "test.sp", &diag);
    auto tokens = lexer.tokenizeAll();
    if (diag.hasErrors()) return nullptr;

    Parser parser(tokens, "test.sp", &diag);
    return parser.parseProgram();
}

} // namespace

TEST(EscapeAnalysisTest, LocalObjectDoesNotEscape) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Connection {
            fn close() {}
        }

        fn process() {
            var conn = new Connection();
            conn.close();
        }
    )";

    auto program = parseSource(source, diag);
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());

    EscapeAnalysis escapeAnalysis;
    escapeAnalysis.analyze(*program);

    EXPECT_FALSE(escapeAnalysis.escapes("process", "conn"));
    EXPECT_EQ(escapeAnalysis.getEscapeState("process", "conn"), EscapeState::LocalNonEscaping);
}

TEST(EscapeAnalysisTest, ReturnedObjectEscapes) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class User {
            int id;
        }

        fn createUser() -> User {
            var u = new User();
            return u;
        }
    )";

    auto program = parseSource(source, diag);
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());

    EscapeAnalysis escapeAnalysis;
    escapeAnalysis.analyze(*program);

    EXPECT_TRUE(escapeAnalysis.escapes("createUser", "u"));
    EXPECT_EQ(escapeAnalysis.getEscapeState("createUser", "u"), EscapeState::GlobalEscaping);
}

TEST(EscapeAnalysisTest, ObjectAssignedToMemberFieldEscapes) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Payload {}
        class Envelope {
            Payload data;
        }

        fn attach(env: Envelope) {
            var payload = new Payload();
            env.data = payload;
        }
    )";

    auto program = parseSource(source, diag);
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());

    EscapeAnalysis escapeAnalysis;
    escapeAnalysis.analyze(*program);

    EXPECT_TRUE(escapeAnalysis.escapes("attach", "payload"));
    EXPECT_EQ(escapeAnalysis.getEscapeState("attach", "payload"), EscapeState::GlobalEscaping);
}

TEST(EscapeAnalysisTest, ObjectAssignedToCollectionIndexEscapes) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Item {}

        fn storeInArray(arr: Item[]) {
            var item = new Item();
            arr[0] = item;
        }
    )";

    auto program = parseSource(source, diag);
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());

    EscapeAnalysis escapeAnalysis;
    escapeAnalysis.analyze(*program);

    EXPECT_TRUE(escapeAnalysis.escapes("storeInArray", "item"));
    EXPECT_EQ(escapeAnalysis.getEscapeState("storeInArray", "item"), EscapeState::GlobalEscaping);
}

TEST(EscapeAnalysisTest, AliasedObjectEscapesWhenAliasEscapes) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Buffer {}

        fn testAlias() -> Buffer {
            var orig = new Buffer();
            var alias = orig;
            return alias;
        }
    )";

    auto program = parseSource(source, diag);
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());

    EscapeAnalysis escapeAnalysis;
    escapeAnalysis.analyze(*program);

    EXPECT_TRUE(escapeAnalysis.escapes("testAlias", "alias"));
    EXPECT_TRUE(escapeAnalysis.escapes("testAlias", "orig"));
}

TEST(EscapeAnalysisTest, NonEscapingAliasedObjectsStayLocal) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Session {}

        fn testLocalAliases() {
            var s1 = new Session();
            var s2 = s1;
            var s3 = s2;
        }
    )";

    auto program = parseSource(source, diag);
    ASSERT_NE(program, nullptr);
    EXPECT_FALSE(diag.hasErrors());

    EscapeAnalysis escapeAnalysis;
    escapeAnalysis.analyze(*program);

    EXPECT_FALSE(escapeAnalysis.escapes("testLocalAliases", "s1"));
    EXPECT_FALSE(escapeAnalysis.escapes("testLocalAliases", "s2"));
    EXPECT_FALSE(escapeAnalysis.escapes("testLocalAliases", "s3"));
}
