# Phase 36 (pro-4.0.0) — Finish what Phase 6 found by proving

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `328eee0` (Phase 6 / Phase 35 done, pushed)
- **HEAD:** `9c09fc7` (this doc is committed on top of it)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, no ADR-001–005 items touched)
- **Scope:** the three concrete gaps Phase 6 found by proving (not by
  reading), plus the verification debt that follows them. Slice A:
  safe-nav completion (`??` after `?.` + `?.` method calls). Slice B:
  `HttpServer.get` vs `getHandler` ABI alignment. Slice C:
  call-result temporary leak (temp-release pass). Slice D:
  verification debt part 4 (doc gate re-run + workaround reverts).
- **Status:** all 4 slices done and merged; combined Release build
  clean; full ctest **804/806 (99%)** in the parent run (only UDP +
  a Json-benchmark flake) and **805/806** in slice D's independent
  run (only UDP). Doc gate **119/128 genuinely RUN-OK** (same
  accounting as Phase 6-D). All live proofs re-run on the combined
  binary and passing (see §6).

## Commits (part-by-part, single push at phase end)

| Commit | Slice | Files | Diff |
|---|---|---|---|
| `70ff1a5` | A — safe-nav completion | 3 | +537/−1 |
| `6eab36d` | B — HttpServer.get vs getHandler ABI alignment | 4 | +364/−19 |
| `8c231db` | C — call-result temporary leak | 4 | +1118/−8 |
| `9c09fc7` | D — doc gate re-run + revert §26 workaround | 2 | +283/−11 |
| **Total `328eee0..HEAD`** | | **10 files** | **+2302/−39** |

Remote `main` still `d10e1c2` (never merged, never touched). Slice
details live in `docs/v2/PHASE-36-SLICE-A.md` (279 lines),
`PHASE-36-SLICE-B.md` (189), `PHASE-36-SLICE-C.md` (437),
`PHASE-36-SLICE-D.md` (271). This doc summarizes them and records
the merge + parent verification.

## Deliberately NOT in Phase 7 (approved with the plan)

- **Cross-function throw unwinding** — needs its own design decision.
- **`?.` flow narrowing + `!` unwrap** — a type-system feature, needs design.
- **Ctor chaining / base-class field initializers, static-init order
  hazard** — design decisions, still parked.
- **ADR-001–005** items (ARC completion, in-process LLVM, WASM,
  self-hosted codegen) — still pending Shanto's approval.

---

# Slice A — Safe-nav completion (2 fixes)

**Commit `70ff1a5`, 3 files (+537/−1). Built in the main tree.
ctest 794/795 in the slice (only UDP). 4 new execution tests.
Proofs in `~/workspace/p7a-proofs/`.**

## 1. `??` after `?.` on a null object now fires

**What was wrong.** Safe-nav yields the null *value* but reported
the field's *non-nullable* type (`m_lastExprType = fieldType`), so
`??` took its no-op path. Probe (Phase 6-D):
`b?.label ?? "default-s"` → `""`, `b?.n ?? 99` → `0`.

**Fix decision (measured, not guessed).** Chose (b) — the `??`
lowering treats a safe-nav LHS (field access, or a call through
one) as nullable at runtime — over (a) making `?.` report a
nullable sema type. (a) was measured and rejected:
`NullableType::isAssignableTo` returns false for plain targets, so
`string s = b?.label;`, `println(b?.label)`, `return b?.label;`
would all become new `spc check` errors. (b) touches only the `??`
lowering and leaves `spc check` semantics intact. A blanket
runtime check was also rejected: plain `int x = 0; x ?? 99` must
stay `0` — pinned by a test.

**Live proof (probe, new binary).**
```
a=default-a
s=default-s
v=99
s2=real
v2=7
```

**Known representation limit (documented, not introduced by this
fix):** nullable int `0` is null in this compiler's convention, so
`b?.n ?? 99` with `n == 0` yields `99` — same as plain
`int? m = 0; m ?? 99`.

## 2. `?.` method calls no longer segfault on a null receiver

**What was wrong.** `obj?.method()` segfaulted: the member-call
path in `visit(CallExpr)` had no null check (Phase 6-A's own code
comment flagged it).

