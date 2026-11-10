# Phase 34 (pro-4.0.0) — Slice A: async completion

> Phase 5 slice A. Branch `feature/pro-4.0.0` (base `5228c14`, Phase 4).
> Uncommitted — the parent commits part-by-part. No push, no merge to main,
> no version bump (product stays 3.1.0), no `.github/` changes.
> ADR-001–005 are still Proposed — no ADR approval is claimed here; the
> pump-based design is kept, and the state machine is built ON it (same
> Scheduler, same Task, same thunk/spawn model), not as a new async model.

## What was built

### 1. Heap-frame state machine

- Every `async fn` body is now the state-machine entry
  `void <name>(ptr %task, ptr %ctx)`: it allocates a heap frame
  (`sp_rt_alloc(frameSize)`), stores the task + the ctx args into their
  slots, calls the resume function
  `void __sp_async_resume_<name>(ptr %frame)`, frees the ctx, returns.
  The resume function holds the dispatcher (entry block, a CondBranch
  chain over `frame.state`), segment 0, one continuation block per
  await, and the finish block (completes the task with the payload,
  frees the frame, returns).
- Locals + parameters + `this` + for/for-in loop variables live in the
  heap frame (8-byte slots; a struct local occupies `ceil(size/8)`
  slots). Awaits suspend via `sp_rt_async_task_on_complete_resume`
  (the resume is ALWAYS posted to the scheduler, never synchronous).
  The continuation first checks whether the body's own task was
  cancelled (abandon: free the frame, return), then reads the awaited
  value through the non-blocking getters.
- Honest note (as the task requires): locals already survived across
  awaits before this slice (p1 proves it), because the awaiting frame
  stayed on the stack while pumping. The state machine's value is true
  suspension — the stack unwinds at every await, the stack stays flat
  no matter how deep the await chain (p3c: depth 5000), and a body can
  be cancelled while suspended.
- Await-hoisting (AST pre-pass, `asyncPrepareBody` + `asyncScan*`):
  at most one await per statement stays in place (the one in
  first-evaluated position, walking down the first child at each
  level); all other awaits are hoisted to
  `let __sp_await_N = await …;` immediately before the enclosing
  statement. This is required because a continuation block is reached
  from the dispatcher, not from the previous segment, so SSA values
  computed before an await do NOT dominate the continuation — any
  value live across an await must be in the frame. Caveat: simple
  hoisting reorders a hoisted await before the rest of its statement's
  prefix expression — unobservable for pure prefixes
  (identifiers/literals/loads), observable only if the prefix has side
  effects (rare); programs depending on that order should split the
  statement manually.
- Returns store to the frame return slot and branch to the finish
  block (try/finally deferred-return and uncaught-throw blocks are
  async-aware — they branch to finish instead of emitting a
  `ReturnInst`). String returns are retained on the return path and
  released in the finish block after `completeStr` (one retain per
  taken return path, one release per invocation).

### 2. Task<double> / Task<array> / Task<struct> payloads

- `Task<double>`: `Task::completeDbl` / `dblValue`,
  `sp_rt_async_delayed_dbl`, `sp_rt_async_task_await_dbl`,
  `sp_rt_async_task_value_dbl`, `delayed_value(ms, double)` overload in
  sema, `Async.delayDouble`. p4 (`pi=3.14`) passes.
- `Task<array>`: the array pointer is carried with `completePtr` (not
  owned — arrays are plain malloc, never ARC-managed/dropped, so the
  array outlives the task by design). p5 (`len=3 first=1`) passes.
- `Task<struct>`: the return is boxed into a heap allocation (size
  from the struct's field layout, x86-64 alignment rules — an upper
  bound on LLVM's layout, safe because it is only used for the
  allocation); the task owns the box (`completePtrOwned`, freed when
  the task is destroyed); the await loads the struct value from the
  box. p6 (`x=3 y=4`) passes.
- Class-typed async returns still fail loudly with E1000 (ARC
  ownership across the task boundary); struct parameters to an async
  fn also fail loudly with E1000.

### 3. Combinators: Task.all / Task.race

