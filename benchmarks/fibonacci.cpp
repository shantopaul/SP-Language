#include <iostream>

long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main() {
    int n = 35;
    long long res = fib(n);
    std::cout << "fib(35) = " << res << "\n";
    return 0;
}
