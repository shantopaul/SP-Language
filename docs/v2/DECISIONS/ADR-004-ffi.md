# ADR-004: Foreign Function Interface (FFI) & C Interoperability

## Status
Accepted (Approved by Shanto Paul, 2026-11-10)

## Context
A new programming language cannot thrive in isolation without access to mature native C libraries (OpenSSL, SQLite, zlib, raylib, OS syscalls).

## Options Considered
1. **Hand-Coded C Wrappers:** Writing C++ glue code for every library. Extremely slow and labor-intensive.
2. **Direct C-ABI Declarations with `extern "C"` & Automated Header Generation (`spc bindgen`):** Frictionless native calling with zero glue code.

## Decision
**Adopt Option 2 (Direct C-ABI + `spc bindgen`).**
- First-class `extern "C"` syntax.
- `#[repr(C)]` attribute for deterministic struct memory layout.
- Raw pointer support (`ptr<T>`) within `unsafe` blocks.
- `spc bindgen` tool to parse C header files via libclang.

## Consequences
- **Positive:** Immediate access to the entire C ecosystem; zero overhead function calls; trivial native library integration in `sp.toml`.
- **Negative:** Requires careful attention to pointer ownership across FFI boundaries.
