import std.io;

fn fibonacci(int n) -> int {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn main() {
    println("Fibonacci Series (First 10 numbers):");
    for (int i = 0; i < 10; i++) {
        println("fib(${i}) = ${fibonacci(i)}");
    }
}
