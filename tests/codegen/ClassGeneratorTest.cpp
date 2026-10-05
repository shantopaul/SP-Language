#include <gtest/gtest.h>
#include "sp/ast/Declaration.h"
#include "sp/codegen/VTableBuilder.h"
#include "sp/codegen/ClassGenerator.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRPrinter.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"

using namespace sp;

namespace {

std::unique_ptr<ir::IRModule> compileToIR(const std::string& source, const std::string& filename = "test.sp") {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, filename, &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return nullptr;

    parser::Parser parser(tokens, filename, &diags);
    auto program = parser.parseProgram();
    if (diags.hasErrors() || !program) return nullptr;

    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) return nullptr;

    ir::IRBuilder builder(sema);
    return builder.build(*program, "test");
}

} // namespace

TEST(ClassGeneratorTest, GeneratesConstructorsMethodsAndThisBinding) {
    std::string source = R"(
        class Point {
            var x: int;
            var y: int;

            Point(int x, int y) {
                this.x = x;
                this.y = y;
            }

            fn getX() -> int {
                return this.x;
            }

            fn getY() -> int {
                return y;
            }
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    // Verify functions exist
    EXPECT_NE(module->getFunction("Point_Point"), nullptr);
    EXPECT_NE(module->getFunction("Point_getX"), nullptr);
    EXPECT_NE(module->getFunction("Point_getY"), nullptr);

    ir::IRPrinter printer;
    std::string irCode = printer.print(*module);

    // Struct type defined
    EXPECT_NE(irCode.find("%class.Point = type { ptr, i32, i32 }"), std::string::npos);

    // VTable defined
    EXPECT_NE(irCode.find("@_vtable_Point ="), std::string::npos);

    // Constructor sets vtable and assigns fields
    EXPECT_NE(irCode.find("define void @Point_Point(ptr %this, i32 %x, i32 %y)"), std::string::npos);
    EXPECT_NE(irCode.find("getelementptr inbounds %class.Point, ptr %this, i32 0, i32 0"), std::string::npos);
    EXPECT_NE(irCode.find("getelementptr inbounds %class.Point, ptr"), std::string::npos);

    // Methods getX and getY load field values via GEP
    EXPECT_NE(irCode.find("define i32 @Point_getX(ptr %this)"), std::string::npos);
    EXPECT_NE(irCode.find("define i32 @Point_getY(ptr %this)"), std::string::npos);
}

TEST(ClassGeneratorTest, GeneratesDefaultConstructorWhenOmitted) {
    std::string source = R"(
        class Entity {
            var id: int;
            fn getId() -> int {
                return this.id;
            }
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    EXPECT_NE(module->getFunction("Entity_Entity"), nullptr);
    EXPECT_NE(module->getFunction("Entity_getId"), nullptr);

    ir::IRPrinter printer;
    std::string irCode = printer.print(*module);
    EXPECT_NE(irCode.find("define void @Entity_Entity(ptr %this)"), std::string::npos);
}

TEST(ClassGeneratorTest, InheritanceSuperConstructorDelegation) {
    std::string source = R"(
        class Base {
            var a: int;
            Base(int a) {
                this.a = a;
            }
        }

        class Derived extends Base {
            var b: int;
            Derived(int a, int b) {
                super(a);
                this.b = b;
            }
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    EXPECT_NE(module->getFunction("Base_Base"), nullptr);
    EXPECT_NE(module->getFunction("Derived_Derived"), nullptr);

    ir::IRPrinter printer;
    std::string irCode = printer.print(*module);

    // Derived constructor calls Base_Base
    EXPECT_NE(irCode.find("call void @Base_Base(ptr"), std::string::npos);
}
