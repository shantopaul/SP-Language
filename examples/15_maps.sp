import std.collections;

fn main() {
    Map<string, int> ages = new Map<string, int>();
    string shanto = "Shanto";
    string alice = "Alice";
    string bob = "Bob";
    string charlie = "Charlie";

    ages.set(shanto, 21);
    ages.set(alice, 25);
    ages.set(bob, 30);

    println("Shanto's age: ${ages.get(shanto)}");

    ages.set(charlie, 28);
    ages.set(shanto, 22);
    ages.remove(bob);

    if (ages.containsKey(alice)) {
        println("Alice is ${ages.get(alice)} years old");
    }

    println("Total entries: ${ages.length()}");
}
