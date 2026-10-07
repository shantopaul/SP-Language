# SP Language — Compiler Subsystem Roadmap

**Document ID:** SP-COMP-001  
**Target:** `compiler/` Directory & Core Engine  

---

## 1. Current State vs. Target Architecture

```
Current Architecture:
  AST ──► IRBuilder ──► IRModule ──► IRPrinter (text .ll) ──► Clang (shell) ──► Binary

Target Architecture (SP 1.2+):
  AST ──► IRBuilder ──► LLVM C-API ──► LLVM Passes (-O3) ──► LLD (in-process) ──► Binary
```

---

## 2. Key Compiler Work Items

### 2.1 In-Process LLVM C-API Integration
- **Goal:** Eliminate disk I/O of temporary `.ll` files and shell-out overhead to `clang++`.
- **Implementation:**
  - Link `LLVM-C` / `llvm::TargetMachine` directly in `sp_compiler`.
  - Use `llvm::IRBuilder<>` or lower SP IR directly into `llvm::Module`.
  - Generate object files (`.o` / `.obj`) directly in memory.
  - Call embedded LLD (`lld::elf`, `lld::coff`, `lld::macho`) for zero-overhead native linking.

### 2.2 Memory Management Pass (ARC Insertion)
- **Goal:** Automated memory safety for reference types.
- **Implementation:**
  - Add `sp::sema::EscapeAnalysis` pass to detect whether a class instance escapes the current function.
  - In `IRBuilder`, emit `sp_rt_retain` on assignment and `sp_rt_release` at scope termination.
  - Optimize: cancel adjacent retain/release pairs.

### 2.3 Algebraic Data Types & Pattern Matching Lowering
- **Goal:** First-class tagged unions for `Result<T, E>` and `Option<T>`.
- **Implementation:**
  - Support enum variants with payloads: `enum Result<T, E> { Ok(T), Err(E) }`.
  - Lower payload enums to tagged structs: `{ i32 tag, union { T, E } data }`.
  - Lower `match` expressions into jump tables with destructuring binding.

### 2.4 DWARF & PDB Native Debug Information
- **Goal:** Enable step-by-step debugging in VS Code / LLDB / Visual Studio with exact SP source variables.
- **Implementation:**
  - Emit `llvm::DIBuilder` metadata for functions, variables, lines, and types.
