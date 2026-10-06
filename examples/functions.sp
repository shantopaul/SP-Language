// ═══════════════════════════════════════════════════
//  SP-NATIVE SYNTAX:  fn name(params) -> returnType
// ═══════════════════════════════════════════════════

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

// ═══════════════════════════════════════════════════
//  JAVA/C++ STYLE:  returnType name(params)
// ═══════════════════════════════════════════════════

int multiply(int a, int b) {
    return a * b;
}

string formatAge(string name, int age) {
    return "${name} is ${age} years old";
}

void sayGoodbye(string name) {
    println("Goodbye, ${name}!");
}

// ═══════════════════════════════════════════════════
//  SHORT FORM:  fn name(params) => expression
// ═══════════════════════════════════════════════════

fn square(int x) => x * x;
fn cube(int x) => x * x * x;
fn max(int a, int b) => a > b ? a : b;
fn min(int a, int b) => a < b ? a : b;
fn isEven(int n) => n % 2 == 0;
fn doubleNum(int n) => n * 2;

// ═══════════════════════════════════════════════════
//  FUNCTIONS WITH DEFAULT PARAMETERS
// ═══════════════════════════════════════════════════

fn createUser(string name, int age, string role = "user") -> string {
    return "${name} (${age}) - ${role}";
}

// ═══════════════════════════════════════════════════
//  RECURSIVE FUNCTIONS
// ═══════════════════════════════════════════════════

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
    // 1. Basic Functions
    int sum = add(10, 20);
    println("Sum: ${sum}");           // Sum: 30

    greet("Shanto");                  // Hello, Shanto!

    println(isAdult(21));             // true
    println(isAdult(15));             // false

    println(getFullName("Shanto", "Paul"));  // Shanto Paul

    // 2. Java/C++ Style Functions
    println(multiply(5, 4));                    // 20
    println(formatAge("Shanto", 21));           // Shanto is 21 years old
    sayGoodbye("World");                        // Goodbye, World!

    // 3. Short Form Functions
    println("5² = ${square(5)}");       // 25
    println("3³ = ${cube(3)}");         // 27
    println("max(3,7) = ${max(3, 7)}"); // 7
    println("min(3,7) = ${min(3, 7)}"); // 3
    println("isEven(4) = ${isEven(4)}");// true
    println("double(8) = ${doubleNum(8)}");// 16

    // 4. Default Parameters
    println(createUser("Shanto", 21));            // Shanto (21) - user
    println(createUser("Admin", 30, "admin"));    // Admin (30) - admin

    // 5. Recursive Functions
    println("5! = ${factorial(5)}");       // 120
    println("10! = ${factorial(10)}");     // 3628800

    println("\nFibonacci sequence:");
    for (int i = 0; i < 10; i++) {
        print("${fibonacci(i)} ");
    }
    println("");
}
