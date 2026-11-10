// TryCatchExecutionTest.cpp — end-to-end tests for Phase 3 slice B
// (pro-4.0.0): real try/catch/finally + throw, the `?` operator, and
// map literals, executed as real native programs via spc (Clang).
// IR-level coverage lives in tests/ir/TryCatchTest.cpp.
//
// The helper below is a local copy of the runSPProgram pattern from
// ExecutionTest.cpp: parse -> sema -> IR -> LLVM -> link -> run the
// executable and return its combined stdout+stderr.

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "sp/codegen/LLVMGenerator.h"
#include "sp/diagnostics/DiagnosticEngine.h"
#include "sp/ir/IRBuilder.h"
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/ModuleLoader.h"
#include "sp/sema/SemanticAnalyzer.h"

namespace fs = std::filesystem;
using namespace sp;

namespace {

fs::path getOutputDir() {
#ifdef SP_PROJECT_ROOT
    fs::path dir = fs::path(SP_PROJECT_ROOT) / "build" / "test_output";
    if (fs::exists(fs::path(SP_PROJECT_ROOT) / "build")) return dir;
#endif
    return fs::temp_directory_path() / "sp_trycatch_exec_tests";
}

std::string runSPProgram(const std::string& source, const std::string& testName, const std::string& input = "") {
    fs::create_directories(getOutputDir());
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, testName + ".sp", &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return "LEXER_ERROR";
    parser::Parser parser(tokens, testName + ".sp", &diags);
    auto program = parser.parseProgram();
    if (!program || diags.hasErrors()) return "PARSER_ERROR";

    // Phase 5 slice C: resolve + merge imports exactly like `spc` does
    // (ModuleLoader::loadAndMergeAll), mirroring the Phase 4 parent fix
    // for ExecutionTest.cpp. Without this, `import std.<module>` silently
    // contributed nothing to this harness, so a program using a real
    // stdlib class failed sema here while the real compiler accepted it.
    sema::ModuleLoader moduleLoader(diags);
    if (!moduleLoader.loadAndMergeAll(*program, testName + ".sp")) return "MODULE_ERROR";

    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) return "SEMA_ERROR";

    ir::IRBuilder builder(sema);
    auto module = builder.build(*program, testName);
    if (!module) return "IR_ERROR";

    fs::path exePath = getOutputDir() / (testName + ".exe");
    codegen::CodegenOptions opts;
    codegen::LLVMGenerator generator(opts);

    if (!generator.compileToExecutable(*module, exePath.string())) {
        return "CODEGEN_ERROR: " + generator.getLastError();
    }

    std::string outPath = exePath.string() + ".out.txt";
    std::string inPath = exePath.string() + ".in.txt";
    std::string cmd;
    if (!input.empty()) {
        std::ofstream inFile(inPath);
        inFile << input;
        inFile.close();
        cmd = "\"" + exePath.string() + "\" < \"" + inPath + "\" > \"" + outPath + "\" 2>&1";
    } else {
        cmd = "\"" + exePath.string() + "\" > \"" + outPath + "\" 2>&1";
    }
    int code = std::system(cmd.c_str());
    (void)code;
    std::ifstream in(outPath);
    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();

    std::error_code ec;
    fs::remove(outPath, ec);
    fs::remove(inPath, ec);
    fs::remove(exePath, ec);
    return ss.str();
}

} // namespace

// --- throw / catch / finally ----------------------------------------------

