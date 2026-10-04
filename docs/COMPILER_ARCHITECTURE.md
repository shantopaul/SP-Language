# SP Compiler Architecture Specification

This document details the internal components, design decisions, and data pipelines of the **SP Compiler (`spc`)**.

---

## 1. Pipeline Overview

The SP Compiler pipeline transforms high-level `.sp` source files into optimized native machine code through a series of modular compiler stages:

```
[ .sp Source ]
       │
       ▼
 [ Lexer ] ────────────> [ Token Stream ]
                               │
                               ▼
 [ Parser ] ───────────> [ Raw AST ]
                               │
                               ▼
 [ Semantic Analyzer ] ─> [ Symbol Table & Resolved AST ]
                               │
                               ▼
 [ Type Checker ] ─────> [ Typed AST & Diagnostics ]
                               │
                               ▼
 [ IR Builder ] ───────> [ High-Level SP IR ]
                               │
                               ▼
 [ LLVM Codegen ] ─────> [ LLVM IR Module ]
                               │
                               ▼
 [ LLVM Backend ] ─────> [ Native Machine Code (.exe / ELF) ]
```

---

## 2. Core Subsystems

### 2.1 Lexer (`sp::lexer`)
- Character-by-character scanner handling UTF-8 source streams.
- Recognizes all keywords, multi-character operators (`==`, `->`, `??`), numeric literals (decimal, hexadecimal `0x`, binary `0b`), strings with escape sequences, comments (`//`, `/* */`, `///`).
- Emits structured `Token` instances tracking line, column, and file source locations.

### 2.2 Parser (`sp::parser`)
- Hand-written recursive descent parser for declarations and statements.
- **Pratt Parser** algorithm for expression parsing, handling operator precedence and associativity without left-recursion pitfalls.
- Syntax normalization layer: converts multiple dialect styles (C++/Java/C# function syntax, inheritance tokens) into single canonical AST nodes.

### 2.3 Semantic Analysis & Type Checker (`sp::semantic`, `sp::types`)
- Scoped symbol table supporting block scoping, lexical closures, and module imports.
- Comprehensive type checking verifying assignment compatibility, parameter passing, return statements, and null safety.
- Diagnostic engine generating human-friendly Rust-style diagnostics with ANSI color highlights and caret indicators.

### 2.4 Intermediate Representation & Codegen (`sp::ir`, `sp::codegen`)
- High-level SP IR allows language-level optimizations (pattern match lowering, devirtualization, inlining) prior to LLVM code generation.
- LLVM Codegen lowers SP IR into LLVM IR using the LLVM 17+ C++ API.
- Native code generation delegates to LLVM's optimization passes (`-O1`, `-O2`, `-O3`) and target machine emitters.

---

## 3. Diagnostics & Error Handling

All compiler errors are routed through `sp::diagnostics::DiagnosticEngine`:
- Error codes structured under `E1000`-`E1999`.
- Diagnostics include exact source file, line number, column range, code snippet with carets, and contextual help hints.
- No compiler phase terminates execution via `exit()` or `abort()`; errors are accumulated so developers see all syntax/type issues in a single build pass.
