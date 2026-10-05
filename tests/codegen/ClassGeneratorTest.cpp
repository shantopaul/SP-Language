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

TEST(ClassGeneratorTest, NewObjectInstantiationAndFieldAccessAndMethodCall) {
    std::string source = R"(
        class Counter {
            var count: int;

            Counter(int initial) {
                this.count = initial;
            }

            fn increment() {
                this.count = this.count + 1;
            }

            fn getCount() -> int {
                return this.count;
            }
        }

        fn main() -> int {
            var c: Counter = new Counter(42);
            c.increment();
            return c.getCount();
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irCode = printer.print(*module);

    // Allocates object with sp_rt_alloc
    EXPECT_NE(irCode.find("call ptr @sp_rt_alloc(i64"), std::string::npos);
    // Calls Counter_Counter
    EXPECT_NE(irCode.find("call void @Counter_Counter(ptr"), std::string::npos);
    // Method call on object via virtual or direct call
    EXPECT_NE(irCode.find("Counter_increment"), std::string::npos);
}

TEST(ClassGeneratorTest, VirtualDispatchPolymorphism) {
    std::string source = R"(
        class Animal {
            fn speak() -> int {
                return 10;
            }
        }

        class Dog extends Animal {
            override fn speak() -> int {
                return 20;
            }
        }

        fn main() -> int {
            var a: Animal = new Dog();
            return a.speak();
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irCode = printer.print(*module);

    // Dog vtable contains Dog_speak at slot 0
    EXPECT_NE(irCode.find("@_vtable_Dog = constant [1 x ptr] [ptr @Dog_speak]"), std::string::npos);

    // Virtual call indexes into vtable
    EXPECT_NE(irCode.find("getelementptr inbounds %class.Animal, ptr"), std::string::npos);
    EXPECT_NE(irCode.find("getelementptr inbounds ptr, ptr"), std::string::npos);
    EXPECT_NE(irCode.find("call i32 %fn_ptr"), std::string::npos);
}

TEST(ClassGeneratorTest, StructValueTypeAndFieldAccess) {
    std::string source = R"(
        struct Point {
            int x;
            int y;
        }

        fn main() -> int {
            Point p;
            p.x = 10;
            p.y = 20;
            return p.x + p.y;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irCode = printer.print(*module);

    // Declares struct type
    EXPECT_NE(irCode.find("%struct.Point = type { i32, i32 }"), std::string::npos);
    // Stack allocation of struct
    EXPECT_NE(irCode.find("alloca %struct.Point"), std::string::npos);
    // GEP access to fields
    EXPECT_NE(irCode.find("getelementptr inbounds %struct.Point, ptr"), std::string::npos);
}

TEST(ClassGeneratorTest, EnumConstantsCodegen) {
    std::string source = R"(
        enum HttpStatus {
            OK = 200,
            NotFound = 404
        }

        fn main() -> int {
            var code: int = HttpStatus.OK;
            return code;
        }
    )";

    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);

    ir::IRPrinter printer;
    std::string irCode = printer.print(*module);

    // Uses 200 constant
    EXPECT_NE(irCode.find("store i32 200, ptr"), std::string::npos);
}


