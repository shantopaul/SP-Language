# Phase 34 (pro-4.0.0) — Slice B: Error Handling Depth

**Branch:** `feature/pro-4.0.0` (uncommitted slice work; parent commits part-by-part).
**Scope:** typed catches, composite throws, refcounted-local cleanup on throw paths.
**Deliberately not in this slice:** the Result<T,E> direction (ADR-002 pending — no user decision), exception hierarchies beyond what exists, finally semantics, the Phase 4 debugger's catch semantics (see follow-ups), the deferred-return leak.

## What was built

### 1. Typed catches

`catch` clauses now match by the thrown value's type. The first clause whose
type matches wins; a non-matching clause is skipped; no match propagates to
the enclosing catch / finally / uncaught path exactly as before.

- **Type tags.** Each function gets three new slots alongside the existing
  string payload and pending flag: a type-tag slot (`i32`), a raw-value slot
  (`i64`, bit-preserving), and an owning-flag slot (`i1`). A per-function tag
  registry maps a canonical `Type::toString()` to a small int (tag 0 =
  unknown/error). Tags are assigned lazily at the throw site and reused by
  catch clauses in the same function; the registry is never reset by
  `ensureTryState` (it is a pure name→int map, so reuse is safe).
- **Sema records the clause type.** `TypeChecker::visit(TryCatchStmt)` records
  the resolved canonical name on each clause (`CatchClause::m_resolvedTypeName`,
  mutable field + getter/setter in `compiler/include/sp/ast/Statement.h`).
  Unresolvable names resolve to `Any` there, so `catch (Exception e)` — there
  is no `Exception` type in the language — records `"any"` and is a catch-all,
  exactly as the conventional spelling suggests. `catch (Any e)` is also a
  catch-all.
- **Dispatch.** The catch block loads the tag once and tests each clause in
  order (`tag == t1 || tag == t2 || ...`). A class or interface clause matches
  its own tag **and** the tags of every subclass (enumerated from the user
  types via `ClassType::isSubtypeOf`), so `catch (Animal e)` catches a thrown
  `Dog`. Matching is on the thrown expression's **static** type: a variable
  statically typed `Animal` never matches `catch (Dog e)`, even if it holds a
  `Dog` at runtime (documented limitation — there is no runtime type tag on
  class values to consult).
- **No-match propagation.** If no clause matches, the pending flag is left
  set and control branches to the finally (if any) and then to the enclosing
  frame's catch/finally/uncaught target — the same target a throw from the try
  body would have taken. Previously the first catch always won, so a
  `catch (string e)` silently caught an `int` throw; that program now falls
  through. This is a **breaking change to Phase 3 semantics**, intended:
  `docs/SP-Syntax-Guide.md` §27 is updated to say so.

### 2. Composite throws

`throw` of array, map, struct, and class values now works through the payload
slots; the old loud E1000 for non-scalars is gone for these types.

- **Scalars** (string, integer, bool, char, float, double): as before —
  stringified for the message / catch-all binding, bit-packed into the raw
  slot for a typed binding (int: extend, bool/char: zero-extend, float:
  BitCast→i32→ZExt, double: BitCast→i64).
- **Pointer values** (arrays, maps, class objects, fn values): the raw
  pointer round-trips through the raw slot (`PtrToInt` at the throw site,
  `IntToPtr` at the catch). The stringified payload is a `"[<type>]"`
  placeholder — a raw array/map/object pointer is not a C string and must
  never reach `sp_rt_panic` as one. An uncaught array throw panics as
  `SP Panic: [int[]]`.
- **By-value user structs:** copied into a per-type struct slot (keyed by
  canonical name). A struct-returning call yields a pointer, so the throw
  site loads through it first. A catch clause for a struct type that was
  never thrown in that try body binds a default value (dead code — the clause
  can never match there); a clause for a struct type with no layout fails
  loudly, as before.
- **Result/Option values are NOT throwable — deliberate semantic restriction,
  not a technical limitation.** They are the return-value error channel
  (ADR-002 direction); throwing one would discard its variant at the
  uncaught boundary (an uncaught Result throw in a Result function would have
  to become `Err` of a placeholder string). `throw r` where `r` is a
  Result/Option stays a loud E1000 with a message pointing at `return
  Err(...)` / `?` instead. The slot machinery could carry the struct — this
  is a decision, reversible only with ADR-002.

### 3. Refcounted-local cleanup on throw paths (the leak fix)

