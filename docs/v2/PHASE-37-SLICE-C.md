# Phase 37 (pro-4.0.0) Slice C — Weak references (cycle breaking)

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `ac8ff9a` (Phase 7 done + ADR-001–005 accepted)
- **Scope:** implement ADR-001's "weak references to break cycles"
  end-to-end in codegen. The runtime primitives and the
  parser/sema already exist; what is missing is the wiring between
  them — today a `weak` field/local behaves exactly like a strong
  one (verified by baselines below).
- **Status:** design written first (this §0), implementation in
  progress. Nothing pushed (never push from a slice worktree).

## 0. Design (written BEFORE implementing)

### 0.1 What already existed (verified, not assumed)

**Runtime — complete (Phase 11, `runtime/src/Runtime.cpp`).**
`SPObjectHeader { magic, strong_count, weak_count, allocated_size,
destructor }`; `sp_rt_arc_alloc(size, dtor)` starts at strong=1,
weak=1 (the strong refs hold one implicit weak ref);
`sp_rt_retain` / `sp_rt_release` (release to 0 invokes the
destructor if any, then `sp_rt_weak_release(header)`); and the
weak family:

- `sp_rt_weak_create(strong_ptr)` — bumps `weak_count`, returns
  the **header pointer** (not the payload); null-safe (returns null
  for null / non-ARC pointers).
- `sp_rt_weak_retain(weak_ptr)` / `sp_rt_weak_release(weak_ptr)` —
  operate on header pointers; the last weak drop frees the block.
- `sp_rt_weak_upgrade(weak_ptr)` — CAS on `strong_count`: returns
  null when the object is already dead, otherwise +1 and the
  payload pointer. This is the zeroing behaviour: a dead target
  reads null, never a dangling pointer.
- `sp_rt_weak_count`.

All five are declared in `compiler/src/ir/IR.cpp` and covered by
`tests/runtime/RuntimeTest.cpp` (`WeakReferenceCycleBreakingLifecycle`).
**No runtime change is needed for this slice.**

**Parser / sema — complete.** Lexer keyword `weak`;
`weak` modifier on fields (`Parser.cpp:745`) and locals
(`Parser.cpp:1225,1254-1256,1348`); `VarDeclStmt.isWeak()`;
`MemberField.isWeak` (populated by `NameResolver.cpp:286`);
E1015 in `NameResolver` + `TypeChecker.cpp:1638` (`weak` only on
Class/Interface types, or a Nullable over one — so `weak int` and
`weak string` are rejected; `weak Node?` is accepted). Params
cannot be `weak` (the parser never accepts the modifier there) —
out of scope by construction. There is even an execution test,
`ExecutionTest.WeakReferenceCycleBreakingNativeExecution`
(`weak Node parent;` … expects `tree_total: 300`), which passes
today only because the field behaves as strong — it checks values,
not ownership.

**Codegen — zero weak handling.** `compiler/src/ir/IRBuilder.cpp`
(11,129 lines) contains no occurrence of "weak". A `weak` field
store retains the new value and releases the old with
`sp_rt_release`; a `weak` load is a plain pointer load; a `weak`
local is retained at declaration and released with
`sp_rt_release` at scope exit. Baselines (§1) confirm it.

### 0.2 Mechanism (chosen): reuse the Phase 11 primitives

A weak slot is a `ptr` slot holding the object **header**
pointer (or null) — the value `sp_rt_weak_create` returns. Same
IR type and same object layout as a strong field, so no layout,
ABI, or vtable change. The compiler's only job is to emit the
right runtime call at each of the five sites where strong and
weak differ:

| Site | Strong (today) | Weak (this slice) |
|---|---|---|
| Field / local **store** | retain new, release old (`sp_rt_release`) | `sp_rt_weak_create(new)` → store header; `sp_rt_weak_release(old)` |
| Field / local **load** | plain load | load header, `sp_rt_weak_upgrade(slot)` → payload (+1) or null |
| **Scope drop** of a local | `sp_rt_release` | `sp_rt_weak_release` |
| Field **initializer** (`new` or factory) | retain unless `new` (adopt +1) | `sp_rt_weak_create(init)`; never retain, never adopt the +1 |
| **Return / throw / adopt** of a loaded value | retain if borrowed | a weak load IS +1 (see §0.3) — adopt, do not retain |

**Store rule detail.** A `+1` RHS must NOT be adopted into a
weak slot: the slot holds no strong share, so the +1 stays pending
and the statement temp-scope pop releases it. That is what makes
`weak Node q = makeNode();` read null — the node has no strong
owner and dies at once, which is the correct semantics (a weak
ref must not keep its target alive). A borrowed RHS needs
nothing: `weak_create` only bumps the weak count.

