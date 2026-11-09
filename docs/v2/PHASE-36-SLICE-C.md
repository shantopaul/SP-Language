# Phase 36 (pro-4.0.0) — Slice C: Call-result temporary leak

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `328eee0` (Phase 6 / Phase 35 done, pushed)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, no ADR-001–005
  items touched, nothing committed or pushed)
- **Scope:** fix the call-result temporary leak Phase 6 slice C left open
  (its doc §9): a call result used only as a temporary is never dropped.
  `takeFoo(makeFoo())` and `makeBox().n` each leak 1 share.
- **Status:** DONE, uncommitted in this worktree (parent merges).
  All proofs pass; ctest 800/801 (only the environmental UDP EPERM).

**Repo AGENTS.md note:** the repo mandates a Graphify protocol
(`python -m graphify update . --force` before/after coding), but
`graphify-out/` does not exist in this worktree and `python -m
graphify` is not installed (verified 2026-11-10, as in Phases 5–6).
It cannot be run; nothing about it is fabricated here.

## 0. Design — ownership rule (written before the implementation)

### 0.1 What is OWNED (+1) and what is BORROWED

The rule reuses Phase 6-C's `isFreshPlusOneExpr` exactly, narrowed to
ARC objects:

- **OWNED (+1)** — a value produced by `new` (class object), or by a
  call that sema resolved to a **non-async user function/method**
  returning a **Class/Interface** type (`semaType->isReferenceType()`).
  The producer's share is owned by whoever receives it. Receivers that
  adopt such a value (variable init, assignment, return, throw payload,
  field init) take it **without retaining again**.
- **BORROWED** — everything else: variable loads, field loads,
  parameters, `this`, scalars, strings, arrays/maps (not ARC class
  objects), results of **unresolved calls** (runtime builtins, extern
  fns, fn-pointer calls — convention unknown; the receiver retains per
  the old behaviour), and async `Task` results (lifetime owned by the
  spawn/await machinery). Borrowed values are never recorded as
  temporaries and never released by this pass.

A temporary is recorded **exactly once, at production**: in
`visit(CallExpr)` via an RAII guard whose destructor runs on *every*
exit path of the ~4000-line visitor (241 early returns make a
single-exit hook impossible), and at the tail of `visit(NewExpr)`.
The guard records `m_lastExprValue` only when the call is a +1
producer by the rule above; it dedupes by pointer identity, so one
share is never recorded twice.

### 0.2 Why releasing a temporary cannot free a borrowed value

This is the fact the whole design rests on, verified by reading the
code rather than assumed:

1. **No `<Class>_destructor` is ever generated.** `visit(NewExpr)`
   looks up `<Class>_destructor` in the module, no emitter ever
   creates one (repo-wide grep: only the lookup exists), so the
   destructor pointer passed to `sp_rt_arc_alloc` is always null.
   When the last strong share of an object is released, the object
   allocation is freed but **its fields are never released**.
2. Therefore a child object loaded from a temporary (`makeBox().child`)
   keeps its own refcount when the parent is dropped — the parent's
   death never decrements it. The child stays alive (its field share
   leaks, as before — but it stays *valid*).
3. `sp_rt_release` is null-safe and magic-checked (`get_arc_header`
   returns null for null and non-ARC pointers → no-op), so releasing a
   possibly-null temporary is safe.
4. The only way a release could free a live value is by releasing a
   share that a variable, field, return value, or throw payload owns.
   That is prevented by adopter-removal (§0.3): every adopting site
   removes the adopted value from the pending list *before* the scope
   pops.

Consequence: at a scope pop, **every remaining pending temporary may
be released unconditionally** — no alias tracking is needed. The
pass is conservative in the other direction instead: whenever the
release point cannot be proven safe, the temporary is not recorded
at all (§0.4).

### 0.3 Mechanism — per-statement temporary scopes

New state in `IRBuilder` (`compiler/include/sp/ir/IRBuilder.h`):

- `m_tempScopes`: a stack of vectors of `IRValuePtr`, one scope per
  statement (or condition) currently being lowered.
- `pushTempScope()` / `popTempScope()`: pop emits `sp_rt_release`
  for each remaining pending temporary (reverse order) in the current
  block, then discards the scope.
