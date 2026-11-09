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

std::string runSPProgram(const std::string& source, const std::string& testName, const std::string& input = "") {
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
    fs::remove(inPath, ec);
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

TEST(ExecutionTest, TypeInferenceNativeExecution) {
    const std::string source = R"(
        fn main() {
            var age = 21;
            var name = "Shanto";
            var price = 99.99;
            var active = true;

            age = 22;
            name = "Shanto Paul";

            let count = 100;
            let message = "Hello";

            const PI = 3.14159;
            const APP = "SP Lang";

            println("Age: ${age}");
            println("Name: ${name}");
            println("PI: ${PI}");
            println("App: ${APP}");
        }
    )";

    std::string output = runSPProgram(source, "exec_type_inference");
    EXPECT_NE(output.find("Age: 22"), std::string::npos);
    EXPECT_NE(output.find("Name: Shanto Paul"), std::string::npos);
    EXPECT_NE(output.find("PI: 3.14159"), std::string::npos);
    EXPECT_NE(output.find("App: SP Lang"), std::string::npos);
}

TEST(ExecutionTest, OperatorsNativeExecution) {
    const std::string source = R"(
        fn main() {
            int a = 20;
            int b = 7;

            println("${a} + ${b} = ${a + b}");
            println("${a} - ${b} = ${a - b}");
            println("${a} * ${b} = ${a * b}");
            println("${a} / ${b} = ${a / b}");
            println("${a} % ${b} = ${a % b}");

            println("${a} == ${b} = ${a == b}");
            println("${a} != ${b} = ${a != b}");
            println("${a} > ${b} = ${a > b}");
            println("${a} < ${b} = ${a < b}");
            println("${a} >= ${b} = ${a >= b}");
            println("${a} <= ${b} = ${a <= b}");

            bool x = true;
            bool y = false;

            println("${x} && ${y} = ${x && y}");
            println("${x} || ${y} = ${x || y}");
            println("!${x} = ${!x}");

            int n = 10;
            n += 5;
            println("n += 5 = ${n}");
            n -= 3;
            println("n -= 3 = ${n}");
            n *= 2;
            println("n *= 2 = ${n}");
            n /= 4;
            println("n /= 4 = ${n}");
            n %= 4;
            println("n %= 4 = ${n}");

            int counter = 0;
            counter++;
            println("counter++ = ${counter}");
            counter++;
            println("counter++ = ${counter}");
            counter--;
            println("counter-- = ${counter}");

            int p = 0b1100;
            int q = 0b1010;

            println("p & q = ${p & q}");
            println("p | q = ${p | q}");
            println("p ^ q = ${p ^ q}");
            println("~p = ${~p}");
            println("p << 1 = ${p << 1}");
            println("p >> 1 = ${p >> 1}");
        }
    )";

    std::string output = runSPProgram(source, "exec_operators");
    EXPECT_NE(output.find("20 + 7 = 27"), std::string::npos);
    EXPECT_NE(output.find("20 - 7 = 13"), std::string::npos);
    EXPECT_NE(output.find("20 * 7 = 140"), std::string::npos);
    EXPECT_NE(output.find("20 / 7 = 2"), std::string::npos);
    EXPECT_NE(output.find("20 % 7 = 6"), std::string::npos);
    EXPECT_NE(output.find("20 == 7 = false"), std::string::npos);
    EXPECT_NE(output.find("20 != 7 = true"), std::string::npos);
    EXPECT_NE(output.find("20 > 7 = true"), std::string::npos);
    EXPECT_NE(output.find("20 < 7 = false"), std::string::npos);
    EXPECT_NE(output.find("20 >= 7 = true"), std::string::npos);
    EXPECT_NE(output.find("20 <= 7 = false"), std::string::npos);
    EXPECT_NE(output.find("true && false = false"), std::string::npos);
    EXPECT_NE(output.find("true || false = true"), std::string::npos);
    EXPECT_NE(output.find("!true = false"), std::string::npos);
    EXPECT_NE(output.find("n += 5 = 15"), std::string::npos);
    EXPECT_NE(output.find("n -= 3 = 12"), std::string::npos);
    EXPECT_NE(output.find("n *= 2 = 24"), std::string::npos);
    EXPECT_NE(output.find("n /= 4 = 6"), std::string::npos);
    EXPECT_NE(output.find("n %= 4 = 2"), std::string::npos);
    EXPECT_NE(output.find("counter++ = 1"), std::string::npos);
    EXPECT_NE(output.find("counter++ = 2"), std::string::npos);
    EXPECT_NE(output.find("counter-- = 1"), std::string::npos);
    EXPECT_NE(output.find("p & q = 8"), std::string::npos);
    EXPECT_NE(output.find("p | q = 14"), std::string::npos);
    EXPECT_NE(output.find("p ^ q = 6"), std::string::npos);
    EXPECT_NE(output.find("~p = -13"), std::string::npos);
    EXPECT_NE(output.find("p << 1 = 24"), std::string::npos);
    EXPECT_NE(output.find("p >> 1 = 6"), std::string::npos);
}

TEST(ExecutionTest, StringsAndInterpolationNativeExecution) {
    const std::string source = R"(
        fn main() {
            string firstName = "Shanto";
            string lastName = "Paul";
            int age = 21;

            string fullName = firstName + " " + lastName;
            println(fullName);

            println("Hello, ${firstName}!");
            println("Full Name: ${firstName} ${lastName}");
            println("Age next year: ${age + 1}");
            println("Name length: ${firstName.length()}");
            println("Uppercase: ${firstName.upper()}");

            println("Line 1\nLine 2");
            println("Tab\there");
            println("She said \"Hello\"");
            println("Backslash: \\");
            println("Unicode: \u0041");

            string poem = "Roses are red,\n"
                        + "Violets are blue,\n"
                        + "SP is awesome,\n"
                        + "And so are you.";

            println(poem);
        }
    )";

    std::string output = runSPProgram(source, "exec_strings_interpolation");
    EXPECT_NE(output.find("Shanto Paul"), std::string::npos);
    EXPECT_NE(output.find("Hello, Shanto!"), std::string::npos);
    EXPECT_NE(output.find("Full Name: Shanto Paul"), std::string::npos);
    EXPECT_NE(output.find("Age next year: 22"), std::string::npos);
    EXPECT_NE(output.find("Name length: 6"), std::string::npos);
    EXPECT_NE(output.find("Uppercase: SHANTO"), std::string::npos);
    EXPECT_NE(output.find("Line 1\nLine 2"), std::string::npos);
    EXPECT_NE(output.find("Tab\there"), std::string::npos);
    EXPECT_NE(output.find("She said \"Hello\""), std::string::npos);
    EXPECT_NE(output.find("Backslash: \\"), std::string::npos);
    EXPECT_NE(output.find("Unicode: A"), std::string::npos);
    EXPECT_NE(output.find("Roses are red,\nViolets are blue,\nSP is awesome,\nAnd so are you."), std::string::npos);
}

TEST(ExecutionTest, ConsoleIOAndParseNativeExecution) {
    const std::string source = R"(
        import std.io;

        fn main() {
            println("Hello, World!");
            print("Enter name: ");

            string name = readLine();
            println("Hello, ${name}!");

            print("Enter age: ");
            int age = int.parse(readLine());
            println("You are ${age} years old.");

            print("Enter score: ");
            double score = double.parse(readLine());
            println("Your score is ${score}");
        }
    )";

    std::string output = runSPProgram(source, "exec_console_io", "Shanto\n21\n95.5\n");
    EXPECT_NE(output.find("Hello, World!"), std::string::npos);
    EXPECT_NE(output.find("Hello, Shanto!"), std::string::npos);
    EXPECT_NE(output.find("You are 21 years old."), std::string::npos);
    EXPECT_NE(output.find("Your score is 95.5"), std::string::npos);
}