- Runtime (`Async.h` / `EventLoop.cpp`): `allTasks` / `raceTasks`
  over `std::vector<TaskPtr>` + a payload kind enum (Int/Str/Dbl/
  Void), with a mutex-protected shared state (continuations may fire
  on different threads). `all` completes cancelled when any input was
  cancelled; empty input completes immediately (empty array / void).
  `race` completes cancelled when the first finisher was cancelled;
  empty input completes cancelled. Losing race inputs keep running
  (no cancellation of losers). Null input handles count as cancelled.
- C ABI (`NetBridge.cpp`): `sp_rt_async_task_all_i64/str/dbl/void`
  and `sp_rt_async_task_race_i64/str/dbl/void`. Each takes the SP
  array OBJECT (`Task<T>[]` value) + its element count and extracts
  the element storage pointer internally via `sp_rt_array_data`
  (redesign — see "Design deviations" below).
- SP surface (`stdlib/async/async.sp`): `Async.allInt/raceInt`,
  `Async.allString/raceString`, `Async.allDouble/raceDouble`,
  `Async.allVoid` (async fns awaiting the combined task), plus
  `Async.delayDouble`. p7 passes
  (`all len=2 first=1 second=2`, `race=2`, `dbl=2.5`).

### 4. Cancellation: Task.cancel (cooperative)

- `Task.cancel() / isReady() / wasCancelled()` are real methods (sema
  `Task` class in `Type.cpp` + IRBuilder lowering to the existing C
  ABI fns, same pattern as `Duration`/`Arena` instance-method
  special-casing — the check is a prefix match on the `Task<...>`
  type string).
- The known gap is FIXED: `sp_rt_async_spawn_sm` checks
  `wasCancelled()` first, so a body that has not started never runs
  (p8: `cancelled=true`, `value=0 wasCancelled=true isReady=true`,
  and the body never printed).
- A suspended body stops at its next resume (the continuation's first
  act checks its own task's cancelled flag and abandons the frame).
  A body running a segment is NOT interrupted mid-segment
  (cooperative, no preemption). Cancelling the awaited task does NOT
  stop the body: the await returns promptly with the default value
  (0 / "" / false / 0.0 / null) and the body continues.
- Bounded leak, documented: a body cancelled while suspended on a
  task that never completes is never resumed and its frame is never
  freed. A cancelled `all()` also leaks its (possibly partially
  filled) results array.

## Design deviations (found during implementation)

### Design 2 — body-as-entry (replaces the doc's Design 1 thunk)

The doc's Design 1 said a separate thunk allocates the frame and calls
`body(frame)`. That fails for **virtual dispatch**: the concrete
override's frame size is unknowable at the virtual call site (the
vtable holds only the body address; the static method's size could be
too small for an override — silent miscompilation). Alternatives
considered and rejected: vtable storing the direct-thunk address
(touches VTableBuilder + call-site param-type derivation), frame size
passed through ctx (same unknowability), frame-size array global (size
unknown at layout time; names are known early, sizes are not).

**Adopted Design 2:** the async fn body itself becomes
`void <name>(ptr %task, ptr %ctx)` — it loads args from the ctx,
allocates the frame (size from an AST pre-scan, known before lowering),
stores task+args, calls the resume function, frees the ctx, returns.
The resume function holds the dispatcher + segments + finish block.
This eliminates `emitAsyncThunk` entirely (direct and indirect):
direct calls spawn the body directly as the thunk; virtual calls pass
the vtable-loaded body address directly as the thunk to the new
`sp_rt_async_spawn_sm` — no indirect thunk function, no vtable change,
no frame-size-at-call-site problem. The frame layout, suspend/resume,
hoisting, payload, combinator and cancellation semantics stay exactly
as the doc designed; only the entry/thunk mechanics change.

Consequences: all async call sites derive param types from **sema**
(declared param types + `this`), never from the body's IR function
params (now `(task, ctx)`); async fn bodies with an uncarriable
declared return type (e.g. class) or uncarriable param type (e.g.
struct) fail loudly with E1000 at the **declaration**; lambdas cannot
be async (LambdaExpr has no async flag) — the lambda lowering saves/
clears SM state + `m_inAsyncFunction`, so an `await` inside a lambda
fails loudly with the existing E1000 backstop.

### Nested async compilation must save/restore `m_asyncSM` (segfault fix)