- `recordTempCallResult(node)` / `recordTempNewResult(node)`:
  production hooks (§0.1).
- `adoptTempValue(v)`: removes `v` from the innermost scope (scanned
  innermost-outward; a no-op when absent).

Scopes are pushed/popped in the statement visitors:

| Site | Scope |
|---|---|
| `ExprStmt` | around the expression |
| `VarDeclStmt` | around the initializer; adopter removes the init value at the retain-decision point, pop after the store |
| `ReturnStmt` (sync + deferred) | around the value; adopter removes the raw return value, pop before the return / stash |
| `ThrowStmt` | around the thrown expression; adopter removes it in the ref-type branch, pop after `emitThrowValue` |
| `IfStmt` / `WhileStmt` / `DoWhileStmt` | around the condition; pop before the conditional branch (releases land in the pre-branch / cond block, which dominates the branch targets) |
| `ForStmt` | around the condition (cond block, per iteration) and around the step (step block); the init is a statement with its own scope |
| `ForInStmt` | around the iterable evaluation; the iterable value itself is adopted first (it is live for the whole loop — see §0.4) |
| `SwitchStmt` / `MatchStmt` | around the subject; the subject value itself is adopted first (live in every test block) |
| Short-form bodies (`=> expr`, fn/method/generic) | around the body expression; adopter removes the raw return value, pop before the `ReturnInst` |
| `emitFieldInitializers` | one scope around all field initializers in a ctor; the field adopts each value |

Adopters (all remove the *raw, pre-cast* produced value — casts and
struct-loads create new SSA values, so removal uses the value captured
right after `accept()`):
`VarDeclStmt` (at the retain decision), `AssignExpr` (plain `=`
on a ref type, at the retain decision), `ReturnStmt` (sync +
deferred, next to `emitRefReturnRetain`), `ThrowStmt` (ref-type
branch, next to the retain), short-form bodies (next to
`emitRefReturnRetain`), `emitFieldInitializers` (next to its retain —
but with a different rule: the field code retains *unless* the
initializer is `new`, so only a `new` initializer is adopted; a
factory-call initializer is retained into the field and its +1 stays
pending, so the scope pop releases it — that is the `box strong 2`
→ `1` fix, §2.5; `compileStaticInitializer` follows the same rule).

### 0.3b Containers retain their elements (found while implementing)

Containers store their elements as **raw i64 bits with no share**
(`sp_rt_list_push_i64`, `sp_rt_set_add_i64`, `sp_rt_map_set_*`,
`sp_rt_array_set_i64`, `sp_rt_channel_send_i64`, ...). That makes
them a second kind of adopter — but one that does *not* take the
+1 share: it keeps the raw pointer and expects someone else to own
the object. Before this slice that someone was the leaked +1. Two
cases prove a plain adopt is wrong here:

- `let xs = [makeFoo()]; xs[0].n` and `xs.push(makeFoo());
  xs[0].n` both read fine on the baseline only because the +1 leaks.
  Releasing the temp at the statement pop would leave the container
  holding a dangling pointer.
- Worse, the *indirect* case cannot be fixed by adopting at all:
  `store(xs, makeFoo())` where `store` pushes its borrowed
  parameter — the caller's temp is released at the caller's pop, in
  a different function from the push. Adopting in the callee is a
  no-op (a parameter is never recorded).

So containers **retain** reference-typed elements instead
(`emitContainerElementRetain`: `sp_rt_retain` on the raw element
value, before the i64 cast, at every raw-bits store — push/add/
enqueue incl. PriorityQueue, `set` for Map/List/Array, array-literal
elements, indexed assignment, `Channel.send`). The container then
owns a share, and the temporary's scope pop correctly releases the
caller's +1 (count 2 → 1). The retained share itself leaks forever
(containers have no destructor/free — same as every other
container-held share today), and the same retain also fixes the
pre-existing dangling pointer when a *variable's* element outlives
the variable. Map-literal values need no retain: the sema rejects
non-integer values there.

### 0.4 Where the pass deliberately does NOT release (leaks remain)

