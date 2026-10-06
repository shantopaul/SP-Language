import std.collections;

fn main() {
    println("--- std.collections: List<int> ---");
    List<int> numbers = new List<int>();
    println("Initial isEmpty: ${numbers.isEmpty()}");
    numbers.push(10);
    numbers.push(20);
    numbers.push(30);
    println("List size: ${numbers.length()}");
    println("Contains 20: ${numbers.contains(20)}");
    println("Contains 99: ${numbers.contains(99)}");
    println("Item at 1: ${numbers.get(1)}");
    numbers.set(1, 25);
    println("Updated at 1: ${numbers.get(1)}");
    numbers.removeAt(0);
    println("After removeAt(0), size: ${numbers.length()}, first: ${numbers.get(0)}");
    numbers.clear();
    println("After clear, isEmpty: ${numbers.isEmpty()}");

    println("--- std.collections: Map<string, int> ---");
    Map<string, int> scores = new Map<string, int>();
    println("Initial map isEmpty: ${scores.isEmpty()}");
    scores.set("Alice", 95);
    scores.set("Bob", 88);
    scores.set("Charlie", 72);
    string alice = "Alice";
    string bob = "Bob";
    string david = "David";
    println("Scores count: ${scores.length()}");
    println("Contains Alice: ${scores.contains(alice)}");
    println("Contains David: ${scores.containsKey(david)}");
    println("Alice score: ${scores.get(alice)}");
    scores.remove(bob);
    println("After remove Bob, count: ${scores.length()}, contains Bob: ${scores.contains(bob)}");
    scores.clear();
    println("After map clear, isEmpty: ${scores.isEmpty()}");

    println("--- std.collections: Set<int> ---");
    Set<int> uniqueNums = new Set<int>();
    println("Initial set isEmpty: ${uniqueNums.isEmpty()}");
    uniqueNums.add(5);
    uniqueNums.add(10);
    uniqueNums.add(5); // Duplicate
    println("Set size: ${uniqueNums.length()}");
    println("Contains 5: ${uniqueNums.contains(5)}");
    println("Contains 15: ${uniqueNums.contains(15)}");
    uniqueNums.remove(5);
    println("After remove 5, contains 5: ${uniqueNums.contains(5)}");
    uniqueNums.clear();
    println("After set clear, isEmpty: ${uniqueNums.isEmpty()}");

    println("std.collections complete!");
}