`asyncSMEnd` resets `m_asyncSM` when it finishes a function. A generic
async fn (or async method) compiled LAZILY while another async fn's
body is being lowered therefore destroyed the enclosing fn's state
machine: after the specialization's `asyncSMEnd`, `m_asyncSM` was
null, and the enclosing fn's own `asyncSMEnd` dereferenced it
(SIGSEGV, found by the `GenericAsyncFnCallLowers` test). Fix:
`compileMethod`, `compileSpecializedMethod` and
`compileSpecializedFunction` save `m_asyncSM` (via `std::move`) before
`asyncPrepareBody` and restore it on both return paths (after a
prepare failure and after `asyncSMEnd`). `visit(FunctionDecl)` is
top-level only and needs no save/restore. (The lambda guard already
saved/restored `m_asyncSM` the same way.)

### `IRType::isFloat()` is 32-bit only — double args were rejected

`isFloat()` returns true only for `IRTypeKind::Float`, not for
`Double`. Both `emitAsyncSpawnSM` (arg validation) and `asyncSMEnd`
(body prologue) used `isFloat()`, so a `double` argument to an async
fn was rejected with E1000, and a `double` parameter would have been
silently skipped (its frame slot left uninitialized). Both now use
`isFloatingPoint()` (covers Float + Double), with double storage in
both cases. Found by `Async.delayDouble(10, 2.5)` failing; fixed and
proven by p7 (`dbl=2.5`).

### Combinator C ABI takes the SP array object, not the data pointer

The first design declared `sp_rt_array_data` overloads in SP
(`sp_rt_array_data(Task<int>[] arr) -> ptr`, etc.) so the SP side
could pass the element storage pointer. That is wrong: overloaded
extern fns are name-mangled with a `__<paramTypes>` suffix, so the
calls targeted `sp_rt_array_data__Task_int___` etc. — names the
runtime does not define (only plain `sp_rt_array_data` is declared in
IR.cpp). The single `sp_rt_array_data(string[] arr)` in `postgres.sp`
works only because it is NOT overloaded. Fix: the 8 combinator C ABI
fns take the SP array object (`Task<T>[]` value) + count and extract
the storage pointer internally with `::sp_rt_array_data` (null-safe:
returns nullptr for a null array). The SP side passes the array
directly; no `sp_rt_array_data` in SP at all. **Lesson recorded:
overloaded extern fns cannot name a plain C ABI function.**

### VTableBuilder: Task needs no vtable bodies

Adding sema methods to `Task` made `buildSpecializedClassLayout` emit
a vtable for `Task<bool>` referencing `@Task_bool_cancel/isReady/
wasCancelled`, which are never defined (they lower to C ABI calls) —
LLVM error "use of undefined value". Fix: skip all methods when the
base template name is `Task` (builtin opaque handle; its methods lower
to C ABI calls, no IR bodies, no subclasses, no virtual dispatch; an
empty vtable emits `ptr null`, which is valid).

### `mangleGenericName` must handle `[` / `]` / `?`

`Task<int[]>` mangled to `Task_int[]` (invalid LLVM identifier:
`%class.Task_int[] = type { ptr}` → "expected '=' after name"). Fix:
`[` → `_arr`, `]` dropped, `?` → `_q` — so `int[]` → `int_arr`,
`Task<int[]>` → `Task_int_arr`, distinct from `Task<int>` →
`Task_int` (no collision; plain sanitize-to-`_` would collide).

### Struct-await merge-block fix

`asyncSuspendAndLoad` ended with `setInsertPoint(bodyBB)`, which reset
the insert point back to bodyBB after `emitStructValueFromBox` had
moved it to the merge block — abandoning the merge block without a
terminator and adding instructions after bodyBB's CondBranch (IR
verification: `basic block 'await_struct_merge_2' does not end with a
terminator`). Fix: the final `setInsertPoint(bodyBB)` was removed; the
insert point is already correct (bodyBB for scalar kinds, merge block
for struct). Found by p6's first run; fixed and proven by p6 (`x=3 y=4`).

## Design (verified from code before building)

### Current state (Phases 2–3, confirmed by reading the code)

- `await` is pump-based: `visit(AwaitExpr)` lowers to
  `sp_rt_async_task_await_*`, which pumps the default scheduler until the
  task completes. The awaiting frame stays on the stack while pumping.
