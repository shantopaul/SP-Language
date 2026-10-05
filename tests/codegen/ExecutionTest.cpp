#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;
using namespace sp;

namespace {

fs::path getOutputDir() {
#ifdef SP_PROJECT_ROOT
    fs::path p = fs::path(SP_PROJECT_ROOT) / "build";
    if (fs::exists(p)) return p;
#endif
    return fs::temp_directory_path();
}

std::string runSPProgram(const std::string& source, const std::string& testName) {
    diagnostics::DiagnosticEngine diags(std::cerr);
    lexer::Lexer lexer(source, testName + ".sp", &diags);
    auto tokens = lexer.tokenizeAll();
    if (diags.hasErrors()) return "LEXER_ERROR";

    parser::Parser parser(tokens, testName + ".sp", &diags);
    auto program = parser.parseProgram();
    if (diags.hasErrors() || !program) return "PARSER_ERROR";

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
    std::string cmd = "\"" + exePath.string() + "\" > \"" + outPath + "\" 2>&1";
#if defined(_WIN32)
    std::string fullCmd = "\"" + cmd + "\"";
    int code = std::system(fullCmd.c_str());
#else
    int code = std::system(cmd.c_str());
#endif
    (void)code;

    std::ifstream in(outPath);
    std::ostringstream ss;
    ss << in.rdbuf();
    in.close();

    std::error_code ec;
    fs::remove(outPath, ec);
    fs::remove(exePath, ec);
    return ss.str();
}

} // namespace

TEST(ExecutionTest, HelloNativeExecutionMilestone) {
    const std::string source = R"(
        fn main() {
            println("Hello, SP!");
        }
    )";

    std::string output = runSPProgram(source, "exec_hello");
    EXPECT_NE(output.find("Hello, SP!"), std::string::npos);
}

TEST(ExecutionTest, BasicArithmeticAndVariables) {
    const std::string source = R"(
        fn main() {
            var a = 15;
            var b = 7;
            println(a + b);
            println(a - b);
            println(a * b);
            println(a / b);
            println(a % b);
        }
    )";

    std::string output = runSPProgram(source, "exec_arith");
    EXPECT_NE(output.find("22"), std::string::npos);
    EXPECT_NE(output.find("8"), std::string::npos);
    EXPECT_NE(output.find("105"), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("1"), std::string::npos);
}

TEST(ExecutionTest, BooleanAndRelationalLogic) {
    const std::string source = R"(
        fn main() {
            var t = true;
            var f = false;
            println(t && f);
            println(t || f);
            println(!f);
            println(10 > 5);
            println(10 == 20);
        }
    )";

    std::string output = runSPProgram(source, "exec_bool");
    EXPECT_NE(output.find("false"), std::string::npos);
    EXPECT_NE(output.find("true"), std::string::npos);
}

TEST(ExecutionTest, ConditionalBranchingAndWhileLoop) {
    const std::string source = R"(
        fn sum_to(n: int) -> int {
            var s = 0;
            var i = 1;
            while (i <= n) {
                s = s + i;
                i = i + 1;
            }
            return s;
        }

        fn main() {
            println(sum_to(10));
        }
    )";

    std::string output = runSPProgram(source, "exec_loop");
    // sum from 1 to 10 is 55
    EXPECT_NE(output.find("55"), std::string::npos);
}

TEST(ExecutionTest, RecursiveFibonacciExecution) {
    const std::string source = R"(
        fn fibonacci(n: int) -> int {
            if (n <= 0) return 0;
            if (n == 1) return 1;
            return fibonacci(n - 1) + fibonacci(n - 2);
        }

        fn main() {
            println(fibonacci(10));
        }
    )";

    std::string output = runSPProgram(source, "exec_fib");
    // fib(10) = 55
    EXPECT_NE(output.find("55"), std::string::npos);
}

TEST(ExecutionTest, ShortFormFunctionExecution) {
    const std::string source = R"(
        fn square(x: int) -> int => x * x;

        fn main() {
            println(square(6));
        }
    )";

    std::string output = runSPProgram(source, "exec_short_fn");
    EXPECT_NE(output.find("36"), std::string::npos);
}

TEST(ExecutionTest, FloatingPointOperations) {
    const std::string source = R"(
        fn main() {
            var x = 3.5;
            var y = 2.5;
            println(x * y);
        }
    )";

    std::string output = runSPProgram(source, "exec_float");
    EXPECT_NE(output.find("8.75"), std::string::npos);
}

TEST(ExecutionTest, RuntimeAssertSuccess) {
    const std::string source = R"(
        fn main() {
            assert(true, "Condition should hold");
            println("Assert passed");
        }
    )";

    std::string output = runSPProgram(source, "exec_assert");
    EXPECT_NE(output.find("Assert passed"), std::string::npos);
}

