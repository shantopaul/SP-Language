# Phase 32 (pro-4.0.0) — Language Depth, Part 1

> Branch `feature/pro-4.0.0`. Production quality. Branch-only push, never
> merge to main. Product version stays **3.1.0** (`compiler/include/sp/Version.h`
> is the single source of truth). This file is written slice by slice; each
> slice owns its own section. (Slice B created this file; slices A, C, D add
> theirs.)

---

## Slice B — throw/catch + `?` operator + map literals

### What was implemented

#### 1. try/catch/finally + throw — real lowering, ADR-002 direction

Phase 1 made `throw`, `catch` clauses and map literals fail loudly with
error[E1000]. Slice B replaces the loud errors for `throw`/`catch` with a
real implementation, designed to fit ADR-002 (Proposed, **not** approved):
**Result<T, E> & Option<T> with the syntactic `?` operator** for
cross-function error propagation; `throw`/`catch` is **local control flow
inside one function**, NOT C++-exception-style stack unwinding. There is
no unwinding runtime, no exception object, and no `Exception` class in the
language (a `catch (Exception e)` clause binds the thrown value as a
string; the type name is a binding annotation only).

Exact semantics (implemented in `compiler/src/ir/IRBuilder.cpp`,
`visit(TryCatchStmt)` / `visit(ThrowStmt)`; protocol slots in
`compiler/include/sp/ir/IRBuilder.h`):

- The try body runs. A `throw e` inside it stringifies `e`, stores the
  string in the function's shared throw-payload slot, sets the
  throw-pending flag, and branches to the catch block. Statements after
  the throw in the try body do not run (`visit(BlockStmt)` stops at the
  terminator).
- The catch variable is bound to the payload string (pointer type).
  Multiple catches: **the first catch wins** — later catches are dead
  code, because there is no runtime type to filter on. (Documented
  limitation; see "Deliberately not done".)
- `finally` runs on every exit path: normal completion, after the catch
  completes, before a deferred return leaves the try, and before a
  throw propagates outward.
- A `throw` with no enclosing catch propagates outward: to the next
  outer try's catch/finally, else to the shared uncaught-throw block:
  - enclosing function returns `Result<T, E>` → `return Err(e)`
    (tagged union tag=1, payload = stringified value);
  - returns `Option<T>` → `return None` (tag=1, payload 0 — the
    thrown value is discarded: Option carries no error);
  - otherwise → `sp_rt_panic(msg)` with the stringified thrown value
    (uncaught throw = panic, per ADR-002: unrecoverable logic errors
    use `panic()`).
- A `return` inside a try **with** a finally is deferred: the value is
  stashed in the return-value slot (reference return values are
  retained at the site), the return-pending flag is set, the finally
  blocks run, and the shared deferred-return block completes the
  return. A return inside a try **without** a finally returns normally.
- `break`/`continue` out of a try with a finally is a loud
  error[E1000] (the finally would be skipped). `break`/`continue` of a
  loop entered *inside* the try is allowed (loop depth at try entry is
  recorded in the frame).
- Throwing a non-scalar value (struct/class/map/array/Result/Option)
  is a loud error[E1000]: the catch binding is a string and there is no
  runtime representation for a composite thrown value. Throwing
  string/int/bool/char/float/double is supported (non-strings are
  stringified via the existing `convertToStringValue`).
- Nested try/catch works via a handler stack (`m_tryStack`); a throw
  goes to the innermost enclosing try. A throw inside a catch body
  propagates to the outer frames (the catch runs its finally first).
- Per-function protocol state (payload slot, pending flags, return
  slot, deferred-return block, uncaught-throw block) is created lazily
  in the entry block and reset whenever lowering moves to a new
  function (`ensureTryState`, hooked into `setInsertPoint`), so
  methods/constructors/specializations work too.
- Known leak (documented, same class as the existing post-terminator
  drop skipping): refcounted locals in a try body are not released on
  throw or deferred-return paths, and the deferred-return block emits
  no scope drops — it is shared by every deferred return, so it cannot
  know which scopes are live at a given return site (dropping a fixed
  snapshot would release uninitialized allocas on a shallower return
  path). Bounded: one function's worth of locals per exceptional exit.

#### 2. `?` operator — already existed; verified, not reimplemented

`expr?` on a Result/Option was already fully implemented (parser postfix
`TryExpr`, sema operand/enclosing-function checks, IRBuilder lowering
that loads the `{i32 tag, i64 payload}` tagged union and early-returns
the whole union on tag != 0). Slice B verified it end-to-end and added
edge execution tests (`?` in return position on `Option<string>`, plus
an Err-propagation test). No compiler change was needed for `?`.

#### 3. Map literals — real lowering

`{"a": 1, "b": 2}` now lowers to `sp_rt_map_alloc` + one
`sp_rt_map_set_str` / `sp_rt_map_set_i64` per entry, matching the
existing map index/store lowering (`visit(MapExpr)`).

Typing rules (the map runtime stores **int64 values only**):
- keys: string (`set_str`) or integer/char/bool (`set_i64`, cast to
  i64). Any other key type → loud error[E1000].
- values: integer/char/bool only (cast to i64). String, float,
  struct, array, map or Result/Option values have **no runtime
  representation** and fail loudly instead of being silently dropped
  or truncated.
- empty literal `{}` allocates an empty map (sema types it
  `Map<string, Any>`).
- string keys are copied by the runtime (`sp_rt_map_set_str`
  duplicates the key), so computed keys are safe.

### Tests

New files only (registered in `tests/CMakeLists.txt`):
- `tests/ir/TryCatchTest.cpp` — IR-level: try/catch/finally lowers,
  throw-in-try lowers, throw-in-Result-fn lowers to Err, non-scalar
  throw fails, break-out-of-try-with-finally fails, break-of-inner-loop
  allowed, `?` lowers, map literal compiles (string + int keys, empty),
  string-valued map literal fails.
- `tests/codegen/TryCatchExecutionTest.cpp` — real native execution
  via `spc` (Clang): no-throw path, throw caught with value, int throw
  caught as string, nested try/catch, throw-in-catch propagates,
  return-in-try-with-finally, throw-in-Result-fn → Err, uncaught throw
  → panic, `examples/27_try_catch.sp` shape, `?` on Option<string> +
  Err propagation, map literal read-back (string keys, int keys,
  empty).

| Suite | Tests | Result |
|---|---|---|
| TryCatchTest (IR) | 12 | **not yet run** — full build blocked (see Verification) |
| TryCatchExecutionTest | 15 | **not yet run** — full build blocked (see Verification) |

### Verification

- **Compile-level (done):** `compiler/src/ir/IRBuilder.cpp` compiles
  clean (direct TU compile in `/tmp/sp-build-phase3-throwcatch`); the
  only errors in that TU are slice C's `taskPayloadKind` call sites
  (see blocker below). Both new test files pass `-fsyntax-only`.
- **Full build + test runs + `spc run` proofs: BLOCKED.** The shared
  tree currently does not build: slice C's half-finished edit in
  `compiler/include/sp/sema/Symbol.h:129` (`FunctionSymbol::isAsync()`
  calls `m_astDecl->isAsync()` on a forward-declared `FunctionDecl` →
  `error: invalid use of incomplete type`) breaks every TU that
  includes it, starting with `src/sema/SymbolTable.cpp`; the same
  slice's `taskPayloadKind(...)` call sites in `IRBuilder.cpp`
  (5 sites) pass a `TypePtr` where the declared parameter is
  `const sema::Type*`. These are slice C's files; per the slice rules
  they were **not** fixed here. Once slice C's build is green, the
  slice-B verification to run is: full Release build in
  `/tmp/sp-build-phase3-throwcatch`, `TryCatchTest` + `TryCatchExecutionTest`
  + affected-area suites, and `spc run examples/27_try_catch.sp`
  (must now compile and print `Result: 5`, `Error: Division by zero`,
  `Result: 0`, `Finally block executed` ×2, no `Caught exception` lines).

