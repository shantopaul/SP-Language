# SP Language — Next-Generation (v2 / Future Direction) Architecture Suite

**Maintainer & Project Lead:** Shanto Paul  
**Status:** Approved Strategic Roadmap  
**Target Evolution:** SP 1.1 through SP 2.0  

This directory contains the complete architectural audit, strategic goal definitions, multi-year roadmap, and Architectural Decision Records (ADRs) for the SP Programming Language.

---

## 📑 Strategic Architecture Directory

### 1. Discovery & Analysis
- **[`SP_LANGUAGE_ANALYSIS.md`](SP_LANGUAGE_ANALYSIS.md):** Full codebase analysis, compiler pipeline audit, documentation vs. reality matrix, technical debt inventory, and subsystem assessment.

### 2. Goals & Identity
- **[`SP_LANGUAGE_GOALS.md`](SP_LANGUAGE_GOALS.md):** The primary strategic document. Defines what SP is, target developers, core differentiators, problem statements, and 1.x / 2.0 milestones.
- **[`SP_LANGUAGE_VISION.md`](SP_LANGUAGE_VISION.md):** 5-year vision, core technical philosophies (clarity, zero-cost, predictable latency), and platform support tiers.
- **[`SP_LANGUAGE_PRINCIPLES.md`](SP_LANGUAGE_PRINCIPLES.md):** The seven architectural pillars guiding every design decision.
- **[`SP_LANGUAGE_NON_GOALS.md`](SP_LANGUAGE_NON_GOALS.md):** Explicit non-goals preventing feature bloat and keeping the language focused and maintainable.

### 3. Core Architecture & Subsystem Options
- **[`SP_MEMORY_MODEL_OPTIONS.md`](SP_MEMORY_MODEL_OPTIONS.md):** Exhaustive investigation and trade-off comparison of 5 memory models (Manual/Arenas, Tracing GC, Borrow Checking, Pure ARC, and Hybrid Value Semantics + ARC).
- **[`SP_CONCURRENCY_OPTIONS.md`](SP_CONCURRENCY_OPTIONS.md):** Comparison of OS threads, green threads (goroutines), and structured `async`/`await` with non-blocking I/O event loop.
- **[`SP_ERROR_HANDLING_OPTIONS.md`](SP_ERROR_HANDLING_OPTIONS.md):** Comparison of stack unwinding exceptions vs. algebraic `Result<T, E>` / `Option<T>` with `?` operator.
- **[`SP_FFI_DESIGN.md`](SP_FFI_DESIGN.md):** Zero-overhead C-ABI interoperability, `#[repr(C)]`, raw pointers in `unsafe`, and automated header binding generator (`spc bindgen`).

### 4. Implementation Roadmaps
- **[`SP_ARCHITECTURE_ROADMAP.md`](SP_ARCHITECTURE_ROADMAP.md):** Master phased progression from SP 1.1 to SP 2.0.
- **[`SP_COMPILER_ROADMAP.md`](SP_COMPILER_ROADMAP.md):** In-process LLVM C-API integration, embedded LLD linker, DWARF/PDB debug symbols, and escape analysis.
- **[`SP_RUNTIME_ROADMAP.md`](SP_RUNTIME_ROADMAP.md):** High-performance allocator (**mimalloc**), leak-free SSO string engine, and cross-platform event loop.
- **[`SP_STDLIB_ROADMAP.md`](SP_STDLIB_ROADMAP.md):** Migration to pure SP self-hosted generic collections (`List`, `Map`, `Set`), `std.sync`, `std.net`, and `std.json`.
- **[`SP_TOOLING_ROADMAP.md`](SP_TOOLING_ROADMAP.md):** Toolchain hardening for `spc`, `spm`, `spls` (LSP), and `spd` (DAP).
- **[`SP_ECOSYSTEM_ROADMAP.md`](SP_ECOSYSTEM_ROADMAP.md):** Package Registry (`registry.splang.shantopaul.com`), supply chain integrity, and cryptographic `sp.lock` signing.
- **[`SP_IMPLEMENTATION_CHECKLIST.md`](SP_IMPLEMENTATION_CHECKLIST.md):** Actionable master engineering checklist with checkboxes across all subsystems.

---

## 🏛️ Architectural Decision Records (ADRs)

Located in [`DECISIONS/`](DECISIONS/):
- **[`ADR-000: Selection of Strategic Direction — Direction B`](DECISIONS/ADR-000-strategic-direction.md):** **[APPROVED]** Formal selection of **Direction B: High-Performance Native Application & Cloud Language** (Backend, Cloud, Microservices, CLI, Data Engines).
- **[`ADR-001: Memory Management Architecture`](DECISIONS/ADR-001-memory-model.md):** Hybrid Value Semantics by default + Compiler-Automated Reference Counting (ARC).
- **[`ADR-002: Error Handling Model`](DECISIONS/ADR-002-error-handling.md):** `Result<T, E>` & `Option<T>` with syntactic `?` operator.
- **[`ADR-003: Concurrency & Async Architecture`](DECISIONS/ADR-003-concurrency.md):** Structured `async`/`await` with Non-Blocking Event Loop (epoll/IOCP/kqueue).
- **[`ADR-004: Foreign Function Interface (FFI) & C Interoperability`](DECISIONS/ADR-004-ffi.md):** Direct C-ABI declarations, `#[repr(C)]`, and `spc bindgen`.
- **[`ADR-005: Package Registry & Supply Chain Security`](DECISIONS/ADR-005-package-system.md):** Immutable content-addressed package registry with cryptographic lockfile validation.
