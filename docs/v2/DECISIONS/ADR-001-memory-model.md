# ADR-001: Memory Management Architecture

## Status
Proposed (Requires Shanto Review & Approval)

## Context
In SP 1.0, memory is allocated via `malloc`/`calloc` in `Runtime.cpp` without automated deallocation. This creates unbounded memory leaks in long-running applications. SP requires a definitive, deterministic memory model that maintains high execution speed, avoids GC pauses, and provides high developer ergonomics.

## Options Considered
1. **Manual / Arena Allocators (Zig style):** High performance, but error-prone and steep learning curve.
2. **Tracing Garbage Collection (Go / Java style):** Easy to use, but introduces pause latency, 2x–3x heap bloat, and complicates C FFI.
3. **Borrow Checking with Lifetimes (Rust style):** Safe and zero-cost, but excessive cognitive overhead and syntax complexity.
4. **Hybrid Value Semantics + Automated Reference Counting (ARC) (Swift / Nim style):** Value structs on stack/buffers (zero overhead); classes use compiler-managed ARC with escape analysis.

## Decision
**Adopt Option 4 (Hybrid Value Semantics + ARC).**
- Value types (`struct`, primitives, arrays, strings) use deterministic scope-based value semantics.
- Reference types (`class`) utilize compiler-managed reference counting with weak references to break cycles.
- Temporary / scratch scopes support memory arenas (`std.mem.Arena`).

## Consequences
- **Positive:** Deterministic, pause-free latency; zero cognitive burden of lifetime annotations; seamless C FFI; low memory footprint.
- **Negative:** Requires implementing an EscapeAnalysis pass in the compiler to optimize retain/release pairs.
