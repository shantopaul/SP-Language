# Phase 37 (pro-4.0.0) — Slice A: Real class destructors + implicit-return scope drops

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `ac8ff9a` (ADR-001–005 Accepted; Phase 7 done at `5ea6a69`)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, nothing pushed)
- **Scope:** part of ADR-001 (Hybrid Value Semantics + ARC). Generate a real
  `<Class>_destructor` for every class with owned reference fields, so a
  dying object releases its fields; and fix the implicit-return path so
  locals are dropped when a function (or synthesized `main`) falls through
  without an explicit `return`.
- **Status:** IN PROGRESS in the main tree (parent implements this slice).

**Repo AGENTS.md note:** the repo mandates a Graphify protocol
(`python -m graphify update . --force` before/after coding), but
`graphify-out/` does not exist and `python -m graphify` is not installed
(verified 2026-11-10, as in Phases 5–7). It cannot be run; nothing about
it is fabricated here.

## 0. Design — written before the implementation

### 0.1 What is wrong today (verified by reading the code, not assumed)

1. **No destructor is ever generated.** `visit(NewExpr)` already looks up
   `<Class>_destructor` in the module (`IRBuilder.cpp`, `effectiveName +
   "_destructor"`) and passes it to `sp_rt_arc_alloc` — but no emitter
   ever creates such a function, so the pointer is always null. The
   runtime already supports it fully (Phase 11): `SPDestructorFn =
   void (*)(void*)`, `sp_rt_arc_alloc(size, dtor)` stores it in the
   object header, `sp_rt_release` calls `header->destructor(ptr)` on
   the last strong release, then releases the weak hold. Only codegen
   is missing.
2. **A dying object never releases its fields.** Consequence, measured
   in Phase 7-C (`temp_chain.sp` on the parent binary): `after chain 1
   / after arg 2` — the outer temporary is dropped but its child keeps
   one leaked share. Same for slice C's `weak_parent_child.sp`:
   `after block 1` — the parent dies, the strong child field is never
   released. Field *reassignment* already releases the old value
   (`AssignExpr`: load old → store new → release old), so this slice
   is only about *death*, not reassignment.
3. **Implicit returns never drop locals.** `popScope()` (which emits
   the scope drops) is called *after* the implicit `ReturnInst` is
   added, in both `visit(Program)` (synthesized `main`) and
   `visit(FunctionDecl)` (sync tail) — and `emitScopeDrops` returns
   early when the block already has a terminator. So a function that
   falls through without an explicit `return` leaks every local
   (Phase 6-C §9 noted this for `main`; it is the same bug in both
   places). Explicit `return` already drops (`visit(ReturnStmt)` calls
   `emitAllFunctionDrops`), so this fix only touches the fall-through
   path.

### 0.2 Mechanism — generate destructors up front, drop before the return

**Destructor generation (`emitClassDestructors`, called from `build()`
right after `buildProgramLayouts`, before `visit(program)`).**
Layouts are built eagerly (plain in step 2, specialized generics in
step 2b, structs in step 3), so every layout exists before any `new`
is compiled — the `NewExpr` lookup then finds the generated function.
For each layout:

- Resolve the sema `ClassType` by layout name (direct `findUserType`,
  else iterate user types comparing `mangleGenericName`). If none is
  found (interfaces, structs, builtins without a decl), skip the
  layout — see §0.5 for the struct gap.
- Releasable fields = `field.irType->isPointer()` AND NOT weak.
  Weak-ness comes from sema, not from the layout: `MemberField.isWeak`
  is populated by `NameResolver` (`.isWeak = varDecl.isWeak()`), so a
  single pre-pass over all user `ClassType`s builds the set
  `(mangleGenericName(className), fieldName)` of weak fields. This
  deliberately does NOT touch `VTableBuilder`'s `FieldLayoutInfo` —
  slice C (parallel worktree) adds `isWeak` plumbing there, and this
  slice must not conflict with its patch.
