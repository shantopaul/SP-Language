# Phase 38 (pro-4.0.0) — Slice A: tagged-union payload ownership (the UAF fix)

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `a17f702` (Phase 37 done)
- **Version:** 3.1.0, unchanged
- **Scope:** fix the proven Result/Option class-payload use-after-free
  (`p3b_uaf.sp`: `Result.Ok(new Foo(7))` dies in `make()`, unwrap
  reads 99 instead of 7). No new syntax, no match changes (slice B),
  no async changes (slice C), no boxing (slice D).
- **Status:** DONE — committed on `feature/pro-4.0.0` (see §0.7).
  Verified: proofs + 4 new tests + full ctest 841/842 (only UDP EPERM).

## 0.7 Verification (2026-11-10, build `~/workspace/build-phase9`, Release)

- Proofs (`~/workspace/p9-probes/`, new `spc`):
  `p3_class_payload` → `before 0 / after make 1 / unwrap n 7 /
  after unwrap 1` (was 0/0/7/0); `p3b_uaf` → `live after make 1 /
  live after g 2 / r unwrap n 7 / g n 99` (was 0/1/99/99 — the
  UAF is gone); `p5_option_try` unchanged (`some 11 / none -1`).
- New tests (4): `ResultClassPayloadLiveCounts`,
  `ResultClassPayloadSurvivesReuse`, `ResultTempPayloadReleased`,
  `ResultReassignReleasesOldPayload` — all pass; the 13-test
  Result/TryOperator filter passes.
- Full ctest: **841/842** (Phase 8 base 838 + 4 new; only
  `NetTest.UdpSocketSendRecv` fails — EPERM, environmental; PG
  skipped, no server).
- Two build fixes during the slice (both mine, both in IRBuilder):
  `getNodeType(node.getArgs()[0])` needs `.get()` (unique_ptr);
  `m_taggedUnionScopes` is a tuple list, not a pair list.
- Known gaps left (leaks, not UAFs): no-init Result locals are not
  registered (uninitialized alloca — dropping it would release
  garbage bits; the first assignment releases the old value via
  AssignExpr, the final value leaks one share); class FIELDS
  holding a Result/Option leak their payload when the object dies
  (same class as the Phase 8-A struct-field gap).

## 0. Design — written before the implementation

### 0.1 The ownership rule (one rule, five sites)

A tagged union (Result/Option/user enum with payloads) **owns** its
payload: exactly one share, following the Phase 7-C +1/borrowed
discipline. The payload slot holds raw i64 bits; ownership is a
compile-time property of the *value*, not of the bits.

- **Construction** (`Result.Ok(v)` / `Err` / `Option.Some(v)` /
  user-enum variant / bare `Ok`/`Some` / uncaught-throw `Err`)
  **retains** an owned payload (adopt a +1 argument, retain a
  borrowed one — same discriminator as VarDecl).
- **Copy** (VarDecl init, assignment, return) of a tagged-union
  value **retains** the payload when the source is borrowed, and
  adopts (no retain) when the source is a fresh +1.
- **Drop** (scope drop of a tagged-union local, temp pop of a
  tagged-union temp, reassignment of the old value) **releases**
  the payload.