TEST(ExecutionTest, TypedVariableDeclarations) {
    const std::string source = R"(
        fn main() {
            let a: int = 10;
            let b: int = 25;
            let sum: int = a + b;
            println("Calculation: 10 + 25 =");
            println(sum);
        }
    )";

    std::string output = runSPProgram(source, "exec_typed_vars");
    EXPECT_NE(output.find("Calculation: 10 + 25 ="), std::string::npos);
    EXPECT_NE(output.find("35"), std::string::npos);
}

TEST(ExecutionTest, ObjectInstantiationAndMethodCalls) {
    const std::string source = R"(
        class Counter {
            var value: int;

            Counter(int initial) {
                this.value = initial;
            }

            fn increment(int delta) {
                this.value = this.value + delta;
            }

            fn getValue() -> int {
                return this.value;
            }
        }

        fn main() {
            var c: Counter = new Counter(100);
            c.increment(25);
            println(c.getValue());
        }
    )";

    std::string output = runSPProgram(source, "exec_counter");
    EXPECT_NE(output.find("125"), std::string::npos);
}

TEST(ExecutionTest, PolymorphicVirtualDispatchNativeExecution) {
    const std::string source = R"(
        class Animal {
            fn getSoundCode() -> int {
                return 101;
            }
        }

        class Dog extends Animal {
            override fn getSoundCode() -> int {
                return 202;
            }
        }

        fn main() {
            var pet: Animal = new Dog();
            println(pet.getSoundCode());
        }
    )";

    std::string output = runSPProgram(source, "exec_poly");
    EXPECT_NE(output.find("202"), std::string::npos);
}

TEST(ExecutionTest, StructInstantiationAndFunctionPassing) {
    const std::string source = R"(
        struct Point {
            int x;
            int y;
        }

        fn manhattan(Point a, Point b) -> int {
            return (b.x - a.x) + (b.y - a.y);
        }

        fn main() {
            Point p1;
            p1.x = 10;
            p1.y = 20;

            Point p2;
            p2.x = 40;
            p2.y = 60;

            println(manhattan(p1, p2));
        }
    )";

    std::string output = runSPProgram(source, "exec_struct");
    EXPECT_NE(output.find("70"), std::string::npos);
}

TEST(ExecutionTest, EnumConstantsAndBranchExecution) {
    const std::string source = R"(
        enum HttpStatus {
            OK = 200,
            NotFound = 404,
            ServerError = 500
        }

        fn main() {
            var status = HttpStatus.NotFound;
            if (status == HttpStatus.OK) {
                println("Success");
            } else if (status == HttpStatus.NotFound) {
                println("Resource Not Found: 404");
            } else {
                println("Other");
            }
        }
    )";

    std::string output = runSPProgram(source, "exec_enum");
    EXPECT_NE(output.find("Resource Not Found: 404"), std::string::npos);
}

TEST(ExecutionTest, NativeArrayAllocationIndexingAndLengthExecution) {
    const std::string source = R"(
        fn main() {
            var nums = [10, 20, 30];
            println(nums.length);
            println(nums[0]);
            println(nums[1]);
            nums[1] = 99;
            println(nums[1]);
        }
    )";

    std::string output = runSPProgram(source, "exec_array");
    EXPECT_NE(output.find("3"), std::string::npos);
    EXPECT_NE(output.find("10"), std::string::npos);
    EXPECT_NE(output.find("99"), std::string::npos);
}

TEST(ExecutionTest, GenericClassMonomorphizationExecution) {
    const std::string source = R"(
        class Box<T> {
            var val: T;

            Box(T initial) {
                this.val = initial;
            }

            fn get() -> T {
                return this.val;
            }

            fn set(T newVal) {
                this.val = newVal;
            }
        }

        fn main() {
            var intBox = new Box<int>(42);
            println(intBox.get());
            intBox.set(100);
            println(intBox.get());
        }
    )";

    std::string output = runSPProgram(source, "exec_box_mono");
    EXPECT_NE(output.find("42"), std::string::npos);
    EXPECT_NE(output.find("100"), std::string::npos);
}

TEST(ExecutionTest, GenericFunctionMonomorphizationExecution) {
    const std::string source = R"(
        fn chooseFirst<T>(T a, T b) -> T {
            return a;
        }

        fn main() {
            var x = chooseFirst<int>(77, 99);
            println(x);
        }
    )";

    std::string output = runSPProgram(source, "exec_fn_mono");
    EXPECT_NE(output.find("77"), std::string::npos);
}

