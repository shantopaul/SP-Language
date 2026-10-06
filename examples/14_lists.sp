import std.collections;

fn main() {
    List<string> fruits = new List<string>();
    fruits.push("Apple");
    fruits.push("Banana");
    fruits.push("Cherry");
    fruits.push("Date");
    fruits.push("Elderberry");

    string firstFruit = fruits.get(0);
    println("First fruit: ${firstFruit}");
    println("List size: ${fruits.length()}");

    fruits.push("Fig");
    fruits.removeAt(3);
    fruits.set(1, "Blueberry");

    println("\nFruit basket:");
    for (string fruit in fruits) {
        println("  ${fruit}");
    }

    if (fruits.contains("Cherry")) {
        println("\nWe have cherries!");
    }

    println("Is empty: ${fruits.isEmpty()}");
}
