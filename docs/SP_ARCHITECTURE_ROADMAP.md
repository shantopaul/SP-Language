# SP Language — Architecture Roadmap (SP 1.0 to SP 2.0)

**Document ID:** SP-ROADMAP-001  
**Status:** Multi-Year Master Plan  

---

## 1. Roadmap Architecture Diagram

```
[ Phase 1: SP 1.1 Foundation ]
  ├── Deterministic ARC & Value Semantics Memory Model
  ├── Direct LLVM C-API & In-Process Linker (LLD)
  └── Pure SP Self-Hosted Generic Collections (List, Map, Set)
           │
           ▼
[ Phase 2: SP 1.2 – 1.4 Systems & I/O ]
  ├── Native Non-Blocking I/O (`std.net`)
  ├── Worker Threads, Channels & Lock-Free Primitives
  ├── Algebraic Result<T, E> & Option<T> with `?` Operator
  └── C-ABI FFI Tools (`spc bindgen`, #[repr(C)])
           │
           ▼
[ Phase 3: SP 1.5 – 1.7 Cloud & Ecosystem ]
  ├── Native JSON & Serialization Engine
  ├── HTTP/1.1 & HTTP/2 Server Framework
  ├── Production Package Registry GA with Cryptographic Signing
  └── Built-in Micro-Benchmark Harness (`spc bench`)
           │
           ▼
[ Phase 4: SP 2.0 Evolution ]
  ├── Compile-Time Execution (`comptime`)
  ├── Self-Hosted Compiler Frontend
  ├── Language Edition Mechanism (Edition 2026, Edition 2028)
  └── WebAssembly / WASI Official Compilation Target
```

---

## 2. Milestone Release Schedule & Scope

| Release | Primary Focus | Key Deliverables | Expected Impact |
|---|---|---|---|
| **SP 1.0** (Current) | Production General Availability | Core compiler, parser, AST, IR, Clang backend, 8 stdlib modules, `spc`, `spm`, `spls`, `spd`, websites. | Working compiler & toolchain baseline. |
| **SP 1.1** | Leak-Free Memory & LLVM | Deterministic ARC for `class`, RAII drops in `IRBuilder`, direct LLVM C-API integration. | Production-grade memory stability, 3x faster compilation. |
| **SP 1.2** | Native Generics & Collections | Pure SP implementations of `List<T>`, `Map<K, V>`, `Set<T>`, `Stack<T>`, `Queue<T>`. | Zero C++ STL dependency in runtime, unboxed memory speed. |
| **SP 1.3** | Error Safety & FFI | `Result<T, E>`, `Option<T>`, `?` operator, `#[repr(C)]`, raw pointers. | Modern zero-cost error propagation and seamless C libraries. |
| **SP 1.4** | High-Performance Concurrency | Cross-platform thread pool, `Channel<T>`, atomic types, lock-free queues. | High-throughput concurrent backend processing. |
| **SP 1.5** | Networking & Protocols | `std.net`, streaming JSON, HTTP client and server engine. | Production-ready microservices and APIs. |
| **SP 1.6** | Tooling & Registry GA | Package registry signing, LSP incremental workspace index, VS Code 1.0 extension. | Thriving developer package ecosystem. |
| **SP 2.0** | Enterprise Mature Platform | `comptime`, self-hosting, language editions, WebAssembly target. | Long-term industry standard language. |
