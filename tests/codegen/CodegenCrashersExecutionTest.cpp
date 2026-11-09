// CodegenCrashersExecutionTest.cpp — end-to-end tests for Phase 6
// slice A (pro-4.0.0): the seven codegen crashers the Phase 5 doc gate
// found, executed as real native programs via spc (Clang).
// IR-level coverage lives in tests/ir/CodegenCrashersIRTest.cpp.
//
// The helper below is a local copy of the runSPProgram pattern from
// ExecutionTest.cpp / TryCatchExecutionTest.cpp: parse -> sema -> IR ->
// LLVM -> link -> run the executable and return its combined
// stdout+stderr.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
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
    return fs::temp_directory_path() / "sp_codegen_crashers_exec_tests";
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
    int rc = std::system(cmd.c_str());
    (void)rc;
    std::ifstream outFile(outPath);
    std::stringstream ss;
    ss << outFile.rdbuf();
    return ss.str();
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

size_t countOccurrences(const std::string& haystack, const std::string& needle) {
    size_t count = 0;
    size_t pos = 0;
    while ((pos = haystack.find(needle, pos)) != std::string::npos) {
        ++count;
        pos += needle.size();
    }
    return count;
}

} // namespace

// --- 1. Map iteration ------------------------------------------------------

TEST(CodegenCrashersExecutionTest, MapIterationStringKeysTerminates) {
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
            println("DONE");
        }
    )";
    // Before the fix this loop never terminated and printed garbage
    // keys forever (sp_rt_array_length read the map's entry-table
    // pointer as a length).
    std::string out = runSPProgram(source, "MapIterStr");
    EXPECT_TRUE(contains(out, "count=3")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
    EXPECT_EQ(countOccurrences(out, "key="), 3u) << out;
    EXPECT_TRUE(contains(out, "key=Shanto")) << out;
    EXPECT_TRUE(contains(out, "key=Alice")) << out;
    EXPECT_TRUE(contains(out, "key=Bob")) << out;
}