### Deliberately not done

- No exception type hierarchy / runtime type filtering: `catch (E e)`
  is first-catch-wins; the type name is a binding annotation. Real
  typed catches need an `Exception` type in the language + a runtime
  representation — an ADR-002 decision, not a slice-B invention.
- No `break`/`continue` out of a try with a finally (loud E1000).
- No throwing composite values (loud E1000).
- No fn-pointer callbacks (unchanged from Phase 2; still loud E1000).
- Refcounted locals leak on throw/deferred-return paths (documented
  above).
- ADR-002 itself is still Proposed/unapproved; the semantics above are
  the minimal honest reading of it, documented here rather than
  silently invented.

## Slice C — `await` on user async fns + Task model completion

### What was implemented

#### 1. Calling an `async fn` returns a Task (spawn model)

Phase 2 slice B2 proved pump-based `await` for the builtins only:
calling a user async fn ran its body to completion inline, so
`await worker()` had no Task to wait on and failed loudly. That is
now the headline of this slice: **calling an async fn spawns its body
as a Task and returns the Task immediately; awaiting the Task pumps
the scheduler until the body completes with the right value.**

- **Sema** (`compiler/src/sema/TypeChecker.cpp`, `Symbol.h`,
  `NameResolver.cpp`): a call to an `async fn` now types as
  `Task<T>` instead of `T` (`wrapAsyncReturn`, applied at all four
  call sites: free fns incl. generics, methods on the enclosing
  class, class/interface member calls). `FunctionSymbol` gained an
  `isAsync` flag (set from the decl by NameResolver, copied from the
  `MemberMethod` for methods — the flag is a plain bool, never a call
  through the forward-declared AST decl, which would be an
  incomplete-type error); `MemberMethod` gained `isAsync`.
- **Runtime** (`runtime/include/sp/runtime/Async.h`,
  `runtime/src/net/EventLoop.cpp`, `runtime/src/net/NetBridge.cpp`):
  new C ABI `sp_rt_async_create_task`,
  `sp_rt_async_spawn_i64/str/void(sched, task, thunk, ctx)`,
  `sp_rt_async_task_await_str`, `sp_rt_async_delayed_str`, and
  `sp_rt_async_scheduler_run_tasks_until_idle`. A thunk is a
  compiler-generated `int64_t/const char*/void (*)(void*)` that loads
  its arguments from a heap ctx, calls the body, frees the ctx, and
  returns the result. Spawning only posts the thunk call: the body
  does **not** run at the call site, and the task completes when
  something pumps the scheduler.
- **IR** (`compiler/src/ir/IRBuilder.cpp`): `visit(CallExpr)`
  intercepts calls whose resolved target is async (free fns incl.
  generics via the monomorphized name, static/direct method calls,
  `super.method(...)`) and lowers them to create_task + `sp_rt_alloc`
  ctx (8-byte slots, byte-offset GEPs over `i8`) + spawn. A new
  `FunctionAddress` IR value kind (`@name`) carries the thunk address
  through the existing indirect `CallInst`. `visit(FunctionDecl)`,
  `compileMethod`, `compileSpecializedMethod` and
  `compileSpecializedFunction` each generate the thunk for their async
  fn and set `m_inAsyncFunction` from the decl, so awaits inside
  async methods and generic specializations now lower too.
- **Task payloads**: `Task` gained a string payload (`completeStr` /
  `strValue`); `delayed_value` gained `(int, string) -> Task<string>`
  and `(int, bool) -> Task<bool>` overloads
  (`compiler/src/sema/SymbolTable.cpp`). Payload kinds the runtime
  can carry: **void, int, bool, string** (bool rides the i64 task
  with a truncating cast at the await site). Anything else (double,
  arrays, ...) still fails loudly with error[E1000].

#### 2. `await` on a Task value (not just on a call)

`visit(AwaitExpr)` now lowers four forms, in order: (1) the B2
builtins `sleep_async` / `delayed_value` (now incl. string/bool
payloads); (2) `await worker(...)` — spawn + await inline, then
destroy the temporary task; (3) `await <expr of type Task<T>>`
(e.g. a variable holding a Task, or a call to a non-async fn
returning `Task<T>`) — pumps until it completes and does **not**
destroy the task, so awaiting it twice returns the same value;
(4) everything else still fails loudly (literals, sync calls,
operators, await outside an `async fn`). Awaiting a Task whose
payload kind cannot be carried fails loudly too.

#### 3. Async methods, including virtual dispatch

Instance methods are virtual in this compiler (every class has a
vtable), so `obj.asyncMethod()` cannot use the direct thunk — the
concrete body is only known at runtime. Such calls spawn through an
**indirect thunk** (`__sp_async_thunk_indirect_<mangled>`): the
vtable function pointer loaded at the call site goes in ctx slot 0,
the thunk loads it and calls it indirectly. The same applies to
unqualified method calls from inside a class (vtable dispatch) and
to `super.method(...)` (direct thunk). Static async methods use the
direct thunk. Async methods may `await` inside their own bodies.

#### 4. Main-exit pump (fire-and-forget tasks finish)

