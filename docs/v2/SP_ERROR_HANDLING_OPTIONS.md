# SP Language — Error Handling Architecture & Options

**Document ID:** SP-ERR-001  
**Status:** Architectural Decision Proposal  

---

## 1. Problem Statement: Current State Audit

In the current SP codebase:
- `try`, `catch`, `finally`, and `throw` statements are parsed into AST nodes (`TryCatchStmt`, `ThrowStmt`).
- However, native stack unwinding (LLVM `landingpad`, DWARF personality routines, or Windows SEH) is not emitted.
- Runtime errors (such as out-of-bounds array access or null assertions) invoke `sp_rt_panic()`, which prints to stderr and calls `std::abort()`.
- Uncontrolled panics prevent recoverable errors (e.g. file not found, network timeout, invalid user input) from being handled cleanly in production servers.

---

## 2. Evaluation of Error Handling Models

### Option 1: Full Exception Stack Unwinding (`try` / `catch` / `throw`) (C++ / Java / C#)
- **Mechanism:** Throwing an object unrolls the stack frame by frame, executing cleanup landing pads until a matching `catch` block is found.
- **Pros:** Familiar to Java/C#/Python developers; separates error paths from business logic.
- **Cons:** Heavy runtime code size overhead (exception tables); non-local control flow; hidden failure modes (functions can throw without callers knowing); difficult optimization in LLVM.

### Option 2: Go-Style Multi-Return (`val, err = fn()`)
- **Mechanism:** Functions return error values as explicit tuple fields.
- **Pros:** Highly visible errors; zero unwinding overhead.
- **Cons:** Boilerplate code (`if err != null { return null, err }` repeated dozens of times); error checking can be easily ignored.

### Option 3: Result & Option Types with `?` Operator (Rust / Swift style) (Recommended)
- **Mechanism:**
  - Recoverable errors are represented as algebraic types:
    ```sp
    enum Result<T, E> {
        Ok(T),
        Err(E)
    }
    ```
  - The `?` operator provides ergonomic early-return propagation:
    ```sp
    let file = fs.open("config.json")?; // returns Err if failed
    ```
  - Unrecoverable bugs (logic bugs, invariant violations) use explicit `panic("fatal state")`.
- **Pros:**
  - Zero-cost: compiles to simple register branch checks without stack unwinding.
  - Complete type safety: callers cannot use `T` without handling the potential `Err`.
  - Highly readable and concise: the `?` operator removes 90% of error boilerplate.
- **Cons:**
  - Requires generic sum types / tagged unions in the compiler.

---

## 3. Recommendation

> **Recommendation:** Adopt **Option 3: `Result<T, E>` and `Option<T>` with the syntactic `?` propagation operator**.
> Retain `panic` strictly for unrecoverable contract violations. Deprecate unchecked `throw`/`catch` in favor of typed, deterministic error propagation.
