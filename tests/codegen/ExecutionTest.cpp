#include <gtest/gtest.h>
#include "sp/lexer/Lexer.h"
#include "sp/parser/Parser.h"
#include "sp/sema/SemanticAnalyzer.h"
#include "sp/ir/IRBuilder.h"
#include "sp/codegen/LLVMGenerator.h"
#include "sp/ir/IRPrinter.h"
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

TEST(ExecutionTest, StandardListCollectionNativeExecution) {
    const std::string source = R"(
        fn main() {
            var list = new List<int>();
            list.push(10);
            list.push(20);
            list.push(30);
            println(list.size());
            println(list[0]);
            println(list[1]);
            list[1] = 55;
            println(list[1]);
            var lastVal = list.pop();
            println(lastVal);
            println(list.size());
        }
    )";

    std::string output = runSPProgram(source, "exec_list");
    EXPECT_NE(output.find("3"), std::string::npos);
    EXPECT_NE(output.find("10"), std::string::npos);
    EXPECT_NE(output.find("55"), std::string::npos);
    EXPECT_NE(output.find("30"), std::string::npos);
    EXPECT_NE(output.find("2"), std::string::npos);
}

TEST(ExecutionTest, StandardMapCollectionNativeExecution) {
    const std::string source = R"(
        fn main() {
            var map = new Map<string, int>();
            map["apple"] = 100;
            map["banana"] = 250;
            println(map.size());
            println(map["apple"]);
            println(map["banana"]);
        }
    )";

    std::string output = runSPProgram(source, "exec_map");
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("100"), std::string::npos);
    EXPECT_NE(output.find("250"), std::string::npos);
}

TEST(ExecutionTest, ForInLoopArrayAndListNativeExecution) {
    const std::string source = R"(
        fn main() {
            var nums = [10, 20, 30];
            var sum = 0;
            for (x in nums) {
                sum = sum + x;
            }
            println("Array Sum:");
            println(sum);

            var list = new List<int>();
            list.push(5);
            list.push(15);
            list.push(25);
            var listSum = 0;
            for (item in list) {
                listSum = listSum + item;
            }
            println("List Sum:");
            println(listSum);
        }
    )";

    std::string output = runSPProgram(source, "exec_for_in");
    EXPECT_NE(output.find("Array Sum:"), std::string::npos);
    EXPECT_NE(output.find("60"), std::string::npos);
    EXPECT_NE(output.find("List Sum:"), std::string::npos);
    EXPECT_NE(output.find("45"), std::string::npos);
}

TEST(ExecutionTest, LambdaFunctionsAndClosuresNativeExecution) {
    const std::string source = R"(
        fn main() {
            var add = (int a, int b) => a + b;
            var square = (int x) => x * x;
            var factorial = (int n) => {
                var r = 1;
                for (var i = 2; i <= n; i = i + 1) {
                    r = r * i;
                }
                return r;
            };

            println(add(12, 18));
            println(square(9));
            println(factorial(5));
        }
    )";

    std::string output = runSPProgram(source, "exec_lambda");
    EXPECT_NE(output.find("30"), std::string::npos);
    EXPECT_NE(output.find("81"), std::string::npos);
    EXPECT_NE(output.find("120"), std::string::npos);
}

TEST(ExecutionTest, HigherOrderFunctionsNativeExecution) {
    const std::string source = R"(
        fn main() {
            var nums = [1, 2, 3, 4, 5];
            var doubled = nums.map((int x) => x * 2);
            for (x in doubled) {
                println(x);
            }

            var evens = nums.filter((int x) => x % 2 == 0);
            for (x in evens) {
                println(x);
            }

            var sum = nums.reduce((int acc, int x) => acc + x, 0);
            println(sum);

            var list = new List<int>();
            list.push(10);
            list.push(20);
            list.push(30);
            var mappedList = list.map((int x) => x / 10);
            for (item in mappedList) {
                println(item);
            }
        }
    )";

    std::string output = runSPProgram(source, "exec_hof");
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("4"), std::string::npos);
    EXPECT_NE(output.find("6"), std::string::npos);
    EXPECT_NE(output.find("8"), std::string::npos);
    EXPECT_NE(output.find("10"), std::string::npos);
    EXPECT_NE(output.find("15"), std::string::npos);
    EXPECT_NE(output.find("1"), std::string::npos);
    EXPECT_NE(output.find("3"), std::string::npos);
}

