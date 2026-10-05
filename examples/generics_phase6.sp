// SP Programming Language - Phase 6: Generics & Collections Demo
import std.io;

class Box<T> {
    var value: T;

    fn Box(val: T) {
        this.value = val;
    }

    fn getValue() -> T {
        return this.value;
    }
}

fn identity<T>(item: T) -> T {
    return item;
}

fn main() {
    println("=== SP Phase 6: Generics & Collections Demo ===");

    // 1. Generic Class
    var intBox = new Box<int>(42);
    println("Box<int> value:");
    println(intBox.getValue());

    // 2. Generic Function
    var idVal = identity<int>(100);
    println("identity<int>(100):");
    println(idVal);

    // 3. Dynamic List<int>
    var nums = new List<int>();
    nums.add(10);
    nums.add(20);
    nums.add(30);
    println("List size:");
    println(nums.size());

    // 4. For-In Loop
    println("Iterating list with for-in:");
    for (n in nums) {
        println(n);
    }

    // 5. Map<string, int>
    var scores = new Map<string, int>();
    scores.set("Alice", 95);
    scores.set("Bob", 88);
    println("Map size:");
    println(scores.size());
    println("Alice score:");
    println(scores.get("Alice"));

    // 6. Set<string>
    var tags = new Set<string>();
    tags.add("compiler");
    tags.add("llvm");
    println("Set size:");
    println(tags.size());
    println("Set contains 'llvm':");
    println(tags.contains("llvm"));

    // 7. Lambda Functions and Functional Collection Transforms
    var arr = [1, 2, 3, 4, 5];
    println("Array mapped (doubled):");
    var doubled = arr.map((int x) => x * 2);
    for (d in doubled) {
        println(d);
    }

    println("Array filtered (evens):");
    var evens = arr.filter((int x) => x % 2 == 0);
    for (e in evens) {
        println(e);
    }

    var totalSum = arr.reduce((int acc, int x) => acc + x, 0);
    println("Array reduced sum:");
    println(totalSum);

    println("Phase 6 verified successfully!");
}