TEST(CodegenCrashersExecutionTest, MapIterationIntKeysTerminates) {
    const std::string source = R"(
        import std.collections;
        fn main() {
            Map<int, int> m = new Map<int, int>();
            m[10] = 1;
            m[20] = 2;
            m[30] = 3;
            var count = 0;
            var sum = 0;
            for (k in m) {
                count = count + 1;
                sum = sum + k;
            }
            println("count=${count} sum=${sum}");
            println("DONE");
        }
    )";
    std::string out = runSPProgram(source, "MapIterInt");
    EXPECT_TRUE(contains(out, "count=3 sum=60")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

TEST(CodegenCrashersExecutionTest, MapIterationEmptyMap) {
    const std::string source = R"(
        import std.collections;
        fn main() {
            Map<string, int> ages = new Map<string, int>();
            var count = 0;
            for (key in ages) {
                count = count + 1;
            }
            println("count=${count}");
            println("DONE");
        }
    )";
    std::string out = runSPProgram(source, "MapIterEmpty");
    EXPECT_TRUE(contains(out, "count=0")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

// --- 2. Interface-typed method calls ---------------------------------------

TEST(CodegenCrashersExecutionTest, InterfaceDispatchDirectAndList) {
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
            fn draw() { println("Circle r=${radius}"); }
            fn getColor() -> string { return color; }
        }
        class Rectangle implements Drawable {
            double width;
            double height;
            string color;
            Rectangle(double w, double h, string color) {
                this.width = w;
                this.height = h;
                this.color = color;
            }
            fn draw() { println("Rect ${width}x${height}"); }
            fn getColor() -> string { return color; }
        }
        fn main() {
            Drawable d1 = new Circle(3.0, "red");
            d1.draw();
            println("color=${d1.getColor()}");
            List<Drawable> shapes = new List<Drawable>();
            shapes.add(new Circle(1.0, "blue"));
            shapes.add(new Rectangle(2.0, 3.0, "green"));
            for (shape in shapes) {
                shape.draw();
            }
            println("DONE");
        }
    )";
    // Before the fix IR verification failed with
    // "call to undeclared function 'draw'".
    std::string out = runSPProgram(source, "IfaceDispatch");
    EXPECT_TRUE(contains(out, "Circle r=3")) << out;
    EXPECT_TRUE(contains(out, "color=red")) << out;
    EXPECT_TRUE(contains(out, "Circle r=1")) << out;
    EXPECT_TRUE(contains(out, "Rect 2x3")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

TEST(CodegenCrashersExecutionTest, InterfaceDispatchSwappedDeclarationOrder) {
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
            println("DONE");
        }
    )";
    // Square declares getColor() BEFORE draw() — a per-class
    // declaration-order slot would dispatch Square's draw() to the
    // wrong function. The interface slot must be stable across classes.
    std::string out = runSPProgram(source, "IfaceSwapOrder");
    EXPECT_TRUE(contains(out, "Circle")) << out;
    EXPECT_TRUE(contains(out, "Square")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

// --- 3/4. Forward method calls ---------------------------------------------

TEST(CodegenCrashersExecutionTest, CtorCallingMethod) {
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
            println("id=${u.id} name=${u.name}");
            println("DONE");
        }
    )";
    // Before the fix the compiler segfaulted in IR verification.
    std::string out = runSPProgram(source, "CtorMethod");
    EXPECT_TRUE(contains(out, "id=user-123 name=Shanto")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

TEST(CodegenCrashersExecutionTest, CtorCallingMethodBare) {
    const std::string source = R"(
        class User {
            string id;
            string name;
            User(string name) {
                this.name = name;
                this.id = generateId();
            }
            fn generateId() -> string {
                return "user-123";
            }
        }
        fn main() {
            var u = new User("Shanto");
            println("id=${u.id} name=${u.name}");
            println("DONE");
        }
    )";
    std::string out = runSPProgram(source, "CtorMethodBare");
    EXPECT_TRUE(contains(out, "id=user-123 name=Shanto")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

TEST(CodegenCrashersExecutionTest, LaterDeclaredValueMethod) {
    const std::string source = R"(
        class Checkout {
            public fn processPayment(double amount, string method) {
                println("Initiating checkout of $${amount}...");
                bool ok = false;
                if (method == "card") {
                    ok = this.payByCard(amount);
                } else if (method == "crypto") {
                    ok = this.payByCrypto(amount);
                }
                if (ok) println("Payment settled successfully.");
            }
            private fn payByCard(double amount) -> bool {
                println("Charging $${amount} via Credit Card gateway.");
                return true;
            }
            private fn payByCrypto(double amount) -> bool {
                println("Generating on-chain transaction for $${amount}.");
                return true;
            }
        }
        fn main() {
            Checkout shop = new Checkout();
            shop.processPayment(150.0, "card");
            shop.processPayment(420.0, "crypto");
            println("DONE");
        }
    )";
    // Before the fix the compiler segfaulted in IR verification: the
    // vtable path computed the call's return type from the module's
    // function table, which only held functions lowered so far.
    std::string out = runSPProgram(source, "MethodOrder");
    EXPECT_TRUE(contains(out, "Charging $150")) << out;
    EXPECT_TRUE(contains(out, "Generating on-chain")) << out;
    EXPECT_EQ(countOccurrences(out, "Payment settled successfully."), 2u) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

TEST(CodegenCrashersExecutionTest, BareMethodCall) {
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
            println("DONE");
        }
    )";
    // Before the fix sema failed with E1001: the builtin free function
    // log(double) shadowed the method and the enclosing-class fallback
    // never ran.
    std::string out = runSPProgram(source, "BareMethodCall");
    EXPECT_TRUE(contains(out, "validated 50")) << out;
    EXPECT_TRUE(contains(out, "log: deposited")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

TEST(CodegenCrashersExecutionTest, BareValueMethodCall) {
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
            println("DONE");
        }
    )";
    std::string out = runSPProgram(source, "BareValueMethodCall");
    EXPECT_TRUE(contains(out, "run=42")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

TEST(CodegenCrashersExecutionTest, BuiltinLogDoubleStillWinsWhenSemaPicksIt) {
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
            println("DONE");
        }
    )";
    // ln(2.5) = 0.91629... — the builtin must win here (sema resolves
    // log(2.5) to the free function first), so the method must NOT run.
    std::string out = runSPProgram(source, "BuiltinLogDoubleStillWins");
    EXPECT_TRUE(contains(out, "v=0.916")) << out;
    EXPECT_FALSE(contains(out, "log:")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

// --- 5. Null coalescing ----------------------------------------------------

TEST(CodegenCrashersExecutionTest, NullCoalesce) {
    const std::string source = R"(
        fn main() {
            string? maybe = null;
            string name = maybe ?? "default";
            println("name=${name}");
            string? present = "hello";
            string name2 = present ?? "default";
            println("name2=${name2}");
            int? n = null;
            int v = n ?? 42;
            println("v=${v}");
            int? m = 7;
            int w = m ?? 42;
            println("w=${w}");
            println("DONE");
        }
    )";
    // Before the fix this emitted `add ptr %v0, @.str.0` — invalid
    // LLVM IR (the default BinaryExpr case is Add).
    std::string out = runSPProgram(source, "NullCoalesce");
    EXPECT_TRUE(contains(out, "name=default")) << out;
    EXPECT_TRUE(contains(out, "name2=hello")) << out;
    EXPECT_TRUE(contains(out, "v=42")) << out;
    EXPECT_TRUE(contains(out, "w=7")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

// --- 6. Safe navigation ----------------------------------------------------

TEST(CodegenCrashersExecutionTest, SafeNavOnNullDoesNotCrash) {
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
            User? user1 = findUser(1);
            println("User 1 name: ${user1?.name}");
            User? user3 = findUser(999);
            println("User 3 name: ${user3?.name}");
            println("DONE");
        }
    )";
    // Before the fix the `user3?.name` interpolation segfaulted (the
    // field load ran on a null object with no null check).
    std::string out = runSPProgram(source, "SafeNavNull");
    EXPECT_TRUE(contains(out, "User 1 name: Shanto")) << out;
    EXPECT_TRUE(contains(out, "User 3 name: ")) << out;
    EXPECT_TRUE(contains(out, "DONE")) << out;
}

// --- 7. fn-pointer HTTP handlers -------------------------------------------
//
// The server is compiled to a real executable, started in the
// background, probed with curl, and killed again — the same shape as
// the Phase 2 registry live proof, but as a gtest.

TEST(CodegenCrashersExecutionTest, NamedFnHttpHandlerTypedRequest) {
    const std::string source = R"(
        import std.web;
        import std.time;
        fn hello(HttpRequest req) -> HttpResponse {
            return req.text("hi typed", 200);
        }
        fn main() {
            var app = new App();
            app.get("/hello", hello);
            app.listen(18771);
            println("serving on 18771");
            while (true) { Time.sleep(1000); }
        }
    )";
    // Build the server executable with the same pipeline as
    // runSPProgram, but keep it around so this test can start it.
    fs::create_directories(getOutputDir());
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, "HttpTypedHandler.sp", &diags);
    auto tokens = lexer.tokenizeAll();
    ASSERT_FALSE(diags.hasErrors());
    parser::Parser parser(tokens, "HttpTypedHandler.sp", &diags);
    auto program = parser.parseProgram();
    ASSERT_NE(program, nullptr);
    ASSERT_FALSE(diags.hasErrors());
    sema::ModuleLoader moduleLoader(diags);
    ASSERT_TRUE(moduleLoader.loadAndMergeAll(*program, "HttpTypedHandler.sp"));
    sema::SemanticAnalyzer sema(diags);
    ASSERT_TRUE(sema.analyze(*program));
    ASSERT_FALSE(diags.hasErrors());
    ir::IRBuilder builder(sema);
    auto module = builder.build(*program, "HttpTypedHandler");
    ASSERT_NE(module, nullptr);
    fs::path exePath = getOutputDir() / "HttpTypedHandler.exe";
    codegen::CodegenOptions opts;
    codegen::LLVMGenerator generator(opts);
    ASSERT_TRUE(generator.compileToExecutable(*module, exePath.string()))
        << generator.getLastError();

    // Skip when curl is unavailable (offline test environment).
    if (std::system("curl --version >/dev/null 2>&1") != 0) {
        GTEST_SKIP() << "curl is not available, cannot probe the test server";
    }

    std::string pidFile = (getOutputDir() / "HttpTypedHandler.pid").string();
    std::string startCmd = "timeout 30 \"" + exePath.string() + "\" >\"" +
        (getOutputDir() / "HttpTypedHandler.log").string() + "\" 2>&1 & echo $! >\"" + pidFile + "\"";
    ASSERT_EQ(std::system(startCmd.c_str()), 0);

    // Poll until the server answers (compile + listen take a moment).
    std::string body;
    for (int i = 0; i < 20; ++i) {
        std::string curlCmd = "curl -s -m 2 http://127.0.0.1:18771/hello";
        FILE* pipe = popen(curlCmd.c_str(), "r");
        if (pipe) {
            char buf[256];
            std::string chunk;
            size_t n = 0;
            while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) chunk.append(buf, n);
            int rc = pclose(pipe);
            if (rc == 0 && !chunk.empty()) {
                body = chunk;
                break;
            }
        }
        int sleepRc = std::system("sleep 1");
        (void)sleepRc;
    }

    // Always kill the server again, even when the assertion below fails.
    std::string killCmd = "kill $(cat \"" + pidFile + "\") 2>/dev/null; sleep 1";
    int killRc = std::system(killCmd.c_str());
    (void)killRc;

    // Before the fix the server segfaulted on the first request (the
    // runtime passed the raw C++ HttpRequest* where the handler
    // expected a real SP HttpRequest object), so curl got an empty
    // reply and the server died.
    EXPECT_EQ(body, "hi typed") << "server log: "
        << [&] {
               std::ifstream f(getOutputDir() / "HttpTypedHandler.log");
               std::stringstream ss;
               ss << f.rdbuf();
               return ss.str();
           }();
}