**Load rule detail.** The upgrade is emitted at the load site —
for a plain load, right after the `LoadInst`; for a safe-nav
(`?.`) load, inside the access block (the raw header is loaded
there; the merge block then yields the upgraded value, which
dominates it). A bare-field read through an implicit `this`
(`visit(IdentifierExpr)`'s field path) is a load site too and
gets the same treatment. `&weakField` (address-of) returns the
slot address unchanged — writing through it bypasses
`weak_create`, the same class of hazard as `&` on a strong field
bypassing retain/release; documented, not fixed.

### 0.3 The upgrade's +1, and a deviation from the brief (justified)

The task brief said "a `weak` load must be BORROWED, never
recorded as a +1 temp". That does **not** hold for the runtime
that actually exists: `sp_rt_weak_upgrade` **returns +1** — it
CAS-increments `strong_count`. Treating the result as borrowed
would leak one strong share per weak read: the target could then
never reach strong 0, weak refs would never read null, and the
cycle proof would still print `after block 1`. A borrowed load
(computing the payload address from the header without the CAS)
would be worse: after the target dies, the header block stays
allocated as a zombie (weak refs keep it alive), so a borrowed
load would read **stale field values** instead of null.

So the rule is: **a weak load is a +1 producer**, recorded as a
pending temp with the new generic `recordTempOwnedValue`
(`recordTempCallResult` / `recordTempNewResult` delegate to it),
exactly like a Phase 7-C call-result temp. Adopters take it
(variable init, assignment to a *strong* target, return, throw,
container store — which retains and lets the pop release the
+1); otherwise the statement scope pop releases it. This
extends Phase 7-C's ownership rule by one clause:

> OWNED (+1) = value produced by `new`, or by a call
> sema-resolved to a non-async user function/method returning a
> Class/Interface type, **or by a weak-reference upgrade**.
> BORROWED = everything else.

Consequently `isFreshPlusOneExpr` is extended to recognise weak
loads (MemberExpr on a weak field, IdentifierExpr on a weak
local / bare weak field), so the return / throw / var-init /
assignment paths do not double-retain an already-+1 upgrade.
Without that extension, `return child.parent;` would retain the
+1 a second time and leak one share per return.

In async functions temp recording is off (as in Phase 7-C), so
weak-load upgrades leak there too — same documented limitation,
same reason (await splits blocks).

### 0.4 Plumbing: where "is this field weak?" is answered

`MemberField.isWeak` exists in sema but codegen cannot see it:
`FieldLayoutInfo` (`compiler/include/sp/codegen/VTableBuilder.h`)
has no `isWeak`. This slice adds `bool isWeak{false}` to
`FieldLayoutInfo` (populated from `MemberField.isWeak` where
`VTableBuilder` builds class layouts) and to `StaticFieldInfo`
(same source), so the layout is the single source of truth for
codegen — and so slice A (parallel work: real
`<Class>_destructor` generation) can later release weak fields
with `sp_rt_weak_release` instead of `sp_rt_release` without
re-deriving weakness. Weak locals are tracked in IRBuilder as a
set of weak local slot pointers (`m_weakLocalAllocas`), filled
at `visit(VarDeclStmt)` when `node.isWeak()`; the set is keyed
by slot identity, so shadowing and async heap-frame slots are
safe, and stale entries from earlier functions can never match
a fresh slot.

### 0.5 Honest limit: no destructors yet (slice A's work)

This slice does **not** generate destructors and does not depend
on slice A. Consequence, stated before implementing so the
proofs are read correctly: when an object dies, its strong
fields are never released (nothing runs to release them). So in
a parent→child cycle (`root.child` strong, `child.parent`
weak), the parent dies on schedule but the child stays alive,
held by the dead parent's strong field — proof 4 prints
`after block 1`, not 0. That last share is released exactly
when destructors exist (slice A), and the `isWeak` plumbing in
§0.4 is the hand-off that lets a destructor tell weak slots
apart. The same applies inside a dead object: a weak slot that
lives in a dead object can never be released without a
destructor, so the dead target's header block stays allocated
(a zombie) until the holder dies — `sp_rt_mem_live_objects()`
still drops to 0 (it counts strong-liveness, not allocation),
so the proof metric is correct, but the memory is retained.
Both limits are ARC-completion work, not this slice.

### 0.6 Out of scope (deliberate)

- **Weak elements in containers** (List/Map/Array/Channel):
  elements are stored as raw i64 bits; a weak element would need
  a different element kind. A strong element that happens to be
  a weak load is fine (the container retains the upgraded +1).
- **Weak params**: the parser does not accept them.
- **Weak statics dropping**: module globals are never dropped;
  a weak static stores via `weak_create` like any weak field and
  its slot lives for the program's lifetime.
- **Async functions**: upgrade +1s leak there (temp recording
  off), same as call temps in Phase 7-C.
- **`weak Any`**: sema allows `Any` as "reference-like", but an
  `Any` value is an i64 payload, not an ARC object —
  `weak_create` on it is the same pre-existing hazard as
  `retain` on an `Any` (both call `get_arc_header` on a
  non-object). Not made worse, not fixed.
- **Compound assignment** (`+=` etc.) on a weak field:
  meaningless for a pointer field; sema rejects arithmetic on
  class types.
- **Struct weak fields**: structs are value types; the sema
  `weak` check for struct members is not wired, and this slice
  does not wire it.
- **Ergonomics note (not changed):** a non-nullable weak field
  cannot be compared to `null` (`== null` on `Node` is E1001),
  so the null-after-death proofs spell the field `weak Node?`
  and read it with `?.`. A weak field is arguably implicitly
  nullable, but changing nullability is a sema decision for a
  later slice; the `?` spelling works and is what the proofs
  use.

## 1. Baselines (parent binary `~/workspace/build-phase7-parent/bin/spc`, code = `ac8ff9a`)

All five proofs run on the parent binary first. `weak` is
parsed and sema-checked, but codegen treats every weak slot as
strong — the baselines prove it.

| Proof | Parent output | Expected after fix |
|---|---|---|
| `weak_self.sp` (self-cycle via weak field) | `before 0 / self n 3 / inside 1 / after block 1` | `after block 0` |
| `weak_null_after_death.sp` (weak field reads null after target dies) | `alive n 42 / alive? yes / after 2 / target alive 42` | `after 1 / target is null` |
| `weak_strong_keeps_alive.sp` (strong owner keeps target readable) | `n 7 / live 2` | unchanged |
| `weak_parent_child.sp` (parent→child strong, child→parent weak) | `before 0 / inside 2 / after block 2` | `inside 2 / after block 1` (see §0.5 — the last share needs a destructor) |
| `weak_local.sp` (`weak` local over a live target; `weak` local over a factory temp) | `p n 9 / inside 1 / q n 5 / after q 2 / after 1` | `p n 9 / inside 1 / q is null / after q 1 / after 1` |

Two proof-writing findings (language behaviour, not bugs):
`h.target == null` on a non-nullable `Node` weak field is E1001
("Cannot compare incompatible types 'Node' and 'null'"), and
`h.target.n` on a `Node?` weak field is E1008 (nullable access
requires `?.`). The proofs therefore spell the null-checked
fields `weak Node?` and read them with `?.` (see §0.6).

Proofs live in `~/workspace/p8c-proofs/` (`weak_self.sp`,
`weak_null_after_death.sp`, `weak_strong_keeps_alive.sp`,
`weak_parent_child.sp`, `weak_local.sp`).

## 2. Implementation

Files changed (worktree only; 5 files):

- `compiler/include/sp/codegen/VTableBuilder.h` — `isWeak` on
  `FieldLayoutInfo` and `StaticFieldInfo`.
- `compiler/src/codegen/VTableBuilder.cpp` — populate `isWeak`
  in both layout paths (AST-based class fields + statics, and
  the sema-based generic path). Base-class field copies carry
  the flag along for free (the whole `FieldLayoutInfo` is
  copied).
- `compiler/include/sp/ir/IRBuilder.h` — `isWeakField`,
  `isWeakLocalSlot`, `isWeakLoadExpr`, `emitWeakCreate`,
  `emitWeakUpgrade`, `recordTempOwnedValue`, and the
  `m_weakLocalAllocas` slot set.
- `compiler/src/ir/IRBuilder.cpp` (+327/−54) — the five sites:
  1. **Store** (`visit(AssignmentExpr)`): a weak target stores
     `sp_rt_weak_create(rhs)` and releases the old slot with
     `sp_rt_weak_release`; no retain, no adopt of a +1 RHS.
  2. **Load** (`visit(MemberExpr)` plain + safe-nav access
     block, `visit(IdentifierExpr)` local / bare-field / static
     paths): load the header, `sp_rt_weak_upgrade` to the
     payload (+1, recorded as a pending temp) or null.
  3. **Locals** (`visit(VarDeclStmt)`): a `weak` local is
     registered in the refcounted scopes and marked in
     `m_weakLocalAllocas`; its initializer is stored via
     `weak_create` with no retain and no adopt.
  4. **Scope drops** (`emitScopeDrops`, which the throw path
     also delegates to): weak slots drop with
     `sp_rt_weak_release`.
  5. **Initializers** (`emitFieldInitializers`, static-init
     function): a weak field stores `weak_create(init)`; no
     retain, no adopt of a +1 `new`.
  Plus the shared machinery: `recordTempOwnedValue` (the two
  existing recorders delegate to it) and the `isFreshPlusOneExpr`
  extension for weak loads (see §0.3), which keeps return /
  throw / var-init / assignment from double-retaining an
  already-+1 upgrade.
- `tests/codegen/ExecutionTest.cpp` — 5 new tests after the
  existing `WeakReferenceCycleBreakingNativeExecution` (see §4).

No runtime change: the Phase 11 primitives are used as-is.
No parser/sema change: `weak` was already parsed and checked.
No layout/ABI change: a weak slot is a `ptr` like a strong one.

Build: fresh Release build in
`~/workspace/build-phase8-weakrefs` (configure with the
Phase 4 googletest source dir; `-j2`).

## 3. Proofs (new binary `~/workspace/build-phase8-weakrefs/bin/spc`)

All five proofs from §1, re-run on the new binary:

| Proof | New output | Verdict |
|---|---|---|
| `weak_self.sp` | `before 0 / self n 3 / inside 1 / after block 0` | ✅ (was 1) |
| `weak_null_after_death.sp` | `alive n 42 / alive? yes / after 1 / target is null` | ✅ (was 2 / alive 42) |
| `weak_strong_keeps_alive.sp` | `n 7 / live 2` | ✅ unchanged |
| `weak_parent_child.sp` | `before 0 / inside 2 / after block 1` | ✅ as designed (§0.5 — the last share needs a destructor) |
| `weak_local.sp` | `p n 9 / inside 1 / q is null / after q 1 / after 1` | ✅ (was `q n 5 / after q 2`) |

## 4. Tests

5 new execution tests in `tests/codegen/ExecutionTest.cpp`
(after the existing `WeakReferenceCycleBreakingNativeExecution`,
which still passes):

- `WeakFieldSelfCycleBreaks` — self-cycle, `after block 0`.
- `WeakFieldReadsNullAfterTargetDies` — `after 1`,
  `target is null`.
- `WeakFieldReadsDoNotLeak` — three weak reads, `live 2`
  (pins the upgrade-+1 recording: without it each read leaks
  a strong share and `live` would be 5).
- `WeakLocalDoesNotRetain` — `q is null`, `after q 1`.
- `WeakParentChildCyclePartiallyBreaks` — `after block 1`,
  with a comment that it becomes 0 when slice A
  (destructor generation) lands.

Targeted run: **6/6 weak tests pass** (5 new + the
pre-existing one). Full ctest in
`~/workspace/build-phase8-weakrefs`: **809/811 (99%)** —
total 811 = 806 (Phase 7) + 5 new. Failures:

| Test | Cause |
|---|---|
| `NetTest.UdpSocketSendRecv` | Sandbox UDP EPERM (environmental, pre-existing) |
| `JsonTest.BenchmarkOneGigabytePerSecThroughput` | Throughput threshold under CPU contention — **fails identically on the unmodified parent binary** (`103.5 MB/s` vs the `250 MB/s` threshold, re-run alone just now), so it is the known flake, not a regression |

## 5. Notes

- **Build race (caught, not shipped).** The first fresh build
  was started before the edits were finished; `IRBuilder.cpp.o`
  (03:27:02) predated the last source edit (03:29:40), so the
  first binary was stale. A deterministic incremental rebuild
  recompiled it — and that rebuild caught a real type error the
  first build had missed: in `visit(MemberExpr)` the loaded
  value is a `shared_ptr<LoadInst>`, which cannot be reassigned
  an `IRValuePtr` upgrade result; fixed by introducing a
  separate `IRValuePtr` (`storedVal` / `value`). Final build:
  exit 0, 0 errors, `spc` + `sp_tests` relinked from the fixed
  object.
- **Graphify:** repo `AGENTS.md` mandates
  `python -m graphify update . --force`, but `graphify-out/`
  does not exist and the tool is not installed — it cannot be
  run; nothing fabricated.
- **Environment:** Clang 18.1.3 was already present; no
  reinstall needed. No Postgres use in this slice.
- **Still out of scope** (see §0.6): weak container elements,
  weak params, async weak-load leaks, `weak Any`, compound
  assignment on weak fields, struct weak fields, `&weakField`
  writes, and the destructor-dependent remainder (parent→child
  strong link after the parent dies; weak slots inside dead
  objects). The `FieldLayoutInfo.isWeak` plumbing is the
  hand-off slice A's destructor generator needs.