**Fix.** The guard wraps the vtable + direct/static dispatch, with
a local `safeNavFinish` lambda funneling the 4 value-producing
exits through the merge block. A diamond around the whole
~1,500-line member-call body cannot work — every sub-path
C++-returns out of `visit()`. Error exits (`reportUnsupported`)
left alone (they abort compilation). Async exits converted too.

**Live proof (`probe_safenav_call.sp`).**
```
s=fallback-greet
v=99
after-null-shout
s2=hi Shanto
v2=42
SHOUT
s3=LOUD hi Mina
```

Null receiver yields fallbacks + void skip; non-null dispatches
normally; **virtual dispatch works** (`LOUD hi Mina`).

## Still broken (pre-existing, verified identical on the Phase 6 binary)

- **Chained safe-nav `a?.b.c`** segfaults on a null intermediate.
- **Generic `List`/`Map` methods** (`List_int_map`,
  `List_int_size`, `isEmpty`, `Mutex_lock`) fail IR verification
  even for plain non-null calls — independent of safe-nav.
- **Builtin member paths:** class-name-gated ones (Mutex/Thread/…)
  are unreachable for nullable receivers (the `?` suffix breaks
  the name check) and fall through to the guarded dispatch; `Task`
  methods are reachable but the runtime handles a null handle
  gracefully (`cancelled=false` on both binaries).
- **`m.get("a")?.n`** (slice C's report, reproduced by slice D on
  both binaries): safe-nav on a map-loaded class value fails with
  an IR type error (`%map_get19` defined as `i64` but expected
  `ptr`) — see slice D §5. Follow-up, slice-A territory.

---

# Slice B — HttpServer.get vs getHandler ABI alignment

**Commit `6eab36d`, 4 files (+364/−19). Built in worktree
`~/workspace/sp-language-review/sp-language-p7-b` (detached at
`328eee0`). ctest 789/792 in the slice (UDP + Json benchmark +
a timing-flaky Observability cron). 1 new test
(`HttpServerGetTypedHandler`). Proofs in `~/workspace/p7b-proofs/`.**

## What was wrong

`App.get` routes to the typed-handler ABI while `HttpServer.get`
routes to the 3-string route ABI — a typed fn handler passed to
`HttpServer.get` segfaults the server on first request (Phase 6-D
§5.2). Verified in code: `HttpServer.get(path, handlerFnPtr)`
called `this.route("GET", path, handlerFnPtr)` →
`sp_rt_http_server_route`, whose handler type is
`const char* (*)(method, path, body)` — it calls the handler with
3 string pointers and reads the return as a C string, so a typed
`fn(HttpRequest) -> HttpResponse` reads the method string as an
`HttpRequest` object and segfaults. `RouteGroup.get` had the
identical trap (`sp_rt_http_group_route`); `WebGroup.get` was
already typed.

## Fix

`HttpServer.get/post/put/patch/deleteRoute/options/head` (and
`RouteGroup.get/...`) now call `routeHandler` (typed ABI) instead
of `route`; `getHandler/...` unchanged. `HttpServer.route` stays
as the low-level 3-string entry point, with a comment stating the
split. No runtime change was needed.

**Caller audit (examples/, tests/, docs/): zero 3-arg handler
lambdas, zero SP-level `.route(` calls** — no SP caller uses the
3-string ABI. All SP handlers are 1-arg `(ptr req)` lambdas or
typed fns, already running through the typed ABI via
`App`/`WebGroup`. C++-level `sp_rt_http_group_route` use
(`HttpTest.cpp:641`) and C++ `server.route` are unaffected.

**Syntax Guide §85** trap note removed; the example now uses
`app.get("/hello", hello)`; `getHandler` noted as the explicit
alias, `route()` as the 3-string API. Updated example passes
`spc check`.

## Live proofs (baseline vs fixed, same program)

- **Baseline (pre-fix binary, port 18773):** first `curl /hello`
  → `Segmentation fault`, server DEAD (proves the test catches
  the real bug).
- **Fixed binary:** `/hello` → `hi typed` 200, `/echo`
  (`(ptr req)` lambda) → `{"echo":true}` 200, `/hi2`
  (getHandler) → `hi typed` 200, `/api/v1` (RouteGroup.get) →
  `hi typed` 200, `/` → JSON 200, `/missing` → 404, server
  ALIVE afterwards.

## Deliberately unfixed

