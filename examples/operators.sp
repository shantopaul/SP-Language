// SP Programming Language - Operators
// From SP-Syntax-Guide.md Section 5: Operators

fn main() {
    // ═══════════════════════════════════════
    //  ARITHMETIC
    // ═══════════════════════════════════════

    int a = 20;
    int b = 7;

    println("${a} + ${b} = ${a + b}");     // 27
    println("${a} - ${b} = ${a - b}");     // 13
    println("${a} * ${b} = ${a * b}");     // 140
    println("${a} / ${b} = ${a / b}");     // 2  (integer division)
    println("${a} % ${b} = ${a % b}");     // 6  (remainder)

    // ═══════════════════════════════════════
    //  COMPARISON
    // ═══════════════════════════════════════

    println("${a} == ${b} → ${a == b}");   // false
    println("${a} != ${b} → ${a != b}");   // true
    println("${a} > ${b}  → ${a > b}");    // true
    println("${a} < ${b}  → ${a < b}");    // false
    println("${a} >= ${b} → ${a >= b}");   // true
    println("${a} <= ${b} → ${a <= b}");   // false

    // ═══════════════════════════════════════
    //  LOGICAL
    // ═══════════════════════════════════════

    bool x = true;
    bool y = false;

    println("${x} && ${y} → ${x && y}");   // false
    println("${x} || ${y} → ${x || y}");   // true
    println("!${x}        → ${!x}");       // false

    // ═══════════════════════════════════════
    //  ASSIGNMENT
    // ═══════════════════════════════════════

    int n = 10;
    n += 5;    println("n += 5  → ${n}");   // 15
    n -= 3;    println("n -= 3  → ${n}");   // 12
    n *= 2;    println("n *= 2  → ${n}");   // 24
    n /= 4;    println("n /= 4  → ${n}");   // 6
    n %= 4;    println("n %= 4  → ${n}");   // 2

    // ═══════════════════════════════════════
    //  INCREMENT / DECREMENT
    // ═══════════════════════════════════════

    int counter = 0;
    counter++;   println("counter++ → ${counter}");  // 1
    counter++;   println("counter++ → ${counter}");  // 2
    counter--;   println("counter-- → ${counter}");  // 1

    // ═══════════════════════════════════════
    //  BITWISE
    // ═══════════════════════════════════════

    int p = 0b1100;   // 12
    int q = 0b1010;   // 10

    println("p & q  = ${p & q}");    // 8  (1000)
    println("p | q  = ${p | q}");    // 14 (1110)
    println("p ^ q  = ${p ^ q}");    // 6  (0110)
    println("~p     = ${~p}");       // -13
    println("p << 1 = ${p << 1}");   // 24
    println("p >> 1 = ${p >> 1}");   // 6
}
