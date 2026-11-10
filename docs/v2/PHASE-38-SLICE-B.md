# Phase 38 (pro-4.0.0) — Slice B: match payload binding

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `665252c` (Phase 38 slice A)
- **Version:** 3.1.0, unchanged
- **Scope:** `match` arms can bind a variant's payload —
  `Result.Ok(v)`, `Result.Err(e)`, `Option.Some(x)`,
  `Shape.Circle(r)`, `Option.None()` / `Option.None`. No nested
  patterns, no literal-argument patterns (clear errors, not
  miscompiles — follow-ups). Also fixes the match-subject temp leak
  (slice A regression for TU temps + pre-existing for class temps).
- **Status:** DONE — committed on `feature/pro-4.0.0` (see §0.6).
  Verified: proofs + 4 new tests + full ctest 845/846 (only UDP EPERM).

## 0.6 Verification (2026-11-10, build `~/workspace/build-phase9`, Release)

- Proofs (`~/workspace/p9-probes/`, new `spc`):
  `p1_user_enum` → `circle 5`; `p4_match_result` → `ok 5` /
  `err division by zero`; `p1b_multi_reject` → one clear E1001
  ("Match on variant 'Rect' with multiple payloads is not
  supported yet (Phase 38 slice D)"); slice A proofs byte-identical
  (`p3` 0/1/7/1, `p3b` 1/2/7/99, `p5` 11/-1).
- New tests (4): `MatchResultBindsPayload`,
  `MatchOptionBindsPayload` (covers both `Option.None()` and
  `Option.None`), `MatchUserEnumBindsPayload`,
  `MatchOnTempResultReleasesPayload` — all pass.
- Full ctest: **845/846** (842 + 4 new; only
  `NetTest.UdpSocketSendRecv` fails — EPERM, environmental; PG
  skipped, no server).
- Bug found during verification (real, mine): the sema shadowing
  check used a bare `m_symbolTable.lookup(name)`, but the
  NameResolver registers enum names as EnumSymbols in the same
  table — so `Shape` always "shadowed" itself and user-enum
  patterns fell through to E1002. Fixed: only Variable /
  Parameter / Constant / Function / Method bindings shadow a type
  name. (`addVariant` already sets the tagged-union flag for
  payload enums — no fix needed there.)
- Known gaps left (leaks, not UAFs): an arm body that RETURNS
  leaks the temporary subject's share (the endBB release covers
  the fallthrough path only); multi-payload variants and nested /
  literal-argument patterns are loud E1001s, not silent
  miscompiles.

## 0. Design — written before the implementation

### 0.1 What the probes showed (Phase 38 investigation)

- `Shape.Circle(r)` / `Result.Ok(v)` parse fine (CallExpr) but
  sema reports E1002 (`r`/`v` undefined) + a confusing E1001
  ("Match pattern type 'Result<<type-error>, any>'").
- The current match lowering evaluates the pattern as an
  EXPRESSION and compares it by value with the subject — for a
  variant construction that is a struct-vs-struct comparison, i.e.
  uncharted / wrong. A variant pattern must compare TAGS.
- The subject is adopted as a temp (`adoptTempValue(subjVal)`) and
  never released: a `match makeFoo()` leaks one share (pre-existing,
  Phase 7-C), and after slice A a `match makeResult()` leaks the
  payload share the same way (the adopt swallows the record).

### 0.2 Sema (TypeChecker::visit(MatchStmt))

A pattern is a **variant pattern** when the subject is a tagged
union and the pattern is one of:

1. CallExpr, callee MemberExpr `E.V` — `E` is an IdentifierExpr
   naming a tagged-union enum (via `findUserType`; a same-named
   VARIABLE shadows it — check the symbol table first), `V` is a
   variant of that enum, and the enum's base name equals the
   subject enum's base name.
2. CallExpr, callee bare IdentifierExpr `V` — `V` is a variant of
   the SUBJECT's enum (`Ok`/`Some`/`None`, or a user enum's bare
   variant).
3. MemberExpr `E.V` (no call) — `V` is a NULLARY variant of the
   enum (e.g. `Option.None`).

For a variant pattern (payload types read off the SUBJECT's
specialized EnumType — `specializeEnumType` already substituted the
concrete args, so `Result<Foo, string>` gives Ok → [Foo]):

- The variant must exist in the subject's enum, else E1001.
- Arg count must equal the variant's payload count, else E1001.
- Every arg must be a bare IdentifierExpr (a binding), else E1001
  ("match pattern arguments must be bare identifiers to bind the
  payload" — literal args and nested patterns are follow-ups, not
  silent miscompiles).
- Each binding is declared (VariableSymbol, payload type) in a
  per-arm scope (the catch-binding pattern:
  `enterScope(Block)` / `define` / check body / `exitScope`).
- The pattern's type for the subject check is the subject type
  itself (matches by construction).

Non-variant patterns keep the old path (checkExpr + assignability).

### 0.3 IR (IRBuilder::visit(MatchStmt))

- Detect the same three forms (mirrored logic; the payload types
  and tag come from the subject's EnumType via
  `getVariantPayload(V)` / `getVariantValue(V)`).
- Test block: load the subject's TAG (field 0, via the new
  `taggedUnionFieldPtr` — the payload helper refactored to take a
  field index) and compare `==` variant tag. No whole-struct
  comparison.
- Body block (after `pushScope()`): for each binding, create an
  alloca in the entry block, store the extracted payload (field 1,
  cast to the payload's IR type) into it, `declareVariable` the
  binding. **Bindings are BORROWED** (no scope-drop registration):
  the subject outlives the arm body (locals) or dies at the
  statement pop (temps) — either way after the body's uses — and
  adopters (assignment, return) retain by the existing copy rule.
- Subject-temp release: before `adoptTempValue(subjVal)`, save the
  payload (TU temp, via `m_tempPayloadOwners`) and note whether
  the subject itself was a recorded class temp; at `endBB`,
  release the saved payload / subject value. A local subject is
  never recorded, so nothing is released for it.

### 0.4 Out of scope

- Nested patterns (`Result.Ok(Some(x))`), literal-argument
  patterns (`Result.Ok(5)`) — clear E1001, follow-ups.
- Class FIELDS holding a Result/Option (slice A gap, unchanged).
- Exhaustiveness checking (no new diagnostics for missing arms).

### 0.5 Expected outcomes

- Proofs (`~/workspace/p9-probes/`): `p1_user_enum` →
  `circle 5`; `p4_match_result` → `ok 5` / `err division by zero`.
- New tests: `MatchResultOkBindsPayload`,
  `MatchResultErrBindsPayload`, `MatchOptionSomeBindsPayload`,
  `MatchUserEnumPayloadBinds`, `MatchOnTempResultReleasesPayload`
  (live count 0 after a match on a temp class-payload Result).
- Full ctest 842 + new tests (only UDP EPERM; PG skipped).
- All Phase 37 + slice A live proofs byte-identical.
