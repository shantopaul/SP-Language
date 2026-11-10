# Phase 35 (pro-4.0.0) — Slice C: Async + Debugger + Robustness Leftovers

**Branch:** `feature/pro-4.0.0` (uncommitted slice work; parent commits part-by-part).
**Base:** `0c5ebbe` (Phase 5 / Phase 34 done).
**Scope:** the seven documented leftovers from Phases 4–5 — debugger
typed-catch divergence, `Task::cancel()` not cancelling the timer,
`SemanticAnalyzer::analyze()` returning `true` for parse-error programs,
the three Phase 5-A async leak edges, the Phase 5-B `throw makeFoo()`
leak, single-statement `await` bodies, and the `listen()` stays-up
question.
**Deliberately not in this slice:** cross-function throw unwinding
(same-function-only stays documented — needs its own design phase),
`?.` flow narrowing / `!` unwrap, ADR-001–005 (ARC completion,
in-process LLVM, WASM, self-hosted compiler — still pending).
**Worktree:** built in a separate worktree
(`~/workspace/sp-language-review/sp-language-slice-c`, detached HEAD
`0c5ebbe`). Slice A of Phase 6 also touches `IRBuilder.cpp` in the main
tree, so the worktree avoids the collision. All changes are left
**uncommitted** here; nothing was pushed or merged.
**Note on the repo's Graphify protocol** (`AGENTS.md` mandates
`python -m graphify update . --force` after edits): `graphify-out/`
does not exist in this worktree and the `graphify` tool is not
installed (verified again 2026-11-09) — the protocol cannot be run,
so it was skipped, as in Phases 1–6. Nothing was fabricated.

## 0. Baseline (before any fix)

Fresh Release build in `~/workspace/build-phase6-asyncdbg` from the
slice base finished clean (exit 0). A baseline ctest run was **invalid**
and is not quoted: it reported 389/735 failed, but 386 were
`BAD_COMMAND` and 3 `Failed`, all caused by my `sp_tests` rebuild
relinking the binary while ctest was running it. The valid baseline
reference is the Phase 5 parent verification: **734/735** on this exact
tree (`0c5ebbe`) — only `NetTest.UdpSocketSendRecv` (sandbox UDP EPERM).
Lesson recorded: never rebuild while ctest runs; the final verification
ctest runs with no concurrent builds.

Failing proofs, all run against the baseline binary
(`build-phase6-asyncdbg/bin/spc`):

- `throw makeFoo()` (`~/workspace/p6c-proofs/throw_factory.sp`):
  `before 0 / caught n 5 / after throw 1 / inside block 2 / after block 2`
  — the thrown factory object AND a plain `var x = makeFoo()` in a block
  both stay alive after their scope ends. The leak is not throw-specific:
  every factory call leaks (see §5).
- Async finally-return string (`async_finally_ret.sp`): `v=by / live 0`
  — NO leak. SP strings are `malloc`'d C strings, not ARC objects, so
  `sp_rt_retain`/`sp_rt_release` on them are no-ops. See §4a.
- Cancelled-while-suspended frame (`async_cancel_frame.sp`):
  `cancelled=true / live 1` — the `Counter` local of the cancelled body
  stays alive: the frame is never freed. See §4b.
- Single-statement await (`await_single_if.sp`, `await_single_if2.sp`):
  `if (c) x = await getVal();` and `while (n > 0) n = await getVal() - 42;`
  both fail with E1000 ("must be inside a block"); the bare form
  `if (c) await Async.sleep(10);` already works. See §6.

## 1. Debugger typed catches

**What was wrong.** The Phase 4 tree-walking interpreter
(`tools/debugger/Interpreter.{h,cpp}`) stringified every thrown value
into `InterpThrow.message` and let the first `catch` win, regardless of
its type name. The compiler has had typed catches since Phase 5 slice B
(first match wins by the thrown value's static type;
`catch (Exception e)` / `catch (Any e)` are catch-alls), so a program
that behaved one way under `spc run` behaved another way under the
debugger.

