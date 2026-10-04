#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"

using namespace sp::diagnostics;
using namespace sp::lexer;
using namespace sp::parser;
using namespace sp::sema;

namespace {

bool analyzeSource(const std::string& source, DiagnosticEngine& diag) {
    Lexer lexer(source, "test.sp", &diag);
    auto tokens = lexer.tokenizeAll();
    if (diag.hasErrors()) return false;

    Parser parser(tokens, "test.sp", &diag);
    auto program = parser.parseProgram();
    if (diag.hasErrors() || !program) return false;

    SemanticAnalyzer sema(diag);
    return sema.analyze(*program);
}

} // namespace

TEST(TypeCheckerTest, ValidVariableDeclarationAndInference) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn main() {
            var x = 42;
            let msg = "Hello SP";
            const double PI = 3.14159;
            bool active = true;
        }
    )";
    EXPECT_TRUE(analyzeSource(source, diag));
    EXPECT_FALSE(diag.hasErrors());
}

TEST(TypeCheckerTest, TypeMismatchInAssignmentReportsE1001) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn main() {
            int x = "incompatible string";
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_EQ(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, UndefinedVariableReportsE1002) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn main() {
            println(unknownVariable);
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, UndefinedFunctionReportsE1003) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn main() {
            nonExistentFunction(42);
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, DuplicateDeclarationReportsE1004) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn main() {
            int a = 10;
            int a = 20;
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, ReturnTypeMismatchReportsE1005) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn getNumber() -> int {
            return "not a number";
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, MissingReturnInNonVoidFunctionReportsE1005) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn calculate(int x) -> int {
            if (x > 0) {
                return 1;
            }
            // Missing else return branch
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, NullSafetyAssignNullToNonNullableReportsE1008) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn main() {
            string s = null; // Error: string is non-nullable
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, NullSafetyNullableTypeAllowed) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        fn main() {
            string? s = null; // Valid
            int? count = 42;  // Valid
        }
    )";
    EXPECT_TRUE(analyzeSource(source, diag));
    EXPECT_FALSE(diag.hasErrors());
}

TEST(TypeCheckerTest, NullSafetyDotAccessRequiresSafeNavReportsE1008) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Person {
            public string name;
        }

        fn check(Person? p) {
            println(p.name); // Error: access on nullable without ?.
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, ValidSafeNavigationAndNullCoalescing) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Person {
            public string name;
        }

        fn check(Person? p) {
            string name = p?.name ?? "Anonymous";
            println(name);
        }
    )";
    EXPECT_TRUE(analyzeSource(source, diag));
    EXPECT_FALSE(diag.hasErrors());
}

TEST(TypeCheckerTest, PrivateMemberAccessViolationReportsE1006) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Secret {
            private int code;

            Secret(int c) {
                this.code = c;
            }
        }

        fn main() {
            Secret s = new Secret(1234);
            int leaked = s.code; // Error: private field access
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}

TEST(TypeCheckerTest, ClassInheritanceAndMethodOverrideSuccess) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        class Animal {
            protected string name;

            Animal(string name) {
                this.name = name;
            }

            public fn speak() -> string {
                return "sound";
            }
        }

        class Dog extends Animal {
            Dog(string name) {
                super(name);
            }

            override fn speak() -> string {
                return "woof";
            }
        }

        fn main() {
            Dog d = new Dog("Buddy");
            Animal a = d;
            println(a.speak());
        }
    )";
    EXPECT_TRUE(analyzeSource(source, diag));
    EXPECT_FALSE(diag.hasErrors());
}

TEST(TypeCheckerTest, InterfaceImplementationCompleteness) {
    DiagnosticEngine diag(std::cerr);
    std::string source = R"(
        interface Greeter {
            fn greet() -> string;
        }

        class IncompleteGreeter : Greeter {
            // Error: does not implement greet()
        }
    )";
    EXPECT_FALSE(analyzeSource(source, diag));
    EXPECT_TRUE(diag.hasErrors());
    EXPECT_GE(diag.errorCount(), 1);
}
