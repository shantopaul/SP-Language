fn main() {
    // ═══════════════════════════════════════
    //  CLASSIC FOR LOOP (C/Java style)
    // ═══════════════════════════════════════

    println("=== Classic For ===");
    for (int i = 1; i <= 5; i++) {
        println("Count: ${i}");
    }

    // ═══════════════════════════════════════
    //  RANGE LOOP (Modern SP)
    // ═══════════════════════════════════════

    println("\n=== Range Loop ===");
    for (i in 0..5) {
        print("${i} ");
    }
    println("");

    // ═══════════════════════════════════════
    //  FOR-EACH (iterate collections)
    // ═══════════════════════════════════════

    println("\n=== For-Each ===");
    let fruits = ["Apple", "Banana", "Cherry", "Date"];

    for (string fruit in fruits) {
        println("🍎 ${fruit}");
    }

    // ═══════════════════════════════════════
    //  NESTED LOOPS
    // ═══════════════════════════════════════

    println("\n=== Multiplication Table (1-5) ===");
    for (int i = 1; i <= 5; i++) {
        for (int j = 1; j <= 5; j++) {
            print("${i * j}\t");
        }
        println("");
    }

    // ═══════════════════════════════════════
    //  BREAK & CONTINUE
    // ═══════════════════════════════════════

    println("\n=== Break at 5 ===");
    for (int i = 1; i <= 10; i++) {
        if (i == 5) break;
        print("${i} ");
    }
    println("");

    println("\n=== Skip even numbers ===");
    for (int i = 1; i <= 10; i++) {
        if (i % 2 == 0) continue;
        print("${i} ");
    }
    println("");

    // ═══════════════════════════════════════
    //  COUNTDOWN
    // ═══════════════════════════════════════

    println("\n=== Countdown ===");
    for (int i = 5; i >= 1; i--) {
        println("${i}...");
    }
    println("🚀 Liftoff!");

    // ═══════════════════════════════════════
    //  PATTERN: Sum of numbers
    // ═══════════════════════════════════════

    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum += i;
    }
    println("\nSum of 1-100: ${sum}");
}
