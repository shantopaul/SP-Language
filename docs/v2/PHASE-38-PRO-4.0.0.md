# Phase 38 (pro-4.0.0) — ADR-002 Result/Option error model

- **Branch:** `feature/pro-4.0.0` (branch only; remote `main` still `d10e1c2`)
- **Base:** `a17f702` (Phase 37, ADR-001 ARC completion)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, nothing pushed)
- **Status:** IN PROGRESS — investigation done, design below; slices to follow.
- **ADR:** `docs/v2/DECISIONS/ADR-002-error-handling.md` (Accepted 2026-11-10):
  recoverable errors return `Result<T, E>`, `?` propagates, panic
  only for unrecoverable logic errors. Consequence: "requires
  compiler support for tagged union enums with payloads".

## 0. Investigation — what exists, what is broken (verified 2026-11-10)

### 0.1 What already works (no work needed)

- `Result<T,E>` / `Option<T>` are builtin enum types (`Type.cpp:575,582`;
  special-cased in TypeChecker). `Result.Ok/Err`, `Option.Some/None`,
  `.isOk/.isErr/.unwrap/.unwrapErr/.unwrapOr/.isSome/.isNone`, plus
  bare `Ok(42)` / `Some(77)` — all covered by
  `ExecutionTest.ResultAndOptionZeroAllocTaggedUnionExecution`
  (int/string payloads).
- `?` is a postfix operator (`TryExpr`, Parser.cpp:500) with full
  lowering (`IRBuilder::visit(TryExpr)`, ~120 lines): tag != 0 →
  early return after `emitAllFunctionDrops()`; tag == 0 → unwrapped
  payload. Verified live: Option `?` inside an `Option`-returning
  fn (`doubleFind(5)` → 11, `doubleFind(-3)` → -1).
- User enums with payloads parse and construct
  (`enum Shape { Circle(int), Rect(int, int) }`, `Shape.Circle(5)`).
- `match` on plain enums works (tag comparison); `panic()` builtin
  exists; `try`/`catch`/`throw` work same-function-only (Phase 3).
- Representation: tagged union = `{i32 tag, i64 payload}` struct.

### 0.2 What is broken — the Phase 9 scope