`HttpServer.route` deliberately keeps the 3-string ABI
(documented — not a trap anymore since `get` no longer routes
there).

---

# Slice C — Call-result temporary leak (temp-release pass)

**Commit `8c231db`, 4 files (+1118/−8). Built in worktree
`~/workspace/sp-language-review/sp-language-p7-c` (detached at
`328eee0`). Targeted tests 11/11; full ctest 800/801 in the slice
(only UDP). 10 new tests + 1 updated expectation.
Proofs in `~/workspace/p7c-proofs/`.**

## Ownership rule (as implemented)

- **OWNED (+1):** a value produced by `new`, or by a call
  sema-resolved to a non-async user function/method returning a
  Class/Interface type (`isFreshPlusOneExpr` +
  `isReferenceType()`). Recorded exactly once, at production —
  via an RAII guard (`TempCallRecorder`, destructor runs on every
  exit path of the ~4,000-line `visit(CallExpr)`) and a tail hook
  in `visit(NewExpr)`.
- **BORROWED:** everything else (variable/field loads, params,
  `this`, scalars, strings, arrays/maps, unresolved calls, async
  Task results). Never recorded, never released.
- **Release is safe because no `<Class>_destructor` is ever
  generated** — dropping a temporary never releases its fields,
  so a child loaded from a temporary stays alive. Every adopter
  removes its value from the pending list before the scope pop.

## Design change found while implementing: containers retain, they don't adopt

Containers store elements as raw i64 bits with no share. Verified
live that `xs.push(makeFoo())`, `let ys = [makeFoo()]`, and
`m.set("a", makeFoo())` all read fine on the baseline only
because the +1 leaks — releasing the temp would leave a dangling
pointer. Worse, the indirect case `store(xs, makeFoo())` (a
function pushing its borrowed parameter) cannot be fixed by
adopting at all. So `emitContainerElementRetain` emits
`sp_rt_retain` on reference-typed elements at every raw-bits store
(push/add/enqueue incl. PQ, `set` for Map/List/Array,
array-literal elements, indexed assignment, `Channel.send`). The
container then owns a share and the temp pop correctly releases
the caller's +1. This also fixes the pre-existing dangling pointer
when a *variable's* element outlives the variable.

**Field initializers differ:** the field code retains unless the
initializer is `new`, so only `new` initializers are adopted; a
factory-call initializer's +1 is released by the scope pop —
`box strong 2` → `1` (verified live with `sp_rt_strong_count`).

## Live proofs (new binary; baselines in the slice doc §1)

- `temp_arg` `takeFoo(makeFoo())` → `after 0` (was 1);
  `temp_member` `makeBox().n` → `after 0` (was 1).
- `temp_stored` / `temp_returned` unchanged (`inside 1 /
  after block 0`); `temp_borrowed` unchanged (`base 2 /
  borrowed 7 / borrowed short 7 / direct 7 / direct short 7 /
  after 2`) — no UAF.
- `temp_chain` → `after chain 1 / after arg 2` (was 2/4 — outer
  temps dropped, inner field shares remain); `temp_self`
  unchanged, no crash (`inside 1 / after block 1 / after take 2`).
- `temp_fieldinit` → `box strong 1` (was 2);
  `temp_throwpath` → `after 0` (was 1); `ret_temp_leak.sp` →
  `after free temp 2` (was 3).
- `temp_container` (push / indirect store / array literal / map
  set): live 1/2/3/4, all reads 42 — identical to baseline;
  `temp_channel` → `after send live 1`, identical to baseline.
- `temp_shortcircuit` (`after and 1 / after or 2 / after tern 3`)
  and `temp_async` (`after await 1`) unchanged — leaks remain by
  design.

## Remaining leak sites (all deliberate, documented in the slice doc §6)

Async-function temps (recording off; await splits blocks).
Short-circuit RHS / ternary-arm temps (defining block doesn't
dominate the merge). Field shares of dropped temporaries (no
destructors — needs ARC completion). Loop iterables /
switch-match subjects (adopted, live past the release point).
Unresolved-call temps (convention unknown).
Container-retained shares leak forever (containers have no
free/destructor). Self-referential cycles (`after block 1`).
Mid-statement throw path — unreachable for same-function throws;
cross-function throws abort the process by design (verified:
same-function string throw caught, cross-function panics).

