// TryCatchTest.cpp — IR-level tests for Phase 3 slice B (pro-4.0.0):
// real try/catch/finally + throw lowering, `?` operator lowering, and
// map-literal lowering. Execution-level coverage lives in
// tests/codegen/TryCatchExecutionTest.cpp.
//
// These tests verify that programs lower to a valid IR module (or fail
// loudly with error[E1000] where the slice deliberately does not
// support a construct), mirroring the compileToIR helper in IRBuilderTest.

#include <gtest/gtest.h>

#include <iostream>
#include <memory>
#include <string>

#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/ir/IRBuilder.h"
#include "sp/ir/IRPrinter.h"

using namespace sp;

namespace {

// Compile SP source to an IR module. Returns nullptr when any phase
// (parse / sema / IR build) reports an error.
static std::unique_ptr<ir::IRModule> compileToIR(const std::string& source) {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lex(source, "test.sp", &diags);
    auto tokens = lex.tokenizeAll();
    if (diags.hasErrors()) return nullptr;
    parser::Parser parser(tokens, "test.sp", &diags);
    auto program = parser.parseProgram();
    if (!program || diags.hasErrors()) return nullptr;
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

} // namespace

// --- try/catch/finally lowering -------------------------------------------

TEST(TryCatchTest, TryCatchFinallyNoThrowCompiles) {
    const std::string source = R"(
        fn main() {
            try {
                println("in try");
            } catch (Exception e) {
                println("in catch");
            } finally {
                println("in finally");
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "sp_rt_println_string"));
    // The lowering creates distinct blocks for each region.
    EXPECT_TRUE(hasInstruction(ir, "try"));
    EXPECT_TRUE(hasInstruction(ir, "catch"));
    EXPECT_TRUE(hasInstruction(ir, "finally"));
    EXPECT_TRUE(hasInstruction(ir, "try_merge"));
}

TEST(TryCatchTest, ThrowInsideTryLowers) {
    const std::string source = R"(
        fn main() {
            try {
                throw "boom";
                println("after throw");
            } catch (Exception e) {
                println(e);
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // The throw sets the pending flag and branches to the catch block.
    EXPECT_TRUE(hasInstruction(ir, "throw_pending"));
    EXPECT_TRUE(hasInstruction(ir, "throw_payload"));
    EXPECT_TRUE(hasInstruction(ir, "catch"));
}

TEST(TryCatchTest, ThrowInResultFunctionLowersToErr) {
    const std::string source = R"(
        fn fail() -> Result<int, string> {
            throw "nope";
        }
        fn main() {
            var r = fail();
            println(r.isErr());
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // Uncaught throw in a Result function builds an Err tagged union.
    EXPECT_TRUE(hasInstruction(ir, "uncaught_throw"));
}

TEST(TryCatchTest, ThrowOfArrayLowers) {
    // Phase 5 slice B: array throws round-trip through the raw slot.
    const std::string source = R"(
        fn main() {
            var arr = [1, 2, 3];
            try {
                throw arr;
            } catch (Exception e) {
                println(e);
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "throw_raw"));
    EXPECT_TRUE(hasInstruction(ir, "throw_type"));
}

TEST(TryCatchTest, ThrowOfMapLowers) {
    // Phase 5 slice B: map throws round-trip through the raw slot.
    const std::string source = R"(
        fn main() {
            var m = {"a": 1};
            try {
                throw m;
            } catch (Exception e) {
                println(e);
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "throw_raw"));
}

TEST(TryCatchTest, ThrowOfClassLowers) {
    // Phase 5 slice B: class-object throws round-trip through the raw
    // slot, with the owning-reference flag for the payload.
    const std::string source = R"(
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn main() {
            try {
                throw new Counter(5);
            } catch (Exception e) {
                println(e);
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "throw_raw"));
    EXPECT_TRUE(hasInstruction(ir, "throw_owns"));
}

TEST(TryCatchTest, ThrowOfStructLowers) {
    // Phase 5 slice B: by-value struct throws copy into a per-type
    // struct slot.
    const std::string source = R"(
        struct Point {
            int x;
            int y;
        }
        fn main() {
            Point p;
            p.x = 3;
            p.y = 4;
            try {
                throw p;
            } catch (Exception e) {
                println(e);
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "throw_struct"));
}

TEST(TryCatchTest, ThrowOfResultFailsLoudly) {
    // Phase 5 slice B: Result/Option values are not throwable by
    // design -- they are the return-value error channel (ADR-002
    // direction), so throwing one must fail loudly.
    const std::string source = R"(
        fn main() {
            var r = Result.Err("bad");
            try {
                throw r;
            } catch (Exception e) {
                println(e);
            }
        }
    )";
    auto module = compileToIR(source);
    EXPECT_EQ(module, nullptr);
}

TEST(TryCatchTest, TypedCatchDispatchLowers) {
    // Phase 5 slice B: a multi-catch lowers to a type-tag dispatch
    // (one check per clause, first match wins, no-match propagates).
    const std::string source = R"(
        fn main() {
            try {
                throw 42;
            } catch (string e) {
                println("str");
            } catch (int e) {
                println("int");
            } catch (Exception e) {
                println("any");
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "catch_tag"));
    EXPECT_TRUE(hasInstruction(ir, "catch_match"));
    EXPECT_TRUE(hasInstruction(ir, "catch_clause"));
    EXPECT_TRUE(hasInstruction(ir, "catch_nomatch"));
}

TEST(TryCatchTest, BreakOutOfTryWithFinallyFailsLoudly) {
    const std::string source = R"(
        fn main() {
            var i = 0;
            while (i < 3) {
                try {
                    break;
                } finally {
                    println("fin");
                }
                i = i + 1;
            }
        }
    )";
    auto module = compileToIR(source);
    EXPECT_EQ(module, nullptr);
}

TEST(TryCatchTest, BreakOfInnerLoopInsideTryIsAllowed) {
    const std::string source = R"(
        fn main() {
            try {
                var i = 0;
                while (i < 3) {
                    break;
                }
                println("after loop");
            } finally {
                println("fin");
            }
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
}

// --- `?` operator lowering (already implemented; regression cover) --------

TEST(TryCatchTest, TryOperatorLowers) {
    const std::string source = R"(
        fn maybe() -> Result<int, string> {
            return Ok(7);
        }
        fn run() -> Result<int, string> {
            var v = maybe()?;
            return Ok(v + 1);
        }
        fn main() {
            var r = run();
            println(r.unwrap());
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    // The `?` lowering branches on the tagged-union tag.
    EXPECT_TRUE(hasInstruction(ir, "try"));
}

// --- map literal lowering --------------------------------------------------

TEST(TryCatchTest, MapLiteralCompiles) {
    const std::string source = R"(
        fn main() {
            var m = {"a": 1, "b": 2};
            println(m["a"]);
            println(m.size());
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "sp_rt_map_alloc"));
    EXPECT_TRUE(hasInstruction(ir, "sp_rt_map_set_str"));
}

TEST(TryCatchTest, MapLiteralIntKeysCompile) {
    const std::string source = R"(
        fn main() {
            var m = {1: 10, 2: 20};
            println(m[1]);
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "sp_rt_map_alloc"));
    EXPECT_TRUE(hasInstruction(ir, "sp_rt_map_set_i64"));
}

TEST(TryCatchTest, EmptyMapLiteralCompiles) {
    const std::string source = R"(
        fn main() {
            var m = {};
            println(m.isEmpty());
        }
    )";
    auto module = compileToIR(source);
    ASSERT_NE(module, nullptr);
    std::string ir = dumpIR(*module);
    EXPECT_TRUE(hasInstruction(ir, "sp_rt_map_alloc"));
}

TEST(TryCatchTest, MapLiteralStringValueFailsLoudly) {
    // The map runtime stores int64 values only; a string value has no
    // representation and must fail loudly instead of being silently
    // dropped or corrupted.
    const std::string source = R"(
        fn main() {
            var m = {"a": "hello"};
            println(m["a"]);
        }
    )";
    auto module = compileToIR(source);
    EXPECT_EQ(module, nullptr);
}
