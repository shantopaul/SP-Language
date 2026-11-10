# SP 2.0 Language Specification & Evolution Manual

**Document Version:** 2.0.0-GA  
**Target Architecture:** Native (x86_64, aarch64), WebAssembly (`wasm32-wasi`)  
**Status:** Official Language Specification  

---

## 1. Executive Summary & Vision

SP 2.0 represents the premier generational leap of the SP programming language. Building upon the deterministic memory safety, ARC zero-cost abstractions, thread pools, and non-blocking I/O foundations delivered in SP 1.0–1.7, SP 2.0 introduces:
1. **The Language Edition Mechanism** (`Edition 2026` and `Edition 2028`), ensuring timeless backwards compatibility while enabling rapid syntactic and semantic evolution.
2. **Compile-Time Function Execution (`comptime`)**, allowing arbitrary deterministic logic to evaluate during compilation, folding constant expressions and validating constraints ahead of code generation.
3. **First-Class WebAssembly (`wasm32-wasi`) Target**, compiling high-performance SP programs to standard WASI binaries for cloud, edge, and browser sandboxes.
4. **Structured Asynchronous Coroutines (`async`/`await`)**, elevating asynchronous programming to first-class language keywords with seamless event loop interoperability.
5. **A Production Self-Hosted Compiler Frontend (`compiler/self_hosted/`)**, authored entirely in SP, demonstrating language maturity and self-sufficiency.
6. **Automated Migration Tooling (`spc fix`)**, modernizing legacy codebases across editions without manual refactoring friction.

---

## 2. Language Editions Architecture

To prevent language stagnation while guaranteeing that code written today compiles unchanged a decade from now, SP 2.0 establishes the **Edition Model**.

### 2.1 Edition Declaration in `sp.toml`
Every SP package declares its edition in the root manifest:
```toml
[package]
name = "my_service"
version = "2.0.0"
edition = "2028" # Or "2026"
```

### 2.2 CLI Overrides
The compiler accepts an explicit edition parameter:
```bash
spc build --edition 2028 src/main.sp
spc check --edition 2026 src/main.sp
```
If unspecified, `spc` automatically detects the edition from the nearest `sp.toml`. If no manifest exists, it defaults to `2026` for maximum backwards compatibility.

### 2.3 Edition Capability Matrix

| Feature | Edition 2026 (SP 1.x Legacy) | Edition 2028 (SP 2.0 Modern) |
|---|---|---|
| ARC & Deterministic Cleanup | Enabled | Enabled |
| Standard Library Modules | Supported | Supported |
| Non-Blocking Networking & Async | Thread Pool & Future/Task | Native `async`/`await` Coroutines |
| Compile-Time Evaluation | Experimental Warning | Native & Enforced |
| WebAssembly (`wasm32-wasi`) | Supported | First-Class |
| Strict Identifier Scoping | Permissive | Enforced |

### 2.4 Automated Migration Tool (`spc fix`)
The `spc fix` subcommand inspects source files and updates syntax to the target edition:
```bash
# Preview modifications
spc fix --dry-run --edition 2028 src/

# Apply automated migrations in-place
spc fix --edition 2028 src/
```
The migration tool:
- Converts deprecated function signatures to current forms.
- Replaces legacy promise chains with `async`/`await` coroutines.
- Updates package manifests (`sp.toml`) to `edition = "2028"`.

---

## 3. Compile-Time Function Execution (`comptime`)

SP 2.0 introduces the `comptime` keyword for compile-time computation.

### 3.1 Comptime Expressions & Blocks
Any deterministic expression can be executed during semantic analysis:
```sp
const BUFFER_SIZE: int = comptime (1024 * 64);
```
> Phase 4 slice D (2026-11-09): the old `comptime { ... }` block example
> below does **not** compile -- a statement block with `for` / `push` /
> `return` is not a supported comptime form (verified with `spc check`:
> the declaration fails with `E1002`/`E1001`). What works today:
> comptime expressions (literals, arithmetic/logic, string concat,
> ternary, arrays + indexing, `.length`) and `comptime fn` calls,
> including recursion (see §3.2, verified: `comptime fibonacci(10)` folds
> to 55). Runtime-dependent comptime fails loudly (`E1000`/`E1806`/
> `E1807`).
>
> ```sp
> const LOOKUP_TABLE: int[] = comptime {
>     let tbl: int[] = [];
>     for (let i = 0; i < 256; i = i + 1) {
>         tbl.push(i * i);
>     }
>     return tbl;
> };
> ```

