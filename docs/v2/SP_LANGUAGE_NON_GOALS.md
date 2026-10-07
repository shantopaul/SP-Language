# SP Language — Explicit Non-Goals

**Document ID:** SP-NONGOALS-001  
**Status:** Approved Architectural Standard  

---

## What SP Will Explicitly NOT Become

Setting clear boundaries is essential for the long-term health and focus of SP. The following items are formally designated as **non-goals**:

### 1. NOT a Replacement for C in OS Kernels & Bootloaders
- SP relies on runtime initialization (allocator, I/O handles, thread initialization).
- We do not target ring-0 kernel drivers, raw bootloaders, or hardware interrupt handlers.
- **Focus:** User-space systems programming, cloud services, and developer tooling.

### 2. NOT a Borrow-Checker Clone
- We explicitly reject Rust-style lifetime syntax (`&'a str`, lifetime bounds, complex borrowing proofs).
- While lifetimes provide impressive safety, they impose an immense cognitive burden and slow team development velocity.
- **Focus:** Value types by default, deterministic scope cleanup, and compiler-automated reference counting (ARC) for shared state.

### 3. NOT a Virtual Machine / Bytecode Language
- SP will not implement a bytecode interpreter, JIT engine, or JVM-style runtime environment.
- **Focus:** Strict ahead-of-time (AOT) compilation directly to native machine code via LLVM.

### 4. NOT a Framework-Heavy Monolith
- We will not bundle large enterprise web frameworks or GUI kits inside the standard library.
- High-level domains (web servers, ORMs, GUI toolkits) belong in community packages published through `spm`.
- **Focus:** A lean, robust, highly portable standard library (`std.io`, `std.fs`, `std.net`, `std.os`, `std.time`, `std.math`, `std.collections`, `std.json`).

### 5. NOT a Language with Unreadable Metaprogramming
- No C++ style template SFINAE complexity or unreadable macros.
- Metaprogramming, when introduced, will strictly follow deterministic compile-time function evaluation (`comptime`) with standard SP syntax.