## Still broken / found while working (not this slice)

`m.get("a")?.n` on a map-loaded class value fails with an IR type
error (`%map_get9` defined as `i64` but expected `ptr`) —
pre-existing safe-nav bug, slice A's territory (see slice D §5).

---

# Slice D — Verification debt part 4 (doc gate + reverts)

**Commit `9c09fc7`, 2 files (+283/−11). Worked in the main tree
(uncommitted until the parent committed it). Own build dir
`~/workspace/build-phase7-docs`. ctest 805/806 in its own build
(only UDP) — Json benchmark passed there; PG live PASSED alone in
303 ms.**

## 1. Doc gate re-run (merged A+B+C compiler)

Scripts `~/workspace/extract_doc_examples.py` +
`~/workspace/run_doc_gate.py` (Phase 5-D), against the merged
compiler. Run twice (before edits, on the final docs) — both agree.

| | Count |
|---|---|
| Blocks extracted | 128 |
| `spc check` standalone-OK | 126 |
| `spc check` standalone-FAIL (fragments by design) | 2 |
| **RUN-OK as printed by the gate** | **120/128** |
| **Genuinely RUN-OK** | **119/128** |

The 9 non-genuine blocks are all accounted for, none is a doc bug:
4 timeouts by design (2 stdin, 2 server loops); 3 no-main
fragments; 2 check-fail skips verified as the §38 project. **The
false pass (same as Phase 6-D §1.1):** ex121 (§100) exits 0 only
because the gate's earlier ex106 timeout leaves an orphaned server
holding port 8080, so ex121's `listen(8080)` fails and `wait()`
returns at once. Verified twice in this slice (orphan killed →
ex121 alone times out, rc=124, correct; gate2 left a fresh orphan,
same result). **No change from Phase 6-D.**

## 2. Workaround reverted: §26 `??` after `?.`

The Phase 6-D workaround (comment + explicit null check) replaced
by `string n3 = user3?.name ?? "Unknown User";` — proven by
running both versions with the merged compiler first (identical
output except the reverted line). The value-type zero edge is
documented, not hidden: `user?.age ?? 99` yields 99 both when the
user is null AND when age is really 0; strings are unaffected
(verified: `null v=99 / zero w=99 / seven x=7`).

## 3. Live proofs (merged binary)

- §26 reverted example — `Name 3: Unknown User` (+ the rest of
  the block unchanged).
