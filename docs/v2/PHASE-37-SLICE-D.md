# Phase 37 (pro-4.0.0) — Slice D: Remaining temp scopes (async + short-circuit/ternary)

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `ac8ff9a` (ADR-001–005 marked Accepted; Phase 7 done at `5ea6a69`)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes)
- **Scope:** close the two cheapest remaining leaks from Phase 7 slice C
  (its doc §6–§7): async-function temporaries and short-circuit RHS /
  ternary-arm temporaries. The Phase 7-C ownership rule (§0.1) and the
  +1 return convention are unchanged; no runtime changes; no
  destructors, container frees, or weak refs (ADR-001 work, not this
  slice).
- **Status:** DONE, uncommitted in this worktree (parent merges).
  All proofs pass; ctest 811/813 (only UDP + the known Json
  benchmark flake, which passes alone).

**Repo AGENTS.md note:** the repo mandates a Graphify protocol
(`python -m graphify update . --force` before/after coding), but
`graphify-out/` does not exist in this worktree and `python -m
graphify` is not installed (verified 2026-11-10, as in Phases 5–7).
It cannot be run; nothing about it is fabricated here.

## 0. Design

### 0.1 What Phase 7-C left open (the two leaks)

- **Async temps** (`temp_async.sp`, `after await 1`): recording was
  disabled whenever `m_inAsyncFunction` was true, because await
  splits blocks — a temporary defined before a suspend point does
  not dominate the resume block, so a release emitted after the
  await would be invalid IR.
- **Short-circuit / ternary temps** (`temp_shortcircuit.sp`, `after
  and 1 / after or 2 / after tern 3`): a branch-depth counter
  suppressed recording inside the RHS of `&&` / `||` / `??` and
  inside ternary arms — those values are defined in a branch block
  that does not dominate the merge block, so a release in the merge
  block would be invalid IR.

### 0.2 Short-circuit / ternary: per-branch pops (as sketched in C §7)

The branch-depth counter is replaced by a **branch-scope stack**
(`m_tempBranchScopes`): while a branch region is open, temporaries
are recorded into the innermost branch scope instead of the
statement scope. Each branch block pops its own scope **in the
branch block itself, before its branch to the merge** — so the
release instructions are emitted where the defining block
dominates them. The branch's result value is adopted first (it is
stored into the result slot and used after the merge, so it must
not be released in the branch — it leaks as before, which is the
pre-existing behaviour for arm-result temps).

Sites: `&&` / `||` RHS, `??` RHS, ternary then-arm, ternary else-arm.
Nested branches stack correctly (each arm pushes its own scope;
the inner pop restores recording into the outer branch scope).
LHS temps are unaffected (defined in the pre-branch block, recorded
in the statement scope as before, released at the statement pop in
the merge block). `adoptTempValue` now scans branch scopes too
(innermost-outward, branch scopes before statement scopes), so
adopters inside a branch (e.g. `flag && (x = makeBox())`) still
find their value. Switch-case-value temps are NOT covered here
(they are evaluated in test blocks — a separate leak, same
mechanism could close it later; see §6).

### 0.3 Async: the state machine carries temp scopes across suspends

**Decision: carry, not per-await release.** Slice C §7 sketched two
options — carry temp scopes across suspend points, or a per-await
release. Investigation of the Phase 5-A state machine showed the
per-await release is **unsound**, so it was rejected (see §0.4).
The chosen mechanism spills pending temporaries into the heap
frame at each suspend and restores them at the resume:

- The pre-scan (`asyncPrepareBody`) guarantees every statement
  contains **at most one await, and it is first-evaluated**
  (extra / non-first awaits are hoisted into preceding
  `let __sp_await_N` statements). So at a suspend point the only
  sub-expression evaluated in the current statement is the await's
  operand — and the only temps pending are the ones produced while
  evaluating that operand.
- `asyncSuspendAndLoad` spills every pending temporary (all open
  temp scopes, in order) into a heap-allocated array and stores
  the array pointer in a new dedicated frame slot
  (`AsyncSM::kTempSpillSlot`, slot 7; `kFirstParamSlot` moves 7→8
  — the frame layout is compiler-side only, all accesses go through
  the slot constants, so the shift is self-consistent). The array
  holds raw pointers, not shares: the spill does not retain — it
  moves the +1 shares into frame memory for the duration of the
  suspend.
- At the resume (start of `bodyBB`), the array is loaded back,
  each temporary is reloaded into a fresh SSA value that dominates
  the rest of the segment, the pending-list entries are updated to
  the reloaded values, and the array is freed. The statement's
  scope pop later releases the reloaded values in the resume
  segment, where they dominate — valid IR.
