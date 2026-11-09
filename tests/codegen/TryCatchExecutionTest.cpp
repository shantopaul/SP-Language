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
    // live: h + h.box + b + the `makeBox().n` temporary in the
    // println (a call-result temporary is never released — see the
    // follow-up note in the slice doc; the two `getBox().n`
    // temporaries also leak a share each, but their object survives
    // via the field anyway, so the object count does not show them;
    // the first makeBox was freed on reassign).
    EXPECT_NE(output.find("live 4"), std::string::npos);
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