Phase 3 documented that refcounted locals leak on throw paths. They no longer
do, on the paths this slice owns:

- **Throw-site drops.** `emitThrowValue` now drops the refcounted scopes from
  the innermost frame's entry depth up to the current scope before branching.
  Locals of enclosing scopes stay live. The throw site runs these drops
  *after* retaining the thrown object into the payload (see ownership below),
  so the drops cannot free the thrown value.
- **Finally-propagation drops.** `emitFinallyDispatch` drops the scopes from
  the propagate-target frame's entry depth when a pending throw flows through
  a finally to an outer catch. The throw site already released the inner
  try's scopes; the scopes still on the stack at the dispatch are the
  enclosing ones. The two drop ranges are disjoint and the compile-time scope
  stack is what makes them so: scopes are popped when their block ends, so a
  scope dropped at the throw site is no longer in the vector at the dispatch.
- **Ownership model.** The payload holds one owning class reference when the
  throw-owns flag is set. A directly-thrown `new` transfers its fresh count-1
  to the payload (no retain). Any other class value is retained into the
  payload at the throw site (so the unwinding drops cannot free it first).
  A matching class-clause binding takes the payload's share without retaining
  (flag set) or retains (borrowed); the catch variable is registered for
  release at catch-body end. A catch-all binding discards the object: it
  releases it if owned. The uncaught Err/None path releases the owned object
  before returning (the panic path does not — it aborts). A throw inside a
  catch body releases the abandoned catch variable via the throw-site drops,
  so **rethrow does not leak** (the earlier design note claiming it did was
  wrong — verified by re-tracing the drop ranges).
- **Bounded, documented leak that remains:** `throw makeFoo()` — a
  factory-call result has no owning variable, so its returned share is never
  released (one share per caught throw). Only direct `new` transfers.
- **Out of scope, unchanged:** the deferred-return path (a `return` inside a
  try with a finally still skips the drops — pre-existing, documented
  follow-up); the `?` operator already drops all locals but skips finally
  (pre-existing); arrays/maps are not refcounted at all (plain `sp_rt_alloc`,
  never freed anywhere in the compiler — pre-existing memory model, not a
  slice regression); an uninitialized refcounted local (`Counter c;`, no init)
  keeps the same risk profile as the existing function-end drops.

## Files changed

- `compiler/include/sp/ast/Statement.h` — `CatchClause::m_resolvedTypeName`
  (+ getter/setter).
- `compiler/src/sema/TypeChecker.cpp` — `visit(TryCatchStmt)` records the
  resolved canonical type name on each clause.
- `compiler/include/sp/ir/IRBuilder.h` — `TryFrame.refScopeDepth`; new
  members `m_throwTypeSlot` / `m_throwRawSlot` / `m_throwOwnsSlot` /
  `m_throwStructSlots` / `m_throwTypeTags` / `m_nextThrowTypeTag`; helpers
  `getOrCreateThrowTypeSlot/RawSlot/OwnsSlot/StructSlot`, `throwTypeTagFor`,
  `catchClauseTags`, `findThrowFrame`, `emitThrowScopeDrops`,
  `emitPackThrowScalar`, `emitUnpackThrowValue`; `emitThrowValue` signature
  extended to `(strVal, rawI64, typeTag, owns)`.
- `compiler/src/ir/IRBuilder.cpp` — `ensureTryState` resets the three new
  slots + clears the struct-slot map (tag registry NOT reset); the slot
  getters + tag helpers + drop/pack/unpack helpers; `getOrCreateUncaughtThrowBlock`
  restructured (Err/None built first, then a conditional owns-release via
  `uncaught_release` / `uncaught_return` blocks); `emitThrowValue` rewritten
  (stores all slots, drops unwound scopes, branches); `emitFinallyDispatch`
  gains the propagate-path drops (`finally_propagate_drops` block);
  `visit(TryCatchStmt)` rewritten (typed catch dispatch chain, per-clause
  binding: catch-all binds the string + discards an owned object, struct
  clauses load from the per-type slot, scalar/pointer clauses unpack from
  the raw slot, class clauses get retain-or-take ownership branching +
  refcount registration, `catchFrame.refScopeDepth = size - 1`, no-match block
  propagates with the pending flag still set); `visit(ThrowStmt)` rewritten
  (Result/Option tagged-union rejection first, then struct / pointer /
  scalar paths, retain-into-payload for non-`new` class values).