**Fix.** `InterpThrow` now carries the thrown `InterpValue` alongside
the stringified message. The catch loop calls a new `typedCatchMatches`
helper that mirrors the compiler's rule using the clause's
sema-resolved type name (`CatchClause::getResolvedTypeName()`, filled
by the TypeChecker during the interpreter's own load pipeline;
unresolvable names — including the conventional `catch (Exception e)`
spelling — resolve to `"any"`): empty/`"any"` matches anything, otherwise
the thrown value's runtime type name must equal the resolved name.
A non-matching clause is skipped; no match propagates as uncaught.
Binding follows the compiler too: a catch-all binds the **stringified**
payload, a typed clause binds the **value itself** (so
`catch (int e) { e + 1 }` gives `43` for `throw 42`, not `"42" + 1`).

Documented divergences (the interpreter has no static types, only the
runtime `Kind`): `float`/`double` alias (both are `InterpValue::Float`),
`int`/`int64` alias (both `Int`), an empty array/map throw has no
element type to consult (`typeName()` is `"array"`/`"map"`) so it never
matches a typed element clause, and there is no subclass matching
(the interpreter does not support classes at all).

**Files:** `tools/debugger/Interpreter.h` (payload field + comment),
`tools/debugger/Interpreter.cpp` (throw site, `typedCatchMatches`,
catch loop + binding).

**Tests.** `tests/debugger/DapServerTest.cpp`: the old
`TryCatchCatchesThrow` comment (which documented first-catch-wins) was
corrected; 3 new DAP tests — `TypedCatchSkipsNonMatchingClause`
(string clause skipped, int clause binds `e` as an int → `43`),
`TypedCatchStringSkipsIntClause`, `TypedCatchNoMatchPropagatesAsUncaught`
(no match → `Uncaught throw`, nothing printed). DAP typed-catch tests:
**4/4 pass**.

## 2. `Task::cancel()` cancels the timer

**What was wrong.** A timer-backed task stored its timer id in
`Task::m_timerId` but held no `Scheduler` reference, so `cancel()`
could not cancel the timer. The stale timer fired later and completed
the already-cancelled task (a no-op) — but the scheduler still had work
until the deadline, so a later `runUntilTasksIdle` waited it out. In
Phase 5-A one test took 4719 ms on a stale 5000 ms timer.

**Fix.** A continuation hook, registered at spawn: each of the four
timer factories in `runtime/src/net/EventLoop.cpp`
(`sleepTask`, `delayedValueTask`, `delayedValueStrTask`,
`delayedValueDblTask`) registers
`task->onComplete([&sched, id](Task& t){ if (t.wasCancelled()) sched.cancel(id); })`
after scheduling. It only cancels when the task was cancelled (a
normally completed task's timer already fired, so there is nothing to
cancel). The constraint is documented on `Task::cancel()`: a task must
not be cancelled after its scheduler is destroyed (the hook captures
the scheduler by reference). `Scheduler` gained a `mutable std::mutex
m_cancelMtx` guarding `m_cancelledTimers`, locked in `cancel()`,
`fireDueTimers()`, and `msUntilNextTimer()` — `cancel()` may run off
the loop thread from `Task::cancel()`; the mutex is never held across
a Task call, so there is no lock-ordering issue.

**Files:** `runtime/src/net/EventLoop.cpp`, `runtime/include/sp/runtime/Async.h`.

**Tests.** `tests/runtime/AsyncTest.cpp`: 3 new tests —
`CancelledTaskCancelsItsTimer`, `CancelledSleepAndValueTasksCancelTheirTimers`
(sleep + Str + Dbl factories), `CompletedTimerTaskLeavesNoPendingTimer`
(control). Written failing-first: the two new tests **failed** on the
unfixed runtime (10 s total — 5 s per test waiting out stale timers)
and pass after the fix. All 5 cancel-related AsyncTest tests pass in
**10 ms** total.

## 3. `SemanticAnalyzer::analyze()` contract

**What was wrong.** `analyze()` returned `errorCount ==
errCountBefore` — it only reported whether sema added NEW errors. So
it returned `true` for a program that failed to parse (as long as sema
added nothing), and callers that checked only the return value would
proceed to codegen on a broken AST. The worst such caller was
`tools/spm/BuildOrchestrator.cpp:441` (checks the return only).
Most other callers already also checked `diags.hasErrors()`
(`tools/spc/main.cpp`, `tools/debugger/Interpreter.cpp`,
`tools/test/TestRunner.cpp`, `tests/codegen/ExecutionTest.cpp`).
`tools/spls/LspServer.cpp` and `SymbolIndex.cpp` ignore the return
value entirely (they use the side effects).

