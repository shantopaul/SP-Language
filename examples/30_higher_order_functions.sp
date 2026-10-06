fn main() {
    println("=== Higher-Order Functions Demo ===");

    int[] nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    // Chaining map -> filter -> reduce
    int[] squares = nums.map((int x) => x * x);
    int[] filtered = squares.filter((int x) => x > 20);
    int result = filtered.reduce((int a, int b) => a + b);

    println("Original numbers: 1 to 10");
    println("Squares > 20 summed: " + result);

    int[] smallList = [2, 4, 6, 8];
    int product = smallList.reduce((int a, int b) => a * b, 1);
    println("Product of [2, 4, 6, 8]: " + product);
}