- If no releasable field exists, generate nothing (the `NewExpr`
  lookup keeps returning null, as today).
- If a function named `<LayoutName>_destructor` already exists in the
  module, generate nothing (a user method named `destructor` would
  mangle to exactly that name; the pre-existing `NewExpr` lookup would
  already use it as the destructor — this slice preserves that
  behaviour and never overwrites it).
- Otherwise emit `void <LayoutName>_destructor(ptr %obj)`: for each
  releasable field at struct index `field.index`, GEP into
  `layout.structType` at `(0, index)`, load the pointer, call
  `sp_rt_release`. `sp_rt_release` is null-safe and magic-checked, so
  a null field and a non-ARC pointer field (string, raw `ptr<T>`) are
  harmless no-ops; the extra calls are the price of not resolving
  field types. Base-class fields are already flattened into the
  derived layout, so the derived destructor releases them directly
  and must NOT call the base destructor (that would double-release).
- The destructor never touches slot 0 (vtable pointer — not in
  `layout.fields`) or static fields (in `layout.staticFields`, not in
  the instance layout; statics live for the process lifetime).

**Why this composes with the parallel slices (no dependency):**
- Slice B (containers): `new List()` etc. never reach the generic
  `NewExpr` path — `visit(NewExpr)` special-cases containers to
  `sp_rt_*_alloc`, whose destructors are C++ functions wired inside
  the runtime. `buildSpecializedClassLayout` returns early for
  List/Map/Set/Stack/Queue/PriorityQueue/Channel, so they have no
  layouts at all and this generator never sees them. A container held
  in a class field is released by the class destructor as a plain
  ARC object, and the container's own (slice B) destructor then
  releases its elements — the two destructors chain through the
  runtime, neither knows about the other.
- Slice C (weak refs): weak fields are skipped here via the sema set,
  so a weak slot (which stores a *header* pointer, not a payload)
  is never passed to `sp_rt_release`. The `FieldLayoutInfo.isWeak`
  plumbing slice C adds is the same fact from the other side; the
  parent merge must keep both.
- Slice D (temp scopes): untouched. The temp-scope pop releases the
  temporary, which now *runs its destructor* — that is the point:
  `makeOuter().child` goes from `after chain 1` to `after chain 0`
  because the pop now cascades through the destructor.

**Implicit-return drops (2 sites, same one-line shape).** Call
`emitAllFunctionDrops()` *before* adding the implicit `ReturnInst`:
- `visit(Program)` synthesized `main`: `emitMainExitPump()` first
  (fire-and-forget tasks drain before locals die), then drops, then
  `return 0`. `popScope()` stays after (its emit is a no-op on the
  terminated block; it only pops the vectors).
- `visit(FunctionDecl)` sync tail: drops before the implicit return
  in all three branches (void / `main` / other — for `main`: pump,
  then drops, then return). Async functions return early through
  `asyncSMEnd` and never reach this code; explicit `return` already
  drops in `visit(ReturnStmt)`, so there is no double-drop path:
  the implicit block only runs when the block has no terminator.

### 0.3 What this pass does NOT change

No runtime changes (no new `sp_rt_*` primitives — the Phase 11
destructor hook already exists); no change to the +1 return
convention, the temp-scope pass, the async state machine, or the
debugger interpreter. Field reassignment already releases the old
value — untouched. Container allocation is untouched (slice B).

### 0.4 Expected proof outcomes (derived before implementing)