**Fix (contract decided: return false).** The passes still run exactly
as before — their side effects (symbol tables, resolved types) are what
the LSP and SymbolIndex use — but the boolean now means "this program
is clean": `return m_diagnostics.errorCount() == 0;` (diagnostics only
accumulate, so this is exactly "the input was clean and sema added
nothing"). The contract is documented on the declaration in
`SemanticAnalyzer.h`. This also fixes the BuildOrchestrator hole: a
parse-error program now fails the build instead of reaching IRBuilder.

**Files:** `compiler/src/sema/SemanticAnalyzer.cpp`,
`compiler/include/sp/sema/SemanticAnalyzer.h`.

**Tests.** `tests/sema/BrokenProgramTest.cpp`: all **26** broken-program
tests had their comment ("its boolean is NOT asserted … it is true when
the parse errors are the only errors") replaced with
`EXPECT_FALSE(r.analyzeOk)` — the new contract is now pinned. The 3
valid-program controls still assert `EXPECT_TRUE(r.analyzeOk)`.
BrokenProgramTest: **29/29 pass**.

## 4. Async leak edges

### 4a. String retain/release in async finally-return — not a leak

**Finding: no code change.** The suspected edge was a retain/release
imbalance for strings returned from an `async fn` through a `finally`.
Measured with `async_finally_ret.sp`: `v=by / live 0` — no leak.
The reason is structural: SP strings are `malloc`'d C strings
(`sp_rt_str_concat` uses plain `malloc`; `alloc_str` likewise), **not**
ARC objects, so `sp_rt_retain`/`sp_rt_release` on them are no-ops
(`get_arc_header` returns null without the ARC magic). The
retain/release pair in `asyncSMEnd`/`visit(ReturnStmt)` is harmless
but misleading; it was left alone (removing it is cosmetic churn with
its own regression risk). Recorded here so the next reader does not
re-investigate it.

### 4b. Cancelled-while-suspended frame leak

**What was wrong.** `asyncSuspendAndLoad` registered the resume only
on the awaited task (`sp_rt_async_task_on_complete_resume`). If the
own task was cancelled while suspended, the awaited task might never
complete, the continuation never fired, and the heap frame leaked —
one frame per such task (plus its locals: the proof showed the
`Counter` local staying alive, `live 1`).

**Fix (two parts).**
1. **Resume on both tasks.** `sp_rt_async_task_on_complete_resume`
   takes a 5th argument, the own task (loaded from frame slot 1 at the
   suspend site), and registers the resume on **both** tasks sharing
   one `std::shared_ptr<std::atomic<bool>>` guard — whichever fires
   first posts the resume to the scheduler, the other is a no-op. So
   exactly one resume runs: a later completion of the awaited task
   cannot resume an already-freed frame, and a cancel after a normal
   resume is a no-op. The resume still always posts (never calls
   synchronously). Either task may be null; only `resumeFn` is
   required. Decls updated in `compiler/src/ir/IR.cpp` and
   `runtime/include/sp/runtime/Runtime.h`; `tests/ir/IRBuilderTest.cpp`
   assertions on the call text (`call void
   @sp_rt_async_task_on_complete_resume(ptr…`) still match with 5 args.
2. **Cancel block.** The continuation's `wasCancelled` branch now
   goes to a new `cancelBB` (instead of straight to `finishBB`) that
   emits `emitAllFunctionDrops()` for the refcounted scopes live at
   the suspend point — the body is abandoned, so its locals die here
   (the normal return path's drops never run for them). The drops are
   emitted from `m_refCountedScopes` as it stands at suspend time,
   which is exactly the live set; the allocas are frame-slot GEPs
   created in the dispatcher block, so they dominate this block. Then,
   if the awaited task is a temporary created by this await
   (`destroyTask`), it is **cancelled** (which also cancels its timer
   — no stale timer left behind) and **destroyed** (safe: the timer
   callback holds a `weak_ptr`, so a pending timer becomes a no-op).
   `cancelBB` then branches to `finishBB`, which completes the task
   (a no-op when cancelled) and frees the frame. Cancelled bodies
   still skip their `finally` blocks by design — preserved, not changed.

**Files:** `runtime/src/net/NetBridge.cpp` (+ `<atomic>` include),
`runtime/include/sp/runtime/Runtime.h`, `compiler/src/ir/IR.cpp`,
`compiler/src/ir/IRBuilder.cpp` (`asyncSuspendAndLoad`).

**Tests.** `tests/runtime/AsyncTest.cpp`: the existing
`OnCompleteResumePostsToScheduler` was updated to the 5-arg call
(own task `nullptr`), plus 2 new tests —
`OnCompleteResumeFiresOnceOnOwnTaskCancel` (cancel own → 1 resume;
awaited completing later → still 1) and
`OnCompleteResumeFiresOnceOnAwaitedComplete` (the mirror order).
`tests/codegen/ExecutionTest.cpp`: 1 new test,
`AsyncCancelWhileSuspendedReleasesFrame` (cancel a worker suspended on
a 2000 ms sleep; the `Counter` local must be gone). All pass.
Live proof (`async_cancel_frame.sp`): `cancelled=true / live 0`
(was `live 1`).

### 4c. Cancelled `all()` leaks the results array

**What was wrong.** `allTasks` allocates a results array
(`sp_rt_array_alloc`) for Int/Str/Dbl kinds. On the success path
`completeAllResult` hands it to the result task via `completePtr`
(the SP side owns it). On the cancel path (`anyCancelled`) it just
called `st->result->cancel()` and returned — the array, and for Str
any slots already copied in via `copyStrOwned` (`sp_rt_alloc`), were
never freed. One leaked array per cancelled `all()`.

**Fix.** The cancel branch of `completeAllResult` now frees the array
(and, for Str, every non-null slot) before cancelling the result task.
`CombinatorState` gained an `inputCount` field (set in `allTasks`);
the array is `calloc`-zeroed by `sp_rt_array_alloc`, so unfilled slots
are null and the free loop is safe.

**Files:** `runtime/src/net/EventLoop.cpp`.

**Tests.** `tests/runtime/AsyncTest.cpp`: 1 new test,
`CAbiAllStrCancelledFreesResults` (one Str input completes first so
its slot is filled, the other is cancelled; the combined task still
completes cancelled with a null array). Passes.

**Honest caveat.** `sp_rt_free` does **not** decrement
`g_allocated_bytes` (only ARC release paths do), so
`sp_rt_mem_allocated_bytes()` cannot observe this free — the fix is
verified by the behaviour test + code inspection, not by a memory
counter. An ASan/LSan harness linking the built static runtime lib
would make it directly observable; that was not built in this slice.

## 5. `throw makeFoo()` — the factory-call leak

**What was wrong — bigger than the one-line symptom.** The baseline
proof showed the leak is not throw-specific: `var x = makeFoo()` in a
block also stayed alive (`after block 2`). Root cause: **both sides
retained**. The callee retained a ref-typed value on return AND the
receiver retained it on receipt, so every factory call leaked one
share. Each site had the same shape — "retain unless directly `new`":
`visit(ReturnStmt)` (2 paths: plain + deferred), `visit(VarDecl)`,
`visit(AssignExpr)`, `visit(ThrowStmt)`.

**Fix — the +1 return convention.** A function/method returns a
ref-typed value as a **fresh +1 share**: a value that is already +1
passes through untouched; a borrowed value (a local, field, or
parameter) is retained on the way out. The receiver adopts the +1
share without retaining again. Implemented as two helpers in
`IRBuilder`:
- `isFreshPlusOneExpr(expr)`: true for `NewExpr`, and for a `CallExpr`
  **only when sema resolved it to a non-async user function/method**
  (every SP function follows the convention). Calls that do not resolve
  (runtime builtins, extern fns, fn-pointer calls — their ownership
  convention is unknown) and async calls (a Task's lifetime is owned
  by the spawn/await machinery, not by this convention) keep the old
  behaviour: the receiver retains. That may leak one share for such a
  call; it can never free a value the callee still owns.
- `emitRefReturnRetain(expr, retVal)`: emits `sp_rt_retain` for a
  ref-typed return value unless it is already +1.

Applied at 8 sites: the two `ReturnStmt` paths now call
`emitRefReturnRetain` (replacing their inline `isRefRet` blocks);
`VarDecl`, `AssignExpr`, and `Throw` use `isFreshPlusOneExpr` in place
of their `NewExpr`-only exception; and the four short-form return
paths (free fn, method, generic method, generic fn — which previously
never retained at all) now call `emitRefReturnRetain` too. That last
part is load-bearing: without it, a short-form getter returning a
borrowed field would hand the caller a borrowed pointer that the
receiver now adopts as +1. Async returns are untouched (async payload
kinds are void/int/bool/string/double/array/struct — class objects
cannot be returned from an `async fn`).

**A wrong turn, found by the tests.** The first end-to-end proof
(`return_convention.sp`) segfaulted. Bisecting showed a borrowed
field getter returning null — not a compiler bug at all: `init()` in
SP is a regular method called explicitly (`h.init()`), not an
auto-run constructor, and the proof never called it. The proof was
fixed; the compiler was not. (Also confirmed in passing: `main` never
drops its own locals — its implicit return emits no scope drops — a
pre-existing leak, not touched here.)

**Files:** `compiler/src/ir/IRBuilder.cpp`,
`compiler/include/sp/ir/IRBuilder.h`.

**Tests.** `tests/codegen/TryCatchExecutionTest.cpp`: 2 new tests —
`FactoryCallThrowDoesNotLeak` (`throw makeCounter()`: `after throw 0`,
`inside 1`, `after block 0`) and
`BorrowedReturnSurvivesAndFactoryAssignFreesOld` (borrowed field getter
and short-form getter survive; re-assigning from a factory frees the
old value; `live 4` — h + h.box + b + the `makeBox().n` temporary;
see §9 for the temporary leak). Live proofs:
`throw_factory.sp` → `before 0 / caught n 5 / after throw 0 /
inside block 1 / after block 0` (was `after throw 1 / inside block 2 /
after block 2`); `return_convention.sp` → `7 / 7 / 5 / 9 / 5 / 5`
(borrowed getter, short-form getter, factory, short-form new +
mutation, reassignment, live objects).

## 6. Single-statement `await` bodies

**What was wrong.** `asyncScanNested`'s `scanBranch` scanned a
single-statement branch body with `asyncScanStmtExprs(*branch,
nullptr)` — `insert == nullptr`, so any await needing hoisting (not
first-evaluated, or more than one) hit `reportUnsupported` at E1000
("must be inside a block"). The bare form `if (c) await
Async.sleep(10);` already worked; the failing form was an await nested
inside a larger expression, e.g. `if (c) x = await getVal();` (an
assignment RHS) or `while (n > 0) n = await getVal() - 42;`.

**Fix.** `scanBranch` now wraps a non-block branch in a synthesized
`BlockStmt` (keeping the branch's location) and scans the block —
hoisted statements go into the new block's list. The pre-scan already
mutates the AST (hoisting), so wrapping is consistent, and lowering
(`visit(IfStmt)`/`visit(WhileStmt)`) already accepts a block branch, so
no lowering change was needed. Applies to if/while/do-while/for/for-in
bodies. A match-arm wrap was considered and **removed**: the parser
only ever produces Block or Expression arm bodies, so that branch is
unreachable — dead code was not left in.

**Files:** `compiler/src/ir/IRBuilder.cpp` (`asyncScanNested`).

**Tests.** `tests/codegen/ExecutionTest.cpp`: 1 new test,
`AwaitInSingleStatementBranchBody` (if-RHS, while-RHS, else-RHS).
Live proofs: `await_single_if.sp` → `if x=42 / while n=0`;
`await_single_if2.sp` → `if await ok / while await n=0` (both were
E1000 before).

## 7. `listen()` vs `wait()`

**Decision: keep `listen()` non-blocking, document `wait()`.**
`HttpServer::listen()` binds + starts worker/accept threads and
returns `bool` — it must stay non-blocking: the C++ tests
(`HttpTest`, `TlsTest`, `WssTest`, `RealtimeTest`) all call `listen()`
then use the server, and the examples use the `bound` bool. A blocking
`wait()` already exists at every layer: C++
`HttpServer::wait()` (joins the accept thread) +
`sp_rt_http_server_wait` + stdlib `HttpServer.wait()`
(`stdlib/http/http.sp:371`) + `App.wait()` and `App.run(port)` =
listen + wait (`stdlib/web/app.sp:227-237`). The gap was purely
documentation: Syntax Guide §85 and §100 taught the manual
`while (true) { Time.sleep(1000); }` loop.

**Fix (docs).** §85 and §100 now use `app.wait()` with a note:
`listen()` is non-blocking by design; `wait()` blocks until `stop()`
is called; do not use the manual sleep loop.

**Fix (real bug found while proving it).** Writing the C++ test for
"wait() returns after stop() from another thread" **hung**: `wait()`
and `stop()` both joined the accept thread, and joining the same
`std::thread` twice is undefined behaviour (it hung in practice).
Fixed with `m_acceptJoinMtx` (`runtime/include/sp/runtime/Http.h`):
both `wait()` and `stop()` take the mutex around the accept-thread
join + reset, so exactly one joins and the other sees a non-joinable
thread. This cannot deadlock: `stop()` clears `m_running` (which is
what lets the accept thread exit — it polls with `acceptTimeout(100)`)
**before** taking the mutex. The destructor calls `stop()`, so it is
covered too.

**Files:** `docs/SP-Syntax-Guide.md` (§85, §100),
`runtime/src/http/Http.cpp`, `runtime/include/sp/runtime/Http.h`.

**Tests.** `tests/runtime/HttpTest.cpp`: 1 new test,
`WaitReturnsAfterStopFromAnotherThread` (server serves `/ping` while
`wait()` is blocked; `stop()` from the main thread releases `wait()`
promptly). Passes in 151 ms (it hung for 60 s+ before the mutex fix).
Live proof (`~/workspace/p6c-proofs/wait_server.sp`, `app.wait()` on
port 18099): while blocked in `wait()`, `curl /` →
`{"message": "Welcome to SP Web Framework!"}` and
`curl /api/health` → `{"status": "healthy", "version": "1.0.0"}`;
the process stayed alive until killed, then the port closed.
(`spc run` stdout is block-buffered when redirected, so the server's
own `println`s did not appear in the log file — the curl responses are
the proof, not the log.)

## 8. Verification

**Fresh build (from scratch).** The build dir was removed entirely
and rebuilt from the final source (`cmake -S … -B
~/workspace/build-phase6-asyncdbg -DCMAKE_BUILD_TYPE=Release`,
then `cmake --build . -j2`): **exit 0, 0 errors**. (An earlier
"fresh" attempt was invalid — it only removed the top-level
`CMakeFiles/`, leaving the per-directory object dirs, so it mostly
relinked. That was caught from the log line count and redone
properly.) Environment note: a VM replacement mid-verification wiped
`/tmp` + apt Clang again (as in Phase 5) and killed the first clean
rebuild at ~5%; clang 18.1.3 was recovered from the local apt cache
(`dpkg-deb -x` into `~/workspace/clang-extracted`, no apt lock
needed — the system OS-intent replay held it) and put on `PATH` for
the build + ctest. The SP build itself uses g++; only the SP
compiler's runtime codegen needs `clang`.

**Full ctest (final binaries, no concurrent builds): 748/749 passed
(99%).** The single failure is `NetTest.UdpSocketSendRecv` — the
known-acceptable sandbox UDP EPERM failure (also the only failure in
the Phase 5 parent verification). `JsonTest.BenchmarkOneGigabytePerSecThroughput`
passed this time. Test count grew 735 → 749: **14 new tests**, all
passing (verified as a group on the final build: 44/44 targeted
tests, including the 29 BrokenProgramTest tests).

**New tests by fix:** fix 1 — 3 DAP; fix 2 — 3 AsyncTest; fix 3 —
26 assertions (no new tests); fix 4b — 2 AsyncTest + 1 ExecutionTest;
fix 4c — 1 AsyncTest; fix 5 — 2 ExecutionTests; fix 6 — 1
ExecutionTest; fix 7 — 1 HttpTest.

**Live proofs re-run on the final binaries** (all match the
per-fix numbers quoted above): `throw_factory.sp` →
`before 0 / caught n 5 / after throw 0 / inside block 1 /
after block 0 / done`; `async_cancel_frame.sp` →
`cancelled=true / live 0`; `await_single_if.sp` →
`if x=42 / while n=0`; `return_convention.sp` →
`7 / 7 / 5 / 9 / 5 / 5`; `async_finally_ret.sp` →
`v=by / live 0`; `wait_server.sp` (port 18099, blocked in `wait()`)
→ `curl /` = `{"message": "Welcome to SP Web Framework!"}`,
`curl /api/health` = `{"status": "healthy", "version": "1.0.0"}`,
port closed after the process was stopped.

**Files changed (all uncommitted in the worktree):** 19 modified +
1 new doc (see `git status`): `compiler/include/sp/ir/IRBuilder.h`,
`compiler/include/sp/sema/SemanticAnalyzer.h`,
`compiler/src/ir/IR.cpp`, `compiler/src/ir/IRBuilder.cpp`,
`compiler/src/sema/SemanticAnalyzer.cpp`,
`docs/SP-Syntax-Guide.md`, `runtime/include/sp/runtime/Async.h`,
`runtime/include/sp/runtime/Http.h`,
`runtime/include/sp/runtime/Runtime.h`,
`runtime/src/http/Http.cpp`, `runtime/src/net/EventLoop.cpp`,
`runtime/src/net/NetBridge.cpp`,
`tests/codegen/ExecutionTest.cpp`,
`tests/codegen/TryCatchExecutionTest.cpp`,
`tests/debugger/DapServerTest.cpp`, `tests/runtime/AsyncTest.cpp`,
`tests/runtime/HttpTest.cpp`, `tests/sema/BrokenProgramTest.cpp`,
`tools/debugger/Interpreter.cpp`, `tools/debugger/Interpreter.h`,
and this file, `docs/v2/PHASE-35-SLICE-C.md` (450 lines). No commits,
no pushes, no merges; main untouched; version stays 3.1.0; no
`.github/` changes; no ADR-001–005 items touched. The only CMake
interaction was none — no new test files were registered (all new
tests went into existing test files), so `tests/CMakeLists.txt` was
not edited.

## 9. Follow-ups / limitations

- **Call-result temporaries still leak one share.** Under the §5
  convention, a call result used as a temporary — a call argument
  (`takeFoo(makeFoo())`), a member access (`makeBox().n`), or any
  other unbound use — is never released: the callee returns +1, the
  callee-side receiver adopts it, but a temporary has no receiver to
  adopt it. (It leaked before too — 2 shares for a call result, 1 for
  a directly-written `new` — so the convention did not make it worse;
  it is now 1 share in both cases.) Fixing it needs a temporary-release
  pass at call sites; `visit(CallExpr)` has ~235 `CallInst` emission
  points across its branches, so that is a slice of its own, not a
  drive-by in this one. Measured: `ret_temp_leak.sp` — a method-call
  temporary leaks a share (invisible in the live-object count: the
  object survives via the field anyway); a free-call temporary leaks
  the whole object (`after free temp 3` vs `base 2`).
- **Named Task handles leak their boxes.** A `Task` value held in a
  named variable is never destroyed (only temporaries go through
  `sp_rt_async_task_destroy`); the release in the scope drops is a
  no-op because a Task box is not an ARC object. Pre-existing, on every
  return path, not just the cancel path — out of scope here.
- **`main` never drops its locals.** The implicit return at the end
  of `main` emits no scope drops. Pre-existing; noted while verifying
  §5, not changed.
- **Fix 4c is not metric-verified.** `sp_rt_free` does not decrement
  `g_allocated_bytes`, so no counter observes the freed array (§4c).
- **STDLIB_REFERENCE has no HTTP section at all** (160 lines; covers
  only std/io, math, string, collections, fs, time, os/process), so
  `wait()`/`listen()` could not be documented there — only the Syntax
  Guide was updated. Adding the whole `std.http`/`std.web` reference
  is a docs task of its own.
- **Cancelled bodies skip `finally`.** Preserved by design (§4b):
  the cancel path branches to the finish block, not through finally.
- **Debugger typed-catch divergences** (documented in §1):
  float/double and int/int64 alias; empty array/map throws never match
  a typed clause; no subclass matching (no classes in the interpreter).
- **Cross-function throw unwinding, `?.` flow narrowing / `!`
  unwrap, and ADR-001–005** remain out of scope, as before.
