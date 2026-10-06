#include <stdio.h>
#include <stdbool.h>

bool isPrime(int n) {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    int i = 5;
    while (i * i <= n) {
        if (n % i == 0 || n % (i + 2) == 0) return false;
        i += 6;
    }
    return true;
}

int main(void) {
    int limit = 100000;
    int count = 0;

    for (int num = 2; num <= limit; num++) {
        if (isPrime(num)) {
            count++;
        }
    }

    printf("Primes up to %d: %d\n", limit, count);
    return 0;
}
