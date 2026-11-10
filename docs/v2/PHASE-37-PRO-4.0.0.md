# Phase 37 (pro-4.0.0) — ADR-001 ARC completion

- **Branch:** `feature/pro-4.0.0` (branch only; remote `main` still `d10e1c2`)
- **Base:** `ac8ff9a` (ADR-001–005 acceptance, 2026-11-10)
- **Final commit:** `8393d38` (slice E) + this doc commit
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, nothing merged to `main`)
- **Diff vs base:** 20 files, +4,486/−174
- **Status:** DONE, pushed to `feature/pro-4.0.0` only (single push at phase end)

ADR-001 (Hybrid Value Semantics + ARC, Swift/Nim style, approved by
Shanto Paul 2026-11-10) is now implemented end to end. Before this
phase, ARC existed as machinery with no teeth: the runtime had
`sp_rt_arc_alloc` / `sp_rt_release` / weak primitives, the temp
pass recorded +1 values, but no `<Class>_destructor` was ever
generated, `main`'s locals were never dropped, containers were not
ARC objects, every weak slot behaved as strong, and the last two
temp scopes (async, short-circuit/ternary) leaked by design. This
phase closes all five gaps.

## Slices

### A — Real destructors + implicit-return scope drops
Commits `c7787d5` (codegen), `9ec0da4` (tests), `5ddfd12` (doc).
`IRBuilder::emitClassDestructors()` generates a real
`<Class>_destructor` per layout (GEP field → load → `sp_rt_release`;
weak fields skipped via sema `MemberField.isWeak`; base-class fields
are flattened into derived layouts, so no base-dtor call is needed).
`emitAllFunctionDrops()` now runs before the implicit `ReturnInst`
in both tails (synthesized main, `visit(FunctionDecl)` sync tail) —
`main`'s locals finally drop. Proofs: `dtor_nested` → `after block 0`,
`dtor_parent_child` → `after block 0`, `dtor_reassign` → `after
reassign 2 / after block 0`, `dtor_fn_fallthrough` → `after helper 0`;
`temp_chain` → `after chain 0 / after arg 0` (was 1/2). ctest 810/811
(+5; only UDP). Doc: `docs/v2/PHASE-37-SLICE-A.md`.

### B — Container element release (ARC destructors)
Merged `e89e6e7` (worktree `sp-language-p8-b`, 6 commits).
Runtime ARC destructors: `sp_rt_list_alloc`/`map`/`set`/`queue`/`pq`/
`array` allocate via `sp_rt_arc_alloc(sizeof(Impl), <kind>_destructor)`;
a compiler-set `elements_are_refs` flag (set when the static element
type is an owned type) makes the destructor release retained
elements, then free internal buffers. Release-on-drop added to 11
mutating fns; Channel drains on destroy. Also fixed: `peek` retain
(theft bug), `trySend` retain+mark, a Set `_str`/`_i64` dispatch bug;
`isFreshPlusOneExpr` widened (array/map literals +1; `reduce`
excluded; `split`/array `map`/`filter` +1); new `isOwnedValueType`
predicate. Proofs: all 15 container drops → 0 (incl. nested,
borrowed, returned, reassign, remove, channel destroy). ctest 817/818
(+12; only UDP). **C-ABI contract change:** runtime arrays must be
freed with `sp_rt_free` (ARC-aware), NOT `std::free` (payload sits 16
bytes after the malloc base). Doc: `docs/v2/PHASE-37-SLICE-B.md`.

### Parent fix — ARC payload registry (`d5e5ea2`)
Found by the A+B combined tree: `CollectionStressAndScalingNativeExecution`
segfaulted at teardown on a 8193-element list. Root cause: B's
`sp_rt_free`/`get_arc_header` read the magic 24 bytes before any
pointer; a ≥128 KiB mmap'd buffer has unmapped bytes before its base.
Fix (Runtime.cpp only, +76 with test): a live ARC payload registry
(`unordered_set` under a mutex, inserted by `sp_rt_arc_alloc`,
erased at the single header-free point); `get_arc_header` checks
membership before reading memory. Lock never held across a destructor
call. Regression test `ContainerLargeListDropDoesNotCrash` (10k
pushes, block-end drop). Merged A+B ctest 823/824 (only UDP).

### C — Weak references (runtime weak table + codegen)
Merged `910e046` (worktree `sp-language-p8-c`, 4 commits). The runtime
already had complete weak primitives (Phase 11) and the parser/sema
already accepted `weak` — only codegen was missing (zero "weak" in
IRBuilder.cpp). A weak slot stores the object **header** pointer;
stores create the weak ref without retaining and never adopt a +1
RHS; loads upgrade to payload (+1) or null (zeroing weak refs). A
weak LOAD is a +1 producer (recorded via `recordTempOwnedValue`),
not borrowed. `FieldLayoutInfo.isWeak` plumbed through both layout
paths; `emitScopeDrops` releases weak locals with
`sp_rt_weak_release`. Proofs: `weak_self` `after block 1` → **0**,
`weak_null_after_death` → `after 1 / target is null`,
`weak_parent_child` → `after block 0` (cross-slice A+C composition).
At merge, the test `WeakParentChildCyclePartiallyBreaks` was renamed
→ `WeakParentChildCycleFullyBreaks` with expectation `after block 0`
(its own comment said to update it when slice A lands). ctest 828/829
(+5; only UDP). Doc: `docs/v2/PHASE-37-SLICE-C.md`.

