#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/sema/ModuleLoader.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace fs = std::filesystem;
using namespace sp;

namespace {

// Phase 6 slice B: execution tests for statics + init semantics.
// Mirrors ExecutionTest::runSPProgram, but the runner also takes argv
// (OS.args() test) and reports the child exit status (the int.parse
// panic test: the child aborts, so the exit status is the signal, not
// a code — the output assertion is the primary check there).
fs::path getStaticsOutputDir() {
#ifdef SP_PROJECT_ROOT
    fs::path p = fs::path(SP_PROJECT_ROOT) / "build";
    if (fs::exists(p)) return p;
#endif
    return fs::temp_directory_path();
}

struct RunResult {
    std::string output;
    // 0 on clean exit; WEXITSTATUS on a normal exit; 128+signal when
    // the child died from a signal (e.g. 134 for SIGABRT); -1 when the
    // program never ran (compile-stage sentinel like "SEMA_ERROR").
    int exitCode{-1};
};

RunResult runSPProgramArgs(const std::string& source, const std::string& testName,
                           const std::vector<std::string>& args = {}) {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, testName + ".sp", &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return {"LEXER_ERROR", -1};

    parser::Parser parser(tokens, testName + ".sp", &diags);
    auto program = parser.parseProgram();
    if (diags.hasErrors() || !program) return {"PARSER_ERROR", -1};

    sema::ModuleLoader moduleLoader(diags);
    if (!moduleLoader.loadAndMergeAll(*program, testName + ".sp")) return {"MODULE_ERROR", -1};

    sema::SemanticAnalyzer sema(diags);
    if (!sema.analyze(*program) || diags.hasErrors()) return {"SEMA_ERROR", -1};

    ir::IRBuilder builder(sema);
    auto module = builder.build(*program, testName);
    if (!module) return {"IR_ERROR", -1};

    fs::path exePath = getStaticsOutputDir() / (testName + ".exe");
    codegen::CodegenOptions opts;
    codegen::LLVMGenerator generator(opts);
    if (!generator.compileToExecutable(*module, exePath.string())) {
        return {"CODEGEN_ERROR: " + generator.getLastError(), -1};
    }

    std::string outPath = exePath.string() + ".out.txt";
    std::string cmd = "\"" + exePath.string() + "\"";
    for (const auto& a : args) cmd += " \"" + a + "\"";
    cmd += " > \"" + outPath + "\" 2>&1";
    int code = std::system(cmd.c_str());

    int exitCode = -1;
#if defined(_WIN32)
    exitCode = code;
#else
    if (WIFEXITED(code)) exitCode = WEXITSTATUS(code);
    else if (WIFSIGNALED(code)) exitCode = 128 + WTERMSIG(code);
#endif

    std::ifstream in(outPath);
    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();

    std::error_code ec;
    fs::remove(outPath, ec);
    fs::remove(exePath, ec);
    return {ss.str(), exitCode};
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

// Fix 1: a field initializer that allocates (`new`) runs before the
// constructor body, so the field is usable immediately.
TEST(StaticsExecutionTest, FieldInitializerWithNew) {
    const std::string source = R"(
        import std.io;
        import std.collections;

        class Holder {
            public List<int> list = new List<int>();
            public fn add(int v) -> void {
                list.add(v);
            }
        }

        fn main() {
            Holder h = new Holder();
            h.add(10);
            h.add(20);
            h.add(12);
            println("sum=${h.list.get(0) + h.list.get(1) + h.list.get(2)}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_field_init_new");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "sum=42")) << r.output;
}

// Fix 2: a static const read from outside the class returns its value
// (it used to segfault: the bare class name lowered to ConstantInt 0).
TEST(StaticsExecutionTest, StaticConstReadFromOutside) {
    const std::string source = R"(
        import std.io;

        class MathUtils {
            public static const double PI = 3.14159;
            public static string env = "Production";
        }

        fn main() {
            println("PI=${MathUtils.PI}");
            println("env=${MathUtils.env}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_const_outside");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "PI=3.14159")) << r.output;
    EXPECT_TRUE(contains(r.output, "env=Production")) << r.output;
}