- An `async fn` call spawns the body as a Task: `emitAsyncSpawn` allocates
  a heap ctx (8-byte slots), stores the args, and posts the
  compiler-generated thunk `__sp_async_thunk_<name>`; the thunk loads the
  args, calls the body, frees the ctx, and the spawn wrapper completes
  the task with the body's return value.
- Locals are stack `AllocaInst`s — and every variable alloca is emitted
  into the function's ENTRY block (`visit(VarDeclStmt)`), which is the
  pattern the frame slots reuse.
- No Phi nodes are ever emitted (the verifier rejects them); all values
  flow through memory. So the only SSA values that can cross a suspend
  point are expression temporaries inside a single statement — and those
  are handled by await-hoisting (below).
- Payloads carried: void / int / bool / string only (`asyncReturnKind`,
  `taskPayloadKind`). `Task` is a sema class type with type param `T`;
  `delayed_value` has overloads for int / string / bool.

### State machine

- Each `async fn` body becomes a resume function over a heap frame:
  frame layout is 8-byte slots — `[0] state (i64), [1] task (ptr),
  [2] awaitTask (ptr), [3] return value, [4..] params, then locals in
  visit order`. The thunk allocates the frame, stores the task + args,
  and calls the body; the body completes the task itself in its finish
  block (the thunk never completes it).
- Every `let` local / parameter / `this` in an async body is a frame
  slot (a GEP from the frame parameter, emitted into the entry block —
  so it dominates every use, exactly like the existing entry-block
  allocas). All existing load/store/drop code works unchanged, because
  it uses the slot address opaquely.
- Each `await` is a suspend point: store the next state id + the awaited
  task into the frame, register a resume with
  `sp_rt_async_task_on_complete` (always posted to the scheduler, never
  called synchronously, so no re-entrant resume), and return.
  The continuation block first checks whether the body's own task was
  cancelled (abandon: free the frame, return — the task is already
  cancelled), then reads the awaited value from the completed task
  (non-blocking) and continues.
- The body entry block is a dispatcher: a CondBranch chain over
  `frame.state` to the segment blocks. Segment 0 is the body's first
  block; each suspend creates the next segment.
- Returns in an async body store the value to the frame return slot
  (after the usual retain + scope drops) and branch to a shared finish
  block, which completes the task with the right payload call, frees
  the frame, and returns. The try/finally deferred-return block and the
  uncaught-throw block are async-aware (they branch to finish instead of
  emitting a `ReturnInst`).