- If the body is cancelled while suspended, the cancel block
  releases the spilled temporaries and frees the array (the body
  is abandoned, so the temps are dead there — mirroring the
  existing `emitAllFunctionDrops()` for locals).
- When no temporary is pending at a suspend, nothing is spilled
  (no allocation, no slot write, no restore) — the common case
  costs nothing.

Recording is enabled in async functions (the `m_inAsyncFunction`
early-returns in `pushTempScope` / `popTempScope` /
`recordTempCallResult` / `recordTempNewResult` are removed), and
the async `ReturnStmt` branch gains the same push/adopt/pop
around its return-value evaluation that the sync branch already
had (async return payloads are never classes, but the return
*expression* can contain temporaries).

### 0.4 Why per-await release was rejected: transitive capture

A per-await release would emit `sp_rt_release` for all pending
temporaries in the suspend block, on the theory that everything
produced while evaluating the await's operand is dead once the
operand's Task exists. That theory is false: a temporary can be
**captured by an async spawn** — directly as an argument
(`await useBox(makeBox())`) or **transitively through a sync call**
(`await takeTask(makeBox())` where `takeTask` forwards its
parameter to an async spawn). The spawn stores the raw pointer
into the task ctx **with no retain** (`emitAsyncSpawnSM`), the
async body borrows it (its parameters are never dropped by the
body — params are not in `m_refCountedScopes`), and the caller's
+1 is the only share keeping the object alive. Releasing the
temporary at the suspend point would free the object while the
spawned body still needs it — a use-after-free.

The direct case is detectable at the spawn site (the argument
value *is* the temporary), but the transitive case is not: at
the spawn, the argument is a load of the callee's parameter, and
nothing connects it back to the caller's temporary. No local
liveness analysis can rule it out without interprocedural
tracking. So per-await release cannot be made sound here, and
was rejected.

### 0.5 Async-call arguments are adopted (transferred to the task)

`emitAsyncSpawnSM` — the single choke point for every async
spawn (free fn, method, generic, fn-pointer) — now adopts each
pointer argument value from the pending lists. Rationale: the
spawn transfers the argument's +1 share to the task (see §0.4);
the task's body borrows it and never drops it, so the share is
owned by the task for the task's lifetime. Releasing it at the
spawning statement's pop would free the object before the body
runs. Adopting keeps the status quo (the share leaks, exactly
as it did when recording was off in async functions) and is
provably safe.

This also fixes a **latent use-after-free in sync code** that
Phase 7-C introduced: `fn main() { let t = useBox(makeBox()); }`
released the temporary at the statement pop, before the spawned
body ran — the body read freed memory and only worked by luck
(proof `p_async_arg_sync.sp`: baseline `body live 0`, the object
already freed when the body ran, `body n 42` by luck of
unreused heap memory). With the adopt, the object stays alive
(`body live 1`) and the read is sound.

The adopt does **not** apply to the task value itself (a Task is
not a reference type — `isReferenceType()` is Class/Interface
only — so a Task is never recorded as a temporary in the first
place), nor to scalar/string arguments (never temporaries).

### 0.6 Why the spill is sound (the liveness argument)

- At a suspend, every pending temporary was produced while
  evaluating the await's operand (§0.3, pre-scan invariant).
  Its consumers are all inside the operand evaluation — except
  for capture by an async spawn (§0.4), which the §0.5 adopt
  removes from the pending lists before the suspend. What
  remains is dead at the suspend (consumed by its parent
  operation inside the operand, or retained-into by a container
  / field store whose retain already happened).
- The spill preserves the shares across the suspend without
  retaining: the array holds the pointers, the frame slot holds
  the array. The await synchronizes with the awaited task — it
  completes before the resume — so by the time the statement
  pop releases the restored temporaries, the awaited task's body
  is done with anything it borrowed.
- Residual hazard (pre-existing, also present in sync code
  today, not introduced here): a temporary captured by a task
  that is spawned but **never awaited** (e.g. an operand that
  spawns a side task and awaits a different one) can be released
  at the statement pop while the side task still runs. That is
  the general "async spawn does not retain its ref arguments"
  flaw — fixing it means retaining ref arguments at spawn,
  which is a spawn-convention change (ARC territory), out of
  scope for this slice.

### 0.7 What this slice does NOT change

