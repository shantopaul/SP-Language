class Vector2 {
    double x;
    double y;

    Vector2(double x, double y) {
        this.x = x;
        this.y = y;
    }

    fn add(Vector2 other) -> Vector2 {
        return new Vector2(this.x + other.x, this.y + other.y);
    }

    fn sub(Vector2 other) -> Vector2 {
        return new Vector2(this.x - other.x, this.y - other.y);
    }

    fn scale(double scalar) -> Vector2 {
        return new Vector2(this.x * scalar, this.y * scalar);
    }

    fn equals(Vector2 other) -> bool {
        return this.x == other.x && this.y == other.y;
    }
}

fn main() {
    println("=== Vector Arithmetic (Operator Overloading Patterns) ===");

    Vector2 a = new Vector2(3.0, 4.0);
    Vector2 b = new Vector2(1.0, 2.0);

    Vector2 sum = a.add(b);
    Vector2 diff = a.sub(b);
    Vector2 scaled = a.scale(2.0);
    bool eq = a.equals(b);

    println("a + b = (" + sum.x + ", " + sum.y + ")");
    println("a - b = (" + diff.x + ", " + diff.y + ")");
    println("a * 2 = (" + scaled.x + ", " + scaled.y + ")");
    println("a == b: " + eq);
}
