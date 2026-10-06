#include <stdio.h>

long long fib(int n) {
    if (n <= 1) return n;
    return fib(n - 1) + fib(n - 2);
}

int main(void) {
    int n = 35;
    long long res = fib(n);
    printf("fib(35) = %lld\n", res);
    return 0;
}
