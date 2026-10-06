// SP Annotations & Metadata Demonstration
// SP 1.0 supports metadata attributes via class-based decorators and documentation annotations.

class RouteAttribute {
    string path;
    string method;

    RouteAttribute(string path, string method) {
        this.path = path;
        this.method = method;
    }
}

class MathService {
    // Annotation: @inline
    // Annotation: @pure
    fn add(int a, int b) -> int {
        return a + b;
    }

    // Annotation: @Test
    fn testAdd() -> bool {
        return this.add(2, 3) == 5;
    }
}

fn main() {
    println("=== Annotations & Metadata Demo ===");

    RouteAttribute route = new RouteAttribute("/users", "GET");
    println("Route: " + route.method + " " + route.path);

    MathService math = new MathService();
    int sum = math.add(2, 3);
    println("2 + 3 = " + sum);
    println("Test passed: " + math.testAdd());
}
