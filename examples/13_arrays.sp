fn main() {
    int[] numbers = {10, 20, 30, 40, 50};
    string[] names = {"Alice", "Bob", "Charlie"};

    let scores = [95, 87, 92, 78, 88];

    println("First: ${numbers[0]}");      // 10
    println("Last: ${numbers[4]}");       // 50

    numbers[0] = 100;
    println("Updated first: ${numbers[0]}");  // 100

    println("\nAll names:");
    for (string name in names) {
        println("  - ${name}");
    }

    println("\nAll scores:");
    for (int i = 0; i < scores.length(); i++) {
        println("  Student ${i + 1}: ${scores[i]}");
    }

    println("\nLength: ${scores.length()}");

    int maxScore = scores[0];
    for (int s in scores) {
        if (s > maxScore) maxScore = s;
    }
    println("Highest score: ${maxScore}");

    int total = 0;
    for (int s in scores) {
        total += s;
    }
    println("Total: ${total}");
    println("Average: ${total / scores.length()}");
}
