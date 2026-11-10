// CodegenCrashersIRTest.cpp — IR-level tests for Phase 6 slice A
// (pro-4.0.0): the seven codegen crashers the Phase 5 doc gate found.
// Execution-level coverage lives in
// tests/codegen/CodegenCrashersExecutionTest.cpp.
//
// Each test compiles a program that previously failed loudly (IR
// verification error / invalid LLVM IR / compiler SIGSEGV) and asserts
// the module now builds, plus a structural check on the emitted IR.

#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <string>

#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/ModuleLoader.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRPrinter.h"

using namespace sp;

namespace {

static std::unique_ptr<ir::IRModule> compileToIR(const std::string& source) {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lex(source, "test.sp", &diags);
    auto tokens = lex.tokenizeAll();
    if (diags.hasErrors()) return nullptr;
    parser::Parser parser(tokens, "test.sp", &diags);
    auto program = parser.parseProgram();
    if (!program || diags.hasErrors()) return nullptr;
    sema::ModuleLoader moduleLoader(diags);
    if (!moduleLoader.loadAndMergeAll(*program, "test.sp")) return nullptr;
    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) return nullptr;
    ir::IRBuilder builder(sema);
    return builder.build(*program, "test_module");
}

static std::string dumpIR(const ir::IRModule& module) {
    ir::IRPrinter printer;
    return printer.print(module);
}

static bool hasInstruction(const std::string& ir, const std::string& needle) {
    return ir.find(needle) != std::string::npos;
}

// Position of `needle` inside the vtable global line for `className`
// (e.g. "@_vtable_Circle = constant ..."), or npos when absent. Used
// to prove that an interface method occupies the SAME slot in every
// implementing class even when the classes declare their methods in
// different orders.
static size_t vtableSlotPos(const std::string& ir, const std::string& className,
                            const std::string& needle) {
    std::string marker = "@_vtable_" + className + " = constant";
    size_t lineStart = ir.find(marker);
    if (lineStart == std::string::npos) return std::string::npos;
    size_t lineEnd = ir.find('\n', lineStart);
    std::string line = ir.substr(lineStart, lineEnd - lineStart);
    size_t pos = line.find(needle);
    return (pos == std::string::npos) ? std::string::npos : pos;
}

} // namespace

// --- 1. Map iteration ------------------------------------------------------

TEST(CodegenCrashersIRTest, MapIterationLowersToMapKeyAccess) {
    const std::string source = R"(
        import std.collections;
        fn main() {
            Map<string, int> ages = new Map<string, int>();
            ages["Shanto"] = 21;
            ages["Alice"] = 25;
            ages["Bob"] = 30;
            var count = 0;
            for (key in ages) {
                println("key=${key}");
                count = count + 1;
            }
            println("count=${count}");
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // The loop must size and fetch keys through the map runtime, never
    // through the array runtime (sp_rt_array_length on a map pointer
    // read the entry-table pointer as a length — garbage, infinite loop).
    // The needles match call instructions, not the `declare` lines the
    // module carries for every runtime function.
    EXPECT_TRUE(hasInstruction(ir, "call i64 @sp_rt_map_size"));
    EXPECT_TRUE(hasInstruction(ir, "call ptr @sp_rt_map_str_key_at"));
    EXPECT_FALSE(hasInstruction(ir, "call i64 @sp_rt_array_length"));
}

TEST(CodegenCrashersIRTest, MapIterationIntKeysLowers) {
    const std::string source = R"(
        import std.collections;
        fn main() {
            Map<int, int> m = new Map<int, int>();
            m[10] = 1;
            m[20] = 2;
            var sum = 0;
            for (k in m) {
                sum = sum + k;
            }
            println("sum=${sum}");
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "call i64 @sp_rt_map_size"));
    EXPECT_TRUE(hasInstruction(ir, "call i64 @sp_rt_map_int_key_at"));
    EXPECT_FALSE(hasInstruction(ir, "call i64 @sp_rt_array_length"));
}

// --- 2. Interface-typed method calls ---------------------------------------

TEST(CodegenCrashersIRTest, InterfaceDispatchLowers) {
    const std::string source = R"(
        interface Drawable {
            fn draw();
            fn getColor() -> string;
        }
        class Circle implements Drawable {
            double radius;
            string color;
            Circle(double radius, string color) {
                this.radius = radius;
                this.color = color;
            }
            fn draw() { println("Circle"); }
            fn getColor() -> string { return color; }
        }
        fn main() {
            Drawable d1 = new Circle(3.0, "red");
            d1.draw();
            println("color=${d1.getColor()}");
        }
    )";
    // Before the fix this failed IR verification with
    // "call to undeclared function 'draw'".
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "@_vtable_Circle"));
}

