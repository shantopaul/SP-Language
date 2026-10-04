# SP Language Specification (Core)

This document formalizes the core language model of the **SP Programming Language**.

---

## 1. Design Goals & Identity

SP is a statically-typed, general-purpose programming language designed to provide:
1. **Familiarity:** Immediate intuitiveness for developers with C, C++, Java, or C# background.
2. **Type Safety:** Static type enforcement with explicit types, safe inference (`var`, `let`, `const`), and compile-time verification.
3. **Null Safety:** First-class nullable types (`T?`), safe navigation (`?.`), and null coalescing (`??`).
4. **Native Compilation:** Direct compilation to optimized native machine binaries via LLVM.
5. **Multi-Syntax Compatibility:** Frontend normalization of canonical SP, Java-style, and C#/C++-style function/declaration grammar into a unified AST.

---

## 2. Program Structure

An SP compilation unit consists of an optional `module` declaration, followed by zero or more `import` statements, followed by top-level declarations (functions, classes, interfaces, structs, enums, variables).

```sp
module myapp.core;

import std.io;

fn main() {
    println("Hello, SP!");
}
```

### Entry Point
Execution begins at the entry point function `main`. Both signatures are supported:
- `fn main() -> void` or `fn main()`
- `fn main(string[] args) -> int` or `fn main(string[] args)`
- Compatibility syntax: `public static void main()`

---

## 3. Scoping & Visibility

SP enforces lexical block scoping:
- Variables declared within `{ ... }` blocks are scoped strictly to that block and shadowed outer variables where permitted.
- Visibility modifiers for class/struct members:
  - `public`: Accessible from any module.
  - `protected`: Accessible within defining class and subclasses.
  - `private`: Accessible only within the defining class scope.
  - `internal`: Accessible only within the same module/package.

---

## 4. Execution Model

- Primary target: Native binary execution (x86_64, ARM64) compiled via LLVM backend.
- Memory management:
  - Primitive and struct values are stack-allocated / value-stored.
  - Objects are heap-allocated with automatic memory management (RAII / managed runtime).
  - Unsafe blocks (`unsafe { ... }`) permit raw pointer operations for systems programming.