- §85 curl (B's aligned `get`): `/` 200, `/api/health` 200,
  `/hello` (typed fn via `HttpServer.get`) 200 `hi typed`,
  `/missing` 404, server alive.
- §100 curl: `/api/products` both products, `/health` healthy.
- §87 built binary: `--verbose -o program.exe -O 3` →
  `Verbose: true / Output: program.exe / Opt: 3`.
- §38 project: `spc check` passes, `spc run` →
  `Found: Shanto, age 21`.
- Slice A re-confirmed on the merged binary (probe →
  `a=default-a / s=default-s / v=99 / s2=real / v2=7`; method-call
  proof → `... / SHOUT / s3=LOUD hi Mina`).
- Slice C re-confirmed on the merged binary (`temp_arg` →
  `before 0 / take 42 / after 0`; `temp_chain` →
  `before 0 / chain 9 / after chain 1 / arg 9 / after arg 2`).

## 4. The `get`/`getHandler` rule — where it is documented

Slice B already updated §85 completely (example registers the
typed handler with `app.get("/hello", hello)`; comment states
`get` and `getHandler` both take typed handlers,
`getHandler` remains the explicit alias, `route()` is the
low-level 3-string entry point). Nothing was missing, so nothing
was duplicated. `STDLIB_REFERENCE.md` was **not** extended,
deliberately: it covers only 7 modules and has no HTTP section
at all (pre-existing gap, noted in Phase 6-C §9). Adding the rule
there would mean creating a whole `std/http` reference section —
a separate docs task.

## 5. `m.get("a")?.n` — reproduced, pre-existing, not fixed

`Map<string, Box>` holding `new Box(42)`, then
`m.get("a")?.n ?? 99`:

- **Phase 6 binary:** `/tmp/sp_ir_...tmp.ll:676:57: error:
  '%map_get19' defined with type 'i64' but expected 'ptr'`
- **Merged Phase 7 binary:** identical error, same shape
  (`%map_get19`, line 680).

Pre-existing (present before Phase 7), not a regression from
slice A or C, and not fixed by slice A: safe-nav on a value
loaded from a map still lowers the `i64` raw-bits map value as if
it were a pointer. The `?.` field access expects a `ptr` base;
`Map.get` returns the stored value as `i64`. Follow-up for the
parent (slice-A territory). Also re-confirmed in passing: the
misleading `spc` "Unsupported backend compiler" message wraps
this real IR error (Phase 6-D §5.3, still present).

---

# Merge summary (parent)

Slices ran in parallel from the phase base `328eee0`: A in the
main tree, B in worktree
`~/workspace/sp-language-review/sp-language-p7-b`, C in worktree
`~/workspace/sp-language-review/sp-language-p7-c`, D in the main
tree after the merge. B and C arrived as patches
(`~/workspace/p7-B.patch`, `~/workspace/p7-C.patch`); the merge
scratch lives in `~/workspace/p6-merge/`'s successor pattern
(`~/workspace/p7-*.patch`).

## A merge

A worked in the main tree and was committed first (`70ff1a5`,
pure A — verified on disk before committing: exactly
`IRBuilder.cpp` + `CodegenCrashersExecutionTest.cpp` + the slice
doc; probe + method proof re-run on A's binary).

## B merge

B's patch applied **cleanly** onto the A tree (`git apply --check`
exit 0, zero conflicts) — even `CodegenCrashersExecutionTest.cpp`,
which A had also extended (different regions). Committed as
`6eab36d` — but the commit initially swept in the two pre-existing
untracked scratch files `app.log` / `notes.txt` (Phase 6-D gate
artifacts, `git add -A`). Caught on review before any push:
`git rm --cached` + `git commit --amend --no-edit`, leaving the
files untracked on disk and the commit with exactly B's 4 files.
Lesson recorded: never `git add -A` in this tree — name the files.

## C merge

C's patch applied **cleanly** onto the A+B tree (`git apply
--check` exit 0, zero conflicts) — even `IRBuilder.cpp`, which A
had also modified (safe-nav region vs temp-scope regions).
Committed as `8c231db`.

---

# Parent verification (combined tree, HEAD `9c09fc7`)

Fresh Release build in `~/workspace/build-phase7-parent` (`-j2`,
Clang 18.1.3, googletest via
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=…/build-phase4-parent/_deps/googletest-src`).
Build clean, 0 errors, `spc`/`spd`/`spls`/`spm` present.

## Full ctest — 804/806 (99%) in the parent run

```
99% tests passed, 2 tests failed out of 806
```

Expected total: 791 (Phase 6) + 4 (A) + 1 (B) + 10 (C) = **806**.