TEST(ExecutionTest, FunctionsComprehensiveNativeExecution) {
    const std::string source = R"(
        fn add(int a, int b) -> int {
            return a + b;
        }

        fn greet(string name) {
            println("Hello, ${name}!");
        }

        fn isAdult(int age) -> bool {
            return age >= 18;
        }

        fn getFullName(string first, string last) -> string {
            return "${first} ${last}";
        }

        int multiply(int a, int b) {
            return a * b;
        }

        string formatAge(string name, int age) {
            return "${name} is ${age} years old";
        }

        void sayGoodbye(string name) {
            println("Goodbye, ${name}!");
        }

        fn square(int x) => x * x;
        fn cube(int x) => x * x * x;
        fn max(int a, int b) => a > b ? a : b;
        fn min(int a, int b) => a < b ? a : b;
        fn isEven(int n) => n % 2 == 0;
        fn doubleNum(int n) => n * 2;

        fn createUser(string name, int age, string role = "user") -> string {
            return "${name} (${age}) - ${role}";
        }

        fn factorial(int n) -> int {
            if (n <= 1) return 1;
            return n * factorial(n - 1);
        }

        fn fibonacci(int n) -> int {
            if (n <= 0) return 0;
            if (n == 1) return 1;
            return fibonacci(n - 1) + fibonacci(n - 2);
        }

        fn main() {
            int sum = add(10, 20);
            println("Sum: ${sum}");

            greet("Shanto");

            println(isAdult(21));
            println(isAdult(15));

            println(getFullName("Shanto", "Paul"));

            println(multiply(5, 4));
            println(formatAge("Shanto", 21));
            sayGoodbye("World");

            println("5² = ${square(5)}");
            println("3³ = ${cube(3)}");
            println("max(3,7) = ${max(3, 7)}");
            println("min(3,7) = ${min(3, 7)}");
            println("isEven(4) = ${isEven(4)}");
            println("double(8) = ${doubleNum(8)}");

            println(createUser("Shanto", 21));
            println(createUser("Admin", 30, "admin"));

            println("5! = ${factorial(5)}");
            println("10! = ${factorial(10)}");

            for (int i = 0; i < 10; i++) {
                print("${fibonacci(i)} ");
            }
            println("");
        }
    )";

    std::string output = runSPProgram(source, "exec_functions");
    EXPECT_NE(output.find("Sum: 30"), std::string::npos);
    EXPECT_NE(output.find("Hello, Shanto!"), std::string::npos);
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("false"), std::string::npos);
    EXPECT_NE(output.find("Shanto Paul"), std::string::npos);
    EXPECT_NE(output.find("20"), std::string::npos);
    EXPECT_NE(output.find("Shanto is 21 years old"), std::string::npos);
    EXPECT_NE(output.find("Goodbye, World!"), std::string::npos);
    EXPECT_NE(output.find("5² = 25"), std::string::npos);
    EXPECT_NE(output.find("3³ = 27"), std::string::npos);
    EXPECT_NE(output.find("max(3,7) = 7"), std::string::npos);
    EXPECT_NE(output.find("min(3,7) = 3"), std::string::npos);
    EXPECT_NE(output.find("isEven(4) = true"), std::string::npos);
    EXPECT_NE(output.find("double(8) = 16"), std::string::npos);
    EXPECT_NE(output.find("Shanto (21) - user"), std::string::npos);
    EXPECT_NE(output.find("Admin (30) - admin"), std::string::npos);
    EXPECT_NE(output.find("5! = 120"), std::string::npos);
    EXPECT_NE(output.find("10! = 3628800"), std::string::npos);
    EXPECT_NE(output.find("0 1 1 2 3 5 8 13 21 34"), std::string::npos);
}

TEST(ExecutionTest, IfElseControlFlowNativeExecution) {
    const std::string source = R"(
        fn main() {
            int age = 21;
            int score = 85;
            string role = "admin";

            if (age >= 18) {
                println("You are an adult.");
            }

            if (age >= 18) {
                println("Welcome! You can enter.");
            } else {
                println("Sorry, you must be 18+.");
            }

            if (score >= 90) {
                println("Grade: A  ⭐ Excellent!");
            } else if (score >= 80) {
                println("Grade: B  👍 Good job!");
            } else if (score >= 70) {
                println("Grade: C  📝 Average");
            } else if (score >= 60) {
                println("Grade: D  ⚠️ Below average");
            } else {
                println("Grade: F  ❌ Failed");
            }

            if (age >= 18) {
                if (role == "admin") {
                    println("Admin access granted.");
                } else {
                    println("User access granted.");
                }
            } else {
                println("Access denied.");
            }

            bool hasLicense = true;

            if (age >= 18 && hasLicense) {
                println("You can drive! 🚗");
            }

            if (role == "admin" || role == "moderator") {
                println("You have elevated permissions.");
            }

            if (!hasLicense) {
                println("Get a license first.");
            }

            string status = age >= 18 ? "Adult" : "Minor";
            println("Status: ${status}");
        }
    )";

    std::string output = runSPProgram(source, "exec_if_else");
    EXPECT_NE(output.find("You are an adult."), std::string::npos);
    EXPECT_NE(output.find("Welcome! You can enter."), std::string::npos);
    EXPECT_NE(output.find("Grade: B  👍 Good job!"), std::string::npos);
    EXPECT_NE(output.find("Admin access granted."), std::string::npos);
    EXPECT_NE(output.find("You can drive! 🚗"), std::string::npos);
    EXPECT_NE(output.find("You have elevated permissions."), std::string::npos);
    EXPECT_NE(output.find("Status: Adult"), std::string::npos);
}

TEST(ExecutionTest, SwitchAndMatchNativeExecution) {
    const std::string source = R"(
        fn classicSwitchDemo(int day) {
            println("--- Classic Switch ---");
            switch (day) {
                case 1:
                    println("Monday");
                    break;
                case 2:
                    println("Tuesday");
                    break;
                case 3:
                    println("Wednesday");
                    break;
                case 4:
                    println("Thursday");
                    break;
                case 5:
                    println("Friday");
                    break;
                case 6:
                    println("Saturday 🎉");
                    break;
                case 7:
                    println("Sunday 🎉");
                    break;
                default:
                    println("Invalid day");
            }
        }

        fn modernMatchDemo() {
            int day = 3;

            println("--- Modern Match ---");
            match day {
                1 => println("Monday"),
                2 => println("Tuesday"),
                3 => println("Wednesday"),
                4 => println("Thursday"),
                5 => println("Friday"),
                6 => println("Saturday 🎉"),
                7 => println("Sunday 🎉"),
                _ => println("Invalid day")
            }

            string command = "start";

            match command {
                "start"   => println("Starting server..."),
                "stop"    => println("Stopping server..."),
                "restart" => println("Restarting server..."),
                "status"  => println("Server is running."),
                _         => println("Unknown command: ${command}")
            }

            enum Season { Spring, Summer, Autumn, Winter }

            Season current = Season.Summer;

            match current {
                Season.Spring => println("🌸 Flowers blooming"),
                Season.Summer => println("☀️ Beach time!"),
                Season.Autumn => println("🍂 Leaves falling"),
                Season.Winter => println("❄️ Stay warm!")
            }
        }

        fn main() {
            classicSwitchDemo(3);
            classicSwitchDemo(99);
            modernMatchDemo();
        }
    )";

    std::string output = runSPProgram(source, "exec_switch_match");
    EXPECT_NE(output.find("--- Classic Switch ---"), std::string::npos);
    EXPECT_NE(output.find("Wednesday"), std::string::npos);
    EXPECT_NE(output.find("Invalid day"), std::string::npos);
    EXPECT_NE(output.find("--- Modern Match ---"), std::string::npos);
    EXPECT_NE(output.find("Starting server..."), std::string::npos);
    EXPECT_NE(output.find("☀️ Beach time!"), std::string::npos);
}

TEST(ExecutionTest, ForLoopsComprehensiveNativeExecution) {
    const std::string source = R"(
        fn main() {
            println("=== Classic For ===");
            for (int i = 1; i <= 5; i++) {
                println("Count: ${i}");
            }

            println("\n=== Range Loop ===");
            for (i in 0..5) {
                print("${i} ");
            }
            println("");

            println("\n=== For-Each ===");
            let fruits = ["Apple", "Banana", "Cherry", "Date"];

            for (string fruit in fruits) {
                println("🍎 ${fruit}");
            }

            println("\n=== Multiplication Table (1-5) ===");
            for (int i = 1; i <= 5; i++) {
                for (int j = 1; j <= 5; j++) {
                    print("${i * j}\t");
                }
                println("");
            }

            println("\n=== Break at 5 ===");
            for (int i = 1; i <= 10; i++) {
                if (i == 5) break;
                print("${i} ");
            }
            println("");

            println("\n=== Skip even numbers ===");
            for (int i = 1; i <= 10; i++) {
                if (i % 2 == 0) continue;
                print("${i} ");
            }
            println("");

            println("\n=== Countdown ===");
            for (int i = 5; i >= 1; i--) {
                println("${i}...");
            }
            println("🚀 Liftoff!");

            int sum = 0;
            for (int i = 1; i <= 100; i++) {
                sum += i;
            }
            println("\nSum of 1-100: ${sum}");
        }
    )";

    std::string output = runSPProgram(source, "exec_for_loops");
    EXPECT_NE(output.find("Count: 1"), std::string::npos);
    EXPECT_NE(output.find("Count: 5"), std::string::npos);
    EXPECT_NE(output.find("0 1 2 3 4"), std::string::npos);
    EXPECT_NE(output.find("🍎 Apple"), std::string::npos);
    EXPECT_NE(output.find("🍎 Date"), std::string::npos);
    EXPECT_NE(output.find("1\t2\t3\t4\t5"), std::string::npos);
    EXPECT_NE(output.find("5\t10\t15\t20\t25"), std::string::npos);
    EXPECT_NE(output.find("1 2 3 4"), std::string::npos);
    EXPECT_NE(output.find("1 3 5 7 9"), std::string::npos);
    EXPECT_NE(output.find("5..."), std::string::npos);
    EXPECT_NE(output.find("1..."), std::string::npos);
    EXPECT_NE(output.find("🚀 Liftoff!"), std::string::npos);
    EXPECT_NE(output.find("Sum of 1-100: 5050"), std::string::npos);
}

