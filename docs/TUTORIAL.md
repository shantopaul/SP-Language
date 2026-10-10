# SP Programming Language — Getting Started & Tutorials

Welcome to the official **SP Programming Language** tutorial! This guide walks you through installing the compiler, setting up your environment, writing your first program, and building real-world applications using the built-in toolchain.

---

## 1. Quick Installation & Upgrades

### Windows (PowerShell)
```powershell
irm https://dl.splang.shantopaul.com/install.ps1 | iex
```

### Linux & macOS (Bash)
```bash
curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh
```

### Upgrading SP
Keep your installation up to date at any time:
```bash
spc upgrade
```
*(Or simply re-run the 1-line installer above — it automatically updates existing installations in 3–5 seconds without re-downloading toolchains).*

Verify your installation:
```bash
spc --version
spc doctor
spm --version
```
Expected output:
```text
spc (SP Programming Language Compiler) version 1.0.0
Website: https://splang.shantopaul.com
Docs:    https://docs.splang.shantopaul.com
Target:  x86_64-w64-windows-gnu (native LLVM backend)
```

---

## 2. Your First SP Program

Create a file named `hello.sp`:

```sp
fn main() -> int {
    let name: string = "World";
    println("Hello, ${name}!");
    return 0;
}
```

Run it directly with the compiler:
```bash
spc run hello.sp
```
Output:
```text
Hello, World!
```

Compile it to a standalone native binary:
```bash
spc build -O3 -o hello hello.sp
./hello
```

---

## 3. Creating a Project with SPM (SP Package Manager)

Initialize a new project:
```bash
spm init my_app
cd my_app
```

This creates a standard project layout:
```text
my_app/
├── sp.toml        # Project manifest
└── src/
    └── main.sp    # Entry point
```

### Project Manifest (`sp.toml`)
```toml
[package]
name = "my_app"
version = "0.1.0"
authors = ["Your Name <you@example.com>"]
description = "My high-performance SP application"
license = "MIT"

[language]
version = "1.0"

[dependencies]
# Dependencies can be added using `spm add <name>`
```

### Building and Running
```bash
spm run
```

---

## 4. Language Fundamentals

### Variables and Type Inference
```sp
// Explicit types
let age: int = 25;
let pi: float = 3.14159;
let active: bool = true;

// Mutable variables
var counter: int = 0;
counter = counter + 1;

// Constants
const MAX_BUFFER: int = 4096;

// Type inference
let greeting = "Welcome to SP!";
```

### Functions & Return Types
```sp
fn add(a: int, b: int) -> int {
    return a + b;
}

// Expression-style functions
fn square(x: int) -> int => x * x;
```

### Control Flow
```sp
// If-else
if (age >= 18) {
    println("Adult");
} else {
    println("Minor");
}

// Switch-case
switch (code) {
    case 200: println("OK"); break;
    case 404: println("Not Found"); break;
    default: println("Other Status"); break;
}

// For loop
for (int i = 0; i < 5; i++) {
    println("Index: ${i}");
}

// While loop
while (counter < 10) {
    counter++;
}
```

---

## 5. Object-Oriented Programming

### Classes & Constructors
```sp
class User {
    public string name;
    public int age;

    public constructor(name: string, age: int) {
        this.name = name;
        this.age = age;
    }

    public fn greet() {
        println("Hello, I am ${this.name}, age ${this.age}.");
    }
}

fn main() -> int {
    let user = new User("Alice", 28);
    user.greet();
    return 0;
}
```

### Inheritance & Polymorphism
```sp
class Animal {
    public string name;
    public constructor(name: string) { this.name = name; }
    public virtual fn speak() { println("Some sound"); }
}

class Dog extends Animal {
    public constructor(name: string) : super(name) {}
    public override fn speak() { println("${this.name} barks!"); }
}
```

---

## 6. Developer Tools & VS Code Integration

1. Install the official extension **"SP Language Support"** (`editors/vscode`).
2. Features enabled out of the box:
   - **Syntax Highlighting:** Full TextMate grammar for all SP keywords and literals.
   - **Autocomplete & Hover:** Powered by the native LSP server (`spls`).
   - **Formatting:** Format any file with `spc fmt main.sp -w`.
   - **Native Testing:** Run all tests with `spc test`.
   - **Documentation Generation:** Run `spc doc src/ -o docs/html`.
   - **Interactive Debugging:** Set breakpoints and step through SP code with `spd`.