- `tests/ir/TryCatchTest.cpp` — `ThrowOfNonScalarFailsLoudly` replaced by
  `ThrowOfArrayLowers` / `ThrowOfMapLowers` / `ThrowOfClassLowers` /
  `ThrowOfStructLowers`; new `ThrowOfResultFailsLoudly` and
  `TypedCatchDispatchLowers` (11 → 16 tests).
- `tests/codegen/TryCatchExecutionTest.cpp` — `ThrowIntCaughtAsString`
  renamed to `ThrowIntCaughtAsInt` (typed semantics); 17 new tests:
  `TypedCatchSkipsNonMatchingClause`, `TypedCatchStringSkipsIntClause`,
  `CatchAllCatchesAnything`, `TypedCatchFallsThroughToOuter`,
  `ArrayThrowRoundTrip`, `MapThrowRoundTrip`, `ClassThrowRoundTrip`,
  `StructThrowRoundTrip`, `SubclassCatchMatches`, `BaseClassThrowSkipsDerivedClause`,
  `FloatAndBoolThrowRoundTrip`, `UncaughtArrayThrowPanicsWithPlaceholder`,
  `ThrowPathReleasesRefcountedLocals`, `ThrowOfAliasedObjectKeepsItAlive`,
  `CaughtNewObjectIsReleasedAfterCatch`,
  `UncaughtClassThrowInResultFunctionReleases`,
  `FinallyPropagationReleasesOuterLocal` (14 → 31 tests).
- `docs/SP-Syntax-Guide.md` — **NOT edited in this slice.** §27's prose is
  now stale ("no typed-exception filtering") and needs a one-paragraph
  update, but the file is mid-rewrite by another Phase 5 slice in the same
  checkout (429 insertions / 311 deletions at the time of writing); editing
  it now would collide with that rewrite. The §27 example was verified
  against the new binary (see live proofs) and still runs correctly — it now
  means what it looks like. The parent should apply the §27 prose update
  after the other slice's rewrite lands.
- `docs/v2/PHASE-34-SLICE-B.md` — this file.

## Verification

- Full Release build in `~/workspace/build-phase5-errors`: **clean, exit 0,
  zero errors** (cmake `--build ... -j2`).
- New tests: `TryCatchTest` **16/16**, `TryCatchExecutionTest` **31/31**
  (47/47 total, `./bin/sp_tests --gtest_filter='TryCatch*'`).
- Full ctest: **675/678 passed (99%), 3 failed.** Total 678 = 655 baseline
  + 22 new TryCatch tests from this slice (5 IR + 17 execution) + 1 new
  Postgres test from Slice C's concurrent work. Failures, all attributed:
  - `NetTest.UdpSocketSendRecv` — environmental, baseline failure (sandbox
    UDP EPERM; reproduces deterministically, `NetTest.cpp:72`).
  - `JsonTest.BenchmarkOneGigabytePerSecThroughput` — environmental,
    baseline failure (CPU contention on this VM; reproduces deterministically,
    `JsonTest.cpp:345/366`).
  - `ObservabilityTest.CronSchedulerIntervalExecution` — **timing-flaky
    under load, not this slice.** It schedules a 20 ms ticker, sleeps
    120 ms, expects ≥3 ticks; the scheduler thread was starved during the
    parallel full-suite run. It **passes on a quiet re-run** (0.22 s).
    This slice touches no scheduler, threading, or observability code, and
    the test involves no try/catch. (Slice C's `BrokenProgramTest.cpp` was
    not in this build at all — the test binary predates its CMakeLists
    registration — so it contributed 0 entries to the 678.)
- **Shared-tree caveat:** the build and all tests below ran against a
  working tree shared with the other Phase 5 slices (they started after
  this slice, in the same checkout). Slice C's concurrent uncommitted
  changes were present: error-recovery null guards in
  `compiler/src/parser/Parser.cpp`, `compiler/include/sp/ast/Expression.h`,
  `compiler/src/sema/TypeChecker.cpp` (disjoint regions from this slice's
  `visit(TryCatchStmt)` edit), its new `tests/sema/BrokenProgramTest.cpp`
  (registered in `tests/CMakeLists.txt`), and additions to
  `tests/runtime/PostgresTest.cpp`. None of those touch `IRBuilder` or
  try/catch lowering, and the null guards only add early returns for null
  AST children that cannot occur in valid programs — but the full-ctest
  total is therefore **not** directly comparable to the 653/655 baseline:
  Slice C added tests of its own. The TryCatch results (47/47) are this
  slice's own.