TEST(CodegenCrashersIRTest, InterfaceSlotIsStableAcrossDeclarationOrders) {
    const std::string source = R"(
        interface Drawable {
            fn draw();
            fn getColor() -> string;
        }
        class Circle implements Drawable {
            fn draw() { println("Circle"); }
            fn getColor() -> string { return "red"; }
        }
        class Square implements Drawable {
            fn getColor() -> string { return "blue"; }
            fn draw() { println("Square"); }
        }
        fn main() {
            Drawable a = new Circle();
            Drawable b = new Square();
            a.draw();
            b.draw();
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // Circle declares draw() first, Square declares getColor() first.
    // The interface slot for draw() must be the SAME index in both
    // vtables, or dispatch through a Drawable reference would call
    // the wrong method for one of the two classes.
    size_t circleDraw = vtableSlotPos(ir, "Circle", "@Circle_draw");
    size_t squareDraw = vtableSlotPos(ir, "Square", "@Square_draw");
    ASSERT_NE(circleDraw, std::string::npos);
    ASSERT_NE(squareDraw, std::string::npos);
    EXPECT_EQ(circleDraw, squareDraw);
}

// --- 3/4. Forward method calls (ctor + later-declared, value-returning) ---

TEST(CodegenCrashersIRTest, CtorCallingMethodLowers) {
    const std::string source = R"(
        class User {
            string id;
            string name;
            User(string name) {
                this.name = name;
                this.id = this.generateId();
            }
            fn generateId() -> string {
                return "user-123";
            }
        }
        fn main() {
            var u = new User("Shanto");
            println("id=${u.id}");
        }
    )";
    // Before the fix the compiler segfaulted in IR verification
    // (a null operand reached IRInstruction::format()).
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
}

TEST(CodegenCrashersIRTest, LaterDeclaredValueMethodLowers) {
    const std::string source = R"(
        class Checkout {
            public fn processPayment(double amount, string method) {
                bool ok = false;
                if (method == "card") {
                    ok = this.payByCard(amount);
                }
                if (ok) println("settled");
            }
            private fn payByCard(double amount) -> bool {
                println("charging");
                return true;
            }
        }
        fn main() {
            Checkout shop = new Checkout();
            shop.processPayment(150.0, "card");
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // The call must carry the real bool return type even though
    // payByCard is lowered after processPayment.
    EXPECT_TRUE(hasInstruction(ir, "call i1"));
}

TEST(CodegenCrashersIRTest, BareMethodCallLowers) {
    const std::string source = R"(
        class Bank {
            fn deposit(int amount) {
                validate(amount);
                log("deposited");
            }
            fn validate(int amount) {
                println("validated ${amount}");
            }
            fn log(string msg) {
                println("log: ${msg}");
            }
        }
        fn main() {
            var b = new Bank();
            b.deposit(50);
        }
    )";
    // Before the fix sema failed with E1001: the builtin free function
    // log(double) shadowed the method and the enclosing-class fallback
    // never ran.
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
}

TEST(CodegenCrashersIRTest, BareValueMethodCallLowers) {
    const std::string source = R"(
        class Calc {
            fn run() -> int {
                return doubleIt(21);
            }
            fn doubleIt(int x) -> int {
                return x * 2;
            }
        }
        fn main() {
            var c = new Calc();
            println("run=${c.run()}");
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "call i32"));
}

TEST(CodegenCrashersIRTest, BareMethodCallNotShadowedByMathBuiltin) {
    const std::string source = R"(
        class Banking {
            fn log(string msg) {
                println("log: ${msg}");
            }
            fn deposit(int amount) {
                log("deposited");
            }
        }
        fn main() {
            var b = new Banking();
            b.deposit(50);
        }
    )";
    // Before the fix the math builtin log(double) ran before the
    // enclosing-method check, so log("deposited") lowered to
    // sp_rt_math_log and the method was never called.
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // Note: `declare double @sp_rt_math_log` is always present in the
    // module prologue — assert on the CALL, not the declaration.
    EXPECT_FALSE(hasInstruction(ir, "call double @sp_rt_math_log"));
    EXPECT_TRUE(hasInstruction(ir, "Banking_log"));
}

TEST(CodegenCrashersIRTest, BuiltinLogDoubleStillWinsWhenSemaPicksIt) {
    const std::string source = R"(
        class Calc {
            fn log(string msg) {
                println("log: ${msg}");
            }
            fn compute() -> double {
                return log(2.5);
            }
        }
        fn main() {
            var c = new Calc();
            println("v=${c.compute()}");
        }
    )";
    // Sema resolves log(2.5) to the builtin (free functions first), so
    // IRBuilder must lower it to sp_rt_math_log even though the class
    // has a same-named log(string) method.
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "call double @sp_rt_math_log"));
}

// --- 5. Null coalescing ----------------------------------------------------

TEST(CodegenCrashersIRTest, NullCoalesceLowers) {
    const std::string source = R"(
        fn main() {
            string? maybe = null;
            string name = maybe ?? "default";
            println("name=${name}");
            int? n = null;
            int v = n ?? 42;
            println("v=${v}");
        }
    )";
    // Before the fix this emitted `add ptr %v0, @.str.0` — invalid
    // LLVM IR (the default BinaryExpr case is Add).
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_FALSE(hasInstruction(ir, "add ptr"));
    EXPECT_TRUE(hasInstruction(ir, "coalesce_null"));
}

// --- 6. Safe navigation ----------------------------------------------------

TEST(CodegenCrashersIRTest, SafeNavLowersWithNullCheck) {
    const std::string source = R"(
        class User {
            string name;
            string? email;
            User(string name, string? email) {
                this.name = name;
                this.email = email;
            }
        }
        fn findUser(int id) -> User? {
            if (id == 1) return new User("Shanto", "shanto@email.com");
            return null;
        }
        fn main() {
            User? user3 = findUser(999);
            println("User 3 name: ${user3?.name}");
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // The safe-navigation lowering must test the object against null
    // before touching its fields (the `safenav_null` compare; the
    // program's own `id == 1` also emits an icmp, so the block label
    // is the discriminating check).
    EXPECT_TRUE(hasInstruction(ir, "safenav_null"));
}
