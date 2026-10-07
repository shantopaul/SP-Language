# ADR-003: Concurrency & Async Architecture

## Status
Proposed (Requires Shanto Review & Approval)

## Context
High-scale backend applications demand high-throughput concurrent I/O. SP currently supports OS threads via C wrappers, which do not scale to tens of thousands of idle network connections.

## Options Considered
1. **OS Threads + Mutexes only:** Excessive memory overhead per connection (1–2 MB stack).
2. **M:N Green Threads / Goroutines:** Heavy runtime scheduler, segmented stacks, complex C FFI.
3. **Structured Async/Await with Non-Blocking Event Loop (Rust / Swift style):** Tasks compile to state machines; non-blocking I/O driven by OS event demultiplexers (epoll/IOCP/kqueue); thread pool for CPU tasks.

## Decision
**Adopt Option 3 (Structured Async/Await with Non-Blocking Event Loop).**
- Foundation phase: Worker thread pools and message-passing channels (`Channel<T>`).
- Evolution phase: `async fn` state machine lowering with zero-allocation tasks.

## Consequences
- **Positive:** Massive I/O connection scalability; negligible memory overhead per idle task; uncompromised C FFI.
- **Negative:** Requires state machine transformation in compiler IR.
