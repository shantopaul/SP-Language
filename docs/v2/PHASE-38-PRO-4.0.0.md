# Phase 38 (pro-4.0.0) — ADR-002 Result/Option error model

- **Branch:** `feature/pro-4.0.0` (branch only; remote `main` still `d10e1c2`)
- **Base:** `a17f702` (Phase 37, ADR-001 ARC completion)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, nothing pushed)
- **Status:** DONE 2026-11-10 — all five slices implemented, verified,
  committed on `feature/pro-4.0.0` (see §0.5); pushed once at the end
  of the phase (single push; remote `main` still `d10e1c2`).
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
- **Unwrap (settled in slice A — BORROWED, not +1).** `?` and
  `unwrap*` yield **borrowed** values (no retain at extraction;
  adopters retain by the copy rule). The +1 alternative was
  rejected: it would double-retain every `unwrap()` used as a
  temporary and leak the extra share on the common
  `println(r.unwrap())` path. The borrowed rule is safe because
  the temp machinery releases a TU temp only after its last use,
  and `?`'s err block flushes pending temps except the operand's
  payload (which moves into the early return).
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

### 0.5 Slices (all DONE — commits on `feature/pro-4.0.0`)

- **A — Payload ownership (the UAF fix).** Commit `665252c`
  (`docs/v2/PHASE-38-SLICE-A.md`). Retain on Ok/Some/variant
  construction; release on Result/Option/enum drop (scope drops +
  temp release + reassignment); `?`/`unwrap*` settled as
  **borrowed**. Proofs: p3/p3b → `live after make 1`,
  `r unwrap n 7 / g n 99` (UAF gone). 4 new tests. ctest 841/842.
- **B — Match payload binding.** Commit `3100474`
  (`docs/v2/PHASE-38-SLICE-B.md`). Sema + IR: `Result.Ok(v)`,
  `Shape.Circle(r)` arms bind the payload (tag check + payload
  extraction into a fresh alloca); one clear E1001 for invalid
  patterns; subject-temp release fixed (was adopted and never
  released). Found a real sema bug: enum names are `EnumSymbol`s
  in the same table as variables, so the shadowing check rejected
  every user-enum pattern — fixed to check symbol kinds. Proofs:
  p1/p4 → `circle 5`, `ok 5` / `err division by zero`. 4 new tests.
  ctest 845/846.
- **C — Async + Result.** Commit `64a53ee`
  (`docs/v2/PHASE-38-SLICE-C.md`). New Task payload kind
  `"taggedunion"`: `taskInnerType` accepts a TU inner type, the
  async return boxes the struct, `?` inside an async fn boxes the
  operand into the frame and branches to the finish block. Await
  ownership: form 2 (`await worker()`) transfers the box's share
  (+1 result); form 3 (`await t`) is borrowed. Proofs: p2/p2b →
  `ok 43` / `err boom`, no hang. 4 new tests. ctest 849/850.
- **D — Payload width (boxing).** Commit `bf0d505`
  (`docs/v2/PHASE-38-SLICE-D.md`). Wide payloads are heap-boxed
  (`sp_rt_arc_alloc` + shared destructors): struct payloads (null
  dtor), nested TUs (`__sp_tu_box_dtor_nested`), multi-payload
  variants (one shared `__sp_tu_box_dtor_multi_<N>`, unused slots
  zeroed). Struct payloads are snapshot-copied at every yield
  point (fixed a struct-payload UAF found by the proofs). Sema
  multi-payload match rejection lifted. Proofs: p6/p7/p8/p8b/p8c/
  p8d → `x 3 y 4`, `rect 3 4`, `v 42`, live counts to 0. 7 new
  tests. ctest 856/857.
- **E — Stdlib migration audit + docs.** Commit `cd3d546`
  (`docs/v2/PHASE-38-SLICE-E.md`). Audit of all 24 stdlib modules:
  only fs/io use Result/Option (15 functions). Bounded migration:
  `FS.tryListDir` / `tryListDir` → `Result<string[], IOError>`
  (NotFound / InvalidInput) and a real `tryReadLine` (EOF →
  `Err(UnexpectedEOF)` via new runtime `sp_rt_stdin_eof()`). Found
  that the stdlib `.sp` files are reference only — the fs/io API
  is compiler builtins (sema registration + IR lowering) — and
  fixed a latent infinite loop: `try*` lowerings used fixed block
  names, so a second call in one function branched back to the
  first call's blocks; all 43 `try*` block creations now use
  `createBlock()` (unique suffix). STDLIB_REFERENCE `std/fs`
  rewritten to the real API + `try*` variants; Syntax Guide §28
  stale "match does not work" note replaced with a verified
  match-binding example + payload-ownership paragraph. 2 new tests.
  ctest 858/859. Doc gate 121/129.

### 0.6 Actual outcomes

- ctest **858/859** (only the standing `NetTest.UdpSocketSendRecv`
  EPERM failure; PG skipped without a server). Phase 37 base was
  838; +21 tests across slices A–E (4 + 4 + 4 + 7 + 2).
- All Phase 37 live proofs byte-identical (no ARC regression);
  slices A–D proofs re-run byte-identical after each later slice
  (except `p1b_multi_reject`, superseded by slice D's multi-payload
  support).
- Doc gate **121/129 RUN-OK** (was 119/128 — one new block, the
  §28 match example, passes). 8 non-OK are the same known
  categories (numbering shifted +1 after §28).
- `docs/v2/PHASE-38-PRO-4.0.0.md` + slice docs A–E complete.
- Total diff `a17f702..cd3d546`: 6 commits; see `git diff --stat`
  at push time (files below in §0.8).

### 0.8 Follow-ups (open, not this phase)

- Class **fields** holding a `Result`/`Option` leak the payload
  when the object dies (slice A gap; same class as Phase 8-A's
  struct-field gap). Globals/statics likewise.
- Match-subject share leaks when an arm body **returns**
  (fallthrough path only releases it — slice B gap).
- Nested patterns (`Result.Ok(Some(x))`), literal-argument
  patterns (`Result.Ok(5)`) — clear E1001s today; exhaustiveness
  checking not implemented.
- `Result`/`Option` as an async fn **argument**; `Task<Task<...>>`,
  `Task<Map<...>>`, plain-enum Task payloads (loud E1000).
- The `?` operand-vs-enclosing-return type mismatch (pre-existing
  sema hole, sync fns too).
- `unwrap` on the wrong variant returns a zero value instead of
  panicking (pre-existing; guarded unbox for wide payloads).
- Per-module stdlib `Result`/`Option` migration beyond fs/io
  (db/json/crypto/net/redis/http/os/process/collections — audit
  table in slice E §0 is the record; each needs its own error enum
  and call-site migration).
- `tryListDir` TOCTOU (needs a runtime error channel to fix).
- Retain/release elision + stack allocation (needs the
  escape-analysis soundness re-audit first — Phase 8-E note).

### 0.7 Graphify protocol (repo AGENTS.md)

`python -m graphify update . --force` cannot run: `graphify-out/`
does not exist and `python -m graphify` is not installed (verified
2026-11-10, as in Phases 5–8). Not fabricated; the phase proceeds
without it.