// Fix 2: static mutable state persists across calls (assigned values
// used to read back as zero).
TEST(StaticsExecutionTest, StaticMutableStatePersists) {
    const std::string source = R"(
        import std.io;

        class MathUtils {
            public static int counter = 0;
            public static fn bump() -> int {
                MathUtils.counter = MathUtils.counter + 1;
                return MathUtils.counter;
            }
        }

        fn main() {
            println("bump1=${MathUtils.bump()}");
            println("bump2=${MathUtils.bump()}");
            println("counter=${MathUtils.counter}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_mutable");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "bump1=1")) << r.output;
    EXPECT_TRUE(contains(r.output, "bump2=2")) << r.output;
    EXPECT_TRUE(contains(r.output, "counter=2")) << r.output;
}

// Fix 2: a static field may be read and assigned by its bare name
// inside the class, and a static initializer may read another static
// field of the same class.
TEST(StaticsExecutionTest, BareStaticAccessInsideClass) {
    const std::string source = R"(
        import std.io;

        class Cfg {
            public static int a = 10;
            public static int b = a + 5;
            public static fn getB() -> int {
                return b;
            }
            public static fn incA() -> int {
                a = a + 1;
                return a;
            }
        }

        fn main() {
            println("b=${Cfg.b}");
            println("getB=${Cfg.getB()}");
            println("incA=${Cfg.incA()}");
            println("a=${Cfg.a}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_bare_inside");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "b=15")) << r.output;
    EXPECT_TRUE(contains(r.output, "getB=15")) << r.output;
    EXPECT_TRUE(contains(r.output, "incA=11")) << r.output;
    EXPECT_TRUE(contains(r.output, "a=11")) << r.output;
}

// Fix 2: a derived class sees its base class's statics under the
// derived name.
TEST(StaticsExecutionTest, InheritedStaticViaDerivedName) {
    const std::string source = R"(
        import std.io;

        class Base {
            public static int baseVal = 7;
            public static string tag = "base";
        }

        class Derived : Base {
            public static int ownVal = 3;
        }

        fn main() {
            println("derivedBase=${Derived.baseVal}");
            println("derivedTag=${Derived.tag}");
            println("own=${Derived.ownVal}");
            println("base=${Base.baseVal}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_inherited");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "derivedBase=7")) << r.output;
    EXPECT_TRUE(contains(r.output, "derivedTag=base")) << r.output;
    EXPECT_TRUE(contains(r.output, "own=3")) << r.output;
    EXPECT_TRUE(contains(r.output, "base=7")) << r.output;
}

// Fix 1 (generic classes): a non-generic field initializer runs in a
// generic class that uses the default constructor.
TEST(StaticsExecutionTest, GenericClassFieldInitializerDefaultCtor) {
    const std::string source = R"(
        import std.io;

        class Box<T> {
            public string label = "box";
            public T value;
        }

        fn main() {
            Box<int> b = new Box<int>();
            println("label=${b.label}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_generic_field_init");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "label=box")) << r.output;
}

// Fix 1 (generic classes, explicit ctor): field initializers also
// run when a generic class declares its own constructor (only
// non-type-param-typed fields — a `T`-typed initializer has no
// substitution and stays zeroed).
TEST(StaticsExecutionTest, GenericClassFieldInitializerExplicitCtor) {
    const std::string source = R"(
        import std.io;

        class Box<T> {
            public string label = "box";
            public T value;
            public Box() {
            }
        }

        fn main() {
            Box<int> b = new Box<int>();
            println("label=${b.label}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_generic_field_init_ctor");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "label=box")) << r.output;
}

// Fix 3: map[k] returns the plain value type (like HashMap.get()), so
// it can be used directly as an int; a missing key reads back the
// zero value (0), matching the runtime.
TEST(StaticsExecutionTest, MapIndexReturnsPlainValue) {
    const std::string source = R"(
        import std.io;
        import std.collections;

        fn main() {
            Map<string, int> m = new Map<string, int>();
            m["a"] = 42;
            int a = m["a"];
            int missing = m["nope"];
            println("a=${a}");
            println("missing=${missing}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_map_index");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "a=42")) << r.output;
    EXPECT_TRUE(contains(r.output, "missing=0")) << r.output;
}

// Fix 4: interpolating a Set prints {1, 2, 3} (set order is hash
// order, so the test checks membership, not order) instead of raw
// pointer bytes.
TEST(StaticsExecutionTest, SetInterpolationPrintsBraces) {
    const std::string source = R"(
        import std.io;
        import std.collections;

        fn main() {
            Set<int> s = new Set<int>();
            s.add(1);
            s.add(2);
            s.add(3);
            println("set=${s}");
            println("size=${s.size()}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_set_interp");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "set={")) << r.output;
    EXPECT_TRUE(contains(r.output, "1")) << r.output;
    EXPECT_TRUE(contains(r.output, "2")) << r.output;
    EXPECT_TRUE(contains(r.output, "3")) << r.output;
    EXPECT_TRUE(contains(r.output, "size=3")) << r.output;
}

