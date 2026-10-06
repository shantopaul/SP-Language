module std.math;

public const double PI = 3.141592653589793;
public const double E  = 2.718281828459045;
public const double TAU = 6.283185307179586;

extern "C" {
    fn sp_rt_math_sqrt(double x) -> double;
    fn sp_rt_math_pow(double base, double exp) -> double;
    fn sp_rt_math_sin(double x) -> double;
    fn sp_rt_math_cos(double x) -> double;
    fn sp_rt_math_tan(double x) -> double;
    fn sp_rt_math_asin(double x) -> double;
    fn sp_rt_math_acos(double x) -> double;
    fn sp_rt_math_atan(double x) -> double;
    fn sp_rt_math_atan2(double y, double x) -> double;
    fn sp_rt_math_floor(double x) -> double;
    fn sp_rt_math_ceil(double x) -> double;
    fn sp_rt_math_round(double x) -> double;
    fn sp_rt_math_abs_double(double x) -> double;
    fn sp_rt_math_abs_int(int x) -> int;
    fn sp_rt_math_min_double(double a, double b) -> double;
    fn sp_rt_math_min_int(int a, int b) -> int;
    fn sp_rt_math_max_double(double a, double b) -> double;
    fn sp_rt_math_max_int(int a, int b) -> int;
    fn sp_rt_math_log(double x) -> double;
    fn sp_rt_math_log10(double x) -> double;
    fn sp_rt_math_log2(double x) -> double;
    fn sp_rt_math_exp(double x) -> double;
}

public fn sqrt(double x) -> double {
    return sp_rt_math_sqrt(x);
}

public fn pow(double base, double exp) -> double {
    return sp_rt_math_pow(base, exp);
}

public fn sin(double x) -> double {
    return sp_rt_math_sin(x);
}

public fn cos(double x) -> double {
    return sp_rt_math_cos(x);
}

public fn tan(double x) -> double {
    return sp_rt_math_tan(x);
}

public fn asin(double x) -> double {
    return sp_rt_math_asin(x);
}

public fn acos(double x) -> double {
    return sp_rt_math_acos(x);
}

public fn atan(double x) -> double {
    return sp_rt_math_atan(x);
}

public fn atan2(double y, double x) -> double {
    return sp_rt_math_atan2(y, x);
}

public fn floor(double x) -> double {
    return sp_rt_math_floor(x);
}

public fn ceil(double x) -> double {
    return sp_rt_math_ceil(x);
}

public fn round(double x) -> double {
    return sp_rt_math_round(x);
}

public fn abs(int x) -> int {
    return sp_rt_math_abs_int(x);
}

public fn abs(double x) -> double {
    return sp_rt_math_abs_double(x);
}

public fn min(int a, int b) -> int {
    return sp_rt_math_min_int(a, b);
}

public fn min(double a, double b) -> double {
    return sp_rt_math_min_double(a, b);
}

public fn max(int a, int b) -> int {
    return sp_rt_math_max_int(a, b);
}

public fn max(double a, double b) -> double {
    return sp_rt_math_max_double(a, b);
}

public fn log(double x) -> double {
    return sp_rt_math_log(x);
}

public fn log10(double x) -> double {
    return sp_rt_math_log10(x);
}

public fn log2(double x) -> double {
    return sp_rt_math_log2(x);
}

public fn exp(double x) -> double {
    return sp_rt_math_exp(x);
}

public class Math {
    public static fn sqrt(double x) -> double {
        return sp_rt_math_sqrt(x);
    }

    public static fn pow(double base, double exp) -> double {
        return sp_rt_math_pow(base, exp);
    }

    public static fn sin(double x) -> double {
        return sp_rt_math_sin(x);
    }

    public static fn cos(double x) -> double {
        return sp_rt_math_cos(x);
    }

    public static fn tan(double x) -> double {
        return sp_rt_math_tan(x);
    }

    public static fn asin(double x) -> double {
        return sp_rt_math_asin(x);
    }

    public static fn acos(double x) -> double {
        return sp_rt_math_acos(x);
    }

    public static fn atan(double x) -> double {
        return sp_rt_math_atan(x);
    }

    public static fn atan2(double y, double x) -> double {
        return sp_rt_math_atan2(y, x);
    }

    public static fn floor(double x) -> double {
        return sp_rt_math_floor(x);
    }

    public static fn ceil(double x) -> double {
        return sp_rt_math_ceil(x);
    }

    public static fn round(double x) -> double {
        return sp_rt_math_round(x);
    }

    public static fn abs(int x) -> int {
        return sp_rt_math_abs_int(x);
    }

    public static fn abs(double x) -> double {
        return sp_rt_math_abs_double(x);
    }

    public static fn min(int a, int b) -> int {
        return sp_rt_math_min_int(a, b);
    }

    public static fn min(double a, double b) -> double {
        return sp_rt_math_min_double(a, b);
    }

    public static fn max(int a, int b) -> int {
        return sp_rt_math_max_int(a, b);
    }

    public static fn max(double a, double b) -> double {
        return sp_rt_math_max_double(a, b);
    }

    public static fn log(double x) -> double {
        return sp_rt_math_log(x);
    }

    public static fn log10(double x) -> double {
        return sp_rt_math_log10(x);
    }

    public static fn log2(double x) -> double {
        return sp_rt_math_log2(x);
    }

    public static fn exp(double x) -> double {
        return sp_rt_math_exp(x);
    }
}
