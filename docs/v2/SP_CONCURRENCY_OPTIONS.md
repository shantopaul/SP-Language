# SP Language — Concurrency Architecture & Options

**Document ID:** SP-CONC-001  
**Status:** Architectural Decision Proposal  

---

## 1. Problem Statement: Current State Audit

In the current SP codebase:
- Concurrency features in examples (`std.thread`, channels, worker pools) are demonstrated through native wrapper functions calling OS threads or simulated abstractions.
- There is no formal language memory model (e.g. acquire/release semantics), no compiler-enforced data race detection, and no native event loop.
- To become a top-tier systems and backend language, SP requires a modern, safe, scalable concurrency architecture.

---

## 2. Evaluation of Concurrency Models

### Model 1: OS Threads + Synchronization Primitives (C++ / Java style)
- **Mechanism:** Direct OS 1:1 threads (`std::thread`), mutexes, condition variables, atomic types.
- **Pros:** Simple runtime implementation; maximum hardware utilization for CPU-bound tasks.
- **Cons:** High memory cost per thread (1–2 MB stack); poor scalability for 100,000+ concurrent network connections; prone to deadlocks and data races.

### Model 2: M:N Green Threads / Goroutines (Go style)
- **Mechanism:** User-space scheduler multiplexing M lightweight tasks over N OS threads with growable stacks.
- **Pros:** Seamless synchronous programming style; millions of concurrent tasks possible.
- **Cons:** Complex runtime scheduler; requires split/segmented stacks or yield checkpoints; heavy runtime footprint; complicates C FFI.

### Model 3: Async/Await with Structured Concurrency & Event Loop (Rust / Swift style) (Recommended)
- **Mechanism:**
  - Functions marked `async fn` compile into state machines.
  - Runtime provides a work-stealing thread pool and non-blocking I/O driver (epoll on Linux, IOCP on Windows, kqueue on macOS).
  - Channels (`Channel<T>`) provide safe message-passing between concurrent tasks.
  - **Structured Concurrency:** Child tasks cannot outlive their parent scope, eliminating dangling async background tasks.
- **Pros:**
  - Zero stack allocation overhead for idle tasks (state machine struct only).
  - C FFI remains native and uncompromised.
  - Predictable, explicit execution flow without hidden scheduler preemption.
- **Cons:**
  - "Function color" problem (`async` vs synchronous functions); requires state machine transformation in `IRBuilder`.

---

## 3. Concurrency Recommendation & Staged Implementation

> **Recommendation:** Adopt a two-phase evolution:
> 1. **SP 1.2 – 1.4:** Hardened **OS Worker Threads + Lock-Free Channels (`Channel<T>`) + Atomics (`Atomic<T>`)**.
> 2. **SP 1.6 – 2.0:** First-class **`async`/`await` with Non-Blocking I/O Event Loop** for massive network scalability.
