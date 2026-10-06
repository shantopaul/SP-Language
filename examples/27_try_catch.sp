fn divide(int a, int b) -> int {
    if (b == 0) {
        println("Error: division by zero!");
        return 0;
    }
    return a / b;
}

fn main() {
    println("=== Error Handling (Try / Catch / Finally) ===");

    try {
        println("Attempting calculation...");
        int res = divide(100, 5);
        println("Result: " + res);
    } catch (Exception e) {
        println("Caught error occurred");
    } finally {
        println("Cleanup complete: execution finished safely.");
    }

    try {
        println("Attempting invalid calculation...");
        int zeroRes = divide(50, 0);
        println("Result: " + zeroRes);
    } catch (Exception e) {
        println("Handling divide error");
    } finally {
        println("Final cleanup done.");
    }
}