### D — Remaining temp scopes (async + short-circuit/ternary)
Merged `c68ba28` (worktree `sp-language-p8-d`, 3 commits). Closes both
Phase 7-C §6 leaks: async temps (the state machine carries pending
temporaries across suspend points — `asyncSpillTempScopes` stores
them into a heap array in the `kTempSpillSlot` frame slot,
`asyncRestoreTempScopes` reloads them at the resume,
`asyncReleaseSpilledTemps` releases them in the cancel block) and
short-circuit RHS / ternary-arm temps (per-branch scopes,
`m_tempBranchScopes`, released by the branch block itself, replacing
the `m_tempBranchDepth > 0` suppression). Proofs: `temp_async` →
`v 42 / after await 0` (was 1), `temp_shortcircuit` → `after or 0 /
after tern 0` (was 2/3), `ret_temp_leak` exactly the Phase 7-C record.
ctest 835/836 (+7; only UDP). Doc: `docs/v2/PHASE-37-SLICE-D.md`.

**Merge corrections (both found by verification, not by review):**
- The first D resolution kept slice C's `m_inAsyncFunction` guard in
  `recordTempOwnedValue` verbatim — that re-suppressed async
  recording and broke D's async proof (`after await 1`). Corrected
  to D's guard (no async suppression; recording + spilling is what
  releases async temps). The D merge also dropped the now-dead
  `m_tempBranchDepth` member and all its uses.
- The C merge needed 4 hand-resolved conflicts and the D merge 3
  (all in IRBuilder.h/cpp, all "keep both" resolutions — B's widened
  `isOwnedValueType` predicate preserved through both, all three
  recorders delegating to the shared `recordTempOwnedValue`).

### E — EscapeAnalysis wiring + value-semantics/Arena audit + doc gate
Commit `8393d38` (main tree). The escape analysis existed, compiled,
and was unit-tested (6 tests) but nothing ever ran it. It now runs
once per `build()` (after the comptime registry, before lowering),
is kept on the builder behind `getEscapeAnalysis()`, and its verdicts
are printed by `spc sema` (sorted, with reasons). 2 new tests pin the
uncovered triggers (outer-scope store; return from a lambda body).
**Deliberately NOT done: retain/release elision.** The +1 convention
makes the pairs load-bearing (an adopted +1 is the object's lifetime;
a borrowed init needs its retain to survive its source), so the only
sound elision is stack allocation — a later phase. A wrong verdict
today costs only a wrong diagnostic line. Audit (no code change):
`std.mem.Arena` is real end to end (stdlib → runtime
`Runtime.cpp:749–833`; `MemoryArenaNativeExecution` passes); structs
are by-value with no destructor (struct ref fields still leak, known
gap). Doc gate re-run: **120/128 printed, 119/128 genuine** —
unchanged from the Phase 5–7 baseline (ex121 still the orphan-server
false pass; verified alone it times out). Doc:
`docs/v2/PHASE-37-SLICE-E.md`.

## Verification (merged tree, incremental build dir `build-phase8-dtors`)

- ctest progression: 806 (Phase 7) → 811 (A) → 823 (A+B) → 824
  (+registry test) → 829 (A+B+C) → 836 (A+B+C+D) → **838
  (A+B+C+D+E)**. Final run **837/838** — only `NetTest.UdpSocketSendRecv`
  (EPERM, environmental, fails on every unmodified tree); PG live
  test skipped (no server).
- Live proofs on the merged binary (all slices' proof dirs):
  `dtor_nested`/`dtor_parent_child`/`dtor_reassign`/
  `dtor_fn_fallthrough` → 0; container drops (list/array/map/set/
  queue/stack/pq/nested/borrowed/returned/reassign/remove/channel)
  → 0; `weak_self` → 0, `weak_null_after_death` → null,
  `weak_parent_child` → 0; `temp_async` → `v 42 / after await 0`,
  `temp_shortcircuit` → `after or 0 / after tern 0`,
  `ret_temp_leak` exactly the Phase 7-C record; 10k-list stress
  prints all 9 lines, exit 0; `spc sema` prints the three escape
  verdicts with reasons.
- Doc gate: 119/128 genuine RUN-OK (see slice E §0.4).

## Follow-ups (not this phase)

- Retain/release elision and stack allocation (needs the escape
  analysis consumed for optimization; re-audit its soundness first).
- Struct destructors (struct fields holding references leak).
- Weak container elements, weak params, `weak Any`, struct weak
  fields, zombie headers (slice C out-of-scope list).
- Derived-container and pop/dequeue-transfer leaks, unresolved-call
  Map/Array temps, container-`new` as pure temp, `recv`'d items,
  nullable container/element, self-cycles (slice B §6 list).
- Still-open SP-Language items from earlier phases: `m.get("a")?.n`
  IR error; chained safe-nav `a?.b.c` segfault; generic List/Map IR
  verification failures; doc-gate orphans (ex007/ex065/ex106 are
  servers); misleading `spc` "Unsupported backend compiler"
  message (ex043/ex126/ex127); no `std/http` section in
  STDLIB_REFERENCE; nullable-int 0-is-null convention; ctor
  chaining / base-class field initializers; static-init cross-class
  order hazard.
- ADR roadmap next: ADR-002 (Result/Option — revisit whether
  cross-function throw unwinding is still needed), ADR-004
  (`spc bindgen`), ADR-003 remainder (epoll event loop), ADR-005
  remainder (registry hosting decision still open with Shanto).
