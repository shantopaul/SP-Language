// SP Fibonacci Benchmark (Recursive)
fn fib(n: int) -> int {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

fn main() -> int {
    let n: int = 35;
    let res: int = fib(n);
    println("fib(35) = ${res}");
    return 0;
}
