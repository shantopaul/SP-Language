// ExternStructIRTest.cpp — IR-level tests for Phase 39 slice A
// (pro-4.0.0): struct-by-value FFI calls.
//
// The C ABI passes structs BY VALUE; SP-internal calls pass them by
// pointer (mapType). An `extern "C"` declaration must therefore use
// the System V ABI coercion for struct params/returns: a struct of
// <=16 bytes is passed as its eightbyte register values (INTEGER
// eightbyte -> i64, SSE eightbyte -> double/float), exactly as
// clang's frontend coerces the same C call. Declaring the extern fn
// with the struct VALUE type alone is NOT enough: LLVM's backend
// flattens a struct-typed argument into its fields (two i32s in
// EDI/ESI for {i32, i32}), while the C callee reads one 64-bit
// register (RDI) — the call then returns (3+0)*2.0 = 6.0 instead of
// 14.0. Structs > 16 bytes keep the struct type: MEMORY-class
// structs are stack-passed correctly by the backend (verified with
// a 24-byte struct probe). These tests assert the emitted IR shape;
// the live end-to-end proof (real C library, `mini_scale` returning
// 14.0) lives in the slice doc and ~/workspace/p10-probes/minilib/.

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

std::unique_ptr<ir::IRModule> compileToIR(const std::string& source) {
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

std::string dumpIR(const ir::IRModule& module) {
    ir::IRPrinter printer;
    return printer.print(module);
}

bool has(const std::string& ir, const std::string& needle) {
    return ir.find(needle) != std::string::npos;
}

// NOTE: SP `int` is i32 (IRBuilder::mapType), so MiniPoint needs
// int64 fields to be 16 bytes -> two INTEGER eightbytes -> coerced
// to (i64, i64). Big is 24 bytes -> keeps the struct type (stack
// passing).
const char* kProgram = R"(
    struct MiniPoint {
        int64 x;
        int64 y;
    }

    struct Big {
        int32 a;
        int32 b;
        int32 c;
        int32 d;
        int32 e;
        int32 f;
    }

    extern "C" {
        fn mini_scale(p: MiniPoint, f: double) -> double;
        fn mini_make(x: int, y: int) -> MiniPoint;
        fn mini_big_sum(b: Big) -> int32;
    }

    fn takePoint(p: MiniPoint) -> int64 {
        return p.x;
    }

    fn main() {
        MiniPoint pt;
        pt.x = 3;
        pt.y = 4;
        double s = mini_scale(pt, 2.0);
        println("scale ${s}");
        MiniPoint q = mini_make(5, 6);
        println("made ${q.x} ${q.y}");
        Big big;
        big.a = 1;
        println("big ${mini_big_sum(big)}");
    }
)";

} // namespace

TEST(ExternStructIRTest, ExternStructParamIsCoercedToRegisters) {
    auto module = compileToIR(kProgram);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);

    // The extern declaration coerces the 16-byte struct param to its
    // two eightbyte registers (i64, i64), then the double param.
    EXPECT_TRUE(has(ir, "declare double @mini_scale(i64, i64, double)"))
        << ir;
    // And it must NOT carry the struct type (the flattening bug).
    EXPECT_FALSE(has(ir, "declare double @mini_scale(%struct.MiniPoint"));
}

TEST(ExternStructIRTest, ExternStructCallPassesRegisterValues) {
    auto module = compileToIR(kProgram);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);

    // The call site loads the two eightbytes as i64s and passes them
    // (no struct-typed call, no pointer).
    EXPECT_TRUE(has(ir, "call double @mini_scale(i64")) << ir;
    EXPECT_FALSE(has(ir, "call double @mini_scale(%struct.MiniPoint"));
    EXPECT_FALSE(has(ir, "call double @mini_scale(ptr"));
}

TEST(ExternStructIRTest, ExternStructReturnIsCoerced) {
    auto module = compileToIR(kProgram);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);

    // A 16-byte struct return is declared as the two-register struct
    // (mini_make's int params are i32).
    EXPECT_TRUE(has(ir, "declare { i64, i64 } @mini_make(i32, i32)"))
        << ir;
    // And the caller converts the coerced result back to the struct
    // value via memory (LLVM bitcast cannot convert an aggregate to
    // a scalar), so `q.x` / `q.y` work.
    EXPECT_TRUE(has(ir, "store { i64, i64 }")) << ir;
    EXPECT_TRUE(has(ir, "load %struct.MiniPoint")) << ir;
}

TEST(ExternStructIRTest, LargeStructKeepsStructType) {
    auto module = compileToIR(kProgram);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);

    // A 24-byte struct keeps the struct type: the backend
    // stack-passes MEMORY-class structs correctly.
    EXPECT_TRUE(has(ir, "declare i32 @mini_big_sum(%struct.Big)")) << ir;
}

TEST(ExternStructIRTest, PlainSPFunctionStillPassesStructByPointer) {
    auto module = compileToIR(kProgram);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);

    // SP-internal calls keep the pointer convention: a user function
    // with a struct param is defined with a pointer param.
    EXPECT_TRUE(has(ir, "define i64 @takePoint(ptr")) << ir;
    // And its declaration is NOT a struct-typed one.
    EXPECT_FALSE(has(ir, "define i64 @takePoint(%struct.MiniPoint)"));
}