- **Async functions.** Recording is disabled whenever
  `m_inAsyncFunction` is true. Await splits blocks: a temporary
  defined before a suspend point does not dominate the resume block,
  so a release emitted after the await would be invalid IR. Temps
  leak in `async fn` exactly as before.
- **Short-circuit branches.** A branch-depth counter suppresses
  recording inside the RHS of `&&` / `||` / `??` and inside ternary
  arms: those values are defined in a branch block that does not
  dominate the merge block, so a release in the merge block would be
  invalid IR. LHS temps (defined in the pre-branch block, which
  dominates the merge) are recorded normally. Temps in `switch` case
  values are likewise not recorded (evaluated in test blocks, after
  the subject scope has popped).
- **Values live past the release point.** The loop iterable
  (`for (x in makeList())`) and the switch/match subject are adopted
  (removed from pending) before the pop — they are live for the whole
  loop / in every test block, so dropping them would be a
  use-after-free. They leak as before.
- **Throw path.** A release emitted after a throwing call never
  executes (control leaves via the throw machinery), so a temporary
  created in a statement whose call throws leaks. Same as before.
- **Unresolved calls.** Runtime builtins, extern fns, fn-pointer
  calls: convention unknown, receiver retains (old behaviour). If
  such a call result is used as a temporary, its share leaks as
  before — it can never be freed by this pass, because the callee
  may still own it.
- **`main` never drops its own locals** (pre-existing, Phase 6-C §9)
  — untouched; temporaries in `main` *are* released by this pass
  (statement scopes apply there too).

### 0.5 What this pass does NOT change

No runtime changes (no new `sp_rt_*` primitives, no change to
`sp_rt_retain` / `sp_rt_release` / `sp_rt_arc_alloc`); no change to
the +1 return convention itself; no change to scope drops, the async
state machine, or the debugger interpreter. The change is confined
to `compiler/src/ir/IRBuilder.cpp` (+ declarations in
`compiler/include/sp/ir/IRBuilder.h`) and tests.

## 1. What was wrong

Under the Phase 6-C +1 convention, a call result used as a temporary
is never released: the callee returns +1, no receiver adopts it, and
nothing ever drops the share. Baseline measurements (parent binary at
`328eee0`, all in `~/workspace/p7c-proofs/`):

| Proof | Baseline | After this slice (expected) |
|---|---|---|
| `temp_arg.sp` — `takeFoo(makeFoo())` | `after 1` | `after 0` |
| `temp_member.sp` — `makeBox().n` | `after 1` | `after 0` |
| `temp_stored.sp` — `x = makeFoo()` | `inside 1 / after block 0` | unchanged (must not regress) |
| `temp_returned.sp` — `return makeFoo()` | `inside 1 / after block 0` | unchanged |
| `temp_borrowed.sp` — `h.getBox()` / short-form | `base 2 / borrowed 7 / after 2` | unchanged (prints must survive) |
| `temp_chain.sp` — `makeOuter().child.n` | `after chain 2 / after arg 4` | `after chain 1 / after arg 2` (outer temps dropped; inner objects keep their field share — no destructors, see §6) |
| `temp_self.sp` — `makeNode().self` | `inside 1 / after block 1 / after take 2` | unchanged counts, no crash (cycle leak is pre-existing) |
| `temp_fieldinit.sp` — field `= makeBox()` | `box strong 2` | `box strong 1` (see §2.5) |
| `temp_shortcircuit.sp` — `&&` / `\|\|` RHS + ternary arm | `after and 1 / after or 2 / after tern 3` | unchanged (leaks remain by design, §0.4) |
| `temp_throwpath.sp` — temp, then same-function `throw` | `after 1` | `after 0` |
| `temp_async.sp` — temp inside `async fn` | `after await 1` | unchanged (leak remains by design, §0.4) |
| `ret_temp_leak.sp` (Phase 6-C) | `after method temp 2 / after free temp 3` | `after method temp 2 / after free temp 2` (the method temp is a share on `h.box`, invisible in the object count) |

