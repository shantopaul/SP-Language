// SP Prime Sieve Benchmark (Count primes up to N)

fn isPrime(n: int) -> bool {
    if (n <= 1) return false;
    if (n <= 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;

    let i: int = 5;
    while (i * i <= n) {
        if (n % i == 0 || n % (i + 2) == 0) {
            return false;
        }
        i = i + 6;
    }
    return true;
}

fn main() -> int {
    let limit: int = 100000;
    let count: int = 0;

    for (int num = 2; num <= limit; num++) {
        if (isPrime(num)) {
            count++;
        }
    }

    println("Primes up to ${limit}: ${count}");
    return 0;
}