No runtime changes; no change to the +1 return convention, the
ownership rule, scope drops, or the debugger interpreter. The
change is confined to `compiler/src/ir/IRBuilder.cpp` (+
declarations in `compiler/include/sp/ir/IRBuilder.h`) and tests,
plus one frame-layout constant (a new reserved slot — the frame
is compiler-side only). No destructors, container frees, or weak
refs are built.

## 1. What was wrong

Under the Phase 7-C +1 convention, two temporary kinds were never
released (baseline measurements with the parent binary at
`ac8ff9a`, all in `~/workspace/p8d-proofs/`):

| Proof | Baseline | After this slice |
|---|---|---|
| `temp_async.sp` — temp inside `async fn` | `after await 1` | `after await 0` |
| `temp_shortcircuit.sp` — `&&` / `\|\|` RHS + ternary arm | `after and 1 / after or 2 / after tern 3` | `after and 0 / after or 0 / after tern 0` |
| `p_async_arg.sp` — temp as async-call arg, awaited | `inline 42 / after inline 1 / spawned 42 / after spawned 2` | unchanged (survives by design — §0.5) |
| `p_async_arg_sync.sp` — temp as async-call arg, sync caller | `after spawn 0 / body live 0 / body n 42` | `after spawn 1 / body live 1 / body n 42` (the latent UAF — §0.5) |
| `p_await_operand_temp.sp` — temp in an await operand, transitively captured | `v 42 / after await 1` | `v 42 / after await 0` |
| `p_await_dead_temp.sp` — temps in async-fn statements around awaits | `after 1 / after2 2` | `after 0 / after2 0` |
| `p_async_return_temp.sp` — temp in an async return expression | `v 42 / after await 1` | `v 42 / after await 0` |
| `p_shortcircuit_nested.sp` — nested `&&` / `\|\|` RHS temps | `after nested or 1 / after nested and 2 / after x 3` | all `0` |
| `p_coalesce_rhs_temp.sp` — temp consumed inside a `??` RHS | `n 5 / after 1` | `n 5 / after 0` |
| `p_tern_arm_result.sp` — ternary arm RESULT is a temp | `b.n 5 / after 1` | unchanged (adopted — the documented boundary, §0.2) |

Also verified while baselining: **nested ternary values are wrong
on the baseline, with no temporaries involved** (`/tmp/nest1.sp`:
`true ? (false ? 3 : 7) : 0` → `0`, expected `7`;
`false ? 1 : (true ? 5 : 0)` → `0`, expected `5`; single ternary
`true ? 9 : 0` → `9`, correct). A pre-existing ternary-lowering
miscompilation, not a temp-scope issue — found, not fixed here
(see §6).

## 2. Implementation

All in `compiler/src/ir/IRBuilder.cpp` (+ declarations in
`compiler/include/sp/ir/IRBuilder.h`); no runtime changes, no new
`sp_rt_*` primitives.

### 2.1 Branch scopes

- `m_tempBranchDepth` is gone, replaced by `m_tempBranchScopes`
  (stack of vectors) with `pushTempBranchScope()` /
  `popTempBranchScope()`; `emitTempScopeReleases()` factors the
  release loop shared by `popTempScope()` and
  `popTempBranchScope()` (both discard the scope without emitting
  when the block already has a terminator).
- `recordTempCallResult` / `recordTempNewResult` record into the
  innermost branch scope while one is open, else the innermost
  statement scope; the `m_tempBranchDepth > 0` suppression is
  removed.
- `adoptTempValue` scans branch scopes innermost-outward first,
  then statement scopes.
- Sites (§0.2): `&&` / `||` RHS, `??` RHS, ternary then/else arms
  — push before the arm accept, adopt the arm result, pop before
  the branch to the merge.

### 2.2 Async scopes + spill

- The `m_inAsyncFunction` early-returns are removed from
  `pushTempScope` / `popTempScope` / `recordTempCallResult` /
  `recordTempNewResult`: async functions record temporaries like
  any other function.
- `emitAsyncSpawnSM` adopts each pointer argument value after
  the spawn (§0.5). It is the single choke point for all async
  spawns (free fn, method, generic, fn-pointer), so the transfer
  covers every async-call form including the `this` receiver
  (`loweredArgs[0]`).
- `asyncSuspendAndLoad` spills (`asyncSpillTempScopes`), restores
  (`asyncRestoreTempScopes`, at the start of `bodyBB`), and
  releases-on-cancel (`asyncReleaseSpilledTemps`, in the cancel
  block after `emitAllFunctionDrops`) — see §0.3. The spill
  array is `sp_rt_alloc`'d per suspend, its pointer lives in the
  new `AsyncSM::kTempSpillSlot` (slot 7; `kFirstParamSlot` moves
  7→8), and it is freed at the restore / in the cancel block.
  Branch scopes are discarded (not released) at a suspend —
  they must be empty there (awaits are hoisted out of branch
  regions), and a release would risk a dominance violation.