Also verified while baselining: a cross-function `throw` is **not**
caught by the caller's `try` — it panics (`SP Panic: bang /
Aborted`) — while a same-function string throw is caught (`caught
bang / done`). Cross-function unwinding is deferred by design
(Phase 6), so a temporary created in a statement whose call throws
cross-function leaks into an aborting process — unobservable, and
not something this pass can (or needs to) fix.

## 2. Implementation

All in `compiler/src/ir/IRBuilder.cpp` (+ declarations in
`compiler/include/sp/ir/IRBuilder.h`); no runtime changes, no new
`sp_rt_*` primitives.

### 2.1 Temp scopes and the production hooks

- `m_tempScopes` (stack of `vector<IRValuePtr>`), `m_tempScopeFunction`
  (tags the stack's owner so a stale stack can never leak across
  functions), `m_tempBranchDepth` (see §2.4).
- `pushTempScope()` / `popTempScope()`: pop emits `sp_rt_release`
  for each remaining pending temporary (reverse order) in the current
  block, then discards the scope. The pop still discards the scope
  when the block already has a terminator (no release can be emitted
  there) — the stack must stay balanced. Both are no-ops in async
  functions (`m_inAsyncFunction`).
- `recordTempCallResult(node)` / `recordTempNewResult(node)`: record
  `m_lastExprValue` only when the producer is +1 by the §0.1 rule
  (`isFreshPlusOneExpr` + reference type), the scope stack is open
  and belongs to the current function, and `m_tempBranchDepth == 0`;
  deduped by pointer identity.
- `TempCallRecorder`: an RAII guard constructed as the first line of
  `visit(CallExpr)`; its destructor records the result on *every*
  exit path (241 early returns). `visit(NewExpr)` records at its tail
  (after the ctor call, `m_lastExprValue = allocCall`).
- `adoptTempValue(v)`: removes `v` from the innermost scope that
  holds it (scanned innermost-outward; no-op when absent).

### 2.2 Scope and adopter sites

| Site | What was added |
|---|---|
| `ExprStmt` | push around the expression, pop after |
| `VarDeclStmt` | push around the initializer; `adoptTempValue(initVal)` at the retain decision (raw value); pop after the store (guarded by `hasInitializer`, so a declaration without one never pops an enclosing scope) |
| `AssignExpr` plain `=` | `adoptTempValue(rhsVal)` in the `isRefAssignment` branch (raw value: the struct load only fires for by-value structs, and a pointer-to-pointer cast is a no-op) |
| `ReturnStmt` sync + deferred | push after the async branch; `adoptTempValue(rawRetVal)` next to `emitRefReturnRetain`; pop before the return / the branch to the finally |
| `ThrowStmt` | push after the enum early-return, before the expression; adopt in the ref-type branch; pop after `emitThrowValue` (and before the two early returns after the push) |
| `If` / `While` / `DoWhile` | push around the condition; pop before the conditional branch |
| `ForStmt` | push around the condition (cond block) and around the step (step block) |
| `ForInStmt` | push at the top; the iterable value is adopted first (live for the whole loop), then pop; the range path pops before the loop starts |
| `SwitchStmt` / `MatchStmt` | push around the subject; subject adopted first (compared in every test block), then pop |
| Short-form bodies ×4 (fn, method, generic method, generic fn) | push around the body; adopt the raw return value; pop before the `ReturnInst` |
| `emitFieldInitializers` + `compileStaticInitializer` | one scope around all initializers; adopt **only** a `new` initializer (the field code retains otherwise — §2.5) |

### 2.3 Containers retain their elements (§0.3b)

`emitContainerElementRetain(expr, v)`: emits `sp_rt_retain(v)` when
the element's sema type is a reference type, at every raw-bits
store — `push`/`add`/`enqueue` (incl. PriorityQueue item),
`set` for Map/List/Array (value and key), array-literal elements,
indexed assignment, `Channel.send`. The retain goes on the raw
value, before the i64 cast. `contains`/`remove` are transient reads
and need neither a retain nor an adopt.

### 2.4 Branch-depth guard

`m_tempBranchDepth` is incremented around the RHS of `&&` / `||` /
`??` and around both ternary arms. Recording is suppressed while it
is > 0 (§0.4).

### 2.5 The field-initializer double share

`emitFieldInitializers` retains a field value *unless* the
initializer is `new`. A factory-call initializer therefore got two
shares: its +1 plus the retain. The scope pop now releases the +1,
so the field holds exactly one (`box strong 2` → `1`, proof
`temp_fieldinit.sp`). A `new` initializer transfers its +1 to the
field and is adopted instead. `compileStaticInitializer` follows
the same rule.

## 3. Live proofs

All programs in `~/workspace/p7c-proofs/`, run with the slice
binary `~/workspace/build-phase7-templeak/bin/spc`. Baseline is
the parent binary at `328eee0` (see §1 for the before numbers).

```
temp_arg.sp        before 0 / take 42 / after 0          (was after 1)
temp_member.sp     before 0 / n 5 / after 0              (was after 1)
temp_stored.sp     before 0 / inside 1 / n 42 / after block 0   (unchanged)
temp_returned.sp   before 0 / inside 1 / n 42 / after block 0   (unchanged)
temp_borrowed.sp   base 2 / borrowed 7 / borrowed short 7 /
                   direct 7 / direct short 7 / after 2        (unchanged)