- Live proofs (via `spc run` with the final binary,
  `~/workspace/build-phase5-errors/bin/spc`, programs in
  `~/workspace/p5-errors-proofs/`):

  `typed.sp` — typed catch match / fallthrough / catch-all:
  ```
  int 42
  str oops
  outer int 7
  any 3.5
  done
  ```
  (no "WRONG" line printed — the non-matching clauses were skipped.)

  `composite.sp` — array / map / class / struct / subclass round-trips:
  ```
  arr len 3 first 10 last 30
  map a 1 size 2
  class n 5
  struct x 3 y 4
  subclass dog
  done
  ```

  `leak.sp` — throw-path cleanup, measured with the runtime live-object
  counter `sp_rt_mem_live_objects` (the `1`s after `after2` are the
  still-live local `d`, not leaks — it stays in scope to the end of `main`):
  ```
  before 0
  inside 1
  caught boom
  after 0
  after2 1
  thrown n 5
  after thrown-new 1
  inner finally
  outer caught deep
  after finally-propagation 1
  done
  ```
  The pre-slice measurement on the same program shape was `after: 1`
  (leaked); it is now `after: 0`.

  `guide27.sp` — the `docs/SP-Syntax-Guide.md` §27 example, verbatim:
  ```
  Validation: Age cannot be negative
  Cleanup complete.
  Caught int: 42
  Done.
  ```

  `result_throw.sp` — throwing a Result is a loud E1000 (exit 1):
  ```
  error[E1000]: 'throw of 'Result<any, string>'' is not yet supported in
  this phase of the SP compiler (Result/Option values are not throwable:
  they are the return-value error channel, so return Err/None (or use `?`)
  instead of throwing one) --> result_throw.sp:4:9
  IR generation failed for result_throw.sp.
  ```

## Deliberately not done

- Result<T,E> direction unchanged (ADR-002 pending).
- No exception hierarchies; no `Exception` type added (the conventional
  `catch (Exception e)` spelling is a catch-all by sema resolution to `Any`).
- Finally semantics unchanged.
- No version bump (3.1.0), no `.github/` edits, no CMake edits (no new test
  files — existing files extended).
- The Phase 4 debugger's interpreter still implements "first catch catches
  all" — it now diverges from the compiler (follow-up for the parent, not
  touched in this slice).

## Known follow-ups (for the parent)

1. **Debugger divergence** (above): the Phase 4 slice-C tree-walking
   interpreter must be taught typed dispatch + composite payloads, or its
   catch behaviour will disagree with compiled programs.
2. **Deferred-return leak**: a `return` inside a try with a finally still
   skips the scope drops (the deferred-return block deliberately emits no
   drops). Same fix shape as this slice (drop at the deferred-return block
   using the frame depth), but it is a separate, untested change.
3. **`?` operator skips finally**: it already drops all locals on the error
   path but does not run the finally body. Pre-existing.
4. **Factory-call throw leak**: `throw makeFoo()` leaks one share per caught
   throw (only direct `new` transfers). Bounded; needs a "returned value is
   owned by the throw" rule if it ever matters.
5. **Static-type-only subclass matching**: a base-typed variable holding a
   derived instance never matches a derived clause. Fixing it needs a
   runtime type tag on class values.
6. **Catch-all discards composites silently**: a catch-all catching a thrown
   array/map/object binds only the `"[<type>]"` placeholder string. The
   value is still there in the raw slot, but there is no syntax to reach it
   from a catch-all.
7. **`docs/SP-Syntax-Guide.md` §27 prose**: one paragraph to update once the
   other slice's rewrite of that file lands ("no typed-exception filtering"
   is no longer true; the example already works under the new semantics).
8. **Shared checkout**: this slice ran while Slices A and C were editing the
   same working tree (all uncommitted, same branch). The parent's part-by-part
   commit pass needs to separate the slices' hunks — notably
   `compiler/src/sema/TypeChecker.cpp` (this slice's `visit(TryCatchStmt)`
   edit vs Slice C's null guards) and `compiler/include/sp/ast/Statement.h`
   (this slice's `CatchClause::m_resolvedTypeName` vs Slice C's `hasBody` /
   `hasTryBlock`). Nothing was pushed or merged by this slice.