| Test | Result | Cause |
|---|---|---|
| `NetTest.UdpSocketSendRecv` | Failed | Sandbox UDP EPERM (pre-existing, environmental) |
| `JsonTest.BenchmarkOneGigabytePerSecThroughput` | Failed | CPU contention flake (passed in slice B's, C's and D's own runs; known flaky under parallel load) |
| `PostgresB1.PgLiveStringArrayRoundTrip` | Skipped in ctest (no server at run time — this VM had been restarted, Postgres down) — see live proof below | — |

Slice D's independent fresh build + ctest in
`~/workspace/build-phase7-docs` gave **805/806** (only UDP;
Json passed there). All 15 new tests pass in the combined run
(verified by name: the 4 A tests, B's `HttpServerGetTypedHandler`,
and C's 10 temp tests incl. `CallResultTemporariesAreReleased`,
`BorrowedGetterTempDoesNotFreeEarly`,
`ContainerStoresRetainTempElements`).

## Live proofs re-run on the final binary (parent, not the slices)

All slice proof programs re-run against
`~/workspace/build-phase7-parent/bin/spc`. Every output below is
from the combined binary.

**Slice A** — probe → `a=default-a / s=default-s / v=99 /
s2=real / v2=7`; method proof → `... / SHOUT / s3=LOUD hi Mina`.

**Slice B** — `wait_server.sp` (port 18099, blocked in `wait()`)
→ `curl /` = `{"message": "Welcome to SP Web Framework!"}`,
`curl /api/health` = `{"status": "healthy", "version": "1.0.0"}`;
B's `HttpServerGetTypedHandler` test passed in the combined run
(the segfault-vs-200 proof is B's, run on its own binaries).

**Slice C** — `temp_arg.sp` → `take 42 / after 0`;
`temp_member.sp` → `n 5 / after 0`; `temp_borrowed.sp` →
`direct 7 / direct short 7 / after 2` (no UAF);
`temp_container.sp` → reads 42, live counts as baseline.

**Phase 5/6 regression checks** — `p5-openapi-probe.sp` →
`openapi_registered: true`; `p5-async-proof/p4_double.sp` →
`pi=3.14`. (One transient run of both probes printed a
"file exists and is accessible" help line; both passed cleanly
on immediate re-run — not reproduced, not a code change.)

**Parent-confirmed still-broken (D §5, reproduced on the combined
binary).** `probe_map_safenav.sp`:
```
error: '%map_get19' defined with type 'i64' but expected 'ptr'
error generated.
```

**Live Postgres** — PostgreSQL 16 was wiped by a VM restart
mid-phase (third time across Phases 5–7) and reinstalled
(`apt-get install -y postgresql-16`, scratch DB `sp_test`,
throwaway local credentials, never committed). One install step
waited on a runtime approval card; the server was already up by
the time the parent checked (`pg_isready` → accepting
connections) and the `sp_test` role/DB already existed:
```
[ RUN      ] PostgresB1.PgLiveStringArrayRoundTrip
[  PASSED  ] 1 test. (295 ms)
```

## Environment incidents (Phase 7)

- **VM restart #3 (mid-Phase 7, during slice C):** wiped apt
  Clang again. Slice C recovered 18.1.3 from the local apt cache
  via `dpkg-deb -x` into `~/workspace/clang-extracted`
  (symlinks `clang`/`clang++` → `-18` there); `/usr/bin/clang`
  was present again by parent verification time. Slice C's
  baseline build took ~35 min wall-clock (three slice builds
  competing on shared 1.5 CPUs).
- **Postgres down at parent ctest time** (restart) — the live
  test skipped in ctest and was run separately after reinstall
  (see above). Slice D hit the same stale-state problem in its
  own run and started the service itself.
- One of slice B's background builds was lost mid-build at 30%
  (no error, no process left); restarted incrementally and
  completed clean. One of slice D's cleanup execs got SIGTERM'd
  by its own `pkill -f` pattern matching its own command line
  (known trap — kill exact PIDs, never `pkill -f <pattern>` from
  the same shell).

---

# Known follow-ups (not Phase 7)

1. **`m.get("a")?.n` IR type error** — safe-nav on a map-loaded
   class value lowers the `i64` raw-bits map value as a `ptr`
   (proven pre-existing on both binaries). Slice-A territory;
   the natural next fix.
2. **Chained safe-nav `a?.b.c`** segfaults on a null intermediate
   (slice A §8, pre-existing).
3. **Generic `List`/`Map` methods** fail IR verification for plain
   calls too (slice A §8, pre-existing).
4. **Remaining temp-leak sites** (slice C doc §6, all deliberate):
   async-function temps, short-circuit RHS / ternary-arm temps,
   field shares of dropped temporaries (needs ARC completion),
   loop iterables / switch-match subjects, unresolved-call temps,
   container-retained shares (containers have no free/destructor),
   self-referential cycles, mid-statement throw path.
5. **Nullable int `0` is null** by this compiler's convention —
   `b?.n ?? 99` with `n == 0` yields `99` (documented in §26).
6. **Doc-gate orphans** (timeout-killed servers stay listening
   and can false-pass a later example) + the misleading `spc`
   "Unsupported backend compiler" message on any backend failure
   (both pre-existing).
7. **No `std/http` section in `STDLIB_REFERENCE.md`** — the
   reference covers only 7 modules; the `get`/`getHandler`/`route`
   rule lives only in guide §85 and code comments (separate docs
   task).
8. **Cross-function throw unwinding, `?.` narrowing + `!` unwrap,
   ctor chaining, static-init order hazard** — still deferred by
   design (need design decisions).
9. **ADR-001–005 still pending** — ARC completion, in-process
   LLVM, WASM, self-hosted codegen untouched by design.
