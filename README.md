<div align="center">

# SP Programming Language

### Familiar Syntax. Modern Power. Native Performance.

[![License](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-0.0.0--alpha-orange.svg)](#versioning)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux%20%7C%20macOS-brightgreen.svg)](#target-platforms)
[![Language](https://img.shields.io/badge/Built%20With-C%2B%2B%2017-red.svg)](#tech-stack)
[![Backend](https://img.shields.io/badge/Backend-LLVM-purple.svg)](#compiler-architecture)

---

**SP** is a modern, general-purpose programming language designed for developers who already know **C**, **C++**, **Java**, or **C#**. It combines the familiar syntax of these languages with modern safety features, powerful tooling, and native performance — all compiled to machine code via **LLVM**.

[Getting Started](#-getting-started) · [Language Tour](#-language-tour) · [Documentation](#-documentation) · [Roadmap](#-roadmap) · [Contributing](#-contributing)

</div>

---

## 🎯 Why SP?

Most modern languages force you to learn entirely new syntax and paradigms. **SP takes a different approach:**

| Problem | SP's Solution |
|---------|---------------|
| New languages have unfamiliar syntax | SP uses syntax you already know from C/C++/Java/C# |
| Scripting languages are too slow | SP compiles to native machine code via LLVM |
| C/C++ lack modern safety features | SP has null safety, type inference, and managed memory |
| Java/C# require heavy runtimes | SP produces standalone native executables |
| Most languages lock you into one syntax style | SP accepts multiple familiar syntax styles |

> **If you know Java, C#, or C++ — you can write SP code on day one.**

---

## ✨ Key Features

<table>
<tr>
<td width="50%">

### 🔒 Modern Type Safety
```sp
int age = 20;
string name = "Shanto";
bool active = true;

// Type inference
var score = 95.5;
const PI = 3.14159;
```

</td>
<td width="50%">

### 🛡️ Null Safety
```sp
User? user = findUser(id);

// Safe access
println(user?.name);

// Null fallback
string name = user?.name ?? "Unknown";
```

</td>
</tr>
<tr>
<td width="50%">

### 🏗️ Classes & OOP
```sp
class User {
    private string name;
    private int age;

    public User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public fn introduce() {
        println("I'm ${name}, age ${age}");
    }
}
```

</td>
<td width="50%">

### 🧬 Generics
```sp
class Box<T> {
    T value;

    Box(T value) {
        this.value = value;
    }

    T getValue() {
        return value;
    }
}

Box<int> box = new Box<int>(42);
```

</td>
</tr>
<tr>
<td width="50%">

### ⚡ Functional Programming
```sp
let numbers = [1, 2, 3, 4, 5];

let result = numbers
    .map(x => x * 2)
    .filter(x => x > 5);

// Lambda functions
var add = (int a, int b) => a + b;
```

</td>
<td width="50%">

### 🔄 Pattern Matching
```sp
match day {
    1 => println("Monday"),
    2 => println("Tuesday"),
    3 => println("Wednesday"),
    _ => println("Other day")
}
```

</td>
</tr>
<tr>
<td width="50%">

### 🌐 Async Programming
```sp
async fn fetchData() {
    let response = await http.get(url);
    println(response.body);
}
```

</td>
<td width="50%">

### 🛠️ Error Handling
```sp
// Traditional try/catch
try {
    riskyOperation();
} catch (Exception e) {
    println(e.message);
}

// Modern Result type
fn divide(int a, int b) -> Result<int, Error> {
    if (b == 0) return Error("Division by zero");
    return Ok(a / b);
}
```

</td>
</tr>
</table>

---

## 🚀 Getting Started

### Hello, SP!

Create a file called `hello.sp`:

```sp
import std.io;

fn main() {
    println("Hello, SP!");
}
```

Compile and run:

```bash
spc run hello.sp
```

Output:

```
Hello, SP!
```

Build a native executable:

```bash
spc build hello.sp
./hello          # Linux/macOS
.\hello.exe      # Windows
```

---

### Your First Project

```bash
# Initialize a new SP project
spm init my-project
cd my-project

# Project structure created:
# my-project/
# ├── sp.toml
# ├── src/
# │   └── main.sp
# ├── tests/
# └── README.md

# Run your project
spm run

# Build for release
spm build --release
```

### Project Configuration (`sp.toml`)

```toml
[package]
name = "my-project"
version = "1.0.0"
authors = ["Your Name"]

[language]
version = "1.0"

[dependencies]
http = "1.2.0"
json = "2.0.0"

[build]
entry = "src/main.sp"
```

---

## 📖 Language Tour

### Variables & Types

```sp
// Explicit types
int age = 21;
double salary = 50000.50;
float percentage = 95.5;
bool isStudent = true;
char grade = 'A';
string name = "Shanto";

// Type inference
var count = 42;           // inferred as int
let message = "Hello";    // inferred as string

// Constants
const int MAX_USERS = 100;
const PI = 3.14159;
```

### Primitive Types

```
int       int8    int16    int32    int64
uint      uint8   uint16   uint32   uint64
float     double  bool     char     string    byte
```

### Functions

SP supports two syntax styles — choose what feels natural:

```sp
// SP-native style
fn add(int a, int b) -> int {
    return a + b;
}

// Java/C#/C++ style (also accepted!)
int add(int a, int b) {
    return a + b;
}

// Short form (type-inferred)
fn add(a, b) => a + b;

// Calling
int result = add(10, 20);   // 30
```

### Control Flow

```sp
// If / Else
if (age >= 18) {
    println("Adult");
} else {
    println("Minor");
}

// For loop
for (int i = 0; i < 10; i++) {
    println(i);
}

// Range loop
for (i in 0..10) {
    println(i);
}

// While loop
while (condition) {
    // code
}

// Switch (classic)
switch (day) {
    case 1: println("Monday"); break;
    case 2: println("Tuesday"); break;
    default: println("Unknown");
}

// Match (modern)
match day {
    1 => println("Monday"),
    2 => println("Tuesday"),
    _ => println("Unknown")
}
```

### Strings

```sp
string name = "Shanto";

// Interpolation
string greeting = "Hello ${name}!";

// Concatenation
string full = "Hello " + name;

// Methods
name.length();           // 6
name.upper();            // "SHANTO"
name.lower();            // "shanto"
name.contains("an");     // true
```

### Collections

```sp
// Arrays
int[] numbers = {1, 2, 3, 4, 5};
let items = [10, 20, 30];

// Lists
List<int> list = new List<int>();
list.add(10);
list.add(20);

// Maps
Map<string, int> ages = new Map<string, int>();
ages["Shanto"] = 21;

// Modern map literal
let ages = {
    "Shanto": 21,
    "Padmasree": 21
};
```

### Classes & Inheritance

```sp
class Animal {
    fn speak() {
        println("Animal sound");
    }
}

class Dog extends Animal {
    override fn speak() {
        println("Woof!");
    }
}

// Interfaces
interface Printable {
    fn print();
}

class Document implements Printable {
    fn print() {
        println("Document content");
    }
}

// Abstract classes
abstract class Shape {
    abstract fn area() -> double;
}
```

### Access Modifiers

```sp
class User {
    public string name;       // Accessible everywhere
    private string password;  // Class only
    protected int id;         // Class + subclasses
    internal string role;     // Module only
}
```

### Structs & Enums

```sp
struct Point {
    int x;
    int y;
}

enum Status {
    Pending,
    Active,
    Completed,
    Failed
}

Status status = Status.Active;
```

### Error Handling

```sp
// Exception-based
try {
    riskyOperation();
} catch (Exception e) {
    println(e.message);
} finally {
    cleanup();
}

// Result-based (modern)
fn divide(int a, int b) -> Result<int, Error> {
    if (b == 0) {
        return Error("Cannot divide by zero");
    }
    return Ok(a / b);
}
```

### Modules & Imports

```sp
// Define a module
module myapp.users;

// Import specific items
import myapp.users.User;

// Import all from module
import myapp.users.*;

// Standard library
import std.io;
import std.math;
import std.collections;
```

### Annotations

```sp
@Serializable
class User {
    string name;
    int age;
}

@Test
fn testAddition() {
    assert(add(2, 3) == 5);
}

@Deprecated
fn oldMethod() { }

@Route("/users")
fn getUsers() { }
```

### Unsafe Mode (Systems Programming)

```sp
unsafe {
    // Low-level pointer operations
    pointer p = ...
}
```

### Native Interop (C FFI)

```sp
extern "C" {
    fn printf(string format);
}
```

---

## 🏗️ Compiler Architecture

SP compiles `.sp` source files to native machine code through an LLVM-based pipeline:

```
                ┌───────────────┐
                │  source.sp    │
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │    Lexer      │  Tokenizes source code
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │    Parser     │  Builds Abstract Syntax Tree
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │     AST       │  Normalized syntax tree
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │ Semantic      │  Name resolution + Type checking
                │ Analysis      │
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │   SP IR       │  High-level intermediate representation
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │  LLVM IR      │  Low-level intermediate representation
                └───────┬───────┘
                        │
                        ▼
                ┌───────────────┐
                │ LLVM Backend  │  Optimization + Code generation
                └───────┬───────┘
                        │
                        ▼
               Native Executable
              (.exe / ELF / Mach-O)
```

### Syntax Compatibility Layer

SP's most unique feature — one AST, multiple familiar syntax styles:

```
            Source Code
                │
    ┌───────────┼───────────┐
    │           │           │
SP Style    Java Style   C#/C++ Style
    │           │           │
    └───────────┼───────────┘
                │
                ▼
          Common AST
                │
                ▼
        Semantic Analysis
                │
                ▼
           LLVM Codegen
```

Both of these produce **identical** compiled output:

```sp
// SP-native syntax
fn add(int a, int b) -> int {
    return a + b;
}

// Java/C++ syntax (also valid SP!)
int add(int a, int b) {
    return a + b;
}
```

---

## 🛠️ Toolchain

### Compiler CLI (`spc`)

```bash
spc build [file]            # Compile to native executable
spc run [file]              # Compile and run immediately
spc check                   # Type-check without compiling
spc test                    # Run test suite
spc fmt                     # Format code
spc doc                     # Generate documentation
spc clean                   # Clean build artifacts
spc init                    # Initialize new project
spc --version               # Show compiler version

# Build modes
spc build --debug           # Debug build (with debug info)
spc build --release         # Optimized release build

# Cross-compilation
spc build --target linux-x64
spc build --target windows-x64
spc build --target macos-arm64
```

### Package Manager (`spm`)

```bash
spm init                    # Create new project with sp.toml
spm add <package>           # Add dependency
spm remove <package>        # Remove dependency
spm update                  # Update dependencies
spm install                 # Install all dependencies
spm build                   # Build project
spm run                     # Build and run
spm publish                 # Publish to registry
spm search <query>          # Search packages
```

### Developer Tools

| Tool | Command | Description |
|------|---------|-------------|
| **Compiler** | `spc` | LLVM-based native compiler |
| **Package Manager** | `spm` | Dependency management |
| **Formatter** | `spc fmt` | Automatic code formatting |
| **Test Runner** | `spc test` | Built-in testing framework |
| **Doc Generator** | `spc doc` | API documentation |
| **Language Server** | `spls` | IDE support (LSP) |
| **Debugger** | DAP-based | VS Code debugging |

---

## 🧪 Testing

SP has a built-in testing framework:

```sp
test "addition works" {
    assert(add(2, 3) == 5);
}

test "user creation" {
    User user = new User("Shanto", 21);
    assert(user.getName() == "Shanto");
}
```

Run tests:

```bash
spc test
```

---

## 💬 Error Messages

SP provides **human-friendly** error messages inspired by Rust:

```
error[E1001]: type mismatch

  --> src/main.sp:8:14

 8 | int age = "21";
   |           ^^^^

 = expected: int
   found:    string

 = help: convert the string to an integer using int.parse("21")
```

---

## 📚 Standard Library

| Module | Description |
|--------|-------------|
| `std.io` | Input/output, println, readLine |
| `std.fs` | File system operations |
| `std.math` | Mathematical functions |
| `std.string` | String manipulation |
| `std.collections` | List, Map, Set, Stack, Queue |
| `std.time` | Date, time, duration |
| `std.json` | JSON parsing and serialization |
| `std.http` | HTTP client/server |
| `std.net` | Networking |
| `std.crypto` | Cryptographic functions |
| `std.regex` | Regular expressions |
| `std.concurrent` | Threading, async, channels |
| `std.process` | Process management |
| `std.os` | OS interaction, environment |
| `std.testing` | Test assertions and utilities |

---

## 🎯 Target Platforms

| Platform | Architecture | Status |
|----------|-------------|--------|
| Windows | x64 | 🟡 Planned |
| Linux | x64 | 🟡 Planned |
| Linux | ARM64 | 🟡 Planned |
| macOS | ARM64 (Apple Silicon) | 🟡 Planned |
| macOS | x64 | 🟡 Planned |
| WebAssembly | WASM | 🔮 Future |
| Android | ARM64 | 🔮 Future |
| iOS | ARM64 | 🔮 Future |

---

## 🗺️ Roadmap

| Version | Milestone | Status |
|---------|-----------|--------|
| **SP 0.1** | Lexer + Parser | 🟡 Next |
| **SP 0.2** | AST + Semantic Analysis | ⬜ Planned |
| **SP 0.3** | Basic LLVM Compiler | ⬜ Planned |
| **SP 0.4** | Functions + Control Flow | ⬜ Planned |
| **SP 0.5** | Classes + Generics + Collections | ⬜ Planned |
| **SP 0.6** | Standard Library | ⬜ Planned |
| **SP 0.7** | Package Manager (`spm`) | ⬜ Planned |
| **SP 0.8** | Language Server + VS Code | ⬜ Planned |
| **SP 0.9** | Production Candidate | ⬜ Planned |
| **SP 1.0** | 🎉 Stable Release | ⬜ Planned |

### Post 1.0 Vision

- **SP 1.x** — Async/Await, Concurrency, Traits, Extension Methods
- **SP 2.0** — Advanced Memory Management (Ownership/Borrowing), Reflection, Annotations
- **SP 3.0** — WebAssembly, Mobile Targets, GUI Framework

---

## 🏛️ Tech Stack

| Component | Technology |
|-----------|-----------|
| Implementation Language | C++17/20 |
| Compiler Backend | LLVM 17+ |
| Build System | CMake 3.20+ |
| Test Framework | GoogleTest / Catch2 |
| Parser Strategy | Hand-written Recursive Descent + Pratt |
| Configuration | TOML (`sp.toml`) |
| CI/CD | GitHub Actions |
| Editor Support | VS Code (primary) |
| Debug Protocol | DAP (Debug Adapter Protocol) |
| Language Server | LSP 3.17+ |

---

## 📁 Project Structure

```
sp-language/
│
├── compiler/                     # SP Compiler (spc)
│   ├── include/sp/
│   │   ├── lexer/                # Tokenizer
│   │   ├── parser/               # Recursive descent parser
│   │   ├── ast/                  # Abstract Syntax Tree nodes
│   │   ├── semantic/             # Name resolution, symbol table
│   │   ├── types/                # Type system & type checker
│   │   ├── ir/                   # SP intermediate representation
│   │   ├── codegen/              # LLVM code generation
│   │   └── diagnostics/          # Error reporting engine
│   └── src/                      # Implementation files
│
├── runtime/                      # SP Runtime (memory, GC, threading)
├── stdlib/                       # Standard Library (io, fs, math, ...)
├── tools/                        # spc CLI, spm, formatter
├── language-server/              # LSP implementation
├── vscode-extension/             # VS Code extension
├── tests/                        # Compiler test suite
├── examples/                     # Example SP programs
├── docs/                         # Documentation
└── benchmarks/                   # Performance benchmarks
```

---

## 📖 Documentation

| Document | Description |
|----------|-------------|
| [Language Specification](SP%20Programming%20Language%20—%20Complete%20Language%20%26%20Compiler%20Specification.md) | Complete 135-section language specification |
| [PRD](PRD.md) | Project Requirement Document |
| [Architecture](Architecture.md) | Compiler architecture & folder structure |
| [Rules](Rules.md) | Coding standards & development rules |
| [Phases](Phases.md) | Development phases & milestones |
| [Design](Design.md) | Visual design system & branding |
| [Memory](Memory.md) | Progress tracker |

---

## 🤝 Contributing

We welcome contributions! SP is an ambitious project and there are many ways to help:

- 🐛 **Report Bugs** — Open an issue with reproduction steps
- 💡 **Suggest Features** — Propose language features or tooling improvements
- 🔧 **Submit Pull Requests** — Implement features from the [Phases](Phases.md) document
- 📝 **Improve Documentation** — Help write guides and tutorials
- 🧪 **Write Tests** — Expand the test suite with edge cases

### Development Setup

```bash
# Clone the repository
git clone https://github.com/shantopaul/SP-Language.git
cd SP-Language

# Prerequisites
# - C++17/20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
# - CMake 3.20+
# - LLVM 17+ development libraries
# - Git

# Build the compiler
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
ctest --output-on-failure
```

### Commit Convention

We use [Conventional Commits](https://www.conventionalcommits.org/):

```
feat: add generic type parameter parsing
fix: resolve operator precedence for ternary expressions
refactor: simplify AST visitor pattern
test: add golden tests for string interpolation
docs: update language specification for match expressions
chore: update LLVM dependency to 17.0.6
```

---

## 📊 Example Application

A complete SP program demonstrating core features:

```sp
import std.io;
import std.collections;

class Task {
    private string title;
    private bool completed;

    public Task(string title) {
        this.title = title;
        this.completed = false;
    }

    public fn complete() {
        this.completed = true;
    }

    public fn getTitle() -> string {
        return this.title;
    }

    public fn isCompleted() -> bool {
        return this.completed;
    }
}

fn main() {
    List<Task> tasks = new List<Task>();

    tasks.add(new Task("Learn SP"));
    tasks.add(new Task("Build compiler"));
    tasks.add(new Task("Write tests"));

    tasks[0].complete();

    for (Task task in tasks) {
        string status = task.isCompleted() ? "✅" : "⬜";
        println("${status} ${task.getTitle()}");
    }
}
```

**Output:**

```
✅ Learn SP
⬜ Build compiler
⬜ Write tests
```

---

## 🔖 Versioning

SP follows [Semantic Versioning](https://semver.org/):

```
MAJOR.MINOR.PATCH
```

- **MAJOR** — Breaking language changes (SP 2.0)
- **MINOR** — New features, backward compatible (SP 1.1)
- **PATCH** — Bug fixes (SP 1.0.1)

Once SP reaches **1.0**, the language specification becomes stable. Breaking changes require a new major version.

---

## 📜 License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.

---

## 👤 Author

**Shanto Paul**

- GitHub: [@shantopaul](https://github.com/shantopaul)
- Project: [SP-Language](https://github.com/shantopaul/SP-Language)

---

<div align="center">

**SP Programming Language** — Built with ❤️ for developers who value familiarity and power.

*Familiar Syntax. Modern Power. Native Performance.*

</div>