- The async `ReturnStmt` branch gains `pushTempScope()` before
  the value evaluation, `adoptTempValue(rawRetVal)`, and
  `popTempScope()` before the branch to the finish/finally block
  (§0.3); the stale comment claiming async functions never
  record temporaries is corrected.

## 3. Live proofs

All programs in `~/workspace/p8d-proofs/` (plus the Phase 7-C
proofs in `~/workspace/p7c-proofs/`), run with the slice binary
`~/workspace/build-phase8-tempscopes/bin/spc`. Baseline is the
parent binary at `ac8ff9a` (see §1 for the before numbers).

```
temp_async.sp        v 42 / after await 0                     (was after await 1)
temp_shortcircuit.sp before 0 / hit / after and 0 / or true /
                     after or 0 / tern 5 / after tern 0      (was 1 / 2 / 3)
p_async_arg.sp       inline 42 / after inline 1 /
                     spawned 42 / after spawned 2            (unchanged, by design)
p_async_arg_sync.sp  after spawn 1 / body live 1 / body n 42 (was 0 / 0 / 42)
p_await_operand_temp.sp v 42 / after await 0                 (was after await 1)
p_await_dead_temp.sp v 7 n 42 / after 0 / m 49 / after2 0     (was 1 / 2)
p_async_return_temp.sp v 42 / after await 0                  (was after await 1)
p_shortcircuit_nested.sp before 0 / nested-or /
                     after nested or 0 / nested-and /
                     after nested and 0 / x true / after x 0 (was 1 / 2 / 3)
p_coalesce_rhs_temp.sp before 0 / n 5 / after 0              (was after 1)
p_tern_arm_result.sp before 0 / b.n 5 / after 1              (unchanged, by design)
```

All other Phase 7-C proofs re-run on the slice binary and
unchanged (byte-identical to the Phase 7-C record):
`temp_arg` (`before 0 / take 42 / after 0`), `temp_member`
(`before 0 / n 5 / after 0`), `temp_stored` /
`temp_returned` (`before 0 / inside 1 / n 42 / after block 0`),
`temp_borrowed` (`base 2 / borrowed 7 / borrowed short 7 /
direct 7 / direct short 7 / after 2`), `temp_chain`
(`before 0 / chain 9 / after chain 1 / arg 9 / after arg 2`),
`temp_self` (`before 0 / self n 3 / inside 1 / after block 1 /
take 3 / after take 2`), `temp_fieldinit` (`before 0 / after
new 2 / box strong 1 / n 5`), `temp_throwpath` (`before 0 /
take 42 / caught bang / after 0`), `temp_container`
(`before 0 / after push live 1 / push n 42 / after store live
2 / store n 42 / after array live 3 / array n 42 / after
mapset live 4`), `temp_channel` (`before 0 / after send
live 1`).

## 4. Tests

`tests/codegen/TryCatchExecutionTest.cpp` — 2 renamed tests with
deliberately updated expectations + 7 new tests; no new files, no
CMakeLists changes. Targeted run: **17/17 pass** (10 of the 11
Phase 7-C temp tests — `FieldInitFactoryDoesNotDoubleCount`
doesn't match the filter — plus all 7 new tests; the full ctest
below covers the rest, including that one).

- `ShortCircuitTempsAreReleased` (renamed from
  `ShortCircuitTempsStillLeakByDesign`) — `after and 0 /
  after or 0 / after tern 0`.
- `AsyncTempIsReleased` (renamed from
  `AsyncTempStillLeaksByDesign`) — `after await 0`.
- `AsyncArgTempSurvivesAwait` — `inline 42 / after inline 1 /
  spawned 42 / after spawned 2` (survival, not release).
- `AsyncOperandTempIsReleasedAfterResume` — `v 42 /
  after await 0` (transitive capture via the spill).
- `AsyncReturnTempIsReleased` — `v 42 / after await 0`.
- `AsyncArgTempSurvivesInSyncCaller` — `after spawn 1 /
  body live 1 / body n 42` (the latent UAF fix).
- `ShortCircuitNestedTempsAreReleased` — nested `&&`/`||`
  RHS temps → all `0`.
- `CoalesceRhsTempIsReleased` — `n 5 / after 0`.
- `TernaryArmResultTempStillLeaks` — `b.n 5 / after 1`
  (pins the documented boundary: arm-result temps are adopted).