- **`?` and `unwrap*` yield BORROWED.** No retain at extraction:
  the Result keeps its share; adopters retain by the copy rule
  above; the operand's recorded payload is released at the
  statement pop (ok path) — the use always precedes the pop. The
  alternative (+1 unwrap) would need `isFreshPlusOneExpr` extended
  to builtins and a new temp-producer kind; borrowed is strictly
  simpler and equally safe. (`unwrapOr`'s default is borrowed too.)
- **Arg passing is borrowed and needs nothing:** function params are
  never registered for scope drops (the only `m_refCountedScopes`
  push is VarDecl), so a callee cannot release a caller's payload.

The Phase 8 parent fix makes unconditional payload release SAFE:
`sp_rt_release` on a non-ARC pointer / small integer / null is a
registry miss (no-op). So the drop/retain helpers release/retain
the slot contents whenever *any* variant's payload type is owned —
the slot holds exactly one payload, and the tag tells which, but the
release does not need to know.

Out of scope (leaks, not UAFs — noted, not fixed here): class
FIELDS holding a Result/Option (the field's payload leaks when the
object dies — same class as the Phase 8-A "struct fields holding
refs" gap); globals/statics.

### 0.2 Helpers (new, IRBuilder)

- `taggedUnionHasOwnedPayload(sema::TypePtr)` — EnumType +
  `isTaggedUnion()` + any variant's `getVariantPayload()` type
  satisfies `isOwnedValueType()`.
- `taggedUnionPayloadPtr(IRValuePtr v)` — pointer to the payload
  slot: GEP `{0,1}` directly when `v` is already a pointer to the
  struct; spill to an entry-block alloca first when `v` is a struct
  SSA value (same pattern as TryExpr's `try_tmp`).
- `emitRetainTaggedUnionPayload(IRValuePtr v, sema::TypePtr t)` /
  `emitReleaseTaggedUnionPayload(IRValuePtr v, sema::TypePtr t)` —
  no-op unless `taggedUnionHasOwnedPayload(t)`; else load the slot
  (bitcast to pointer) and call `sp_rt_retain` / `sp_rt_release`.
- Construction helper: `emitTaggedUnionConstructionRetain(argNode,
  payloadType, payloadVal)` — `adoptTempValue(payloadVal)`, then
  retain unless `isFreshPlusOneExpr(*argNode)`. (The arg's own temp
  record is removed by the adopt; the Result's payload record is
  made by the caller's recorder — see §0.3.)

### 0.3 Temp machinery (the one shared change)

A tagged-union temp's payload is recorded, not the struct: a new
map `m_tempPayloadOwners` (struct-pointer → payload-pointer).

- `recordTempCallResult`: when the call's sema return type is a
  tagged union with an owned payload, load the payload and push
  it into the current temp scope, and record the struct→payload
  mapping. (Independent of `isFreshPlusOneExpr` — a `Result.Ok(f)`
  result is a fresh +1 Result even though the call is a builtin.)
- `adoptTempValue(v)`: existing identity removal, PLUS: if `v` is
  a key in `m_tempPayloadOwners`, remove its payload from the
  pending scopes and erase the map entry. All adoption sites pass
  the struct, so one change covers VarDecl/return/assign.
- `emitTempScopeReleases`: unchanged (it releases whatever
  IRValuePtrs are pending — payload pointers are plain pointers).
- Async spill/restore: unchanged shape (it spills pending values;
  restored payload pointers are plain pending values released at
  the pop). Adoption of a spilled temp does not happen: a temp
  produced before an await is consumed (borrowed) after the resume,
  never adopted — the statement's adopted value is computed after
  the resume. (If this assumption is ever violated, the failure
  mode is a leak, not a UAF: the payload record survives and is
  released at the pop.)
- `m_tempPayloadOwners` is cleared per function with the temp
  scopes (same reset point as `m_tempScopes`).

### 0.4 Drop sites

- **Scope drops:** new parallel list `m_taggedUnionScopes`
  (same shape as `m_refCountedScopes`: per-scope list of
  (name, allocaPtr)). VarDecl registers a local when its sema type
  is a tagged union with an owned payload. `emitScopeDrops` also
  drops these: GEP payload → load → `sp_rt_release`. (Weak Result
  locals are out of scope — `weak` applies to class slots only.)
- **Reassignment:** AssignExpr mirrors the class path for a
  tagged-union target: adopt rhs, retain payload if not fresh,
  release the OLD payload (load old struct, release its payload)
  before storing.
- **Return:** `emitRefReturnRetain` extended — a borrowed
  tagged-union return retains its payload (fresh +1 returns do
  not). Struct-valued `retVal` goes through the spill pattern.
- **TryExpr err block:** the early return moves the operand Result
  to the caller, so its payload must NOT be released — but other
  pending temps (e.g. a class temp produced earlier in the same
  statement) must be flushed, or the `?` path leaks them (a
  pre-existing leak class, Phase 7-C §6 "mid-statement throw
  path"). In the err block, emit releases for all pending temp
  values EXCEPT the operand's payload (looked up via
  `m_tempPayloadOwners`); the compile-time stacks are untouched
  (the ok path pops normally).

### 0.5 Construction sites (all call the §0.2 helper)

`Result.Ok/Err`, `Option.Some` member arms; bare `Ok/Err/Some`
free-call arms; user-enum variant construction; uncaught-throw
`Err` (the thrown value's ownership follows the throw-owns flag —
read that block before touching it; the helper's adopt+retain is
the transfer, the existing release of the thrown share stays).

### 0.6 Expected outcomes

- Proofs (`~/workspace/p9-probes/`): `p3_class_payload` →
  `before 0 / after make 1 / unwrap n 7 / after unwrap 1`;
  `p3b_uaf` → `live after make 1 / live after g 2 /
  r unwrap n 7 / g n 99` (was 0 / 1 / 99 / 99);
  `p5_option_try` unchanged (`some 11 / none -1`).
- New tests: `ResultClassPayloadLiveCounts` (p3), `ResultClassPayloadSurvivesReuse` (p3b),
  `ResultTempPayloadReleased` (pure-temp Result does not leak),
  `ResultReassignReleasesOldPayload`.
- Full ctest 838 + new tests (only UDP EPERM; PG skipped).
- All Phase 37 live proofs byte-identical (no ARC regression).