temp_chain.sp      before 0 / chain 9 / after chain 1 /
                   arg 9 / after arg 2                   (was 2 / 4)
temp_self.sp       before 0 / self n 3 / inside 1 / after block 1 /
                   take 3 / after take 2                        (unchanged)
temp_fieldinit.sp  before 0 / after new 2 / box strong 1 / n 5  (was strong 2)
temp_shortcircuit.sp before 0 / hit / after and 1 / or true /
                   after or 2 / tern 5 / after tern 3         (unchanged, by design)
temp_throwpath.sp  before 0 / take 42 / caught bang / after 0   (was after 1)
temp_async.sp      v 42 / after await 1                        (unchanged, by design)
ret_temp_leak.sp   base 2 / t1 7 / after method temp 2 /
                   t2 5 / after free temp 2                  (was after free temp 3)
temp_container.sp  before 0 / after push live 1 / push n 42 /
                   after store live 2 / store n 42 /
                   after array live 3 / array n 42 /
                   after mapset live 4               (identical to baseline)
temp_channel.sp    before 0 / after send live 1       (identical to baseline)
```

Also verified while baselining (not a proof of this slice, but it
shaped the throw-path design): a same-function string throw is
caught (`caught bang / done`); a cross-function throw panics
(`SP Panic: bang / Aborted`) — cross-function unwinding is deferred
by design, so a temporary created in a statement whose call throws
cross-function leaks into an aborting process and is unobservable.
And `m.get("a")?.n` on a map-loaded class value fails with an IR
type error (`%map_get9` defined as `i64` but expected `ptr`) — a
pre-existing safe-nav bug on map values, slice A's territory, not
this slice's.

## 4. Tests

`tests/codegen/TryCatchExecutionTest.cpp` — 10 new tests, 1 updated
expectation; no new files, no CMakeLists changes. All 11 targeted
tests pass; full ctest §5.

- `CallResultTemporariesAreReleased` — arg + member temps → 0;
  stored / returned results still live until scope end.
- `BorrowedGetterTempDoesNotFreeEarly` — long-form and short-form
  borrowed getters print 7 and live counts are unchanged.
- `TempChainDropsOuterKeepsFieldShare` — `after chain 1 /
  after arg 2`.
- `SelfReferentialTempDoesNotCrash` — cycle object: `inside 1 /
  after block 1 / after take 2`, no crash.
- `FieldInitFactoryDoesNotDoubleCount` — `box strong 1`.
- `ContainerStoresRetainTempElements` — push / indirect store /
  array literal / map set: live 1/2/3/4, all reads 42.
- `ChannelSendRetainsTempElement` — `after send live 1`.
- `ShortCircuitTempsStillLeakByDesign` — `after and 1 / after or 2 /
  after tern 3` (pinned behaviour).
- `ThrowAfterTempDoesNotLeak` — `after 0` after a same-function throw.
- `AsyncTempStillLeaksByDesign` — `after await 1` (pinned behaviour).
- `BorrowedReturnSurvivesAndFactoryAssignFreesOld` (updated) —
  `live 4` → `live 3` (the `makeBox().n` temp is now dropped).

## 5. Files changed

- `compiler/include/sp/ir/IRBuilder.h` — temp-scope state
  (`m_tempScopes`, `m_tempScopeFunction`, `m_tempBranchDepth`),
  `pushTempScope` / `popTempScope` / `recordTempCallResult` /
  `recordTempNewResult` / `adoptTempValue` /
  `emitContainerElementRetain`, `TempCallRecorder` guard.
- `compiler/src/ir/IRBuilder.cpp` — +301/−1 lines: the methods
  above, the `visit(CallExpr)` guard + `visit(NewExpr)` tail hook,
  scope/adopter sites (§2.2), container retains (§2.3), branch
  guards (§2.4).
- `tests/codegen/TryCatchExecutionTest.cpp` — 10 new tests + the
  `live 4` → `live 3` update.
- `docs/v2/PHASE-36-SLICE-C.md` — this doc.

Full ctest in `~/workspace/build-phase7-templeak`: **800/801
passed (99%)**. The single failure is `NetTest.UdpSocketSendRecv`
(test #528) — the known environmental failure: the VM sandbox
returns EPERM for UDP `sendto` (`sent` is -1, expected 14; the
test's own comment documents this sandbox behaviour). Same failure
as Phase 6 (790/791). No other failures; `JsonTest.BenchmarkOneGigabytePerSecThroughput` passed.

## 6. Deliberately not done / remaining leak sites

- **Async temporaries** (`temp_async.sp`, `after await 1`): recording
  is off in `async fn`. Needs the async state machine to carry temp
  scopes across suspend points, or a per-await release — a design of
  its own.
- **Short-circuit branch temporaries** (`temp_shortcircuit.sp`,
  `after and 1 / after or 2 / after tern 3`): RHS/ternary-arm temps
  are defined in blocks that do not dominate the merge block. A fix
  needs a release in each branch block before its branch to the
  merge (per-branch pops), not a single pop at the merge.
- **Field shares of dropped temporaries** (`temp_chain.sp`, `after
  chain 1 / after arg 2`): when a temporary parent dies, its fields
  are never released (no destructors are generated), so each child
  object keeps one leaked share. Fixing this needs real destructors
  that release fields — i.e. ARC completion, out of scope for this
  slice (and for Phase 7).
- **Loop iterables and switch/match subjects**: adopted, leak as
  before (§0.4) — they are live past the release point.
- **Unresolved-call temporaries** (runtime builtins, extern fns,
  fn-pointer calls): convention unknown, never recorded, leak as
  before.
- **Container-retained shares leak forever**: every object pushed
  into a container (or sent on a channel) holds a retained share
  that nothing ever releases — containers have no destructor/free.
  The counts are exactly the old leaked-+1 counts, but the leak is
  now in the container's name. Releasing container elements needs
  container destructors (same ARC-completion work as above).
- **Self-referential cycles** (`temp_self.sp`, `after block 1`):
  a cycle keeps itself alive forever — needs cycle collection or
  weak references, out of scope.
- **Throw path**: a temporary created in a statement whose call
  throws mid-statement leaks (the release never executes). For
  same-function throws this is unreachable mid-expression (a `throw`
  is a statement, not an expression); for cross-function throws the
  process aborts anyway (§1).
- **`main` never drops its own locals** (pre-existing) — untouched.

## 7. Follow-ups

- A per-branch release for short-circuit RHS / ternary arms (§6) is
  the next cheapest leak to close; it needs a pop in each branch
  block rather than at the merge.
- Async temp scopes (§6) need the state machine to own them.
- Real destructors + container element release (§6) is the ARC
  completion that closes the chain/field-share and container-retain
  remainders — a larger design decision (ADR territory), not a
  follow-up fix.
- The `?.` IR type error on map-loaded class values (`m.get("a")?.n`,
  `%map_get9` i64 vs ptr) belongs to slice A (safe-nav completion).
- `sp_rt_strong_count` exists in the runtime as `i32(ptr)` — the
  proof declares it as `int`, not `int64` (an `int64` declaration
  fails IR verification with a signature mismatch).