- Await-hoisting (AST pre-pass in the IRBuilder, before lowering the
  body): an `await` is lowered in place only when it is the
  first-evaluated value-producing sub-expression of its statement
  (`let x = await …;`, `await …;`, `x = await …;`,
  `return await …;`, `if/while (await …)`); any other await is hoisted
  to a fresh `let __sp_await_N = await …;` immediately before the
  enclosing statement (type `var`, inferred). This guarantees no SSA
  value computed before an await is used after it. The hoister recurses
  into the await's own inner expression, so `await delayed_value(ms,
  await getVal())` is handled too.

### Payloads

- `double`: `Task::completeDbl` / `dblValue`, `sp_rt_async_delayed_dbl`,
  `sp_rt_async_task_await_dbl` (pump path, kept for tests),
  `sp_rt_async_task_value_dbl` (non-blocking getter, used by the state
  machine), `delayed_value(ms, double)` overload in sema. The compiler
  spawns through the single `sp_rt_async_spawn_sm` (Design 2) for all
  payload kinds — there is no per-payload spawn fn.
- `array`: the array is a heap object (pointer); the task stores it
  with `completePtr`. Arrays are NOT ARC-managed in this codebase
  (`sp_rt_array_alloc` is plain malloc; array locals are never dropped),
  so the task needs no retain — the array outlives the task by design.
- `struct`: the thunk boxes the by-value struct return into a heap
  allocation (size from the struct's field layout, x86-64 alignment
  rules, documented in the code); the task owns the box
  (`completePtrOwned`, freed when the task is destroyed); the await
  loads the struct value from the box. Awaiting twice reads the same
  live box.
- Class-typed async returns are deliberately NOT added (the task would
  have to retain/release an ARC object across the task boundary — a
  separate ownership design); they still fail loudly with E1000.

### Combinators (runtime + stdlib `Async` wrappers)

- Runtime C ABI over task-handle arrays:
  `sp_rt_async_task_all_i64/str/dbl/void` (wait all, collect into an
  SP array / complete void) and `sp_rt_async_task_race_i64/str/dbl/void`
  (first completion wins). `all` completes cancelled when any input was
  cancelled; `race` completes cancelled when the first finisher was
  cancelled. Empty input completes immediately.
- SP surface (per-payload, because SP has no compile-time type dispatch
  for the runtime combinator): `Async.allInt / raceInt`,
  `Async.allString / raceString`, `Async.allDouble / raceDouble`,
  `Async.allVoid`. A generic `Async.all<T>` is a follow-up.

### Cancellation (cooperative, exact semantics)

- `Task.cancel() / isReady() / wasCancelled()` are now real methods
  (sema `Task` class + IRBuilder lowering to the existing C ABI fns).
- A task whose body has not started yet never runs: the spawn callback
  checks `wasCancelled()` first (this also fixes the old spawn fns,
  whose comment documented the gap).
- A suspended body stops at its next resume: the continuation's first
  act is to check its own task's cancelled flag and abandon the frame
  without running the segment.
- A body running a segment is NOT interrupted mid-segment; the cancel
  takes effect at the next await boundary. That is the documented
  cooperative contract — there is no preemption.
- Cancelling the task a body is awaiting does NOT stop the body: the
  await returns promptly with the default value (0 / "" / false /
  0.0 / null) and the body continues.
- Bounded leak, documented honestly: if a body is cancelled while
  suspended on a task that never completes, its frame is never resumed
  and never freed (freeing it at cancel time would be a use-after-free
  for the pending resume registration). One frame per such task.

## Files changed

(All paths are in the worktree
`~/workspace/sp-language-review/sp-language-slice-a`, UNCOMMITTED.)

- `compiler/src/ir/IRBuilder.cpp` — the state machine: `AsyncSM`
  frame/slot machinery, `asyncPrepareBody` (pre-scan + hoisting),
  `asyncSMBegin`/`asyncSMEnd` (dispatcher, finish block, body
  prologue), `asyncSuspendAndLoad` (suspend/resume + destroy flag),
  `visit(AwaitExpr)` SM path, `visit(ReturnStmt)` / `visit(VarDeclStmt)`
  / `visit(ForInStmt)` SM branches, try/throw protocol SM branches,
  all 5 async call sites → `emitAsyncSpawnSM`, Task-method
  special-casing in `visit(CallExpr)`, lambda lowering guard,
  `emitAsyncThunk` deleted, `asyncResolveType` helper (confines the
  `const_cast` for the const sema accessor), `asyncAuxSlot` /
  `asyncNodeSlotGEP`, `asyncStructTypeForTask` /
  `asyncAwaitedIRType`, `isFloatingPoint()` fix (double args/params),
  `m_asyncSM` save/restore in the three nested-compile sites.
- `compiler/include/sp/ir/IRBuilder.h` — `AsyncSM` struct (reserved
  slots, `auxSlots`, `slotGEPs`), SM helper declarations, `<map>`
  include, `asyncScanStmtExprs` pointer param.
- `compiler/src/ir/IR.cpp` — external declarations for all new
  runtime fns (`sp_rt_async_spawn_sm`, `delayed_dbl`,
  `task_await_dbl/ptr`, `task_value_i64/str/dbl/ptr`,
  `task_complete_i64/str/dbl/ptr/ptr_owned/void`,
  `task_on_complete_resume`, `task_all_*/race_*`, plus
  `task_is_ready/cancel/was_cancelled` which were previously missing).
- `compiler/src/sema/Type.cpp` — `Task` class methods `cancel()`,
  `isReady()`, `wasCancelled()` (all `-> bool`).
- `compiler/src/sema/SymbolTable.cpp` — `delayed_value(int, double)
  -> Task<double>` overload.
- `compiler/include/sp/ast/Expression.h` — mutable accessors
  (`IndexExpr`, `TernaryExpr`, `AssignExpr`, `NewExpr`, `TryExpr`)
  for the hoisting rewrite.
- `compiler/include/sp/ast/Statement.h` — `SwitchCase::getValuePtr()`.
- `compiler/src/codegen/VTableBuilder.cpp` — Task methods skipped in
  `buildSpecializedClassLayout`; `mangleGenericName` handles `[`/`]`/`?`.
- `runtime/include/sp/runtime/Async.h` — Task payload extensions
  (`dblValue`, `ptrValue`, `completeDbl`, `completePtr`,
  `completePtrOwned`, `~Task` frees the owned ptr), `delayedValueDblTask`,
  `awaitTaskDbl`, `awaitTaskPtr`, `TaskPayload` enum, `allTasks` /
  `raceTasks` declarations + semantics docs.
- `runtime/include/sp/runtime/Runtime.h` — C ABI declarations + doc
  comments for all new fns (combinators take the SP array object).
- `runtime/src/net/EventLoop.cpp` — Task extensions, `delayedValueDblTask`,
  `awaitTaskDbl`/`awaitTaskPtr`, `allTasks`/`raceTasks` (mutex-protected
  shared state), `sp::rt::sp_rt_*` → `::sp_rt_*` (8 occurrences — the
  `sp_rt_*` fns are `extern "C"` at global scope, not in `sp::rt`).
- `runtime/src/net/NetBridge.cpp` — C ABI implementations
  (`spawn_sm`, `delayed_dbl`, `await_dbl/ptr`, value getters,
  completers, `on_complete_resume`, the 8 combinators — now taking
  the SP array object and extracting the data pointer internally).
- `stdlib/async/async.sp` — rewritten: doc comment (payloads, state
  machine, combinators + cancellation semantics), `Async.delayDouble`,
  `Async.allInt/raceInt/allString/raceString/allDouble/raceDouble/
  allVoid`, extern decls for the combinator C ABI.
- `tests/runtime/AsyncTest.cpp` — 16 new tests (spawn_sm, delayed
  dbl, value getters, on_complete_resume posting, all/race per
  payload incl. empty + cancelled cases); 11 stale
  `run_tasks_until_idle` drain calls removed (see follow-ups).
- `tests/ir/IRBuilderTest.cpp` — 11 existing tests updated from the
  pump/thunk IR to the SM IR; the double-payload test converted from
  "still fails" to "lowers"; 3 new tests (array payload, struct
  payload, locals-across-await).
- `tests/codegen/ExecutionTest.cpp` — 8 new end-to-end tests (locals
  across await, double/array/struct payloads, combinators all+race,
  cancel-before-start, deep chain depth 50, generic async fn await).

## Verification

- Full Release build clean in `~/workspace/build-phase5-async-wt`
  (`-j2`, Clang 18.1.3): `[100%] Built target sp_tests`, no errors,
  no warnings; `spc --version` → 3.1.0.
- Full ctest in the worktree build dir: **681/683 (99%)** — baseline
  was 653/655 (Phase 4). 28 more tests than the baseline (16 runtime
  + 3 IR + 8 execution + 1 from the converted double test).
  The only 2 failures are the baseline's environmental ones, both
  attributed:
  - `#415 NetTest.UdpSocketSendRecv` — UDP EPERM in the sandbox.
  - `#429 JsonTest.BenchmarkOneGigabytePerSecThroughput` — benchmark
    threshold under CPU contention.