### 3.2 Comptime Functions
Functions marked `comptime` are guaranteed to evaluate at compile time when called with constant arguments:
```sp
comptime fn fibonacci(int n) -> int {
    if (n <= 1) return n;
    return fibonacci(n - 1) + fibonacci(n - 2);
}

fn main() {
    // Computed during compilation into an immediate constant 55
    int fib10 = comptime fibonacci(10);
    println("Fibonacci(10) = ${fib10}");
}
```

### 3.3 Compile-Time Assertions
The builtin `assert(cond, message)` is evaluated when it appears inside
a `comptime fn` (verified with `spc run`: `comptime checkedSize(65536)`
with `assert(n >= 4096, ...)` inside the function folds correctly).
If an assertion fails, compilation halts with a precise source
diagnostic before LLVM IR generation begins.
> Phase 4 slice D (2026-11-09): the old standalone `comptime { assert(...); }`
> block form below does **not** parse (`E1000: Expected ';' after
> expression`, verified with `spc check`). Put the assertion inside a
> `comptime fn` instead:
> ```sp
> comptime fn checkedSize(int n) -> int {
>     assert(n >= 4096, "Buffer size must exceed 4KB");
>     return n;
> }
> ```

---

## 4. WebAssembly Target (`wasm32-wasi`)

SP 2.0 natively targets WebAssembly System Interface (WASI).

### 4.1 Compiling to WebAssembly
```bash
spc build --target wasm32-wasi -o app.wasm src/main.sp
```
### 4.2 Data Layout & ABI
- Architecture: 32-bit linear memory (`wasm32`).
- Target Triple: `wasm32-unknown-wasi`.
- Data Layout: `e-m:e-p:32:32-p10:8:8-p20:8:8-i64:64-n32:64-S128-ni:1:10:20`.
- System Calls: Standard WASI preview 1 syscall imports (`fd_write`, `fd_read`, `proc_exit`, `clock_time_get`).

### 4.3 Execution with WASI Runtimes
Binaries produced by `spc` run directly in any WASI-compliant engine:
```bash
wasmtime app.wasm
wasmer run app.wasm
node --experimental-wasi-unstable-preview1 app.wasm
```

---

## 5. Structured `async`/`await` Coroutines

SP 2.0 natively introduces asynchronous coroutines. They run on the
event-loop pump: awaiting yields to the loop, but the awaiting frame
stays on the stack (no heap-frame state machine yet), so this is not
zero-overhead.

### 5.1 Async Functions
Functions declared with `async fn` return a managed `Task<T>`. There is
no async I/O yet: `await` works only on async functions and on the
builtins `sleep_async(ms)` / `delayed_value(ms, value)` (Phase 4 slice D,
verified with `spc check` -- the old `await client.get(url)` example
does not compile):
```sp
async fn fetchRemoteData(url: string) -> string {
    await sleep_async(10);
    return "data for " + url;
}
```

### 5.2 Await Expressions
Inside an async function context, `await <expr>` yields execution to the SP event loop until the operand task resolves:
```sp
// The fetchRemoteData function from §5.1:
async fn fetchRemoteData(string url) -> string {
    await sleep_async(10);
    return "data for " + url;
}

async fn processBatch(string[] urls) {
    for (string url in urls) {
        string content = await fetchRemoteData(url);
        println("Fetched: " + url + " (${content.length()} bytes)");
    }
}
```

### 5.3 Typechecker Constraints
- Calling `await` outside an `async fn` context results in compiler error `E1802` (Phase 4 slice D: verified with `spc check`; the spec previously said `E1801`).
- The operand of `await` must resolve to `Task<T>` or a compatible future type; the expression produces `T`.

---

## 6. Self-Hosted SP Compiler Frontend

Under `compiler/self_hosted/`, SP 2.0 implements its complete compilation frontend written purely in SP:
- `tokens.sp`: Lexical tokens and categories.
- `lexer.sp`: Scanner supporting string literals, comments, keywords, multi-character operators, and source locations.
- `ast.sp`: Strongly-typed AST nodes with object-oriented hierarchy.
- `parser.sp`: Recursive-descent parser with precedence climbing for operators and statement blocks.
- `sema.sp`: Symbol table scopes and semantic validation.
- `main.sp`: Compiler frontend driver pipeline.

---

## 7. Backwards Compatibility Guarantees

1. **Edition 2026 Stability:** Existing SP 1.0–1.7 code continues to compile without modification under default settings.
2. **Cross-Edition Dependency Linking:** Packages compiled targeting Edition 2026 can be imported and consumed seamlessly by Edition 2028 projects.
3. **No Hidden Runtime Tax:** Comptime logic executes entirely in the compiler process; zero runtime overhead is introduced.