1. **Payload ownership — USE-AFTER-FREE (critical).** Class/container
   payloads are stored as raw i64 bits with no retain. Proven live
   (`~/workspace/p9-probes/p3b_uaf.sp` on the Phase 37 binary):
   `make()` returns `Result.Ok(new Foo(7))`; `live after make` is **0**;
   allocating `Foo(99)` reuses the freed memory; `r.unwrap().n`
   prints **99**, not 7. The Foo died when `make()` returned (its
   only +1 released by `f`'s scope drop) and the Result holds a
   dangling pointer. The Result struct is by-value with no destructor
   (Phase 8 known gap), so there is no release point either —
   retaining on construction without a drop point would leak instead.
   Open sub-question for slice A: `unwrap()`/`unwrapOr()`/`unwrapErr()`
   ownership — is the returned payload +1 or borrowed? (If borrowed,
   `var f = r.unwrap();` then using `f` after `r` drops is the same
   UAF through another door.)
2. **Match cannot bind payloads.** `Shape.Circle(r)` and
   `Result.Ok(v)` arms fail: `r`/`v` undefined (E1002), plus a
   confusing E1001 ("Match pattern type 'Result<<type-error>, any>'").
   Extraction today is only via `.unwrap()` — which is exactly the
   door the UAF above walks through.
3. **Async + Result does not compose.** An `async fn` returning
   `Result<...>` is rejected: Task payloads support only
   void/int/bool/string/double/array/struct (E1000, loud). `?` inside
   async fns is untested because the return type cannot exist.
4. **Payload width.** Anything that does not fit one i64 has no
   representation: struct payloads (16+ bytes), multi-payload
   variants (`Rect(int, int)`), nested tagged unions
   (`Result<Result<int,string>, string>` — inner is 16 bytes).
   Current behaviour: untested / presumably miscompiled or rejected
   somewhere unclear. Needs a decision, not a guess.
5. **throw/catch role (DECISION — see §0.4).** ADR-002 says panic for
   unrecoverable and Result for recoverable; it does not mention
   `throw`. The deferred cross-function throw unwinding is answered
   by ADR-002 itself: **no unwinding** — Result replaces it.

### 0.3 Design — payload ownership

The payload slot holds either a scalar/pointer directly, or (slice D)
a pointer to a heap box for payloads that do not fit i64. Ownership
follows the ARC +1 rule from Phase 7-C, extended to enum payloads:
- **Construction** (`Result.Ok(v)` / `Option.Some(v)` / user-enum
  variant construction) **retains** a reference-type payload (+1
  moves into the tagged union).
- **Drop** — when a Result/Option/enum value dies (scope drop, temp
  release, reassignment of the old value), the payload is **released**
  if its static type is owned. This needs a targeted special case in
  the drop machinery for the builtin tagged unions (and user enums
  with payloads): Results are structs, and structs have no
  destructors — the drop site must release the payload field
  directly, keyed on the value's sema type, not on a generated
  destructor.
- **Unwrap** returns the payload as +1 (the Result keeps its own
  share until it dies; unwrap retains a second share for the
  caller — to be confirmed in slice A; the alternative, borrowed
  unwrap, is only safe if the Result provably outlives the use,
  which the temp machinery cannot prove).
- **Boxed payloads (slice D):** the box is an ARC object; construction
  allocates + retains, drop releases the box (which releases its
  contents). One mechanism, two payload shapes.

### 0.4 Decision — throw stays, unwinding does not

`throw`/`try`/`catch` stay exactly as they are (same-function-only,
local control flow) and are documented as such. No cross-function
stack unwinding is built — ADR-002's Result model replaces it, and
the ADR's own consequence list (zero unwinding overhead) rules it
out. `panic()` stays for unrecoverable logic errors. This is the
conservative reading of the ADR; if Shanto wants `throw` deprecated
or removed, that is a one-line override.

### 0.5 Slices

- **A — Payload ownership (the UAF fix).** Retain on Ok/Some/variant
  construction; release on Result/Option/enum drop (scope drops +
  temp release + reassignment); settle unwrap ownership (+1).
  Proofs: p3/p3b → live counts correct, unwrap reads 7 after
  allocating 99; `temp_chain`-style proof for Result temps.
- **B — Match payload binding.** Sema + IRBuilder: `Result.Ok(v)`,
  `Shape.Circle(r)` arms bind the payload (with the tag check);
  fix the E1001 confusion; `_` and multi-arm fallthrough unchanged.
  Proofs: p1/p4 → `circle 5`, `ok 5` / `err division by zero`.
- **C — Async + Result.** Task payloads carry Result/Option
  (extend `taskInnerType` + the async return machinery); `?` inside
  async fns (early return through the state machine — the err block
  must release the frame's temps, cf. Phase 8-D's cancel path).
  Proofs: p2 → async work returns `Ok(43)` through await + `?`.
- **D — Payload width (boxing).** Struct payloads, multi-payload
  variants, nested tagged unions → heap-boxed payloads under the
  slice-A ownership rule; loud E1000 for anything still
  unrepresentable. Proofs: struct payload round-trips; `Rect(3,4)`
  binds both fields.
- **E — Stdlib migration audit + docs.** Audit which fallible stdlib
  fns still return bool/null instead of Result/Option (fs has `try*`
  variants already; io/db/crypto partially); migrate the bounded
  set (fs/io first), document the error model in the Syntax Guide +
  STDLIB_REFERENCE, re-run the doc gate.

### 0.6 Expected outcomes

- ctest 838 → 838 + new tests (only UDP EPERM; PG skipped).
- All Phase 37 live proofs byte-identical (no ARC regression).
- Doc gate 119/128 genuine (unchanged).
- `docs/v2/PHASE-38-PRO-4.0.0.md` + slice docs A–E at the end.

### 0.7 Graphify protocol (repo AGENTS.md)

`python -m graphify update . --force` cannot run: `graphify-out/`
does not exist and `python -m graphify` is not installed (verified
2026-11-10, as in Phases 5–8). Not fabricated; the phase proceeds
without it.
