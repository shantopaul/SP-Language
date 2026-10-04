module std.math;

public const double PI = 3.141592653589793;
public const double E  = 2.718281828459045;

public fn abs(int x) -> int {
    return x < 0 ? -x : x;
}

public fn abs(double x) -> double {
    return x < 0.0 ? -x : x;
}

public fn max(int a, int b) -> int {
    return a > b ? a : b;
}

public fn min(int a, int b) -> int {
    return a < b ? a : b;
}

public fn max(double a, double b) -> double {
    return a > b ? a : b;
}

public fn min(double a, double b) -> double {
    return a < b ? a : b;
}
