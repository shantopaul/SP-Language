# SP Language — Memory Model Investigation & Strategic Options

**Document ID:** SP-MEM-001  
**Status:** Architectural Decision Proposal  
**Category:** Core Language & Runtime Architecture  

---

## 1. Problem Statement: Current State Audit

In the current SP 1.0 implementation:
- Allocations for objects (`new Class()`), strings (`sp_rt_str_*`), and collections (`sp_rt_list_*`, `sp_rt_map_*`) occur via `std::calloc` and `malloc` in `runtime/src/Runtime.cpp`.
- There is **no automated deallocation**: neither garbage collection, reference counting, nor compiler-inserted `drop` destructors.
- Unless manually freed (which is error-prone and violates modern ergonomics), every string manipulation and collection allocation permanently leaks memory for the life of the process.

**This is the single most urgent architectural foundation to resolve.**

---

## 2. Evaluation of Memory Model Alternatives

We evaluate five distinct architectural approaches:

### Option A: Manual Memory Management with Arenas & Defer (Zig / C style)
- **Mechanism:** Explicit allocators passed to functions, `defer allocator.free(ptr)`, and regional arena allocators for batch lifetimes.
- **Pros:**
  - Zero runtime overhead; total memory control; extremely predictable.
- **Cons:**
  - High developer fatigue; use-after-free and double-free bugs remain possible; steep learning curve for developers coming from Go/TypeScript.

### Option B: Pure Tracing Garbage Collection (Go / Java / C# style)
- **Mechanism:** Background concurrent mark-and-sweep or generational garbage collector traversing heap roots.
- **Pros:**
  - High developer comfort; trivial memory management; no use-after-free.
- **Cons:**
  - Massive runtime complexity; non-deterministic pause latency (p99 latency spikes); 2x–3x heap memory overhead; awkward FFI with native C code.

### Option C: Strict Ownership & Borrow Checking with Lifetimes (Rust style)
- **Mechanism:** Compile-time affine type system tracking single-ownership, unique mutability (`&mut`), and lifetime annotations (`'a`).
- **Pros:**
  - Zero runtime overhead; guaranteed memory safety; deterministic drop.
- **Cons:**
  - Drastically increases cognitive burden; steep learning curve; slows development velocity; complex self-referential graph structures become difficult.

### Option D: Deterministic Automatic Reference Counting (ARC) (Swift / Nim style)
- **Mechanism:** Compiler automatically inserts atomic retain (`rc++`) and release (`rc--`) calls. When `rc == 0`, destructor executes immediately.
- **Pros:**
  - Deterministic cleanup at scope exit; zero stop-the-world pauses; excellent FFI interoperability; familiar and intuitive.
- **Cons:**
  - Reference count increment/decrement overhead on copies; potential reference cycle leaks if cyclic references exist (requires `weak` references).

### Option E: Hybrid Value Semantics by Default + ARC for Shared Classes (Recommended)
- **Mechanism:**
  1. **Value Types (`struct`, primitive types, arrays, strings):** Allocated directly on stack or in contiguous buffers with value semantics (copy on write or move). Zero reference counting overhead.
  2. **Reference Types (`class`):** Managed via compiler-automated reference counting (ARC) with compile-time escape analysis (eliding retain/release when a reference does not escape the local stack frame).
  3. **Weak References (`weak ClassName`):** Break cyclic reference graphs.
  4. **Arena Allocators (`std.mem.Arena`):** Provided for high-throughput batch workloads (request-scoped web handlers, parsers).
- **Pros:**
  - Exceptional speed: 80%+ of allocations are stack/value-based with zero overhead.
  - Predictable deterministic latency: objects destroyed the instant their last reference expires.
  - Seamless C FFI: value structs match C struct layouts exactly.
  - High developer ergonomics: no manual `free()` and no lifetime parameters.
- **Cons:**
  - Requires compiler escape analysis in `IRBuilder` to eliminate redundant retain/release pairs.

---

## 3. Comparative Evaluation Matrix

| Criteria | Option A (Manual/Arena) | Option B (Tracing GC) | Option C (Borrow Checker) | Option D (Pure ARC) | Option E (Hybrid Value + ARC) ⭐ |
|---|:---:|:---:|:---:|:---:|:---:|
| **Memory Safety** | Low | High | Very High | High | **High** |
| **Deterministic Latency** | Very High | Low | Very High | High | **Very High** |
| **Runtime Overhead** | None | High | None | Low | **Negligible** |
| **Developer Experience** | Medium | High | Low | High | **Very High** |
| **Compiler Complexity** | Low | Medium | Very High | Medium | **Medium-High** |
| **C FFI Compatibility** | Perfect | Poor | Complex | Good | **Excellent** |
| **Fit for SP Vision** | 3.5 / 5 | 2.5 / 5 | 3.0 / 5 | 4.2 / 5 | **4.9 / 5** |

---

## 4. Architectural Recommendation

> **Recommendation:** Adopt **Option E: Hybrid Value Semantics by Default + Compiler-Automated Reference Counting (ARC)**.
> 
> *Note: Final adoption requires formal review and approval by Shanto.*

### Implementation Blueprint for Option E:
1. **Object Header:** Every `class` heap allocation prepends an 8-byte header:
   ```c
   struct SPObjectHeader {
       uint32_t strong_count;
       uint32_t weak_count;
       const void* vtable_ptr;
   };
   ```
2. **Compiler Insertion (`IRBuilder`):**
   - At variable assignment or function argument pass: emit `sp_rt_retain(ptr)`.
   - At variable scope exit or reassignment: emit `sp_rt_release(ptr)`.
   - When `strong_count == 0`: invoke the class destructor, then release the memory buffer.
3. **Optimizations:**
   - Run a dead-reference elimination pass in IR to remove retain/release pairs where ownership does not cross function boundaries.