A sync `main` cannot `await`, so a spawned task with no awaiter
would previously be dropped silently at process exit. The compiler
now emits a tasks-only drain at every normal main exit (explicit
`return`, implicit fall-through, and the synthesized main for
top-level statements): `sp_rt_async_scheduler_run_tasks_until_idle`
drains timers/queued tasks and **ignores registered sockets**, so it
cannot hang a server program, and returns immediately when no async
work is pending. Known limitation: a `return` deferred by a
`try`/`finally` in main skips this drain (the deferred-return path
belongs to slice B's finally machinery).

#### 5. `stdlib/async` module

New `stdlib/async/async.sp` (`module std.async;`, registered in
`tools/spm/main.cpp`): `public class Async` with `public static
async fn sleep(ms)`, `delayInt/delayString/delayBool(ms, value)` —
thin `async fn` wrappers over the builtins, so programs can `await
Async.sleep(50)` through the module. The builtins stay global; the
module is a namespace + documentation, not a new primitive. Task
combinators (all / race / timeout), cancellation handles, and async
I/O are deliberately **not** in the module (not faked).

Implementation note (honest): the first version declared the
wrappers as top-level free `async fn`s. That compiled (`spc check`
passed) but `import std.async;` left `Async` undefined at the use
site — the ModuleLoader merges a file module's declarations into
the root program, and only a `public class` binds a namespace
(`Time`, `Json` follow the same pattern). The wrappers were
rewritten as `public static async fn` methods inside `public class
Async`; `Async.sleep` / `Async.delayString` then resolve and run
(proof P4 below).

### The Task model (as now implemented)

- A `Task<T>` is an opaque handle (a pointer at runtime) created by
  the runtime; calling an async fn returns it, awaiting pumps the
  scheduler until it completes. The awaiting frame **stays on the
  stack while pumping** (B2 pump design — see §6).
- Ownership: whoever creates a Task owns it. `await f()` destroys
  the temporary task it created; `await t` (a variable) does not —
  the variable's owner keeps it. A task that is never awaited or
  pumped never runs its body, and its heap ctx leaks (documented,
  not hidden).
- Cancelling a task marks it done so a pending await returns
  promptly; it does not un-schedule an already-posted body.
- The scheduler is single-threaded; the pump is re-entrant (the
  ready deque is drained pop-front/push-back with no iterators held
  across callbacks), which is what lets a spawned body `await`
  inside a pump callback.

### Tests

Extended (no new test files):
- `tests/runtime/AsyncTest.cpp` — spawn runs only when pumped (and
  not before), string/void payloads, the headline loop-not-blocked
  proof through the spawn path (a spawned body that itself awaits a
  150 ms inner task; a 20 ms timer fires *during* the outer await),
  `delayedValueStrTask` + `awaitTaskStr`, `runUntilTasksIdle`
  drains timers, returns immediately with no work, and is not kept
  going by a registered socket (socketpair), C ABI delayed-str +
  await-str.
- `tests/ir/IRBuilderTest.cpp` — the Phase 2 test
  `AwaitOnUserAsyncFnCallStillFailsIRGeneration` is replaced by
  `AwaitOnUserAsyncFnCallLowers` (spawn + await + destroy in IR,
  thunk defined); new: call-without-await returns a Task (no await
  emitted) + main-exit pump in IR, await-on-Task-variable lowers
  without destroy, string/bool payloads, `delayed_value` string/bool
  overloads, async method through virtual dispatch (indirect thunk),
  await inside an async method, generic async fn
  (`identity<int>`), and the still-failing double-payload case.
  The older still-fail tests are kept (`await 1`, `await now()`,
  `await x` on a non-Task variable).

| Suite | Tests | Result |
|---|---|---|
| AsyncTest (runtime) | 18 (11 B2 + 7 new) | **18/18 pass** |
| IRBuilderTest (await/async subset) | 17 (7 B2 + 10 new/renamed) | **17/17 pass** |

### Verification

- **Build:** full Release build in `/tmp/sp-build-phase3-async`
  (`cmake -S . -B <dir> -DCMAKE_BUILD_TYPE=Release
  -DSP_BUILD_TESTS=ON`, `cmake --build <dir> -j2`) — **clean, exit
  0, 0 errors** (`spc`, `sp_tests`, `spm`, `spd`, `spls` all
  built; build log `/tmp/sp-p3-async-build4.log`). Two self-caught
  errors in this slice's code were fixed before the clean build:
  `IRBuilder.cpp` `fnPtr` declared as `IRValuePtr` but passed to
  `addInstruction` (needs an instruction type), and `SP_EVENT_READ`
  unqualified in `AsyncTest.cpp` (lives in `sp::rt::net`). The
  runtime library `libsp_runtime.a` had already built clean on the
  first attempt.
- **Affected-area suites + new tests:** `AsyncTest` **18/18 pass**;
  IR await/async subset **17/17 pass**; the four affected suites
  together (`IRBuilderTest`, `TypeCheckerTest`, `TryCatchTest`,
  `ComptimeEvaluatorTest`) **89/94 pass** — the 5 failures are all
  in other slices' regions (see below). **Full suite:
  592 passed / 11 failed out of 603** (the known-hanging
  `NetTest.UdpSocketSendRecv` excluded, as in Phase 2). The 11
  failures break down as: 5× `IRBuilderTest` (slice A/B's
  intentional behaviour changes — `Throw`/`TryCatch`/`MapLiteral`
  now compile where Phase 1 tests assert they fail, same-name
  overload clash now compiles under slice A's mangling, and slice
  A's new `NonCapturingLambdaStillCompiles` control case fails at
  parse — the `(int x) -> int { ... }` syntax is rejected by the
  unmodified parser; none of the 5 touches async code), 2×
  `ExecutionTest` (`StdlibTimeNativeExecution`,
  `StdlibFullIntegrationNativeExecution` — the Phase 2
  pre-existing stdlib-time epoch-ms failures, `now_gt_0: true`
  not found in program output), 4× `TryCatchExecutionTest`
  (slice B's new execution tests, in progress).
- **`spc run` proofs (Clang 18.1.3, observed output — not `spc
  check`), all exit 0, programs in `/tmp/sp-p3-async-proofs/`:**
  - **P1** `await` on a user async fn returning a value:
    `async fn compute() -> int { return 42; }` + `async fn main()
    { var v = await compute(); println(v); println("done"); }` →
    `42`, `done`. (Also proves `async fn main` works: its awaits
    pump, and the implicit main return drains.)
  - **P2** loop stays live during the await: `slow()` prints
    `slow start`, awaits `delayed_value(150, 7)`, prints `slow
    done`; main spawns it, prints `spawned`, awaits it → output
    `spawned`, `slow start`, `slow done`, `7`, `done` — the body
    runs only when pumped (after `spawned`) and the await waits
    for it. The rigorous loop-not-blocked proof (a 20 ms timer
    firing *during* the outer await, with timestamps) is the C++
    test `LoopNotBlockedWhileAwaitingSpawnedTask`, which passes.
  - **P3** fire-and-forget from sync main (exit pump): `fn main()
    { var t = greet(); println("spawned"); }` →
    `spawned`, `greeted` — the spawned body runs during the
    main-exit drain.
  - **P4** `stdlib/async`: `import std.async;` + `await
    Async.sleep(20); var s = await Async.delayString(10, "hi");`
    → `hi`, `done`.
  - **P5** async method via virtual dispatch: `class Greeter {
    public async fn greet() -> string { ... return "hello"; } }`,
    `await g.greet()` → `hello`, `done`.
  - **P6** generic async fn: `await identity<int>(99)` → `99`,
    `done`.
  - **P7** bool payload: `await check()` where `check` awaits
    `delayed_value(10, true)` → `yes`, `done`.
  - **P8** double-await of the same Task variable: `await t`
    twice → `5`, `5`, `done` (no destroy on the owner's task).
- **Pre-existing failures hit:** the 2 stdlib-time epoch-ms
  `ExecutionTest` failures above (Phase 2 known, untouched by
  this slice). No other pre-existing failure was hit; the 9
  remaining full-suite failures are other slices' in-progress
  work (listed above), not pre-existing at `f471a69`.

### Deliberately not done

- **No heap-frame state machine.** The awaiting frame still stays
  on the stack while pumping (same as B2). Consequences, stated
  plainly: an async fn cannot return a Task that outlives its own
  frame; `let t = worker();` without await only runs when something
  pumps; a task whose body never returns (or that is never pumped)
  never completes. A real state machine is future work — ADR-003
  (still Proposed) names it as the evolution step, and this slice
  does not claim it.
- No `Task<double>` / array / struct payloads (loud E1000).
- No async-fn arguments of struct/array/class type through a Task
  (only int, bool, string, double args are carried; loud E1000).
- No Task combinators (all / race / timeout), no cancellation
  handles in SP, no async HTTP/fetch primitive.
- `async fn main` **is** proven in this slice: proofs P1, P2, P4–P8
  all run under `async fn main()` — its awaits pump, and the
  implicit main return drains any remaining tasks. (A sync `main`
  still cannot `await`; it gets the exit drain instead.)
- Diagnostic wart (not a behaviour bug): `await dbl()` on a
  double-payload async fn reports the E1000 twice — once at the
  await site and once at the call site (the await check runs
  before visiting the call, and the spawn intercept reports again).
  Either error alone is sufficient; the duplicate is noise, not a
  second failure mode.
- A `return` inside a `try`/`finally` in main skips the exit drain.
- Overloaded async fns share the thunk-naming scheme with slice A's
  overload mangling (`llvmFunctionName` / `llvmCallName`); two
  same-name async overloads get distinct thunks via the mangled
  suffix, but this interaction is only covered by the generic +
  method tests above, not by a dedicated overloaded-async-fn test.
- ADR-003 itself is still Proposed/unapproved; the spawn model
  above follows its direction (non-blocking loop, clear task
  lifetimes) without claiming the state-machine evolution.

---

## Slice A — Function pointers + overload name mangling + stdlib callback wrappers

Phase 2 left two loud gaps that this slice closes. First, a bare SP `fn`
used as a value lowered to null (Phase 2 made that a loud `error[E1000]`
instead of a silent null, because a null callback made queue jobs land in
the DLQ with no error anywhere). Second, same-name overloads failed loudly
at IR verification (`Function 'add' redefined with a different signature`),
pending name mangling. This slice implements the real lowering behind both
loud errors and wires the stdlib callback wrappers that depend on it
(queue handler, cron task, WebSocket route).

### What was implemented

#### 1. Overload name mangling — `compiler/src/codegen/VTableBuilder.cpp`, `compiler/include/sp/codegen/VTableBuilder.h`

Shared helpers on `VTableBuilder` (used by the vtable builder itself and
by `IRBuilder`, so definition and call sites agree):

- `sanitizeMangledType(typeName)` — keeps `[A-Za-z0-9_]`, replaces every
  other character with `_` (`int[]` → `int__`, `ptr<int>` → `ptr_int_`).
- `overloadSuffix(canonicalParamTypes)` — `"__"` + sanitized param types
  joined with `"_"`; zero params → `"__void"`.
- `mangleOverloaded(baseName, canonicalParamTypes, isOverloaded)` —
  returns the plain base name when not overloaded, else base + suffix.

**Mangling rule (Option B): a single-definition function keeps its plain
LLVM name.** Only an overloaded name gains a suffix. `main` (the entry
point) and `extern "C"` declarations are never mangled. Generic
specializations keep their existing `name_typeArgs` scheme.

Examples (param types are sema-canonical `Type::toString()` values, so
`int` and `int32` spell the same signature):

| SP declaration | LLVM name |
|---|---|
| `fn add(int a, int b) -> int` (only `add`) | `add` |
| `fn add(int a, int b) -> int` + `fn add(int a, int b, int c) -> int` | `add__int_int`, `add__int_int_int` |
| `fn describe(int x) -> int` + `fn describe(string s) -> int` | `describe__int`, `describe__string` |
| `fn f()` (overloaded, zero params) | `f__void` |
| `fn get(int[] xs)` (overloaded) | `get__int__` |
| `fn f(ptr<int> p)` (overloaded) | `f__ptr_int_` |
| `class Animal { fn speak() }` (only `speak`) | `Animal_speak` |
| `class C { fn greet(int) }` + `fn greet(string)` | `C_greet__int`, `C_greet__string` |

"Overloaded" means **more than one distinct signature visible with the
name** — an override of the same signature is NOT an overload
(`findMethods` dedupes overrides by `FunctionType::equals`, and the free
function table rejects identical signatures), so a derived override keeps
its plain name when the base had only that signature.

**Vtable slots** (`buildClassLayout`): the slot map is now keyed by
`methodName + overloadSuffix(paramTypes)` — always suffixed, even for
non-overloaded methods — so a derived override (same signature, same key)
finds the base slot, while a new overload (different key) gets a new slot.
The stored `mangledName` (the LLVM function name) only carries the suffix
when the name is overloaded in the defining class, so existing single
method LLVM names are unchanged. `getMethodVTableSlot(className,
methodName)` is kept as a legacy first-match-by-name lookup (used for
generic-class layouts, whose slot map is still keyed by plain name);
overloaded lookups use the new `getMethodVTableSlotByKey(className,
methodKey)`.

#### 2. Call-site overload resolution — `compiler/src/sema/TypeChecker.cpp`, `compiler/include/sp/sema/TypeChecker.h`, `compiler/include/sp/sema/SemanticAnalyzer.h`

`TypeChecker` records the overload it resolved for every call in a new
per-node map `m_nodeCallTargets` (`CallExpr*` → `FunctionSymbol`),
parallel to the existing `m_nodeTypes` map, exposed via
`TypeChecker::getCallTarget()` and `SemanticAnalyzer::getCallTarget()`.
Recorded at all four resolution sites: direct free-function calls,
methods on the enclosing class, class methods, and interface methods.
`nullptr` when the call resolved to a builtin, a variable holding a
function, or nothing. IRBuilder uses the recorded target's canonical
param types to compute the mangled name — it never re-resolves overloads
itself.

#### 3. IRBuilder definition + call sites — `compiler/src/ir/IRBuilder.cpp`, `compiler/include/sp/ir/IRBuilder.h`

- `llvmFunctionName(decl)` / `llvmMethodName(className, decl)` /
  `llvmCallName(calleeName, target)` — LLVM names for definitions and
  direct calls, using the helpers above. `fnParamTypes(decl)` takes
  canonical param types from the decl's `FunctionSymbol` (falling back to
  the decl's raw param strings when a symbol carries no AST decl, e.g.
  some stdlib functions).
- `visit(Program)` indexes `m_astFunctions` by the **mangled** name (free
  functions and `Class_method`), so both overloads coexist in the index.
  Extern pre-declarations still use the plain C name.
- `visit(FunctionDecl)` / `compileMethod` define functions under their
  mangled name. `compileSpecializedMethod` (generic classes) is unchanged
  — see "Deliberately not done".
- Call sites: the general free-function path, the `MemberExpr` virtual
  dispatch path (slot lookup by signature key, falling back to the legacy
  name lookup for generic layouts), the `MemberExpr` direct/static path,
  the `super.method(...)` path, and the unqualified-method-from-inside-a-
  class path all use the recorded target's param types. Every path falls
  back to the old plain-name behaviour when no target was recorded or the
  class type is not found, so unresolved/builtin/generic calls are
  unaffected.
- `VTableBuilder::canonicalParamTypes(clsType, decl)` /
  `isMethodOverloaded(clsType, methodName)` — canonical method param
  types from the class's `MemberMethod` table (the decl is matched by
  name + arity, preferring the candidate whose canonical param types
  spell the same as the decl's raw strings, so aliases such as `int32`
  vs `int` agree) and the overload test, shared by the vtable builder
  and IRBuilder so both sides compute the same names.

#### 4. Real function-pointer lowering — `visit(IdentifierExpr)`

The Phase 2 loud error is replaced by real lowering for the supported
case: a bare free function used as a value now emits a `GlobalVariable`
(`IRType::getInt8()`, `"@" + llvmName`) with type `ptr` — exactly the
pattern `visit(LambdaExpr)` already uses. LLVM accepts forward
references and the verifier does not check bare `@name` operands, so
the definition may come later in the file, and an `extern "C"` function
used as a value also lowers to its (plain) address.

Still loud `error[E1000]` where a plain function pointer cannot exist:

- **Overloaded bare name** — ambiguous; SP has only untyped `ptr`, so
  there is no way to pick an overload.
- **Bare method name** — needs an instance. (Methods are not in the
  symbol table, so this is checked against the enclosing class's
  `findMethods` when inside a class.)
- **Generic function as a bare value** — no type arguments at a bare use.

A bare function assigned to a `ptr` variable also lowers to its address,
and calling through that variable uses the existing indirect-call path.

#### 5. Capturing lambdas fail loudly — `visit(LambdaExpr)`, `visit(IdentifierExpr)`

`visit(LambdaExpr)` snapshots the enclosing variable names (all scopes)
before clearing them for the lambda body. An identifier in the body that
names an outer variable is a capture attempt and now reports
`error[E1000]` ("lambda captures variable ...; capturing lambdas are not
supported; pass the value as a parameter") instead of silently lowering
to `0`. Non-capturing lambdas are unaffected (their bodies contain no
outer-variable reference).

#### 6. Stdlib callback wrappers

`stdlib/queue/queue.sp`:

- `extern "C"`: `sp_rt_job_queue_register_handler(ptr queue, string name,
  ptr handlerFnPtr)`, `sp_rt_cron_schedule_interval(ptr cron, string
  name, int interval_ms, ptr taskFnPtr)`, `sp_rt_cron_schedule(ptr cron,
  string name, string cron_expr, ptr taskFnPtr)` — the exact lines Slice
  C specified.
- `JobQueue.registerHandler(string name, ptr handlerFnPtr)` — handler
  shape: `fn handler(string jobId, string jobName, string payload) ->
  bool` (return `false` to retry, then DLQ). Follows the existing
  `HttpServer.route` pattern (null-handle guard, pass the `ptr`
  straight through).
- `CronScheduler.scheduleInterval(string name, int intervalMs, ptr
  taskFnPtr)` and `CronScheduler.schedule(string name, string cronExpr,
  ptr taskFnPtr)` — task shape: `fn task()`.

`stdlib/net/ws.sp` (see note below on the file choice):

- `extern "C"`: `sp_rt_ws_route(string path, ptr handlerFnPtr)`,
  `sp_rt_ws_send(int64 conn_id, string msg) -> bool`,
  `sp_rt_ws_broadcast(string msg)`.
- Free functions `wsRoute(string path, ptr handlerFnPtr)`,
  `wsSend(int64 connId, string msg) -> bool`, `wsBroadcast(string msg)`.
  Handler shape: `fn onMessage(int64 connId, string msg)`. `int64`
  matches the C `int64_t conn_id` exactly (`long` is not a valid SP
  source-level type — sema rejects it with `Undefined type 'long'`).

**File-choice note (corrects the Phase 31 doc — parent fix):** The Phase 31
doc specified the WS wrapper path as `stdlib/net/websocket.sp`, but that
file is never loaded:
`stdlib/net/ws.sp` declares the same `module std.net.ws`, and the
`ModuleLoader` resolves `import std.net.ws` to `stdlib/net/ws.sp`
(priority b: `stdlib/<subpath>.sp`). The two files were byte-identical
before this slice (verified by diff). The wrapper therefore lives in the
live file, `stdlib/net/ws.sp`; `websocket.sp` is untouched (it is dead
duplicate code — a candidate for deletion in a later cleanup, not this
slice).

### Files changed

| File | Change |
|---|---|
| `compiler/include/sp/codegen/VTableBuilder.h` | Mangling helpers, `getMethodVTableSlotByKey`, `canonicalParamTypes`, `isMethodOverloaded` |
| `compiler/src/codegen/VTableBuilder.cpp` | Helper implementations, key-based vtable slots, legacy name lookup kept |
| `compiler/include/sp/sema/TypeChecker.h` | `m_nodeCallTargets` map + `getCallTarget()` |
| `compiler/src/sema/TypeChecker.cpp` | Record resolved target at 4 call sites |
| `compiler/include/sp/sema/SemanticAnalyzer.h` | `getCallTarget()` pass-through |
| `compiler/include/sp/ir/IRBuilder.h` | `llvmFunctionName` / `llvmMethodName` / `llvmCallName` / `fnParamTypes` / `targetParamTypes`, `m_lambdaOuterVars` |
| `compiler/src/ir/IRBuilder.cpp` | Mangled definitions + call sites, fn-pointer lowering, capture check, lambda snapshot |
| `stdlib/queue/queue.sp` | `registerHandler`, `scheduleInterval`, `schedule` + 3 `extern "C"` decls |
| `stdlib/net/ws.sp` | `wsRoute`, `wsSend`, `wsBroadcast` + 3 `extern "C"` decls |
| `tests/ir/IRBuilderTest.cpp` | Phase 2 bare-fn test replaced + 8 new tests (see below) |
| `docs/v2/PHASE-32-PRO-4.0.0.md` | This section |

No other files touched by this slice. No version bump (stays 3.1.0), no
`.github/` changes, no CMakeLists changes (tests added to the existing
`IRBuilderTest.cpp`, no new test file).

### Tests — `tests/ir/IRBuilderTest.cpp`

The Phase 2 test `BareFunctionNameAsValueFailsIRGeneration` is replaced
by `BareFunctionNameAsValueLowersToRealAddress` (same source, now
compiles; asserts `call void @fake_register(ptr @handler)` and `define
void @handler()`).

New tests (all in `IRBuilderTest.cpp`):

| Test | Asserts |
|---|---|
| `BareFunctionNameAsValueLowersToRealAddress` | Bare fn lowers to `@handler`, not null |
| `FunctionPointerAssignedToPtrVariable` | `let f = doubleIt` holds the address; `f(21)` indirect-calls it |
| `OverloadedFreeFunctionsBothCompileAndResolve` | `add(int,int)` / `add(int,int,int)` → `add__int_int` / `add__int_int_int`, both defined and called; no plain `@add(` remains |
| `OverloadedFreeFunctionsByParamType` | `describe(int)` / `describe(string)` → `describe__int` / `describe__string` |
| `OverloadedMethodsBothCompileAndResolve` | `Greeter.greet(int)` / `greet(string)` → `Greeter_greet__int` / `Greeter_greet__string`, both in the vtable |
| `OverloadedBareNameAsValueStillFailsIRGeneration` | Overloaded bare name as a value still fails loudly |
| `BareMethodNameAsValueStillFailsIRGeneration` | Bare method name as a value still fails loudly |
| `CapturingLambdaAsValueStillFailsIRGeneration` | Lambda capturing an outer local still fails loudly |
| `NonCapturingLambdaStillCompiles` | Control case: non-capturing lambda still compiles |

### Verification performed

**What was verified (this slice, in its own build dir
`/tmp/sp-build-phase3-fnptr` unless noted):**

- **Per-TU compile (done):** `VTableBuilder.cpp`, `TypeChecker.cpp` and
  `tests/ir/IRBuilderTest.cpp` compile clean (`g++ -fsyntax-only`,
  C++20). `IRBuilder.cpp` compiles clean **except for 8 errors that are
  all slice C's async code** (7× `taskPayloadKind(TypePtr)` vs declared
  `const sema::Type*`, 1× `addInstruction(fnPtr)` type mismatch at
  line 8019) — no errors in slice-A code. (Slice C fixed the 7
  `taskPayloadKind` sites after that check; only the 8019 error
  remains.)
- **Mangling helpers (done, real compiled code):** standalone harness
  `/tmp/sp-p3-proofs/mangle_check.cpp` (links `VTableBuilder.cpp` +
  sema sources, no IRBuilder) — **15/15 checks pass** (`sanitize`,
  `overloadSuffix`, `mangleOverloaded` incl. `add__int_int`,
  `add__int_int_int`, `f__void`, `Greeter_greet__string`,
  `get__int__`).
- **Sema call-target recording (done, real lexer/parser/sema):**
  standalone harness `/tmp/sp-p3-proofs/sema_check.cpp` (same link
  set) — **13/13 checks pass**: overloaded `add`/`describe` calls and
  `Greeter.greet` calls (direct, method, and unqualified-from-inside-
  a-method) all record the correct overload via `getCallTarget()`.
- **Stdlib wrappers type-check (done, Phase 2 `spc` binary
  `/tmp/sp-build-phase2/bin/spc`, which loads the current stdlib):**
  `spc check` passes for the queue, cron and ws proof programs
  (`/tmp/sp-p3-proofs/proof_{queue,cron,ws}.sp`). Found and fixed two
  real issues this way: `long` is not a valid SP type (sema:
  `Undefined type 'long'` — wrappers use `int64`), and the WS wrapper
  belongs in `stdlib/net/ws.sp`, not `websocket.sp` (the loader only
  ever loads `ws.sp` for `import std.net.ws`; the files were
  byte-identical).
- **Queue harness end-to-end (done, Phase 2 `spc run`):** the
  no-handler queue proof runs: `ENQUEUED: job_1`, `COMPLETED: 0`,
  `DLQ: 1` — a job with no handler lands in the DLQ, as designed.
- **Phase 2 binary confirms the exact errors this slice removes:**
  `spc run proof_overload.sp` → `error[E1000]: IR verification
  failed: Function 'add' redefined with a different signature...`;
  `spc run proof_queue.sp` → `error[E1000]: 'function 'sendEmail'
  used as a value' is not yet supported...`.

**BLOCKED — full build, `sp_tests`, and `spc run` proofs:** the shared
tree does not build. One error remains, in slice C's code:
`compiler/src/ir/IRBuilder.cpp:8019` (`emitAsyncThunk`):
`m_currentBlock->addInstruction(fnPtr)` — `fnPtr` is `IRValuePtr`
but `addInstruction` takes `shared_ptr<IRInstruction>`:
`error: cannot convert 'shared_ptr<sp::ir::IRValue>' to
'shared_ptr<sp::ir::IRInstruction>'`. (Two earlier blockers from the
same slice — `Symbol.h:129` `isAsync()` incomplete-type error and 7×
`taskPayloadKind` call sites — were fixed by slice C during this
slice's work.) Per the slice rules this was **not** fixed here.
Once it is fixed, run, in `/tmp/sp-build-phase3-fnptr`:
`cmake --build /tmp/sp-build-phase3-fnptr -j2`, then
`./bin/sp_tests --gtest_filter='IRBuilderTest.*'` (9 slice-A tests:
the replaced bare-fn test + 8 new), the affected-area suites
(`VTableBuilderTest`, `TypeCheckerTest`, `ExecutionTest`), and the SP
proofs with the new binary (`/tmp/sp-build-phase3-fnptr/bin/spc run
/tmp/sp-p3-proofs/proof_{queue,cron,overload}.sp`, plus `spc check`
for `proof_ws.sp`): queue must print `HANDLER FIRED: ...`,
`COMPLETED: 1`, `DLQ: 0`; cron must print `CRON FIRED FILE:` with
tick lines; overload must print `ADD2: 3`, `ADD3: 6`,
`DESC_INT: 40`, `DESC_STR: 7`, `GREET_INT: 300`, `GREET_STR: 5`.

### Deliberately not done

- **Overloaded generic functions / methods.** A generic `fn` with
  same-name overloads, and overloaded methods on generic classes, still
  collide on the monomorphized name and fail loudly (pre-existing
  behaviour). The mangling scheme covers non-generic overloads only.
- **Overloaded constructors.** Two ctors with different params still
  collide (`Class_Class`) and fail loudly.
- **Capturing lambdas / closures.** Out of scope by design — they fail
  loudly instead of silently miscompiling (see §5).
- **Bare method names as values.** No instance to bind; fails loudly.
  (A method can still be used as a callback through a non-capturing
  lambda or a free function.)
- **Return-type-only overloads.** Rejected by sema before mangling
  (`checkSameParams` compares param types only), unchanged.
- **`spc bindgen`.** Out of scope per ADR-004; fn pointers here serve
  the Direct C-ABI direction only.
- **Deleting the dead `stdlib/net/websocket.sp` duplicate.** Noted above;
  left for a later cleanup so this slice stays additive.
- **WS end-to-end run.** `wsRoute` compiles and registers; a live
  WebSocket round-trip (HTTP server + WS client) was not run in this
  environment — see Verification.

---

## Slice D — Real comptime evaluation

### Background

The language spec (`docs/SP-Syntax-Guide.md` §101, "SP 2.0 Compile-Time
Execution (`comptime`)") describes `comptime fn` + `comptime expr` evaluated
by "the SP compiler AST evaluator" and folded into immediate constants, with
the example `comptime fn factorial(int n) -> int` and
`let result = comptime factorial(10);` producing `3628800`. But the v1
analysis found the real behaviour was pass-through: `comptime` expressions
were lowered as ordinary runtime code, so programs could compile and run
wrong with no error. Phase 1 (Phase 30, item 12) made `comptime` fail loudly
with `error[E1000]` instead of passing through. This slice makes comptime
real: expressions are evaluated at compile time and the computed constant is
folded into the IR, with zero runtime code emitted for the comptime part.

A `ComptimeEvaluator` already existed in sema
(`compiler/include/sp/sema/ComptimeEvaluator.h`,
`compiler/src/sema/ComptimeEvaluator.cpp`, 610 lines, with tests in
`tests/sema/ComptimeEvaluatorTest.cpp`) but it was **never wired into the
pipeline**: `processProgram` only registered functions and nothing ever
folded a value. `tools/spc/main.cpp` instantiated it and called
`processProgram` before sema, but the IR builder ignored it.

### What was implemented

**1. Wiring: evaluation runs in the IR builder, on the sema evaluator
(`compiler/src/ir/IRBuilder.cpp`, `compiler/include/sp/ir/IRBuilder.h`)**

- `IRBuilder::build()` constructs a member `m_comptimeEval`
  (`std::unique_ptr<sema::ComptimeEvaluator>`) and calls
  `processProgram(program)` on the whole program **before** lowering, so the
  evaluator's registry of `comptime fn` functions is ready.
- `IRBuilder::visit(const ast::ComptimeExpr&)` now calls
  `m_comptimeEval->evaluate(node)` (its `ComptimeExpr` case handles both the
  expression form and the block form) and lowers the resulting value via
  `value.toLiteralAST()` + `accept(*this)` — i.e. ordinary literal lowering,
  so the constant is folded into the IR. If the evaluator reports an error
  or the result is `Void`, the visit reports `error[E1000]` embedding the
  evaluator's specific message (e.g. `Undefined variable in compile-time
  context: 'n'`, `Division by zero in compile-time evaluation`) plus the
  supported-subset text. The Phase 1 loud-fail behaviour is kept for exactly
  the cases that cannot be evaluated; the error is now specific instead of
  "not implemented yet".

**2. Evaluator fixes: close silent-failure paths
(`compiler/src/sema/ComptimeEvaluator.cpp`,
`compiler/include/sp/sema/ComptimeEvaluator.h`)**

- `processProgram` now takes `const ast::Program&` and registers **only**
  `isComptime()` functions. The old code registered *all* functions, so a
  comptime call could silently interpret a runtime function body (whose
  I/O / side effects the evaluator cannot reproduce).
- Unknown call → new error **E1806** ("only `comptime fn` functions and the
  builtins min/max/abs/assert are available"). Previously an unknown call
  silently produced a void value.
- New **E1807** (unsupported construct in comptime) for: the `evaluate`
  default branch, `MemberExpr` other than `.length`, `IndexExpr` kind
  mismatch, `AssignExpr` with a non-identifier target, `evaluateBinary` /
  `evaluateUnary` fallthrough, and the `evaluateStmt` default
  (throw / try-catch / switch / ... are loud errors, never silently skipped).
  Each is guarded by `if (!m_hasError)` to avoid double-reporting.
- Correctness fixes in comparisons: `char ==` / `!=` now compare values
  (the old fallback returned `true` for any two chars, comparing only
  kinds); mixed int/float `==` / `!=` compare numerically; `<=` / `>=` now
  handle float mixes like `<` / `>` already did.

### Exact supported subset

Syntax (as accepted by the parser/sema; unchanged by this slice):

- `comptime <operand>` as an expression, where the operand binds at **unary
  precedence** — the same binding as `await` in `Parser.cpp`. Calls, member
  access (`.length`), indexing, and literals bind inside; **binary operators
  do not**. `comptime 1 + 2` parses as `(comptime 1) + 2` at runtime — a
  binary expression must be parenthesized: `comptime (1 + 2)`. The ternary
  likewise needs parens: `comptime ((2 > 1) ? 10 : 20)`.
- `comptime { <block> }` as an expression. The block's value is its **last
  statement's value** (normally a trailing expression statement, e.g.
  `a * b;`). A block that produces no value (statement-only, e.g.
  `comptime { assert(...); }`) yields `Void` and fails loudly with E1000.
- `comptime fn <name>(...) -> T { ... }` declarations; calls to them from
  `comptime` expressions.

What evaluates (via the sema evaluator):

- Literal `int` / `float` / `bool` / `string` / `char` arithmetic and logic,
  including precedence, comparison, bitwise and logical operators.
- String concatenation with `+`.
- Ternary expressions, arrays + indexing, `.length` (strings and arrays),
  string indexing.
- Calls to `comptime fn` functions, including recursion and bodies using
  `if` / `while` / `for`, variable declarations, and `return`. Builtins
  `min` / `max` / `abs` / `assert` are available.
- Step limit 200000 (E1800); assert failure E1801; undefined variable E1803;
  index out of bounds E1804; division/modulo by zero E1805.

What fails loudly (E1000 from IR lowering, with the evaluator's reason):

- Anything depending on runtime values (function parameters, variables
  assigned at runtime, I/O, method calls such as `"abc".indexOf("b")`).
- Calls to non-`comptime` functions, calls to unknown functions (E1806).
- Unsupported statements/expressions inside comptime (E1807).
- Division/modulo by zero (E1805 → E1000), assertion failure (E1801 → E1000).

Known limitation: a `return` inside a `comptime { ... }` block is
type-checked as a return from the enclosing function, so in a `void`
function sema rejects it with `error[E1005]: Void function cannot return a
value`. Use a trailing expression statement for the block's value instead
(see the block test). `comptime` remains experimental in Edition 2026
(warning W1801, stabilized in Edition 2028); this slice does not change the
edition gating.

### Files changed (6, uncommitted — parent commits part-by-part)

| File | Change |
|---|---|
| `compiler/include/sp/ir/IRBuilder.h` | Include `sp/sema/ComptimeEvaluator.h`; member `m_comptimeEval` |
| `compiler/src/ir/IRBuilder.cpp` | `build()` constructs the evaluator + `processProgram`; `visit(ComptimeExpr)` rewritten to evaluate-and-fold |
| `compiler/include/sp/sema/ComptimeEvaluator.h` | `processProgram(const ast::Program&)` |
| `compiler/src/sema/ComptimeEvaluator.cpp` | Comptime-only registration, E1806/E1807, char + mixed int/float comparison fixes |
| `tests/ir/IRBuilderTest.cpp` | Replaced obsolete Phase 1 test `ComptimeExpressionFailsIRGeneration` (which asserted comptime *fails*) with the new suite below |
| `tests/sema/ComptimeEvaluatorTest.cpp` | 4 new evaluator tests |

No CMake changes were needed (both test files were already registered).
Version untouched (3.1.0, `compiler/include/sp/Version.h` single source of
truth). `.github/` untouched. No changes to `Parser.cpp`, the spec, or any
other slice's files.

### Tests

Targeted suite — **21/21 passed**:

| Test | What it proves |
|---|---|
| `IRBuilderTest.ComptimeArithmeticFoldsToConstant` | `comptime (1 + 2)` → IR contains `i32 3`, no `add i32` |
| `IRBuilderTest.ComptimePrecedenceFoldsToConstant` | `comptime (2 + 3 * 4)` → `i32 14`, no `add`/`mul i32` |
| `IRBuilderTest.ComptimeRecursiveFunctionFoldsToConstant` | Spec §101 example: `comptime factorial(10)` → `3628800` |
| `IRBuilderTest.ComptimeStringConcatFoldsToConstant` | `comptime ("SP " + "2.0")` → `"SP 2.0"` |
| `IRBuilderTest.ComptimeBlockFoldsToConstant` | `comptime { let a = 6; let b = 7; a * b; }` → `i32 42`, no `mul i32` |
| `IRBuilderTest.ComptimeTernaryAndLengthFoldToConstant` | `comptime ((2 > 1) ? 10 : 20)` → `i32 10`; `comptime "hello".length` → `i32 5` |
| `IRBuilderTest.ComptimeArrayIndexFoldsToConstant` | `comptime [10, 20, 30][1]` → `i32 20` |
| `IRBuilderTest.ComptimeWithRuntimeValueFailsIRGeneration` | `comptime (n * 2)` with runtime `n` → IR generation fails (nullptr) |
| `IRBuilderTest.ComptimeCallingNonComptimeFunctionFailsIRGeneration` | `comptime double(21)` (runtime fn) → fails |
| `IRBuilderTest.ComptimeDivisionByZeroFailsIRGeneration` | `comptime (1 / 0)` → fails |
| `IRBuilderTest.ComptimeWithMethodCallFailsIRGeneration` | `comptime "abc".indexOf("b")` → fails |
| `ComptimeEvaluatorTest.ComptimeFnRecursionViaProcessProgram` *(new)* | factorial(5) = 120 through `processProgram` registration |
| `ComptimeEvaluatorTest.NonComptimeFunctionNotRegistered` *(new)* | Calling a runtime fn reports E1806 |
| `ComptimeEvaluatorTest.UnknownCallReportsError` *(new)* | Calling an unknown fn reports E1806 |
| `ComptimeEvaluatorTest.CharEqualityComparesValues` *(new)* | `'a' == 'b'` is false (was wrongly true) |
| `ComptimeEvaluatorTest.*` (6 pre-existing) | Unchanged, all pass |

Two test bugs found by the first run and fixed in the tests (not the
implementation): (1) the bare forms `comptime 1 + 2` / `comptime "SP " +
"2.0"` / `comptime 1 / 0` parse as runtime binary ops outside comptime
because `comptime` binds at unary precedence — the tests now parenthesize,
with a comment in the test file; (2) a bare trailing `a * b` in a comptime
block is a parse error (`Expected ';' after expression`) — it needs the
semicolon (`a * b;`).

### Verification

Verified in an **isolated worktree** (`/tmp/sp-worktree-comptime` at
`f471a69` + only this slice's 6 files; the worktree diff is byte-identical
to this slice's hunks in the shared tree), build dir
`/tmp/sp-build-phase3-comptime-iso`, Release + `SP_BUILD_TESTS=ON`,
Clang 18.1.3. Isolation was necessary because the shared tree currently
also holds sibling slices' half-finished edits, and the shared-tree build
failed on *their* code when observed during this slice's work:
`compiler/src/ir/IRBuilder.cpp` errors at lines 234/240 (`IRType` has no
member `getName`) and 1755 (`sema::Type` has no member `isBool`, did you
mean `isBoolean`) — from the try/catch + overload-mangling slice. Those
files were not touched by this slice; the parent should re-run the
shared-tree build once the sibling slices land their fixes.

- Full Release build: **clean, 0 errors**.
- Targeted tests: **21/21 passed** (`IRBuilderTest.Comptime*` +
  `ComptimeEvaluatorTest.*`).
- Full `ctest`: **550/554 passed (99%)**. The 4 failures are all
  pre-existing / environmental, none in comptime code:
  - `ExecutionTest.StdlibTimeNativeExecution` (#256) and
    `ExecutionTest.StdlibFullIntegrationNativeExecution` (#259) — the same
    epoch-ms failures Phase 2 reported (`now_gt_0: true` not found);
    reproduced identically on rerun.
  - `NetTest.UdpSocketSendRecv` (#336) — the known hanging UDP test; it
    hung 10+ minutes and was killed so ctest could finish ("Subprocess
    terminated").
  - `JsonTest.BenchmarkOneGigabytePerSecThroughput` (#350) — throughput
    thresholds missed under load (175.9 MB/s vs 500 required) while sibling
    slices were building on the same 1.5-CPU VM; **passes on rerun**
    (517.1 MB/s, 37.3 MB/s) — flaky / load-dependent, unrelated to this
    slice (no JSON or runtime code touched).

`spc run` proofs (observed program output, not just `spc check`):

- Spec §101 example (`factorial.sp`): `comptime factorial(10)` →
  observed output **`3628800`**, exit 0. (W1801 edition warning only.)
- Combined proof (`arith.sp`): `comptime (1 + 2)`, `(2 + 3 * 4)`,
  `("SP " + "2.0")`, `((2 > 1) ? 10 : 20)`, comptime block `6 * 7`,
  `[10, 20, 30][1]`, `"hello".length` → observed output
  `3`, `14`, `SP 2.0`, `10`, `42`, `20`, `5` — all correct, exit 0.
- Runtime-dependent comptime (`runtime_fail.sp`: `comptime (n * 2)` with
  runtime `n`) → `error[E1000]` naming `Undefined variable in compile-time
  context: 'n'` plus the supported subset, exit 1.
- `comptime (1 / 0)` → `error[E1000]` (`Division by zero in compile-time
  evaluation`) plus the supported subset, exit 1.

Proof programs are in `/tmp/sp-p3-comptime-proofs/` (`factorial.sp`,
`arith.sp`, `runtime_fail.sp`, `divzero.sp`).

### Deliberately not done

- No change to the parser's `comptime` precedence (unary binding stands;
  documented above, with the parens rule).
- No `return`-as-block-value in void functions (sema E1005; see limitation).
- No comptime evaluation of `await`, I/O, method calls, generics, or
  runtime-dependent constructs — all fail loudly by design.
- No edition change: comptime stays experimental in Edition 2026 (W1801).
- No `docs/` spec change: §101 already describes this behaviour; the code
  now matches it.

---

## Parent verification + integration fixes (2026-11-09)

All four slices landed uncommitted on `feature/pro-4.0.0`. The parent
verified the combined tree from scratch (the slices could only verify
against a tree their siblings were still editing — slices A and B were
blocked by slice C's half-finished edits at the time they finished, and
slice D verified in an isolated worktree).

**Environment incident (second VM replacement):** the VM was replaced
mid-verification (the first full-build + ctest run was killed by the
restart, and `/tmp` was wiped again — build dirs and all slice proof
programs). The repo tree in `~/workspace` was intact (26 files,
+3,153/−196 incl. parent fixes); Clang was gone again and was
reinstalled (`apt-get install clang`, 18.1.3). The build was redone from
scratch and the headline proofs were re-created and re-run (see below).

### Full build + test suite (fresh build dir `/tmp/sp-build-phase3`)

- Full Release build clean, **0 errors** (`cmake -S . -B
  /tmp/sp-build-phase3 -DCMAKE_BUILD_TYPE=Release -DSP_BUILD_TESTS=ON`,
  `cmake --build /tmp/sp-build-phase3 -j2`), all binaries built
  (`spc`, `sp_tests`, `spm`, `spd`, `spls`).
- **ctest: 601/604 passed (99%)** (`-j2 --timeout 120`). The 3
  failures are all pre-existing, unchanged since Phase 2:
  `ExecutionTest.StdlibTimeNativeExecution` +
  `ExecutionTest.StdlibFullIntegrationNativeExecution` (stdlib-time
  epoch-ms truncation) and `NetTest.UdpSocketSendRecv` (known hang, now
  bounded by the 120 s timeout).
- Before the parent fixes below, the first full run was **591/604**:
  5 obsolete Phase 1 tests (throw/try-catch/map/overload-clash now
  compile by design), 1 slice-A test with wrong lambda syntax, 4 slice-B
  execution tests (map read-back + return-in-try-finally). All 10 are
  fixed below and pass now. `ObjectEmitterTest.EmitsNativeObjectFromIRText`
  failed once in the pre-fix run and passes on re-run — flaky under
  parallel ctest (shared `/tmp` path), not a code defect.

### Parent fixes (in the parent commit, not the slice commits)

1. **Map literals read back garbage — real bug, fixed.** Slice B's
   `visit(MapExpr)` created the `sp_rt_map_alloc` call with no result
   name, so the emitted IR read `call void @sp_rt_map_set_str(ptr ,
   ptr @.str.0, i64 1)` — a bare `ptr` with no value — and Clang
   rejected it (`expected value token`). Fix: pass
   `m_currentFunction->nextTempName("map_alloc")` as the call's result
   name (the same 4-arg `CallInst` form the existing `.set()`
   lowering uses). All three map execution tests pass now.
   (One-line fix inside slice B's MapExpr hunk; disclosed in slice B's
   commit message.)
2. **Sema rejected `return` inside try — real gap, fixed.** The test
   program `fn answer() -> int { try { return 42; } finally { ... } }`
   failed sema E1005 ("Missing return statement") because
   `TypeChecker::hasGuaranteedReturn` had no `TryCatchStmt` case.
   Fix: a try/catch guarantees a return iff the try body does AND
   every catch body does (a throw with no catch propagates out, which
   the analysis already counts as a guaranteed exit).
3. **Slice A's non-capturing-lambda test used wrong syntax.**
   `(int x) -> int { ... }` is rejected by the parser; lambdas use a
   fat arrow: `(int x) => { ... }` (`Parser::parseGroupedOrLambda`).
   Test fixed; the implementation was correct.
4. **Four obsolete Phase 1 tests renamed to positive tests**
   (`Throw/TryCatch/MapLiteral/OverloadedFunctionSignatureClash...
   FailsIRGeneration` → `...LowersIRGeneration`, `EXPECT_EQ(nullptr)`
   → `EXPECT_NE`). The constructs now compile by design; the tests
   assert that instead of asserting failure.

### SP program proofs (re-created after the VM wipe, all observed output)

- **Overloads:** `add(1,2)` → `3`, `add(1,2,3)` → `6`,
  `describe(20)` → `40`, `describe("hello")` → `5`.
- **`examples/27_try_catch.sp` compiles and runs again** (Phase 1 made
  it fail by design): `Result: 20`, `Error: division by zero!`,
  `Result: 0`, both finally lines, no catch lines.
- **Queue callback (the headline):** `ENQUEUED: job_1`,
  `HANDLER FIRED: job_1 send_email {"to":"alice@example.com"}`,
  `COMPLETED: 1`, `DLQ: 0` — an SP function ran as a queue handler for
  the first time. (Proof program note: `enqueue` with 2 args fails
  sema — default parameters do not satisfy overload resolution; the
  proof passes all 5 args explicitly.)
- **Async:** `await compute()` (user async fn, `delayed_value`) →
  `42`, `done`.
- **Comptime:** `comptime factorial(10)` → `3628800`,
  `comptime (2 + 3 * 4)` → `14` (recursion in a `comptime fn` works).

### Commits (part-by-part, single push at the end)

One commit per slice (A, B, C, D) + one parent commit (fixes above +
this doc), pushed once to `feature/pro-4.0.0` when complete. `main`
untouched, never merged. Two one-line parent corrections ride inside
slice commits and are disclosed in their messages: the `map_alloc`
result name (slice B's commit) and the lambda-test syntax (slice A's
commit).

### Deliberately not done (unchanged from the slice sections)

- No heap-frame state machine (async still pumps with the frame on the
  stack); no double/array/struct Task payloads; no combinators.
- No typed catches / exception hierarchy; no throwing composite values.
- No capturing lambdas, bare methods as values, generic overloads.
- **ADR-gated items untouched:** ARC memory-model completion (ADR-001),
  in-process LLVM API + LLD, WASM + playground, self-hosted compiler
  codegen, LSP depth, real debugger. ADR-001–005 remain Proposed,
  awaiting Shanto's approval.

<!-- Other Phase 3 slices append their sections below. -->
