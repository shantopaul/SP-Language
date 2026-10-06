fn main() {
    println("=== Lambda Functions Demo ===");

    int[] numbers = [1, 2, 3, 4, 5];

    println("Original numbers: [1, 2, 3, 4, 5]");

    int[] doubled = numbers.map((int x) => x * 2);
    for (int d in doubled) {
        println("Doubled: " + d);
    }

    int[] evens = numbers.filter((int x) => x % 2 == 0);
    for (int e in evens) {
        println("Even: " + e);
    }

    int sum = numbers.reduce((int a, int b) => a + b);
    println("Sum of elements: " + sum);
}