// Fix 4: a Set is iterable with for-in.
TEST(StaticsExecutionTest, SetForInIteration) {
    const std::string source = R"(
        import std.io;
        import std.collections;

        fn main() {
            Set<int> s = new Set<int>();
            s.add(1);
            s.add(2);
            s.add(3);
            int sum = 0;
            for (int x in s) {
                sum = sum + x;
            }
            println("sum=${sum}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_set_forin");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "sum=6")) << r.output;
}

// Fix 5: valid input still parses.
TEST(StaticsExecutionTest, IntParseValid) {
    const std::string source = R"(
        import std.io;

        fn main() {
            int ok = int.parse("42");
            println("ok=${ok}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_parse_valid");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "ok=42")) << r.output;
}

// Fix 5: invalid input panics loudly naming the input instead of
// silently returning 0; the line after the parse never runs.
TEST(StaticsExecutionTest, IntParseInvalidPanics) {
    const std::string source = R"(
        import std.io;

        fn main() {
            int bad = int.parse("abc");
            println("after bad");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_parse_invalid");
    EXPECT_NE(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "SP Panic: int.parse: invalid integer 'abc'")) << r.output;
    EXPECT_FALSE(contains(r.output, "after bad")) << r.output;
}

// Fix 6: one-argument List.reduce works for both int and string
// (the accumulator is the element type).
TEST(StaticsExecutionTest, ReduceOneArgList) {
    const std::string source = R"(
        import std.io;
        import std.collections;

        fn main() {
            List<int> numbers = new List<int>();
            for (int i = 1; i <= 10; i++) { numbers.add(i); }
            int sum = numbers.reduce((int acc, int x) => acc + x);
            println("sum=${sum}");
            List<string> words = new List<string>();
            words.add("a");
            words.add("b");
            words.add("c");
            string joined = words.reduce((string acc, string x) => acc + x);
            println("joined=${joined}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_reduce_onearg");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "sum=55")) << r.output;
    EXPECT_TRUE(contains(r.output, "joined=abc")) << r.output;
}

// Fix 6 (regression guard): the two-argument typed reduce keeps
// working.
TEST(StaticsExecutionTest, ReduceTwoArgTyped) {
    const std::string source = R"(
        import std.io;
        import std.collections;

        fn main() {
            List<int> numbers = new List<int>();
            for (int i = 1; i <= 10; i++) { numbers.add(i); }
            int sum = numbers.reduce((int acc, int x) => acc + x, 0);
            println("sum=${sum}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_reduce_twoarg");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "sum=55")) << r.output;
}

// Fix 7: a method's default argument is applied at the call site —
// the default value (200) is observable in the result, and an
// unqualified call from inside the class also picks it up.
TEST(StaticsExecutionTest, MethodDefaultArgumentApplied) {
    const std::string source = R"(
        import std.io;

        class Responder {
            public fn text(string content, int status = 200) -> string {
                return "${content} ${status}";
            }
            public fn greet(string name) -> string {
                return text(name);
            }
        }

        fn main() {
            Responder r = new Responder();
            println(r.text("hello"));
            println(r.text("hello", 404));
            println(r.greet("world"));
        }
    )";
    auto r = runSPProgramArgs(source, "statics_method_default");
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "hello 200")) << r.output;
    EXPECT_TRUE(contains(r.output, "hello 404")) << r.output;
    EXPECT_TRUE(contains(r.output, "world 200")) << r.output;
}

// Fix 8: OS.args() returns the real argv (program name excluded).
TEST(StaticsExecutionTest, OsArgsReturnsArgv) {
    const std::string source = R"(
        import std.io;
        import std.os;

        fn main() {
            string[] args = OS.args();
            println("argc=${args.length}");
            println("arg0=${args[0]}");
            println("arg1=${args[1]}");
        }
    )";
    auto r = runSPProgramArgs(source, "statics_os_args", {"hello", "world"});
    ASSERT_EQ(r.exitCode, 0) << r.output;
    EXPECT_TRUE(contains(r.output, "argc=2")) << r.output;
    EXPECT_TRUE(contains(r.output, "arg0=hello")) << r.output;
    EXPECT_TRUE(contains(r.output, "arg1=world")) << r.output;
}
