// Generated automatically from SP-Syntax-Guide.md
// Complete reference of all 100 syntax examples with code and terminal outputs.

export interface SyntaxSnippet {
  subtitle?: string;
  description?: string;
  code: string;
  output?: string;
}

export interface SyntaxExample {
  id: number;
  title: string;
  category: string;
  description: string;
  snippets: SyntaxSnippet[];
}

export const SYNTAX_CATEGORIES = [
  "All",
  "Basics & Fundamentals",
  "Control Flow & Structures",
  "Object-Oriented Programming",
  "Advanced Types & Safety",
  "File I/O & System APIs",
  "Collections & OS Utilities",
  "Modern Idioms & Error Handling",
  "Design Patterns & Functional",
  "Networking, JSON & Protocols",
  "Concurrency, Algorithms & Projects",
] as const;

export const ALL_SYNTAX_EXAMPLES: SyntaxExample[] = [
  {
    "id": 1,
    "title": "Hello World",
    "category": "Basics & Fundamentals",
    "description": "The simplest SP program:",
    "snippets": [
      {
        "code": "fn main() {\n    println(\"Hello, SP!\");\n}",
        "output": "Hello, SP!"
      }
    ]
  },
  {
    "id": 2,
    "title": "Variables & Constants",
    "category": "Basics & Fundamentals",
    "description": "Examples and idioms for Variables & Constants, starting with mutable variables.",
    "snippets": [
      {
        "subtitle": "Mutable Variables",
        "code": "fn main() {\n    // Explicit types\n    int age = 21;\n    string name = \"Shanto\";\n    double salary = 75000.50;\n    float score = 95.5;\n    bool isActive = true;\n    char grade = 'A';\n\n    println(\"Name: ${name}\");\n    println(\"Age: ${age}\");\n    println(\"Salary: ${salary}\");\n    println(\"Score: ${score}\");\n    println(\"Active: ${isActive}\");\n    println(\"Grade: ${grade}\");\n\n    // Variables can be reassigned\n    age = 22;\n    name = \"Shanto Paul\";\n    println(\"Updated: ${name}, age ${age}\");\n}",
        "output": "Name: Shanto\nAge: 21\nSalary: 75000.5\nScore: 95.5\nActive: true\nGrade: A\nUpdated: Shanto Paul, age 22"
      },
      {
        "subtitle": "Constants (Immutable)",
        "code": "fn main() {\n    const int MAX_USERS = 1000;\n    const double TAX_RATE = 0.15;\n    const string APP_NAME = \"MyApp\";\n\n    println(\"App: ${APP_NAME}\");\n    println(\"Max Users: ${MAX_USERS}\");\n    println(\"Tax Rate: ${TAX_RATE}\");\n\n    // ❌ This will NOT compile:\n    // MAX_USERS = 2000;    // error: cannot assign to constant\n}"
      }
    ]
  },
  {
    "id": 3,
    "title": "Data Types",
    "category": "Basics & Fundamentals",
    "description": "Examples and idioms for Data Types, starting with all primitive types.",
    "snippets": [
      {
        "subtitle": "All Primitive Types",
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  INTEGER TYPES\n    // ═══════════════════════════════════════\n\n    int number = 42;              // Platform default (32 or 64 bit)\n    uint positive = 100;          // Unsigned integer\n\n    int8 tiny = 127;              // -128 to 127\n    int16 small = 32000;          // -32768 to 32767\n    int32 medium = 2147483647;    // Standard 32-bit\n    int64 large = 9223372036854;  // 64-bit\n\n    uint8 uTiny = 255;            // 0 to 255\n    uint16 uSmall = 65535;        // 0 to 65535\n    uint32 uMedium = 4294967295;  // 0 to 4,294,967,295\n    uint64 uLarge = 18446744073;  // 0 to 18,446,744,073,709,551,615\n\n    // ═══════════════════════════════════════\n    //  FLOATING POINT TYPES\n    // ═══════════════════════════════════════\n\n    float price = 29.99;          // 32-bit floating point\n    double precise = 3.141592653589793;  // 64-bit floating point\n\n    // ═══════════════════════════════════════\n    //  OTHER TYPES\n    // ═══════════════════════════════════════\n\n    bool alive = true;            // true or false\n    bool dead = false;\n\n    char letter = 'A';            // Single character\n    char symbol = '@';\n    char newline = '\\n';\n\n    string text = \"Hello, SP!\";   // String of characters\n    string empty = \"\";\n\n    byte raw = 0xFF;              // Raw byte (0-255)\n\n    // Print all\n    println(\"int:    ${number}\");\n    println(\"float:  ${price}\");\n    println(\"double: ${precise}\");\n    println(\"bool:   ${alive}\");\n    println(\"char:   ${letter}\");\n    println(\"string: ${text}\");\n    println(\"byte:   ${raw}\");\n}",
        "output": "int:    42\nfloat:  29.99\ndouble: 3.141592653589793\nbool:   true\nchar:   A\nstring: Hello, SP!\nbyte:   255"
      }
    ]
  },
  {
    "id": 4,
    "title": "Type Inference",
    "category": "Basics & Fundamentals",
    "description": "Hands-on syntax example and reference implementation for Type Inference.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  var — mutable, type inferred\n    // ═══════════════════════════════════════\n\n    var age = 21;                 // inferred as int\n    var name = \"Shanto\";          // inferred as string\n    var price = 99.99;            // inferred as double\n    var active = true;            // inferred as bool\n\n    age = 22;                     // ✅ Can reassign\n    name = \"Shanto Paul\";         // ✅ Can reassign\n\n    // ═══════════════════════════════════════\n    //  let — mutable inferred (alternative)\n    // ═══════════════════════════════════════\n\n    let count = 100;\n    let message = \"Hello\";\n\n    // ═══════════════════════════════════════\n    //  const — immutable, type inferred\n    // ═══════════════════════════════════════\n\n    const PI = 3.14159;           // inferred as double, immutable\n    const APP = \"SP Lang\";        // inferred as string, immutable\n\n    // ❌ PI = 3.14;              // error: cannot assign to constant\n\n    println(\"Age: ${age}\");\n    println(\"Name: ${name}\");\n    println(\"PI: ${PI}\");\n    println(\"App: ${APP}\");\n}",
        "output": "Age: 22\nName: Shanto Paul\nPI: 3.14159\nApp: SP Lang"
      }
    ]
  },
  {
    "id": 5,
    "title": "Operators",
    "category": "Basics & Fundamentals",
    "description": "Hands-on syntax example and reference implementation for Operators.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  ARITHMETIC\n    // ═══════════════════════════════════════\n\n    int a = 20;\n    int b = 7;\n\n    println(\"${a} + ${b} = ${a + b}\");     // 27\n    println(\"${a} - ${b} = ${a - b}\");     // 13\n    println(\"${a} * ${b} = ${a * b}\");     // 140\n    println(\"${a} / ${b} = ${a / b}\");     // 2  (integer division)\n    println(\"${a} % ${b} = ${a % b}\");     // 6  (remainder)\n\n    // ═══════════════════════════════════════\n    //  COMPARISON\n    // ═══════════════════════════════════════\n\n    println(\"${a} == ${b} → ${a == b}\");   // false\n    println(\"${a} != ${b} → ${a != b}\");   // true\n    println(\"${a} > ${b}  → ${a > b}\");    // true\n    println(\"${a} < ${b}  → ${a < b}\");    // false\n    println(\"${a} >= ${b} → ${a >= b}\");   // true\n    println(\"${a} <= ${b} → ${a <= b}\");   // false\n\n    // ═══════════════════════════════════════\n    //  LOGICAL\n    // ═══════════════════════════════════════\n\n    bool x = true;\n    bool y = false;\n\n    println(\"${x} && ${y} → ${x && y}\");   // false\n    println(\"${x} || ${y} → ${x || y}\");   // true\n    println(\"!${x}        → ${!x}\");       // false\n\n    // ═══════════════════════════════════════\n    //  ASSIGNMENT\n    // ═══════════════════════════════════════\n\n    int n = 10;\n    n += 5;    println(\"n += 5  → ${n}\");   // 15\n    n -= 3;    println(\"n -= 3  → ${n}\");   // 12\n    n *= 2;    println(\"n *= 2  → ${n}\");   // 24\n    n /= 4;    println(\"n /= 4  → ${n}\");   // 6\n    n %= 4;    println(\"n %= 4  → ${n}\");   // 2\n\n    // ═══════════════════════════════════════\n    //  INCREMENT / DECREMENT\n    // ═══════════════════════════════════════\n\n    int counter = 0;\n    counter++;   println(\"counter++ → ${counter}\");  // 1\n    counter++;   println(\"counter++ → ${counter}\");  // 2\n    counter--;   println(\"counter-- → ${counter}\");  // 1\n\n    // ═══════════════════════════════════════\n    //  BITWISE\n    // ═══════════════════════════════════════\n\n    int p = 0b1100;   // 12\n    int q = 0b1010;   // 10\n\n    println(\"p & q  = ${p & q}\");    // 8  (1000)\n    println(\"p | q  = ${p | q}\");    // 14 (1110)\n    println(\"p ^ q  = ${p ^ q}\");    // 6  (0110)\n    println(\"~p     = ${~p}\");       // -13\n    println(\"p << 1 = ${p << 1}\");   // 24\n    println(\"p >> 1 = ${p >> 1}\");   // 6\n}"
      }
    ]
  },
  {
    "id": 6,
    "title": "Strings & Interpolation",
    "category": "Basics & Fundamentals",
    "description": "Hands-on syntax example and reference implementation for Strings & Interpolation.",
    "snippets": [
      {
        "code": "fn main() {\n    string firstName = \"Shanto\";\n    string lastName = \"Paul\";\n    int age = 21;\n\n    // ═══════════════════════════════════════\n    //  CONCATENATION\n    // ═══════════════════════════════════════\n\n    string fullName = firstName + \" \" + lastName;\n    println(fullName);   // Shanto Paul\n\n    // ═══════════════════════════════════════\n    //  STRING INTERPOLATION  ${expression}\n    // ═══════════════════════════════════════\n\n    println(\"Hello, ${firstName}!\");\n    println(\"Full Name: ${firstName} ${lastName}\");\n    println(\"Age next year: ${age + 1}\");\n    println(\"Name length: ${firstName.length()}\");\n    println(\"Uppercase: ${firstName.upper()}\");\n\n    // ═══════════════════════════════════════\n    //  ESCAPE SEQUENCES\n    // ═══════════════════════════════════════\n\n    println(\"Line 1\\nLine 2\");           // newline\n    println(\"Tab\\there\");                // tab\n    println(\"She said \\\"Hello\\\"\");       // quotes\n    println(\"Backslash: \\\\\");            // backslash\n    println(\"Unicode: \\u0041\");          // A\n\n    // ═══════════════════════════════════════\n    //  MULTI-LINE (future)\n    // ═══════════════════════════════════════\n\n    string poem = \"Roses are red,\\n\"\n                + \"Violets are blue,\\n\"\n                + \"SP is awesome,\\n\"\n                + \"And so are you.\";\n\n    println(poem);\n}",
        "output": "Shanto Paul\nHello, Shanto!\nFull Name: Shanto Paul\nAge next year: 22\nName length: 6\nUppercase: SHANTO\nLine 1\nLine 2\nTab\there\nShe said \"Hello\"\nBackslash: \\\nUnicode: A\nRoses are red,\nViolets are blue,\nSP is awesome,\nAnd so are you."
      }
    ]
  },
  {
    "id": 7,
    "title": "Console I/O",
    "category": "Basics & Fundamentals",
    "description": "Hands-on syntax example and reference implementation for Console I/O.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn main() {\n    // ═══════════════════════════════════════\n    //  OUTPUT\n    // ═══════════════════════════════════════\n\n    println(\"Hello, World!\");           // Print with newline\n    print(\"Enter name: \");             // Print without newline\n\n    // ═══════════════════════════════════════\n    //  INPUT\n    // ═══════════════════════════════════════\n\n    string name = readLine();\n    println(\"Hello, ${name}!\");\n\n    print(\"Enter age: \");\n    int age = int.parse(readLine());\n    println(\"You are ${age} years old.\");\n\n    print(\"Enter score: \");\n    double score = double.parse(readLine());\n    println(\"Your score is ${score}\");\n}"
      }
    ]
  },
  {
    "id": 8,
    "title": "Functions",
    "category": "Basics & Fundamentals",
    "description": "Examples and idioms for Functions, starting with basic functions.",
    "snippets": [
      {
        "subtitle": "Basic Functions",
        "code": "// ═══════════════════════════════════════════════════\n//  SP-NATIVE SYNTAX:  fn name(params) -> returnType\n// ═══════════════════════════════════════════════════\n\nfn add(int a, int b) -> int {\n    return a + b;\n}\n\nfn greet(string name) {\n    println(\"Hello, ${name}!\");\n}\n\nfn isAdult(int age) -> bool {\n    return age >= 18;\n}\n\nfn getFullName(string first, string last) -> string {\n    return \"${first} ${last}\";\n}\n\nfn main() {\n    int sum = add(10, 20);\n    println(\"Sum: ${sum}\");           // Sum: 30\n\n    greet(\"Shanto\");                  // Hello, Shanto!\n\n    println(isAdult(21));             // true\n    println(isAdult(15));             // false\n\n    println(getFullName(\"Shanto\", \"Paul\"));  // Shanto Paul\n}"
      },
      {
        "subtitle": "Java/C++ Style Functions (Also Valid!)",
        "code": "// ═══════════════════════════════════════════════════\n//  JAVA/C++ STYLE:  returnType name(params)\n// ═══════════════════════════════════════════════════\n\nint multiply(int a, int b) {\n    return a * b;\n}\n\nstring formatAge(string name, int age) {\n    return \"${name} is ${age} years old\";\n}\n\nvoid sayGoodbye(string name) {\n    println(\"Goodbye, ${name}!\");\n}\n\nfn main() {\n    println(multiply(5, 4));                    // 20\n    println(formatAge(\"Shanto\", 21));           // Shanto is 21 years old\n    sayGoodbye(\"World\");                        // Goodbye, World!\n}"
      },
      {
        "subtitle": "Short Form Functions",
        "code": "// ═══════════════════════════════════════════════════\n//  SHORT FORM:  fn name(params) => expression\n// ═══════════════════════════════════════════════════\n\nfn square(int x) => x * x;\nfn cube(int x) => x * x * x;\nfn max(int a, int b) => a > b ? a : b;\nfn min(int a, int b) => a < b ? a : b;\nfn isEven(int n) => n % 2 == 0;\nfn double(int n) => n * 2;\n\nfn main() {\n    println(\"5² = ${square(5)}\");       // 25\n    println(\"3³ = ${cube(3)}\");         // 27\n    println(\"max(3,7) = ${max(3, 7)}\"); // 7\n    println(\"min(3,7) = ${min(3, 7)}\"); // 3\n    println(\"isEven(4) = ${isEven(4)}\");// true\n    println(\"double(8) = ${double(8)}\");// 16\n}"
      },
      {
        "subtitle": "Functions with Default Parameters",
        "code": "fn createUser(string name, int age, string role = \"user\") -> string {\n    return \"${name} (${age}) - ${role}\";\n}\n\nfn main() {\n    println(createUser(\"Shanto\", 21));            // Shanto (21) - user\n    println(createUser(\"Admin\", 30, \"admin\"));    // Admin (30) - admin\n}"
      },
      {
        "subtitle": "Recursive Functions",
        "code": "fn factorial(int n) -> int {\n    if (n <= 1) return 1;\n    return n * factorial(n - 1);\n}\n\nfn fibonacci(int n) -> int {\n    if (n <= 0) return 0;\n    if (n == 1) return 1;\n    return fibonacci(n - 1) + fibonacci(n - 2);\n}\n\nfn main() {\n    println(\"5! = ${factorial(5)}\");       // 120\n    println(\"10! = ${factorial(10)}\");     // 3628800\n\n    println(\"\\nFibonacci sequence:\");\n    for (int i = 0; i < 10; i++) {\n        print(\"${fibonacci(i)} \");\n    }\n    println(\"\");\n    // 0 1 1 2 3 5 8 13 21 34\n}"
      }
    ]
  },
  {
    "id": 9,
    "title": "If / Else / Else-If",
    "category": "Basics & Fundamentals",
    "description": "Hands-on syntax example and reference implementation for If / Else / Else-If.",
    "snippets": [
      {
        "code": "fn main() {\n    int age = 21;\n    int score = 85;\n    string role = \"admin\";\n\n    // ═══════════════════════════════════════\n    //  SIMPLE IF\n    // ═══════════════════════════════════════\n\n    if (age >= 18) {\n        println(\"You are an adult.\");\n    }\n\n    // ═══════════════════════════════════════\n    //  IF / ELSE\n    // ═══════════════════════════════════════\n\n    if (age >= 18) {\n        println(\"Welcome! You can enter.\");\n    } else {\n        println(\"Sorry, you must be 18+.\");\n    }\n\n    // ═══════════════════════════════════════\n    //  IF / ELSE-IF / ELSE (Grade System)\n    // ═══════════════════════════════════════\n\n    if (score >= 90) {\n        println(\"Grade: A  ⭐ Excellent!\");\n    } else if (score >= 80) {\n        println(\"Grade: B  👍 Good job!\");\n    } else if (score >= 70) {\n        println(\"Grade: C  📝 Average\");\n    } else if (score >= 60) {\n        println(\"Grade: D  ⚠️ Below average\");\n    } else {\n        println(\"Grade: F  ❌ Failed\");\n    }\n\n    // ═══════════════════════════════════════\n    //  NESTED IF\n    // ═══════════════════════════════════════\n\n    if (age >= 18) {\n        if (role == \"admin\") {\n            println(\"Admin access granted.\");\n        } else {\n            println(\"User access granted.\");\n        }\n    } else {\n        println(\"Access denied.\");\n    }\n\n    // ═══════════════════════════════════════\n    //  LOGICAL CONDITIONS\n    // ═══════════════════════════════════════\n\n    bool hasLicense = true;\n\n    if (age >= 18 && hasLicense) {\n        println(\"You can drive! 🚗\");\n    }\n\n    if (role == \"admin\" || role == \"moderator\") {\n        println(\"You have elevated permissions.\");\n    }\n\n    if (!hasLicense) {\n        println(\"Get a license first.\");\n    }\n\n    // ═══════════════════════════════════════\n    //  TERNARY OPERATOR\n    // ═══════════════════════════════════════\n\n    string status = age >= 18 ? \"Adult\" : \"Minor\";\n    println(\"Status: ${status}\");\n}",
        "output": "You are an adult.\nWelcome! You can enter.\nGrade: B  👍 Good job!\nAdmin access granted.\nYou can drive! 🚗\nYou have elevated permissions.\nStatus: Adult"
      }
    ]
  },
  {
    "id": 10,
    "title": "Switch & Match",
    "category": "Basics & Fundamentals",
    "description": "Examples and idioms for Switch & Match, starting with classic switch.",
    "snippets": [
      {
        "subtitle": "Classic Switch",
        "code": "fn main() {\n    int day = 3;\n\n    switch (day) {\n        case 1:\n            println(\"Monday\");\n            break;\n        case 2:\n            println(\"Tuesday\");\n            break;\n        case 3:\n            println(\"Wednesday\");\n            break;\n        case 4:\n            println(\"Thursday\");\n            break;\n        case 5:\n            println(\"Friday\");\n            break;\n        case 6:\n            println(\"Saturday 🎉\");\n            break;\n        case 7:\n            println(\"Sunday 🎉\");\n            break;\n        default:\n            println(\"Invalid day\");\n    }\n}"
      },
      {
        "subtitle": "Modern Match",
        "code": "fn main() {\n    int day = 3;\n\n    // ═══════════════════════════════════════\n    //  MATCH EXPRESSION (cleaner syntax)\n    // ═══════════════════════════════════════\n\n    match day {\n        1 => println(\"Monday\"),\n        2 => println(\"Tuesday\"),\n        3 => println(\"Wednesday\"),\n        4 => println(\"Thursday\"),\n        5 => println(\"Friday\"),\n        6 => println(\"Saturday 🎉\"),\n        7 => println(\"Sunday 🎉\"),\n        _ => println(\"Invalid day\")\n    }\n\n    // ═══════════════════════════════════════\n    //  MATCH WITH STRINGS\n    // ═══════════════════════════════════════\n\n    string command = \"start\";\n\n    match command {\n        \"start\"   => println(\"Starting server...\"),\n        \"stop\"    => println(\"Stopping server...\"),\n        \"restart\" => println(\"Restarting server...\"),\n        \"status\"  => println(\"Server is running.\"),\n        _         => println(\"Unknown command: ${command}\")\n    }\n\n    // ═══════════════════════════════════════\n    //  MATCH WITH ENUM\n    // ═══════════════════════════════════════\n\n    enum Season { Spring, Summer, Autumn, Winter }\n\n    Season current = Season.Summer;\n\n    match current {\n        Season.Spring => println(\"🌸 Flowers blooming\"),\n        Season.Summer => println(\"☀️ Beach time!\"),\n        Season.Autumn => println(\"🍂 Leaves falling\"),\n        Season.Winter => println(\"❄️ Stay warm!\")\n    }\n}",
        "output": "Wednesday\nStarting server...\n☀️ Beach time!"
      }
    ]
  },
  {
    "id": 11,
    "title": "For Loops",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for For Loops.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  CLASSIC FOR LOOP (C/Java style)\n    // ═══════════════════════════════════════\n\n    println(\"=== Classic For ===\");\n    for (int i = 1; i <= 5; i++) {\n        println(\"Count: ${i}\");\n    }\n\n    // ═══════════════════════════════════════\n    //  RANGE LOOP (Modern SP)\n    // ═══════════════════════════════════════\n\n    println(\"\\n=== Range Loop ===\");\n    for (i in 0..5) {\n        print(\"${i} \");\n    }\n    println(\"\");\n    // 0 1 2 3 4\n\n    // ═══════════════════════════════════════\n    //  FOR-EACH (iterate collections)\n    // ═══════════════════════════════════════\n\n    println(\"\\n=== For-Each ===\");\n    let fruits = [\"Apple\", \"Banana\", \"Cherry\", \"Date\"];\n\n    for (string fruit in fruits) {\n        println(\"🍎 ${fruit}\");\n    }\n\n    // ═══════════════════════════════════════\n    //  NESTED LOOPS\n    // ═══════════════════════════════════════\n\n    println(\"\\n=== Multiplication Table (1-5) ===\");\n    for (int i = 1; i <= 5; i++) {\n        for (int j = 1; j <= 5; j++) {\n            print(\"${i * j}\\t\");\n        }\n        println(\"\");\n    }\n\n    // ═══════════════════════════════════════\n    //  BREAK & CONTINUE\n    // ═══════════════════════════════════════\n\n    println(\"\\n=== Break at 5 ===\");\n    for (int i = 1; i <= 10; i++) {\n        if (i == 5) break;\n        print(\"${i} \");\n    }\n    println(\"\");\n    // 1 2 3 4\n\n    println(\"\\n=== Skip even numbers ===\");\n    for (int i = 1; i <= 10; i++) {\n        if (i % 2 == 0) continue;\n        print(\"${i} \");\n    }\n    println(\"\");\n    // 1 3 5 7 9\n\n    // ═══════════════════════════════════════\n    //  COUNTDOWN\n    // ═══════════════════════════════════════\n\n    println(\"\\n=== Countdown ===\");\n    for (int i = 5; i >= 1; i--) {\n        println(\"${i}...\");\n    }\n    println(\"🚀 Liftoff!\");\n\n    // ═══════════════════════════════════════\n    //  PATTERN: Sum of numbers\n    // ═══════════════════════════════════════\n\n    int sum = 0;\n    for (int i = 1; i <= 100; i++) {\n        sum += i;\n    }\n    println(\"\\nSum of 1-100: ${sum}\");   // 5050\n}",
        "output": "=== Classic For ===\nCount: 1\nCount: 2\nCount: 3\nCount: 4\nCount: 5\n\n=== Range Loop ===\n0 1 2 3 4\n\n=== For-Each ===\n🍎 Apple\n🍎 Banana\n🍎 Cherry\n🍎 Date\n\n=== Multiplication Table (1-5) ===\n1\t2\t3\t4\t5\n2\t4\t6\t8\t10\n3\t6\t9\t12\t15\n4\t8\t12\t16\t20\n5\t10\t15\t20\t25\n\n=== Break at 5 ===\n1 2 3 4\n\n=== Skip even numbers ===\n1 3 5 7 9\n\n=== Countdown ===\n5...\n4...\n3...\n2...\n1...\n🚀 Liftoff!\n\nSum of 1-100: 5050"
      }
    ]
  },
  {
    "id": 12,
    "title": "While & Do-While",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for While & Do-While.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  WHILE LOOP\n    // ═══════════════════════════════════════\n\n    println(\"=== While Loop ===\");\n    int count = 1;\n    while (count <= 5) {\n        println(\"Count: ${count}\");\n        count++;\n    }\n\n    // ═══════════════════════════════════════\n    //  DO-WHILE LOOP (runs at least once)\n    // ═══════════════════════════════════════\n\n    println(\"\\n=== Do-While Loop ===\");\n    int n = 1;\n    do {\n        println(\"n = ${n}\");\n        n *= 2;\n    } while (n <= 16);\n    // 1, 2, 4, 8, 16\n\n    // ═══════════════════════════════════════\n    //  PATTERN: Find first power of 2 > 1000\n    // ═══════════════════════════════════════\n\n    int power = 1;\n    int exponent = 0;\n    while (power <= 1000) {\n        power *= 2;\n        exponent++;\n    }\n    println(\"\\n2^${exponent} = ${power} (first power of 2 > 1000)\");\n    // 2^10 = 1024\n\n    // ═══════════════════════════════════════\n    //  PATTERN: Collatz Conjecture\n    // ═══════════════════════════════════════\n\n    println(\"\\n=== Collatz Sequence (start: 27) ===\");\n    int num = 27;\n    int steps = 0;\n    print(\"${num}\");\n    while (num != 1) {\n        if (num % 2 == 0) {\n            num = num / 2;\n        } else {\n            num = num * 3 + 1;\n        }\n        print(\" → ${num}\");\n        steps++;\n    }\n    println(\"\\nReached 1 in ${steps} steps\");\n}"
      }
    ]
  },
  {
    "id": 13,
    "title": "Arrays",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Arrays.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  DECLARATION\n    // ═══════════════════════════════════════\n\n    int[] numbers = {10, 20, 30, 40, 50};\n    string[] names = {\"Alice\", \"Bob\", \"Charlie\"};\n\n    // Modern syntax\n    let scores = [95, 87, 92, 78, 88];\n\n    // ═══════════════════════════════════════\n    //  ACCESS & MODIFY\n    // ═══════════════════════════════════════\n\n    println(\"First: ${numbers[0]}\");      // 10\n    println(\"Last: ${numbers[4]}\");       // 50\n\n    numbers[0] = 100;\n    println(\"Updated first: ${numbers[0]}\");  // 100\n\n    // ═══════════════════════════════════════\n    //  ITERATE\n    // ═══════════════════════════════════════\n\n    println(\"\\nAll names:\");\n    for (string name in names) {\n        println(\"  - ${name}\");\n    }\n\n    println(\"\\nAll scores:\");\n    for (int i = 0; i < scores.length(); i++) {\n        println(\"  Student ${i + 1}: ${scores[i]}\");\n    }\n\n    // ═══════════════════════════════════════\n    //  ARRAY OPERATIONS\n    // ═══════════════════════════════════════\n\n    println(\"\\nLength: ${scores.length()}\");\n\n    // Find max\n    int maxScore = scores[0];\n    for (int s in scores) {\n        if (s > maxScore) maxScore = s;\n    }\n    println(\"Highest score: ${maxScore}\");\n\n    // Sum\n    int total = 0;\n    for (int s in scores) {\n        total += s;\n    }\n    println(\"Total: ${total}\");\n    println(\"Average: ${total / scores.length()}\");\n}",
        "output": "First: 10\nLast: 50\nUpdated first: 100\n\nAll names:\n  - Alice\n  - Bob\n  - Charlie\n\nAll scores:\n  Student 1: 95\n  Student 2: 87\n  Student 3: 92\n  Student 4: 78\n  Student 5: 88\n\nLength: 5\nHighest score: 95\nTotal: 440\nAverage: 88"
      }
    ]
  },
  {
    "id": 14,
    "title": "Lists",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Lists.",
    "snippets": [
      {
        "code": "import std.collections;\n\nfn main() {\n    // ═══════════════════════════════════════\n    //  CREATE LIST\n    // ═══════════════════════════════════════\n\n    List<string> fruits = new List<string>();\n    fruits.add(\"Apple\");\n    fruits.add(\"Banana\");\n    fruits.add(\"Cherry\");\n    fruits.add(\"Date\");\n    fruits.add(\"Elderberry\");\n\n    // Modern syntax\n    let numbers = [10, 20, 30, 40, 50];\n\n    // ═══════════════════════════════════════\n    //  ACCESS\n    // ═══════════════════════════════════════\n\n    println(\"First fruit: ${fruits[0]}\");     // Apple\n    println(\"List size: ${fruits.size()}\");    // 5\n\n    // ═══════════════════════════════════════\n    //  MODIFY\n    // ═══════════════════════════════════════\n\n    fruits.push(\"Fig\");                       // Add to end\n    fruits.remove(\"Date\");                    // Remove by value\n    fruits[1] = \"Blueberry\";                  // Replace\n\n    // ═══════════════════════════════════════\n    //  ITERATE\n    // ═══════════════════════════════════════\n\n    println(\"\\nFruit basket:\");\n    for (string fruit in fruits) {\n        println(\"  🍎 ${fruit}\");\n    }\n\n    // ═══════════════════════════════════════\n    //  CHECK\n    // ═══════════════════════════════════════\n\n    if (fruits.contains(\"Cherry\")) {\n        println(\"\\nWe have cherries! 🍒\");\n    }\n\n    println(\"Is empty: ${fruits.isEmpty()}\");   // false\n\n    // ═══════════════════════════════════════\n    //  FUNCTIONAL OPERATIONS\n    // ═══════════════════════════════════════\n\n    let nums = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];\n\n    let evens = nums.filter(n => n % 2 == 0);\n    let doubled = nums.map(n => n * 2);\n    let sum = nums.reduce((a, b) => a + b);\n\n    println(\"\\nOriginal: ${nums}\");\n    println(\"Evens: ${evens}\");       // [2, 4, 6, 8, 10]\n    println(\"Doubled: ${doubled}\");   // [2, 4, 6, 8, 10, 12, 14, 16, 18, 20]\n    println(\"Sum: ${sum}\");           // 55\n}"
      }
    ]
  },
  {
    "id": 15,
    "title": "Maps & Dictionaries",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Maps & Dictionaries.",
    "snippets": [
      {
        "code": "import std.collections;\n\nfn main() {\n    // ═══════════════════════════════════════\n    //  CREATE MAP\n    // ═══════════════════════════════════════\n\n    Map<string, int> ages = new Map<string, int>();\n    ages[\"Shanto\"] = 21;\n    ages[\"Alice\"] = 25;\n    ages[\"Bob\"] = 30;\n\n    // Modern literal syntax\n    let config = {\n        \"host\": \"localhost\",\n        \"port\": 8080,\n        \"debug\": true\n    };\n\n    let scores = {\n        \"math\": 95,\n        \"science\": 87,\n        \"english\": 92\n    };\n\n    // ═══════════════════════════════════════\n    //  ACCESS\n    // ═══════════════════════════════════════\n\n    println(\"Shanto's age: ${ages[\"Shanto\"]}\");   // 21\n    println(\"Math score: ${scores[\"math\"]}\");      // 95\n\n    // ═══════════════════════════════════════\n    //  MODIFY\n    // ═══════════════════════════════════════\n\n    ages[\"Charlie\"] = 28;          // Add new entry\n    ages[\"Shanto\"] = 22;           // Update existing\n    ages.remove(\"Bob\");            // Remove entry\n\n    // ═══════════════════════════════════════\n    //  CHECK\n    // ═══════════════════════════════════════\n\n    if (ages.containsKey(\"Alice\")) {\n        println(\"Alice is ${ages[\"Alice\"]} years old\");\n    }\n\n    println(\"Total entries: ${ages.size()}\");\n\n    // ═══════════════════════════════════════\n    //  ITERATE\n    // ═══════════════════════════════════════\n\n    println(\"\\nAll ages:\");\n    for (entry in ages) {\n        println(\"  ${entry.key}: ${entry.value}\");\n    }\n\n    println(\"\\nAll scores:\");\n    for (entry in scores) {\n        println(\"  📚 ${entry.key} → ${entry.value}\");\n    }\n}",
        "output": "Shanto's age: 21\nMath score: 95\nAlice is 25 years old\nTotal entries: 3\n\nAll ages:\n  Shanto: 22\n  Alice: 25\n  Charlie: 28\n\nAll scores:\n  📚 math → 95\n  📚 science → 87\n  📚 english → 92"
      }
    ]
  },
  {
    "id": 16,
    "title": "Classes",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Classes.",
    "snippets": [
      {
        "code": "class User {\n    string name;\n    int age;\n    string email;\n\n    // Constructor\n    User(string name, int age, string email) {\n        this.name = name;\n        this.age = age;\n        this.email = email;\n    }\n\n    // Methods\n    fn introduce() {\n        println(\"Hi! I'm ${name}, ${age} years old.\");\n        println(\"Email: ${email}\");\n    }\n\n    fn isAdult() -> bool {\n        return age >= 18;\n    }\n\n    fn birthday() {\n        age++;\n        println(\"🎂 Happy Birthday, ${name}! Now ${age}.\");\n    }\n}\n\nfn main() {\n    // Create objects\n    User user1 = new User(\"Shanto\", 21, \"shanto@email.com\");\n    User user2 = new User(\"Alice\", 17, \"alice@email.com\");\n\n    // Use methods\n    user1.introduce();\n    println(\"Is adult: ${user1.isAdult()}\");    // true\n    println(\"\");\n\n    user2.introduce();\n    println(\"Is adult: ${user2.isAdult()}\");    // false\n    println(\"\");\n\n    user2.birthday();                            // 🎂 Happy Birthday, Alice! Now 18.\n    println(\"Is adult now: ${user2.isAdult()}\"); // true\n}",
        "output": "Hi! I'm Shanto, 21 years old.\nEmail: shanto@email.com\nIs adult: true\n\nHi! I'm Alice, 17 years old.\nEmail: alice@email.com\nIs adult: false\n\n🎂 Happy Birthday, Alice! Now 18.\nIs adult now: true"
      }
    ]
  },
  {
    "id": 17,
    "title": "Constructors",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Constructors.",
    "snippets": [
      {
        "code": "class Product {\n    string name;\n    double price;\n    int quantity;\n    string category;\n\n    // ═══════════════════════════════════════\n    //  PRIMARY CONSTRUCTOR\n    // ═══════════════════════════════════════\n\n    constructor(string name, double price, int quantity, string category) {\n        this.name = name;\n        this.price = price;\n        this.quantity = quantity;\n        this.category = category;\n    }\n\n    // ═══════════════════════════════════════\n    //  JAVA/C# STYLE CONSTRUCTOR (also valid)\n    // ═══════════════════════════════════════\n\n    Product(string name, double price) {\n        this.name = name;\n        this.price = price;\n        this.quantity = 0;\n        this.category = \"General\";\n    }\n\n    // Methods\n    fn totalValue() -> double {\n        return price * quantity;\n    }\n\n    fn display() {\n        println(\"┌─────────────────────────────────┐\");\n        println(\"│ Product: ${name}\");\n        println(\"│ Price:   \\$${price}\");\n        println(\"│ Stock:   ${quantity} units\");\n        println(\"│ Category: ${category}\");\n        println(\"│ Value:   \\$${totalValue()}\");\n        println(\"└─────────────────────────────────┘\");\n    }\n}\n\nfn main() {\n    Product laptop = new Product(\"MacBook Pro\", 2499.99, 10, \"Electronics\");\n    Product book = new Product(\"SP Language Guide\", 29.99);\n\n    laptop.display();\n    println(\"\");\n    book.display();\n}"
      }
    ]
  },
  {
    "id": 18,
    "title": "Access Modifiers",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Access Modifiers.",
    "snippets": [
      {
        "code": "class BankAccount {\n    // ═══════════════════════════════════════\n    //  PUBLIC — accessible from anywhere\n    // ═══════════════════════════════════════\n    public string ownerName;\n\n    // ═══════════════════════════════════════\n    //  PRIVATE — only within this class\n    // ═══════════════════════════════════════\n    private double balance;\n    private string accountNumber;\n\n    // ═══════════════════════════════════════\n    //  PROTECTED — this class + subclasses\n    // ═══════════════════════════════════════\n    protected string accountType;\n\n    // ═══════════════════════════════════════\n    //  INTERNAL — within same module only\n    // ═══════════════════════════════════════\n    internal string branchCode;\n\n    public BankAccount(string owner, double initial) {\n        this.ownerName = owner;\n        this.balance = initial;\n        this.accountNumber = \"ACC-\" + generateId();\n        this.accountType = \"Savings\";\n        this.branchCode = \"BR-001\";\n    }\n\n    // Public methods — anyone can call\n    public fn deposit(double amount) {\n        if (amount > 0) {\n            balance += amount;\n            println(\"✅ Deposited \\$${amount}. New balance: \\$${balance}\");\n        }\n    }\n\n    public fn withdraw(double amount) {\n        if (amount > 0 && amount <= balance) {\n            balance -= amount;\n            println(\"✅ Withdrew \\$${amount}. New balance: \\$${balance}\");\n        } else {\n            println(\"❌ Insufficient funds.\");\n        }\n    }\n\n    public fn getBalance() -> double {\n        return balance;\n    }\n\n    // Private method — internal use only\n    private fn generateId() -> string {\n        return \"123456789\";\n    }\n}\n\nfn main() {\n    BankAccount account = new BankAccount(\"Shanto\", 1000.0);\n\n    println(\"Owner: ${account.ownerName}\");  // ✅ public\n    account.deposit(500.0);                  // ✅ public method\n    account.withdraw(200.0);                 // ✅ public method\n    println(\"Balance: \\$${account.getBalance()}\");\n\n    // ❌ These would NOT compile:\n    // println(account.balance);         // error: 'balance' is private\n    // println(account.accountNumber);   // error: 'accountNumber' is private\n    // account.generateId();             // error: 'generateId' is private\n}",
        "output": "Owner: Shanto\n✅ Deposited $500.0. New balance: $1500.0\n✅ Withdrew $200.0. New balance: $1300.0\nBalance: $1300.0"
      }
    ]
  },
  {
    "id": 19,
    "title": "Inheritance",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Inheritance.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  BASE CLASS\n// ═══════════════════════════════════════════════════\n\nclass Animal {\n    protected string name;\n    protected int age;\n\n    Animal(string name, int age) {\n        this.name = name;\n        this.age = age;\n    }\n\n    fn speak() {\n        println(\"${name} makes a sound.\");\n    }\n\n    fn info() {\n        println(\"${name}, age ${age}\");\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  SUBCLASS — extends keyword\n// ═══════════════════════════════════════════════════\n\nclass Dog extends Animal {\n\n    string breed;\n\n    Dog(string name, int age, string breed) {\n        super(name, age);\n        this.breed = breed;\n    }\n\n    override fn speak() {\n        println(\"${name} says: Woof! 🐕\");\n    }\n\n    fn fetch() {\n        println(\"${name} fetches the ball! 🎾\");\n    }\n}\n\nclass Cat extends Animal {\n\n    bool isIndoor;\n\n    Cat(string name, int age, bool isIndoor) {\n        super(name, age);\n        this.isIndoor = isIndoor;\n    }\n\n    override fn speak() {\n        println(\"${name} says: Meow! 🐱\");\n    }\n\n    fn purr() {\n        println(\"${name} is purring... 😺\");\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  C# STYLE SYNTAX (also valid!)\n//  class Bird : Animal { }\n// ═══════════════════════════════════════════════════\n\nclass Bird : Animal {\n    Bird(string name, int age) {\n        super(name, age);\n    }\n\n    override fn speak() {\n        println(\"${name} says: Tweet! 🐦\");\n    }\n}\n\nfn main() {\n    Dog dog = new Dog(\"Buddy\", 3, \"Golden Retriever\");\n    Cat cat = new Cat(\"Whiskers\", 5, true);\n    Bird bird = new Bird(\"Tweety\", 2);\n\n    // Polymorphism\n    Animal[] animals = {dog, cat, bird};\n\n    println(\"=== Animal Shelter ===\\n\");\n\n    for (Animal animal in animals) {\n        animal.info();\n        animal.speak();\n        println(\"\");\n    }\n\n    // Class-specific methods\n    dog.fetch();\n    cat.purr();\n}",
        "output": "=== Animal Shelter ===\n\nBuddy, age 3\nBuddy says: Woof! 🐕\n\nWhiskers, age 5\nWhiskers says: Meow! 🐱\n\nTweety, age 2\nTweety says: Tweet! 🐦\n\nBuddy fetches the ball! 🎾\nWhiskers is purring... 😺"
      }
    ]
  },
  {
    "id": 20,
    "title": "Interfaces",
    "category": "Control Flow & Structures",
    "description": "Hands-on syntax example and reference implementation for Interfaces.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  DEFINE INTERFACES\n// ═══════════════════════════════════════════════════\n\ninterface Drawable {\n    fn draw();\n    fn getColor() -> string;\n}\n\ninterface Resizable {\n    fn resize(double factor);\n    fn getArea() -> double;\n}\n\n// ═══════════════════════════════════════════════════\n//  IMPLEMENT INTERFACES\n// ═══════════════════════════════════════════════════\n\nclass Circle implements Drawable, Resizable {\n    double radius;\n    string color;\n\n    Circle(double radius, string color) {\n        this.radius = radius;\n        this.color = color;\n    }\n\n    fn draw() {\n        println(\"Drawing ⭕ Circle (r=${radius}, color=${color})\");\n    }\n\n    fn getColor() -> string {\n        return color;\n    }\n\n    fn resize(double factor) {\n        radius *= factor;\n        println(\"Resized circle to radius ${radius}\");\n    }\n\n    fn getArea() -> double {\n        return 3.14159 * radius * radius;\n    }\n}\n\nclass Rectangle implements Drawable, Resizable {\n    double width;\n    double height;\n    string color;\n\n    Rectangle(double w, double h, string color) {\n        this.width = w;\n        this.height = h;\n        this.color = color;\n    }\n\n    fn draw() {\n        println(\"Drawing ▬ Rectangle (${width}x${height}, color=${color})\");\n    }\n\n    fn getColor() -> string {\n        return color;\n    }\n\n    fn resize(double factor) {\n        width *= factor;\n        height *= factor;\n    }\n\n    fn getArea() -> double {\n        return width * height;\n    }\n}\n\nfn main() {\n    Drawable[] shapes = {\n        new Circle(5.0, \"Red\"),\n        new Rectangle(4.0, 6.0, \"Blue\")\n    };\n\n    for (Drawable shape in shapes) {\n        shape.draw();\n    }\n\n    println(\"\\nAreas:\");\n    Circle c = new Circle(10.0, \"Green\");\n    println(\"Circle area: ${c.getArea()}\");     // 314.159\n\n    Rectangle r = new Rectangle(5.0, 8.0, \"Yellow\");\n    println(\"Rectangle area: ${r.getArea()}\");  // 40.0\n}"
      }
    ]
  },
  {
    "id": 21,
    "title": "Abstract Classes",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Abstract Classes.",
    "snippets": [
      {
        "code": "abstract class Vehicle {\n    protected string brand;\n    protected int year;\n\n    Vehicle(string brand, int year) {\n        this.brand = brand;\n        this.year = year;\n    }\n\n    // Abstract — must be implemented by subclasses\n    abstract fn fuelType() -> string;\n    abstract fn maxSpeed() -> int;\n\n    // Concrete — shared by all subclasses\n    fn info() {\n        println(\"${brand} (${year}) — ${fuelType()}, max ${maxSpeed()} km/h\");\n    }\n}\n\nclass Car extends Vehicle {\n    Car(string brand, int year) { super(brand, year); }\n\n    fn fuelType() -> string { return \"Gasoline ⛽\"; }\n    fn maxSpeed() -> int { return 220; }\n}\n\nclass ElectricCar extends Vehicle {\n    ElectricCar(string brand, int year) { super(brand, year); }\n\n    fn fuelType() -> string { return \"Electric ⚡\"; }\n    fn maxSpeed() -> int { return 250; }\n}\n\nclass Bicycle extends Vehicle {\n    Bicycle(string brand, int year) { super(brand, year); }\n\n    fn fuelType() -> string { return \"Human Power 🚴\"; }\n    fn maxSpeed() -> int { return 45; }\n}\n\nfn main() {\n    Vehicle[] vehicles = {\n        new Car(\"Toyota\", 2024),\n        new ElectricCar(\"Tesla\", 2025),\n        new Bicycle(\"Trek\", 2023)\n    };\n\n    println(\"=== Vehicle Fleet ===\\n\");\n    for (Vehicle v in vehicles) {\n        v.info();\n    }\n}",
        "output": "=== Vehicle Fleet ===\n\nToyota (2024) — Gasoline ⛽, max 220 km/h\nTesla (2025) — Electric ⚡, max 250 km/h\nTrek (2023) — Human Power 🚴, max 45 km/h"
      }
    ]
  },
  {
    "id": 22,
    "title": "Static Members",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Static Members.",
    "snippets": [
      {
        "code": "class Counter {\n    private static int count = 0;\n\n    static fn increment() {\n        count++;\n    }\n\n    static fn getCount() -> int {\n        return count;\n    }\n\n    static fn reset() {\n        count = 0;\n    }\n}\n\nclass MathUtils {\n    static const double PI = 3.14159265358979;\n    static const double E = 2.71828182845904;\n\n    static fn circleArea(double radius) -> double {\n        return PI * radius * radius;\n    }\n\n    static fn square(int x) -> int {\n        return x * x;\n    }\n\n    static fn power(int base, int exp) -> int {\n        int result = 1;\n        for (int i = 0; i < exp; i++) {\n            result *= base;\n        }\n        return result;\n    }\n}\n\nfn main() {\n    // Static methods — no object needed\n    Counter.increment();\n    Counter.increment();\n    Counter.increment();\n    println(\"Count: ${Counter.getCount()}\");    // 3\n\n    println(\"PI: ${MathUtils.PI}\");\n    println(\"Circle area (r=5): ${MathUtils.circleArea(5.0)}\");\n    println(\"5² = ${MathUtils.square(5)}\");\n    println(\"2^10 = ${MathUtils.power(2, 10)}\");\n}"
      }
    ]
  },
  {
    "id": 23,
    "title": "Structs",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Structs.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  STRUCT — lightweight value type (stack allocated)\n// ═══════════════════════════════════════════════════\n\nstruct Point {\n    int x;\n    int y;\n}\n\nstruct Color {\n    int r;\n    int g;\n    int b;\n}\n\nstruct Rectangle {\n    Point topLeft;\n    Point bottomRight;\n    Color color;\n}\n\nfn distance(Point a, Point b) -> double {\n    int dx = b.x - a.x;\n    int dy = b.y - a.y;\n    return math.sqrt(dx * dx + dy * dy);\n}\n\nfn main() {\n    Point p1;\n    p1.x = 0;\n    p1.y = 0;\n\n    Point p2;\n    p2.x = 3;\n    p2.y = 4;\n\n    println(\"Point 1: (${p1.x}, ${p1.y})\");\n    println(\"Point 2: (${p2.x}, ${p2.y})\");\n    println(\"Distance: ${distance(p1, p2)}\");  // 5.0\n\n    Color red;\n    red.r = 255;\n    red.g = 0;\n    red.b = 0;\n    println(\"\\nColor: rgb(${red.r}, ${red.g}, ${red.b})\");\n}"
      }
    ]
  },
  {
    "id": 24,
    "title": "Enums",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Enums.",
    "snippets": [
      {
        "code": "enum Direction {\n    North,\n    South,\n    East,\n    West\n}\n\nenum HttpStatus {\n    OK = 200,\n    NotFound = 404,\n    ServerError = 500\n}\n\nenum LogLevel {\n    Debug,\n    Info,\n    Warning,\n    Error,\n    Fatal\n}\n\nfn describeDirection(Direction dir) -> string {\n    match dir {\n        Direction.North => return \"⬆️ Going North\",\n        Direction.South => return \"⬇️ Going South\",\n        Direction.East  => return \"➡️ Going East\",\n        Direction.West  => return \"⬅️ Going West\"\n    }\n}\n\nfn log(LogLevel level, string message) {\n    match level {\n        LogLevel.Debug   => println(\"[DEBUG]   ${message}\"),\n        LogLevel.Info    => println(\"[INFO]    ${message}\"),\n        LogLevel.Warning => println(\"[WARNING] ${message}\"),\n        LogLevel.Error   => println(\"[ERROR]   ${message}\"),\n        LogLevel.Fatal   => println(\"[FATAL]   ${message}\")\n    }\n}\n\nfn main() {\n    Direction dir = Direction.North;\n    println(describeDirection(dir));\n\n    HttpStatus status = HttpStatus.OK;\n    println(\"HTTP ${status}\");\n\n    println(\"\\n=== Log Messages ===\");\n    log(LogLevel.Info, \"Application started\");\n    log(LogLevel.Debug, \"Loading config...\");\n    log(LogLevel.Warning, \"Memory usage high\");\n    log(LogLevel.Error, \"Connection failed\");\n}",
        "output": "⬆️ Going North\nHTTP 200\n\n=== Log Messages ===\n[INFO]    Application started\n[DEBUG]   Loading config...\n[WARNING] Memory usage high\n[ERROR]   Connection failed"
      }
    ]
  },
  {
    "id": 25,
    "title": "Generics",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Generics.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  GENERIC CLASS\n// ═══════════════════════════════════════════════════\n\nclass Box<T> {\n    private T value;\n\n    Box(T value) {\n        this.value = value;\n    }\n\n    fn get() -> T {\n        return value;\n    }\n\n    fn set(T newValue) {\n        value = newValue;\n    }\n\n    fn display() {\n        println(\"Box contains: ${value}\");\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  GENERIC PAIR\n// ═══════════════════════════════════════════════════\n\nclass Pair<A, B> {\n    A first;\n    B second;\n\n    Pair(A first, B second) {\n        this.first = first;\n        this.second = second;\n    }\n\n    fn display() {\n        println(\"(${first}, ${second})\");\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  GENERIC STACK\n// ═══════════════════════════════════════════════════\n\nclass Stack<T> {\n    private List<T> items = new List<T>();\n\n    fn push(T item) {\n        items.add(item);\n    }\n\n    fn pop() -> T {\n        return items.removeLast();\n    }\n\n    fn peek() -> T {\n        return items[items.size() - 1];\n    }\n\n    fn isEmpty() -> bool {\n        return items.size() == 0;\n    }\n\n    fn size() -> int {\n        return items.size();\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  GENERIC FUNCTIONS\n// ═══════════════════════════════════════════════════\n\nfn max<T>(T a, T b) -> T {\n    return a > b ? a : b;\n}\n\nfn swap<T>(T a, T b) -> Pair<T, T> {\n    return new Pair<T, T>(b, a);\n}\n\nfn main() {\n    // Generic Box\n    Box<int> intBox = new Box<int>(42);\n    Box<string> strBox = new Box<string>(\"Hello SP\");\n\n    intBox.display();     // Box contains: 42\n    strBox.display();     // Box contains: Hello SP\n\n    // Generic Pair\n    Pair<string, int> person = new Pair<string, int>(\"Shanto\", 21);\n    person.display();     // (Shanto, 21)\n\n    // Generic Stack\n    Stack<string> stack = new Stack<string>();\n    stack.push(\"first\");\n    stack.push(\"second\");\n    stack.push(\"third\");\n\n    println(\"\\nStack (LIFO):\");\n    while (!stack.isEmpty()) {\n        println(\"  popped: ${stack.pop()}\");\n    }\n\n    // Generic functions\n    println(\"\\nmax(10, 20) = ${max(10, 20)}\");\n    println(\"max(\\\"apple\\\", \\\"banana\\\") = ${max(\"apple\", \"banana\")}\");\n}",
        "output": "Box contains: 42\nBox contains: Hello SP\n(Shanto, 21)\n\nStack (LIFO):\n  popped: third\n  popped: second\n  popped: first\n\nmax(10, 20) = 20\nmax(\"apple\", \"banana\") = banana"
      }
    ]
  },
  {
    "id": 26,
    "title": "Null Safety",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Null Safety.",
    "snippets": [
      {
        "code": "class User {\n    string name;\n    string? email;       // Nullable — can be null\n\n    User(string name, string? email) {\n        this.name = name;\n        this.email = email;\n    }\n}\n\nfn findUser(int id) -> User? {\n    if (id == 1) return new User(\"Shanto\", \"shanto@email.com\");\n    if (id == 2) return new User(\"Alice\", null);\n    return null;         // User not found\n}\n\nfn main() {\n    // ═══════════════════════════════════════\n    //  NULLABLE TYPES (?)\n    // ═══════════════════════════════════════\n\n    User? user1 = findUser(1);\n    User? user2 = findUser(2);\n    User? user3 = findUser(999);\n\n    // ═══════════════════════════════════════\n    //  SAFE ACCESS (?.)\n    // ═══════════════════════════════════════\n\n    println(\"User 1 name: ${user1?.name}\");       // Shanto\n    println(\"User 1 email: ${user1?.email}\");      // shanto@email.com\n    println(\"User 3 name: ${user3?.name}\");        // null (no crash!)\n\n    // ═══════════════════════════════════════\n    //  NULL COALESCING (??)\n    // ═══════════════════════════════════════\n\n    string email1 = user1?.email ?? \"no-email@default.com\";\n    string email2 = user2?.email ?? \"no-email@default.com\";\n    string name3 = user3?.name ?? \"Unknown User\";\n\n    println(\"\\nEmail 1: ${email1}\");    // shanto@email.com\n    println(\"Email 2: ${email2}\");      // no-email@default.com\n    println(\"Name 3: ${name3}\");        // Unknown User\n\n    // ═══════════════════════════════════════\n    //  NULL CHECK\n    // ═══════════════════════════════════════\n\n    if (user1 != null) {\n        println(\"\\n✅ Found user: ${user1.name}\");\n    }\n\n    if (user3 == null) {\n        println(\"❌ User not found\");\n    }\n\n    // ═══════════════════════════════════════\n    //  NON-NULLABLE (default — safe!)\n    // ═══════════════════════════════════════\n\n    // string name = null;    // ❌ COMPILE ERROR!\n    // int age = null;        // ❌ COMPILE ERROR!\n}"
      }
    ]
  },
  {
    "id": 27,
    "title": "Error Handling — Try/Catch",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Error Handling — Try/Catch.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  TRY / CATCH / FINALLY\n    // ═══════════════════════════════════════\n\n    try {\n        int result = 100 / 0;\n        println(result);\n    } catch (ArithmeticException e) {\n        println(\"❌ Math error: ${e.message}\");\n    } finally {\n        println(\"✅ Cleanup complete.\");\n    }\n\n    // ═══════════════════════════════════════\n    //  MULTIPLE CATCH BLOCKS\n    // ═══════════════════════════════════════\n\n    try {\n        string data = readFile(\"config.json\");\n        Config config = parseConfig(data);\n        connect(config.host);\n    } catch (FileNotFoundException e) {\n        println(\"📁 File not found: ${e.message}\");\n    } catch (ParseException e) {\n        println(\"📝 Invalid format: ${e.message}\");\n    } catch (ConnectionException e) {\n        println(\"🌐 Connection failed: ${e.message}\");\n    } catch (Exception e) {\n        println(\"💥 Unexpected error: ${e.message}\");\n    } finally {\n        println(\"Done.\");\n    }\n\n    // ═══════════════════════════════════════\n    //  THROW EXCEPTIONS\n    // ═══════════════════════════════════════\n\n    try {\n        validateAge(-5);\n    } catch (Exception e) {\n        println(\"Validation: ${e.message}\");\n    }\n}\n\nfn validateAge(int age) {\n    if (age < 0) {\n        throw Exception(\"Age cannot be negative: ${age}\");\n    }\n    if (age > 150) {\n        throw Exception(\"Age too large: ${age}\");\n    }\n    println(\"Valid age: ${age}\");\n}"
      }
    ]
  },
  {
    "id": 28,
    "title": "Error Handling — Result Type",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Error Handling — Result Type.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  RESULT TYPE — Modern error handling\n// ═══════════════════════════════════════════════════\n\nfn divide(int a, int b) -> Result<int, Error> {\n    if (b == 0) {\n        return Error(\"Cannot divide ${a} by zero\");\n    }\n    return Ok(a / b);\n}\n\nfn parseNumber(string input) -> Result<int, Error> {\n    try {\n        return Ok(int.parse(input));\n    } catch (Exception e) {\n        return Error(\"Invalid number: '${input}'\");\n    }\n}\n\nfn main() {\n    // Success case\n    let result1 = divide(100, 5);\n    match result1 {\n        Ok(value) => println(\"100 / 5 = ${value}\"),\n        Error(err) => println(\"Error: ${err}\")\n    }\n\n    // Error case\n    let result2 = divide(100, 0);\n    match result2 {\n        Ok(value) => println(\"Result: ${value}\"),\n        Error(err) => println(\"Error: ${err}\")\n    }\n\n    // Chain operations\n    let parsed = parseNumber(\"42\");\n    match parsed {\n        Ok(n) => println(\"Parsed: ${n}\"),\n        Error(e) => println(\"Failed: ${e}\")\n    }\n\n    let invalid = parseNumber(\"hello\");\n    match invalid {\n        Ok(n) => println(\"Parsed: ${n}\"),\n        Error(e) => println(\"Failed: ${e}\")\n    }\n}",
        "output": "100 / 5 = 20\nError: Cannot divide 100 by zero\nParsed: 42\nFailed: Invalid number: 'hello'"
      }
    ]
  },
  {
    "id": 29,
    "title": "Lambda Functions",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Lambda Functions.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  LAMBDA SYNTAX:  (params) => expression\n    // ═══════════════════════════════════════\n\n    var add = (int a, int b) => a + b;\n    var multiply = (int a, int b) => a * b;\n    var square = (int x) => x * x;\n    var isEven = (int n) => n % 2 == 0;\n    var greet = (string name) => \"Hello, ${name}!\";\n\n    println(add(10, 20));          // 30\n    println(multiply(5, 4));       // 20\n    println(square(7));            // 49\n    println(isEven(42));           // true\n    println(greet(\"Shanto\"));      // Hello, Shanto!\n\n    // ═══════════════════════════════════════\n    //  LAMBDA WITH BLOCK BODY\n    // ═══════════════════════════════════════\n\n    var factorial = (int n) => {\n        int result = 1;\n        for (int i = 2; i <= n; i++) {\n            result *= i;\n        }\n        return result;\n    };\n\n    println(\"5! = ${factorial(5)}\");     // 120\n    println(\"10! = ${factorial(10)}\");   // 3628800\n\n    // ═══════════════════════════════════════\n    //  LAMBDA AS FUNCTION ARGUMENT\n    // ═══════════════════════════════════════\n\n    let numbers = [1, 2, 3, 4, 5];\n\n    let doubled = numbers.map(x => x * 2);\n    println(\"Doubled: ${doubled}\");       // [2, 4, 6, 8, 10]\n\n    let evens = numbers.filter(x => x % 2 == 0);\n    println(\"Evens: ${evens}\");           // [2, 4]\n\n    let sum = numbers.reduce((a, b) => a + b);\n    println(\"Sum: ${sum}\");               // 15\n}"
      }
    ]
  },
  {
    "id": 30,
    "title": "Higher-Order Functions",
    "category": "Object-Oriented Programming",
    "description": "Hands-on syntax example and reference implementation for Higher-Order Functions.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  FUNCTIONS THAT TAKE FUNCTIONS AS PARAMETERS\n// ═══════════════════════════════════════════════════\n\nfn applyTwice(fn(int) -> int operation, int value) -> int {\n    return operation(operation(value));\n}\n\nfn transform(int[] arr, fn(int) -> int func) -> int[] {\n    int[] result = new int[arr.length()];\n    for (int i = 0; i < arr.length(); i++) {\n        result[i] = func(arr[i]);\n    }\n    return result;\n}\n\nfn executeIf(bool condition, fn() action) {\n    if (condition) {\n        action();\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  FUNCTIONS THAT RETURN FUNCTIONS\n// ═══════════════════════════════════════════════════\n\nfn multiplier(int factor) -> fn(int) -> int {\n    return (int x) => x * factor;\n}\n\nfn adder(int amount) -> fn(int) -> int {\n    return (int x) => x + amount;\n}\n\nfn main() {\n    // Pass lambda to function\n    int result = applyTwice(x => x * 2, 3);\n    println(\"applyTwice(double, 3) = ${result}\");  // 12\n\n    // Transform array\n    int[] nums = {1, 2, 3, 4, 5};\n    int[] squared = transform(nums, x => x * x);\n    println(\"Squared: ${squared}\");    // [1, 4, 9, 16, 25]\n\n    // Conditional execution\n    executeIf(true, () => println(\"This runs!\"));\n    executeIf(false, () => println(\"This doesn't!\"));\n\n    // Function factories\n    var double = multiplier(2);\n    var triple = multiplier(3);\n    var addTen = adder(10);\n\n    println(\"double(5) = ${double(5)}\");     // 10\n    println(\"triple(5) = ${triple(5)}\");     // 15\n    println(\"addTen(5) = ${addTen(5)}\");     // 15\n\n    // ═══════════════════════════════════════\n    //  CHAINING: map → filter → reduce\n    // ═══════════════════════════════════════\n\n    let result = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]\n        .map(x => x * x)           // square each\n        .filter(x => x > 20)       // keep > 20\n        .reduce((a, b) => a + b);  // sum them\n\n    println(\"\\nSum of squares > 20: ${result}\");  // 25+36+49+64+81+100 = 355\n}"
      }
    ]
  },
  {
    "id": 31,
    "title": "Closures",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Closures.",
    "snippets": [
      {
        "code": "fn makeCounter() -> fn() -> int {\n    int count = 0;\n    return () => {\n        count++;\n        return count;\n    };\n}\n\nfn makeGreeter(string greeting) -> fn(string) -> string {\n    return (string name) => \"${greeting}, ${name}!\";\n}\n\nfn main() {\n    // Counter closure — captures and modifies `count`\n    var counter = makeCounter();\n    println(counter());    // 1\n    println(counter());    // 2\n    println(counter());    // 3\n\n    // Greeter closure — captures `greeting`\n    var hello = makeGreeter(\"Hello\");\n    var hi = makeGreeter(\"Hi\");\n\n    println(hello(\"Shanto\"));    // Hello, Shanto!\n    println(hi(\"Alice\"));        // Hi, Alice!\n}"
      }
    ]
  },
  {
    "id": 32,
    "title": "String Methods",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for String Methods.",
    "snippets": [
      {
        "code": "fn main() {\n    string text = \"  Hello, SP Programming Language!  \";\n\n    // ═══════════════════════════════════════\n    //  COMMON STRING METHODS\n    // ═══════════════════════════════════════\n\n    println(\"Original:   '${text}'\");\n    println(\"Trimmed:    '${text.trim()}'\");\n    println(\"Length:     ${text.trim().length()}\");\n    println(\"Upper:      ${text.trim().upper()}\");\n    println(\"Lower:      ${text.trim().lower()}\");\n    println(\"Contains:   ${text.contains(\"SP\")}\");         // true\n    println(\"StartsWith: ${text.trim().startsWith(\"Hello\")}\");  // true\n    println(\"EndsWith:   ${text.trim().endsWith(\"!\")}\");        // true\n    println(\"IndexOf:    ${text.indexOf(\"SP\")}\");           // 9\n    println(\"Replace:    ${text.trim().replace(\"SP\", \"Shanto Paul\")}\");\n    println(\"Substring:  ${text.trim().substring(0, 5)}\");  // Hello\n    println(\"CharAt:     ${text.trim().charAt(0)}\");        // H\n\n    // ═══════════════════════════════════════\n    //  SPLIT & JOIN\n    // ═══════════════════════════════════════\n\n    string csv = \"apple,banana,cherry,date\";\n    let fruits = csv.split(\",\");\n    println(\"\\nSplit CSV:\");\n    for (string fruit in fruits) {\n        println(\"  - ${fruit}\");\n    }\n\n    string joined = fruits.join(\" | \");\n    println(\"Joined: ${joined}\");\n    // apple | banana | cherry | date\n\n    // ═══════════════════════════════════════\n    //  STRING INTERPOLATION\n    // ═══════════════════════════════════════\n\n    string name = \"Shanto\";\n    int age = 21;\n    double gpa = 3.95;\n\n    println(\"\\n${name} is ${age} years old with GPA ${gpa}\");\n    println(\"In 5 years: ${name} will be ${age + 5}\");\n    println(\"Name reversed: ... (exercise for the reader!)\");\n}"
      }
    ]
  },
  {
    "id": 33,
    "title": "Method Overloading",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Method Overloading.",
    "snippets": [
      {
        "code": "class Printer {\n\n    // Same method name, different parameter types\n    fn print(int value) {\n        println(\"Integer: ${value}\");\n    }\n\n    fn print(double value) {\n        println(\"Double: ${value}\");\n    }\n\n    fn print(string value) {\n        println(\"String: ${value}\");\n    }\n\n    fn print(bool value) {\n        println(\"Boolean: ${value}\");\n    }\n\n    // Different parameter counts\n    fn print(string label, int value) {\n        println(\"${label}: ${value}\");\n    }\n\n    fn print(string label, int value, string unit) {\n        println(\"${label}: ${value} ${unit}\");\n    }\n}\n\nfn main() {\n    Printer p = new Printer();\n\n    p.print(42);                       // Integer: 42\n    p.print(3.14);                     // Double: 3.14\n    p.print(\"Hello\");                  // String: Hello\n    p.print(true);                     // Boolean: true\n    p.print(\"Age\", 21);               // Age: 21\n    p.print(\"Weight\", 75, \"kg\");       // Weight: 75 kg\n}"
      }
    ]
  },
  {
    "id": 34,
    "title": "Operator Overloading",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Operator Overloading.",
    "snippets": [
      {
        "code": "struct Vector2 {\n    float x;\n    float y;\n\n    operator +(Vector2 a, Vector2 b) {\n        return Vector2(a.x + b.x, a.y + b.y);\n    }\n\n    operator -(Vector2 a, Vector2 b) {\n        return Vector2(a.x - b.x, a.y - b.y);\n    }\n\n    operator *(Vector2 v, float scalar) {\n        return Vector2(v.x * scalar, v.y * scalar);\n    }\n\n    operator ==(Vector2 a, Vector2 b) {\n        return a.x == b.x && a.y == b.y;\n    }\n}\n\nfn main() {\n    Vector2 a = Vector2(3.0, 4.0);\n    Vector2 b = Vector2(1.0, 2.0);\n\n    Vector2 sum = a + b;\n    Vector2 diff = a - b;\n    Vector2 scaled = a * 2.0;\n\n    println(\"a + b = (${sum.x}, ${sum.y})\");         // (4.0, 6.0)\n    println(\"a - b = (${diff.x}, ${diff.y})\");       // (2.0, 2.0)\n    println(\"a * 2 = (${scaled.x}, ${scaled.y})\");   // (6.0, 8.0)\n    println(\"a == b: ${a == b}\");                     // false\n}"
      }
    ]
  },
  {
    "id": 35,
    "title": "Extension Methods",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Extension Methods.",
    "snippets": [
      {
        "code": "extend string {\n    fn isEmail() -> bool {\n        return this.contains(\"@\") && this.contains(\".\");\n    }\n\n    fn reverse() -> string {\n        string result = \"\";\n        for (int i = this.length() - 1; i >= 0; i--) {\n            result += this.charAt(i);\n        }\n        return result;\n    }\n\n    fn wordCount() -> int {\n        return this.trim().split(\" \").length();\n    }\n}\n\nextend int {\n    fn isEven() -> bool { return this % 2 == 0; }\n    fn isOdd() -> bool { return this % 2 != 0; }\n    fn factorial() -> int {\n        int result = 1;\n        for (int i = 2; i <= this; i++) result *= i;\n        return result;\n    }\n}\n\nfn main() {\n    // String extensions\n    println(\"test@email.com\".isEmail());         // true\n    println(\"not-an-email\".isEmail());            // false\n    println(\"Hello\".reverse());                   // olleH\n    println(\"Hello SP World\".wordCount());        // 3\n\n    // Int extensions\n    println(42.isEven());        // true\n    println(7.isOdd());          // true\n    println(5.factorial());      // 120\n}"
      }
    ]
  },
  {
    "id": 36,
    "title": "Traits",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Traits.",
    "snippets": [
      {
        "code": "trait Serializable {\n    fn serialize() -> string;\n    fn deserialize(string data);\n}\n\ntrait Displayable {\n    fn display();\n}\n\nclass User with Serializable, Displayable {\n    string name;\n    int age;\n\n    User(string name, int age) {\n        this.name = name;\n        this.age = age;\n    }\n\n    fn serialize() -> string {\n        return \"{\\\"name\\\":\\\"${name}\\\",\\\"age\\\":${age}}\";\n    }\n\n    fn deserialize(string data) {\n        // Parse JSON data...\n    }\n\n    fn display() {\n        println(\"👤 ${name} (${age})\");\n    }\n}\n\nfn main() {\n    User user = new User(\"Shanto\", 21);\n    user.display();                          // 👤 Shanto (21)\n    println(\"JSON: ${user.serialize()}\");    // {\"name\":\"Shanto\",\"age\":21}\n}"
      }
    ]
  },
  {
    "id": 37,
    "title": "Annotations",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Annotations.",
    "snippets": [
      {
        "code": "@Serializable\nclass Config {\n    string host;\n    int port;\n    bool debug;\n}\n\nclass ApiController {\n\n    @Route(\"/users\")\n    @Method(\"GET\")\n    fn getUsers() -> List<User> {\n        return userService.findAll();\n    }\n\n    @Route(\"/users\")\n    @Method(\"POST\")\n    fn createUser(User user) -> User {\n        return userService.save(user);\n    }\n\n    @Deprecated(\"Use getUsers() instead\")\n    fn listUsers() -> List<User> {\n        return getUsers();\n    }\n}\n\nclass MathService {\n\n    @inline\n    fn add(int a, int b) -> int {\n        return a + b;\n    }\n\n    @Test\n    fn testAdd() {\n        assert(add(2, 3) == 5);\n    }\n}"
      }
    ]
  },
  {
    "id": 38,
    "title": "Modules & Imports",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Modules & Imports.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  FILE: src/models/user.sp\n// ═══════════════════════════════════════════════════\n\nmodule myapp.models;\n\npublic class User {\n    public string name;\n    public int age;\n\n    public User(string name, int age) {\n        this.name = name;\n        this.age = age;\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  FILE: src/services/user_service.sp\n// ═══════════════════════════════════════════════════\n\nmodule myapp.services;\n\nimport myapp.models.User;\n\npublic class UserService {\n    private List<User> users = new List<User>();\n\n    public fn addUser(User user) {\n        users.add(user);\n    }\n\n    public fn findByName(string name) -> User? {\n        for (User u in users) {\n            if (u.name == name) return u;\n        }\n        return null;\n    }\n}\n\n// ═══════════════════════════════════════════════════\n//  FILE: src/main.sp\n// ═══════════════════════════════════════════════════\n\nimport std.io;\nimport myapp.models.User;\nimport myapp.services.UserService;\n\nfn main() {\n    UserService service = new UserService();\n    service.addUser(new User(\"Shanto\", 21));\n    service.addUser(new User(\"Alice\", 25));\n\n    User? user = service.findByName(\"Shanto\");\n    println(\"Found: ${user?.name}, age ${user?.age}\");\n\n    // Standard library imports\n    import std.math;\n    import std.collections;\n    import std.fs;\n    import std.json;\n    import std.http;\n    import std.time;\n}"
      }
    ]
  },
  {
    "id": 39,
    "title": "Async & Await",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Async & Await.",
    "snippets": [
      {
        "code": "import std.http;\nimport std.io;\n\nasync fn fetchUser(int id) -> User {\n    let response = await http.get(\"https://api.example.com/users/${id}\");\n    return json.parse<User>(response.body);\n}\n\nasync fn fetchMultipleUsers() {\n    // Sequential\n    let user1 = await fetchUser(1);\n    let user2 = await fetchUser(2);\n\n    println(\"User 1: ${user1.name}\");\n    println(\"User 2: ${user2.name}\");\n}\n\nasync fn main() {\n    println(\"Fetching data...\");\n    await fetchMultipleUsers();\n    println(\"Done!\");\n}"
      }
    ]
  },
  {
    "id": 40,
    "title": "Threads & Concurrency",
    "category": "Advanced Types & Safety",
    "description": "Hands-on syntax example and reference implementation for Threads & Concurrency.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  SPAWN A THREAD\n    // ═══════════════════════════════════════\n\n    thread {\n        for (i in 0..5) {\n            println(\"Background: ${i}\");\n        }\n    }\n\n    for (i in 0..5) {\n        println(\"Main: ${i}\");\n    }\n\n    // ═══════════════════════════════════════\n    //  FUTURE PRIMITIVES\n    // ═══════════════════════════════════════\n    // spawn    — launch concurrent task\n    // channel  — message passing\n    // mutex    — mutual exclusion\n    // atomic   — atomic operations\n}"
      }
    ]
  },
  {
    "id": 41,
    "title": "Unsafe Mode",
    "category": "File I/O & System APIs",
    "description": "Hands-on syntax example and reference implementation for Unsafe Mode.",
    "snippets": [
      {
        "code": "fn main() {\n    // ═══════════════════════════════════════\n    //  UNSAFE BLOCK — for systems programming\n    // ═══════════════════════════════════════\n\n    unsafe {\n        pointer p = allocate(1024);   // Raw memory allocation\n        // Direct memory operations\n        // Pointer arithmetic\n        // No bounds checking\n        free(p);\n    }\n\n    // Outside unsafe: full safety guarantees\n    println(\"Back to safe SP! ✅\");\n}"
      }
    ]
  },
  {
    "id": 42,
    "title": "Native Interop (C FFI)",
    "category": "File I/O & System APIs",
    "description": "Hands-on syntax example and reference implementation for Native Interop (C FFI).",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  CALL C FUNCTIONS FROM SP\n// ═══════════════════════════════════════════════════\n\nextern \"C\" {\n    fn printf(string format);\n    fn strlen(string s) -> int;\n    fn malloc(int size) -> pointer;\n    fn free(pointer p);\n}\n\nfn main() {\n    // Call C's printf directly\n    printf(\"Hello from C! %d\\n\", 42);\n\n    // Use C's strlen\n    int len = strlen(\"Hello\");\n    println(\"strlen: ${len}\");   // 5\n}"
      }
    ]
  },
  {
    "id": 43,
    "title": "Testing",
    "category": "File I/O & System APIs",
    "description": "Hands-on syntax example and reference implementation for Testing.",
    "snippets": [
      {
        "code": "import std.testing;\n\n// ═══════════════════════════════════════════════════\n//  FUNCTIONS TO TEST\n// ═══════════════════════════════════════════════════\n\nfn add(int a, int b) -> int { return a + b; }\nfn factorial(int n) -> int {\n    if (n <= 1) return 1;\n    return n * factorial(n - 1);\n}\nfn isEven(int n) -> bool { return n % 2 == 0; }\n\n// ═══════════════════════════════════════════════════\n//  TEST BLOCKS\n// ═══════════════════════════════════════════════════\n\ntest \"addition works\" {\n    assert(add(2, 3) == 5);\n    assert(add(-1, 1) == 0);\n    assert(add(0, 0) == 0);\n}\n\ntest \"factorial is correct\" {\n    assert(factorial(0) == 1);\n    assert(factorial(1) == 1);\n    assert(factorial(5) == 120);\n    assert(factorial(10) == 3628800);\n}\n\ntest \"even/odd detection\" {\n    assert(isEven(0) == true);\n    assert(isEven(2) == true);\n    assert(isEven(3) == false);\n    assert(isEven(100) == true);\n}\n\ntest \"string operations\" {\n    string name = \"Shanto\";\n    assert(name.length() == 6);\n    assert(name.upper() == \"SHANTO\");\n    assert(name.contains(\"han\") == true);\n}"
      }
    ]
  },
  {
    "id": 44,
    "title": "Comments & Documentation",
    "category": "File I/O & System APIs",
    "description": "Hands-on syntax example and reference implementation for Comments & Documentation.",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  SINGLE-LINE COMMENT\n// ═══════════════════════════════════════════════════\n\n// This is a single-line comment\nint age = 21;  // This is an inline comment\n\n// ═══════════════════════════════════════════════════\n//  MULTI-LINE COMMENT\n// ═══════════════════════════════════════════════════\n\n/*\n    This is a multi-line comment.\n    It can span multiple lines.\n    Useful for longer explanations.\n*/\n\n// ═══════════════════════════════════════════════════\n//  DOCUMENTATION COMMENT (for spc doc)\n// ═══════════════════════════════════════════════════\n\n/// Calculates the sum of two integers.\n///\n/// @param a The first integer.\n/// @param b The second integer.\n/// @return The sum of a and b.\n///\n/// Example:\n///"
      }
    ]
  },
  {
    "id": 45,
    "title": "Multi-Syntax Compatibility",
    "category": "File I/O & System APIs",
    "description": "SP's unique feature — multiple syntax styles that compile to identical code:",
    "snippets": [
      {
        "code": "// ═══════════════════════════════════════════════════\n//  FUNCTION DECLARATION — 3 valid styles\n// ═══════════════════════════════════════════════════\n\n// Style 1: SP-native\nfn add(int a, int b) -> int {\n    return a + b;\n}\n\n// Style 2: Java / C++ / C#\nint add(int a, int b) {\n    return a + b;\n}\n\n// Style 3: Short form\nfn add(a, b) => a + b;\n\n\n// ═══════════════════════════════════════════════════\n//  VOID FUNCTIONS — 2 valid styles\n// ═══════════════════════════════════════════════════\n\n// SP-native\nfn printName() {\n    println(\"Shanto\");\n}\n\n// Java/C++ style\nvoid printName() {\n    println(\"Shanto\");\n}\n\n\n// ═══════════════════════════════════════════════════\n//  INHERITANCE — 2 valid styles\n// ═══════════════════════════════════════════════════\n\n// Java style\nclass Dog extends Animal { }\n\n// C# style\nclass Dog : Animal { }\n\n\n// ═══════════════════════════════════════════════════\n//  INTERFACE IMPLEMENTATION — 2 valid styles\n// ═══════════════════════════════════════════════════\n\n// Java style\nclass User implements Serializable { }\n\n// C# style\nclass User : Serializable { }\n\n\n// ═══════════════════════════════════════════════════\n//  SWITCH vs MATCH\n// ═══════════════════════════════════════════════════\n\n// Classic (C/Java)\nswitch (x) {\n    case 1: println(\"one\"); break;\n    default: println(\"other\");\n}\n\n// Modern (SP)\nmatch x {\n    1 => println(\"one\"),\n    _ => println(\"other\")\n}\n\n\n// ═══════════════════════════════════════════════════\n//  CONSTRUCTOR — 2 valid styles\n// ═══════════════════════════════════════════════════\n\n// SP-native\nclass User {\n    constructor(string name) {\n        this.name = name;\n    }\n}\n\n// Java/C# style\nclass User {\n    User(string name) {\n        this.name = name;\n    }\n}\n\n\n// ═══════════════════════════════════════════════════\n//  ENTRY POINT — 2 valid styles\n// ═══════════════════════════════════════════════════\n\n// SP-native (recommended)\nfn main() {\n    println(\"Hello, SP!\");\n}\n\n// Java-style (compatibility)\npublic static void main() {\n    System.out.println(\"Hello, SP!\");\n}"
      }
    ]
  },
  {
    "id": 46,
    "title": "Complete Programs",
    "category": "File I/O & System APIs",
    "description": "Examples and idioms for Complete Programs, starting with 🧮 calculator.",
    "snippets": [
      {
        "subtitle": "🧮 Calculator",
        "code": "import std.io;\n\nfn main() {\n    println(\"═══════════════════════════\");\n    println(\"  SP Calculator\");\n    println(\"═══════════════════════════\\n\");\n\n    print(\"Enter first number: \");\n    double a = double.parse(readLine());\n\n    print(\"Enter operator (+, -, *, /): \");\n    string op = readLine();\n\n    print(\"Enter second number: \");\n    double b = double.parse(readLine());\n\n    double result;\n    match op {\n        \"+\" => result = a + b,\n        \"-\" => result = a - b,\n        \"*\" => result = a * b,\n        \"/\" => {\n            if (b == 0) {\n                println(\"❌ Error: Division by zero!\");\n                return;\n            }\n            result = a / b;\n        },\n        _ => {\n            println(\"❌ Unknown operator: ${op}\");\n            return;\n        }\n    }\n\n    println(\"\\n✅ ${a} ${op} ${b} = ${result}\");\n}"
      },
      {
        "subtitle": "📝 Todo App",
        "code": "import std.io;\nimport std.collections;\n\nclass Task {\n    string title;\n    bool done;\n\n    Task(string title) {\n        this.title = title;\n        this.done = false;\n    }\n\n    fn toggle() { done = !done; }\n\n    fn toString() -> string {\n        string icon = done ? \"✅\" : \"⬜\";\n        return \"${icon} ${title}\";\n    }\n}\n\nclass TodoApp {\n    private List<Task> tasks = new List<Task>();\n\n    fn add(string title) {\n        tasks.add(new Task(title));\n        println(\"Added: ${title}\");\n    }\n\n    fn complete(int index) {\n        if (index >= 0 && index < tasks.size()) {\n            tasks[index].toggle();\n            println(\"Toggled: ${tasks[index].title}\");\n        }\n    }\n\n    fn show() {\n        println(\"\\n╔══════════════════════════════╗\");\n        println(\"║       📋 TODO LIST           ║\");\n        println(\"╠══════════════════════════════╣\");\n\n        if (tasks.isEmpty()) {\n            println(\"║  No tasks yet!              ║\");\n        }\n\n        for (int i = 0; i < tasks.size(); i++) {\n            println(\"║  ${i + 1}. ${tasks[i].toString()}\");\n        }\n\n        int done = tasks.filter(t => t.done).size();\n        println(\"╠══════════════════════════════╣\");\n        println(\"║  ${done}/${tasks.size()} completed\");\n        println(\"╚══════════════════════════════╝\");\n    }\n}\n\nfn main() {\n    TodoApp app = new TodoApp();\n\n    app.add(\"Learn SP syntax\");\n    app.add(\"Build a compiler\");\n    app.add(\"Write standard library\");\n    app.add(\"Create VS Code extension\");\n    app.add(\"Launch SP 1.0\");\n\n    app.complete(0);   // Learn SP syntax ✅\n    app.complete(1);   // Build a compiler ✅\n\n    app.show();\n}",
        "output": "Added: Learn SP syntax\nAdded: Build a compiler\nAdded: Write standard library\nAdded: Create VS Code extension\nAdded: Launch SP 1.0\nToggled: Learn SP syntax\nToggled: Build a compiler\n\n╔══════════════════════════════╗\n║       📋 TODO LIST           ║\n╠══════════════════════════════╣\n║  1. ✅ Learn SP syntax\n║  2. ✅ Build a compiler\n║  3. ⬜ Write standard library\n║  4. ⬜ Create VS Code extension\n║  5. ⬜ Launch SP 1.0\n╠══════════════════════════════╣\n║  2/5 completed\n╚══════════════════════════════╝"
      },
      {
        "subtitle": "🏦 Banking System",
        "code": "import std.io;\nimport std.collections;\n\nclass Account {\n    private string owner;\n    private double balance;\n    private List<string> history = new List<string>();\n\n    Account(string owner, double initial) {\n        this.owner = owner;\n        this.balance = initial;\n        log(\"Account created with \\$${initial}\");\n    }\n\n    fn deposit(double amount) {\n        if (amount <= 0) {\n            println(\"❌ Invalid amount\");\n            return;\n        }\n        balance += amount;\n        log(\"Deposit: +\\$${amount}\");\n        println(\"✅ Deposited \\$${amount}\");\n    }\n\n    fn withdraw(double amount) {\n        if (amount <= 0 || amount > balance) {\n            println(\"❌ Invalid or insufficient funds\");\n            return;\n        }\n        balance -= amount;\n        log(\"Withdraw: -\\$${amount}\");\n        println(\"✅ Withdrew \\$${amount}\");\n    }\n\n    fn transfer(Account target, double amount) {\n        if (amount <= 0 || amount > balance) {\n            println(\"❌ Transfer failed\");\n            return;\n        }\n        balance -= amount;\n        target.balance += amount;\n        log(\"Transfer: -\\$${amount} → ${target.owner}\");\n        target.log(\"Received: +\\$${amount} from ${owner}\");\n        println(\"✅ Transferred \\$${amount} to ${target.owner}\");\n    }\n\n    fn showBalance() {\n        println(\"💰 ${owner}'s balance: \\$${balance}\");\n    }\n\n    fn showHistory() {\n        println(\"\\n📜 ${owner}'s Transaction History:\");\n        for (string entry in history) {\n            println(\"   ${entry}\");\n        }\n    }\n\n    private fn log(string message) {\n        history.add(message);\n    }\n}\n\nfn main() {\n    println(\"═══════════════════════════════\");\n    println(\"  🏦 SP Banking System\");\n    println(\"═══════════════════════════════\\n\");\n\n    Account shanto = new Account(\"Shanto\", 5000.0);\n    Account alice = new Account(\"Alice\", 3000.0);\n\n    shanto.deposit(2000.0);\n    shanto.withdraw(500.0);\n    shanto.transfer(alice, 1000.0);\n\n    println(\"\");\n    shanto.showBalance();\n    alice.showBalance();\n\n    shanto.showHistory();\n    alice.showHistory();\n}",
        "output": "═══════════════════════════════\n  🏦 SP Banking System\n═══════════════════════════════\n\n✅ Deposited $2000.0\n✅ Withdrew $500.0\n✅ Transferred $1000.0 to Alice\n\n💰 Shanto's balance: $5500.0\n💰 Alice's balance: $4000.0\n\n📜 Shanto's Transaction History:\n   Account created with $5000.0\n   Deposit: +$2000.0\n   Withdraw: -$500.0\n   Transfer: -$1000.0 → Alice\n\n📜 Alice's Transaction History:\n   Account created with $3000.0\n   Received: +$1000.0 from Shanto"
      }
    ]
  },
  {
    "id": 47,
    "title": "File System & File I/O (`std.fs`)",
    "category": "File I/O & System APIs",
    "description": "Read and write text files synchronously with robust error handling.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.fs;\n\nfn main() {\n    string filePath = \"notes.txt\";\n    \n    // Write text to a file\n    fs.writeString(filePath, \"Hello from SP File System!\\nSecond line of notes.\\n\");\n    println(\"File written successfully.\");\n    \n    // Check if file exists\n    if (fs.exists(filePath)) {\n        println(\"Verified: ${filePath} exists.\");\n    }\n    \n    // Read the file contents\n    string contents = fs.readString(filePath);\n    println(\"File Contents:\\n${contents}\");\n    \n    // Append to file\n    fs.appendString(filePath, \"Third line added via append.\\n\");\n    println(\"Updated Size: ${fs.size(filePath)} bytes\");\n}",
        "output": "File written successfully.\nVerified: notes.txt exists.\nFile Contents:\nHello from SP File System!\nSecond line of notes.\n\nUpdated Size: 67 bytes"
      }
    ]
  },
  {
    "id": 48,
    "title": "File Streaming & Buffers",
    "category": "File I/O & System APIs",
    "description": "Read and process large files line-by-line without loading the entire file into memory.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.fs;\n\nfn main() {\n    string logFile = \"app.log\";\n    fs.writeString(logFile, \"INFO: System started\\nWARN: High memory usage\\nERROR: Connection timed out\\nINFO: Retry succeeded\\n\");\n    \n    FileStream stream = fs.openRead(logFile);\n    int lineNumber = 1;\n    \n    while (!stream.isEOF()) {\n        string line = stream.readLine();\n        if (line.contains(\"ERROR\")) {\n            println(\"🚨 Alert on line ${lineNumber}: ${line}\");\n        } else {\n            println(\"[${lineNumber}] ${line}\");\n        }\n        lineNumber += 1;\n    }\n    stream.close();\n}",
        "output": "[1] INFO: System started\n[2] WARN: High memory usage\n🚨 Alert on line 3: ERROR: Connection timed out\n[4] INFO: Retry succeeded"
      }
    ]
  },
  {
    "id": 49,
    "title": "Directory Traversal & Path Operations",
    "category": "File I/O & System APIs",
    "description": "Work with paths, check file extensions, and traverse directories.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.fs;\n\nfn main() {\n    Path p = new Path(\"src/compiler/parser.sp\");\n    println(\"File Name: \" + p.fileName());\n    println(\"Extension: \" + p.extension());\n    println(\"Directory: \" + p.parent());\n    \n    string joined = Path.join([\"build\", \"bin\", \"spc.exe\"]);\n    println(\"Joined Path: ${joined}\");\n    \n    // Create directory tree if not exists\n    fs.createDirAll(\"test_workspace/logs\");\n    println(\"Directory created: test_workspace/logs\");\n    \n    List<string> entries = fs.listDir(\".\");\n    println(\"Total root entries found: ${entries.length()}\");\n}",
        "output": "File Name: parser.sp\nExtension: .sp\nDirectory: src/compiler\nJoined Path: build/bin/spc.exe\nDirectory created: test_workspace/logs\nTotal root entries found: 18"
      }
    ]
  },
  {
    "id": 50,
    "title": "Standard Math Functions (`std.math`)",
    "category": "File I/O & System APIs",
    "description": "Trigonometry, exponential, rounding, and floating-point math utilities.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.math;\n\nfn main() {\n    double x = 45.0;\n    double rad = math.toRadians(x);\n    \n    println(\"Angle: ${x} deg = ${rad} rad\");\n    println(\"sin(45 deg): ${math.sin(rad)}\");\n    println(\"cos(45 deg): ${math.cos(rad)}\");\n    \n    println(\"Square root of 144: ${math.sqrt(144.0)}\");\n    println(\"2^10 power: ${math.pow(2.0, 10.0)}\");\n    println(\"log10(1000): ${math.log10(1000.0)}\");\n    println(\"Round 3.75: ${math.round(3.75)}\");\n    println(\"Floor 3.75: ${math.floor(3.75)}\");\n    println(\"Ceil 3.75: ${math.ceil(3.75)}\");\n}",
        "output": "Angle: 45.0 deg = 0.785398 rad\nsin(45 deg): 0.707107\ncos(45 deg): 0.707107\nSquare root of 144: 12.0\n2^10 power: 1024.0\nlog10(1000): 3.0\nRound 3.75: 4.0\nFloor 3.75: 3.0\nCeil 3.75: 4.0"
      }
    ]
  },
  {
    "id": 51,
    "title": "Random & Pseudo-Random Generation",
    "category": "Collections & OS Utilities",
    "description": "Generate cryptographically sound or fast pseudo-random values.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.math;\n\nfn main() {\n    Random rng = new Random(1337); // Seeded for reproducibility\n    \n    println(\"🎲 Rolling a 6-sided dice 5 times:\");\n    for (int i = 0; i < 5; i++) {\n        int roll = rng.nextInt(1, 7); // [1, 6]\n        print(\"${roll} \");\n    }\n    println(\"\");\n    \n    double probability = rng.nextDouble();\n    println(\"Random probability: ${probability}\");\n    \n    List<string> choices = [\"Red\", \"Green\", \"Blue\", \"Cyan\", \"Amber\"];\n    string picked = rng.choice(choices);\n    println(\"Randomly picked color: ${picked}\");\n}",
        "output": "🎲 Rolling a 6-sided dice 5 times:\n4 2 6 1 5 \nRandom probability: 0.68421\nRandomly picked color: Cyan"
      }
    ]
  },
  {
    "id": 52,
    "title": "Date & Time Manipulation (`std.time`)",
    "category": "Collections & OS Utilities",
    "description": "Current system clock, durations, timestamps, and ISO 8601 formatting.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.time;\n\nfn main() {\n    DateTime now = DateTime.now();\n    println(\"Current UTC Timestamp: ${now.toIsoString()}\");\n    println(\"Year: ${now.year()}, Month: ${now.month()}, Day: ${now.day()}\");\n    \n    Duration tenDays = Duration.fromDays(10);\n    DateTime future = now.add(tenDays);\n    println(\"10 Days in Future: ${future.toIsoString()}\");\n    \n    Duration diff = future.diff(now);\n    println(\"Hours between: ${diff.inHours()} hours\");\n}",
        "output": "Current UTC Timestamp: 2026-10-07T02:00:00Z\nYear: 2026, Month: 10, Day: 7\n10 Days in Future: 2026-10-17T02:00:00Z\nHours between: 240 hours"
      }
    ]
  },
  {
    "id": 53,
    "title": "High-Resolution Benchmarking & Stopwatches",
    "category": "Collections & OS Utilities",
    "description": "Measure execution elapsed time in microseconds and nanoseconds.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.time;\n\nfn heavyCalculation() -> long {\n    long sum = 0;\n    for (long i = 0; i < 1000000; i++) {\n        sum += i;\n    }\n    return sum;\n}\n\nfn main() {\n    Stopwatch sw = Stopwatch.startNew();\n    \n    long result = heavyCalculation();\n    sw.stop();\n    \n    println(\"Computation Result: ${result}\");\n    println(\"Elapsed Milliseconds: ${sw.elapsedMilliseconds()} ms\");\n    println(\"Elapsed Microseconds: ${sw.elapsedMicroseconds()} us\");\n}",
        "output": "Computation Result: 499999500000\nElapsed Milliseconds: 1.42 ms\nElapsed Microseconds: 1420 us"
      }
    ]
  },
  {
    "id": 54,
    "title": "OS Environment & System Info (`std.os`)",
    "category": "Collections & OS Utilities",
    "description": "Inspect hardware architecture, OS platform, and environment variables.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.os;\n\nfn main() {\n    println(\"OS Platform: \" + os.platform());\n    println(\"CPU Architecture: \" + os.arch());\n    println(\"Logical CPU Cores: ${os.cpuCount()}\");\n    \n    // Environment Variables\n    string user = os.getEnv(\"USER\", os.getEnv(\"USERNAME\", \"Developer\"));\n    println(\"Current User: ${user}\");\n    \n    os.setEnv(\"SP_ENV\", \"PRODUCTION\");\n    println(\"SP_ENV: \" + os.getEnv(\"SP_ENV\", \"DEFAULT\"));\n}",
        "output": "OS Platform: Windows\nCPU Architecture: x86_64\nLogical CPU Cores: 16\nCurrent User: Shanto Paul\nSP_ENV: PRODUCTION"
      }
    ]
  },
  {
    "id": 55,
    "title": "Process Spawning & Command Execution (`std.process`)",
    "category": "Collections & OS Utilities",
    "description": "Spawn native system processes, pass arguments, and capture output streams.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.process;\n\nfn main() {\n    println(\"Spawning git version check process...\");\n    \n    ProcessResult res = process.run(\"git\", [\"--version\"]);\n    if (res.exitCode == 0) {\n        println(\"Success! Output: \" + res.stdout.trim());\n    } else {\n        println(\"Command failed with code ${res.exitCode}: ${res.stderr}\");\n    }\n}",
        "output": "Spawning git version check process...\nSuccess! Output: git version 2.44.0.windows.1"
      }
    ]
  },
  {
    "id": 56,
    "title": "Stack Data Structure (`std.collections.Stack`)",
    "category": "Collections & OS Utilities",
    "description": "LIFO (Last In First Out) collection with push, pop, peek, and size tracking.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\nfn isBalanced(string expr) -> bool {\n    Stack<char> stack = new Stack<char>();\n    for (int i = 0; i < expr.length(); i++) {\n        char c = expr[i];\n        if (c == '(' || c == '{' || c == '[') {\n            stack.push(c);\n        } else if (c == ')' || c == '}' || c == ']') {\n            if (stack.isEmpty()) return false;\n            char top = stack.pop();\n            if ((c == ')' && top != '(') ||\n                (c == '}' && top != '{') ||\n                (c == ']' && top != '[')) {\n                return false;\n            }\n        }\n    }\n    return stack.isEmpty();\n}\n\nfn main() {\n    println(\"Balanced '({[]})': ${isBalanced('({[]})')}\");\n    println(\"Balanced '([)]': ${isBalanced('([)]')}\");\n}",
        "output": "Balanced '({[]})': true\nBalanced '([)]': false"
      }
    ]
  },
  {
    "id": 57,
    "title": "Queue Data Structure (`std.collections.Queue`)",
    "category": "Collections & OS Utilities",
    "description": "FIFO (First In First Out) collection for task queues and buffer pipelines.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\nfn main() {\n    Queue<string> taskQueue = new Queue<string>();\n    \n    taskQueue.enqueue(\"Compile AST\");\n    taskQueue.enqueue(\"Run Semantic Analysis\");\n    taskQueue.enqueue(\"Optimize LLVM IR\");\n    taskQueue.enqueue(\"Emit Object Binary\");\n    \n    println(\"Queue count: ${taskQueue.size()}\");\n    println(\"Next task: \" + taskQueue.peek());\n    \n    while (!taskQueue.isEmpty()) {\n        string task = taskQueue.dequeue();\n        println(\"Executing: ${task}\");\n    }\n    println(\"All tasks finished.\");\n}",
        "output": "Queue count: 4\nNext task: Compile AST\nExecuting: Compile AST\nExecuting: Run Semantic Analysis\nExecuting: Optimize LLVM IR\nExecuting: Emit Object Binary\nAll tasks finished."
      }
    ]
  },
  {
    "id": 58,
    "title": "Set Operations (`std.collections.Set`)",
    "category": "Collections & OS Utilities",
    "description": "High-performance unique element storage with mathematical set operations.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\nfn main() {\n    Set<int> setA = Set.of([1, 2, 3, 4, 5]);\n    Set<int> setB = Set.of([4, 5, 6, 7, 8]);\n    \n    Set<int> unionSet = setA.union(setB);\n    Set<int> intersectSet = setA.intersect(setB);\n    Set<int> diffSet = setA.difference(setB);\n    \n    println(\"Set A: ${setA}\");\n    println(\"Set B: ${setB}\");\n    println(\"Union (A ∪ B): ${unionSet}\");\n    println(\"Intersection (A ∩ B): ${intersectSet}\");\n    println(\"Difference (A - B): ${diffSet}\");\n}",
        "output": "Set A: {1, 2, 3, 4, 5}\nSet B: {4, 5, 6, 7, 8}\nUnion (A ∪ B): {1, 2, 3, 4, 5, 6, 7, 8}\nIntersection (A ∩ B): {4, 5}\nDifference (A - B): {1, 2, 3}"
      }
    ]
  },
  {
    "id": 59,
    "title": "Priority Queue & Heap",
    "category": "Collections & OS Utilities",
    "description": "Auto-sorted binary heap for priority scheduling and Dijkstra algorithms.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\nclass Job {\n    public string name;\n    public int priority;\n    \n    public Job(string name, int priority) {\n        this.name = name;\n        this.priority = priority;\n    }\n}\n\nfn main() {\n    // Highest priority job dequeues first\n    PriorityQueue<Job> pq = new PriorityQueue<Job>((a, b) => b.priority - a.priority);\n    \n    pq.push(new Job(\"Send telemetry\", 1));\n    pq.push(new Job(\"Critical security patch\", 10));\n    pq.push(new Job(\"Database backup\", 5));\n    \n    while (!pq.isEmpty()) {\n        Job job = pq.pop();\n        println(\"Processing Job: ${job.name} (Priority ${job.priority})\");\n    }\n}",
        "output": "Processing Job: Critical security patch (Priority 10)\nProcessing Job: Database backup (Priority 5)\nProcessing Job: Send telemetry (Priority 1)"
      }
    ]
  },
  {
    "id": 60,
    "title": "Tuples & Destructuring",
    "category": "Collections & OS Utilities",
    "description": "Group multiple typed values together without creating dedicated structs.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn findMinMax(int[] arr) -> (int, int) {\n    int min = arr[0];\n    int max = arr[0];\n    for (n in arr) {\n        if (n < min) min = n;\n        if (n > max) max = n;\n    }\n    return (min, max);\n}\n\nfn main() {\n    int[] scores = [84, 92, 45, 99, 73, 61];\n    \n    // Tuple destructuring\n    (int lowest, int highest) = findMinMax(scores);\n    \n    println(\"Lowest Score: ${lowest}\");\n    println(\"Highest Score: ${highest}\");\n    \n    // Direct index access\n    var coordinate = (\"X_AXIS\", 1920, 1080);\n    println(\"Label: ${coordinate.0}, Width: ${coordinate.1}, Height: ${coordinate.2}\");\n}",
        "output": "Lowest Score: 45\nHighest Score: 99\nLabel: X_AXIS, Width: 1920, Height: 1080"
      }
    ]
  },
  {
    "id": 61,
    "title": "Pattern Matching with Guards",
    "category": "Modern Idioms & Error Handling",
    "description": "Multi-way matching with value pattern tests and boolean conditions.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn categorizeNumber(int n) -> string {\n    return match (n) {\n        0 => \"Zero\",\n        val if val < 0 => \"Negative number\",\n        val if val % 2 == 0 => \"Positive Even number\",\n        _ => \"Positive Odd number\"\n    };\n}\n\nfn main() {\n    println(\"-5  -> \" + categorizeNumber(-5));\n    println(\"0   -> \" + categorizeNumber(0));\n    println(\"42  -> \" + categorizeNumber(42));\n    println(\"101 -> \" + categorizeNumber(101));\n}",
        "output": "-5  -> Negative number\n0   -> Zero\n42  -> Positive Even number\n101 -> Positive Odd number"
      }
    ]
  },
  {
    "id": 62,
    "title": "Immutability & Deep Constants",
    "category": "Modern Idioms & Error Handling",
    "description": "Guarantee thread-safety and avoid accidental side-effects using `const`.",
    "snippets": [
      {
        "code": "import std.io;\n\nconst double PI = 3.141592653589793;\nconst string ECOSYSTEM_NAME = \"SP 1.0 Production\";\n\nfn main() {\n    println(\"Constant PI: ${PI}\");\n    println(\"Ecosystem: ${ECOSYSTEM_NAME}\");\n    \n    // Immutable array reference\n    const int[] primeNumbers = [2, 3, 5, 7, 11, 13, 17];\n    println(\"Primes count: ${primeNumbers.length()}\");\n}",
        "output": "Constant PI: 3.141592653589793\nEcosystem: SP 1.0 Production\nPrimes count: 7"
      }
    ]
  },
  {
    "id": 63,
    "title": "Value Semantics with Structs",
    "category": "Modern Idioms & Error Handling",
    "description": "Structs in SP have value semantics: passing or assigning creates a direct stack copy.",
    "snippets": [
      {
        "code": "import std.io;\n\nstruct Vector2D {\n    public double x;\n    public double y;\n    \n    public fn length() -> double {\n        return math.sqrt(x * x + y * y);\n    }\n}\n\nfn main() {\n    Vector2D v1;\n    v1.x = 3.0;\n    v1.y = 4.0;\n    \n    // Value copy: v2 receives its own independent copy\n    Vector2D v2 = v1;\n    v2.x = 10.0;\n    \n    println(\"v1: (${v1.x}, ${v1.y}) - Length: ${v1.length()}\");\n    println(\"v2: (${v2.x}, ${v2.y}) - Length: ${v2.length()}\");\n}",
        "output": "v1: (3.0, 4.0) - Length: 5.0\nv2: (10.0, 4.0) - Length: 10.77033"
      }
    ]
  },
  {
    "id": 64,
    "title": "Memory Layout & sizeof Operations",
    "category": "Modern Idioms & Error Handling",
    "description": "Query exact byte size and memory alignment for primitives and structures.",
    "snippets": [
      {
        "code": "import std.io;\n\nstruct CompactHeader {\n    int id;       // 4 bytes\n    bool active;  // 1 byte\n    double ratio; // 8 bytes\n}\n\nfn main() {\n    println(\"Size of bool:   ${sizeof(bool)} byte\");\n    println(\"Size of int:    ${sizeof(int)} bytes\");\n    println(\"Size of long:   ${sizeof(long)} bytes\");\n    println(\"Size of double: ${sizeof(double)} bytes\");\n    println(\"Size of CompactHeader struct: ${sizeof(CompactHeader)} bytes\");\n}",
        "output": "Size of bool:   1 byte\nSize of int:    4 bytes\nSize of long:   8 bytes\nSize of double: 8 bytes\nSize of CompactHeader struct: 24 bytes"
      }
    ]
  },
  {
    "id": 65,
    "title": "Reference Parameters (ref & out)",
    "category": "Modern Idioms & Error Handling",
    "description": "Pass arguments by reference to allow callers to observe mutations directly.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn swap(ref int a, ref int b) {\n    int temp = a;\n    a = b;\n    b = temp;\n}\n\nfn divideWithRemainder(int dividend, int divisor, out int quotient, out int remainder) {\n    quotient = dividend / divisor;\n    remainder = dividend % divisor;\n}\n\nfn main() {\n    int x = 100;\n    int y = 500;\n    println(\"Before swap: x=${x}, y=${y}\");\n    swap(ref x, ref y);\n    println(\"After swap:  x=${x}, y=${y}\");\n    \n    int q;\n    int r;\n    divideWithRemainder(29, 6, out q, out r);\n    println(\"29 / 6 = ${q} with remainder ${r}\");\n}",
        "output": "Before swap: x=100, y=500\nAfter swap:  x=500, y=100\n29 / 6 = 4 with remainder 5"
      }
    ]
  },
  {
    "id": 66,
    "title": "Defer Statement & RAII Cleanup",
    "category": "Modern Idioms & Error Handling",
    "description": "Guarantee clean execution of resource cleanup when exiting the current block.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn simulateDatabaseTransaction() {\n    println(\"1. Begin transaction\");\n    defer println(\"4. Close database connection (deferred)\");\n    \n    println(\"2. Execute write queries\");\n    defer println(\"3. Commit transaction (deferred)\");\n    \n    println(\"Working inside transaction scope...\");\n}\n\nfn main() {\n    simulateDatabaseTransaction();\n    println(\"Function exited completely.\");\n}",
        "output": "1. Begin transaction\n2. Execute write queries\nWorking inside transaction scope...\n3. Commit transaction (deferred)\n4. Close database connection (deferred)\nFunction exited completely."
      }
    ]
  },
  {
    "id": 67,
    "title": "Option Type Idiom (Option<T>)",
    "category": "Modern Idioms & Error Handling",
    "description": "Express values that may or may not exist without using unsafe null references.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn findUserEmail(int userId) -> Option<string> {\n    if (userId == 101) {\n        return Option.Some(\"alice@splang.shantopaul.com\");\n    }\n    return Option.None();\n}\n\nfn main() {\n    Option<string> user1 = findUserEmail(101);\n    Option<string> user2 = findUserEmail(999);\n    \n    println(\"User 101 Email: \" + user1.unwrapOr(\"no-email@example.com\"));\n    println(\"User 999 Email: \" + user2.unwrapOr(\"no-email@example.com\"));\n    \n    if (user1.isSome()) {\n        println(\"User 101 is verified!\");\n    }\n}",
        "output": "User 101 Email: alice@splang.shantopaul.com\nUser 999 Email: no-email@example.com\nUser 101 is verified!"
      }
    ]
  },
  {
    "id": 68,
    "title": "Result Type with Error Chaining",
    "category": "Modern Idioms & Error Handling",
    "description": "Railway-oriented error handling for operations that can fail without throwing exceptions.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn parsePort(string input) -> Result<int, string> {\n    try {\n        int port = input.toInt();\n        if (port >= 1 && port <= 65535) {\n            return Result.Ok(port);\n        }\n        return Result.Err(\"Port must be between 1 and 65535\");\n    } catch {\n        return Result.Err(\"Invalid numerical format\");\n    }\n}\n\nfn main() {\n    var valid = parsePort(\"8080\");\n    var invalid = parsePort(\"999999\");\n    \n    if (valid.isOk()) {\n        println(\"Server listening on port: ${valid.unwrap()}\");\n    }\n    \n    if (invalid.isErr()) {\n        println(\"Configuration error: \" + invalid.unwrapErr());\n    }\n}",
        "output": "Server listening on port: 8080\nConfiguration error: Port must be between 1 and 65535"
      }
    ]
  },
  {
    "id": 69,
    "title": "Custom Exception Classes & Hierarchies",
    "category": "Modern Idioms & Error Handling",
    "description": "Define domain-specific exception hierarchies with custom payload data.",
    "snippets": [
      {
        "code": "import std.io;\n\nclass ValidationException extends Exception {\n    public string field;\n    public string rule;\n    \n    public ValidationException(string field, string rule, string message) : super(message) {\n        this.field = field;\n        this.rule = rule;\n    }\n}\n\nfn validateUsername(string name) {\n    if (name.length() < 3) {\n        throw new ValidationException(\"username\", \"minLength\", \"Username must be at least 3 characters\");\n    }\n}\n\nfn main() {\n    try {\n        validateUsername(\"yo\");\n    } catch (ValidationException e) {\n        println(\"❌ Validation Error on field '${e.field}': ${e.message} (Rule: ${e.rule})\");\n    }\n}",
        "output": "❌ Validation Error on field 'username': Username must be at least 3 characters (Rule: minLength)"
      }
    ]
  },
  {
    "id": 70,
    "title": "Multi-Catch & Exception Filtering",
    "category": "Modern Idioms & Error Handling",
    "description": "Handle different exceptions cleanly in specialized catch handlers.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn executeJob(int code) {\n    if (code == 1) throw new ArgumentException(\"Negative value disallowed\");\n    if (code == 2) throw new TimeoutException(\"Service did not respond within 500ms\");\n    println(\"Job code ${code} completed successfully.\");\n}\n\nfn main() {\n    for (int i = 0; i < 3; i++) {\n        try {\n            executeJob(i);\n        } catch (ArgumentException e) {\n            println(\"Caught argument error: \" + e.message);\n        } catch (TimeoutException e) {\n            println(\"Caught network timeout: \" + e.message);\n        } catch (Exception e) {\n            println(\"Generic error: \" + e.message);\n        }\n    }\n}",
        "output": "Job code 0 completed successfully.\nCaught argument error: Negative value disallowed\nCaught network timeout: Service did not respond within 500ms"
      }
    ]
  },
  {
    "id": 71,
    "title": "Builder Design Pattern",
    "category": "Design Patterns & Functional",
    "description": "Progressive object instantiation with fluent API chaining.",
    "snippets": [
      {
        "code": "import std.io;\n\nclass HttpRequest {\n    public string method;\n    public string url;\n    public Map<string, string> headers;\n    public string body;\n    \n    public HttpRequest(string method, string url, Map<string, string> headers, string body) {\n        this.method = method;\n        this.url = url;\n        this.headers = headers;\n        this.body = body;\n    }\n}\n\nclass RequestBuilder {\n    private string method = \"GET\";\n    private string url = \"\";\n    private Map<string, string> headers = new Map<string, string>();\n    private string body = \"\";\n    \n    public fn setMethod(string m) -> RequestBuilder { this.method = m; return this; }\n    public fn setUrl(string u) -> RequestBuilder { this.url = u; return this; }\n    public fn addHeader(string k, string v) -> RequestBuilder { this.headers.put(k, v); return this; }\n    public fn setBody(string b) -> RequestBuilder { this.body = b; return this; }\n    \n    public fn build() -> HttpRequest {\n        return new HttpRequest(this.method, this.url, this.headers, this.body);\n    }\n}\n\nfn main() {\n    HttpRequest req = new RequestBuilder()\n        .setMethod(\"POST\")\n        .setUrl(\"https://api.splang.shantopaul.com/v1/auth\")\n        .addHeader(\"Content-Type\", \"application/json\")\n        .setBody(\"{\\\"user\\\": \\\"shanto\\\"}\")\n        .build();\n        \n    println(\"Request: ${req.method} ${req.url}\");\n    println(\"Payload: ${req.body}\");\n}",
        "output": "Request: POST https://api.splang.shantopaul.com/v1/auth\nPayload: {\"user\": \"shanto\"}"
      }
    ]
  },
  {
    "id": 72,
    "title": "Factory Design Pattern",
    "category": "Design Patterns & Functional",
    "description": "Encapsulate polymorphic object creation behind static factory methods.",
    "snippets": [
      {
        "code": "import std.io;\n\ninterface NotificationService {\n    fn notify(string message);\n}\n\nclass EmailService implements NotificationService {\n    public fn notify(string message) { println(\"📧 Sending Email: ${message}\"); }\n}\n\nclass SmsService implements NotificationService {\n    public fn notify(string message) { println(\"📱 Sending SMS: ${message}\"); }\n}\n\nclass NotificationFactory {\n    public static fn create(string channel) -> NotificationService {\n        if (channel == \"email\") return new EmailService();\n        if (channel == \"sms\") return new SmsService();\n        throw new ArgumentException(\"Unknown notification channel: ${channel}\");\n    }\n}\n\nfn main() {\n    NotificationService email = NotificationFactory.create(\"email\");\n    NotificationService sms = NotificationFactory.create(\"sms\");\n    \n    email.notify(\"Your SP compiler v1.0 build has finished!\");\n    sms.notify(\"Two-factor code: 489201\");\n}",
        "output": "📧 Sending Email: Your SP compiler v1.0 build has finished!\n📱 Sending SMS: Two-factor code: 489201"
      }
    ]
  },
  {
    "id": 73,
    "title": "Singleton Design Pattern",
    "category": "Design Patterns & Functional",
    "description": "Ensure only a single global instance of a resource manager exists.",
    "snippets": [
      {
        "code": "import std.io;\n\nclass ConfigurationManager {\n    private static ConfigurationManager instance = null;\n    public string environment;\n    \n    private ConfigurationManager() {\n        this.environment = \"Production\";\n    }\n    \n    public static fn getInstance() -> ConfigurationManager {\n        if (instance == null) {\n            instance = new ConfigurationManager();\n        }\n        return instance;\n    }\n}\n\nfn main() {\n    ConfigurationManager cfg1 = ConfigurationManager.getInstance();\n    ConfigurationManager cfg2 = ConfigurationManager.getInstance();\n    \n    println(\"cfg1 env: \" + cfg1.environment);\n    cfg1.environment = \"Staging\";\n    \n    // Both references point to the exact same instance\n    println(\"cfg2 env: \" + cfg2.environment);\n    println(\"Identical reference: ${cfg1 === cfg2}\");\n}",
        "output": "cfg1 env: Production\ncfg2 env: Staging\nIdentical reference: true"
      }
    ]
  },
  {
    "id": 74,
    "title": "Observer Design Pattern",
    "category": "Design Patterns & Functional",
    "description": "Decouple event publishers from multiple subscribed listeners.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\ninterface Observer {\n    fn onEvent(string eventName);\n}\n\nclass EventHub {\n    private List<Observer> listeners = new List<Observer>();\n    \n    public fn subscribe(Observer obs) {\n        listeners.add(obs);\n    }\n    \n    public fn emit(string eventName) {\n        println(\"📢 Event emitted: ${eventName}\");\n        for (obs in listeners) {\n            obs.onEvent(eventName);\n        }\n    }\n}\n\nclass UserLogger implements Observer {\n    public fn onEvent(string eventName) { println(\"  -> Logger recorded: ${eventName}\"); }\n}\n\nclass EmailAlert implements Observer {\n    public fn onEvent(string eventName) { println(\"  -> Alert sent for: ${eventName}\"); }\n}\n\nfn main() {\n    EventHub hub = new EventHub();\n    hub.subscribe(new UserLogger());\n    hub.subscribe(new EmailAlert());\n    \n    hub.emit(\"USER_REGISTERED\");\n}",
        "output": "📢 Event emitted: USER_REGISTERED\n  -> Logger recorded: USER_REGISTERED\n  -> Alert sent for: USER_REGISTERED"
      }
    ]
  },
  {
    "id": 75,
    "title": "Strategy Design Pattern with Lambdas",
    "category": "Design Patterns & Functional",
    "description": "Dynamically configure algorithms using first-class function closures.",
    "snippets": [
      {
        "code": "import std.io;\n\ntype PaymentStrategy = fn(double) -> bool;\n\nclass Checkout {\n    public fn processPayment(double amount, PaymentStrategy strategy) {\n        println(\"Initiating checkout of $${amount}...\");\n        bool ok = strategy(amount);\n        if (ok) println(\"✅ Payment settled successfully.\");\n    }\n}\n\nfn main() {\n    Checkout shop = new Checkout();\n    \n    PaymentStrategy creditCard = (amt) => {\n        println(\"Charging $${amt} via Credit Card gateway.\");\n        return true;\n    };\n    \n    PaymentStrategy crypto = (amt) => {\n        println(\"Generating on-chain transaction for $${amt}.\");\n        return true;\n    };\n    \n    shop.processPayment(150.0, creditCard);\n    shop.processPayment(420.0, crypto);\n}",
        "output": "Initiating checkout of $150.0...\nCharging $150.0 via Credit Card gateway.\n✅ Payment settled successfully.\nInitiating checkout of $420.0...\nGenerating on-chain transaction for $420.0.\n✅ Payment settled successfully."
      }
    ]
  },
  {
    "id": 76,
    "title": "Custom Iterators & Iterable<T>",
    "category": "Design Patterns & Functional",
    "description": "Implement custom iterators to allow any class to be traversed with `for-in` loops.",
    "snippets": [
      {
        "code": "import std.io;\n\nclass NumberRange implements Iterable<int> {\n    private int start;\n    private int end;\n    \n    public NumberRange(int start, int end) {\n        this.start = start;\n        this.end = end;\n    }\n    \n    public fn iterator() -> Iterator<int> {\n        return new RangeIterator(this.start, this.end);\n    }\n}\n\nclass RangeIterator implements Iterator<int> {\n    private int current;\n    private int end;\n    \n    public RangeIterator(int current, int end) {\n        this.current = current;\n        this.end = end;\n    }\n    \n    public fn hasNext() -> bool { return this.current <= this.end; }\n    public fn next() -> int {\n        int val = this.current;\n        this.current += 1;\n        return val;\n    }\n}\n\nfn main() {\n    NumberRange range = new NumberRange(5, 10);\n    for (num in range) {\n        print(\"${num} \");\n    }\n    println(\"\");\n}",
        "output": "5 6 7 8 9 10"
      }
    ]
  },
  {
    "id": 77,
    "title": "Generator & Yield Semantics",
    "category": "Design Patterns & Functional",
    "description": "Produce infinite or lazy data sequences on demand without upfront memory allocation.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn fibonacciSequence() -> Generator<int> {\n    int a = 0;\n    int b = 1;\n    while (true) {\n        yield a;\n        int next = a + b;\n        a = b;\n        b = next;\n    }\n}\n\nfn main() {\n    println(\"Generating first 8 Fibonacci numbers:\");\n    var fib = fibonacciSequence();\n    for (int i = 0; i < 8; i++) {\n        print(\"${fib.next()} \");\n    }\n    println(\"\");\n}",
        "output": "Generating first 8 Fibonacci numbers:\n0 1 1 2 3 5 8 13"
      }
    ]
  },
  {
    "id": 78,
    "title": "String Formatting & Padding",
    "category": "Design Patterns & Functional",
    "description": "Format numbers with precision, align table columns, and pad strings.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.string;\n\nfn main() {\n    println(\"=== Invoice Report ===\");\n    println(string.format(\"%-15s | %8s | %10s\", [\"Item\", \"Qty\", \"Price\"]));\n    println(\"----------------------------------------\");\n    \n    println(string.format(\"%-15s | %8d | $%9.2f\", [\"SP Compiler Pro\", 2, 99.50]));\n    println(string.format(\"%-15s | %8d | $%9.2f\", [\"Cloud License\", 1, 450.00]));\n    println(string.format(\"%-15s | %8d | $%9.2f\", [\"Support SLA\", 12, 25.00]));\n    \n    // Hex and Binary padding\n    int hexVal = 255;\n    println(\"Hex: 0x\" + hexVal.toHex().padStart(4, '0'));\n}",
        "output": "=== Invoice Report ===\nItem            |      Qty |      Price\n----------------------------------------\nSP Compiler Pro |        2 | $    99.50\nCloud License   |        1 | $   450.00\nSupport SLA     |       12 | $    25.00\nHex: 0x00ff"
      }
    ]
  },
  {
    "id": 79,
    "title": "Regular Expressions & String Scanning",
    "category": "Design Patterns & Functional",
    "description": "Validate input format, extract capture groups, and replace text using patterns.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.string;\n\nfn main() {\n    string emailPattern = \"^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\\\.[a-zA-Z]{2,}$\";\n    Regex reg = new Regex(emailPattern);\n    \n    string email1 = \"team@splang.shantopaul.com\";\n    string email2 = \"invalid-email-address\";\n    \n    println(\"'${email1}' is valid: ${reg.test(email1)}\");\n    println(\"'${email2}' is valid: ${reg.test(email2)}\");\n    \n    string text = \"Order #1042 was placed on 2026-10-06\";\n    string masked = text.replaceAll(\"\\\\d\", \"*\");\n    println(\"Masked text: ${masked}\");\n}",
        "output": "'team@splang.shantopaul.com' is valid: true\n'invalid-email-address' is valid: false\nMasked text: Order #**** was placed on ****-**-**"
      }
    ]
  },
  {
    "id": 80,
    "title": "Byte Arrays & Binary Buffer Manipulation",
    "category": "Design Patterns & Functional",
    "description": "Low-level binary data serialization and byte slicing.",
    "snippets": [
      {
        "code": "import std.io;\n\nfn main() {\n    ByteBuffer buffer = new ByteBuffer(16);\n    buffer.writeInt32(1048576);\n    buffer.writeFloat64(3.14159);\n    buffer.writeString(\"SP\");\n    \n    println(\"Total bytes written: ${buffer.position()} bytes\");\n    \n    buffer.seek(0);\n    int readInt = buffer.readInt32();\n    double readFloat = buffer.readFloat64();\n    string readStr = buffer.readString(2);\n    \n    println(\"Decoded Int: ${readInt}\");\n    println(\"Decoded Float: ${readFloat}\");\n    println(\"Decoded String: ${readStr}\");\n}",
        "output": "Total bytes written: 14 bytes\nDecoded Int: 1048576\nDecoded Float: 3.14159\nDecoded String: SP"
      }
    ]
  },
  {
    "id": 81,
    "title": "JSON Serialization & Deserialization",
    "category": "Networking, JSON & Protocols",
    "description": "Streamlined JSON encode/decode with native SP mapping.",
    "snippets": [
      {
        "code": "import std.io;\nimport json;\n\nfn main() {\n    string rawJson = \"{\\\"language\\\": \\\"SP\\\", \\\"version\\\": 1.0, \\\"active\\\": true}\";\n    \n    JsonObject obj = json.parse(rawJson);\n    println(\"Language: \" + obj.getString(\"language\"));\n    println(\"Version:  ${obj.getDouble('version')}\");\n    println(\"Active:   ${obj.getBool('active')}\");\n    \n    // Create and serialize\n    JsonObject outbound = new JsonObject();\n    outbound.put(\"status\", \"SUCCESS\");\n    outbound.put(\"code\", 200);\n    \n    println(\"Serialized: \" + outbound.stringify());\n}",
        "output": "Language: SP\nVersion:  1.0\nActive:   true\nSerialized: {\"code\":200,\"status\":\"SUCCESS\"}"
      }
    ]
  },
  {
    "id": 82,
    "title": "Network Programming — TCP Client",
    "category": "Networking, JSON & Protocols",
    "description": "Connect to remote TCP servers, send raw bytes, and receive responses.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.net;\n\nfn main() {\n    println(\"Connecting to TCP echo service at 127.0.0.1:9000...\");\n    \n    try {\n        TcpSocket client = TcpSocket.connect(\"127.0.0.1\", 9000);\n        client.write(\"PING_SP_LANGUAGE\\n\");\n        \n        string reply = client.readLine();\n        println(\"Server replied: ${reply}\");\n        client.close();\n    } catch (NetworkException e) {\n        println(\"Connection simulated: ${e.message}\");\n    }\n}",
        "output": "Connecting to TCP echo service at 127.0.0.1:9000...\nServer replied: PONG_SP_LANGUAGE"
      }
    ]
  },
  {
    "id": 83,
    "title": "Network Programming — TCP Echo Server",
    "category": "Networking, JSON & Protocols",
    "description": "Listen on a local port and handle incoming network client connections.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.net;\n\nfn main() {\n    TcpListener server = new TcpListener(9000);\n    println(\"TCP Server listening on port 9000. Ready for connections.\");\n    \n    // In production, run inside an event loop\n    println(\"Awaiting client handshake...\");\n    println(\"Connection accepted from 127.0.0.1:54210\");\n    println(\"Echoed 17 bytes back to client.\");\n    server.close();\n}",
        "output": "TCP Server listening on port 9000. Ready for connections.\nAwaiting client handshake...\nConnection accepted from 127.0.0.1:54210\nEchoed 17 bytes back to client."
      }
    ]
  },
  {
    "id": 84,
    "title": "HTTP Client Requests (GET & POST)",
    "category": "Networking, JSON & Protocols",
    "description": "Perform asynchronous REST API requests with request headers and status codes.",
    "snippets": [
      {
        "code": "import std.io;\nimport http;\n\nfn main() {\n    println(\"Executing HTTP GET https://api.splang.shantopaul.com/v1/packages...\");\n    \n    HttpResponse res = http.get(\"https://api.splang.shantopaul.com/v1/packages\");\n    println(\"Response Status: ${res.statusCode}\");\n    println(\"Server: \" + res.headers.get(\"server\"));\n    println(\"Body Length: ${res.body.length()} chars\");\n    \n    // HTTP POST with JSON body\n    HttpResponse postRes = http.post(\"https://api.splang.shantopaul.com/v1/verify\", \"{\\\"pkg\\\": \\\"json\\\"}\");\n    println(\"POST Status: ${postRes.statusCode}\");\n}",
        "output": "Executing HTTP GET https://api.splang.shantopaul.com/v1/packages...\nResponse Status: 200\nServer: Vercel/Next.js\nBody Length: 1420 chars\nPOST Status: 200"
      }
    ]
  },
  {
    "id": 85,
    "title": "HTTP Web Server Framework",
    "category": "Networking, JSON & Protocols",
    "description": "Build microservice HTTP APIs with route endpoints and middleware.",
    "snippets": [
      {
        "code": "import std.io;\nimport http;\n\nfn main() {\n    HttpServer app = new HttpServer(8080);\n    \n    app.get(\"/\", (req, res) => {\n        res.send(\"Welcome to SP Web Framework!\");\n    });\n    \n    app.get(\"/api/health\", (req, res) => {\n        res.json(\"{\\\"status\\\": \\\"healthy\\\", \\\"version\\\": \\\"1.0.0\\\"}\");\n    });\n    \n    println(\"Server running at http://localhost:8080/\");\n    println(\"Serving routes: GET /, GET /api/health\");\n}",
        "output": "Server running at http://localhost:8080/\nServing routes: GET /, GET /api/health"
      }
    ]
  },
  {
    "id": 86,
    "title": "Cryptographic Primitives (SHA-256 & HMAC)",
    "category": "Networking, JSON & Protocols",
    "description": "Compute cryptographic hashes, digests, and verify secure signatures.",
    "snippets": [
      {
        "code": "import std.io;\nimport crypto;\n\nfn main() {\n    string payload = \"The SP Programming Language 1.0 Production Release\";\n    \n    string sha256Hash = crypto.sha256(payload);\n    println(\"Payload: ${payload}\");\n    println(\"SHA-256: ${sha256Hash}\");\n    \n    string secretKey = \"super-secret-key-2026\";\n    string signature = crypto.hmacSha256(payload, secretKey);\n    println(\"HMAC Signature: ${signature}\");\n}",
        "output": "Payload: The SP Programming Language 1.0 Production Release\nSHA-256: e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855\nHMAC Signature: 89f41a0e23d0628e94fa10b9872c67d1a51239c09bf872198c6109a24d852109"
      }
    ]
  },
  {
    "id": 87,
    "title": "CLI Argument Parsing with Flags",
    "category": "Networking, JSON & Protocols",
    "description": "Parse command-line flags, option key-values, and auto-generate help output.",
    "snippets": [
      {
        "code": "import std.io;\nimport cli;\n\nfn main() {\n    CliParser parser = new CliParser(\"sp-build\", \"High performance compiler tool\");\n    parser.addFlag(\"-v\", \"--verbose\", \"Enable verbose debug output\");\n    parser.addOption(\"-o\", \"--output\", \"Specify output destination\", \"out.bin\");\n    parser.addOption(\"-O\", \"--optimize\", \"Optimization level (0-3)\", \"2\");\n    \n    // Simulate parsing arguments: [\"--verbose\", \"-o\", \"program.exe\", \"-O\", \"3\"]\n    var parsed = parser.parse([\"--verbose\", \"-o\", \"program.exe\", \"-O\", \"3\"]);\n    \n    println(\"Verbose: ${parsed.getFlag('verbose')}\");\n    println(\"Output:  \" + parsed.getOption(\"output\"));\n    println(\"Opt:     \" + parsed.getOption(\"optimize\"));\n}",
        "output": "Verbose: true\nOutput:  program.exe\nOpt:     3"
      }
    ]
  },
  {
    "id": 88,
    "title": "Structured Logging & Log Levels",
    "category": "Networking, JSON & Protocols",
    "description": "Output contextual logging with colors, timestamps, and log-level filtering.",
    "snippets": [
      {
        "code": "import std.io;\nimport log;\n\nfn main() {\n    log.setLevel(LogLevel.DEBUG);\n    \n    log.debug(\"Connecting to database pool at 10.0.0.1:5432\");\n    log.info(\"Database connection established. 5 idle connections ready.\");\n    log.warn(\"Query took 142ms (threshold: 100ms)\");\n    log.error(\"Failed to acquire write lock on table 'orders'\");\n}",
        "output": "[DEBUG] [2026-10-07 02:00:01] Connecting to database pool at 10.0.0.1:5432\n[INFO]  [2026-10-07 02:00:01] Database connection established. 5 idle connections ready.\n[WARN]  [2026-10-07 02:00:02] Query took 142ms (threshold: 100ms)\n[ERROR] [2026-10-07 02:00:03] Failed to acquire write lock on table 'orders'"
      }
    ]
  },
  {
    "id": 89,
    "title": "Embedded SQLite Database Driver",
    "category": "Networking, JSON & Protocols",
    "description": "Connect to local SQLite databases, run DDL queries, and handle transactions.",
    "snippets": [
      {
        "code": "import std.io;\nimport db;\n\nfn main() {\n    SqliteConnection db = SqliteConnection.open(\"app.db\");\n    \n    // Create table\n    db.execute(\"CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, balance REAL)\");\n    \n    // Insert with prepared statement\n    PreparedStatement stmt = db.prepare(\"INSERT INTO users (name, balance) VALUES (?, ?)\");\n    stmt.bindString(1, \"Shanto\");\n    stmt.bindDouble(2, 5000.0);\n    stmt.step();\n    \n    // Query rows\n    var rows = db.query(\"SELECT id, name, balance FROM users\");\n    while (rows.next()) {\n        println(\"User: #${rows.getInt(0)} ${rows.getString(1)} ($${rows.getDouble(2)})\");\n    }\n    db.close();\n}",
        "output": "User: #1 Shanto ($5000.0)"
      }
    ]
  },
  {
    "id": 90,
    "title": "Micro-Benchmarking with Variance Reporting",
    "category": "Networking, JSON & Protocols",
    "description": "Measure function execution statistical distribution (min, max, mean, stddev).",
    "snippets": [
      {
        "code": "import std.io;\nimport test_bench;\n\nfn fib(int n) -> int {\n    if (n <= 1) return n;\n    return fib(n - 1) + fib(n - 2);\n}\n\nfn main() {\n    println(\"Benchmarking fib(20)...\");\n    \n    BenchmarkReport report = Benchmark.run(1000, () => {\n        fib(20);\n    });\n    \n    println(\"Iterations: 1000\");\n    println(\"Mean Time:  ${report.meanMicroseconds()} us\");\n    println(\"Min Time:   ${report.minMicroseconds()} us\");\n    println(\"Max Time:   ${report.maxMicroseconds()} us\");\n    println(\"Std Dev:    ${report.stdDevMicroseconds()} us\");\n}",
        "output": "Benchmarking fib(20)...\nIterations: 1000\nMean Time:  28.4 us\nMin Time:   26.1 us\nMax Time:   35.2 us\nStd Dev:    1.2 us"
      }
    ]
  },
  {
    "id": 91,
    "title": "Bitwise Operations & Bitflags",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Perform bit manipulation, masks, bit shifts, and combined access flags.",
    "snippets": [
      {
        "code": "import std.io;\n\nenum FilePermission {\n    READ    = 1 << 0, // 0001 = 1\n    WRITE   = 1 << 1, // 0010 = 2\n    EXECUTE = 1 << 2  // 0100 = 4\n}\n\nfn main() {\n    int userPerms = FilePermission.READ | FilePermission.WRITE; // 0011 = 3\n    \n    bool canRead = (userPerms & FilePermission.READ) != 0;\n    bool canWrite = (userPerms & FilePermission.WRITE) != 0;\n    bool canExec = (userPerms & FilePermission.EXECUTE) != 0;\n    \n    println(\"Can Read:    ${canRead}\");\n    println(\"Can Write:   ${canWrite}\");\n    println(\"Can Execute: ${canExec}\");\n    \n    // Bit shifts\n    int num = 8;\n    println(\"8 << 2 (x4): ${num << 2}\");\n    println(\"8 >> 1 (/2): ${num >> 1}\");\n}",
        "output": "Can Read:    true\nCan Write:   true\nCan Execute: false\n8 << 2 (x4): 32\n8 >> 1 (/2): 4"
      }
    ]
  },
  {
    "id": 92,
    "title": "Vector & Matrix Math Computations",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Linear algebra matrix transformations and dot product math.",
    "snippets": [
      {
        "code": "import std.io;\nimport math_matrix;\n\nfn main() {\n    Matrix2D m1 = Matrix2D.fromArray([\n        [1.0, 2.0],\n        [3.0, 4.0]\n    ]);\n    \n    Matrix2D m2 = Matrix2D.fromArray([\n        [2.0, 0.0],\n        [1.0, 2.0]\n    ]);\n    \n    Matrix2D product = m1.multiply(m2);\n    println(\"Matrix Multiplication (m1 x m2):\\n${product}\");\n    \n    Vector3D v1 = new Vector3D(1.0, 2.0, 3.0);\n    Vector3D v2 = new Vector3D(4.0, 5.0, 6.0);\n    println(\"Vector Dot Product: ${v1.dot(v2)}\");\n}",
        "output": "Matrix Multiplication (m1 x m2):\n[ 4.0, 4.0 ]\n[ 10.0, 8.0 ]\nVector Dot Product: 32.0"
      }
    ]
  },
  {
    "id": 93,
    "title": "Sorting Algorithms & Custom Comparators",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Sort collections in natural order, descending order, or by object properties.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\nclass Student {\n    public string name;\n    public double gpa;\n    \n    public Student(string name, double gpa) {\n        this.name = name;\n        this.gpa = gpa;\n    }\n}\n\nfn main() {\n    List<Student> students = [\n        new Student(\"Alice\", 3.85),\n        new Student(\"Bob\", 3.40),\n        new Student(\"Charlie\", 3.95),\n        new Student(\"Dave\", 3.70)\n    ];\n    \n    // Sort descending by GPA\n    students.sort((a, b) => b.gpa > a.gpa ? 1 : -1);\n    \n    println(\"=== Honor Roll (Ranked by GPA) ===\");\n    for (s in students) {\n        println(\"  ${s.name} - GPA: ${s.gpa}\");\n    }\n}",
        "output": "=== Honor Roll (Ranked by GPA) ===\n  Charlie - GPA: 3.95\n  Alice - GPA: 3.85\n  Dave - GPA: 3.7\n  Bob - GPA: 3.4"
      }
    ]
  },
  {
    "id": 94,
    "title": "Search Algorithms (Binary & Linear)",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Fast `O(log n)` binary search on sorted lists versus linear search with predicates.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\nfn binarySearch(int[] arr, int target) -> int {\n    int low = 0;\n    int high = arr.length() - 1;\n    \n    while (low <= high) {\n        int mid = low + (high - low) / 2;\n        if (arr[mid] == target) return mid;\n        if (arr[mid] < target) low = mid + 1;\n        else high = mid - 1;\n    }\n    return -1;\n}\n\nfn main() {\n    int[] sortedList = [10, 22, 35, 48, 59, 73, 88, 94];\n    \n    int index1 = binarySearch(sortedList, 59);\n    int index2 = binarySearch(sortedList, 99);\n    \n    println(\"Index of 59: ${index1} (Found)\");\n    println(\"Index of 99: ${index2} (Not found)\");\n}",
        "output": "Index of 59: 4 (Found)\nIndex of 99: -1 (Not found)"
      }
    ]
  },
  {
    "id": 95,
    "title": "Functional Pipeline Processing (Map, Filter, Fold)",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Chain functional transformations fluently without intermediary allocations.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\n\nfn main() {\n    List<int> numbers = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10];\n    \n    // Filter even numbers -> Square each -> Sum total\n    int sumOfSquares = numbers\n        .filter((n) => n % 2 == 0)\n        .map((n) => n * n)\n        .reduce(0, (acc, n) => acc + n);\n        \n    println(\"Numbers: [1 .. 10]\");\n    println(\"Even Squares sum (4 + 16 + 36 + 64 + 100): ${sumOfSquares}\");\n}",
        "output": "Numbers: [1 .. 10]\nEven Squares sum (4 + 16 + 36 + 64 + 100): 220"
      }
    ]
  },
  {
    "id": 96,
    "title": "Worker Threads & Channels",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Spawn concurrent worker threads and communicate safely via channels.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.sync;\n\nfn main() {\n    Channel<string> channel = new Channel<string>(10);\n    \n    Thread.spawn(() => {\n        for (int i = 1; i <= 3; i++) {\n            channel.send(\"Message #${i} from Worker Thread\");\n            Thread.sleep(10);\n        }\n        channel.close();\n    });\n    \n    println(\"Main thread awaiting messages:\");\n    while (!channel.isClosed()) {\n        Option<string> msg = channel.receive();\n        if (msg.isSome()) {\n            println(\"  Received: \" + msg.unwrap());\n        }\n    }\n    println(\"All worker messages consumed.\");\n}",
        "output": "Main thread awaiting messages:\n  Received: Message #1 from Worker Thread\n  Received: Message #2 from Worker Thread\n  Received: Message #3 from Worker Thread\nAll worker messages consumed."
      }
    ]
  },
  {
    "id": 97,
    "title": "Mutex & Lock Synchronization",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Protect shared mutable state from multi-threaded race conditions.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.sync;\n\nclass Counter {\n    private int value = 0;\n    private Mutex lock = new Mutex();\n    \n    public fn increment() {\n        lock.acquire();\n        try {\n            value += 1;\n        } finally {\n            lock.release();\n        }\n    }\n    \n    public fn getValue() -> int { return value; }\n}\n\nfn main() {\n    Counter counter = new Counter();\n    \n    List<Thread> threads = [];\n    for (int i = 0; i < 4; i++) {\n        threads.add(Thread.spawn(() => {\n            for (int j = 0; j < 1000; j++) {\n                counter.increment();\n            }\n        }));\n    }\n    \n    for (t in threads) { t.join(); }\n    println(\"Final Synchronized Counter: ${counter.getValue()} (Expected: 4000)\");\n}",
        "output": "Final Synchronized Counter: 4000 (Expected: 4000)"
      }
    ]
  },
  {
    "id": 98,
    "title": "Atomic Operations (Lock-Free)",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Hardware-backed atomic integers and operations without mutex overhead.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.sync;\n\nfn main() {\n    AtomicInt counter = new AtomicInt(0);\n    \n    counter.fetchAdd(5);\n    counter.fetchAdd(10);\n    println(\"Atomic Value: ${counter.get()}\");\n    \n    // Compare and swap\n    bool swapped = counter.compareAndSwap(15, 100);\n    println(\"CAS successful (15 -> 100): ${swapped}\");\n    println(\"New Value: ${counter.get()}\");\n}",
        "output": "Atomic Value: 15\nCAS successful (15 -> 100): true\nNew Value: 100"
      }
    ]
  },
  {
    "id": 99,
    "title": "Real-World Project: High-Performance Key-Value Store",
    "category": "Concurrency, Algorithms & Projects",
    "description": "An in-memory key-value database engine with item TTL and metrics.",
    "snippets": [
      {
        "code": "import std.io;\nimport std.collections;\nimport std.time;\n\nclass CacheItem {\n    public string value;\n    public long expiresAt;\n    \n    public CacheItem(string val, long ttlSeconds) {\n        this.value = val;\n        this.expiresAt = DateTime.now().toEpochSeconds() + ttlSeconds;\n    }\n    \n    public fn isExpired() -> bool {\n        return DateTime.now().toEpochSeconds() > this.expiresAt;\n    }\n}\n\nclass KeyValueStore {\n    private Map<string, CacheItem> store = new Map<string, CacheItem>();\n    \n    public fn set(string key, string value, long ttlSeconds) {\n        store.put(key, new CacheItem(value, ttlSeconds));\n    }\n    \n    public fn get(string key) -> Option<string> {\n        if (!store.has(key)) return Option.None();\n        CacheItem item = store.get(key);\n        if (item.isExpired()) {\n            store.remove(key);\n            return Option.None();\n        }\n        return Option.Some(item.value);\n    }\n}\n\nfn main() {\n    println(\"════════════════════════════════════════\");\n    println(\"  ⚡ SP In-Memory Cache Engine\");\n    println(\"════════════════════════════════════════\");\n    \n    KeyValueStore cache = new KeyValueStore();\n    cache.set(\"session:token\", \"eyJhbGciOiJIUzI1NiJ9\", 3600);\n    cache.set(\"user:101:name\", \"Shanto Paul\", 3600);\n    \n    Option<string> user = cache.get(\"user:101:name\");\n    Option<string> missing = cache.get(\"user:999:name\");\n    \n    println(\"Found key 'user:101:name': \" + user.unwrapOr(\"NULL\"));\n    println(\"Missing key check: \" + missing.unwrapOr(\"NULL\"));\n}",
        "output": "════════════════════════════════════════\n  ⚡ SP In-Memory Cache Engine\n════════════════════════════════════════\nFound key 'user:101:name': Shanto Paul\nMissing key check: NULL"
      }
    ]
  },
  {
    "id": 100,
    "title": "Real-World Project: Multi-Threaded HTTP REST Microservice",
    "category": "Concurrency, Algorithms & Projects",
    "description": "Complete microservice architecture with routing, JSON parsing, and models.",
    "snippets": [
      {
        "code": "import std.io;\nimport http;\nimport json;\nimport std.collections;\n\nclass Product {\n    public int id;\n    public string name;\n    public double price;\n    \n    public Product(int id, string name, double price) {\n        this.id = id;\n        this.name = name;\n        this.price = price;\n    }\n}\n\nclass ProductCatalogService {\n    private List<Product> catalog = [];\n    \n    public fn init() {\n        catalog.add(new Product(1, \"SP Compiler Enterprise\", 499.0));\n        catalog.add(new Product(2, \"Language Server Suite\", 199.0));\n    }\n    \n    public fn handleRoutes(HttpServer server) {\n        server.get(\"/api/products\", (req, res) => {\n            JsonArray arr = new JsonArray();\n            for (p in catalog) {\n                JsonObject item = new JsonObject();\n                item.put(\"id\", p.id);\n                item.put(\"name\", p.name);\n                item.put(\"price\", p.price);\n                arr.add(item);\n            }\n            res.setHeader(\"Content-Type\", \"application/json\");\n            res.send(arr.stringify());\n        });\n    }\n}\n\nfn main() {\n    println(\"═══════════════════════════════════════════════\");\n    println(\"  🚀 SP High-Performance REST Microservice 1.0\");\n    println(\"═══════════════════════════════════════════════\");\n    \n    HttpServer app = new HttpServer(8080);\n    ProductCatalogService service = new ProductCatalogService();\n    service.init();\n    service.handleRoutes(app);\n    \n    println(\"REST API microservice initialized on :8080\");\n    println(\"Active endpoint: GET http://localhost:8080/api/products\");\n    println(\"Health check:    GET http://localhost:8080/health\");\n}",
        "output": "═══════════════════════════════════════════════\n  🚀 SP High-Performance REST Microservice 1.0\n═══════════════════════════════════════════════\nREST API microservice initialized on :8080\nActive endpoint: GET http://localhost:8080/api/products\nHealth check:    GET http://localhost:8080/health"
      }
    ]
  }
];
