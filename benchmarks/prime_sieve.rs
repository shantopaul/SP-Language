fn is_prime(n: i32) -> bool {
    if n <= 1 { return false; }
    if n <= 3 { return true; }
    if n % 2 == 0 || n % 3 == 0 { return false; }

    let mut i = 5;
    while i * i <= n {
        if n % i == 0 || n % (i + 2) == 0 { return false; }
        i += 6;
    }
    true
}

fn main() {
    let limit = 100000;
    let mut count = 0;

    for num in 2..=limit {
        if is_prime(num) {
            count += 1;
        }
    }

    println!("Primes up to {}: {}", limit, count);
}