TEST(TryCatchExecutionTest, TryCatchFinallyNoThrow) {
    const std::string source = R"(
        fn main() {
            try {
                println("in try");
            } catch (Exception e) {
                println("in catch");
            } finally {
                println("in finally");
            }
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_try_no_throw");
    EXPECT_NE(output.find("in try"), std::string::npos);
    EXPECT_EQ(output.find("in catch"), std::string::npos);
    EXPECT_NE(output.find("in finally"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowCaughtWithValue) {
    const std::string source = R"(
        fn main() {
            try {
                throw "boom";
                println("after throw");
            } catch (Exception e) {
                println("caught: " + e);
            } finally {
                println("fin");
            }
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_caught");
    EXPECT_NE(output.find("caught: boom"), std::string::npos);
    EXPECT_EQ(output.find("after throw"), std::string::npos);
    EXPECT_NE(output.find("fin"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowIntCaughtAsInt) {
    // Phase 5 slice B: typed catches -- `throw 42` binds as an int in
    // `catch (int e)`. (Previously the first catch won regardless of
    // type and `e` was always the stringified value.)
    const std::string source = R"(
        fn main() {
            try {
                throw 42;
            } catch (int e) {
                println("code " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_int");
    EXPECT_NE(output.find("code 42"), std::string::npos);
}

TEST(TryCatchExecutionTest, NestedTryCatch) {
    const std::string source = R"(
        fn main() {
            try {
                try {
                    throw "inner";
                } catch (Exception e) {
                    println("inner catch: " + e);
                }
                println("between");
                throw "outer";
            } catch (Exception e) {
                println("outer catch: " + e);
            }
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_nested_try");
    EXPECT_NE(output.find("inner catch: inner"), std::string::npos);
    EXPECT_NE(output.find("between"), std::string::npos);
    EXPECT_NE(output.find("outer catch: outer"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowInsideCatchPropagatesOutward) {
    const std::string source = R"(
        fn main() {
            try {
                try {
                    throw "first";
                } catch (Exception e) {
                    println("first catch");
                    throw "second";
                }
            } catch (Exception e) {
                println("second catch: " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_rethrow");
    EXPECT_NE(output.find("first catch"), std::string::npos);
    EXPECT_NE(output.find("second catch: second"), std::string::npos);
}

TEST(TryCatchExecutionTest, ReturnInsideTryWithFinallyRunsFinally) {
    const std::string source = R"(
        fn answer() -> int {
            try {
                return 42;
            } finally {
                println("fin");
            }
        }
        fn main() {
            println(answer());
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_return_finally");
    EXPECT_NE(output.find("fin"), std::string::npos);
    EXPECT_NE(output.find("42"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowInResultFunctionBecomesErr) {
    const std::string source = R"(
        fn fail() -> Result<int, string> {
            throw "nope";
        }
        fn main() {
            var r = fail();
            println(r.isErr());
            println(r.unwrapErr());
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_result");
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("nope"), std::string::npos);
}

TEST(TryCatchExecutionTest, UncaughtThrowPanics) {
    const std::string source = R"(
        fn main() {
            throw "fatal";
            println("never");
        }
    )";
    std::string output = runSPProgram(source, "exec_uncaught_throw");
    EXPECT_NE(output.find("SP Panic: fatal"), std::string::npos);
    EXPECT_EQ(output.find("never"), std::string::npos);
}

TEST(TryCatchExecutionTest, Example27TryCatchRuns) {
    // examples/27_try_catch.sp (the headline proof): two try/catch/
    // finally blocks that never throw — try runs, catch skipped,
    // finally runs.
    const std::string source = R"(
        fn divide(int a, int b) -> int {
            if (b == 0) {
                println("Error: Division by zero");
                return 0;
            }
            return a / b;
        }
        fn main() {
            try {
                var result = divide(10, 2);
                println("Result: " + result);
            } catch (Exception e) {
                println("Caught exception: " + e);
            } finally {
                println("Finally block executed");
            }
            try {
                var result2 = divide(10, 0);
                println("Result: " + result2);
            } catch (Exception e) {
                println("Caught exception: " + e);
            } finally {
                println("Finally block executed");
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_example27");
    EXPECT_NE(output.find("Result: 5"), std::string::npos);
    EXPECT_NE(output.find("Error: Division by zero"), std::string::npos);
    EXPECT_NE(output.find("Result: 0"), std::string::npos);
    EXPECT_EQ(output.find("Caught exception"), std::string::npos);
    // "Finally block executed" must appear exactly twice.
    size_t first = output.find("Finally block executed");
    ASSERT_NE(first, std::string::npos);
    size_t second = output.find("Finally block executed", first + 1);
    EXPECT_NE(second, std::string::npos);
    EXPECT_EQ(output.find("Finally block executed", second + 1), std::string::npos);
}

// --- `?` operator (already implemented; end-to-end regression cover) -----

TEST(TryCatchExecutionTest, TryOperatorInReturnPositionAndOption) {
    const std::string source = R"(
        fn maybe() -> Option<string> {
            return Some("hi");
        }
        fn greet() -> Option<string> {
            var name = maybe()?;
            return Some("hello " + name);
        }
        fn main() {
            var g = greet();
            println(g.unwrap());
        }
    )";
    std::string output = runSPProgram(source, "exec_try_opt");
    EXPECT_NE(output.find("hello hi"), std::string::npos);
}

TEST(TryCatchExecutionTest, TryOperatorPropagatesErr) {
    const std::string source = R"(
        fn fail() -> Result<int, string> {
            return Err("bad");
        }
        fn run() -> Result<int, string> {
            var v = fail()?;
            return Ok(v + 1);
        }
        fn main() {
            var r = run();
            println(r.isErr());
            println(r.unwrapErr());
        }
    )";
    std::string output = runSPProgram(source, "exec_try_err");
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("bad"), std::string::npos);
}

// --- typed catches + composite throws (Phase 5 slice B) --------------

TEST(TryCatchExecutionTest, TypedCatchSkipsNonMatchingClause) {
    const std::string source = R"(
        fn main() {
            try {
                throw 42;
            } catch (string e) {
                println("str " + e);
            } catch (int e) {
                println("int " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_typed_skip_str");
    EXPECT_EQ(output.find("str "), std::string::npos);
    EXPECT_NE(output.find("int 42"), std::string::npos);
}

TEST(TryCatchExecutionTest, TypedCatchStringSkipsIntClause) {
    const std::string source = R"(
        fn main() {
            try {
                throw "oops";
            } catch (int e) {
                println("int " + e);
            } catch (string e) {
                println("str " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_typed_skip_int");
    EXPECT_EQ(output.find("int "), std::string::npos);
    EXPECT_NE(output.find("str oops"), std::string::npos);
}

TEST(TryCatchExecutionTest, CatchAllCatchesAnything) {
    const std::string source = R"(
        fn main() {
            try {
                throw 42;
            } catch (Exception e) {
                println("caught " + e);
            }
            try {
                throw "boom";
            } catch (Exception e) {
                println("caught " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_catch_all");
    EXPECT_NE(output.find("caught 42"), std::string::npos);
    EXPECT_NE(output.find("caught boom"), std::string::npos);
}

TEST(TryCatchExecutionTest, TypedCatchFallsThroughToOuter) {
    // No clause matches in the inner try: the throw propagates to the
    // outer catch (which matches), it does not vanish.
    const std::string source = R"(
        fn main() {
            try {
                try {
                    throw 42;
                } catch (string e) {
                    println("inner str");
                }
                println("inner after");
            } catch (int e) {
                println("outer int " + e);
            }
            println("done");
        }
    )";
    std::string output = runSPProgram(source, "exec_typed_fallthrough");
    EXPECT_EQ(output.find("inner str"), std::string::npos);
    EXPECT_EQ(output.find("inner after"), std::string::npos);
    EXPECT_NE(output.find("outer int 42"), std::string::npos);
    EXPECT_NE(output.find("done"), std::string::npos);
}

TEST(TryCatchExecutionTest, ArrayThrowRoundTrip) {
    const std::string source = R"(
        fn main() {
            var arr = [1, 2, 3];
            try {
                throw arr;
            } catch (int[] e) {
                println("len " + e.length());
                println("first " + e[0]);
                println("last " + e[2]);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_array");
    EXPECT_NE(output.find("len 3"), std::string::npos);
    EXPECT_NE(output.find("first 1"), std::string::npos);
    EXPECT_NE(output.find("last 3"), std::string::npos);
}

TEST(TryCatchExecutionTest, MapThrowRoundTrip) {
    const std::string source = R"(
        fn main() {
            var m = {"a": 1, "b": 2};
            try {
                throw m;
            } catch (Map<string, int> e) {
                println("a " + e["a"]);
                println("size " + e.size());
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_map");
    EXPECT_NE(output.find("a 1"), std::string::npos);
    EXPECT_NE(output.find("size 2"), std::string::npos);
}

TEST(TryCatchExecutionTest, ClassThrowRoundTrip) {
    const std::string source = R"(
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn main() {
            try {
                throw new Counter(5);
            } catch (Counter e) {
                println("n " + e.n);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_class");
    EXPECT_NE(output.find("n 5"), std::string::npos);
}

TEST(TryCatchExecutionTest, StructThrowRoundTrip) {
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
            } catch (Point e) {
                println("x " + e.x);
                println("y " + e.y);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_struct");
    EXPECT_NE(output.find("x 3"), std::string::npos);
    EXPECT_NE(output.find("y 4"), std::string::npos);
}

TEST(TryCatchExecutionTest, SubclassCatchMatches) {
    const std::string source = R"(
        class Animal {
            public string name() { return "animal"; }
        }
        class Dog extends Animal {
            public string name() { return "dog"; }
        }
        fn main() {
            try {
                throw new Dog();
            } catch (Animal e) {
                println("animal caught " + e.name());
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_subclass_catch");
    EXPECT_NE(output.find("animal caught dog"), std::string::npos);
}

TEST(TryCatchExecutionTest, BaseClassThrowSkipsDerivedClause) {
    // Matching is on the thrown expression's static type: a variable
    // statically typed Animal never matches `catch (Dog e)`, even
    // though the clause set is ordered.
    const std::string source = R"(
        class Animal {
            public string name() { return "animal"; }
        }
        class Dog extends Animal {
            public string name() { return "dog"; }
        }
        fn main() {
            try {
                throw new Animal();
            } catch (Dog e) {
                println("dog caught");
            } catch (Exception e) {
                println("fallback " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_base_skips_derived");
    EXPECT_EQ(output.find("dog caught"), std::string::npos);
    EXPECT_NE(output.find("fallback [Animal]"), std::string::npos);
}

TEST(TryCatchExecutionTest, FloatAndBoolThrowRoundTrip) {
    const std::string source = R"(
        fn main() {
            try {
                throw 3.5;
            } catch (double e) {
                println("dbl " + e);
            }
            try {
                throw true;
            } catch (bool e) {
                println("bool " + e);
            }
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_scalar");
    EXPECT_NE(output.find("dbl 3.5"), std::string::npos);
    EXPECT_NE(output.find("bool true"), std::string::npos);
}

TEST(TryCatchExecutionTest, UncaughtArrayThrowPanicsWithPlaceholder) {
    // A composite value has no string form: the panic message is the
    // "[<type>]" placeholder, never the raw pointer bytes.
    const std::string source = R"(
        fn main() {
            var arr = [1, 2, 3];
            throw arr;
        }
    )";
    std::string output = runSPProgram(source, "exec_uncaught_array");
    EXPECT_NE(output.find("SP Panic: [int[]]"), std::string::npos);
}

// --- throw-path refcount cleanup (Phase 5 slice B) ----------------------
// Measured with the runtime's live ARC-object counter
// (sp_rt_mem_live_objects): a refcounted local created inside a try
// must be released when a throw unwinds past it.

TEST(TryCatchExecutionTest, ThrowPathReleasesRefcountedLocals) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            try {
                var c = new Counter(7);
                println("inside " + sp_rt_mem_live_objects());
                throw "boom";
            } catch (Exception e) {
                println("caught " + e);
            }
            println("after " + sp_rt_mem_live_objects());
            var d = new Counter(9);
            println("after2 " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_drops");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 1"), std::string::npos);
    EXPECT_NE(output.find("after 0"), std::string::npos);
    EXPECT_NE(output.find("after2 1"), std::string::npos);
}

TEST(TryCatchExecutionTest, ThrowOfAliasedObjectKeepsItAlive) {
    // The thrown object is a local the code still uses afterwards:
    // the throw retains it into the payload, so the unwinding drop
    // of the local cannot free it out from under the catch or the
    // code after the try.
    const std::string source = R"(
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn main() {
            var o = new Counter(7);
            try {
                throw o;
            } catch (Counter e) {
                println("caught " + e.n);
            }
            println("alive " + o.n);
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_alias");
    EXPECT_NE(output.find("caught 7"), std::string::npos);
    EXPECT_NE(output.find("alive 7"), std::string::npos);
}

TEST(TryCatchExecutionTest, CaughtNewObjectIsReleasedAfterCatch) {
    // A directly-thrown `new` transfers its count-1 to the payload;
    // the catch variable takes that share and releases it when the
    // catch body ends -- no leak, no double free.
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            try {
                throw new Counter(5);
            } catch (Counter e) {
                println("n " + e.n);
                println("in catch " + sp_rt_mem_live_objects());
            }
            println("after " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_new_release");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("n 5"), std::string::npos);
    EXPECT_NE(output.find("in catch 1"), std::string::npos);
    EXPECT_NE(output.find("after 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, UncaughtClassThrowInResultFunctionReleases) {
    // Uncaught class throw in a Result function becomes Err -- and
    // the owned object is released by the uncaught handler, not leaked.
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn fail() -> Result<int, string> {
            throw new Counter(1);
        }
        fn main() {
            var r = fail();
            println("err " + r.isErr());
            println("live " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_uncaught_class_err");
    EXPECT_NE(output.find("err true"), std::string::npos);
    EXPECT_NE(output.find("live 0"), std::string::npos);
}

// --- +1 return convention (Phase 6 slice C, fix 5) ----------------------
// A factory call returns a fresh +1 share: the receiver adopts it
// without retaining again. Before this, the callee retained on return
// AND the receiver retained on receipt, so `var x = makeFoo()` and
// `throw makeFoo()` each leaked one share per call.
TEST(TryCatchExecutionTest, FactoryCallThrowDoesNotLeak) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn makeCounter() -> Counter { return new Counter(5); }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            try {
                throw makeCounter();
            } catch (Counter e) {
                println("n " + e.n);
            }
            println("after throw " + sp_rt_mem_live_objects());
            {
                var x = makeCounter();
                println("inside " + sp_rt_mem_live_objects());
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_throw_factory");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("n 5"), std::string::npos);
    EXPECT_NE(output.find("after throw 0"), std::string::npos);
    EXPECT_NE(output.find("inside 1"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, BorrowedReturnSurvivesAndFactoryAssignFreesOld) {
    // The other side of the +1 convention: a function returning a
    // borrowed value (a field) must retain it on the way out, so the
    // caller receives a +1 share — no premature free. And re-assigning
    // a variable from a factory call releases the old value.
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Box {
            public int n;
        }
        class Holder {
            public Box box;
            fn init() { box = new Box(); box.n = 7; }
            fn getBox() -> Box { return box; }
            fn getBoxShort() -> Box => box;
        }
        fn makeBox() -> Box { let b = new Box(); b.n = 5; return b; }
        fn main() {
            let h = new Holder();
            h.init();
            println("borrowed " + h.getBox().n);
            println("borrowed short " + h.getBoxShort().n);
            println("factory " + makeBox().n);
            var b = makeBox();
            b = makeBox();
            println("reassigned " + b.n);
            println("live " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_return_convention");
    EXPECT_NE(output.find("borrowed 7"), std::string::npos);
    EXPECT_NE(output.find("borrowed short 7"), std::string::npos);
    EXPECT_NE(output.find("factory 5"), std::string::npos);
    EXPECT_NE(output.find("reassigned 5"), std::string::npos);
    // live: h + h.box + b. Phase 7 slice C drops the `makeBox().n`
    // temporary in the println (it used to leak a share, making this
    // 4). The two `getBox().n` temporaries are borrowed returns —
    // never recorded, never released — so their object survives via
    // the field anyway, and the object count does not show them; the
    // first makeBox was freed on reassign.
    EXPECT_NE(output.find("live 3"), std::string::npos);
}

// Phase 7 slice C: call-result temporaries are released at the end
// of the statement that consumes them. A stored result stays live
// until its scope ends, and a returned factory result stays live for
// the caller.
TEST(TryCatchExecutionTest, CallResultTemporariesAreReleased) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn takeFoo(f: Foo) -> int { return f.n; }
        fn getFoo() -> Foo { return makeFoo(); }
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 5; return b; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            println("take " + takeFoo(makeFoo()));
            println("after arg " + sp_rt_mem_live_objects());
            println("member " + makeBox().n);
            println("after member " + sp_rt_mem_live_objects());
            {
                var x = makeFoo();
                println("inside " + sp_rt_mem_live_objects());
                println("n " + x.n);
            }
            println("after block " + sp_rt_mem_live_objects());
            {
                var y = getFoo();
                println("ret inside " + sp_rt_mem_live_objects());
                println("ret n " + y.n);
            }
            println("ret after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_released");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("take 42"), std::string::npos);
    EXPECT_NE(output.find("after arg 0"), std::string::npos);
    EXPECT_NE(output.find("member 5"), std::string::npos);
    EXPECT_NE(output.find("after member 0"), std::string::npos);
    EXPECT_NE(output.find("inside 1"), std::string::npos);
    EXPECT_NE(output.find("n 42"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
    EXPECT_NE(output.find("ret inside 1"), std::string::npos);
    EXPECT_NE(output.find("ret n 42"), std::string::npos);
    EXPECT_NE(output.find("ret after block 0"), std::string::npos);
}

// Phase 7 slice C: the critical regression proof — a borrowed getter
// result (a field, short-form or long-form) used after the call must
// keep working. Borrowed values are never recorded as temporaries,
// so they are never released by this pass.
TEST(TryCatchExecutionTest, BorrowedGetterTempDoesNotFreeEarly) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Box { public int n; }
        class Holder {
            public Box box;
            fn init() { box = new Box(); box.n = 7; }
            fn getBox() -> Box { return box; }
            fn getBoxShort() -> Box => box;
        }
        fn main() {
            let h = new Holder();
            h.init();
            println("base " + sp_rt_mem_live_objects());
            println("borrowed " + h.getBox().n);
            println("borrowed short " + h.getBoxShort().n);
            println("direct " + h.getBox().n);
            println("direct short " + h.getBoxShort().n);
            println("after " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_borrowed");
    EXPECT_NE(output.find("base 2"), std::string::npos);
    EXPECT_NE(output.find("borrowed 7"), std::string::npos);
    EXPECT_NE(output.find("borrowed short 7"), std::string::npos);
    EXPECT_NE(output.find("direct 7"), std::string::npos);
    EXPECT_NE(output.find("direct short 7"), std::string::npos);
    EXPECT_NE(output.find("after 2"), std::string::npos);
}

// Phase 8 slice A: a chain of temporaries now drops the outer objects
// AND their fields — the generated <Class>_destructor releases each
// owned reference field when the parent dies (Phase 7-C left the inner
// objects alive because no destructor existed: after chain 1 /
// after arg 2). Renamed from TempChainDropsOuterKeepsFieldShare.
TEST(TryCatchExecutionTest, DestructorReleasesFieldShare) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Inner { public int n; }
        class Outer { public Inner child; }
        fn makeInner() -> Inner { let i = new Inner(); i.n = 9; return i; }
        fn makeOuter() -> Outer { let o = new Outer(); o.child = makeInner(); return o; }
        fn takeInner(i: Inner) -> int { return i.n; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            println("chain " + makeOuter().child.n);
            println("after chain " + sp_rt_mem_live_objects());
            println("arg " + takeInner(makeOuter().child));
            println("after arg " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_chain");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("chain 9"), std::string::npos);
    EXPECT_NE(output.find("after chain 0"), std::string::npos);
    EXPECT_NE(output.find("arg 9"), std::string::npos);
    EXPECT_NE(output.find("after arg 0"), std::string::npos);
}

// Phase 8 slice A: a three-level nesting (outer -> inner -> leaf) is
// fully freed when the outermost object dies — destructors cascade.
TEST(TryCatchExecutionTest, DestructorReleasesNestedFields) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Leaf { public int n; }
        class Inner { public Leaf leaf; }
        class Outer { public Inner inner; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                var o = new Outer();
                o.inner = new Inner();
                o.inner.leaf = new Leaf();
                o.inner.leaf.n = 5;
                println("inside " + sp_rt_mem_live_objects());
                println("leaf " + o.inner.leaf.n);
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_dtor_nested");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 3"), std::string::npos);
    EXPECT_NE(output.find("leaf 5"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

// Phase 8 slice A: a parent holding a strong child frees the child
// when the parent dies at block end (Phase 7-C / slice C left this at
// `after block 1` — the parent's death never released its field).
TEST(TryCatchExecutionTest, DestructorReleasesParentChild) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Child { public int n; }
        class Parent { public Child child; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                var p = new Parent();
                p.child = new Child();
                p.child.n = 7;
                println("inside " + sp_rt_mem_live_objects());
                println("n " + p.child.n);
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_dtor_parent_child");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 2"), std::string::npos);
    EXPECT_NE(output.find("n 7"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

// Phase 8 slice A: reassigning a field releases the old child (that
// already worked), and the new child is freed when the holder dies.
TEST(TryCatchExecutionTest, DestructorReleasesReassignedField) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Box { public int n; }
        class Holder { public Box box; }
        fn makeBox(n: int) -> Box { let b = new Box(); b.n = n; return b; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                var h = new Holder();
                h.box = makeBox(1);
                println("first " + sp_rt_mem_live_objects());
                h.box = makeBox(2);
                println("after reassign " + sp_rt_mem_live_objects());
                println("n " + h.box.n);
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_dtor_reassign");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("first 2"), std::string::npos);
    EXPECT_NE(output.find("after reassign 2"), std::string::npos);
    EXPECT_NE(output.find("n 2"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

// Phase 8 slice A: a void function that falls through without an
// explicit return must still drop its locals — the implicit-return
// path used to add the ReturnInst before popScope(), so the drops
// were silently skipped (terminated block).
TEST(TryCatchExecutionTest, ImplicitReturnDropsLocals) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn helper() {
            var x = makeFoo();
            println("helper inside " + sp_rt_mem_live_objects());
        }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            helper();
            println("after helper " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_dtor_fallthrough");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("helper inside 1"), std::string::npos);
    EXPECT_NE(output.find("after helper 0"), std::string::npos);
}

// Phase 8 slice A: the same implicit-return drop must work when the
// function body ends after a try/catch/finally — the finally runs on
// the normal path, then the fall-through drops the locals.
TEST(TryCatchExecutionTest, FallThroughTryFinallyDropsLocals) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn helper() {
            try {
                var x = makeFoo();
                println("try inside " + sp_rt_mem_live_objects());
            } catch (Exception e) {
                println("caught " + e);
            } finally {
                println("finally ran");
            }
        }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            helper();
            println("after helper " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_dtor_try_finally");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("try inside 1"), std::string::npos);
    EXPECT_NE(output.find("finally ran"), std::string::npos);
    EXPECT_NE(output.find("after helper 0"), std::string::npos);
}

// Phase 7 slice C: a self-referential temporary (a field pointing at
// its own object) must not crash when the temporary is dropped — the
// release frees the allocation, the cycle's field share leaks, and
// the object stays readable through the taken value.
TEST(TryCatchExecutionTest, SelfReferentialTempDoesNotCrash) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Node { public Node self; public int n; }
        fn makeNode() -> Node { let nd = new Node(); nd.n = 3; nd.self = nd; return nd; }
        fn takeNode(nd: Node) -> int { return nd.n; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                var x = makeNode().self;
                println("self n " + x.n);
                println("inside " + sp_rt_mem_live_objects());
            }
            println("after block " + sp_rt_mem_live_objects());
            println("take " + takeNode(makeNode().self));
            println("after take " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_self");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("self n 3"), std::string::npos);
    EXPECT_NE(output.find("inside 1"), std::string::npos);
    EXPECT_NE(output.find("after block 1"), std::string::npos);
    EXPECT_NE(output.find("take 3"), std::string::npos);
    EXPECT_NE(output.find("after take 2"), std::string::npos);
}

// Phase 7 slice C: a factory call used as a field initializer used
// to give the field TWO shares (the +1 plus the retain in
// emitFieldInitializers). The initializer's +1 is now released at
// the constructor's temp-scope pop, so the field holds exactly one.
TEST(TryCatchExecutionTest, FieldInitFactoryDoesNotDoubleCount) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
            fn sp_rt_strong_count(p: ptr) -> int;
        }
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 5; return b; }
        class Holder { public Box box = makeBox(); }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let h = new Holder();
            println("after new " + sp_rt_mem_live_objects());
            println("box strong " + sp_rt_strong_count(h.box));
            println("n " + h.box.n);
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_fieldinit");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("after new 2"), std::string::npos);
    EXPECT_NE(output.find("box strong 1"), std::string::npos);
    EXPECT_NE(output.find("n 5"), std::string::npos);
}

// Phase 7 slice C: containers store elements as raw i64 bits, so a
// reference-typed element is retained into the container —
// including when the element arrives through a function that pushes
// its borrowed parameter (the caller's temp is released at the
// caller's pop, so the container is the only owner). Counts match
// the baseline exactly; the reads prove no use-after-free.
TEST(TryCatchExecutionTest, ContainerStoresRetainTempElements) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn store(xs: List<Foo>, f: Foo) { xs.push(f); }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let xs = new List<Foo>();
            xs.push(makeFoo());
            println("after push live " + sp_rt_mem_live_objects());
            println("push n " + xs[0].n);
            store(xs, makeFoo());
            println("after store live " + sp_rt_mem_live_objects());
            println("store n " + xs[1].n);
            let ys = [makeFoo()];
            println("after array live " + sp_rt_mem_live_objects());
            println("array n " + ys[0].n);
            let m = new Map<string, Foo>();
            m.set("a", makeFoo());
            println("after mapset live " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_container");
    // Phase 37 slice B: the counts grew by one live object per
    // container — List/Map/Array are ARC objects now, so they count
    // in sp_rt_mem_live_objects() alongside the Foos they hold
    // (push: list + Foo = 2; store: + Foo = 3; array: + array + Foo
    // = 5; mapset: + map + Foo = 7). The reads still prove the
    // retained temporaries stay alive.
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("after push live 2"), std::string::npos);
    EXPECT_NE(output.find("push n 42"), std::string::npos);
    EXPECT_NE(output.find("after store live 3"), std::string::npos);
    EXPECT_NE(output.find("store n 42"), std::string::npos);
    EXPECT_NE(output.find("after array live 5"), std::string::npos);
    EXPECT_NE(output.find("array n 42"), std::string::npos);
    EXPECT_NE(output.find("after mapset live 7"), std::string::npos);
}

// Phase 7 slice C: a channel stores the sent value as raw i64 bits
// too — the same retain keeps a sent temporary alive past the
// sender's scope pop.
TEST(TryCatchExecutionTest, ChannelSendRetainsTempElement) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let ch = new Channel<Foo>(4);
            ch.send(makeFoo());
            println("after send live " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_channel");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("after send live 1"), std::string::npos);
}

// Phase 8 slice D: temporaries produced inside short-circuit
// branches (&& / || RHS, ternary arms) are now released by the
// branch block itself (per-branch pops — each branch block pops
// its own branch scope before its branch to the merge). This
// replaces the Phase 7-C pinned leak (after and 1 / after or 2 /
// after tern 3); the old test name is retired deliberately, not
// deleted — the coverage stays, the expectation is the fix.
TEST(TryCatchExecutionTest, ShortCircuitTempsAreReleased) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 5; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            if (true && makeFoo().n == 5) { println("hit"); }
            println("after and " + sp_rt_mem_live_objects());
            if (false || makeFoo().n == 5) { println("or true"); }
            println("after or " + sp_rt_mem_live_objects());
            let t = true ? makeFoo().n : 0;
            println("tern " + t);
            println("after tern " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_shortcircuit");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("hit"), std::string::npos);
    EXPECT_NE(output.find("after and 0"), std::string::npos);
    EXPECT_NE(output.find("or true"), std::string::npos);
    EXPECT_NE(output.find("after or 0"), std::string::npos);
    EXPECT_NE(output.find("tern 5"), std::string::npos);
    EXPECT_NE(output.find("after tern 0"), std::string::npos);
}

// Phase 7 slice C: a temporary consumed by an earlier statement in
// a try block is dropped before a later same-function throw — the
// throw path itself does not leak it.
TEST(TryCatchExecutionTest, ThrowAfterTempDoesNotLeak) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn takeFoo(f: Foo) -> int { return f.n; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            try {
                println("take " + takeFoo(makeFoo()));
                throw "bang";
            } catch (string e) {
                println("caught " + e);
            }
            println("after " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_throwpath");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("take 42"), std::string::npos);
    EXPECT_NE(output.find("caught bang"), std::string::npos);
    EXPECT_NE(output.find("after 0"), std::string::npos);
}

// Phase 8 slice D: temporaries inside an async fn are now
// recorded and released — the state machine spills them across
// suspend points (see docs/v2/PHASE-37-SLICE-D.md §0.3) and the
// statement's scope pop releases them in the resume segment.
// This replaces the Phase 7-C pinned leak (after await 1); the old
// test name is retired deliberately, the coverage stays.
TEST(TryCatchExecutionTest, AsyncTempIsReleased) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        import std.async;
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn takeFoo(f: Foo) -> int { return f.n; }
        async fn useTemp() -> int {
            let n = takeFoo(makeFoo());
            return n;
        }
        async fn run() {
            let v = await useTemp();
            println("v " + v);
            println("after await " + sp_rt_mem_live_objects());
        }
        fn main() {
            run();
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_async");
    EXPECT_NE(output.find("v 42"), std::string::npos);
    EXPECT_NE(output.find("after await 0"), std::string::npos);
}

// Phase 8 slice D: a temporary passed as an argument to an async
// call survives the await — the spawn stores the raw pointer with
// no retain, so the share is transferred to the task (adopted at
// the spawn) instead of being released. The body must still read
// the value (no use-after-free); the counts stay 1 / 2 because
// the adopted shares leak, as before.
TEST(TryCatchExecutionTest, AsyncArgTempSurvivesAwait) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        import std.async;
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 42; return b; }
        async fn useBox(b: Box) -> int { return b.n; }
        async fn run() {
            let v = await useBox(makeBox());
            println("inline " + v);
            println("after inline " + sp_rt_mem_live_objects());
            let t = useBox(makeBox());
            let w = await t;
            println("spawned " + w);
            println("after spawned " + sp_rt_mem_live_objects());
        }
        fn main() {
            run();
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_async_arg");
    EXPECT_NE(output.find("inline 42"), std::string::npos);
    EXPECT_NE(output.find("after inline 1"), std::string::npos);
    EXPECT_NE(output.find("spawned 42"), std::string::npos);
    EXPECT_NE(output.find("after spawned 2"), std::string::npos);
}

// Phase 8 slice D: a temporary produced while evaluating an await's
// operand — here captured transitively (takeTask forwards its
// parameter to an async spawn) — survives the suspend via the
// frame spill and is released by the statement's scope pop after
// the resume. This is the case that rules out a per-await release
// (releasing it at the suspend would free the object while the
// spawned body still needs it).
TEST(TryCatchExecutionTest, AsyncOperandTempIsReleasedAfterResume) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        import std.async;
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 42; return b; }
        async fn useBox(b: Box) -> int { return b.n; }
        fn takeTask(b: Box) -> Task<int> { return useBox(b); }
        async fn run() {
            let v = await takeTask(makeBox());
            println("v " + v);
            println("after await " + sp_rt_mem_live_objects());
        }
        fn main() {
            run();
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_async_operand");
    EXPECT_NE(output.find("v 42"), std::string::npos);
    EXPECT_NE(output.find("after await 0"), std::string::npos);
}

// Phase 8 slice D: a temporary in an async fn's return-value
// expression is released (the async ReturnStmt branch now has the
// same push/adopt/pop as the sync branch).
TEST(TryCatchExecutionTest, AsyncReturnTempIsReleased) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        import std.async;
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 42; return b; }
        fn takeBox(b: Box) -> int { return b.n; }
        async fn useTemp() -> int {
            return takeBox(makeBox());
        }
        async fn run() {
            let v = await useTemp();
            println("v " + v);
            println("after await " + sp_rt_mem_live_objects());
        }
        fn main() {
            run();
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_async_return");
    EXPECT_NE(output.find("v 42"), std::string::npos);
    EXPECT_NE(output.find("after await 0"), std::string::npos);
}

// Phase 8 slice D: the same adopt in a sync caller — before this
// slice the temporary was released at the spawning statement's
// pop, before the spawned body ran (baseline "body live 0": the
// object already freed when the body ran, "body n 42" only by luck
// of unreused heap memory). Now the body sees a live object.
TEST(TryCatchExecutionTest, AsyncArgTempSurvivesInSyncCaller) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        import std.async;
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 42; return b; }
        async fn useBox(b: Box) -> int {
            println("body live " + sp_rt_mem_live_objects());
            println("body n " + b.n);
            return b.n;
        }
        fn main() {
            let t = useBox(makeBox());
            println("after spawn " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_async_arg_sync");
    EXPECT_NE(output.find("after spawn 1"), std::string::npos);
    EXPECT_NE(output.find("body live 1"), std::string::npos);
    EXPECT_NE(output.find("body n 42"), std::string::npos);
}

// Phase 8 slice D: nested short-circuit branches — each branch
// block pops its own branch scope, so nested RHS temporaries are
// released too.
TEST(TryCatchExecutionTest, ShortCircuitNestedTempsAreReleased) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 5; return b; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            if (true && (false || makeBox().n == 5)) { println("nested-or"); }
            println("after nested or " + sp_rt_mem_live_objects());
            if (false || (true && makeBox().n == 5)) { println("nested-and"); }
            println("after nested and " + sp_rt_mem_live_objects());
            let x = true && makeBox().n == 5;
            println("x " + x);
            println("after x " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_shortcircuit_nested");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("nested-or"), std::string::npos);
    EXPECT_NE(output.find("after nested or 0"), std::string::npos);
    EXPECT_NE(output.find("nested-and"), std::string::npos);
    EXPECT_NE(output.find("after nested and 0"), std::string::npos);
    EXPECT_NE(output.find("x true"), std::string::npos);
    EXPECT_NE(output.find("after x 0"), std::string::npos);
}

// Phase 8 slice D: a temporary consumed inside a ?? RHS is
// released by the RHS branch block itself.
TEST(TryCatchExecutionTest, CoalesceRhsTempIsReleased) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 5; return b; }
        fn maybeNull() -> int? { return null; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let n = maybeNull() ?? makeBox().n;
            println("n " + n);
            println("after " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_coalesce_rhs");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("n 5"), std::string::npos);
    EXPECT_NE(output.find("after 0"), std::string::npos);
}

// Phase 8 slice D: a ternary arm whose RESULT is itself a
// temporary is adopted at the arm exit (the value is stored into
// the result slot and read after the merge), so it still leaks —
// the documented boundary of the per-branch release. The value
// itself must survive (no use-after-free).
TEST(TryCatchExecutionTest, TernaryArmResultTempStillLeaks) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Box { public int n; }
        fn makeBox() -> Box { let b = new Box(); b.n = 5; return b; }
        fn makeOther() -> Box { let b = new Box(); b.n = 9; return b; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let b = true ? makeBox() : makeOther();
            println("b.n " + b.n);
            println("after " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_temp_tern_arm_result");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("b.n 5"), std::string::npos);
    EXPECT_NE(output.find("after 1"), std::string::npos);
}

TEST(TryCatchExecutionTest, FinallyPropagationReleasesOuterLocal) {
    // A throw inside an inner try with only a finally propagates
    // through the finally to the outer catch: the outer try's
    // refcounted local must be released on that path too.
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Counter {
            public int n;
            public Counter(int v) { this.n = v; }
        }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            try {
                var c = new Counter(7);
                println("inside " + sp_rt_mem_live_objects());
                try {
                    throw "deep";
                } finally {
                    println("inner fin");
                }
            } catch (Exception e) {
                println("outer caught " + e);
            }
            println("after " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_finally_propagate_drops");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 1"), std::string::npos);
    EXPECT_NE(output.find("inner fin"), std::string::npos);
    EXPECT_NE(output.find("outer caught deep"), std::string::npos);
    EXPECT_NE(output.find("after 0"), std::string::npos);
}

// --- map literals ----------------------------------------------------------

TEST(TryCatchExecutionTest, MapLiteralReadsBack) {
    const std::string source = R"(
        fn main() {
            var m = {"a": 1, "b": 2};
            println(m["a"]);
            println(m["b"]);
            println(m.size());
            println(m.contains("a"));
        }
    )";
    std::string output = runSPProgram(source, "exec_map_literal");
    EXPECT_NE(output.find("1"), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("true"), std::string::npos);
}

TEST(TryCatchExecutionTest, MapLiteralIntKeysReadBack) {
    const std::string source = R"(
        fn main() {
            var m = {1: 10, 2: 20};
            println(m[1]);
            println(m[2]);
            println(m.size());
        }
    )";
    std::string output = runSPProgram(source, "exec_map_int_keys");
    EXPECT_NE(output.find("10"), std::string::npos);
    EXPECT_NE(output.find("20"), std::string::npos);
}

TEST(TryCatchExecutionTest, EmptyMapLiteralWorks) {
    const std::string source = R"(
        fn main() {
            var m = {};
            println(m.isEmpty());
            println(m.size());
        }
    )";
    std::string output = runSPProgram(source, "exec_map_empty");
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("0"), std::string::npos);
}

// Phase 37 slice B: containers are ARC objects now, and a container's
// destructor releases its retained reference-typed elements. Each
// test below is the execution-test form of a proof in
// ~/workspace/p8b-proofs/ (same program shape, same expected counts).
// Live-object counts include the container itself (it is ARC now),
// so "inside 2" is the container + the one Foo it holds.

TEST(TryCatchExecutionTest, ContainerListDropReleasesElements) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                let xs = new List<Foo>();
                xs.push(makeFoo());
                println("inside " + sp_rt_mem_live_objects());
                println("n " + xs[0].n);
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_list_drop");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 2"), std::string::npos);
    EXPECT_NE(output.find("n 42"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerArrayAndMapDropReleaseElements) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                let arr = [makeFoo()];
                println("arr inside " + sp_rt_mem_live_objects());
                println("arr n " + arr[0].n);
            }
            println("arr after block " + sp_rt_mem_live_objects());
            {
                let m = new Map<string, Foo>();
                m.set("a", makeFoo());
                println("map inside " + sp_rt_mem_live_objects());
                println("map n " + m["a"].n);
            }
            println("map after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_array_map_drop");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("arr inside 2"), std::string::npos);
    EXPECT_NE(output.find("arr n 42"), std::string::npos);
    EXPECT_NE(output.find("arr after block 0"), std::string::npos);
    EXPECT_NE(output.find("map inside 2"), std::string::npos);
    EXPECT_NE(output.find("map n 42"), std::string::npos);
    EXPECT_NE(output.find("map after block 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerSetDropReleasesElements) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
            fn sp_rt_strong_count(p: ptr) -> int;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                let s = new Set<Foo>();
                let f = makeFoo();
                s.add(f);
                println("inside " + sp_rt_mem_live_objects());
                println("contains " + s.contains(f));
                println("f strong " + sp_rt_strong_count(f));
                for (x in s) {
                    println("forin n " + x.n);
                }
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_set_drop");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 2"), std::string::npos);
    EXPECT_NE(output.find("contains true"), std::string::npos);
    EXPECT_NE(output.find("f strong 2"), std::string::npos);
    EXPECT_NE(output.find("forin n 42"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerQueueStackPQDropReleasesElements) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                let q = new Queue<Foo>();
                q.enqueue(makeFoo());
                println("q inside " + sp_rt_mem_live_objects());
            }
            println("q after block " + sp_rt_mem_live_objects());
            {
                let st = new Stack<Foo>();
                st.push(makeFoo());
                println("st inside " + sp_rt_mem_live_objects());
            }
            println("st after block " + sp_rt_mem_live_objects());
            {
                let pq = new PriorityQueue<Foo>();
                pq.push(makeFoo(), 1);
                println("pq inside " + sp_rt_mem_live_objects());
            }
            println("pq after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_queue_stack_pq_drop");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("q inside 2"), std::string::npos);
    EXPECT_NE(output.find("q after block 0"), std::string::npos);
    EXPECT_NE(output.find("st inside 2"), std::string::npos);
    EXPECT_NE(output.find("st after block 0"), std::string::npos);
    EXPECT_NE(output.find("pq inside 2"), std::string::npos);
    EXPECT_NE(output.find("pq after block 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerNestedListReleasesRecursively) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
            fn sp_rt_strong_count(p: ptr) -> int;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                let outer = new List<List<Foo>>();
                let inner = new List<Foo>();
                inner.push(makeFoo());
                outer.push(inner);
                println("inside " + sp_rt_mem_live_objects());
                println("outer n " + outer[0][0].n);
                let outerArr = new List<Foo[]>();
                let innerArr = [makeFoo()];
                outerArr.push(innerArr);
                println("arr inside " + sp_rt_mem_live_objects());
                println("inner strong " + sp_rt_strong_count(innerArr));
                println("arr n " + outerArr[0][0].n);
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_nested");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 3"), std::string::npos);
    EXPECT_NE(output.find("outer n 42"), std::string::npos);
    EXPECT_NE(output.find("arr inside 6"), std::string::npos);
    EXPECT_NE(output.find("inner strong 2"), std::string::npos);
    EXPECT_NE(output.find("arr n 42"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerBorrowedPassedToFunctionIsNotFreed) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
            fn sp_rt_strong_count(p: ptr) -> int;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn useList(xs: List<Foo>) {
            println("held " + sp_rt_mem_live_objects());
            println("in fn " + xs[0].n);
        }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let f = makeFoo();
            println("f strong " + sp_rt_strong_count(f));
            let xs = new List<Foo>();
            xs.push(f);
            println("f strong after push " + sp_rt_strong_count(f));
            useList(xs);
            println("f strong after call " + sp_rt_strong_count(f));
            println("after call " + sp_rt_mem_live_objects());
            println("n " + f.n);
        }
    )";
    std::string output = runSPProgram(source, "exec_container_borrowed");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("f strong 1"), std::string::npos);
    EXPECT_NE(output.find("f strong after push 2"), std::string::npos);
    EXPECT_NE(output.find("held 2"), std::string::npos);
    EXPECT_NE(output.find("in fn 42"), std::string::npos);
    EXPECT_NE(output.find("f strong after call 2"), std::string::npos);
    EXPECT_NE(output.find("after call 2"), std::string::npos);
    EXPECT_NE(output.find("n 42"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerReturnedFromFunctionSurvives) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn makeList() -> List<Foo> {
            let xs = new List<Foo>();
            xs.push(makeFoo());
            return xs;
        }
        fn makeMap() -> Map<string, Foo> {
            let m = new Map<string, Foo>();
            m.set("a", makeFoo());
            return m;
        }
        fn makeArray() -> Foo[] {
            let ys = [makeFoo()];
            return ys;
        }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let ys = makeList();
            println("list n " + ys[0].n);
            println("after list " + sp_rt_mem_live_objects());
            let m = makeMap();
            println("map n " + m["a"].n);
            println("after map " + sp_rt_mem_live_objects());
            let arr = makeArray();
            println("arr n " + arr[0].n);
            println("after arr " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_returned");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("list n 42"), std::string::npos);
    EXPECT_NE(output.find("after list 2"), std::string::npos);
    EXPECT_NE(output.find("map n 42"), std::string::npos);
    EXPECT_NE(output.find("after map 4"), std::string::npos);
    EXPECT_NE(output.find("arr n 42"), std::string::npos);
    EXPECT_NE(output.find("after arr 6"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerRemoveAndClearReleaseElements) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_strong_count(p: ptr) -> int;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            let f1 = makeFoo();
            let f2 = makeFoo();
            let xs = new List<Foo>();
            xs.push(f1);
            xs.push(f2);
            println("f1 strong " + sp_rt_strong_count(f1));
            println("f2 strong " + sp_rt_strong_count(f2));
            xs.removeAt(0);
            println("f1 strong after removeAt " + sp_rt_strong_count(f1));
            println("f2 strong after removeAt " + sp_rt_strong_count(f2));
            println("n " + xs[0].n);
            xs.clear();
            println("f2 strong after clear " + sp_rt_strong_count(f2));
            println("size " + xs.size());
            let m = new Map<string, Foo>();
            m.set("a", f1);
            m.set("b", f2);
            println("f1 strong after mapset " + sp_rt_strong_count(f1));
            m.set("a", f2);
            println("f1 strong after overwrite " + sp_rt_strong_count(f1));
            println("f2 strong after overwrite " + sp_rt_strong_count(f2));
            m.remove("a");
            println("f2 strong after mapremove " + sp_rt_strong_count(f2));
        }
    )";
    std::string output = runSPProgram(source, "exec_container_remove_clear");
    EXPECT_NE(output.find("f1 strong 2"), std::string::npos);
    EXPECT_NE(output.find("f2 strong 2"), std::string::npos);
    EXPECT_NE(output.find("f1 strong after removeAt 1"), std::string::npos);
    EXPECT_NE(output.find("f2 strong after removeAt 2"), std::string::npos);
    EXPECT_NE(output.find("n 42"), std::string::npos);
    EXPECT_NE(output.find("f2 strong after clear 1"), std::string::npos);
    EXPECT_NE(output.find("size 0"), std::string::npos);
    EXPECT_NE(output.find("f1 strong after mapset 2"), std::string::npos);
    EXPECT_NE(output.find("f1 strong after overwrite 1"), std::string::npos);
    EXPECT_NE(output.find("f2 strong after overwrite 3"), std::string::npos);
    EXPECT_NE(output.find("f2 strong after mapremove 2"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerReassignReleasesOldContainer) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let xs = new List<Foo>();
            xs.push(makeFoo());
            xs.push(makeFoo());
            println("before reassign " + sp_rt_mem_live_objects());
            xs = new List<Foo>();
            println("after reassign " + sp_rt_mem_live_objects());
            println("size " + xs.size());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_reassign");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("before reassign 3"), std::string::npos);
    EXPECT_NE(output.find("after reassign 1"), std::string::npos);
    EXPECT_NE(output.find("size 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerChannelDestroyDrainsQueuedItems) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            let ch = new Channel<Foo>(4);
            ch.send(makeFoo());
            println("after send " + sp_rt_mem_live_objects());
            ch.destroy();
            println("after destroy " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_channel_destroy");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("after send 1"), std::string::npos);
    EXPECT_NE(output.find("after destroy 0"), std::string::npos);
}

// Parent fix (Phase 37 merge): dropping a List grown past 8192
// elements segfaulted — its items buffer is a 128 KiB mmap'd region
// and sp_rt_free probed the magic 24 bytes before its base, which
// are unmapped. get_arc_header now consults the live ARC payload
// registry instead of reading memory, so the drop is a safe miss
// and the buffer is freed with std::free.
TEST(TryCatchExecutionTest, ContainerLargeListDropDoesNotCrash) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
        }
        import std.collections;
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                var list = new List<int>();
                for (var i = 0; i < 10000; i = i + 1) { list.push(i); }
                println("inside " + sp_rt_mem_live_objects());
                println("size " + list.length());
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_large_list_drop");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("inside 1"), std::string::npos);
    EXPECT_NE(output.find("size 10000"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerPeekDoesNotStealShare) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
            fn sp_rt_strong_count(p: ptr) -> int;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn takeFoo(f: Foo) { println("take " + f.n); }
        fn main() {
            let f = makeFoo();
            let q = new Queue<Foo>();
            q.enqueue(f);
            println("strong after enqueue " + sp_rt_strong_count(f));
            takeFoo(q.peek());
            println("strong after peek temp " + sp_rt_strong_count(f));
            println("q n " + f.n);
            println("live " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_peek_retain");
    EXPECT_NE(output.find("strong after enqueue 2"), std::string::npos);
    EXPECT_NE(output.find("take 42"), std::string::npos);
    EXPECT_NE(output.find("strong after peek temp 2"), std::string::npos);
    EXPECT_NE(output.find("q n 42"), std::string::npos);
    EXPECT_NE(output.find("live 2"), std::string::npos);
}

TEST(TryCatchExecutionTest, ContainerReduceBorrowedResultIsRetainedOnAssign) {
    const std::string source = R"(
        extern "C" {
            fn sp_rt_mem_live_objects() -> int64;
            fn sp_rt_strong_count(p: ptr) -> int;
        }
        class Foo { public int n; }
        fn makeFoo() -> Foo { let f = new Foo(); f.n = 42; return f; }
        fn main() {
            println("before " + sp_rt_mem_live_objects());
            {
                let f = makeFoo();
                let xs = new List<Foo>();
                xs.push(f);
                println("strong after push " + sp_rt_strong_count(f));
                let r = xs.reduce((Foo a, Foo b) => a, f);
                println("strong after reduce assign " + sp_rt_strong_count(f));
                let r2: Foo = xs.reduce((Foo a, Foo b) => a, f);
                println("strong after typed reduce assign " + sp_rt_strong_count(f));
                println("f n " + f.n);
                println("r2 n " + r2.n);
                println("live " + sp_rt_mem_live_objects());
            }
            println("after block " + sp_rt_mem_live_objects());
        }
    )";
    std::string output = runSPProgram(source, "exec_container_reduce_borrowed");
    EXPECT_NE(output.find("before 0"), std::string::npos);
    EXPECT_NE(output.find("strong after push 2"), std::string::npos);
    EXPECT_NE(output.find("strong after reduce assign 2"), std::string::npos);
    EXPECT_NE(output.find("strong after typed reduce assign 3"), std::string::npos);
    EXPECT_NE(output.find("f n 42"), std::string::npos);
    EXPECT_NE(output.find("r2 n 42"), std::string::npos);
    EXPECT_NE(output.find("live 2"), std::string::npos);
    EXPECT_NE(output.find("after block 0"), std::string::npos);
}