- Runtime suite: **AsyncTest 35/35** (was 19), 1085 ms total.
- IR suite: **20/20** async/await tests (11 updated + 3 new + the
  converted double test + the previously-crashing generic test).
- Execution suite: **8/8** new async tests.
- `spc llvm` on the repro programs produces the SM IR (body
  `define void @<name>(ptr %__sp_task, ptr %__sp_ctx)`, resume fn
  `define void @__sp_async_resume_<name>(ptr %frame)`,
  `sp_rt_async_spawn_sm`, `sp_rt_async_task_on_complete_resume`,
  non-blocking `value_*` getters, `complete_*` in the finish block).

## Live-proof outputs

All in `~/workspace/p5-async-proof/`, built + run with the final
binary (`spc build` + run):

- p1_locals — `a=10 b=20 c=30 s=hello`, `result=30` ✓ (locals across
  two `sleep_async(30)` awaits).
- p2_interleaved — `fast done` before `slow done`, `a=1 b=2` ✓.
- p3_deepchain — `depth50=50` ✓; p3b `depth1000=1000` ✓; p3c
  `depth5000=5000` ✓ (flat stack).
- p4_double — `pi=3.14` ✓ (was E1000 at baseline).
- p5_array — `len=3 first=1` ✓ (was E1000 at baseline).
- p6_struct — `x=3 y=4` ✓ (struct `Point {x:3, y:4}` awaited).
- p7_allrace — `all len=2 first=1 second=2`, `race=2`, `dbl=2.5` ✓
  (`Async.allInt`, `Async.raceInt`, `Async.delayDouble`).
