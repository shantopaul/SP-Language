<div align="center">

<br>

```
                         ███████╗██████╗     ██╗      █████╗ ███╗   ██╗ ██████╗ 
                         ██╔════╝██╔══██╗    ██║     ██╔══██╗████╗  ██║██╔════╝ 
                         ███████╗██████╔╝    ██║     ███████║██╔██╗ ██║██║  ███╗
                         ╚════██║██╔═══╝     ██║     ██╔══██║██║╚██╗██║██║   ██║
                         ███████║██║         ███████╗██║  ██║██║ ╚████║╚██████╔╝
                         ╚══════╝╚═╝         ╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝ ╚═════╝ 
```

### A modern, general-purpose programming language

*Familiar Syntax · Modern Safety · Native Performance*

<br>

[![MIT License](https://img.shields.io/badge/license-MIT-blue?style=for-the-badge)](LICENSE)
&nbsp;
[![Version](https://img.shields.io/badge/version-4.0.0_GA-06b6d4?style=for-the-badge)](#-roadmap)
&nbsp;
[![C++](https://img.shields.io/badge/C%2B%2B-17%2F20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)](#-tech-stack)
&nbsp;
[![LLVM](https://img.shields.io/badge/LLVM-17%2B-262D3A?style=for-the-badge&logo=llvm&logoColor=white)](#-compiler-architecture)

[![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat-square&logo=windows&logoColor=white)](#-platform-support)
&nbsp;
[![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat-square&logo=linux&logoColor=black)](#-platform-support)
&nbsp;
[![macOS](https://img.shields.io/badge/macOS-000000?style=flat-square&logo=apple&logoColor=white)](#-platform-support)

<br>

[Why SP?](#-why-sp) · [Quick Start](#-quick-start) · [Language Guide](#-language-guide) · [Architecture](#-compiler-architecture) · [Roadmap](#-roadmap) · [Docs](#-documentation) · [Contributing](#-contributing)

<br>

🌐 **Official Website:** [https://splang.shantopaul.com](https://splang.shantopaul.com) &nbsp;|&nbsp; 📖 **Documentation:** [https://docs.splang.shantopaul.com](https://docs.splang.shantopaul.com) &nbsp;|&nbsp; 📦 **Package Registry:** [https://registry.splang.shantopaul.com](https://registry.splang.shantopaul.com)

<br>

</div>

---

## 💡 Why SP?

**SP** is a programming language for developers who value both **familiarity** and **power**. If you know C, C++, Java, or C# — you already know how to write SP.

Unlike other modern languages that force entirely new syntax paradigms, SP meets developers where they are:

```sp
import std.io;

class User {
    private string name;
    private int age;

    public User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public fn greet() {
        println("Hi, I'm ${name} and I'm ${age} years old.");
    }
}

fn main() {
    User user = new User("Shanto", 21);
    user.greet();
}
```

> **☝️ If you understood that code instantly — SP is for you.**

<br>

### How SP Compares

| Feature | C++ | Java | C# | Rust | Go | **SP 2.0** |
|:--------|:---:|:----:|:--:|:----:|:--:|:------:|
| Native binaries | ✅ | ❌ | ❌ | ✅ | ✅ | **✅** |
| Zero GC runtime pause (Deterministic ARC) | ✅ | ❌ | ❌ | ✅ | ❌ | **✅** |
| Null safety | ❌ | ❌ | ✅ | ✅ | ❌ | **✅** |
| Type inference | Partial | Partial | ✅ | ✅ | ✅ | **✅** |
| Zero-cost Generics | ✅ | ❌ | ✅ | ✅ | ❌ | **✅** |
| Pattern matching | Limited | ✅ | ✅ | ✅ | ❌ | **✅** |
| Familiar OOP & Interfaces | ✅ | ✅ | ✅ | ❌ | ❌ | **✅** |
| Compile-Time Metaprogramming (`comptime`) | ✅ | ❌ | ❌ | ❌ | ❌ | **✅** |
| First-Class WebAssembly (`wasm32-wasi`) | Limited | ❌ | ❌ | ✅ | ✅ | **✅** |
| Structured Coroutines (`async`/`await`) | Partial | ❌ | ✅ | ✅ | ✅ | **✅** |
| Non-breaking Language Editions | ❌ | ❌ | ❌ | ✅ | ❌ | **✅** |
| Built-in package manager (`spm`) | ❌ | ❌ | ✅ | ✅ | ✅ | **✅** |
| Human-friendly compiler errors | ❌ | ❌ | ❌ | ✅ | ✅ | **✅** |

<br>

### Core Design Principles

| Principle | Description |
|:----------|:------------|
| **🔤 Familiar Syntax** | Recognizable constructs from C/C++/Java/C# — zero learning curve for experienced devs |
| **🔒 Modern Safety** | Static typing, null safety (`?`, `?.`, `??`), Result types, and managed memory |
| **⚡ Native Performance** | Compiles directly to machine code via LLVM — no VM, no interpreter |
| **🧩 Multi-Style** | Write `fn add() -> int` or `int add()` — both compile to the same native code |
| **🛠️ Batteries Included** | Compiler, package manager, formatter, test runner, language server, debugger |

---

## 🚀 Quick Start

### Installation

**1-Click Install & Universal Auto-Upgrade:**

- **Windows (PowerShell):**
  ```powershell
  irm https://dl.splang.shantopaul.com/install.ps1 | iex
  ```
- **Linux & macOS (Bash):**
  ```bash
  curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh
  ```

> 💡 **Auto-Upgrade:** Run `spc upgrade` at any time, or re-run the installer command above to automatically upgrade to the latest release in 3–5 seconds with zero data loss.

**Or Build From Source:**

```bash
# Clone the repository
git clone https://github.com/shantopaul/SP-Language.git
cd SP-Language

# Build (requires C++17 compiler, CMake 3.20+, LLVM 17+)
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build .
```

### Hello, World!

Create `hello.sp`:

```sp
fn main() {
    println("Hello, SP!");
}
```

```bash
$ spc run hello.sp
Hello, SP!
```

```bash
$ spc build hello.sp        # Produces native executable
$ ./hello                    # Run it directly
Hello, SP!
```

### Create a Project

```bash
$ spm init my-app
$ cd my-app
$ spm run
```

Generated project structure:

```
my-app/
├── sp.toml              # Project configuration
├── src/
│   └── main.sp          # Entry point
├── tests/
│   └── main_test.sp     # Tests
└── README.md
```

`sp.toml`:
```toml
[package]
name = "my-app"
version = "0.1.0"
authors = ["Shanto Paul"]

[language]
version = "1.0"

[dependencies]

[build]
entry = "src/main.sp"
```

---

## 📖 Language Guide

<details>
<summary><b>📦 Variables & Types</b></summary>

<br>

```sp
// Explicit typing
int age = 21;
double salary = 50000.50;
float rate = 95.5;
bool active = true;
char grade = 'A';
string name = "Shanto";

// Type inference
var count = 42;            // int
let message = "Hello";     // string (immutable)

// Constants
const int MAX = 100;
const PI = 3.14159;        // inferred const
```

**Primitive types:**

| Category | Types |
|----------|-------|
| Integers | `int` `uint` `int8` `int16` `int32` `int64` `uint8` `uint16` `uint32` `uint64` |
| Floating | `float` `double` |
| Other | `bool` `char` `string` `byte` |

</details>

<details>
<summary><b>🔧 Functions</b></summary>

<br>

SP accepts **multiple function syntax styles** — all compile to identical machine code:

```sp
// ✅ SP-native style
fn add(int a, int b) -> int {
    return a + b;
}

// ✅ Java/C++/C# style (also valid!)
int add(int a, int b) {
    return a + b;
}

// ✅ Short form
fn add(a, b) => a + b;
```

**Higher-order functions & lambdas:**

```sp
var multiply = (int a, int b) => a * b;
println(multiply(5, 4));   // 20

let numbers = [1, 2, 3, 4, 5];
let doubled = numbers.map(x => x * 2);         // [2, 4, 6, 8, 10]
let evens = numbers.filter(x => x % 2 == 0);   // [2, 4]
```

</details>

<details>
<summary><b>🔀 Control Flow</b></summary>

<br>

```sp
// If / Else
if (age >= 18) {
    println("Adult");
} else if (age >= 13) {
    println("Teenager");
} else {
    println("Child");
}

// For loops
for (int i = 0; i < 10; i++) {
    println(i);
}

// Range loops
for (i in 0..10) {
    println(i);
}

// While / Do-While
while (running) { update(); }
do { retry(); } while (!success);

// Pattern matching (modern)
match status {
    Status.Active    => println("Active"),
    Status.Pending   => println("Waiting..."),
    Status.Completed => println("Done!"),
    _                => println("Unknown")
}

// Switch (classic)
switch (day) {
    case 1:  println("Monday"); break;
    case 2:  println("Tuesday"); break;
    default: println("Other");
}
```

</details>

<details>
<summary><b>🏗️ Classes & OOP</b></summary>

<br>

```sp
// Classes with access modifiers
class User {
    private string name;
    protected int age;
    public string email;

    public User(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public fn getName() -> string { return this.name; }
}

// Inheritance
class Animal {
    fn speak() { println("..."); }
}

class Dog extends Animal {
    override fn speak() { println("Woof!"); }
}

// Also valid with C# colon syntax:
class Cat : Animal {
    override fn speak() { println("Meow!"); }
}

// Interfaces
interface Serializable {
    fn serialize() -> string;
}

class Config implements Serializable {
    fn serialize() -> string { return "{}"; }
}

// Abstract classes
abstract class Shape {
    abstract fn area() -> double;
}

// Static members
class MathUtil {
    static fn square(int x) -> int { return x * x; }
}
int result = MathUtil.square(5);  // 25
```

</details>

<details>
<summary><b>🧬 Generics</b></summary>

<br>

```sp
class Box<T> {
    T value;
    Box(T value) { this.value = value; }
    T get() { return value; }
}

Box<int> intBox = new Box<int>(42);
Box<string> strBox = new Box<string>("hello");

// Generic functions
fn max<T>(T a, T b) -> T {
    return a > b ? a : b;
}

// Generic collections
List<int> numbers = new List<int>();
Map<string, User> users = new Map<string, User>();
```

</details>

<details>
<summary><b>🛡️ Null Safety</b></summary>

<br>

```sp
// Nullable types use ?
User? user = findUser(id);      // might be null

// Safe navigation
println(user?.name);             // null if user is null

// Null coalescing
string name = user?.name ?? "Anonymous";

// Non-nullable (default) — this won't compile:
User user = null;  // ❌ Compile error!
```

</details>

<details>
<summary><b>⚠️ Error Handling</b></summary>

<br>

```sp
// Exception-based (familiar)
try {
    let data = readFile("config.json");
    process(data);
} catch (FileNotFound e) {
    println("File missing: ${e.message}");
} catch (Exception e) {
    println("Error: ${e.message}");
} finally {
    cleanup();
}

// Result-based (modern)
fn divide(int a, int b) -> Result<int, Error> {
    if (b == 0) return Error("Division by zero");
    return Ok(a / b);
}
```

</details>

<details>
<summary><b>📝 Strings</b></summary>

<br>

```sp
string name = "Shanto";

// Interpolation
string msg = "Hello, ${name}! You are ${age} years old.";

// Concatenation
string full = "Hello " + name;

// Built-in methods
name.length();           // 6
name.upper();            // "SHANTO"
name.lower();            // "shanto"
name.contains("ant");    // true
name.split("a");         // ["Sh", "nto"]
name.trim();
name.replace("S", "s");
```

</details>

<details>
<summary><b>📦 Collections</b></summary>

<br>

```sp
// Arrays
int[] nums = {1, 2, 3, 4, 5};
let items = [10, 20, 30];

// Lists (dynamic)
List<string> names = new List<string>();
names.add("Shanto");
names.push("Paul");

// Maps
let config = {
    "host": "localhost",
    "port": 8080
};

Map<string, int> scores = new Map<string, int>();
scores["math"] = 95;

// Functional operations
let result = [1, 2, 3, 4, 5]
    .map(x => x * x)
    .filter(x => x > 10)
    .reduce((a, b) => a + b);
```

</details>

<details>
<summary><b>📦 Structs, Enums & Traits</b></summary>

<br>

```sp
// Structs (value types)
struct Point {
    int x;
    int y;
}
Point p;
p.x = 10;
p.y = 20;

// Enums
enum Color { Red, Green, Blue }
Color c = Color.Red;

// Operator overloading
struct Vector2 {
    float x; float y;

    operator +(Vector2 a, Vector2 b) {
        return Vector2(a.x + b.x, a.y + b.y);
    }
}

// Traits
trait Printable {
    fn display();
}

class Log with Printable {
    fn display() { println("Log entry"); }
}

// Extension methods
extend string {
    fn isEmail() -> bool { /* ... */ }
}
"test@email.com".isEmail();  // true
```

</details>

<details>
<summary><b>🌐 Async & Concurrency</b></summary>

<br>

```sp
// Async/Await
async fn fetchData(string url) {
    let response = await http.get(url);
    println(response.body);
}

// Threads
thread {
    println("Running in background");
}

// Future primitives
async    await    spawn
channel  mutex    atomic
```

</details>

<details>
<summary><b>📁 Modules & Imports</b></summary>

<br>

```sp
// Declare module
module myapp.services;

// Import specific
import myapp.models.User;

// Import all
import myapp.utils.*;

// Standard library
import std.io;
import std.math;
import std.collections;
import std.json;
import std.http;
import std.fs;
```

</details>

<details>
<summary><b>🧪 Testing</b></summary>

<br>

```sp
test "addition is correct" {
    assert(add(2, 3) == 5);
}

test "user has default name" {
    User user = new User("Shanto", 21);
    assert(user.getName() == "Shanto");
}

test "division by zero returns error" {
    let result = divide(10, 0);
    assert(result.isError());
}
```

```bash
$ spc test

  ✅ addition is correct .............. passed (0.2ms)
  ✅ user has default name ............ passed (0.1ms)
  ✅ division by zero returns error ... passed (0.1ms)

  3 tests passed | 0 failed | 0.4ms total
```

</details>

<details>
<summary><b>🔗 Native Interop & Unsafe</b></summary>

<br>

```sp
// C FFI
extern "C" {
    fn printf(string format);
}

// Unsafe blocks for low-level operations
unsafe {
    pointer p = ...
    // Direct memory manipulation
}

// Annotations
@Serializable
@Route("/api/users")
@Deprecated("Use newMethod() instead")
@inline
```

</details>

---

## 🏛️ Compiler Architecture

SP uses a multi-stage compilation pipeline targeting LLVM for native code generation:

```
    ╔══════════════════════════════════════════════════════════════╗
    ║                    SP COMPILER PIPELINE                     ║
    ╠══════════════════════════════════════════════════════════════╣
    ║                                                             ║
    ║   source.sp                                                 ║
    ║       │                                                     ║
    ║       ▼                                                     ║
    ║   ┌─────────┐    Characters → Tokens                        ║
    ║   │  LEXER  │    Keywords, identifiers, literals, operators ║
    ║   └────┬────┘                                               ║
    ║        │                                                    ║
    ║        ▼                                                    ║
    ║   ┌─────────┐    Tokens → Abstract Syntax Tree              ║
    ║   │ PARSER  │    Recursive descent + Pratt parsing          ║
    ║   └────┬────┘                                               ║
    ║        │                                                    ║
    ║        ▼                                                    ║
    ║   ┌─────────┐    Scoping, symbol resolution, type checking  ║
    ║   │SEMANTIC │    Null safety, generics, overload resolution ║
    ║   │ANALYSIS │                                               ║
    ║   └────┬────┘                                               ║
    ║        │                                                    ║
    ║        ▼                                                    ║
    ║   ┌─────────┐    High-level intermediate representation     ║
    ║   │  SP IR  │    Platform-independent optimization          ║
    ║   └────┬────┘                                               ║
    ║        │                                                    ║
    ║        ▼                                                    ║
    ║   ┌─────────┐    Low-level IR + LLVM optimizations          ║
    ║   │LLVM IR  │    Dead code elimination, inlining, etc.      ║
    ║   └────┬────┘                                               ║
    ║        │                                                    ║
    ║        ▼                                                    ║
    ║   ┌─────────┐                                               ║
    ║   │ NATIVE  │    .exe (Windows) / ELF (Linux) / Mach-O (Mac)║
    ║   │ BINARY  │                                               ║
    ║   └─────────┘                                               ║
    ║                                                             ║
    ╚══════════════════════════════════════════════════════════════╝
```

### Syntax Compatibility — SP's Unique Superpower

SP's parser accepts **multiple syntax styles** and normalizes them to a **single canonical AST**. This means Java, C#, and C++ developers can write natural code from day one:

```sp
// All three compile to IDENTICAL machine code:

fn add(int a, int b) -> int { return a + b; }    // SP style
int add(int a, int b) { return a + b; }           // Java/C++ style
fn add(a, b) => a + b;                            // Short form
```

```
   Java dev          C# dev          C++ dev         SP dev
      │                │               │               │
      ▼                ▼               ▼               ▼
 ┌─────────────────────────────────────────────────────────┐
 │              SP Parser (Compatibility Layer)            │
 └────────────────────────┬────────────────────────────────┘
                          │
                          ▼
                    Common SP AST
                          │
                          ▼
                     LLVM Codegen
                          │
                          ▼
                   Native Executable
```

---

## 💬 Error Messages

SP produces **human-friendly diagnostics** inspired by Rust's error reporting:

```
error[E1001]: type mismatch
  --> src/main.sp:8:14
   |
 8 |     int age = "twenty-one";
   |               ^^^^^^^^^^^^
   |
   = expected: int
     found:    string
   = help: convert the string to an integer using int.parse("twenty-one")
```

```
error[E1002]: undefined variable
  --> src/main.sp:12:5
   |
12 |     println(username);
   |             ^^^^^^^^
   |
   = `username` is not defined in this scope
   = help: did you mean `userName`?
```

```
  Compiling my-app v0.1.0
   Finished release build in 0.82s
        Bin target/release/my-app (2.4 MB)
```

---

## 🧰 Complete Toolchain

| Tool | Binary | Description |
|:-----|:-------|:------------|
| **Compiler** | `spc` | LLVM-based native compiler |
| **Package Manager** | `spm` | Dependency management & project scaffolding |
| **Formatter** | `spc fmt` | Opinionated code formatter |
| **Test Runner** | `spc test` | Built-in test framework |
| **Doc Generator** | `spc doc` | API documentation from `///` comments |
| **Type Checker** | `spc check` | Fast type-checking without full compilation |
| **Language Server** | `spls` | LSP for IDE integration |
| **Debugger** | DAP | Breakpoints, stepping, variable inspection |

<details>
<summary><b>Full CLI Reference</b></summary>

<br>

```bash
# Compiler (spc)
spc build [file]                # Compile to native executable
spc build --release             # Optimized release build
spc build --debug               # Debug build with symbols
spc build --target linux-x64    # Cross-compile
spc run [file]                  # Compile and execute
spc check                       # Type-check only (fast)
spc test                        # Run all tests
spc fmt                         # Format all source files
spc doc                         # Generate documentation
spc clean                       # Remove build artifacts
spc doctor                      # Diagnose installation & toolchain health
spc setup                       # Auto-provision portable LLVM toolchain
spc upgrade                     # In-place auto-upgrade compiler & stdlib
spc uninstall                   # Cleanly remove SP Language and toolchains
spc init [name]                 # Create new project
spc --version                   # Show version info

# Package Manager (spm)
spm init [name]                 # Initialize project with sp.toml
spm add <package>               # Add dependency
spm remove <package>            # Remove dependency
spm update                      # Update all dependencies
spm install                     # Install from sp.lock
spm build                       # Build project
spm run                         # Build and run
spm publish                     # Publish to registry
spm search <query>              # Search package registry
```

</details>

---

## 📚 Standard Library

| Module | Description | Example |
|:-------|:------------|:--------|
| `std.io` | Console I/O | `println("Hello")` |
| `std.fs` | File system | `fs.readText("data.txt")` |
| `std.math` | Mathematics | `math.sqrt(144)` |
| `std.string` | String ops | `name.upper()` |
| `std.collections` | Data structures | `List<T>`, `Map<K,V>`, `Set<T>` |
| `std.time` | Date & time | `time.now()` |
| `std.json` | JSON parse/serialize | `json.parse<Config>(str)` |
| `std.http` | HTTP client/server | `http.get(url)` |
| `std.net` | Networking | Sockets, TCP, UDP |
| `std.crypto` | Cryptography | Hashing, encryption |
| `std.regex` | Regular expressions | Pattern matching |
| `std.concurrent` | Concurrency | Threads, channels, mutex |
| `std.process` | Process management | `process.exec("cmd")` |
| `std.os` | OS interaction | `os.env("PATH")` |
| `std.testing` | Test utilities | `assert`, `expect` |

---

## 🎯 Platform Support

| Platform | Arch | Status |
|:---------|:-----|:------:|
| Windows | x86-64 | 🟡 In Development |
| Linux | x86-64 | 🟡 In Development |
| Linux | ARM64 | 📋 Planned |
| macOS | ARM64 (Apple Silicon) | 📋 Planned |
| macOS | x86-64 | 📋 Planned |
| WebAssembly | WASM | 🔮 Future |
| Android | ARM64 | 🔮 Future |
| iOS | ARM64 | 🔮 Future |

---

## 🗺️ Roadmap
 
 ```
  v1.0 ──── v1.5 ──── v1.6 ──── v1.7 ──── v2.0 ──── v3.1 ──── v4.0 (GA)
      │        │        │        │        │        │          │
      ▼        ▼        ▼        ▼        ▼        ▼          ▼
    Core     Sync &    HTTP    Registry Comptime, Web &    ARC, Async Epoll,
   Release  Channels  & JSON      GA     WASI   Toolchain  Bindgen & IDE Depth
 ```
 
 | Version | Milestone | Key Deliverables | Status |
 |:--------|:----------|:-----------------|:------:|
 | `0.1`–`0.9` | **Bootstrap & Compiler Pipeline** | Lexer, parser, type checker, LLVM backend, generics, stdlib | ✅ Done |
 | `1.0` | **🎉 Stable Release** | Production compiler, VS Code extension, `spm` CLI, full docs | ✅ Done |
 | `1.5` | **Concurrency & Channels** | Worker thread pool, lock-free MPMC channels, atomic memory | ✅ Done |
 | `1.6` | **Networking & JSON Engine** | Non-blocking TCP/HTTP client/server, high-throughput JSON | ✅ Done |
 | `1.7` | **Package Registry GA** | Supply chain verification, cryptographic checksums, publishing | ✅ Done |
 | `2.0` | **🚀 SP 2.0 General Availability** | `comptime` execution, `wasm32-wasi` target, editions (`2026`/`2028`), coroutines (`async`/`await`), self-hosted frontend, `spc fix` (346/346 CTests passing) | ✅ Done |
 | `3.1` | **🌐 Enterprise Backend & Universal Toolchain** | `std.web` framework, DB pooling, Redis, WebSockets, Argon2id/JWT, OpenAPI 3.0.3, `spc upgrade`, cross-platform installers | ✅ Done |
 | `4.0` | **⚡ SP 4.0.0 General Availability** | Deterministic ARC & cycle breaking, `Result`/`Option` + `?` & `try`/`catch`, real `async`/`await` + `epoll` event loop, `spc bindgen` FFI, LSP/DAP depth | ✅ Done |


---

## 🏗️ Tech Stack

| Component | Choice | Rationale |
|:----------|:-------|:----------|
| **Language** | C++17/20 | Best LLVM API integration, performance |
| **Backend** | LLVM 17+ | Battle-tested codegen for all major platforms |
| **Build System** | CMake 3.20+ | Industry standard for C++ |
| **Testing** | GoogleTest / Catch2 | Mature C++ test frameworks |
| **Parser** | Hand-written RD + Pratt | Superior error messages vs. generated parsers |
| **Config** | TOML (`sp.toml`) | Human-readable project configuration |
| **CI/CD** | GitHub Actions | Automated build, test, release |
| **LSP** | LSP 3.17+ | Universal IDE support |
| **Debug** | DAP | VS Code debugger integration |

---

## 📂 Project Structure

```
SP-Language/
│
├── compiler/                          # 🔨 SP Compiler (spc)
│   ├── include/sp/
│   │   ├── lexer/                     #    Tokenizer
│   │   ├── parser/                    #    Recursive descent + Pratt
│   │   ├── ast/                       #    AST node definitions
│   │   ├── semantic/                  #    Name resolver, symbol table
│   │   ├── types/                     #    Type system & checker
│   │   ├── ir/                        #    SP intermediate representation
│   │   ├── codegen/                   #    LLVM code generation
│   │   └── diagnostics/              #    Error reporting engine
│   └── src/                           #    Implementation (.cpp)
│
├── runtime/                           # ⚙️ SP Runtime (GC, memory, threading)
├── stdlib/                            # 📚 Standard Library
├── tools/                             # 🧰 Developer Toolchain (spc, spm, spls, spd, debugger)
├── editors/                           # 🔌 VS Code extension & language tooling
├── benchmarks/                        # 📊 Multi-language performance benchmarks
├── examples/                          # 📝 Code examples & fullstack CRUD application
├── tests/                             # ✅ Comprehensive unit & golden test suite
├── installer/                         # 📦 Cross-platform installer definitions
├── Logo/                              # 🎨 Official brand assets, emblems, and vector lockups
├── docs/                              # 📖 Comprehensive documentation & specifications
│   ├── architecture/                  #    System architecture & compiler pipeline
│   ├── design/                        #    Design system & theme specifications
│   ├── planning/                      #    Roadmap, phases, rules, and PRD
│   ├── specifications/                #    Formal language & compiler specifications
│   ├── reports/                       #    QA testing & verification reports
│   └── SP-Syntax-Guide.md             #    100-section complete syntax guide
│
├── splang/                            # 🌐 Official landing portal (Next.js 14)
├── splang-docs/                       # 📖 Interactive documentation portal (Next.js 14)
└── splang-registry/                   # 📦 Package registry portal (Next.js 14)
```

---

## 📖 Documentation

### 🌐 Official Online Resources

- **Official Website:** [splang.shantopaul.com](https://splang.shantopaul.com)
- **Official Documentation:** [docs.splang.shantopaul.com](https://docs.splang.shantopaul.com)
- **Package Registry:** [registry.splang.shantopaul.com](https://registry.splang.shantopaul.com)

### 📚 Repository Specifications & Guides

| Document | Description |
|:---------|:------------|
| **[Syntax Guide](docs/SP-Syntax-Guide.md)** | Complete 100-section syntax showcase with working examples |
| **[Language & Compiler Spec](docs/specifications/COMPILER_LANGUAGE_SPEC.md)** | Complete 135-section language and compiler specification |
| **[Architecture](docs/architecture/ARCHITECTURE.md)** | Compiler pipeline, intermediate representation, and runtime design |
| **[Development Roadmap](docs/planning/PHASES.md)** | Comprehensive roadmap and task breakdown |
| **[Coding Rules & Guidelines](docs/planning/RULES.md)** | Engineering standards, memory safety rules, and boundaries |
| **[Project Requirements (PRD)](docs/planning/PRD.md)** | Core vision, target audience, and language pillars |
| **[Design System](docs/design/DESIGN.md)** | Brand tokens, CLI palette, and error diagnostics styling |
| **[Memory & Progress Log](docs/planning/MEMORY.md)** | Project milestones and architecture tracker |

---

## 🤝 Contributing

SP is an open-source project and contributions are welcome! Whether it's code, documentation, bug reports, or feature suggestions — every contribution matters.

### Getting Started

```bash
# 1. Fork & clone
git clone https://github.com/YOUR-USERNAME/SP-Language.git

# 2. Install prerequisites
#    - C++17/20 compiler (GCC 10+, Clang 12+, MSVC 2019+)
#    - CMake 3.20+
#    - LLVM 17+ development libraries

# 3. Build
mkdir build && cd build
cmake .. && cmake --build .

# 4. Run tests
ctest --output-on-failure

# 5. Create a branch & submit PR
git checkout -b feature/my-feature
```

### Contribution Areas

| Area | Good For | Difficulty |
|:-----|:---------|:-----------|
| 🐛 Bug reports | Everyone | Easy |
| 📝 Documentation | Writers, new contributors | Easy |
| 🧪 Test cases | Developers familiar with compilers | Medium |
| 🔧 Compiler features | C++ developers | Hard |
| 🎨 VS Code extension | TypeScript developers | Medium |
| 📦 Standard library | SP developers | Medium |

### Commit Convention

```
feat:     add generic type parameter parsing
fix:      resolve operator precedence for ternary
refactor: simplify AST visitor pattern
test:     add golden tests for string interpolation
docs:     update language spec for match expressions
chore:    bump LLVM dependency to 17.0.6
```

See **[Rules](docs/planning/RULES.md)** for complete coding standards and **[Phases](docs/planning/PHASES.md)** for development roadmap.

---

## 📊 Example: Complete Application

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

    public fn toString() -> string {
        string icon = this.completed ? "✅" : "⬜";
        return "${icon} ${this.title}";
    }
}

fn main() {
    let tasks = [
        new Task("Learn SP"),
        new Task("Build compiler"),
        new Task("Write standard library"),
        new Task("Create VS Code extension")
    ];

    tasks[0].complete();
    tasks[1].complete();

    println("=== SP TODO App ===\n");

    for (Task task in tasks) {
        println(task.toString());
    }

    int done = tasks.filter(t => t.completed).length();
    println("\n${done}/${tasks.length()} completed");
}
```

**Output:**
```
=== SP TODO App ===

✅ Learn SP
✅ Build compiler
⬜ Write standard library
⬜ Create VS Code extension

2/4 completed
```

---

## 📜 License

This project is licensed under the **MIT License** — see [LICENSE](LICENSE) for details.

---

## 👤 Author

<table>
<tr>
<td>

**Shanto Paul**

[![GitHub](https://img.shields.io/badge/-shantopaul-181717?style=flat-square&logo=github)](https://github.com/shantopaul)

Creator & Lead Developer of the SP Programming Language

</td>
</tr>
</table>

---

<div align="center">

<br>

**SP Programming Language**

*Familiar Syntax · Modern Safety · Native Performance*

<br>

If you find SP interesting, consider giving it a ⭐

<br>

</div>
