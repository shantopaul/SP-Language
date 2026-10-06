function fib(n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

function main() {
    const n = 35;
    const res = fib(n);
    console.log(`fib(35) = ${res}`);
}

main();
