import std.math;

fn main() {
    println("--- std.math Constants ---");
    println("PI: ${PI}");
    println("E: ${E}");
    println("TAU: ${TAU}");
    println("Math.PI: ${Math.PI}");

    println("--- Basic Math Functions ---");
    double root = sqrt(16.0);
    println("sqrt(16.0) = ${root}");

    double powered = pow(2.0, 8.0);
    println("pow(2.0, 8.0) = ${powered}");

    int negInt = -42;
    int absI = abs(negInt);
    println("abs(-42) = ${absI}");

    double negDbl = -99.5;
    double absD = abs(negDbl);
    println("abs(-99.5) = ${absD}");

    int minVal = min(10, 20);
    int maxVal = max(10, 20);
    println("min(10, 20) = ${minVal}, max(10, 20) = ${maxVal}");

    double minD = min(3.14, 2.71);
    double maxD = max(3.14, 2.71);
    println("min(3.14, 2.71) = ${minD}, max(3.14, 2.71) = ${maxD}");

    println("--- Rounding Functions ---");
    double fl = floor(3.7);
    double cl = ceil(3.2);
    double rd = round(3.5);
    println("floor(3.7) = ${fl}");
    println("ceil(3.2) = ${cl}");
    println("round(3.5) = ${rd}");

    println("--- Trigonometric Functions ---");
    double sin0 = sin(0.0);
    double cos0 = cos(0.0);
    println("sin(0.0) = ${sin0}");
    println("cos(0.0) = ${cos0}");

    println("--- Log & Exp Functions ---");
    double l2 = log2(8.0);
    double l10 = log10(100.0);
    double ex = exp(1.0);
    println("log2(8.0) = ${l2}");
    println("log10(100.0) = ${l10}");
    println("exp(1.0) = ${ex}");

    println("--- Math Static Class ---");
    double mSqrt = Math.sqrt(25.0);
    double mPow = Math.pow(3.0, 3.0);
    int mAbs = Math.abs(-123);
    println("Math.sqrt(25.0) = ${mSqrt}");
    println("Math.pow(3.0, 3.0) = ${mPow}");
    println("Math.abs(-123) = ${mAbs}");

    println("std.math complete!");
}
