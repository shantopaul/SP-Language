<div align="center">

# SP Language — Syntax Guide & Code Showcase

### Every SP syntax feature demonstrated with real code examples

*Learn SP in one file — from Hello World to advanced patterns*

</div>

---

## Table of Contents

- [x] [1. Hello World](#1-hello-world) ✅
- [x] [2. Variables & Constants](#2-variables--constants) ✅
- [x] [3. Data Types](#3-data-types) ✅
- [x] [4. Type Inference](#4-type-inference) ✅
- [x] [5. Operators](#5-operators) ✅
- [x] [6. Strings & Interpolation](#6-strings--interpolation) ✅
- [x] [7. Console I/O](#7-console-io) ✅
- [x] [8. Functions](#8-functions) ✅
- [x] [9. If / Else / Else-If](#9-if--else--else-if) ✅
- [x] [10. Switch & Match](#10-switch--match) ✅
- [x] [11. For Loops](#11-for-loops) ✅
- [x] [12. While & Do-While](#12-while--do-while) ✅
- [x] [13. Arrays](#13-arrays) ✅
- [x] [14. Lists](#14-lists) ✅
- [x] [15. Maps & Dictionaries](#15-maps--dictionaries) ✅
- [x] [16. Classes](#16-classes) ✅
- [x] [17. Constructors](#17-constructors) ✅
- [x] [18. Access Modifiers](#18-access-modifiers) ✅
- [x] [19. Inheritance](#19-inheritance) ✅
- [x] [20. Interfaces](#20-interfaces) ✅
- [x] [21. Abstract Classes](#21-abstract-classes) ✅
- [x] [22. Static Members](#22-static-members) ✅
- [x] [23. Structs](#23-structs) ✅
- [x] [24. Enums](#24-enums) ✅
- [x] [25. Generics](#25-generics) ✅
- [x] [26. Null Safety](#26-null-safety) ✅
- [x] [27. Error Handling — Try/Catch](#27-error-handling--trycatch) ✅
- [x] [28. Error Handling — Result Type](#28-error-handling--result-type) ✅
- [x] [29. Lambda Functions](#29-lambda-functions) ✅
- [x] [30. Higher-Order Functions](#30-higher-order-functions) ✅
- [x] [31. Closures](#31-closures) ✅
- [x] [32. String Methods](#32-string-methods) ✅
- [x] [33. Method Overloading](#33-method-overloading) ✅
- [x] [34. Operator Overloading](#34-operator-overloading) ✅
- [x] [35. Extension Methods](#35-extension-methods) ✅
- [x] [36. Traits](#36-traits) ✅
- [x] [37. Annotations](#37-annotations) ✅
- [x] [38. Modules & Imports](#38-modules--imports) ✅
- [x] [39. Async & Await](#39-async--await) ✅
- [x] [40. Threads & Concurrency](#40-threads--concurrency) ✅
- [41. Unsafe Mode](#41-unsafe-mode)
- [42. Native Interop (C FFI)](#42-native-interop-c-ffi)
- [43. Testing](#43-testing)
- [44. Comments & Documentation](#44-comments--documentation)
- [45. Multi-Syntax Compatibility](#45-multi-syntax-compatibility)
- [46. Complete Programs](#46-complete-programs)

---

## 1. Hello World ✅

The simplest SP program:

```sp
fn main() {
    println("Hello, SP!");
}
```

**Output:**
```
Hello, SP!
```

**Compile & Run:**
```bash
$ spc run hello.sp
Hello, SP!

$ spc build hello.sp
$ ./hello
Hello, SP!
```

---

## 2. Variables & Constants ✅

### Mutable Variables

```sp
fn main() {
    // Explicit types
    int age = 21;
    string name = "Shanto";
    double salary = 75000.50;
    float score = 95.5;
    bool isActive = true;
    char grade = 'A';

    println("Name: ${name}");
    println("Age: ${age}");
    println("Salary: ${salary}");
    println("Score: ${score}");
    println("Active: ${isActive}");
    println("Grade: ${grade}");

    // Variables can be reassigned
    age = 22;
    name = "Shanto Paul";
    println("Updated: ${name}, age ${age}");
}
```

**Output:**
```
Name: Shanto
Age: 21
Salary: 75000.5
Score: 95.5
Active: true
Grade: A
Updated: Shanto Paul, age 22
```

### Constants (Immutable)

```sp
fn main() {
    const int MAX_USERS = 1000;
    const double TAX_RATE = 0.15;
    const string APP_NAME = "MyApp";

    println("App: ${APP_NAME}");
    println("Max Users: ${MAX_USERS}");
    println("Tax Rate: ${TAX_RATE}");

    // ❌ This will NOT compile:
    // MAX_USERS = 2000;    // error: cannot assign to constant
}
```

---

## 3. Data Types ✅

### All Primitive Types

```sp
fn main() {
    // ═══════════════════════════════════════
    //  INTEGER TYPES
    // ═══════════════════════════════════════

    int number = 42;              // Platform default (32 or 64 bit)
    uint positive = 100;          // Unsigned integer

    int8 tiny = 127;              // -128 to 127
    int16 small = 32000;          // -32768 to 32767
    int32 medium = 2147483647;    // Standard 32-bit
    int64 large = 9223372036854;  // 64-bit

    uint8 uTiny = 255;            // 0 to 255
    uint16 uSmall = 65535;        // 0 to 65535
    uint32 uMedium = 4294967295;  // 0 to 4,294,967,295
    uint64 uLarge = 18446744073;  // 0 to 18,446,744,073,709,551,615

    // ═══════════════════════════════════════
    //  FLOATING POINT TYPES
    // ═══════════════════════════════════════

    float price = 29.99;          // 32-bit floating point
    double precise = 3.141592653589793;  // 64-bit floating point

    // ═══════════════════════════════════════
    //  OTHER TYPES
    // ═══════════════════════════════════════

    bool alive = true;            // true or false
    bool dead = false;

    char letter = 'A';            // Single character
    char symbol = '@';
    char newline = '\n';

    string text = "Hello, SP!";   // String of characters
    string empty = "";

    byte raw = 0xFF;              // Raw byte (0-255)

    // Print all
    println("int:    ${number}");
    println("float:  ${price}");
    println("double: ${precise}");
    println("bool:   ${alive}");
    println("char:   ${letter}");
    println("string: ${text}");
    println("byte:   ${raw}");
}
```

**Output:**
```
int:    42
float:  29.99
double: 3.141592653589793
bool:   true
char:   A
string: Hello, SP!
byte:   255
```

---

## 4. Type Inference ✅

```sp
fn main() {
    // ═══════════════════════════════════════
    //  var — mutable, type inferred
    // ═══════════════════════════════════════

    var age = 21;                 // inferred as int
    var name = "Shanto";          // inferred as string
    var price = 99.99;            // inferred as double
    var active = true;            // inferred as bool

    age = 22;                     // ✅ Can reassign
    name = "Shanto Paul";         // ✅ Can reassign

    // ═══════════════════════════════════════
    //  let — mutable inferred (alternative)
    // ═══════════════════════════════════════

    let count = 100;
    let message = "Hello";

    // ═══════════════════════════════════════
    //  const — immutable, type inferred
    // ═══════════════════════════════════════

    const PI = 3.14159;           // inferred as double, immutable
    const APP = "SP Lang";        // inferred as string, immutable

    // ❌ PI = 3.14;              // error: cannot assign to constant

    println("Age: ${age}");
    println("Name: ${name}");
    println("PI: ${PI}");
    println("App: ${APP}");
}
```

**Output:**
```
Age: 22
Name: Shanto Paul
PI: 3.14159
App: SP Lang
```

---

## 5. Operators ✅

```sp
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
```

---

## 6. Strings & Interpolation ✅

```sp
fn main() {
    string firstName = "Shanto";
    string lastName = "Paul";
    int age = 21;

    // ═══════════════════════════════════════
    //  CONCATENATION
    // ═══════════════════════════════════════

    string fullName = firstName + " " + lastName;
    println(fullName);   // Shanto Paul

    // ═══════════════════════════════════════
    //  STRING INTERPOLATION  ${expression}
    // ═══════════════════════════════════════

    println("Hello, ${firstName}!");
    println("Full Name: ${firstName} ${lastName}");
    println("Age next year: ${age + 1}");
    println("Name length: ${firstName.length()}");
    println("Uppercase: ${firstName.upper()}");

    // ═══════════════════════════════════════
    //  ESCAPE SEQUENCES
    // ═══════════════════════════════════════

    println("Line 1\nLine 2");           // newline
    println("Tab\there");                // tab
    println("She said \"Hello\"");       // quotes
    println("Backslash: \\");            // backslash
    println("Unicode: \u0041");          // A

    // ═══════════════════════════════════════
    //  MULTI-LINE (future)
    // ═══════════════════════════════════════

    string poem = "Roses are red,\n"
                + "Violets are blue,\n"
                + "SP is awesome,\n"
                + "And so are you.";

    println(poem);
}
```

**Output:**
```
Shanto Paul
Hello, Shanto!
Full Name: Shanto Paul
Age next year: 22
Name length: 6
Uppercase: SHANTO
Line 1
Line 2
Tab	here
She said "Hello"
Backslash: \
Unicode: A
Roses are red,
Violets are blue,
SP is awesome,
And so are you.
```

---

## 7. Console I/O ✅

```sp
import std.io;

fn main() {
    // ═══════════════════════════════════════
    //  OUTPUT
    // ═══════════════════════════════════════

    println("Hello, World!");           // Print with newline
    print("Enter name: ");             // Print without newline

    // ═══════════════════════════════════════
    //  INPUT
    // ═══════════════════════════════════════

    string name = readLine();
    println("Hello, ${name}!");

    print("Enter age: ");
    int age = int.parse(readLine());
    println("You are ${age} years old.");

    print("Enter score: ");
    double score = double.parse(readLine());
    println("Your score is ${score}");
}
```

**Terminal Session:**
```
Enter name: Shanto
Hello, Shanto!
Enter age: 21
You are 21 years old.
Enter score: 95.5
Your score is 95.5
```

---

## 8. Functions ✅

### Basic Functions

```sp
// ═══════════════════════════════════════════════════
//  SP-NATIVE SYNTAX:  fn name(params) -> returnType
// ═══════════════════════════════════════════════════

fn add(int a, int b) -> int {
    return a + b;
}

fn greet(string name) {
    println("Hello, ${name}!");
}

fn isAdult(int age) -> bool {
    return age >= 18;
}

fn getFullName(string first, string last) -> string {
    return "${first} ${last}";
}

fn main() {
    int sum = add(10, 20);
    println("Sum: ${sum}");           // Sum: 30

    greet("Shanto");                  // Hello, Shanto!

    println(isAdult(21));             // true
    println(isAdult(15));             // false

    println(getFullName("Shanto", "Paul"));  // Shanto Paul
}
```

### Java/C++ Style Functions (Also Valid!)

```sp
// ═══════════════════════════════════════════════════
//  JAVA/C++ STYLE:  returnType name(params)
// ═══════════════════════════════════════════════════

int multiply(int a, int b) {
    return a * b;
}

string formatAge(string name, int age) {
    return "${name} is ${age} years old";
}

void sayGoodbye(string name) {
    println("Goodbye, ${name}!");
}

fn main() {
    println(multiply(5, 4));                    // 20
    println(formatAge("Shanto", 21));           // Shanto is 21 years old
    sayGoodbye("World");                        // Goodbye, World!
}
```

### Short Form Functions

```sp
// ═══════════════════════════════════════════════════
//  SHORT FORM:  fn name(params) => expression
// ═══════════════════════════════════════════════════

fn square(int x) => x * x;
fn cube(int x) => x * x * x;
fn max(int a, int b) => a > b ? a : b;
fn min(int a, int b) => a < b ? a : b;
fn isEven(int n) => n % 2 == 0;
fn double(int n) => n * 2;

fn main() {
    println("5² = ${square(5)}");       // 25
    println("3³ = ${cube(3)}");         // 27
    println("max(3,7) = ${max(3, 7)}"); // 7
    println("min(3,7) = ${min(3, 7)}"); // 3
    println("isEven(4) = ${isEven(4)}");// true
    println("double(8) = ${double(8)}");// 16
}
```

### Functions with Default Parameters

```sp
fn createUser(string name, int age, string role = "user") -> string {
    return "${name} (${age}) - ${role}";
}

fn main() {
    println(createUser("Shanto", 21));            // Shanto (21) - user
    println(createUser("Admin", 30, "admin"));    // Admin (30) - admin
}
```

### Recursive Functions

```sp
fn factorial(int n) -> int {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

fn fibonacci(int n) -> int {
    if (n <= 0) return 0;
    if (n == 1) return 1;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn main() {
    println("5! = ${factorial(5)}");       // 120
    println("10! = ${factorial(10)}");     // 3628800

    println("\nFibonacci sequence:");
    for (int i = 0; i < 10; i++) {
        print("${fibonacci(i)} ");
    }
    println("");
    // 0 1 1 2 3 5 8 13 21 34
}
```

---

## 9. If / Else / Else-If ✅

```sp
fn main() {
    int age = 21;
    int score = 85;
    string role = "admin";

    // ═══════════════════════════════════════
    //  SIMPLE IF
    // ═══════════════════════════════════════

    if (age >= 18) {
        println("You are an adult.");
    }

    // ═══════════════════════════════════════
    //  IF / ELSE
    // ═══════════════════════════════════════

    if (age >= 18) {
        println("Welcome! You can enter.");
    } else {
        println("Sorry, you must be 18+.");
    }

    // ═══════════════════════════════════════
    //  IF / ELSE-IF / ELSE (Grade System)
    // ═══════════════════════════════════════

    if (score >= 90) {
        println("Grade: A  ⭐ Excellent!");
    } else if (score >= 80) {
        println("Grade: B  👍 Good job!");
    } else if (score >= 70) {
        println("Grade: C  📝 Average");
    } else if (score >= 60) {
        println("Grade: D  ⚠️ Below average");
    } else {
        println("Grade: F  ❌ Failed");
    }

    // ═══════════════════════════════════════
    //  NESTED IF
    // ═══════════════════════════════════════

    if (age >= 18) {
        if (role == "admin") {
            println("Admin access granted.");
        } else {
            println("User access granted.");
        }
    } else {
        println("Access denied.");
    }

    // ═══════════════════════════════════════
    //  LOGICAL CONDITIONS
    // ═══════════════════════════════════════

    bool hasLicense = true;

    if (age >= 18 && hasLicense) {
        println("You can drive! 🚗");
    }

    if (role == "admin" || role == "moderator") {
        println("You have elevated permissions.");
    }

    if (!hasLicense) {
        println("Get a license first.");
    }

    // ═══════════════════════════════════════
    //  TERNARY OPERATOR
    // ═══════════════════════════════════════

    string status = age >= 18 ? "Adult" : "Minor";
    println("Status: ${status}");
}
```

**Output:**
```
You are an adult.
Welcome! You can enter.
Grade: B  👍 Good job!
Admin access granted.
You can drive! 🚗
You have elevated permissions.
Status: Adult
```

---

## 10. Switch & Match ✅

### Classic Switch

```sp
fn main() {
    int day = 3;

    switch (day) {
        case 1:
            println("Monday");
            break;
        case 2:
            println("Tuesday");
            break;
        case 3:
            println("Wednesday");
            break;
        case 4:
            println("Thursday");
            break;
        case 5:
            println("Friday");
            break;
        case 6:
            println("Saturday 🎉");
            break;
        case 7:
            println("Sunday 🎉");
            break;
        default:
            println("Invalid day");
    }
}
```

### Modern Match

```sp
fn main() {
    int day = 3;

    // ═══════════════════════════════════════
    //  MATCH EXPRESSION (cleaner syntax)
    // ═══════════════════════════════════════

    match day {
        1 => println("Monday"),
        2 => println("Tuesday"),
        3 => println("Wednesday"),
        4 => println("Thursday"),
        5 => println("Friday"),
        6 => println("Saturday 🎉"),
        7 => println("Sunday 🎉"),
        _ => println("Invalid day")
    }

    // ═══════════════════════════════════════
    //  MATCH WITH STRINGS
    // ═══════════════════════════════════════

    string command = "start";

    match command {
        "start"   => println("Starting server..."),
        "stop"    => println("Stopping server..."),
        "restart" => println("Restarting server..."),
        "status"  => println("Server is running."),
        _         => println("Unknown command: ${command}")
    }

    // ═══════════════════════════════════════
    //  MATCH WITH ENUM
    // ═══════════════════════════════════════

    enum Season { Spring, Summer, Autumn, Winter }

    Season current = Season.Summer;

    match current {
        Season.Spring => println("🌸 Flowers blooming"),
        Season.Summer => println("☀️ Beach time!"),
        Season.Autumn => println("🍂 Leaves falling"),
        Season.Winter => println("❄️ Stay warm!")
    }
}
```

**Output:**
```
Wednesday
Starting server...
☀️ Beach time!
```

---

## 11. For Loops ✅

```sp
fn main() {
    // ═══════════════════════════════════════
    //  CLASSIC FOR LOOP (C/Java style)
    // ═══════════════════════════════════════

    println("=== Classic For ===");
    for (int i = 1; i <= 5; i++) {
        println("Count: ${i}");
    }

    // ═══════════════════════════════════════
    //  RANGE LOOP (Modern SP)
    // ═══════════════════════════════════════

    println("\n=== Range Loop ===");
    for (i in 0..5) {
        print("${i} ");
    }
    println("");
    // 0 1 2 3 4

    // ═══════════════════════════════════════
    //  FOR-EACH (iterate collections)
    // ═══════════════════════════════════════

    println("\n=== For-Each ===");
    let fruits = ["Apple", "Banana", "Cherry", "Date"];

    for (string fruit in fruits) {
        println("🍎 ${fruit}");
    }

    // ═══════════════════════════════════════
    //  NESTED LOOPS
    // ═══════════════════════════════════════

    println("\n=== Multiplication Table (1-5) ===");
    for (int i = 1; i <= 5; i++) {
        for (int j = 1; j <= 5; j++) {
            print("${i * j}\t");
        }
        println("");
    }

    // ═══════════════════════════════════════
    //  BREAK & CONTINUE
    // ═══════════════════════════════════════

    println("\n=== Break at 5 ===");
    for (int i = 1; i <= 10; i++) {
        if (i == 5) break;
        print("${i} ");
    }
    println("");
    // 1 2 3 4

    println("\n=== Skip even numbers ===");
    for (int i = 1; i <= 10; i++) {
        if (i % 2 == 0) continue;
        print("${i} ");
    }
    println("");
    // 1 3 5 7 9

    // ═══════════════════════════════════════
    //  COUNTDOWN
    // ═══════════════════════════════════════

    println("\n=== Countdown ===");
    for (int i = 5; i >= 1; i--) {
        println("${i}...");
    }
    println("🚀 Liftoff!");

    // ═══════════════════════════════════════
    //  PATTERN: Sum of numbers
    // ═══════════════════════════════════════

    int sum = 0;
    for (int i = 1; i <= 100; i++) {
        sum += i;
    }
    println("\nSum of 1-100: ${sum}");   // 5050
}
```

**Output:**
```
=== Classic For ===
Count: 1
Count: 2
Count: 3
Count: 4
Count: 5

=== Range Loop ===
0 1 2 3 4

=== For-Each ===
🍎 Apple
🍎 Banana
🍎 Cherry
🍎 Date

=== Multiplication Table (1-5) ===
1	2	3	4	5
2	4	6	8	10
3	6	9	12	15
4	8	12	16	20
5	10	15	20	25

=== Break at 5 ===
1 2 3 4

=== Skip even numbers ===
1 3 5 7 9

=== Countdown ===
5...
4...
3...
2...
1...
🚀 Liftoff!

Sum of 1-100: 5050
```

---

## 12. While & Do-While ✅

```sp
fn main() {
    // ═══════════════════════════════════════
    //  WHILE LOOP
    // ═══════════════════════════════════════

    println("=== While Loop ===");
    int count = 1;
    while (count <= 5) {
        println("Count: ${count}");
        count++;
    }

    // ═══════════════════════════════════════
    //  DO-WHILE LOOP (runs at least once)
    // ═══════════════════════════════════════

    println("\n=== Do-While Loop ===");
    int n = 1;
    do {
        println("n = ${n}");
        n *= 2;
    } while (n <= 16);
    // 1, 2, 4, 8, 16

    // ═══════════════════════════════════════
    //  PATTERN: Find first power of 2 > 1000
    // ═══════════════════════════════════════

    int power = 1;
    int exponent = 0;
    while (power <= 1000) {
        power *= 2;
        exponent++;
    }
    println("\n2^${exponent} = ${power} (first power of 2 > 1000)");
    // 2^10 = 1024

    // ═══════════════════════════════════════
    //  PATTERN: Collatz Conjecture
    // ═══════════════════════════════════════

    println("\n=== Collatz Sequence (start: 27) ===");
    int num = 27;
    int steps = 0;
    print("${num}");
    while (num != 1) {
        if (num % 2 == 0) {
            num = num / 2;
        } else {
            num = num * 3 + 1;
        }
        print(" → ${num}");
        steps++;
    }
    println("\nReached 1 in ${steps} steps");
}
```

---

## 13. Arrays ✅

```sp
fn main() {
    // ═══════════════════════════════════════
    //  DECLARATION
    // ═══════════════════════════════════════

    int[] numbers = {10, 20, 30, 40, 50};
    string[] names = {"Alice", "Bob", "Charlie"};

    // Modern syntax
    let scores = [95, 87, 92, 78, 88];

    // ═══════════════════════════════════════
    //  ACCESS & MODIFY
    // ═══════════════════════════════════════

    println("First: ${numbers[0]}");      // 10
    println("Last: ${numbers[4]}");       // 50

    numbers[0] = 100;
    println("Updated first: ${numbers[0]}");  // 100

    // ═══════════════════════════════════════
    //  ITERATE
    // ═══════════════════════════════════════

    println("\nAll names:");
    for (string name in names) {
        println("  - ${name}");
    }

    println("\nAll scores:");
    for (int i = 0; i < scores.length(); i++) {
        println("  Student ${i + 1}: ${scores[i]}");
    }

    // ═══════════════════════════════════════
    //  ARRAY OPERATIONS
    // ═══════════════════════════════════════

    println("\nLength: ${scores.length()}");

    // Find max
    int maxScore = scores[0];
    for (int s in scores) {
        if (s > maxScore) maxScore = s;
    }
    println("Highest score: ${maxScore}");

    // Sum
    int total = 0;
    for (int s in scores) {
        total += s;
    }
    println("Total: ${total}");
    println("Average: ${total / scores.length()}");
}
```

**Output:**
```
First: 10
Last: 50
Updated first: 100

All names:
  - Alice
  - Bob
  - Charlie

All scores:
  Student 1: 95
  Student 2: 87
  Student 3: 92
  Student 4: 78
  Student 5: 88

Length: 5
Highest score: 95
Total: 440
Average: 88
```

---

## 14. Lists ✅

```sp
import std.collections;

fn main() {
    // ═══════════════════════════════════════
    //  CREATE LIST
    // ═══════════════════════════════════════

    List<string> fruits = new List<string>();
    fruits.add("Apple");
    fruits.add("Banana");
    fruits.add("Cherry");
    fruits.add("Date");
    fruits.add("Elderberry");

    // Modern syntax
    let numbers = [10, 20, 30, 40, 50];

    // ═══════════════════════════════════════
    //  ACCESS
    // ═══════════════════════════════════════

    println("First fruit: ${fruits[0]}");     // Apple
    println("List size: ${fruits.size()}");    // 5

    // ═══════════════════════════════════════
    //  MODIFY
    // ═══════════════════════════════════════

    fruits.push("Fig");                       // Add to end
    fruits.remove("Date");                    // Remove by value
    fruits[1] = "Blueberry";                  // Replace

    // ═══════════════════════════════════════
    //  ITERATE
    // ═══════════════════════════════════════

    println("\nFruit basket:");
    for (string fruit in fruits) {
        println("  🍎 ${fruit}");
    }

    // ═══════════════════════════════════════
    //  CHECK
    // ═══════════════════════════════════════

    if (fruits.contains("Cherry")) {
        println("\nWe have cherries! 🍒");
    }

    println("Is empty: ${fruits.isEmpty()}");   // false

    // ═══════════════════════════════════════
    //  FUNCTIONAL OPERATIONS
    // ═══════════════════════════════════════

    let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];

    let evens = nums.filter(n => n % 2 == 0);
    let doubled = nums.map(n => n * 2);
    let sum = nums.reduce((a, b) => a + b);

    println("\nOriginal: ${nums}");
    println("Evens: ${evens}");       // [2, 4, 6, 8, 10]
    println("Doubled: ${doubled}");   // [2, 4, 6, 8, 10, 12, 14, 16, 18, 20]
    println("Sum: ${sum}");           // 55
}
```

---

## 15. Maps & Dictionaries ✅

```sp
import std.collections;

fn main() {
    // ═══════════════════════════════════════
    //  CREATE MAP
    // ═══════════════════════════════════════

    Map<string, int> ages = new Map<string, int>();
    ages["Shanto"] = 21;
    ages["Alice"] = 25;
    ages["Bob"] = 30;

    // Modern literal syntax
    let config = {
        "host": "localhost",
        "port": 8080,
        "debug": true
    };

    let scores = {
        "math": 95,
        "science": 87,
        "english": 92
    };

    // ═══════════════════════════════════════
    //  ACCESS
    // ═══════════════════════════════════════

    println("Shanto's age: ${ages["Shanto"]}");   // 21
    println("Math score: ${scores["math"]}");      // 95

    // ═══════════════════════════════════════
    //  MODIFY
    // ═══════════════════════════════════════

    ages["Charlie"] = 28;          // Add new entry
    ages["Shanto"] = 22;           // Update existing
    ages.remove("Bob");            // Remove entry

    // ═══════════════════════════════════════
    //  CHECK
    // ═══════════════════════════════════════

    if (ages.containsKey("Alice")) {
        println("Alice is ${ages["Alice"]} years old");
    }

    println("Total entries: ${ages.size()}");

    // ═══════════════════════════════════════
    //  ITERATE
    // ═══════════════════════════════════════

    println("\nAll ages:");
    for (entry in ages) {
        println("  ${entry.key}: ${entry.value}");
    }

    println("\nAll scores:");
    for (entry in scores) {
        println("  📚 ${entry.key} → ${entry.value}");
    }
}
```

**Output:**
```
Shanto's age: 21
Math score: 95
Alice is 25 years old
Total entries: 3

All ages:
  Shanto: 22
  Alice: 25
  Charlie: 28

All scores:
  📚 math → 95
  📚 science → 87
  📚 english → 92
```

---

## 16. Classes ✅

```sp
class User {
    string name;
    int age;
    string email;

    // Constructor
    User(string name, int age, string email) {
        this.name = name;
        this.age = age;
        this.email = email;
    }

    // Methods
    fn introduce() {
        println("Hi! I'm ${name}, ${age} years old.");
        println("Email: ${email}");
    }

    fn isAdult() -> bool {
        return age >= 18;
    }

    fn birthday() {
        age++;
        println("🎂 Happy Birthday, ${name}! Now ${age}.");
    }
}

fn main() {
    // Create objects
    User user1 = new User("Shanto", 21, "shanto@email.com");
    User user2 = new User("Alice", 17, "alice@email.com");

    // Use methods
    user1.introduce();
    println("Is adult: ${user1.isAdult()}");    // true
    println("");

    user2.introduce();
    println("Is adult: ${user2.isAdult()}");    // false
    println("");

    user2.birthday();                            // 🎂 Happy Birthday, Alice! Now 18.
    println("Is adult now: ${user2.isAdult()}"); // true
}
```

**Output:**
```
Hi! I'm Shanto, 21 years old.
Email: shanto@email.com
Is adult: true

Hi! I'm Alice, 17 years old.
Email: alice@email.com
Is adult: false

🎂 Happy Birthday, Alice! Now 18.
Is adult now: true
```

---

## 17. Constructors ✅

```sp
class Product {
    string name;
    double price;
    int quantity;
    string category;

    // ═══════════════════════════════════════
    //  PRIMARY CONSTRUCTOR
    // ═══════════════════════════════════════

    constructor(string name, double price, int quantity, string category) {
        this.name = name;
        this.price = price;
        this.quantity = quantity;
        this.category = category;
    }

    // ═══════════════════════════════════════
    //  JAVA/C# STYLE CONSTRUCTOR (also valid)
    // ═══════════════════════════════════════

    Product(string name, double price) {
        this.name = name;
        this.price = price;
        this.quantity = 0;
        this.category = "General";
    }

    // Methods
    fn totalValue() -> double {
        return price * quantity;
    }

    fn display() {
        println("┌─────────────────────────────────┐");
        println("│ Product: ${name}");
        println("│ Price:   \$${price}");
        println("│ Stock:   ${quantity} units");
        println("│ Category: ${category}");
        println("│ Value:   \$${totalValue()}");
        println("└─────────────────────────────────┘");
    }
}

fn main() {
    Product laptop = new Product("MacBook Pro", 2499.99, 10, "Electronics");
    Product book = new Product("SP Language Guide", 29.99);

    laptop.display();
    println("");
    book.display();
}
```

---

## 18. Access Modifiers ✅

```sp
class BankAccount {
    // ═══════════════════════════════════════
    //  PUBLIC — accessible from anywhere
    // ═══════════════════════════════════════
    public string ownerName;

    // ═══════════════════════════════════════
    //  PRIVATE — only within this class
    // ═══════════════════════════════════════
    private double balance;
    private string accountNumber;

    // ═══════════════════════════════════════
    //  PROTECTED — this class + subclasses
    // ═══════════════════════════════════════
    protected string accountType;

    // ═══════════════════════════════════════
    //  INTERNAL — within same module only
    // ═══════════════════════════════════════
    internal string branchCode;

    public BankAccount(string owner, double initial) {
        this.ownerName = owner;
        this.balance = initial;
        this.accountNumber = "ACC-" + generateId();
        this.accountType = "Savings";
        this.branchCode = "BR-001";
    }

    // Public methods — anyone can call
    public fn deposit(double amount) {
        if (amount > 0) {
            balance += amount;
            println("✅ Deposited \$${amount}. New balance: \$${balance}");
        }
    }

    public fn withdraw(double amount) {
        if (amount > 0 && amount <= balance) {
            balance -= amount;
            println("✅ Withdrew \$${amount}. New balance: \$${balance}");
        } else {
            println("❌ Insufficient funds.");
        }
    }

    public fn getBalance() -> double {
        return balance;
    }

    // Private method — internal use only
    private fn generateId() -> string {
        return "123456789";
    }
}

fn main() {
    BankAccount account = new BankAccount("Shanto", 1000.0);

    println("Owner: ${account.ownerName}");  // ✅ public
    account.deposit(500.0);                  // ✅ public method
    account.withdraw(200.0);                 // ✅ public method
    println("Balance: \$${account.getBalance()}");

    // ❌ These would NOT compile:
    // println(account.balance);         // error: 'balance' is private
    // println(account.accountNumber);   // error: 'accountNumber' is private
    // account.generateId();             // error: 'generateId' is private
}
```

**Output:**
```
Owner: Shanto
✅ Deposited $500.0. New balance: $1500.0
✅ Withdrew $200.0. New balance: $1300.0
Balance: $1300.0
```

---

## 19. Inheritance ✅

```sp
// ═══════════════════════════════════════════════════
//  BASE CLASS
// ═══════════════════════════════════════════════════

class Animal {
    protected string name;
    protected int age;

    Animal(string name, int age) {
        this.name = name;
        this.age = age;
    }

    fn speak() {
        println("${name} makes a sound.");
    }

    fn info() {
        println("${name}, age ${age}");
    }
}

// ═══════════════════════════════════════════════════
//  SUBCLASS — extends keyword
// ═══════════════════════════════════════════════════

class Dog extends Animal {

    string breed;

    Dog(string name, int age, string breed) {
        super(name, age);
        this.breed = breed;
    }

    override fn speak() {
        println("${name} says: Woof! 🐕");
    }

    fn fetch() {
        println("${name} fetches the ball! 🎾");
    }
}

class Cat extends Animal {

    bool isIndoor;

    Cat(string name, int age, bool isIndoor) {
        super(name, age);
        this.isIndoor = isIndoor;
    }

    override fn speak() {
        println("${name} says: Meow! 🐱");
    }

    fn purr() {
        println("${name} is purring... 😺");
    }
}

// ═══════════════════════════════════════════════════
//  C# STYLE SYNTAX (also valid!)
//  class Bird : Animal { }
// ═══════════════════════════════════════════════════

class Bird : Animal {
    Bird(string name, int age) {
        super(name, age);
    }

    override fn speak() {
        println("${name} says: Tweet! 🐦");
    }
}

fn main() {
    Dog dog = new Dog("Buddy", 3, "Golden Retriever");
    Cat cat = new Cat("Whiskers", 5, true);
    Bird bird = new Bird("Tweety", 2);

    // Polymorphism
    Animal[] animals = {dog, cat, bird};

    println("=== Animal Shelter ===\n");

    for (Animal animal in animals) {
        animal.info();
        animal.speak();
        println("");
    }

    // Class-specific methods
    dog.fetch();
    cat.purr();
}
```

**Output:**
```
=== Animal Shelter ===

Buddy, age 3
Buddy says: Woof! 🐕

Whiskers, age 5
Whiskers says: Meow! 🐱

Tweety, age 2
Tweety says: Tweet! 🐦

Buddy fetches the ball! 🎾
Whiskers is purring... 😺
```

---

## 20. Interfaces ✅

```sp
// ═══════════════════════════════════════════════════
//  DEFINE INTERFACES
// ═══════════════════════════════════════════════════

interface Drawable {
    fn draw();
    fn getColor() -> string;
}

interface Resizable {
    fn resize(double factor);
    fn getArea() -> double;
}

// ═══════════════════════════════════════════════════
//  IMPLEMENT INTERFACES
// ═══════════════════════════════════════════════════

class Circle implements Drawable, Resizable {
    double radius;
    string color;

    Circle(double radius, string color) {
        this.radius = radius;
        this.color = color;
    }

    fn draw() {
        println("Drawing ⭕ Circle (r=${radius}, color=${color})");
    }

    fn getColor() -> string {
        return color;
    }

    fn resize(double factor) {
        radius *= factor;
        println("Resized circle to radius ${radius}");
    }

    fn getArea() -> double {
        return 3.14159 * radius * radius;
    }
}

class Rectangle implements Drawable, Resizable {
    double width;
    double height;
    string color;

    Rectangle(double w, double h, string color) {
        this.width = w;
        this.height = h;
        this.color = color;
    }

    fn draw() {
        println("Drawing ▬ Rectangle (${width}x${height}, color=${color})");
    }

    fn getColor() -> string {
        return color;
    }

    fn resize(double factor) {
        width *= factor;
        height *= factor;
    }

    fn getArea() -> double {
        return width * height;
    }
}

fn main() {
    Drawable[] shapes = {
        new Circle(5.0, "Red"),
        new Rectangle(4.0, 6.0, "Blue")
    };

    for (Drawable shape in shapes) {
        shape.draw();
    }

    println("\nAreas:");
    Circle c = new Circle(10.0, "Green");
    println("Circle area: ${c.getArea()}");     // 314.159

    Rectangle r = new Rectangle(5.0, 8.0, "Yellow");
    println("Rectangle area: ${r.getArea()}");  // 40.0
}
```

---

## 21. Abstract Classes ✅

```sp
abstract class Vehicle {
    protected string brand;
    protected int year;

    Vehicle(string brand, int year) {
        this.brand = brand;
        this.year = year;
    }

    // Abstract — must be implemented by subclasses
    abstract fn fuelType() -> string;
    abstract fn maxSpeed() -> int;

    // Concrete — shared by all subclasses
    fn info() {
        println("${brand} (${year}) — ${fuelType()}, max ${maxSpeed()} km/h");
    }
}

class Car extends Vehicle {
    Car(string brand, int year) { super(brand, year); }

    fn fuelType() -> string { return "Gasoline ⛽"; }
    fn maxSpeed() -> int { return 220; }
}

class ElectricCar extends Vehicle {
    ElectricCar(string brand, int year) { super(brand, year); }

    fn fuelType() -> string { return "Electric ⚡"; }
    fn maxSpeed() -> int { return 250; }
}

class Bicycle extends Vehicle {
    Bicycle(string brand, int year) { super(brand, year); }

    fn fuelType() -> string { return "Human Power 🚴"; }
    fn maxSpeed() -> int { return 45; }
}

fn main() {
    Vehicle[] vehicles = {
        new Car("Toyota", 2024),
        new ElectricCar("Tesla", 2025),
        new Bicycle("Trek", 2023)
    };

    println("=== Vehicle Fleet ===\n");
    for (Vehicle v in vehicles) {
        v.info();
    }
}
```

**Output:**
```
=== Vehicle Fleet ===

Toyota (2024) — Gasoline ⛽, max 220 km/h
Tesla (2025) — Electric ⚡, max 250 km/h
Trek (2023) — Human Power 🚴, max 45 km/h
```

---

## 22. Static Members ✅

```sp
class Counter {
    private static int count = 0;

    static fn increment() {
        count++;
    }

    static fn getCount() -> int {
        return count;
    }

    static fn reset() {
        count = 0;
    }
}

class MathUtils {
    static const double PI = 3.14159265358979;
    static const double E = 2.71828182845904;

    static fn circleArea(double radius) -> double {
        return PI * radius * radius;
    }

    static fn square(int x) -> int {
        return x * x;
    }

    static fn power(int base, int exp) -> int {
        int result = 1;
        for (int i = 0; i < exp; i++) {
            result *= base;
        }
        return result;
    }
}

fn main() {
    // Static methods — no object needed
    Counter.increment();
    Counter.increment();
    Counter.increment();
    println("Count: ${Counter.getCount()}");    // 3

    println("PI: ${MathUtils.PI}");
    println("Circle area (r=5): ${MathUtils.circleArea(5.0)}");
    println("5² = ${MathUtils.square(5)}");
    println("2^10 = ${MathUtils.power(2, 10)}");
}
```

---

## 23. Structs ✅

```sp
// ═══════════════════════════════════════════════════
//  STRUCT — lightweight value type (stack allocated)
// ═══════════════════════════════════════════════════

struct Point {
    int x;
    int y;
}

struct Color {
    int r;
    int g;
    int b;
}

struct Rectangle {
    Point topLeft;
    Point bottomRight;
    Color color;
}

fn distance(Point a, Point b) -> double {
    int dx = b.x - a.x;
    int dy = b.y - a.y;
    return math.sqrt(dx * dx + dy * dy);
}

fn main() {
    Point p1;
    p1.x = 0;
    p1.y = 0;

    Point p2;
    p2.x = 3;
    p2.y = 4;

    println("Point 1: (${p1.x}, ${p1.y})");
    println("Point 2: (${p2.x}, ${p2.y})");
    println("Distance: ${distance(p1, p2)}");  // 5.0

    Color red;
    red.r = 255;
    red.g = 0;
    red.b = 0;
    println("\nColor: rgb(${red.r}, ${red.g}, ${red.b})");
}
```

---

## 24. Enums ✅

```sp
enum Direction {
    North,
    South,
    East,
    West
}

enum HttpStatus {
    OK = 200,
    NotFound = 404,
    ServerError = 500
}

enum LogLevel {
    Debug,
    Info,
    Warning,
    Error,
    Fatal
}

fn describeDirection(Direction dir) -> string {
    match dir {
        Direction.North => return "⬆️ Going North",
        Direction.South => return "⬇️ Going South",
        Direction.East  => return "➡️ Going East",
        Direction.West  => return "⬅️ Going West"
    }
}

fn log(LogLevel level, string message) {
    match level {
        LogLevel.Debug   => println("[DEBUG]   ${message}"),
        LogLevel.Info    => println("[INFO]    ${message}"),
        LogLevel.Warning => println("[WARNING] ${message}"),
        LogLevel.Error   => println("[ERROR]   ${message}"),
        LogLevel.Fatal   => println("[FATAL]   ${message}")
    }
}

fn main() {
    Direction dir = Direction.North;
    println(describeDirection(dir));

    HttpStatus status = HttpStatus.OK;
    println("HTTP ${status}");

    println("\n=== Log Messages ===");
    log(LogLevel.Info, "Application started");
    log(LogLevel.Debug, "Loading config...");
    log(LogLevel.Warning, "Memory usage high");
    log(LogLevel.Error, "Connection failed");
}
```

**Output:**
```
⬆️ Going North
HTTP 200

=== Log Messages ===
[INFO]    Application started
[DEBUG]   Loading config...
[WARNING] Memory usage high
[ERROR]   Connection failed
```

---

## 25. Generics ✅

```sp
// ═══════════════════════════════════════════════════
//  GENERIC CLASS
// ═══════════════════════════════════════════════════

class Box<T> {
    private T value;

    Box(T value) {
        this.value = value;
    }

    fn get() -> T {
        return value;
    }

    fn set(T newValue) {
        value = newValue;
    }

    fn display() {
        println("Box contains: ${value}");
    }
}

// ═══════════════════════════════════════════════════
//  GENERIC PAIR
// ═══════════════════════════════════════════════════

class Pair<A, B> {
    A first;
    B second;

    Pair(A first, B second) {
        this.first = first;
        this.second = second;
    }

    fn display() {
        println("(${first}, ${second})");
    }
}

// ═══════════════════════════════════════════════════
//  GENERIC STACK
// ═══════════════════════════════════════════════════

class Stack<T> {
    private List<T> items = new List<T>();

    fn push(T item) {
        items.add(item);
    }

    fn pop() -> T {
        return items.removeLast();
    }

    fn peek() -> T {
        return items[items.size() - 1];
    }

    fn isEmpty() -> bool {
        return items.size() == 0;
    }

    fn size() -> int {
        return items.size();
    }
}

// ═══════════════════════════════════════════════════
//  GENERIC FUNCTIONS
// ═══════════════════════════════════════════════════

fn max<T>(T a, T b) -> T {
    return a > b ? a : b;
}

fn swap<T>(T a, T b) -> Pair<T, T> {
    return new Pair<T, T>(b, a);
}

fn main() {
    // Generic Box
    Box<int> intBox = new Box<int>(42);
    Box<string> strBox = new Box<string>("Hello SP");

    intBox.display();     // Box contains: 42
    strBox.display();     // Box contains: Hello SP

    // Generic Pair
    Pair<string, int> person = new Pair<string, int>("Shanto", 21);
    person.display();     // (Shanto, 21)

    // Generic Stack
    Stack<string> stack = new Stack<string>();
    stack.push("first");
    stack.push("second");
    stack.push("third");

    println("\nStack (LIFO):");
    while (!stack.isEmpty()) {
        println("  popped: ${stack.pop()}");
    }

    // Generic functions
    println("\nmax(10, 20) = ${max(10, 20)}");
    println("max(\"apple\", \"banana\") = ${max("apple", "banana")}");
}
```

**Output:**
```
Box contains: 42
Box contains: Hello SP
(Shanto, 21)

Stack (LIFO):
  popped: third
  popped: second
  popped: first

max(10, 20) = 20
max("apple", "banana") = banana
```

---

## 26. Null Safety ✅

```sp
class User {
    string name;
    string? email;       // Nullable — can be null

    User(string name, string? email) {
        this.name = name;
        this.email = email;
    }
}

fn findUser(int id) -> User? {
    if (id == 1) return new User("Shanto", "shanto@email.com");
    if (id == 2) return new User("Alice", null);
    return null;         // User not found
}

fn main() {
    // ═══════════════════════════════════════
    //  NULLABLE TYPES (?)
    // ═══════════════════════════════════════

    User? user1 = findUser(1);
    User? user2 = findUser(2);
    User? user3 = findUser(999);

    // ═══════════════════════════════════════
    //  SAFE ACCESS (?.)
    // ═══════════════════════════════════════

    println("User 1 name: ${user1?.name}");       // Shanto
    println("User 1 email: ${user1?.email}");      // shanto@email.com
    println("User 3 name: ${user3?.name}");        // null (no crash!)

    // ═══════════════════════════════════════
    //  NULL COALESCING (??)
    // ═══════════════════════════════════════

    string email1 = user1?.email ?? "no-email@default.com";
    string email2 = user2?.email ?? "no-email@default.com";
    string name3 = user3?.name ?? "Unknown User";

    println("\nEmail 1: ${email1}");    // shanto@email.com
    println("Email 2: ${email2}");      // no-email@default.com
    println("Name 3: ${name3}");        // Unknown User

    // ═══════════════════════════════════════
    //  NULL CHECK
    // ═══════════════════════════════════════

    if (user1 != null) {
        println("\n✅ Found user: ${user1.name}");
    }

    if (user3 == null) {
        println("❌ User not found");
    }

    // ═══════════════════════════════════════
    //  NON-NULLABLE (default — safe!)
    // ═══════════════════════════════════════

    // string name = null;    // ❌ COMPILE ERROR!
    // int age = null;        // ❌ COMPILE ERROR!
}
```

---

## 27. Error Handling — Try/Catch ✅

```sp
fn main() {
    // ═══════════════════════════════════════
    //  TRY / CATCH / FINALLY
    // ═══════════════════════════════════════

    try {
        int result = 100 / 0;
        println(result);
    } catch (ArithmeticException e) {
        println("❌ Math error: ${e.message}");
    } finally {
        println("✅ Cleanup complete.");
    }

    // ═══════════════════════════════════════
    //  MULTIPLE CATCH BLOCKS
    // ═══════════════════════════════════════

    try {
        string data = readFile("config.json");
        Config config = parseConfig(data);
        connect(config.host);
    } catch (FileNotFoundException e) {
        println("📁 File not found: ${e.message}");
    } catch (ParseException e) {
        println("📝 Invalid format: ${e.message}");
    } catch (ConnectionException e) {
        println("🌐 Connection failed: ${e.message}");
    } catch (Exception e) {
        println("💥 Unexpected error: ${e.message}");
    } finally {
        println("Done.");
    }

    // ═══════════════════════════════════════
    //  THROW EXCEPTIONS
    // ═══════════════════════════════════════

    try {
        validateAge(-5);
    } catch (Exception e) {
        println("Validation: ${e.message}");
    }
}

fn validateAge(int age) {
    if (age < 0) {
        throw Exception("Age cannot be negative: ${age}");
    }
    if (age > 150) {
        throw Exception("Age too large: ${age}");
    }
    println("Valid age: ${age}");
}
```

---

## 28. Error Handling — Result Type ✅

```sp
// ═══════════════════════════════════════════════════
//  RESULT TYPE — Modern error handling
// ═══════════════════════════════════════════════════

fn divide(int a, int b) -> Result<int, Error> {
    if (b == 0) {
        return Error("Cannot divide ${a} by zero");
    }
    return Ok(a / b);
}

fn parseNumber(string input) -> Result<int, Error> {
    try {
        return Ok(int.parse(input));
    } catch (Exception e) {
        return Error("Invalid number: '${input}'");
    }
}

fn main() {
    // Success case
    let result1 = divide(100, 5);
    match result1 {
        Ok(value) => println("100 / 5 = ${value}"),
        Error(err) => println("Error: ${err}")
    }

    // Error case
    let result2 = divide(100, 0);
    match result2 {
        Ok(value) => println("Result: ${value}"),
        Error(err) => println("Error: ${err}")
    }

    // Chain operations
    let parsed = parseNumber("42");
    match parsed {
        Ok(n) => println("Parsed: ${n}"),
        Error(e) => println("Failed: ${e}")
    }

    let invalid = parseNumber("hello");
    match invalid {
        Ok(n) => println("Parsed: ${n}"),
        Error(e) => println("Failed: ${e}")
    }
}
```

**Output:**
```
100 / 5 = 20
Error: Cannot divide 100 by zero
Parsed: 42
Failed: Invalid number: 'hello'
```

---

## 29. Lambda Functions ✅

```sp
fn main() {
    // ═══════════════════════════════════════
    //  LAMBDA SYNTAX:  (params) => expression
    // ═══════════════════════════════════════

    var add = (int a, int b) => a + b;
    var multiply = (int a, int b) => a * b;
    var square = (int x) => x * x;
    var isEven = (int n) => n % 2 == 0;
    var greet = (string name) => "Hello, ${name}!";

    println(add(10, 20));          // 30
    println(multiply(5, 4));       // 20
    println(square(7));            // 49
    println(isEven(42));           // true
    println(greet("Shanto"));      // Hello, Shanto!

    // ═══════════════════════════════════════
    //  LAMBDA WITH BLOCK BODY
    // ═══════════════════════════════════════

    var factorial = (int n) => {
        int result = 1;
        for (int i = 2; i <= n; i++) {
            result *= i;
        }
        return result;
    };

    println("5! = ${factorial(5)}");     // 120
    println("10! = ${factorial(10)}");   // 3628800

    // ═══════════════════════════════════════
    //  LAMBDA AS FUNCTION ARGUMENT
    // ═══════════════════════════════════════

    let numbers = [1, 2, 3, 4, 5];

    let doubled = numbers.map(x => x * 2);
    println("Doubled: ${doubled}");       // [2, 4, 6, 8, 10]

    let evens = numbers.filter(x => x % 2 == 0);
    println("Evens: ${evens}");           // [2, 4]

    let sum = numbers.reduce((a, b) => a + b);
    println("Sum: ${sum}");               // 15
}
```

---

## 30. Higher-Order Functions ✅

```sp
// ═══════════════════════════════════════════════════
//  FUNCTIONS THAT TAKE FUNCTIONS AS PARAMETERS
// ═══════════════════════════════════════════════════

fn applyTwice(fn(int) -> int operation, int value) -> int {
    return operation(operation(value));
}

fn transform(int[] arr, fn(int) -> int func) -> int[] {
    int[] result = new int[arr.length()];
    for (int i = 0; i < arr.length(); i++) {
        result[i] = func(arr[i]);
    }
    return result;
}

fn executeIf(bool condition, fn() action) {
    if (condition) {
        action();
    }
}

// ═══════════════════════════════════════════════════
//  FUNCTIONS THAT RETURN FUNCTIONS
// ═══════════════════════════════════════════════════

fn multiplier(int factor) -> fn(int) -> int {
    return (int x) => x * factor;
}

fn adder(int amount) -> fn(int) -> int {
    return (int x) => x + amount;
}

fn main() {
    // Pass lambda to function
    int result = applyTwice(x => x * 2, 3);
    println("applyTwice(double, 3) = ${result}");  // 12

    // Transform array
    int[] nums = {1, 2, 3, 4, 5};
    int[] squared = transform(nums, x => x * x);
    println("Squared: ${squared}");    // [1, 4, 9, 16, 25]

    // Conditional execution
    executeIf(true, () => println("This runs!"));
    executeIf(false, () => println("This doesn't!"));

    // Function factories
    var double = multiplier(2);
    var triple = multiplier(3);
    var addTen = adder(10);

    println("double(5) = ${double(5)}");     // 10
    println("triple(5) = ${triple(5)}");     // 15
    println("addTen(5) = ${addTen(5)}");     // 15

    // ═══════════════════════════════════════
    //  CHAINING: map → filter → reduce
    // ═══════════════════════════════════════

    let result = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        .map(x => x * x)           // square each
        .filter(x => x > 20)       // keep > 20
        .reduce((a, b) => a + b);  // sum them

    println("\nSum of squares > 20: ${result}");  // 25+36+49+64+81+100 = 355
}
```

---

## 31. Closures ✅

```sp
fn makeCounter() -> fn() -> int {
    int count = 0;
    return () => {
        count++;
        return count;
    };
}

fn makeGreeter(string greeting) -> fn(string) -> string {
    return (string name) => "${greeting}, ${name}!";
}

fn main() {
    // Counter closure — captures and modifies `count`
    var counter = makeCounter();
    println(counter());    // 1
    println(counter());    // 2
    println(counter());    // 3

    // Greeter closure — captures `greeting`
    var hello = makeGreeter("Hello");
    var hi = makeGreeter("Hi");

    println(hello("Shanto"));    // Hello, Shanto!
    println(hi("Alice"));        // Hi, Alice!
}
```

---

## 32. String Methods ✅

```sp
fn main() {
    string text = "  Hello, SP Programming Language!  ";

    // ═══════════════════════════════════════
    //  COMMON STRING METHODS
    // ═══════════════════════════════════════

    println("Original:   '${text}'");
    println("Trimmed:    '${text.trim()}'");
    println("Length:     ${text.trim().length()}");
    println("Upper:      ${text.trim().upper()}");
    println("Lower:      ${text.trim().lower()}");
    println("Contains:   ${text.contains("SP")}");         // true
    println("StartsWith: ${text.trim().startsWith("Hello")}");  // true
    println("EndsWith:   ${text.trim().endsWith("!")}");        // true
    println("IndexOf:    ${text.indexOf("SP")}");           // 9
    println("Replace:    ${text.trim().replace("SP", "Shanto Paul")}");
    println("Substring:  ${text.trim().substring(0, 5)}");  // Hello
    println("CharAt:     ${text.trim().charAt(0)}");        // H

    // ═══════════════════════════════════════
    //  SPLIT & JOIN
    // ═══════════════════════════════════════

    string csv = "apple,banana,cherry,date";
    let fruits = csv.split(",");
    println("\nSplit CSV:");
    for (string fruit in fruits) {
        println("  - ${fruit}");
    }

    string joined = fruits.join(" | ");
    println("Joined: ${joined}");
    // apple | banana | cherry | date

    // ═══════════════════════════════════════
    //  STRING INTERPOLATION
    // ═══════════════════════════════════════

    string name = "Shanto";
    int age = 21;
    double gpa = 3.95;

    println("\n${name} is ${age} years old with GPA ${gpa}");
    println("In 5 years: ${name} will be ${age + 5}");
    println("Name reversed: ... (exercise for the reader!)");
}
```

---

## 33. Method Overloading ✅

```sp
class Printer {

    // Same method name, different parameter types
    fn print(int value) {
        println("Integer: ${value}");
    }

    fn print(double value) {
        println("Double: ${value}");
    }

    fn print(string value) {
        println("String: ${value}");
    }

    fn print(bool value) {
        println("Boolean: ${value}");
    }

    // Different parameter counts
    fn print(string label, int value) {
        println("${label}: ${value}");
    }

    fn print(string label, int value, string unit) {
        println("${label}: ${value} ${unit}");
    }
}

fn main() {
    Printer p = new Printer();

    p.print(42);                       // Integer: 42
    p.print(3.14);                     // Double: 3.14
    p.print("Hello");                  // String: Hello
    p.print(true);                     // Boolean: true
    p.print("Age", 21);               // Age: 21
    p.print("Weight", 75, "kg");       // Weight: 75 kg
}
```

---

## 34. Operator Overloading ✅

```sp
struct Vector2 {
    float x;
    float y;

    operator +(Vector2 a, Vector2 b) {
        return Vector2(a.x + b.x, a.y + b.y);
    }

    operator -(Vector2 a, Vector2 b) {
        return Vector2(a.x - b.x, a.y - b.y);
    }

    operator *(Vector2 v, float scalar) {
        return Vector2(v.x * scalar, v.y * scalar);
    }

    operator ==(Vector2 a, Vector2 b) {
        return a.x == b.x && a.y == b.y;
    }
}

fn main() {
    Vector2 a = Vector2(3.0, 4.0);
    Vector2 b = Vector2(1.0, 2.0);

    Vector2 sum = a + b;
    Vector2 diff = a - b;
    Vector2 scaled = a * 2.0;

    println("a + b = (${sum.x}, ${sum.y})");         // (4.0, 6.0)
    println("a - b = (${diff.x}, ${diff.y})");       // (2.0, 2.0)
    println("a * 2 = (${scaled.x}, ${scaled.y})");   // (6.0, 8.0)
    println("a == b: ${a == b}");                     // false
}
```

---

## 35. Extension Methods ✅

```sp
extend string {
    fn isEmail() -> bool {
        return this.contains("@") && this.contains(".");
    }

    fn reverse() -> string {
        string result = "";
        for (int i = this.length() - 1; i >= 0; i--) {
            result += this.charAt(i);
        }
        return result;
    }

    fn wordCount() -> int {
        return this.trim().split(" ").length();
    }
}

extend int {
    fn isEven() -> bool { return this % 2 == 0; }
    fn isOdd() -> bool { return this % 2 != 0; }
    fn factorial() -> int {
        int result = 1;
        for (int i = 2; i <= this; i++) result *= i;
        return result;
    }
}

fn main() {
    // String extensions
    println("test@email.com".isEmail());         // true
    println("not-an-email".isEmail());            // false
    println("Hello".reverse());                   // olleH
    println("Hello SP World".wordCount());        // 3

    // Int extensions
    println(42.isEven());        // true
    println(7.isOdd());          // true
    println(5.factorial());      // 120
}
```

---

## 36. Traits ✅

```sp
trait Serializable {
    fn serialize() -> string;
    fn deserialize(string data);
}

trait Displayable {
    fn display();
}

class User with Serializable, Displayable {
    string name;
    int age;

    User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    fn serialize() -> string {
        return "{\"name\":\"${name}\",\"age\":${age}}";
    }

    fn deserialize(string data) {
        // Parse JSON data...
    }

    fn display() {
        println("👤 ${name} (${age})");
    }
}

fn main() {
    User user = new User("Shanto", 21);
    user.display();                          // 👤 Shanto (21)
    println("JSON: ${user.serialize()}");    // {"name":"Shanto","age":21}
}
```

---

## 37. Annotations ✅

```sp
@Serializable
class Config {
    string host;
    int port;
    bool debug;
}

class ApiController {

    @Route("/users")
    @Method("GET")
    fn getUsers() -> List<User> {
        return userService.findAll();
    }

    @Route("/users")
    @Method("POST")
    fn createUser(User user) -> User {
        return userService.save(user);
    }

    @Deprecated("Use getUsers() instead")
    fn listUsers() -> List<User> {
        return getUsers();
    }
}

class MathService {

    @inline
    fn add(int a, int b) -> int {
        return a + b;
    }

    @Test
    fn testAdd() {
        assert(add(2, 3) == 5);
    }
}
```

---

## 38. Modules & Imports ✅

```sp
// ═══════════════════════════════════════════════════
//  FILE: src/models/user.sp
// ═══════════════════════════════════════════════════

module myapp.models;

public class User {
    public string name;
    public int age;

    public User(string name, int age) {
        this.name = name;
        this.age = age;
    }
}

// ═══════════════════════════════════════════════════
//  FILE: src/services/user_service.sp
// ═══════════════════════════════════════════════════

module myapp.services;

import myapp.models.User;

public class UserService {
    private List<User> users = new List<User>();

    public fn addUser(User user) {
        users.add(user);
    }

    public fn findByName(string name) -> User? {
        for (User u in users) {
            if (u.name == name) return u;
        }
        return null;
    }
}

// ═══════════════════════════════════════════════════
//  FILE: src/main.sp
// ═══════════════════════════════════════════════════

import std.io;
import myapp.models.User;
import myapp.services.UserService;

fn main() {
    UserService service = new UserService();
    service.addUser(new User("Shanto", 21));
    service.addUser(new User("Alice", 25));

    User? user = service.findByName("Shanto");
    println("Found: ${user?.name}, age ${user?.age}");

    // Standard library imports
    import std.math;
    import std.collections;
    import std.fs;
    import std.json;
    import std.http;
    import std.time;
}
```

---

## 39. Async & Await ✅

```sp
import std.http;
import std.io;

async fn fetchUser(int id) -> User {
    let response = await http.get("https://api.example.com/users/${id}");
    return json.parse<User>(response.body);
}

async fn fetchMultipleUsers() {
    // Sequential
    let user1 = await fetchUser(1);
    let user2 = await fetchUser(2);

    println("User 1: ${user1.name}");
    println("User 2: ${user2.name}");
}

async fn main() {
    println("Fetching data...");
    await fetchMultipleUsers();
    println("Done!");
}
```

---

## 40. Threads & Concurrency ✅

```sp
fn main() {
    // ═══════════════════════════════════════
    //  SPAWN A THREAD
    // ═══════════════════════════════════════

    thread {
        for (i in 0..5) {
            println("Background: ${i}");
        }
    }

    for (i in 0..5) {
        println("Main: ${i}");
    }

    // ═══════════════════════════════════════
    //  FUTURE PRIMITIVES
    // ═══════════════════════════════════════
    // spawn    — launch concurrent task
    // channel  — message passing
    // mutex    — mutual exclusion
    // atomic   — atomic operations
}
```

---

## 41. Unsafe Mode

```sp
fn main() {
    // ═══════════════════════════════════════
    //  UNSAFE BLOCK — for systems programming
    // ═══════════════════════════════════════

    unsafe {
        pointer p = allocate(1024);   // Raw memory allocation
        // Direct memory operations
        // Pointer arithmetic
        // No bounds checking
        free(p);
    }

    // Outside unsafe: full safety guarantees
    println("Back to safe SP! ✅");
}
```

---

## 42. Native Interop (C FFI)

```sp
// ═══════════════════════════════════════════════════
//  CALL C FUNCTIONS FROM SP
// ═══════════════════════════════════════════════════

extern "C" {
    fn printf(string format);
    fn strlen(string s) -> int;
    fn malloc(int size) -> pointer;
    fn free(pointer p);
}

fn main() {
    // Call C's printf directly
    printf("Hello from C! %d\n", 42);

    // Use C's strlen
    int len = strlen("Hello");
    println("strlen: ${len}");   // 5
}
```

---

## 43. Testing

```sp
import std.testing;

// ═══════════════════════════════════════════════════
//  FUNCTIONS TO TEST
// ═══════════════════════════════════════════════════

fn add(int a, int b) -> int { return a + b; }
fn factorial(int n) -> int {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}
fn isEven(int n) -> bool { return n % 2 == 0; }

// ═══════════════════════════════════════════════════
//  TEST BLOCKS
// ═══════════════════════════════════════════════════

test "addition works" {
    assert(add(2, 3) == 5);
    assert(add(-1, 1) == 0);
    assert(add(0, 0) == 0);
}

test "factorial is correct" {
    assert(factorial(0) == 1);
    assert(factorial(1) == 1);
    assert(factorial(5) == 120);
    assert(factorial(10) == 3628800);
}

test "even/odd detection" {
    assert(isEven(0) == true);
    assert(isEven(2) == true);
    assert(isEven(3) == false);
    assert(isEven(100) == true);
}

test "string operations" {
    string name = "Shanto";
    assert(name.length() == 6);
    assert(name.upper() == "SHANTO");
    assert(name.contains("han") == true);
}
```

**Run tests:**
```bash
$ spc test

  ✅ addition works ............ passed (0.1ms)
  ✅ factorial is correct ...... passed (0.2ms)
  ✅ even/odd detection ........ passed (0.1ms)
  ✅ string operations ......... passed (0.1ms)

  Tests:  4 passed | 0 failed
  Time:   0.5ms
```

---

## 44. Comments & Documentation

```sp
// ═══════════════════════════════════════════════════
//  SINGLE-LINE COMMENT
// ═══════════════════════════════════════════════════

// This is a single-line comment
int age = 21;  // This is an inline comment

// ═══════════════════════════════════════════════════
//  MULTI-LINE COMMENT
// ═══════════════════════════════════════════════════

/*
    This is a multi-line comment.
    It can span multiple lines.
    Useful for longer explanations.
*/

// ═══════════════════════════════════════════════════
//  DOCUMENTATION COMMENT (for spc doc)
// ═══════════════════════════════════════════════════

/// Calculates the sum of two integers.
///
/// @param a The first integer.
/// @param b The second integer.
/// @return The sum of a and b.
///
/// Example:
/// ```sp
/// int result = add(10, 20);  // returns 30
/// ```
fn add(int a, int b) -> int {
    return a + b;
}

/// Represents a user in the system.
///
/// Users have a name, age, and email address.
/// Email is optional and can be null.
class User {
    /// The user's full name
    string name;

    /// The user's age in years
    int age;

    /// The user's email (optional)
    string? email;
}
```

---

## 45. Multi-Syntax Compatibility

SP's unique feature — multiple syntax styles that compile to identical code:

```sp
// ═══════════════════════════════════════════════════
//  FUNCTION DECLARATION — 3 valid styles
// ═══════════════════════════════════════════════════

// Style 1: SP-native
fn add(int a, int b) -> int {
    return a + b;
}

// Style 2: Java / C++ / C#
int add(int a, int b) {
    return a + b;
}

// Style 3: Short form
fn add(a, b) => a + b;


// ═══════════════════════════════════════════════════
//  VOID FUNCTIONS — 2 valid styles
// ═══════════════════════════════════════════════════

// SP-native
fn printName() {
    println("Shanto");
}

// Java/C++ style
void printName() {
    println("Shanto");
}


// ═══════════════════════════════════════════════════
//  INHERITANCE — 2 valid styles
// ═══════════════════════════════════════════════════

// Java style
class Dog extends Animal { }

// C# style
class Dog : Animal { }


// ═══════════════════════════════════════════════════
//  INTERFACE IMPLEMENTATION — 2 valid styles
// ═══════════════════════════════════════════════════

// Java style
class User implements Serializable { }

// C# style
class User : Serializable { }


// ═══════════════════════════════════════════════════
//  SWITCH vs MATCH
// ═══════════════════════════════════════════════════

// Classic (C/Java)
switch (x) {
    case 1: println("one"); break;
    default: println("other");
}

// Modern (SP)
match x {
    1 => println("one"),
    _ => println("other")
}


// ═══════════════════════════════════════════════════
//  CONSTRUCTOR — 2 valid styles
// ═══════════════════════════════════════════════════

// SP-native
class User {
    constructor(string name) {
        this.name = name;
    }
}

// Java/C# style
class User {
    User(string name) {
        this.name = name;
    }
}


// ═══════════════════════════════════════════════════
//  ENTRY POINT — 2 valid styles
// ═══════════════════════════════════════════════════

// SP-native (recommended)
fn main() {
    println("Hello, SP!");
}

// Java-style (compatibility)
public static void main() {
    System.out.println("Hello, SP!");
}
```

> **All styles compile to the same AST → same LLVM IR → same native binary.**

---

## 46. Complete Programs

### 🧮 Calculator

```sp
import std.io;

fn main() {
    println("═══════════════════════════");
    println("  SP Calculator");
    println("═══════════════════════════\n");

    print("Enter first number: ");
    double a = double.parse(readLine());

    print("Enter operator (+, -, *, /): ");
    string op = readLine();

    print("Enter second number: ");
    double b = double.parse(readLine());

    double result;
    match op {
        "+" => result = a + b,
        "-" => result = a - b,
        "*" => result = a * b,
        "/" => {
            if (b == 0) {
                println("❌ Error: Division by zero!");
                return;
            }
            result = a / b;
        },
        _ => {
            println("❌ Unknown operator: ${op}");
            return;
        }
    }

    println("\n✅ ${a} ${op} ${b} = ${result}");
}
```

### 📝 Todo App

```sp
import std.io;
import std.collections;

class Task {
    string title;
    bool done;

    Task(string title) {
        this.title = title;
        this.done = false;
    }

    fn toggle() { done = !done; }

    fn toString() -> string {
        string icon = done ? "✅" : "⬜";
        return "${icon} ${title}";
    }
}

class TodoApp {
    private List<Task> tasks = new List<Task>();

    fn add(string title) {
        tasks.add(new Task(title));
        println("Added: ${title}");
    }

    fn complete(int index) {
        if (index >= 0 && index < tasks.size()) {
            tasks[index].toggle();
            println("Toggled: ${tasks[index].title}");
        }
    }

    fn show() {
        println("\n╔══════════════════════════════╗");
        println("║       📋 TODO LIST           ║");
        println("╠══════════════════════════════╣");

        if (tasks.isEmpty()) {
            println("║  No tasks yet!              ║");
        }

        for (int i = 0; i < tasks.size(); i++) {
            println("║  ${i + 1}. ${tasks[i].toString()}");
        }

        int done = tasks.filter(t => t.done).size();
        println("╠══════════════════════════════╣");
        println("║  ${done}/${tasks.size()} completed");
        println("╚══════════════════════════════╝");
    }
}

fn main() {
    TodoApp app = new TodoApp();

    app.add("Learn SP syntax");
    app.add("Build a compiler");
    app.add("Write standard library");
    app.add("Create VS Code extension");
    app.add("Launch SP 1.0");

    app.complete(0);   // Learn SP syntax ✅
    app.complete(1);   // Build a compiler ✅

    app.show();
}
```

**Output:**
```
Added: Learn SP syntax
Added: Build a compiler
Added: Write standard library
Added: Create VS Code extension
Added: Launch SP 1.0
Toggled: Learn SP syntax
Toggled: Build a compiler

╔══════════════════════════════╗
║       📋 TODO LIST           ║
╠══════════════════════════════╣
║  1. ✅ Learn SP syntax
║  2. ✅ Build a compiler
║  3. ⬜ Write standard library
║  4. ⬜ Create VS Code extension
║  5. ⬜ Launch SP 1.0
╠══════════════════════════════╣
║  2/5 completed
╚══════════════════════════════╝
```

### 🏦 Banking System

```sp
import std.io;
import std.collections;

class Account {
    private string owner;
    private double balance;
    private List<string> history = new List<string>();

    Account(string owner, double initial) {
        this.owner = owner;
        this.balance = initial;
        log("Account created with \$${initial}");
    }

    fn deposit(double amount) {
        if (amount <= 0) {
            println("❌ Invalid amount");
            return;
        }
        balance += amount;
        log("Deposit: +\$${amount}");
        println("✅ Deposited \$${amount}");
    }

    fn withdraw(double amount) {
        if (amount <= 0 || amount > balance) {
            println("❌ Invalid or insufficient funds");
            return;
        }
        balance -= amount;
        log("Withdraw: -\$${amount}");
        println("✅ Withdrew \$${amount}");
    }

    fn transfer(Account target, double amount) {
        if (amount <= 0 || amount > balance) {
            println("❌ Transfer failed");
            return;
        }
        balance -= amount;
        target.balance += amount;
        log("Transfer: -\$${amount} → ${target.owner}");
        target.log("Received: +\$${amount} from ${owner}");
        println("✅ Transferred \$${amount} to ${target.owner}");
    }

    fn showBalance() {
        println("💰 ${owner}'s balance: \$${balance}");
    }

    fn showHistory() {
        println("\n📜 ${owner}'s Transaction History:");
        for (string entry in history) {
            println("   ${entry}");
        }
    }

    private fn log(string message) {
        history.add(message);
    }
}

fn main() {
    println("═══════════════════════════════");
    println("  🏦 SP Banking System");
    println("═══════════════════════════════\n");

    Account shanto = new Account("Shanto", 5000.0);
    Account alice = new Account("Alice", 3000.0);

    shanto.deposit(2000.0);
    shanto.withdraw(500.0);
    shanto.transfer(alice, 1000.0);

    println("");
    shanto.showBalance();
    alice.showBalance();

    shanto.showHistory();
    alice.showHistory();
}
```

**Output:**
```
═══════════════════════════════
  🏦 SP Banking System
═══════════════════════════════

✅ Deposited $2000.0
✅ Withdrew $500.0
✅ Transferred $1000.0 to Alice

💰 Shanto's balance: $5500.0
💰 Alice's balance: $4000.0

📜 Shanto's Transaction History:
   Account created with $5000.0
   Deposit: +$2000.0
   Withdraw: -$500.0
   Transfer: -$1000.0 → Alice

📜 Alice's Transaction History:
   Account created with $3000.0
   Received: +$1000.0 from Shanto
```

---

<div align="center">

### 🎉 You now know SP!

**Start writing code in `hello.sp` and run it with `spc run hello.sp`**

[← Back to README](README.md) · [Language Specification](SP%20Programming%20Language%20—%20Complete%20Language%20%26%20Compiler%20Specification.md) · [Phases](Phases.md)

</div>