| Proof | Parent (`ac8ff9a`) | After this slice |
|---|---|---|
| `temp_chain.sp` (Phase 7-C) | `after chain 1 / after arg 2` | `after chain 0 / after arg 0` — the outer temp's death now cascades to the child |
| `weak_parent_child.sp` (slice C) | `after block 1` | `after block 0` — the parent's death releases the strong child field |
| `dtor_nested.sp` — outer{inner{leaf}} | `after block 2` (inner+leaf leak) | `after block 0` |
| `dtor_reassign.sp` — field reassigned, then block end | old child freed at reassign (already worked); `after block 1` (new child leaks at death) | `after block 0` |
| `dtor_fn_fallthrough.sp` — `fn helper() { let x = makeFoo(); }` (no return), `main` prints `sp_rt_mem_live_objects()` before/after `helper()` | `after helper 1` | `after helper 0` |
| Synthesized `main` locals | unobservable from inside `main` (drops run after the last statement) — verified by IR inspection: `spc --emit-ir` must show `sp_rt_release` calls before `ret i32 0` in `main` | same check passes |

### 0.5 Deliberately not done / known limits

- **Structs with reference fields.** Struct layouts are skipped (no
  `ClassType` resolves for them). A struct holding a class reference
  leaks that share — struct destructors / deep value semantics are a
  follow-up, not this slice.
- **Destructor recursion depth.** A destructor releases fields, each
  release can run another destructor — a 10k-deep linked list frees
  through 10k nested C++ calls. Same shape as Swift `deinit` chains;
  no mitigation in this slice, documented here.
- **Self-referential strong cycles.** A cycle never reaches a zero
  strong count, so no destructor ever runs — needs weak refs
  (slice C). A destructor cannot fix a cycle.
- **Globals / statics / `main` locals at process exit.** Statics live
  for the process lifetime by design; the synthesized-`main` fix
  drops main's *locals* at its return, but nothing runs after
  `main` returns, so that fix is only observable in the IR (see
  §0.4) and by the fall-through function test.
- **Weak fields in classes without a sema decl.** If no `ClassType`
  resolves for a layout, the layout is skipped entirely — no
  destructor is generated, so no weak slot can be mis-released.

## 1. Verification (parent, this slice)

Build dir `~/workspace/build-phase8-dtors` (fresh configure; the first
build was interrupted by a VM restart at ~29% and resumed incrementally
— build exit 0, 0 errors). Clang 18.1.3 was wiped by the restart and
recovered from `~/workspace/clang-extracted`. Proofs in
`~/workspace/p8a-proofs/`, run on the slice-A binary:

| Proof | Result |
|---|---|
| `dtor_nested.sp` (outer→inner→leaf) | `before 0 / inside 3 / leaf 5 / after block 0` ✓ |
| `dtor_parent_child.sp` (parent→child) | `before 0 / inside 2 / n 7 / after block 0` ✓ |
| `dtor_reassign.sp` | `before 0 / first 2 / after reassign 2 / n 2 / after block 0` ✓ |
| `dtor_fn_fallthrough.sp` | `before 0 / helper inside 1 / after helper 0` ✓ |
| `temp_chain.sp` (Phase 7-C) | `before 0 / chain 9 / after chain 0 / arg 9 / after arg 0` ✓ (was 1 / 2) |

IR inspection (`spc llvm`, the `--emit-ir` flag is actually the
`llvm` subcommand): `Parent_destructor` is generated — GEP
`(0, 1)`, load, `sp_rt_release`, `ret void` — and `new Parent()`
passes `@Parent_destructor` to `sp_rt_arc_alloc`, while `new Child()`
(int field only) correctly passes `ptr null`. The synthesized
`main` in `dtor_main_ir.sp` shows the pump, then `%drop_x13 =
load ptr, ptr %x_1; call void @sp_rt_release(ptr %drop_x13)`, then
`ret i32 0` — the implicit-return drop verified in IR (unobservable
from inside the program, as documented).

ctest: **810/811** (99%) on the slice-A tree — the only failure is
the known environmental `NetTest.UdpSocketSendRecv` (EPERM, same as
every phase); `PostgresB1.PgLiveStringArrayRoundTrip` skipped
(no Postgres in the fresh VM — skip-guarded). All 6 new/renamed
destructor tests pass (415–420), and the unchanged
`SelfReferentialTempDoesNotCrash` and
`FieldInitFactoryDoesNotDoubleCount` still pass.