TEST(ExecutionTest, StandardSetCollectionNativeExecution) {
    const std::string source = R"(
        fn main() {
            var s = new Set<int>();
            s.add(10);
            s.add(20);
            s.add(10);
            println(s.size());
            println(s.contains(10));
            println(s.contains(30));
            s.remove(10);
            println(s.size());
            println(s.contains(10));

            var strSet = new Set<string>();
            strSet.add("apple");
            strSet.add("banana");
            println(strSet.size());
            println(strSet.contains("apple"));
            println(strSet.contains("orange"));
        }
    )";

    std::string output = runSPProgram(source, "exec_set");
    EXPECT_NE(output.find("2"), std::string::npos);
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("false"), std::string::npos);
    EXPECT_NE(output.find("1"), std::string::npos);
}

TEST(ExecutionTest, VariablesConstantsAndInterpolationNativeExecution) {
    const std::string source = R"(
        main() {
            // Explicit types
            int age = 21;
            string name = "Shanto";
            double salary = 75000.50;
            float score = 95.5;
            bool isActive = true;
            char grade = 'A';

            println("Name: ${name}");
            println("Age: ${age}");
            println("Salary: ${salary}");
            println("Score: ${score}");
            println("Active: ${isActive}");
            println("Grade: ${grade}");

            // Variables can be reassigned
            age = 22;
            name = "Shanto Paul";
            println("Updated: ${name}, age ${age}");

            // Constants
            const int MAX_USERS = 1000;
            const double TAX_RATE = 0.15;
            const string APP_NAME = "MyApp";

            println("App: ${APP_NAME}");
            println("Max Users: ${MAX_USERS}");
            println("Tax Rate: ${TAX_RATE}");
        }
    )";

    std::string output = runSPProgram(source, "exec_var_const");
    EXPECT_NE(output.find("Name: Shanto"), std::string::npos);
    EXPECT_NE(output.find("Age: 21"), std::string::npos);
    EXPECT_NE(output.find("Salary: 75000.5"), std::string::npos);
    EXPECT_NE(output.find("Score: 95.5"), std::string::npos);
    EXPECT_NE(output.find("Active: true"), std::string::npos);
    EXPECT_NE(output.find("Grade: A"), std::string::npos);
    EXPECT_NE(output.find("Updated: Shanto Paul, age 22"), std::string::npos);
    EXPECT_NE(output.find("App: MyApp"), std::string::npos);
    EXPECT_NE(output.find("Max Users: 1000"), std::string::npos);
    EXPECT_NE(output.find("Tax Rate: 0.15"), std::string::npos);
}

TEST(ExecutionTest, AllPrimitiveTypesNativeExecution) {
    const std::string source = R"(
        fn main() {
            int number = 42;
            uint positive = 100;

            int8 tiny = 127;
            int16 small = 32000;
            int32 medium = 2147483647;
            int64 large = 9223372036854;

            uint8 uTiny = 255;
            uint16 uSmall = 65535;
            uint32 uMedium = 4294967295;
            uint64 uLarge = 18446744073;

            float price = 29.99;
            double precise = 3.141592653589793;

            bool alive = true;
            bool dead = false;

            char letter = 'A';
            char symbol = '@';
            char newline = '\n';

            string text = "Hello, SP!";
            string empty = "";

            byte raw = 0xFF;

            println("int:    ${number}");
            println("float:  ${price}");
            println("double: ${precise}");
            println("bool:   ${alive}");
            println("char:   ${letter}");
            println("string: ${text}");
            println("byte:   ${raw}");
        }
    )";

    std::string output = runSPProgram(source, "exec_primitive_types");
    EXPECT_NE(output.find("int:    42"), std::string::npos);
    EXPECT_NE(output.find("float:  29.99"), std::string::npos);
    EXPECT_NE(output.find("double: 3.141592653589793"), std::string::npos);
    EXPECT_NE(output.find("bool:   true"), std::string::npos);
    EXPECT_NE(output.find("char:   A"), std::string::npos);
    EXPECT_NE(output.find("string: Hello, SP!"), std::string::npos);
    EXPECT_NE(output.find("byte:   255"), std::string::npos);
}


