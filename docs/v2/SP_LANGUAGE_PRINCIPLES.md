# SP Language — Core Principles

**Document ID:** SP-PRIN-001  
**Status:** Approved Architectural Standard  

---

## The Seven Pillars of SP Design

### 1. Correctness Before Convenience
A program that compiles cleanly should behave as intended. We reject silent type coercions, unhandled null values, and implicit memory mutations that create subtle bugs.

### 2. Native Speed Without Complexity
High performance should not require a PhD in compiler mechanics. A developer writing straightforward, readable SP code should achieve performance comparable to idiomatic C++ and Rust automatically.

### 3. Predictable Resource Management
Memory and system resources should be released deterministically when their scope terminates. Developers must know exactly when an allocation occurs, where it lives, and when it is destroyed.

### 4. Zero-Friction Developer Experience
Getting from `spm init` to a running, tested, formatted, and debugged binary should take seconds. Error messages must point directly to the offending line, highlight the root cause with color and context, and suggest the exact fix.

### 5. Explicit Semantics Over Hidden Magic
No invisible background thread schedulers stealing CPU, no implicit reflection lookups that degrade performance, and no complex monkey-patching.

### 6. Universal Interoperability (FFI)
SP recognizes that trillions of lines of battle-tested C code exist. Calling a C function or linking a native dynamic library (`.dll`, `.so`, `.dylib`) should require zero glue code or manual wrapping.

### 7. Sustainable Project Evolution
Language features are permanent commitments. We add syntax only when a clear, widely experienced problem cannot be solved elegantly in the standard library.
