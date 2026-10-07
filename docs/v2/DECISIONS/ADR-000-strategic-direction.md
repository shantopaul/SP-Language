# ADR-000: Selection of Strategic Direction — Direction B

## Status
**APPROVED**  
**Decision Maker:** Shanto Paul (Lead Architect & Maintainer)  
**Date of Ratification:** October 7, 2026  
**Implementation Horizon:** SP 1.1 through SP 2.0  

---

## Context
Following the full codebase analysis of the SP Language repository (compiler pipeline, AST, SSA IR, VTable generator, monomorphized generics, runtime, and toolchain), three potential long-term strategic directions were evaluated:
1. **Direction A — Hardened Systems Language:** Manual memory management, bare-metal kernels, embedded devices (Zig / C style).
2. **Direction B — High-Performance Native Application & Cloud Language:** Cloud microservices, CLI developer tools, networking, data engines, deterministic zero-GC latency, hybrid value semantics + compiler ARC (Go / Swift / modern Java style).
3. **Direction C — Broad General-Purpose Language:** Full-stack, desktop GUI, mobile, web, and scripting (Kotlin / Dart style).

---

## Decision
**Formally Adopt Direction B: High-Performance Native Application & Cloud Language.**

SP will not attempt to be a bare-metal kernel language (Direction A) nor an overly broad cross-platform GUI/mobile generalist (Direction C). Instead, SP will dominate the **high-performance native backend, cloud infrastructure, and developer tooling space**.

---

## Core Strategic Pillars of Direction B

### 1. Primary Target Domains
* **Backend Cloud Services & REST/gRPC Microservices:** Ultra-fast startup (< 3ms), low baseline memory (< 10MB RAM), sub-millisecond p99 response times.
* **CLI Developer Tools & Background Daemons:** Standalone, zero-dependency native executables with instant execution.
* **Networking & Data Engines:** In-memory caches, custom protocol parsers, stream processors, and embedded database layers.

### 2. Primary Competitors & SP’s Competitive Advantage
* **vs. Go:** SP eliminates stop-the-world garbage collection pauses and memory bloat, while providing true zero-cost monomorphized generics and expressive pattern matching.
* **vs. Rust:** SP eliminates the immense cognitive overhead of lifetime annotations (`'a`) and borrow-checker friction, offering high development velocity with native LLVM performance.
* **vs. Java & C#:** SP compiles ahead-of-time (AOT) to native machine code with zero JVM/CLR runtime footprint, instant container cold starts, and tiny Docker images (< 15MB).

### 3. Core Technical Pillars
* **Deterministic Sub-Millisecond Latency:** Zero stop-the-world GC pauses. Memory is reclaimed deterministically as scopes terminate.
* **Hybrid Value Semantics + Compiler ARC:**
  * Value types (`struct`, numbers, strings, arrays) reside directly on the stack or in contiguous buffers with zero overhead.
  * Reference types (`class`) utilize compiler-automated reference counting with escape analysis optimization.
* **Modern Error Handling:** `Result<T, E>` and `Option<T>` with the syntactic `?` operator for zero-cost, crash-free error propagation.
* **Pure SP Standard Collections:** `List<T>`, `Map<K, V>`, and `Set<T>` self-hosted in pure SP generics with direct heap buffer indexing.
* **Direct LLVM C-API Integration:** Native machine code generation directly in memory with embedded LLD linking, eliminating external Clang dependencies.

---

## Consequences

### Positive
* **Defensible Market Position:** Capitalizes on a massive unmet industry need: developers wanting native speed and zero GC pauses without Rust's extreme complexity.
* **100% Codebase Alignment:** Directly builds upon and refines the existing C++20 compiler, Pratt parser, VTable generator, and monomorphized generics.
* **Sustainable Maintenance:** Keeps the language core, standard library, and toolchain focused on a unified, high-value domain.

### Negative & Mitigations
* **Not Suitable for OS Kernels:** Accepted as an explicit non-goal (defined in `docs/v2/SP_LANGUAGE_NON_GOALS.md`).
* **Requires Compiler ARC Pass:** Implementation scheduled for milestone SP 1.1 with dedicated escape analysis.