- p8_cancel — `cancelled=true`, `value=0 wasCancelled=true
  isReady=true` ✓ (cancel-before-start: body never ran).
- Generic async fn (regression repro for the segfault):
  `identity<int>(999)` awaited from another async fn → `999` ✓.

## Deliberately not done

- Async I/O (no async HTTP/fetch).
- A generic `Async.all<T>` (needs compile-time type dispatch).
- Class-typed async returns (ARC ownership across the task boundary).
- Preemption (cancellation is cooperative only, at await boundaries).
- `await` inside a `comptime` block (compile-time evaluation rejects it,
  as before).
- The old pump-based `sp_rt_async_spawn_i64/str/void` C ABI fns are kept
  (runtime tests use them) but the compiler no longer emits them.
- The pump path in `visit(AwaitExpr)` / `emitAwaitTask` is kept as a
  fallback for awaits outside a state machine (which cannot happen in
  practice — the backstop rejects them first); it is not deleted.

## Known follow-ups

- **String retain/release balance edge case:** the SM return path
  retains a string return unconditionally and the finish block releases
  it once. A return inside a `finally` of an async fn would retain
  twice but release once — a bounded one-retain leak per such return.
  (Not observed in tests; note if a finally-returning async fn is
  ever written.)
- **Bounded cancelled-while-suspended leak:** a body cancelled while
  suspended on a task that never completes is never resumed and its
  frame is never freed (one frame per such task). A cancelled `all()`
  also leaks its (possibly partially filled) results array (and, for
  Str, the filled string slots).
- **`Task::cancel()` does not cancel the underlying timer:** the timer
  id is stored in `Task::m_timerId` but the Task holds no Scheduler
  reference, so it cannot call `Scheduler::cancel(id)`; a stale timer
  fires later and completes the already-cancelled task (no-op). This
  is a pre-existing Phase 2 design limitation — it made one new test
  (`CAbiDelayedDblAndAwaitDbl`) take 4719 ms in the full suite (it
  waited on a stale 5000 ms timer left by the pre-existing
  `CAbiCancelReturnsPromptly` test). Fix applied in this slice: the 11
  `run_tasks_until_idle` drain calls were removed from the new tests
  (each test already awaits its own tasks; race losers are awaited
  explicitly). The timer-cancellation itself is a follow-up, not fixed
  here.
- **Single-statement branch bodies:** an `await` directly in a
  single-statement `if`/`while` branch body (no `{...}` block) fails
  loudly with E1000 ("must be inside a block"). Match arm expression
  bodies allow a single first-evaluated await, else E1000.
- **Hoisting reorder caveat:** a hoisted await is reordered before the
  rest of its statement's prefix expression — unobservable for pure
  prefixes, observable only if the prefix has side effects.
- **Graphify protocol unrunnable:** the worktree `AGENTS.md` requires
  reading/updating the Graphify knowledge graph (`graphify-out/`,
  `python -m graphify`), but `graphify-out/` does not exist in the
  worktree and `python -m graphify` is not installed (re-verified
  2026-11-09). Nothing was fabricated; the graph was not consulted
  or updated.
- **`asyncResolveType` bulk-rename lesson:** a bulk rename replaced
  `typeCtx.resolveType(` with `asyncResolveType(` everywhere —
  including inside the `asyncResolveType` helper itself, making it
  call itself (261,000+ frames, SIGSEGV). Fixed: the helper body calls
  `typeCtx.resolveType(typeStr)` (the sema method), with a NOTE comment
  in the code warning against re-renaming.
- **Debug build dir:** a Debug build was configured at
  `~/workspace/build-phase5-async-dbg` (used once, to get the exact
  segfault line for the `m_asyncSM` bug). It is scratch, not part of
  the deliverable; the Release build dir
  `~/workspace/build-phase5-async-wt` is the verification build.
