# ADR-002: Error Handling Model

## Status
Proposed (Requires Shanto Review & Approval)

## Context
SP currently parses `try`/`catch`/`throw` statements but lacks native stack unwinding tables. Runtime assertion failures call `sp_rt_panic()`, aborting the program. A production language requires a clear distinction between recoverable operational errors and unrecoverable bugs.

## Options Considered
1. **Full Exception Stack Unwinding:** Heavy binary size overhead, non-local control flow, hidden failure modes.
2. **Go-Style Tuple Returns (`val, err`):** Repetitive error-handling boilerplate on every call.
3. **Algebraic Result<T, E> & Option<T> with Syntactic `?` Operator (Rust / Swift style):** Explicit, zero-cost, type-safe, concise.

## Decision
**Adopt Option 3 (Result<T, E> and Option<T> with `?` propagation).**
- Recoverable errors return `Result<T, E>`.
- The `?` operator provides ergonomic early returns.
- Unrecoverable logic errors continue to use `panic()`.

## Consequences
- **Positive:** Zero unwinding overhead; complete compile-time type safety; clean, readable error handling.
- **Negative:** Requires compiler support for tagged union enums with payloads.
