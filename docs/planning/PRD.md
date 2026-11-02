# Product Requirements Document (PRD): SP Programming Language

## 1. Executive Summary
The **SP Programming Language (SP)** is a modern, general-purpose programming language designed to bridge the gap between the familiar syntax of C, C++, Java, and C# and the safety, performance, and modern features of languages like Rust and Swift. SP compiles to native machine code via LLVM and provides a comprehensive ecosystem out of the box, including a package manager (`spm`), language server (`spls`), and native debugger.

## 2. Project Overview

| Attribute | Details |
| :--- | :--- |
| **Project Name** | SP Programming Language |
| **Short Name** | SP |
| **Source Extension** | `.sp` |
| **Compiler** | `spc` (LLVM-based native compiler) |
| **Package Manager** | `spm` |
| **Language Type** | General-Purpose Programming Language |
| **Primary Paradigms** | Procedural, Object-Oriented, Functional, Generic |
| **Target Platforms** | Windows, Linux, macOS |
| **Implementation** | C++ (Recommended Primary) or Rust |
| **Execution** | Native Executable (via LLVM) or alternative SP Virtual Machine |

## 3. Problem Statement
Developers coming from traditional C-family languages (C++, Java, C#) often face a steep learning curve when adopting modern, memory-safe, and highly performant systems languages (like Rust). Conversely, languages with lower barriers to entry often sacrifice native performance or memory control. There is a lack of a unified programming language that offers modern constructs (null safety, pattern matching, async/await, Result types) while retaining familiar syntax and seamless low-level interoperability without sacrificing native performance.

## 4. Solution
SP solves this by providing:
- **Familiar Syntax & Syntax Compatibility Layer**: Allows standard SP-native syntax alongside C#-style or Java-style syntax, normalizing them into a single Abstract Syntax Tree (AST).
- **Native Performance**: Compiles down to LLVM IR, leveraging state-of-the-art optimizations.
- **Modern Safety**: Enforces static typing, null safety (`?`, `?.`, `??`), and explicit error handling via `Result<T, E>`.
- **First-Class Tooling**: A built-in language server (`spls`), package manager (`spm`), formatter (`spc fmt`), and documentation generator (`spc doc`).

## 5. Target Users
1. **Primary**: Developers with C, C++, Java, C# backgrounds seeking a modern, natively compiled language with familiar syntax.
2. **Secondary**: Students and educators learning programming language design and compiler construction.
3. **Tertiary**: Systems programmers who require native performance combined with modern developer ergonomics.
4. **Long-Term**: Web developers (via WebAssembly), game developers, and CLI tool authors.

## 6. Core Features

### 6.1 Language Semantics & Syntax
- **Type System**: Statically typed with type inference (`var`, `let`).
- **Primitive Types**: `int`, `uint`, `int8-64`, `uint8-64`, `float`, `double`, `bool`, `char`, `string`, `byte`.
- **Object-Oriented Programming**: Classes, interfaces, abstract classes, single inheritance (`extends` or `:`), access modifiers (public, private, protected, internal).
- **Functional Programming**: Lambdas, closures, higher-order functions, map/filter/reduce.
- **Memory Model**: Stack allocation for primitives, managed heap for objects. Optional Garbage Collection (mark-and-sweep initially, generational later) and RAII for resources.
- **Null Safety**: Strict null checks by default, nullable types (`T?`), safe navigation (`?.`), null coalescing (`??`).
- **Error Handling**: Dual-paradigm support: `try/catch/finally` exceptions and modern `Result<T, E>` types.
- **Generics**: Reified generics at both class (`Box<T>`) and function (`fn max<T>()`) levels.
- **Advanced Constructs**: Pattern matching (`match`), struct & algebraic enums, traits, string interpolation (`"Hello ${name}"`).
- **Concurrency**: First-class `async fn`, `await`, `spawn`, channels, and mutexes.
- **Unsafe Mode**: `unsafe { }` blocks for raw pointer manipulation and systems-level programming.
- **Interoperability**: C FFI (`extern "C"`) out of the box, with planned bridges for Java JNI, .NET, and C++.

### 6.2 The Ecosystem & Tooling
The language is only as good as its tooling. The SP ecosystem includes:
- **`spc` (Compiler)**: Native LLVM-based compiler.
- **`spm` (Package Manager)**: Manages dependencies via `sp.toml` and lockfiles. Commands: `init`, `add`, `remove`, `update`, `publish`.
- **`spls` (Language Server)**: Full LSP implementation for IDE integration (VS Code).
- **`spr` (Runtime)**: Handles async execution, threading, memory management, FFI, I/O.
- **Standard Library (`stdlib`)**: Comprehensive modules including `std.io`, `std.fs`, `std.net`, `std.http`, `std.json`, `std.crypto`, `std.concurrent`, `std.testing`.

### 6.3 Compiler CLI Commands
| Command | Description |
| :--- | :--- |
| `spc build [file]` | Compile source to executable |
| `spc run [file]` | Compile and execute immediately |
| `spc check` | Type-check project without compiling |
| `spc test` | Run built-in tests |
| `spc fmt` | Format source code |
| `spc doc` | Generate documentation from code |
| `spc clean` | Clean build artifacts |
| `spc init` | Initialize a new SP project |
| `spc --version` | Show compiler version |
| `spc build --debug` | Create unoptimized build with debug symbols |
| `spc build --release` | Create optimized release build |
| `spc build --target X` | Cross-compile to target architecture/OS |

## 7. Technical Requirements

### 7.1 Compiler Architecture
- **Lexer**: Tokenizes `.sp` source files.
- **Parser**: Hand-written recursive descent parser incorporating Pratt parsing for expressions.
- **AST**: Extensible node system with ~25+ types (Program, Module, ClassDeclaration, Expressions, etc.).
- **Semantic Analyzer**: Symbol table construction, scope resolution, and comprehensive type checking.
- **IR & Code Gen**: Translation to SP Intermediate Representation (SP IR), lowering to LLVM IR, and outputting native machine code.

### 7.2 Human-Friendly Errors
- Rust-style rich error messages.
- Error codes (e.g., `E1000`, `E1001`).
- Source location highlighting, caret pointers, and actionable help suggestions.

### 7.3 Project Configuration (`sp.toml`)
Requires a structured configuration file with the following sections:
- `[package]`: Metadata (name, version, authors).
- `[language]`: Edition and syntax preferences.
- `[dependencies]`: External crates/packages.
- `[build]`: Compiler flags and target overrides.

## 8. Version Roadmap Summary

| Version | Milestone | Focus Areas |
| :--- | :--- | :--- |
| **SP 0.1** | Lexer + Parser | Tokenization, Basic AST Generation |
| **SP 0.2** | AST + Semantic Analysis | Name Resolution, Type Checking |
| **SP 0.3** | Basic LLVM compiler | Primitive types, LLVM IR emission, Executable generation |
| **SP 0.4** | Functions + Classes | OOP, methods, access modifiers |
| **SP 0.5** | Generics + Collections | Type parameters, Lists, Maps |
| **SP 0.6** | Standard Library | `std.io`, `std.fs`, `std.net`, `std.math` |
| **SP 0.7** | Package Manager | `spm`, `sp.toml` integration, dependency resolution |
| **SP 0.8** | Language Server | LSP implementation, VS Code Extension |
| **SP 0.9** | Production Candidate | Bug fixes, optimization, cross-compilation stability |
| **SP 1.0** | Stable Language | API freeze, stable ABI, official release |

## 9. Non-Functional Requirements
- **Performance**: Compilation speeds should rival Go. Runtime performance should rival C++ and Rust.
- **Reliability**: The compiler must not crash on invalid syntax; it must gracefully recover and report errors.
- **Cross-Platform**: The compiler and runtime must execute seamlessly across Windows, Linux, and macOS.
- **Extensibility**: The AST and internal IR must be cleanly separated to allow future backends (e.g., WebAssembly, Cranelift, JVM).

## 10. Risks & Mitigations
- **Risk**: The Syntax Compatibility Layer introduces parsing ambiguity.
  - *Mitigation*: Strictly define syntax resolution rules and prioritize SP-native syntax. Ensure robust test coverage for parser edge cases.
- **Risk**: Memory Management (GC + RAII) overhead.
  - *Mitigation*: Start with a basic mark-and-sweep GC, but design the runtime API to allow plugging in a generational GC later. Use stack allocation heavily.
- **Risk**: LLVM API complexity.
  - *Mitigation*: Maintain a clear abstraction layer (SP IR) before generating LLVM IR to isolate LLVM version migrations.

## 11. Out of Scope (Initial Release)
- 100% source compatibility with Java, C#, or C++.
- Direct development for Mobile apps (Android/iOS).
- Native desktop GUI frameworks or Game Engines.
- Web application frameworks or Database ORMs.
- Full IDE implementation (VS Code extension will be prioritized instead).
- Exhaustive third-party package ecosystem.

## 12. Dependencies
- **LLVM Framework**: For backend code generation and optimization.
- **C++ (>= C++20) or Rust**: As the implementation language for the compiler.
- **Platform OS APIs**: POSIX (Linux/macOS) and Win32 (Windows) for runtime implementation.

## 13. Glossary
- **AST**: Abstract Syntax Tree.
- **FFI**: Foreign Function Interface (for calling C code).
- **LSP**: Language Server Protocol.
- **RAII**: Resource Acquisition Is Initialization (deterministic resource management).
- **spc**: SP Compiler.
- **spls**: SP Language Server.
- **spm**: SP Package Manager.
- **spr**: SP Runtime.
