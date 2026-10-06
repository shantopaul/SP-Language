fn main() {
    // ═══════════════════════════════════════
    //  WHILE LOOP
    // ═══════════════════════════════════════

    println("=== While Loop ===");
    int count = 1;
    while (count <= 5) {
        println("Count: ${count}");
        count++;
    }

    // ═══════════════════════════════════════
    //  DO-WHILE LOOP (runs at least once)
    // ═══════════════════════════════════════

    println("\n=== Do-While Loop ===");
    int n = 1;
    do {
        println("n = ${n}");
        n *= 2;
    } while (n <= 16);
    // 1, 2, 4, 8, 16

    // ═══════════════════════════════════════
    //  PATTERN: Find first power of 2 > 1000
    // ═══════════════════════════════════════

    int power = 1;
    int exponent = 0;
    while (power <= 1000) {
        power *= 2;
        exponent++;
    }
    println("\n2^${exponent} = ${power} (first power of 2 > 1000)");
    // 2^10 = 1024

    // ═══════════════════════════════════════
    //  PATTERN: Collatz Conjecture
    // ═══════════════════════════════════════

    println("\n=== Collatz Sequence (start: 27) ===");
    int num = 27;
    int steps = 0;
    print("${num}");
    while (num != 1) {
        if (num % 2 == 0) {
            num = num / 2;
        } else {
            num = num * 3 + 1;
        }
        print(" → ${num}");
        steps++;
    }
    println("\nReached 1 in ${steps} steps");
}