Full ctest in `~/workspace/build-phase8-tempscopes`:
**811/813 passed (99%)**. Total 813 = 806 (Phase 7) + 7 new
tests. The two failures are the known non-deterministic ones:
`NetTest.UdpSocketSendRecv` (environmental UDP EPERM) and
`JsonTest.BenchmarkOneGigabytePerSecThroughput` (CPU-contention
flake — **passed alone in 1,724 ms** on re-run). Live Postgres
`PgLiveStringArrayRoundTrip` **PASSED in 298 ms** (Postgres
was already running; no reinstall needed this session).

## 5. Files changed

- `compiler/include/sp/ir/IRBuilder.h` — `m_tempBranchDepth` →
  `m_tempBranchScopes` + `push/popTempBranchScope` +
  `emitTempScopeReleases`; `asyncSpillTempScopes` /
  `asyncRestoreTempScopes` / `asyncReleaseSpilledTemps`;
  `AsyncSM::kTempSpillSlot = 7`, `kFirstParamSlot` 7→8.
- `compiler/src/ir/IRBuilder.cpp` — temp-scope methods (§2.1),
  branch sites (§2.1), async guards removed, spawn adopt (§0.5),
  spill/restore/cancel (§2.2), async ReturnStmt scope (§2.2).
- `tests/codegen/TryCatchExecutionTest.cpp` — 2 renamed tests +
  7 new tests.
- `docs/v2/PHASE-37-SLICE-D.md` — this doc.

## 6. Deliberately not done / remaining leak sites

- **Ternary / `??` arm-result temporaries** (`p_tern_arm_result.sp`,
  `after 1`): an arm result that is itself a temporary is adopted
  at the arm exit — it is stored into the result slot and read
  after the merge, so no release point in the branch block can
  free it, and a release in the merge block would not dominate.
  Closing this needs the merged value's adopter (e.g. a
  VarDeclStmt initializer) to release the *original* SSA value —
  which the merge block does not dominate either. Same shape as
  the pre-existing problem; needs a design of its own.
- **Switch-case-value temporaries** (Phase 7-C §0.4): evaluated
  in test blocks, after the subject scope has popped — not
  recorded. The branch-scope mechanism could close this the
  same way (a scope per case value, popped in the test block);
  not attempted here — the task's scope is short-circuit /
  ternary only.
- **Async-call-argument shares** (§0.5): adopted at the spawn
  and never released — the async body borrows them and never
  drops its parameters. Closing this needs the spawn to retain
  ref arguments (or the body to drop its params at return) —
  a spawn-convention change, ARC territory.
- **Field shares of dropped temporaries, loop iterables /
  switch subjects, unresolved-call temps, container-retained
  shares, self-referential cycles, throw path** — unchanged from
  Phase 7-C §6 (all deliberate, all need ARC completion /
  destructors).
- **Residual hazard (§0.6)**: a temporary captured by a task
  that is spawned but never awaited can be released at the
  statement pop while the side task still runs. Pre-existing
  (same in sync code today) — the spawn-retain flaw, not
  introduced here.
- **Nested ternary values are wrong** (§1): pre-existing
  miscompilation, reproduced on the baseline with no temporaries
  involved. Found, not fixed — it is ternary lowering, not temp
  scopes, and needs its own slice.

## 7. Follow-ups

- The ternary / `??` arm-result leak (§6) is the next cheapest
  temp leak to close; it needs a way to release a value whose
  only post-merge use is the merged load.
- Switch-case-value temps (§6) can use the branch-scope
  mechanism directly.
- The nested-ternary value bug (§1, §6) is a correctness bug,
  not a leak — it deserves its own slice (ternary lowering).
- Spawn-time retain of ref arguments (§0.5, §0.6) is the
  ARC-completion work that closes the async-arg and
  never-awaited-side-task remainders.
- `sp_rt_strong_count` exists in the runtime as `i32(ptr)` —
  proofs declare it as `int`, not `int64` (Phase 7-C §7).
- The misleading `spc` "Unsupported backend compiler" message
  fired again while baselining: `findClangCompiler()` returned
  `/usr/bin/clang++`, whose lowercase contains the substring
  `g++`, so the GCC guard rejected a real Clang. The proofs
  only ran once the binary was invoked from its build dir (the
  direct-linker fast path with the precompiled runtime lib
  skips the clang check). Pre-existing, out of scope — but the
  substring check is plainly wrong and will bite any
  out-of-build-dir invocation.
