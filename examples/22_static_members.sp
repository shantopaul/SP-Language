class MathUtils {
    static fn circleArea(double radius) -> double {
        return 3.14159265358979 * radius * radius;
    }

    static fn square(int x) -> int {
        return x * x;
    }

    static fn power(int base, int exp) -> int {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result = result * base;
        }
        return result;
    }
}

fn main() {
    println("Circle area (r=5): " + MathUtils.circleArea(5.0));
    println("5^2 = " + MathUtils.square(5));
    println("2^10 = " + MathUtils.power(2, 10));
}