TEST(ExecutionTest, WhileAndDoWhileNativeExecution) {
    const std::string source = R"sp(
        fn main() {
            println("=== While Loop ===");
            int count = 1;
            while (count <= 5) {
                println("Count: ${count}");
                count++;
            }

            println("\n=== Do-While Loop ===");
            int n = 1;
            do {
                println("n = ${n}");
                n *= 2;
            } while (n <= 16);

            int power = 1;
            int exponent = 0;
            while (power <= 1000) {
                power *= 2;
                exponent++;
            }
            println("\n2^${exponent} = ${power} (first power of 2 > 1000)");

            println("\n=== Collatz Sequence (start: 27) ===");
            int num = 27;
            int steps = 0;
            print("${num}");
            while (num != 1) {
                if (num % 2 == 0) {
                    num = num / 2;
                } else {
                    num = num * 3 + 1;
                }
                print(" -> ${num}");
                steps++;
            }
            println("\nReached 1 in ${steps} steps");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_while_loops");
    EXPECT_NE(output.find("=== While Loop ==="), std::string::npos);
    EXPECT_NE(output.find("Count: 1"), std::string::npos);
    EXPECT_NE(output.find("Count: 5"), std::string::npos);
    EXPECT_NE(output.find("=== Do-While Loop ==="), std::string::npos);
    EXPECT_NE(output.find("n = 1"), std::string::npos);
    EXPECT_NE(output.find("n = 16"), std::string::npos);
    EXPECT_NE(output.find("2^10 = 1024 (first power of 2 > 1000)"), std::string::npos);
    EXPECT_NE(output.find("=== Collatz Sequence (start: 27) ==="), std::string::npos);
    EXPECT_NE(output.find("27 -> 82"), std::string::npos);
    EXPECT_NE(output.find("-> 1"), std::string::npos);
    EXPECT_NE(output.find("Reached 1 in 111 steps"), std::string::npos);
}

TEST(ExecutionTest, ArraysComprehensiveNativeExecution) {
    const std::string source = R"sp(
        fn main() {
            int[] numbers = {10, 20, 30, 40, 50};
            string[] names = {"Alice", "Bob", "Charlie"};
            let scores = [95, 87, 92, 78, 88];

            println("First: ${numbers[0]}");
            println("Last: ${numbers[4]}");

            numbers[0] = 100;
            println("Updated first: ${numbers[0]}");

            println("\nAll names:");
            for (string name in names) {
                println("  - ${name}");
            }

            println("\nAll scores:");
            for (int i = 0; i < scores.length(); i++) {
                println("  Student ${i + 1}: ${scores[i]}");
            }

            println("\nLength: ${scores.length()}");

            int maxScore = scores[0];
            for (int s in scores) {
                if (s > maxScore) maxScore = s;
            }
            println("Highest score: ${maxScore}");

            int total = 0;
            for (int s in scores) {
                total += s;
            }
            println("Total: ${total}");
            println("Average: ${total / scores.length()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_arrays");
    EXPECT_NE(output.find("First: 10"), std::string::npos);
    EXPECT_NE(output.find("Last: 50"), std::string::npos);
    EXPECT_NE(output.find("Updated first: 100"), std::string::npos);
    EXPECT_NE(output.find("All names:"), std::string::npos);
    EXPECT_NE(output.find("- Alice"), std::string::npos);
    EXPECT_NE(output.find("- Bob"), std::string::npos);
    EXPECT_NE(output.find("- Charlie"), std::string::npos);
    EXPECT_NE(output.find("Student 1: 95"), std::string::npos);
    EXPECT_NE(output.find("Student 5: 88"), std::string::npos);
    EXPECT_NE(output.find("Length: 5"), std::string::npos);
    EXPECT_NE(output.find("Highest score: 95"), std::string::npos);
    EXPECT_NE(output.find("Total: 440"), std::string::npos);
    EXPECT_NE(output.find("Average: 88"), std::string::npos);
}

TEST(ExecutionTest, StdlibIONativeExecution) {
    const std::string source = R"sp(
        import std.io;

        fn main() {
            println("=== Stdlib IO Test ===");
            print("Direct print: ");
            println(42);
            println(3.14);
            println(true);

            eprintln("[STDERR] Test diagnostic message");

            IO.println("Via IO.println");
            IO.print("Via IO.print\n");
            IO.flush();
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_io");
    EXPECT_NE(output.find("=== Stdlib IO Test ==="), std::string::npos);
    EXPECT_NE(output.find("Direct print: 42"), std::string::npos);
    EXPECT_NE(output.find("3.14"), std::string::npos);
    EXPECT_NE(output.find("true"), std::string::npos);
    EXPECT_NE(output.find("Via IO.println"), std::string::npos);
    EXPECT_NE(output.find("Via IO.print"), std::string::npos);
}

TEST(ExecutionTest, StdlibStringNativeExecution) {
    const std::string source = R"sp(
        import std.string;

        fn main() {
            string text = "  Hello, SP!  ";
            string trimmed = text.trim();
            println("Trimmed: '${trimmed}'");
            println("Upper: '${trimmed.upper()}'");
            println("Lower: '${trimmed.lower()}'");
            println("Length: ${text.length()}");

            string spSub = "SP";
            println("Contains SP: ${trimmed.contains(spSub)}");
            string rep = trimmed.replace("SP", "World");
            println("Replaced: '${rep}'");

            string csv = "one,two,three";
            string delim = ",";
            string[] items = csv.split(delim);
            println("Split count: ${items.length()}");
            println("First: ${items[0]}");
            println("Last: ${items[2]}");

            string word = "Language";
            println("Reversed: ${word.reverse()}");
            string pad = "AB";
            println("Repeat: ${pad.repeat(3)}");

            string author = "shanto";
            println("Via String.upper: ${String.upper(author)}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_string");
    EXPECT_NE(output.find("Trimmed: 'Hello, SP!'"), std::string::npos);
    EXPECT_NE(output.find("Upper: 'HELLO, SP!'"), std::string::npos);
    EXPECT_NE(output.find("Lower: 'hello, sp!'"), std::string::npos);
    EXPECT_NE(output.find("Length: 14"), std::string::npos);
    EXPECT_NE(output.find("Contains SP: true"), std::string::npos);
    EXPECT_NE(output.find("Replaced: 'Hello, World!'"), std::string::npos);
    EXPECT_NE(output.find("Split count: 3"), std::string::npos);
    EXPECT_NE(output.find("First: one"), std::string::npos);
    EXPECT_NE(output.find("Last: three"), std::string::npos);
    EXPECT_NE(output.find("Reversed: egaugnaL"), std::string::npos);
    EXPECT_NE(output.find("Repeat: ABABAB"), std::string::npos);
    EXPECT_NE(output.find("Via String.upper: SHANTO"), std::string::npos);
}

TEST(ExecutionTest, StdlibMathNativeExecution) {
    const std::string source = R"sp(
        import std.math;

        fn main() {
            println("PI: ${PI}");
            println("E: ${E}");
            println("TAU: ${TAU}");
            println("Math.PI: ${Math.PI}");

            println("sqrt: ${sqrt(16.0)}");
            println("pow: ${pow(2.0, 4.0)}");
            int a = -25;
            println("abs_int: ${abs(a)}");
            double d = -12.5;
            println("abs_dbl: ${abs(d)}");
            println("min_int: ${min(7, 3)}");
            println("max_int: ${max(7, 3)}");
            println("min_dbl: ${min(4.5, 9.5)}");
            println("max_dbl: ${max(4.5, 9.5)}");
            println("floor: ${floor(5.8)}");
            println("ceil: ${ceil(5.2)}");
            println("round: ${round(5.5)}");
            println("sin0: ${sin(0.0)}");
            println("cos0: ${cos(0.0)}");
            println("log2: ${log2(8.0)}");
            println("log10: ${log10(100.0)}");

            println("Math.sqrt: ${Math.sqrt(49.0)}");
            println("Math.pow: ${Math.pow(3.0, 2.0)}");
            int mNeg = -77;
            println("Math.abs: ${Math.abs(mNeg)}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_math");
    EXPECT_NE(output.find("PI: 3.14159"), std::string::npos);
    EXPECT_NE(output.find("E: 2.71828"), std::string::npos);
    EXPECT_NE(output.find("TAU: 6.28318"), std::string::npos);
    EXPECT_NE(output.find("Math.PI: 3.14159"), std::string::npos);
    EXPECT_NE(output.find("sqrt: 4"), std::string::npos);
    EXPECT_NE(output.find("pow: 16"), std::string::npos);
    EXPECT_NE(output.find("abs_int: 25"), std::string::npos);
    EXPECT_NE(output.find("abs_dbl: 12.5"), std::string::npos);
    EXPECT_NE(output.find("min_int: 3"), std::string::npos);
    EXPECT_NE(output.find("max_int: 7"), std::string::npos);
    EXPECT_NE(output.find("min_dbl: 4.5"), std::string::npos);
    EXPECT_NE(output.find("max_dbl: 9.5"), std::string::npos);
    EXPECT_NE(output.find("floor: 5"), std::string::npos);
    EXPECT_NE(output.find("ceil: 6"), std::string::npos);
    EXPECT_NE(output.find("round: 6"), std::string::npos);
    EXPECT_NE(output.find("sin0: 0"), std::string::npos);
    EXPECT_NE(output.find("cos0: 1"), std::string::npos);
    EXPECT_NE(output.find("log2: 3"), std::string::npos);
    EXPECT_NE(output.find("log10: 2"), std::string::npos);
    EXPECT_NE(output.find("Math.sqrt: 7"), std::string::npos);
    EXPECT_NE(output.find("Math.pow: 9"), std::string::npos);
    EXPECT_NE(output.find("Math.abs: 77"), std::string::npos);
}




TEST(ExecutionTest, StdlibCollectionsNativeExecution) {
    const std::string source = R"sp(
        import std.collections;

        fn main() {
            List<int> numbers = new List<int>();
            println("list_empty_1: ${numbers.isEmpty()}");
            numbers.push(10);
            numbers.push(20);
            numbers.push(30);
            println("list_len: ${numbers.length()}");
            println("list_has_20: ${numbers.contains(20)}");
            println("list_has_99: ${numbers.contains(99)}");
            println("list_get_1: ${numbers.get(1)}");
            numbers.set(1, 25);
            println("list_get_1_after_set: ${numbers.get(1)}");
            numbers.removeAt(0);
            println("list_after_remove: len=${numbers.length()}, first=${numbers.get(0)}");
            numbers.clear();
            println("list_empty_2: ${numbers.isEmpty()}");

            Map<string, int> scores = new Map<string, int>();
            println("map_empty_1: ${scores.isEmpty()}");
            scores.set("Alice", 95);
            scores.set("Bob", 88);
            scores.set("Charlie", 72);
            string alice = "Alice";
            string bob = "Bob";
            string david = "David";
            println("map_len: ${scores.length()}");
            println("map_has_alice: ${scores.contains(alice)}");
            println("map_has_david: ${scores.containsKey(david)}");
            println("map_get_alice: ${scores.get(alice)}");
            scores.remove(bob);
            println("map_after_remove_bob: len=${scores.length()}, has_bob=${scores.contains(bob)}");
            scores.clear();
            println("map_empty_2: ${scores.isEmpty()}");

            Set<int> uniqueNums = new Set<int>();
            println("set_empty_1: ${uniqueNums.isEmpty()}");
            uniqueNums.add(5);
            uniqueNums.add(10);
            uniqueNums.add(5);
            println("set_len: ${uniqueNums.length()}");
            println("set_has_5: ${uniqueNums.contains(5)}");
            println("set_has_15: ${uniqueNums.contains(15)}");
            uniqueNums.remove(5);
            println("set_after_remove_5: has_5=${uniqueNums.contains(5)}");
            uniqueNums.clear();
            println("set_empty_2: ${uniqueNums.isEmpty()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_collections");
    EXPECT_NE(output.find("list_empty_1: true"), std::string::npos);
    EXPECT_NE(output.find("list_len: 3"), std::string::npos);
    EXPECT_NE(output.find("list_has_20: true"), std::string::npos);
    EXPECT_NE(output.find("list_has_99: false"), std::string::npos);
    EXPECT_NE(output.find("list_get_1: 20"), std::string::npos);
    EXPECT_NE(output.find("list_get_1_after_set: 25"), std::string::npos);
    EXPECT_NE(output.find("list_after_remove: len=2, first=25"), std::string::npos);
    EXPECT_NE(output.find("list_empty_2: true"), std::string::npos);

    EXPECT_NE(output.find("map_empty_1: true"), std::string::npos);
    EXPECT_NE(output.find("map_len: 3"), std::string::npos);
    EXPECT_NE(output.find("map_has_alice: true"), std::string::npos);
    EXPECT_NE(output.find("map_has_david: false"), std::string::npos);
    EXPECT_NE(output.find("map_get_alice: 95"), std::string::npos);
    EXPECT_NE(output.find("map_after_remove_bob: len=2, has_bob=false"), std::string::npos);
    EXPECT_NE(output.find("map_empty_2: true"), std::string::npos);

    EXPECT_NE(output.find("set_empty_1: true"), std::string::npos);
    EXPECT_NE(output.find("set_len: 2"), std::string::npos);
    EXPECT_NE(output.find("set_has_5: true"), std::string::npos);
    EXPECT_NE(output.find("set_has_15: false"), std::string::npos);
    EXPECT_NE(output.find("set_after_remove_5: has_5=false"), std::string::npos);
    EXPECT_NE(output.find("set_empty_2: true"), std::string::npos);
}

TEST(ExecutionTest, StdlibFsNativeExecution) {
    const std::string source = R"sp(
        import std.fs;

        fn main() {
            string testFile = "temp_exec_fs.txt";
            string testDir = "temp_exec_dir";
            string msg1 = "Hello Filesystem!";
            string msg2 = " More content.";

            bool wOk = writeText(testFile, msg1);
            println("w_ok: ${wOk}");
            println("exists_1: ${exists(testFile)}");
            println("is_file_1: ${isFile(testFile)}");
            println("read_1: ${readText(testFile)}");

            bool appOk = appendText(testFile, msg2);
            println("app_ok: ${appOk}");
            println("read_2: ${readText(testFile)}");

            bool mkdirOk = createDir(testDir);
            println("mkdir_ok: ${mkdirOk}");
            println("dir_exists_1: ${exists(testDir)}");
            println("is_dir_1: ${isDir(testDir)}");

            string nested = "temp_exec_dir/child.txt";
            string nestedMsg = "child";
            writeText(nested, nestedMsg);
            string[] items = listDir(testDir);
            println("dir_items_len: ${items.length()}");
            println("dir_item_0: ${items[0]}");

            bool delF = delete(testFile);
            bool delD = delete(testDir);
            println("del_file: ${delF}");
            println("del_dir: ${delD}");
            println("exists_after_del: ${exists(testFile)}");
            println("dir_exists_after_del: ${exists(testDir)}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_fs");
    EXPECT_NE(output.find("w_ok: true"), std::string::npos);
    EXPECT_NE(output.find("exists_1: true"), std::string::npos);
    EXPECT_NE(output.find("is_file_1: true"), std::string::npos);
    EXPECT_NE(output.find("read_1: Hello Filesystem!"), std::string::npos);
    EXPECT_NE(output.find("app_ok: true"), std::string::npos);
    EXPECT_NE(output.find("read_2: Hello Filesystem! More content."), std::string::npos);
    EXPECT_NE(output.find("mkdir_ok: true"), std::string::npos);
    EXPECT_NE(output.find("dir_exists_1: true"), std::string::npos);
    EXPECT_NE(output.find("is_dir_1: true"), std::string::npos);
    EXPECT_NE(output.find("dir_items_len: 1"), std::string::npos);
    EXPECT_NE(output.find("dir_item_0: child.txt"), std::string::npos);
    EXPECT_NE(output.find("del_file: true"), std::string::npos);
    EXPECT_NE(output.find("del_dir: true"), std::string::npos);
    EXPECT_NE(output.find("exists_after_del: false"), std::string::npos);
    EXPECT_NE(output.find("dir_exists_after_del: false"), std::string::npos);
}

TEST(ExecutionTest, StdlibTimeNativeExecution) {
    const std::string source = R"sp(
        import std.time;

        fn main() {
            // Phase 4 slice D: epoch values are int64 (see SymbolTable.cpp
            // + stdlib/time/time.sp). `int` truncated ~1.7e12 ms to negative.
            int64 t1 = now();
            int64 tSecs = nowSeconds();
            println("now_gt_0: ${t1 > 0}");
            println("secs_gt_0: ${tSecs > 0}");

            sleep(15);
            int64 t2 = now();
            int64 elapsed = t2 - t1;
            println("elapsed_gte_10: ${elapsed >= 10}");

            string iso = formatTime(1700000000);
            println("formatted_iso: ${iso}");

            int64 parsed = parseTime("2023-11-14T22:13:20Z");
            println("parsed_epoch: ${parsed}");

            Duration d = Duration.fromSeconds(3);
            println("d_millis: ${d.toMillis()}");
            println("d_seconds: ${d.toSeconds()}");

            Duration dMin = Duration.fromMinutes(1);
            println("d_min_millis: ${dMin.toMillis()}");

            int64 staticNow = Time.now();
            println("time_static_gt_0: ${staticNow > 0}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_time");
    EXPECT_NE(output.find("now_gt_0: true"), std::string::npos);
    EXPECT_NE(output.find("secs_gt_0: true"), std::string::npos);
    EXPECT_NE(output.find("elapsed_gte_10: true"), std::string::npos);
    EXPECT_NE(output.find("formatted_iso: 2023-11-14T22:13:20Z"), std::string::npos);
    EXPECT_NE(output.find("parsed_epoch: 1700000000"), std::string::npos);
    EXPECT_NE(output.find("d_millis: 3000"), std::string::npos);
    EXPECT_NE(output.find("d_seconds: 3"), std::string::npos);
    EXPECT_NE(output.find("d_min_millis: 60000"), std::string::npos);
    EXPECT_NE(output.find("time_static_gt_0: true"), std::string::npos);
}

TEST(ExecutionTest, StdlibOsNativeExecution) {
    const std::string source = R"sp(
        import std.os;

        fn main() {
            string plat = platform();
            println("platform_len_gt_0: ${plat.length() > 0}");

            string architecture = arch();
            println("arch_len_gt_0: ${architecture.length() > 0}");

            string k = "SP_TEST_KEY";
            string v = "SP_VAL_42";
            bool sOk = setEnv(k, v);
            println("set_env_ok: ${sOk}");

            string got = getEnv(k);
            println("get_env: ${got}");

            string[] myArgs = args();
            println("args_gte_0: ${myArgs.length() >= 0}");

            println("os_static_plat: ${OS.platform().length() > 0}");
            println("os_static_arch: ${OS.arch().length() > 0}");
            println("os_static_get: ${OS.getEnv(k)}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_os");
    EXPECT_NE(output.find("platform_len_gt_0: true"), std::string::npos);
    EXPECT_NE(output.find("arch_len_gt_0: true"), std::string::npos);
    EXPECT_NE(output.find("set_env_ok: true"), std::string::npos);
    EXPECT_NE(output.find("get_env: SP_VAL_42"), std::string::npos);
    EXPECT_NE(output.find("args_gte_0: true"), std::string::npos);
    EXPECT_NE(output.find("os_static_plat: true"), std::string::npos);
    EXPECT_NE(output.find("os_static_arch: true"), std::string::npos);
    EXPECT_NE(output.find("os_static_get: SP_VAL_42"), std::string::npos);
}

TEST(ExecutionTest, StdlibProcessNativeExecution) {
#if defined(_WIN32)
    const std::string okCmd = "cmd.exe /c exit 0";
    const std::string echoCmd = "cmd.exe /c echo HelloProcess";
#else
    const std::string okCmd = "sh -c exit";
    const std::string echoCmd = "echo HelloProcess";
#endif

    const std::string source = std::string(R"sp(
        import std.process;

        fn main() {
            int curPid = pid();
            println("pid_gt_0: ${curPid > 0}");

            string okCmd = ")sp") + okCmd + R"sp(";
            int ret0 = exec(okCmd);
            println("ret_is_0: ${ret0 == 0}");

            string echoCmd = ")sp" + echoCmd + R"sp(";
            string out = runCommand(echoCmd);
            string targetSub = "HelloProcess";
            bool hasSub = out.contains(targetSub);
            println("out_contains: ${hasSub}");

            int chPid = spawn(okCmd);
            println("spawn_gt_0: ${chPid > 0}");

            int staticRet = Process.exec(okCmd);
            println("static_ret_0: ${staticRet == 0}");

            string staticOut = Process.run(echoCmd);
            bool hasStaticSub = staticOut.contains(targetSub);
            println("static_out_contains: ${hasStaticSub}");

            int staticPid = Process.pid();
            println("static_pid_gt_0: ${staticPid > 0}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_process");
    EXPECT_NE(output.find("pid_gt_0: true"), std::string::npos);
    EXPECT_NE(output.find("ret_is_0: true"), std::string::npos);
    EXPECT_NE(output.find("out_contains: true"), std::string::npos);
    EXPECT_NE(output.find("spawn_gt_0: true"), std::string::npos);
    EXPECT_NE(output.find("static_ret_0: true"), std::string::npos);
    EXPECT_NE(output.find("static_out_contains: true"), std::string::npos);
    EXPECT_NE(output.find("static_pid_gt_0: true"), std::string::npos);
}

TEST(ExecutionTest, StdlibFullIntegrationNativeExecution) {
    const std::string source = R"sp(
        import std.io;
        import std.string;
        import std.math;
        import std.collections;
        import std.fs;
        import std.time;
        import std.os;
        import std.process;

        fn main() {
            // 1. std.io
            println("Full Stdlib Integration Verified!");

            // 2. std.string
            string raw = "  sp language  ";
            string clean = raw.trim().upper();
            println("clean_str: '${clean}'");

            // 3. std.math
            double sq = sqrt(64.0);
            println("math_sqrt: ${sq}");

            // 4. std.collections
            List<int> nums = new List<int>();
            nums.push(10);
            nums.push(20);
            nums.push(30);
            println("list_len: ${nums.length()}");

            Map<string, int> ages = new Map<string, int>();
            string name = "Antigravity";
            ages.set(name, 25);
            println("map_val: ${ages.get(name)}");

            // 5. std.fs
            string dummyPath = "non_existent_test_file.sp";
            println("fs_exists: ${exists(dummyPath)}");

            // 6. std.time
            // Phase 4 slice D: epoch-ms is int64; `int` truncated it negative.
            int64 t = now();
            println("time_now_gt_0: ${t > 0}");

            // 7. std.os
            string curPlatform = platform();
            string curArch = arch();
            println("os_info: ${curPlatform.length() > 0 && curArch.length() > 0}");

            // 8. std.process
            int currentPid = pid();
            println("proc_pid_gt_0: ${currentPid > 0}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stdlib_full_integration");
    EXPECT_NE(output.find("Full Stdlib Integration Verified!"), std::string::npos);
    EXPECT_NE(output.find("clean_str: 'SP LANGUAGE'"), std::string::npos);
    EXPECT_NE(output.find("math_sqrt: 8"), std::string::npos);
    EXPECT_NE(output.find("list_len: 3"), std::string::npos);
    EXPECT_NE(output.find("map_val: 25"), std::string::npos);
    EXPECT_NE(output.find("fs_exists: false"), std::string::npos);
    EXPECT_NE(output.find("time_now_gt_0: true"), std::string::npos);
    EXPECT_NE(output.find("os_info: true"), std::string::npos);
    EXPECT_NE(output.find("proc_pid_gt_0: true"), std::string::npos);
}

TEST(ExecutionTest, ARCObjectAllocationAndLifecycle) {
    const std::string source = R"sp(
        import std.io;

        class Task {
            int id;
            int priority;

            Task(int id, int priority) {
                this.id = id;
                this.priority = priority;
            }

            fn execute() -> int {
                return this.id * this.priority;
            }
        }

        fn runJob() -> int {
            var t1 = new Task(5, 10);
            var t2 = new Task(2, 4);
            int res1 = t1.execute();
            {
                var t3 = new Task(10, 20);
                res1 = res1 + t3.execute();
            }
            return res1 + t2.execute();
        }

        fn main() {
            int result = runJob();
            println("job_result: ${result}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_arc_object_lifecycle");
    EXPECT_NE(output.find("job_result: 258"), std::string::npos);
}

TEST(ExecutionTest, WeakReferenceCycleBreakingNativeExecution) {
    const std::string source = R"sp(
        import std.io;

        class Node {
            int value;
            weak Node parent;

            Node(int v) {
                this.value = v;
            }

            fn getValue() -> int {
                return this.value;
            }
        }

        fn buildTree() -> int {
            var root = new Node(100);
            var child = new Node(200);
            child.parent = root;
            return root.getValue() + child.getValue();
        }

        fn main() {
            int total = buildTree();
            println("tree_total: ${total}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_weak_cycle_breaking");
    EXPECT_NE(output.find("tree_total: 300"), std::string::npos);
}

TEST(ExecutionTest, MemoryArenaNativeExecution) {
    const std::string source = R"sp(
        import std.io;
        import std.mem;

        fn main() {
            var arena = new Arena(1024);
            int p1 = arena.alloc(128);
            int p2 = arena.alloc(256);
            int bytes = arena.allocatedBytes();
            println("arena_allocated: ${bytes >= 384}");
            arena.reset();
            println("arena_reset: true");
            arena.destroy();
            println("arena_done: true");
            println("live_objs_gte_0: ${totalLiveObjects() >= 0}");
            println("total_mem_gte_0: ${totalAllocatedBytes() >= 0}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_memory_arena");
    EXPECT_NE(output.find("arena_allocated: true"), std::string::npos);
    EXPECT_NE(output.find("arena_reset: true"), std::string::npos);
    EXPECT_NE(output.find("arena_done: true"), std::string::npos);
    EXPECT_NE(output.find("live_objs_gte_0: true"), std::string::npos);
    EXPECT_NE(output.find("total_mem_gte_0: true"), std::string::npos);
}

TEST(ExecutionTest, StackCollectionNativeExecution) {
    const std::string source = R"sp(
        import std.collections;

        fn main() {
            var s = new Stack<int>();
            println("stack_empty_1: ${s.isEmpty()}");
            s.push(11);
            s.push(22);
            s.push(33);
            println("stack_len: ${s.size()}");
            println("stack_peek: ${s.peek()}");
            println("stack_pop_1: ${s.pop()}");
            println("stack_pop_2: ${s.pop()}");
            println("stack_len_after: ${s.size()}");
            s.clear();
            println("stack_empty_2: ${s.isEmpty()}");

            var strStack = new Stack<string>();
            strStack.push("hello");
            strStack.push("world");
            println("str_stack_pop: ${strStack.pop()}");
            println("str_stack_peek: ${strStack.peek()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_stack_collection");
    EXPECT_NE(output.find("stack_empty_1: true"), std::string::npos);
    EXPECT_NE(output.find("stack_len: 3"), std::string::npos);
    EXPECT_NE(output.find("stack_peek: 33"), std::string::npos);
    EXPECT_NE(output.find("stack_pop_1: 33"), std::string::npos);
    EXPECT_NE(output.find("stack_pop_2: 22"), std::string::npos);
    EXPECT_NE(output.find("stack_len_after: 1"), std::string::npos);
    EXPECT_NE(output.find("stack_empty_2: true"), std::string::npos);
    EXPECT_NE(output.find("str_stack_pop: world"), std::string::npos);
    EXPECT_NE(output.find("str_stack_peek: hello"), std::string::npos);
}

TEST(ExecutionTest, QueueCollectionNativeExecution) {
    const std::string source = R"sp(
        import std.collections;

        fn main() {
            var q = new Queue<int>();
            println("queue_empty_1: ${q.isEmpty()}");
            q.enqueue(100);
            q.enqueue(200);
            q.enqueue(300);
            println("queue_len: ${q.size()}");
            println("queue_peek: ${q.peek()}");
            println("queue_deq_1: ${q.dequeue()}");
            println("queue_deq_2: ${q.dequeue()}");
            println("queue_len_after: ${q.size()}");
            q.clear();
            println("queue_empty_2: ${q.isEmpty()}");

            var strQueue = new Queue<string>();
            strQueue.enqueue("first");
            strQueue.enqueue("second");
            println("str_queue_deq: ${strQueue.dequeue()}");
            println("str_queue_peek: ${strQueue.peek()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_queue_collection");
    EXPECT_NE(output.find("queue_empty_1: true"), std::string::npos);
    EXPECT_NE(output.find("queue_len: 3"), std::string::npos);
    EXPECT_NE(output.find("queue_peek: 100"), std::string::npos);
    EXPECT_NE(output.find("queue_deq_1: 100"), std::string::npos);
    EXPECT_NE(output.find("queue_deq_2: 200"), std::string::npos);
    EXPECT_NE(output.find("queue_len_after: 1"), std::string::npos);
    EXPECT_NE(output.find("queue_empty_2: true"), std::string::npos);
    EXPECT_NE(output.find("str_queue_deq: first"), std::string::npos);
    EXPECT_NE(output.find("str_queue_peek: second"), std::string::npos);
}

TEST(ExecutionTest, PriorityQueueCollectionNativeExecution) {
    const std::string source = R"sp(
        import std.collections;

        fn main() {
            var pq = new PriorityQueue<string>();
            println("pq_empty_1: ${pq.isEmpty()}");
            pq.push("low", 100);
            pq.push("highest", 1);
            pq.push("medium", 50);
            pq.push("high", 10);
            println("pq_len: ${pq.size()}");
            println("pq_peek: ${pq.peek()}");
            println("pq_peek_prio: ${pq.peekPriority()}");
            println("pq_pop_1: ${pq.pop()}");
            println("pq_pop_2: ${pq.pop()}");
            println("pq_pop_3: ${pq.pop()}");
            println("pq_pop_4: ${pq.pop()}");
            println("pq_empty_2: ${pq.isEmpty()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_pq_collection");
    EXPECT_NE(output.find("pq_empty_1: true"), std::string::npos);
    EXPECT_NE(output.find("pq_len: 4"), std::string::npos);
    EXPECT_NE(output.find("pq_peek: highest"), std::string::npos);
    EXPECT_NE(output.find("pq_peek_prio: 1"), std::string::npos);
    EXPECT_NE(output.find("pq_pop_1: highest"), std::string::npos);
    EXPECT_NE(output.find("pq_pop_2: high"), std::string::npos);
    EXPECT_NE(output.find("pq_pop_3: medium"), std::string::npos);
    EXPECT_NE(output.find("pq_pop_4: low"), std::string::npos);
    EXPECT_NE(output.find("pq_empty_2: true"), std::string::npos);
}

TEST(ExecutionTest, PureSPGenericCollectionsNativeExecution) {
    const std::string source = R"sp(
        class ArrayList<T> {
            var backend: List<T>;

            fn ArrayList() {
                this.backend = new List<T>();
            }

            fn add(val: T) {
                this.backend.push(val);
            }

            fn get(index: int) -> T {
                return this.backend.get(index);
            }

            fn set(index: int, val: T) {
                this.backend.set(index, val);
            }

            fn pop() -> T {
                return this.backend.pop();
            }

            fn size() -> int {
                return this.backend.size();
            }

            fn contains(val: T) -> bool {
                return this.backend.contains(val);
            }
        }

        class HashMap<K, V> {
            var backend: Map<K, V>;

            fn HashMap() {
                this.backend = new Map<K, V>();
            }

            fn set(key: K, val: V) {
                this.backend.set(key, val);
            }

            fn get(key: K) -> V {
                return this.backend.get(key);
            }

            fn contains(key: K) -> bool {
                return this.backend.contains(key);
            }

            fn remove(key: K) {
                this.backend.remove(key);
            }

            fn size() -> int {
                return this.backend.size();
            }
        }

        class HashSet<T> {
            var backend: Set<T>;

            fn HashSet() {
                this.backend = new Set<T>();
            }

            fn add(val: T) {
                this.backend.add(val);
            }

            fn contains(val: T) -> bool {
                return this.backend.contains(val);
            }

            fn remove(val: T) {
                this.backend.remove(val);
            }

            fn size() -> int {
                return this.backend.size();
            }
        }

        fn main() {
            var list = new ArrayList<int>();
            list.add(10);
            list.add(20);
            list.add(30);
            println("arrlist_size: ${list.size()}");
            println("arrlist_get_1: ${list.get(1)}");
            list.set(1, 99);
            println("arrlist_get_1_after: ${list.get(1)}");
            println("arrlist_has_99: ${list.contains(99)}");
            println("arrlist_pop: ${list.pop()}");
            println("arrlist_size_after: ${list.size()}");

            var map = new HashMap<string, int>();
            string alice = "Alice";
            string bob = "Bob";
            map.set(alice, 95);
            map.set(bob, 88);
            println("hashmap_size: ${map.size()}");
            println("hashmap_get_alice: ${map.get(alice)}");
            println("hashmap_has_bob: ${map.contains(bob)}");
            map.remove(bob);
            println("hashmap_has_bob_after: ${map.contains(bob)}");

            var set = new HashSet<int>();
            set.add(42);
            set.add(42);
            set.add(77);
            println("hashset_size: ${set.size()}");
            println("hashset_has_42: ${set.contains(42)}");
            set.remove(42);
            println("hashset_has_42_after: ${set.contains(42)}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_pure_sp_collections");
    EXPECT_NE(output.find("arrlist_size: 3"), std::string::npos);
    EXPECT_NE(output.find("arrlist_get_1: 20"), std::string::npos);
    EXPECT_NE(output.find("arrlist_get_1_after: 99"), std::string::npos);
    EXPECT_NE(output.find("arrlist_has_99: true"), std::string::npos);
    EXPECT_NE(output.find("arrlist_pop: 30"), std::string::npos);
    EXPECT_NE(output.find("arrlist_size_after: 2"), std::string::npos);
    EXPECT_NE(output.find("hashmap_size: 2"), std::string::npos);
    EXPECT_NE(output.find("hashmap_get_alice: 95"), std::string::npos);
    EXPECT_NE(output.find("hashmap_has_bob: true"), std::string::npos);
    EXPECT_NE(output.find("hashmap_has_bob_after: false"), std::string::npos);
    EXPECT_NE(output.find("hashset_size: 2"), std::string::npos);
    EXPECT_NE(output.find("hashset_has_42: true"), std::string::npos);
    EXPECT_NE(output.find("hashset_has_42_after: false"), std::string::npos);
}

TEST(ExecutionTest, CollectionStressAndScalingNativeExecution) {
    const std::string source = R"sp(
        import std.collections;

        fn main() {
            var list = new List<int>();
            var count = 10000;
            for (var i = 0; i < count; i = i + 1) {
                list.push(i);
            }
            println("stress_list_size: ${list.length()}");
            println("stress_list_first: ${list.get(0)}");
            println("stress_list_mid: ${list.get(5000)}");
            println("stress_list_last: ${list.get(9999)}");

            var map = new Map<int, int>();
            for (var i = 0; i < 2000; i = i + 1) {
                map.set(i, i * 2);
            }
            println("stress_map_size: ${map.length()}");
            println("stress_map_0: ${map.get(0)}");
            println("stress_map_1000: ${map.get(1000)}");
            println("stress_map_1999: ${map.get(1999)}");
            println("stress_map_has_500: ${map.contains(500)}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_collection_stress");
    EXPECT_NE(output.find("stress_list_size: 10000"), std::string::npos);
    EXPECT_NE(output.find("stress_list_first: 0"), std::string::npos);
    EXPECT_NE(output.find("stress_list_mid: 5000"), std::string::npos);
    EXPECT_NE(output.find("stress_list_last: 9999"), std::string::npos);
    EXPECT_NE(output.find("stress_map_size: 2000"), std::string::npos);
    EXPECT_NE(output.find("stress_map_0: 0"), std::string::npos);
    EXPECT_NE(output.find("stress_map_1000: 2000"), std::string::npos);
    EXPECT_NE(output.find("stress_map_1999: 3998"), std::string::npos);
    EXPECT_NE(output.find("stress_map_has_500: true"), std::string::npos);
}

TEST(ExecutionTest, ResultAndOptionZeroAllocTaggedUnionExecution) {
    const std::string source = R"sp(
        fn divide(int a, int b) -> Result<int, string> {
            if (b == 0) {
                return Result.Err("division by zero");
            }
            return Result.Ok(a / b);
        }

        fn findPositive(int n) -> Option<int> {
            if (n > 0) {
                return Option.Some(n * 2);
            }
            return Option.None();
        }

        fn main() {
            var okRes = divide(20, 4);
            println("okRes.isOk: ${okRes.isOk()}");
            println("okRes.unwrap: ${okRes.unwrap()}");

            var errRes = divide(10, 0);
            println("errRes.isErr: ${errRes.isErr()}");
            println("errRes.unwrapErr: ${errRes.unwrapErr()}");
            println("errRes.unwrapOr: ${errRes.unwrapOr(999)}");

            var someOpt = findPositive(5);
            println("someOpt.isSome: ${someOpt.isSome()}");
            println("someOpt.unwrap: ${someOpt.unwrap()}");

            var noneOpt = findPositive(-3);
            println("noneOpt.isNone: ${noneOpt.isNone()}");
            println("noneOpt.unwrapOr: ${noneOpt.unwrapOr(-1)}");

            var directOk = Ok(42);
            println("directOk.unwrap: ${directOk.unwrap()}");

            var directSome = Some(77);
            println("directSome.unwrap: ${directSome.unwrap()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_result_option");
    EXPECT_NE(output.find("okRes.isOk: true"), std::string::npos);
    EXPECT_NE(output.find("okRes.unwrap: 5"), std::string::npos);
    EXPECT_NE(output.find("errRes.isErr: true"), std::string::npos);
    EXPECT_NE(output.find("errRes.unwrapErr: division by zero"), std::string::npos);
    EXPECT_NE(output.find("errRes.unwrapOr: 999"), std::string::npos);
    EXPECT_NE(output.find("someOpt.isSome: true"), std::string::npos);
    EXPECT_NE(output.find("someOpt.unwrap: 10"), std::string::npos);
    EXPECT_NE(output.find("noneOpt.isNone: true"), std::string::npos);
    EXPECT_NE(output.find("noneOpt.unwrapOr: -1"), std::string::npos);
    EXPECT_NE(output.find("directOk.unwrap: 42"), std::string::npos);
    EXPECT_NE(output.find("directSome.unwrap: 77"), std::string::npos);
}

TEST(ExecutionTest, TryOperatorPropagationNativeExecution) {
    const std::string source = R"sp(
        fn stepOk(x: int): Result<int, string> {
            return Ok(x * 2);
        }

        fn stepFail(msg: string): Result<int, string> {
            return Err(msg);
        }

        fn pipelineOk(a: int, b: int): Result<int, string> {
            var v1 = stepOk(a)?;
            var v2 = stepOk(b)?;
            return Ok(v1 + v2);
        }

        fn pipelineFail(a: int): Result<int, string> {
            var v1 = stepOk(a)?;
            var v2 = stepFail("pipeline broke")?;
            return Ok(v1 + v2);
        }

        fn optDouble(x: int): Option<int> {
            if (x > 0) {
                return Some(x * 2);
            }
            return None;
        }

        fn pipelineOpt(x: int, y: int): Option<int> {
            var a = optDouble(x)?;
            var b = optDouble(y)?;
            return Some(a + b);
        }

        fn main() {
            var res1 = pipelineOk(10, 5);
            println("pipelineOk.isOk: ${res1.isOk()}");
            println("pipelineOk.unwrap: ${res1.unwrap()}");

            var res2 = pipelineFail(20);
            println("pipelineFail.isErr: ${res2.isErr()}");
            println("pipelineFail.unwrapErr: ${res2.unwrapErr()}");

            var opt1 = pipelineOpt(3, 7);
            println("opt1.isSome: ${opt1.isSome()}");
            println("opt1.unwrap: ${opt1.unwrap()}");

            var opt2 = pipelineOpt(3, -1);
            println("opt2.isNone: ${opt2.isNone()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_try_operator");
    EXPECT_NE(output.find("pipelineOk.isOk: true"), std::string::npos);
    EXPECT_NE(output.find("pipelineOk.unwrap: 30"), std::string::npos);
    EXPECT_NE(output.find("pipelineFail.isErr: true"), std::string::npos);
    EXPECT_NE(output.find("pipelineFail.unwrapErr: pipeline broke"), std::string::npos);
    EXPECT_NE(output.find("opt1.isSome: true"), std::string::npos);
    EXPECT_NE(output.find("opt1.unwrap: 20"), std::string::npos);
    EXPECT_NE(output.find("opt2.isNone: true"), std::string::npos);
}

TEST(ExecutionTest, RawPointersAndUnsafeBlocksNativeExecution) {
    const std::string source = R"sp(
        #[repr(C)]
        struct Point {
            x: int;
            y: int;
        }

        unsafe fn addThroughPointer(p: ptr<int>, val: int) {
            *p = *p + val;
        }

        fn main() {
            var num: int = 50;

            unsafe {
                var pNum: ptr<int> = &num;
                println("initial *pNum: ${*pNum}");

                *pNum = 100;
                println("after *pNum=100: ${num}");

                addThroughPointer(pNum, 25);
                println("after addThroughPointer: ${num}");

                pNum[0] = 200;
                println("after pNum[0]=200: ${pNum[0]}");
            }

            println("final num: ${num}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_raw_pointers");
    EXPECT_NE(output.find("initial *pNum: 50"), std::string::npos);
    EXPECT_NE(output.find("after *pNum=100: 100"), std::string::npos);
    EXPECT_NE(output.find("after addThroughPointer: 125"), std::string::npos);
    EXPECT_NE(output.find("after pNum[0]=200: 200"), std::string::npos);
    EXPECT_NE(output.find("final num: 200"), std::string::npos);
}

TEST(ExecutionTest, ExternCFunctionCallsNativeExecution) {
    const std::string source = R"sp(
        extern "C" {
            fn abs(n: int): int;
        }

        fn main() {
            var val: int = -42;
            var res: int = abs(val);
            println("abs(-42) = ${res}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_extern_c");
    EXPECT_NE(output.find("abs(-42) = 42"), std::string::npos);
}

TEST(ExecutionTest, StdlibFsAndIoResultErrorHandlingNativeExecution) {
    const std::string source = R"sp(
        import std.fs;

        fn readFileContent(path: string) -> Result<string, IOError> {
            let content: string = tryReadText(path)?;
            return Result.Ok(content);
        }

        fn main() {
            string testFile = "temp_fs_result_test.txt";
            string testContent = "SP 1.4 Result Handling!";

            // 1. Reading non-existent file returns Err(NotFound)
            let errRes: Result<string, IOError> = readFileContent("non_existent_file_xyz.txt");
            println("err_is_err: ${errRes.isErr()}");

            // 2. Writing text returns Ok
            let writeRes: Result<bool, IOError> = tryWriteText(testFile, testContent);
            println("write_is_ok: ${writeRes.isOk()}");

            // 3. Reading with try operator ? on existing file
            let okRes: Result<string, IOError> = readFileContent(testFile);
            println("read_is_ok: ${okRes.isOk()}");
            println("read_content: ${okRes.unwrap()}");

            // Clean up
            let delRes: Result<bool, IOError> = tryDelete(testFile);
            println("del_is_ok: ${delRes.isOk()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_fs_result");
    EXPECT_NE(output.find("err_is_err: true"), std::string::npos);
    EXPECT_NE(output.find("write_is_ok: true"), std::string::npos);
    EXPECT_NE(output.find("read_is_ok: true"), std::string::npos);
    EXPECT_NE(output.find("read_content: SP 1.4 Result Handling!"), std::string::npos);
    EXPECT_NE(output.find("del_is_ok: true"), std::string::npos);
}

TEST(ExecutionTest, ConcurrencyMutexAndAtomicsExecution) {
    const std::string source = R"sp(
        fn main() {
            var mtx = new Mutex();
            mtx.lock();
            println("mutex_locked: true");
            mtx.unlock();
            println("mutex_unlocked: true");

            var atom = new AtomicInt(100);
            println("atom_initial: ${atom.get()}");
            atom.add(50);
            println("atom_after_add: ${atom.get()}");
            atom.sub(20);
            println("atom_after_sub: ${atom.get()}");
            atom.increment();
            println("atom_after_inc: ${atom.get()}");
            var casOk = atom.compareAndSwap(131, 200);
            println("atom_cas_ok: ${casOk}");
            println("atom_final: ${atom.get()}");

            var b = new AtomicBool(false);
            println("bool_initial: ${b.get()}");
            b.set(true);
            println("bool_after_set: ${b.get()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_conc_atomics");
    EXPECT_NE(output.find("mutex_locked: true"), std::string::npos);
    EXPECT_NE(output.find("mutex_unlocked: true"), std::string::npos);
    EXPECT_NE(output.find("atom_initial: 100"), std::string::npos);
    EXPECT_NE(output.find("atom_after_add: 150"), std::string::npos);
    EXPECT_NE(output.find("atom_after_sub: 130"), std::string::npos);
    EXPECT_NE(output.find("atom_after_inc: 131"), std::string::npos);
    EXPECT_NE(output.find("atom_cas_ok: true"), std::string::npos);
    EXPECT_NE(output.find("atom_final: 200"), std::string::npos);
    EXPECT_NE(output.find("bool_initial: false"), std::string::npos);
    EXPECT_NE(output.find("bool_after_set: true"), std::string::npos);
}

TEST(ExecutionTest, ConcurrencyChannelSendRecvExecution) {
    const std::string source = R"sp(
        fn main() {
            var chan = new Channel<int>(10);
            println("chan_cap: ${chan.capacity()}");
            println("chan_initial_size: ${chan.size()}");

            chan.send(42);
            chan.send(100);
            chan.send(2026);
            println("chan_size: ${chan.size()}");

            var v1 = chan.recv();
            var v2 = chan.recv();
            var v3 = chan.recv();
            println("recv_1: ${v1}");
            println("recv_2: ${v2}");
            println("recv_3: ${v3}");
            println("chan_empty_size: ${chan.size()}");

            chan.close();
            println("chan_closed: ${chan.isClosed()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_conc_channel");
    EXPECT_NE(output.find("chan_cap: 10"), std::string::npos);
    EXPECT_NE(output.find("chan_initial_size: 0"), std::string::npos);
    EXPECT_NE(output.find("chan_size: 3"), std::string::npos);
    EXPECT_NE(output.find("recv_1: 42"), std::string::npos);
    EXPECT_NE(output.find("recv_2: 100"), std::string::npos);
    EXPECT_NE(output.find("recv_3: 2026"), std::string::npos);
    EXPECT_NE(output.find("chan_empty_size: 0"), std::string::npos);
    EXPECT_NE(output.find("chan_closed: true"), std::string::npos);
}

TEST(ExecutionTest, ConcurrencyThreadStaticMethodsExecution) {
    const std::string source = R"sp(
        fn main() {
            var cores = Thread.hardwareConcurrency();
            var tid = Thread.id();
            println("has_cores: ${cores > 0}");
            println("has_tid: ${tid != 0}");
            Thread.yield();
            Thread.sleep(1);
            println("thread_yield_sleep_ok: true");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_conc_thread");
    EXPECT_NE(output.find("has_cores: true"), std::string::npos);
    EXPECT_NE(output.find("has_tid: true"), std::string::npos);
    EXPECT_NE(output.find("thread_yield_sleep_ok: true"), std::string::npos);
}

TEST(ExecutionTest, CryptoSha256AndUuidExecution) {
    const std::string source = R"sp(
        extern "C" {
            fn sp_rt_crypto_sha256(string data) -> string;
            fn sp_rt_crypto_uuid() -> string;
        }

        fn main() {
            var hash = sp_rt_crypto_sha256("hello");
            println("sha256_hello: ${hash}");

            var id = sp_rt_crypto_uuid();
            println("uuid_len: ${id.length()}");
        }
    )sp";

    std::string output = runSPProgram(source, "exec_crypto_sha256");
    EXPECT_NE(output.find("sha256_hello: 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824"), std::string::npos);
    EXPECT_NE(output.find("uuid_len: 36"), std::string::npos);
}

TEST(ExecutionTest, JsonParseAndStringifyExecution) {
    const std::string source = R"sp(
        extern "C" {
            fn sp_rt_json_parse(string jsonStr) -> ptr;
            fn sp_rt_json_stringify(ptr handle, int indent) -> string;
            fn sp_rt_json_obj_get(ptr handle, string key) -> ptr;
            fn sp_rt_json_get_string(ptr handle) -> string;
            fn sp_rt_json_get_int(ptr handle) -> int64;
            fn sp_rt_json_destroy(ptr handle);
        }

        fn main() {
            var root = sp_rt_json_parse("{\"app\":\"SP Cloud\",\"port\":8080}");
            var appVal = sp_rt_json_obj_get(root, "app");
            var portVal = sp_rt_json_obj_get(root, "port");

            var appName = sp_rt_json_get_string(appVal);
            var portNum = sp_rt_json_get_int(portVal);

            println("app_name: ${appName}");
            println("app_port: ${portNum}");

            var outJson = sp_rt_json_stringify(root, 0);
            println("serialized: ${outJson}");

            sp_rt_json_destroy(appVal);
            sp_rt_json_destroy(portVal);
            sp_rt_json_destroy(root);
        }
    )sp";

    std::string output = runSPProgram(source, "exec_json_parse");
    EXPECT_NE(output.find("app_name: SP Cloud"), std::string::npos);
    EXPECT_NE(output.find("app_port: 8080"), std::string::npos);
    EXPECT_NE(output.find("serialized: {\"app\":\"SP Cloud\",\"port\":8080}"), std::string::npos);
}

TEST(ExecutionTest, NetTcpSocketEchoExecution) {
    const std::string source = R"sp(
        extern "C" {
            fn sp_rt_tcp_listen(string host, int port, int backlog) -> ptr;
            fn sp_rt_tcp_listener_port(ptr listener) -> int;
            fn sp_rt_tcp_connect(string host, int port, int timeoutMs) -> ptr;
            fn sp_rt_tcp_accept(ptr listener) -> ptr;
            fn sp_rt_tcp_write(ptr sock, string str) -> int;
            fn sp_rt_tcp_read(ptr sock, int maxBytes) -> string;
            fn sp_rt_tcp_close(ptr sock);
            fn sp_rt_tcp_listener_close(ptr listener);
        }

        fn main() {
            var listener = sp_rt_tcp_listen("127.0.0.1", 0, 10);
            var port = sp_rt_tcp_listener_port(listener);
            println("listening_port_gt_zero: ${port > 0}");

            var client = sp_rt_tcp_connect("127.0.0.1", port, 2000);
            var serverConn = sp_rt_tcp_accept(listener);

            sp_rt_tcp_write(client, "Hello TCP!");
            var received = sp_rt_tcp_read(serverConn, 128);
            println("server_received: ${received}");

            sp_rt_tcp_write(serverConn, "Echo: " + received);
            var echoed = sp_rt_tcp_read(client, 128);
            println("client_echoed: ${echoed}");

            sp_rt_tcp_close(client);
            sp_rt_tcp_close(serverConn);
            sp_rt_tcp_listener_close(listener);
        }
    )sp";

    std::string output = runSPProgram(source, "exec_net_tcp");
    EXPECT_NE(output.find("listening_port_gt_zero: true"), std::string::npos);
    EXPECT_NE(output.find("server_received: Hello TCP!"), std::string::npos);
    EXPECT_NE(output.find("client_echoed: Echo: Hello TCP!"), std::string::npos);
}




