# SP Language — Vision, Philosophy & Platform Strategy

**Document ID:** SP-VISION-001  
**Status:** Architecture Blueprint  
**Author:** Shanto Paul & Core Architecture Team  
**Scope:** 5-Year Language Strategy  

---

## 1. Long-Term Vision

Our vision is for **SP to become the default language for modern native software**: the first choice for engineers building cloud infrastructure, high-throughput microservices, developer tools, distributed nodes, and performance-critical applications.

In ten years, software systems should not be divided into an uneasy compromise between *slow, bloated managed runtimes* and *arcane, unsafe systems languages*. SP aims to prove that **native machine performance, absolute memory safety, and delightful developer ergonomics can coexist seamlessly in a single language.**

---

## 2. Technical Philosophies

### 2.1 The Philosophy of Clarity Over Cleverness
- Code is read vastly more often than it is written.
- SP favors explicit, unambiguous syntax over obscure shorthand or hidden magical behavior.
- Every function call, allocation, and flow transition should be comprehensible to a developer reading the code for the first time.

### 2.2 The Philosophy of Zero-Cost Execution
- What you do not use, you do not pay for.
- Generics monomorphize at compile time into specialized machine instructions.
- Structs and primitive values are laid out contiguously on the stack or in heap buffers without mandatory pointer indirection or boxing.
- When dynamic polymorphism is required, it uses explicit, efficient VTable dispatch.

### 2.3 The Philosophy of Predictable Latency
- High-scale services fail not because their average throughput is low, but because their 99th-percentile (p99) latency spikes unpredictably.
- By eliminating stop-the-world garbage collection pauses, SP provides deterministic, smooth response times under heavy concurrent load.

### 2.4 The Philosophy of Unified Tooling
- A programming language is only as good as its developer tooling.
- Tooling is not an afterthought; the compiler (`spc`), package manager (`spm`), language server (`spls`), debugger (`spd`), formatter, and test runner are developed as first-class citizens in the core repository.

---

## 3. Platform Strategy

### Tier 1 Platforms (Guaranteed Official Support, CI Verified)
- **Windows 10 / 11 (x86_64):** Full native MSVC / MinGW ABI compatibility, Windows API integration.
- **Linux (x86_64):** GNU/Linux and musl libc compatibility, systemd/container optimized.
- **macOS (Apple Silicon arm64 & Intel x86_64):** Full Darwin SDK and Mach-O binary emission.

### Tier 2 Platforms (Community Maintained, Core Functional)
- **Linux (aarch64 / ARM64):** AWS Graviton, Raspberry Pi 4/5, Oracle Ampere cloud nodes.
- **FreeBSD / OpenBSD (x86_64):** Native BSD kernel bindings.

### Tier 3 Platforms (Roadmap / Future)
- **WebAssembly (wasm32-wasi):** Standalone serverless functions and browser execution.
- **Embedded ARM Cortex-M:** Bare-metal microcontroller profiles.

---

## 4. Compatibility & Stability Philosophy

1. **Semantic Versioning 2.0.0:** SP strictly adheres to SemVer:
   - `MAJOR`: Incompatible language changes (reserved for SP 2.0).
   - `MINOR`: New backward-compatible language features, standard library additions, performance improvements.
   - `PATCH`: Bug fixes, security patches, compiler diagnostic enhancements.
2. **Language Edition Model (SP 2.0+):**
   - Similar to Rust editions, code targeting SP Edition 2026 will compile unmodified alongside dependencies written for future editions.
3. **No Silent Breaking Changes:**
   - Any deprecation will emit compiler warnings for at least two minor releases before deprecation removal.
