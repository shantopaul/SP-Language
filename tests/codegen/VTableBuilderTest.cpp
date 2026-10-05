#include <gtest/gtest.h>
#include "sp/codegen/VTableBuilder.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRModule.h"

using namespace sp;

TEST(VTableBuilderTest, ClassFieldLayoutAndInheritedVTable) {
    const std::string source = R"(
        class Animal {
            string name;
            int age;

            fn speak() {
                println("Animal speaks");
            }

            fn move() {
                println("Animal moves");
            }
        }

        class Dog extends Animal {
            int breedId;

            fn speak() {
                println("Woof");
            }

            fn fetch() {
                println("Dog fetches");
            }
        }

        struct Point {
            int x;
            int y;
        }
    )";

    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, "vtable_test.sp", &diags);
    auto tokens = lexer.tokenizeAll();
    ASSERT_FALSE(diags.hasErrors());

    parser::Parser parser(tokens, "vtable_test.sp", &diags);
    auto program = parser.parseProgram();
    ASSERT_NE(program, nullptr);
    ASSERT_FALSE(diags.hasErrors());

    sema::SemanticAnalyzer sema(diags);
    ASSERT_TRUE(sema.analyze(*program));

    ir::IRModule module("test_vtable_mod");
    codegen::VTableBuilder builder(sema);
    builder.buildProgramLayouts(*program, module);

    // 1. Check Animal layout
    const auto* animalLayout = builder.getClassLayout("Animal");
    ASSERT_NE(animalLayout, nullptr);
    EXPECT_EQ(animalLayout->fields.size(), 2);
    // Index 0 is vtable ptr, so fields start at 1
    EXPECT_EQ(builder.getFieldIndex("Animal", "name"), 1);
    EXPECT_EQ(builder.getFieldIndex("Animal", "age"), 2);
    EXPECT_EQ(animalLayout->vtableSlots.size(), 2);
    EXPECT_EQ(builder.getMethodVTableSlot("Animal", "speak"), 0);
    EXPECT_EQ(builder.getMethodVTableSlot("Animal", "move"), 1);
    EXPECT_EQ(animalLayout->vtableSlots[0].mangledName, "Animal_speak");
    EXPECT_EQ(animalLayout->vtableSlots[1].mangledName, "Animal_move");

    // 2. Check Dog layout (inheritance and override)
    const auto* dogLayout = builder.getClassLayout("Dog");
    ASSERT_NE(dogLayout, nullptr);
    // Inherited fields: name (1), age (2), breedId (3)
    EXPECT_EQ(dogLayout->fields.size(), 3);
    EXPECT_EQ(builder.getFieldIndex("Dog", "name"), 1);
    EXPECT_EQ(builder.getFieldIndex("Dog", "age"), 2);
    EXPECT_EQ(builder.getFieldIndex("Dog", "breedId"), 3);

    // Dog overrides speak() at slot 0!
    // Dog inherits move() at slot 1!
    // Dog introduces fetch() at slot 2!
    EXPECT_EQ(dogLayout->vtableSlots.size(), 3);
    EXPECT_EQ(builder.getMethodVTableSlot("Dog", "speak"), 0);
    EXPECT_EQ(dogLayout->vtableSlots[0].mangledName, "Dog_speak"); // overridden!
    EXPECT_EQ(builder.getMethodVTableSlot("Dog", "move"), 1);
    EXPECT_EQ(dogLayout->vtableSlots[1].mangledName, "Animal_move"); // inherited!
    EXPECT_EQ(builder.getMethodVTableSlot("Dog", "fetch"), 2);
    EXPECT_EQ(dogLayout->vtableSlots[2].mangledName, "Dog_fetch"); // new slot!

    // 3. Check Struct Point layout (no vtable)
    const auto* pointLayout = builder.getClassLayout("Point");
    ASSERT_NE(pointLayout, nullptr);
    EXPECT_FALSE(pointLayout->hasVTable);
    EXPECT_EQ(pointLayout->fields.size(), 2);
    EXPECT_EQ(builder.getFieldIndex("Point", "x"), 0);
    EXPECT_EQ(builder.getFieldIndex("Point", "y"), 1);
}
