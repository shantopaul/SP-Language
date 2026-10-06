// SP-Native Syntax
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

// Java/C++ Style
int multiply(int a, int b) {
    return a * b;
}

string formatAge(string name, int age) {
    return "${name} is ${age} years old";
}

void sayGoodbye(string name) {
    println("Goodbye, ${name}!");
}

// Short form
fn square(int x) => x * x;
fn cube(int x) => x * x * x;
fn max(int a, int b) => a > b ? a : b;
fn min(int a, int b) => a < b ? a : b;
fn isEven(int n) => n % 2 == 0;
fn dbl(int n) => n * 2;

// Default parameter
fn createUser(string name, int age, string role = "user") -> string {
    return "${name} (${age}) - ${role}";
}

// Recursion
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
    println("double(8) = ${dbl(8)}");

    println(createUser("Shanto", 21));
    println(createUser("Admin", 30, "admin"));

    println("5! = ${factorial(5)}");
    println("10! = ${factorial(10)}");

    println("\nFibonacci sequence:");
    for (int i = 0; i < 10; i++) {
        print("${fibonacci(i)} ");
    }
    println("");
}
