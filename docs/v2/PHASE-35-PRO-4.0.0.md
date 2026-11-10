# Phase 35 (pro-4.0.0) — Fix the gaps Phase 5-D's doc gate found

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `0c5ebbe` (Phase 5 / Phase 34 done, pushed)
- **HEAD:** `17f397c` (this doc is committed on top of it)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, no ADR-001–005 items touched)
- **Scope:** Phase 5 slice D's doc gate found 12 real compiler/runtime gaps
  (plus 3 follow-ups from Phase 5: debugger typed-catch divergence,
  `Task::cancel()` not cancelling the timer, the `analyze()` contract
  question). Phase 6 fixes them in 4 slices: A codegen crashers, B
  statics + init semantics, C async + debugger + robustness leftovers,
  D verification debt part 3 (doc gate re-run + workaround reverts).
- **Status:** all 4 slices done and merged; combined Release build clean;
  full ctest **790/791 (99%)** — only `NetTest.UdpSocketSendRecv`
  (sandbox UDP EPERM). Doc gate **119/128 genuinely RUN-OK** (same
  accounting as Phase 5-D). All live proofs re-run on the combined
  binary and passing (see §7).

## Commits (part-by-part, single push at phase end)

| Commit | Slice | Files | Diff |
|---|---|---|---|
| `7de7fd7` | A — codegen crashers | 17 | +2387/−163 |
| `4c1691b` | B — statics + init semantics | 16 | +1874/−29 |
| `e1d904b` | C — async + debugger + robustness leftovers | 21 | +1417/−192 |
| `7a44415` | Parent fix — repair the B-merge resolution of `IRBuilder.cpp` | 1 | +23/−134 |
| `17f397c` | D — doc gate re-run + revert 20 guide workarounds | 3 | +771/−222 |
| **Total `0c5ebbe..HEAD`** | | **42 files** | **+6336/−604** |

Remote `main` still `d10e1c2` (never merged, never touched). Slice
details live in `docs/v2/PHASE-35-SLICE-A.md` (595 lines),
`PHASE-35-SLICE-B.md` (562), `PHASE-35-SLICE-C.md` (510),
`PHASE-35-SLICE-D.md` (541). This doc summarizes them and records the
merge + parent verification.

## Deliberately NOT in Phase 6 (approved with the plan)

- **Cross-function throw unwinding** (throw in a called function caught
  by the caller's `try`) — real stack unwinding, needs its own design
  decision. Same-function-only stays documented.
- **`?.` flow narrowing + `!` unwrap** — a type-system feature, needs
  design.
- **ADR-001–005** items (ARC completion, in-process LLVM, WASM,
  self-hosted codegen) — still pending Shanto's approval.

---

# Slice A — Codegen crashers (7 fixes)

**Commit `7de7fd7`, 17 files (+2387/−163). Built in the main tree.
ctest 760/761 in the slice (only UDP). 26 new tests (12 IR
`CodegenCrashersIRTest` + 14 execution `CodegenCrashersExecutionTest`).
19 proof programs in `~/workspace/p6a-proofs/`.**

## 1. Map iteration

**What was wrong.** Every non-List iterable was treated as an array, so
`sp_rt_array_length` read the map's entry-table pointer as a length and
the loop never terminated, yielding garbage keys forever.

**Fix.** Runtime `sp_rt_map_str_key_at` / `sp_rt_map_int_key_at` +
a `ForInStmt` map branch in `IRBuilder.cpp`. Iteration order is table
order (unspecified) — do not mutate the map while iterating.

**Live proof (`p1_map_iter.sp`).**
```
key=Alice
key=Shanto
count=3
DONE
```

## 2. Interface-typed method calls

**What was wrong.** Calling a method through an interface-typed
variable was an IR-generation failure.

**Fix.** Dispatch via the vtable. Proven with `List<Drawable>` loops
and generic interfaces (`p2_iface.sp`, `p2c_generic_iface.sp`,
`p2d_super_iface.sp`).

**Live proof (`p2_iface.sp`).**
```
color=red
Circle r=1 color=blue
Rect 2x3 color=green
DONE
```

## 3. Constructor calling a method

**What was wrong.** A ctor calling a method on `this` segfaulted.

**Fix.** Ctor-time `this` is now a valid dispatch target
(`p3_ctor_method.sp`, `p3b_ctor_bare.sp`).

**Live proof (`p3_ctor_method.sp`).**
```
id=user-123 name=Shanto
DONE
```

## 4. Method order + bare method calls

**What was wrong.** The builtin `log(double)` shadowed same-named
methods in sema (E1001): a method called before its declaration, or a
bare unqualified call, was rejected or misrouted.

**Fix.** In sema, the method path runs before the builtin special
cases, and only when sema resolved the call to a method (`isMethod`) —
a same-signature builtin still wins when sema picked it. IR follows
sema's resolved target. (`p4_method_order.sp`, `p4b_bare_call.sp`,
`p4c_private_order.sp`, `p4d_strategy_order.sp`.)

## 5. `??` no longer emits invalid LLVM IR

**What was wrong.** The null-coalescing operator emitted `add ptr` —
invalid IR.

**Fix.** `??` now lowers through a proper null check + select.
(`p5_null_coalesce.sp`, `p5b_null_int.sp`, `p5c_null_ret.sp`.)

**Live proof (`p5_null_coalesce.sp`).**
```
name=default
name2=hello
v=42
DONE
```

## 6. `?.` on null no longer segfaults

**What was wrong.** Safe navigation on a null object segfaulted
instead of short-circuiting to the null value.

**Fix.** `?.` evaluates the base once, null-checks it, and yields the
null value of the result type. (`p6_null_conditional.sp`.)

**Live proof (`p6_null_conditional.sp`).**
```
User 1 name: Shanto
User 3 name:
DONE
```

**Known edge (found by slice D, §5):** `??` *after* `?.` on a null
object does not fire (`b?.label ?? "default"` → `""`,
`b?.n ?? 99` → `0`). Safe-nav yields a null value with a
non-nullable type, so `??` trusts the type. Plain `??` on a nullable
works. Documented in the Syntax Guide (§26); a compiler fix is a
follow-up, not a Phase 6 item.

## 7. Fn-pointer HTTP handlers

**What was wrong.** Passing a named function / typed fn as an HTTP
handler failed at link time (`undefined reference to
_vtable_HttpRequest` — a weak-symbol vtable problem).

**Fix.** Weak-symbol vtable fix + typed-handler ABI in
`NetBridge.cpp` / `Http.cpp`. (`p7_http_named_fn.sp`,
`p7b_http_typed_fn.sp`, `p7c_vtable_sym.sp`.)

## Deliberately unfixed (slice A doc §9)

- Map iteration order is unspecified (table order).
- Interface dispatch covers the vtable path only; exotic cases
  (interface-typed values stored in generic containers) are not
  proven.

---

# Slice B — Statics + init semantics (8 fixes)

**Commit `4c1691b`, 16 files (+1874/−29). Built in worktree
`~/workspace/sp-language-review/sp-language-slice-b` (detached at
`0c5ebbe`). ctest 750/751 in the slice (only UDP; the Json benchmark
flaked once under contention, passed on the final run). 16 new tests
(`StaticsExecutionTest`). 14 proof programs in
`~/workspace/p6b-proofs/`.**

## 1. Field initializers with `new` run in every constructor

**What was wrong.** A field initializer was never evaluated:
`sp_rt_arc_alloc` zeroes the payload, the constructor body ran, and
the field stayed null.

**Fix.** `emitFieldInitializers` runs in declaration order at the
start of every constructor form (default / explicit /
specialized-generic for non-type-param fields).
(`p1_field_init.sp`, `p1b_generic_field_init.sp`,
`p1c_generic_explicit_ctor.sp`.)

**Deliberately unfixed:** base-class field initializers do not run
(no ctor chaining — design decision); generic `T`-typed field
initializers are skipped in specialized ctors; ctor defaults not
covered.

## 2. Statics

**What was wrong.** `MathUtils.PI` segfaulted: the old path lowered
the bare class name to `ConstantInt 0`.

**Fix.** Static fields live in module globals `@<Class>_<field>`
(registered in `VTableBuilder`, inherited statics visible via the
derived name); `__sp_static_init_<Class>()` is called from `main` in
declaration order; `emitMemberPointer` intercepts `ClassName.field`
(and `this.field` where the field is static) **before** the object is
evaluated. Bare static read/assign inside the class works.
(`p2_statics.sp`, `p2b_bare_static.sp`, `p2c_inherited_static.sp`.)

**Live proof (`p2_statics.sp`).**
```
env=Production
bump1=1
bump2=2
counter=2
```

**Deliberately unfixed:** static-init order is class declaration
order — a cross-class read of a later class sees zero (documented
C++-style hazard).

## 3. Map index-read returns the plain value

**Decision:** `HashMap.get()` semantics win over the nullable
alternative. A missing key reads back zero — use `containsKey()` to
test presence. (`p3_map_index.sp`.) The old Syntax Guide §99
paragraph saying otherwise is obsolete (reverted by slice D).

## 4. Set

**What was wrong.** `Set` printed as an opaque object and was not
iterable.

**Fix.** `sp_rt_set_get_i64` / `sp_rt_set_get_str`,
`sp_rt_set_to_string`; `for-in` over a set works via the index
accessors. (`p4_set.sp`, `p4b_interp.sp`.)

**Live proof (`p4_set.sp`).**
```
size=3
elem=3
elem=2
elem=1
```

(Set order is hash order — documented, deterministic across runs.)

## 5. `int.parse` is strict

**What was wrong.** `int.parse("abc")` silently returned 0.

**Fix.** Trim, base 10, full consumption; invalid input panics naming
the input. `bool.parse` deliberately unchanged.
(`p5_parse.sp`.)

**Live proof (`p5_parse.sp`).**
```
ok=42
SP Panic: int.parse: invalid integer 'abc'
Aborted
```

## 6. `reduce()`

**What was wrong.** The 1-arg `List.reduce` overload did not exist;
the accumulator was typed `any`.

**Fix.** 1-arg `List.reduce` overload registered; the accumulator is
the element type. (`p6_reduce.sp` → `sum=55, joined=abc`;
`p6b_untyped.sp`, `p6c_chain.sp`, `p6d_onearg.sp`, `p6e_array.sp`,
`p6f_arr1.sp`.) A 1-arg reduce needs an element-typed accumulator —
documented.

## 7. Method default args

**What was wrong.** `r.text("hello")` on a method with a defaulted
second parameter failed with E1001.

**Fix.** Sema attaches the method AST decl to candidates
(`MemberMethod::astDecl` via `NameResolver`); IR
`appendDefaultArgs` fills omitted trailing defaults at all 4
method-call sites (member calls, `this` calls, unqualified
enclosing-method calls, static calls). Defaults must be trailing.
(`p7_defaults.sp`, `p7b_defaults_obs.sp`.)

**Live proof (`p7_defaults.sp`).**
```
hi Shanto!
hi Shanto?
hello
hello
```

## 8. `OS.args()` returns the real argv

**What was wrong.** `OS.args()` returned an empty array even for a
built binary run with arguments.

**Fix.** `main` now takes `(i32 %argc, ptr %argv)` and forwards them
via `sp_rt_set_args`; `sp_rt_os_args` builds the array from the deep
copy (`argv[1..]` — program name excluded). 4 existing tests updated
for the new `main` signature (`IRBuilderTest` ×2,
`LLVMGeneratorTest` ×2). (`p8_args.sp`.)

**Live proof (final binary, built with `spc build`, run as
`./p8_args_bin hello world`).**
```
argc=2
arg[0]=hello
arg[1]=world
```

## Doc impact (for slice D — all applied)

§68 `parsePort` now panics; §22 statics real; §58 Set; §87
`OS.args()`; §95 reduce; §99 map-index paragraph obsolete; §38 /
Todo / Banking / §71 / §74 / §97 field initializers.

---

# Slice C — Async + debugger + robustness leftovers (7 fixes)

**Commit `e1d904b`, 21 files (+1417/−192). Built in worktree
`~/workspace/sp-language-review/sp-language-slice-c` (detached at
`0c5ebbe`). ctest 748/749 in the slice (only UDP); 14 new tests
(735→749 in its tree); fresh build clean. Proof programs in
`~/workspace/p6c-proofs/`.**

## 1. Debugger typed catches

**What was wrong.** The Phase 4 tree-walking interpreter stringified
every thrown value into `InterpThrow.message` and let the first
`catch` win regardless of its type name — while the compiler has had
typed catches since Phase 5 slice B.

**Fix.** `InterpThrow` now carries the thrown value;
`typedCatchMatches` mirrors the compiler's Phase 5-B rules (first
match wins; `catch (Exception e)` / `catch (Any e)` are catch-alls;
a catch-all binds the stringified value, a typed clause binds the
value). 3 new DAP tests.

## 2. `Task::cancel()` cancels the timer

**What was wrong.** Cancelling a task left its timer running — the
cancel tests took ~10 s on stale timers.

**Fix.** `onComplete` hooks in all 4 timer factories +
`Scheduler::m_cancelMtx`. 5 cancel tests now pass in **10 ms**.

## 3. `analyze()` contract

**What was wrong.** `analyze()` returned `true` for parse-error
programs when sema added no errors — and `BuildOrchestrator` could
codegen a broken AST.

**Fix.** `analyze()` now returns `errorCount() == 0` (false on
parse-error programs). LSP side effects intact; the
codegen-on-broken-AST hole is closed. 26 `BrokenProgramTest`
assertions pin the contract, 29/29.

## 4. Async leak edges

- **(a) String finally-return: non-issue.** SP strings are not ARC
  objects (live count 0). No code change.
- **(b) Cancel-resume fixed.** `on_complete_resume` 5th arg is the
  task itself + a shared atomic exactly-one-resume guard + `cancelBB`
  with suspend-time drops. Proof: `live 1 → live 0`
  (`async_cancel_frame.sp` → `cancelled=true / live 0`).
- **(c) Cancelled `all()` frees its results array + `Str` slots**
  (C++ behavior test; memory-counter verification impossible —
  `sp_rt_free` does not decrement `g_allocated_bytes`).

**Live proof (`async_finally_ret.sp`).**
```
v=by
live 0
```

## 5. `throw makeFoo()`

**What was wrong.** A factory call as a throw operand (or return /
assignment operand) double-retained: the callee returned +1 and the
receiver retained again.

**Fix.** +1 return convention at **8 sites** (ReturnStmt ×2, VarDecl,
Assign, Throw, short-form ×4 — short-form previously never retained,
needed so borrowed getters do not break).

**Live proof (`throw_factory.sp`).**
```
before 0
caught n 5
after throw 0
inside block 1
after block 0
done
```

**Live proof (`return_convention.sp`).**
```
7
7
5
9
5
5
```

## 6. Single-statement await

**What was wrong.** `if (c) x = await getVal();` and
`while (n > 0) n = await getVal() - 42;` failed with E1000 ("must be
inside a block").

**Fix.** `scanBranch` wraps non-block branch bodies in a synthesized
`BlockStmt` for hoisting. (A match-arm wrap was considered and
removed — the parser never creates statement arm bodies, so it would
have been dead code.)

**Live proofs.**
```
await_single_if.sp  → if x=42 / while n=0
await_single_if2.sp → if await ok / while await n=0
```

## 7. `listen()` stays non-blocking; `wait()` documented + fixed

**Decision: keep `listen()` non-blocking, document `wait()`.**
`HttpServer::listen()` binds + starts worker/accept threads and
returns `bool`. The Syntax Guide §§85/§100 now teach `app.wait()`.

**Real bug found + fixed while proving it:** `wait()` + `stop()` from
another thread hung (double-join UB on the same `std::thread`) —
fixed with `m_acceptJoinMtx`. C++ test 151 ms; live curl proof
(`/` and `/api/health` served, `wait()` blocks).

**Live proof (`wait_server.sp`, port 18099, blocked in `wait()`).**
```
curl /           → {"message": "Welcome to SP Web Framework!"}
curl /api/health → {"status": "healthy", "version": "1.0.0"}
```

## Deliberately unfixed (slice C doc §9)

- **Call-result temporary still leaks 1 share** (`takeFoo(makeFoo())`,
  `makeBox().n` — the fix needs a temporary-release pass over ~235
  `CallInst` emission sites; separate slice).
- `main` does not drop its own locals; named Task handle box leak;
  `STDLIB_REFERENCE` has no HTTP section (all pre-existing).

---

# Slice D — Verification debt part 3 (doc gate + reverts)

**Commit `17f397c`, 3 files (+771/−222). Worked in the main tree
(uncommitted until the parent committed it). Own build dir
`~/workspace/build-phase6-docs`. ctest 790/791 in its own build
(only UDP) — identical to the parent's combined run.**

## 1. Doc gate re-run (merged A+B+C compiler)

Scripts `~/workspace/extract_doc_examples.py` +
`~/workspace/run_doc_gate.py` (Phase 5-D), against the merged
compiler. Run three times (before edits, after the first round, on
the final docs) — all three agree.

| | Count |
|---|---|
| Blocks extracted | 128 |
| `spc check` standalone-OK | 126 |
| `spc check` standalone-FAIL (fragments by design) | 2 |
| **RUN-OK as printed by the gate** | **120/128** |
| **Genuinely RUN-OK** | **119/128** |

The 9 non-genuine blocks are all accounted for, none is a doc bug:
4 timeouts by design (2 stdin, 2 server loops); 3 no-main fragments;
2 check-fail skips verified as the §38 project. **The false pass:**
ex121 (§100) exits 0 in the gate only because the gate's earlier
ex106 timeout leaves an orphaned server holding port 8080, so
ex121's `listen(8080)` fails and `wait()` returns at once. Verified:
orphan killed → ex121 alone times out (rc=124, correct).

Also found (pre-existing, flagged not fixed): `spc` wraps *any*
backend failure (e.g. a missing-`main` link error) in a misleading
"Unsupported backend compiler '/usr/bin/clang++'" message.

## 2. Workarounds reverted (20 guide sections + STDLIB_REFERENCE)

§15 map iteration, §18 ctor-calls-method + method order, §20
interface dispatch (incl. `List<Drawable>` loop), §22 real statics
(mutable counter + `MathUtils.PI` from outside), §26 `?.`/`??`, §38 +
§46 Todo/Banking + §71/§74/§97/§99 field initializers at
declaration, §58 Set prints + iterates, §68 strict `int.parse` with
validation, §72 interface-typed factory return, **§73 Singleton ❌ →
✅** (eager form proven working), §75 natural method order, §85
fn-pointer handler, §87 `OS.args()`, §95 1-arg reduce, §100 comment
fix. Output blocks corrected to real runs (§§15, 58, 68).
STDLIB_REFERENCE: `OS.args()` + Set/Map notes. SPEC: nothing to
revert. Every reverted example was run with the merged compiler
first; hash-order outputs verified deterministic across runs.

## 3. Live proofs

- §85 curl: `/` 200, `/api/health` 200, `/hello` (typed fn handler)
  200 `hi typed`, `/missing` 404, server stayed alive.
- §100 curl: `/api/products` both products, `/health` healthy.
- §87 built binary: `--verbose -o program.exe -O 3` →
  `Verbose: true / Output: program.exe / Opt: 3`.
- §38 project: `spc check` passes, `spc run` →
  `Found: Shanto, age 21`.

## 4. Still broken / found by proving (not fixed — out of D scope)

1. **`??` after `?.` on a null object doesn't fire** (§5.1, see
   slice A §6 above). Probe
   `~/workspace/p6d-proofs/probe_qq_safenav.sp`. §26 documents it
   and avoids the combination.
2. **`HttpServer.get` vs `getHandler` ABI trap** (§5.2): `App.get`
   routes to the typed-handler ABI (slice A fix), but
   `HttpServer.get` routes to the 3-string route ABI — a typed
   handler segfaults the server on first request (D's first §85
   draft proved this). The guide now uses `getHandler` and
   documents the trap; aligning the two is a stdlib design decision
   for the parent.
3. Gate orphans (§1.1) and the misleading `spc` backend message
   (§5.3).

---

# Merge summary (parent)

Slices ran in parallel from the phase base `0c5ebbe`: A in the main
tree, B in worktree `~/workspace/sp-language-review/sp-language-slice-b`,
C in worktree `~/workspace/sp-language-review/sp-language-slice-c`,
D in the main tree after the merge. B and C arrived as patches
(`~/workspace/p6-B.patch`, `~/workspace/p6-C.patch`); the merge
scratch lives in `~/workspace/p6-merge/`.

## B merge

B's patch initially failed on 3 files vs the A-dirtied tree
(`IRBuilder.cpp`, `TypeChecker.cpp`, `tests/CMakeLists.txt`); the
other 13 files applied. To commit a pure A first, the parent reversed
B's hunks (`git apply -R --include=<file>`) on the 6 contaminated
files (`VTableBuilder.h`, `IRBuilder.h`, `VTableBuilder.cpp`, `IR.cpp`,
`Runtime.h`, `Runtime.cpp` — verified B markers `sp_rt_set_args` /
`__sp_static_init` gone), committed A, then re-applied B forward.
`TypeChecker.cpp` and `tests/CMakeLists.txt` merged cleanly via
`git merge-file` (exit 0, no markers).

The one remaining file, `compiler/src/ir/IRBuilder.cpp`, needed a
3-way merge (`git merge-file -p`, base = `0c5ebbe` version) →
`~/workspace/p6-merge/irb-merged.cpp` with **5 conflicts**, resolved
by hand (script `~/workspace/p6-merge/resolve_irb.py`):

1. **ForInStmt decls** — A added `isMap`, B added `isSet`. **Kept
   both.**
2. **ForInStmt length call** — A: `isMap ? sp_rt_map_size :
   (isList ? sp_rt_list_size : sp_rt_array_length)`; B: `isList ?
   sp_rt_list_size : (isSet ? sp_rt_set_size : sp_rt_array_length)`.
   **Combined 4-way ternary.**
3. **ForInStmt element fetch** — A's `if (isMap) {…}` block vs B's
   `if (isSet) {…}` block. **Chained:** `if (isMap) {A} else if
   (isSet) {B} else if (isList) {…}`.
4. **Enclosing-method dispatch** — A side empty vs B side adding the
   "Check if callee is a method on 'this'" block. **Took B's side.**
   (This was the wrong call — see the parent fix below. A had
   *moved* the block earlier in the function; B re-added it at the
   old position.)
5. **`emitMemberPointer`** — A: 3-arg form (`preEvaluatedObject` for
   safe-nav) + `nullValueOf` helper; B: 2-arg form + static-field
   intercept. **Kept A's helper + 3-arg signature, with B's static
   intercept first** (it must run before the object is evaluated).
   The header already defaults the 3rd parameter to `nullptr`, so
   B's 2-arg call sites compile unchanged.

## C merge

C's patch applied **cleanly** onto the A+B tree (`git apply --check`
exit 0, zero conflicts) — even `IRBuilder.cpp` and `NetBridge.cpp`.

## Parent fix (`7a44415`) — two resolution bugs the combined build exposed

Each slice built clean alone; the combined build failed twice, and
both failures were in the hand-resolved `IRBuilder.cpp`:

1. **Duplicated `if (isSet) {` opener.** The chained fetch branch
   (conflict 3) kept B's `if (isSet) {` line after the inserted
   `} else if (isSet) {`, unbalancing the braces and breaking every
   definition after line 1938. **One line removed.**
2. **Duplicated enclosing-method dispatch.** Conflict 4's "take B"
   duplicated the whole dispatch block: A's moved version (with the
   `isMethod` guard, running before the builtins) at ~7173 and B's
   version at ~7999, redeclaring `enclosingSlotIdx`. **Deleted B's
   duplicate block (133 lines) and ported its one real addition —
   the `appendDefaultArgs` fill for omitted method default args —
   into A's block**, so a method called unqualified from its class
   still gets its defaults filled. A's position + guard win (a
   same-signature builtin must win when sema picked it).

After the fix the combined Release build is clean (0 errors).

---

# Parent verification (combined tree, HEAD `17f397c`)

Fresh Release build in `~/workspace/build-phase6-parent` (`-j2`,
Clang 18.1.3, googletest via
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=…/build-phase4-parent/_deps/googletest-src`).
Build clean, 0 errors, `spc`/`spd`/`spls`/`spm` present.

## Full ctest — 790/791 (99%)

```
99% tests passed, 1 tests failed out of 791
```

Expected total: 735 (Phase 5) + 26 (A) + 16 (B) + 14 (C) = **791**.
The only failure is the environmental one:

| Test | Result | Cause |
|---|---|---|
| `NetTest.UdpSocketSendRecv` | Failed | Sandbox UDP EPERM (pre-existing, environmental) |
| `PostgresB1.PgLiveStringArrayRoundTrip` | Skipped in ctest (no server at run time) — see live proof below | — |

Slice D's independent fresh build + ctest in
`~/workspace/build-phase6-docs` gave the **same 790/791**.

## Live proofs re-run on the final binary (parent, not the slices)

All slice proof programs re-run against
`~/workspace/build-phase6-parent/bin/spc`. Every output below is
from the combined binary.

**Slice A** — `p1_map_iter.sp` → `key=Alice / key=Shanto / count=3 /
DONE`; `p2_iface.sp` → `color=red / Circle r=1 color=blue /
Rect 2x3 color=green / DONE`; `p3_ctor_method.sp` →
`id=user-123 name=Shanto / DONE`; `p5_null_coalesce.sp` →
`name=default / name2=hello / v=42 / DONE`;
`p6_null_conditional.sp` → `User 1 name: Shanto / User 3 name: /
DONE`.

**Slice B** — `p2_statics.sp` → `env=Production / bump1=1 / bump2=2 /
counter=2`; `p4_set.sp` → `size=3 / elem=3 / elem=2 / elem=1`;
`p7_defaults.sp` → `hi Shanto! / hi Shanto? / hello / hello`;
`p5_parse.sp` → `ok=42` then `SP Panic: int.parse: invalid integer
'abc'`; `p8_args.sp` built with `spc build`, run as
`./p8_args_bin hello world` → `argc=2 / arg[0]=hello / arg[1]=world`.
(`spc run` does not forward args — `argc=0` there is expected, not a
regression; B's proof is the built binary.)

**Slice C** — `await_single_if.sp` → `if x=42 / while n=0`;
`await_single_if2.sp` → `if await ok / while await n=0`;
`return_convention.sp` → `7 / 7 / 5 / 9 / 5 / 5`;
`throw_factory.sp` → `before 0 / caught n 5 / after throw 0 /
inside block 1 / after block 0 / done`; `async_cancel_frame.sp` →
`cancelled=true / live 0`; `async_finally_ret.sp` → `v=by / live 0`;
`wait_server.sp` (port 18099, blocked in `wait()`) →
`curl /` = `{"message": "Welcome to SP Web Framework!"}`,
`curl /api/health` = `{"status": "healthy", "version": "1.0.0"}`.

**Phase 5 regression checks** — `p5-openapi-probe.sp` →
`openapi_registered: true`; `p5-async-proof/p4_double.sp` →
`pi=3.14`; `p5-async-proof/p7_allrace.sp` → `all len=2 first=1
second=2 / race=2 / dbl=2.5`.

**Live Postgres** — PostgreSQL 16 was wiped by a VM restart mid-phase
and reinstalled (`apt-get install -y postgresql-16`, scratch DB
`sp_test`, throwaway local credentials, never committed):
```
[ RUN      ] PostgresB1.PgLiveStringArrayRoundTrip
[       OK ] PostgresB1.PgLiveStringArrayRoundTrip (316 ms)
[  PASSED  ] 1 test.
```

**Parent-confirmed still-broken (D §5.1, reproduced on the combined
binary).** `probe_qq_safenav.sp`:
```
a=default-a     (plain ?? works)
s=              (b?.label ?? "default-s" — should be "default-s")
v=0             (b?.n ?? 99 — should be 99)
s2=real         (non-null ?. works)
v2=7
```

## Environment incidents (Phase 6)

- **VM restart #2 (~22:43, mid-Phase 6):** wiped `/tmp`, apt Clang,
  Postgres; SIGTERM-killed the parent's slice-A ctest re-run (A's
  760/761 is agent-reported, **not** parent-verified — the combined
  790/791 run above is the real gate). Clang 18.1.3 recovered (slice C
  via `dpkg-deb -x` from the local apt cache; parent confirmed
  `clang --version` at merge time). Postgres reinstalled for the live
  proof above. All patches, proofs and build dirs live under
  `~/workspace/` (home fs persists); nothing was lost.
- A system security notice flagged a `ps` output (it contained the
  runtime's internal replay shell-script text) as failing a
  prompt-injection check — **nothing in that output was acted on**;
  the issue became moot when clang reappeared.

---

# Known follow-ups (not Phase 6)

1. **Call-result temporary leaks 1 share** (`takeFoo(makeFoo())`,
   `makeBox().n`) — needs a temporary-release pass over ~235
   `CallInst` emission sites; separate slice.
2. **Base-class field initializers / ctor chaining** — design decision
   (no chaining yet).
3. **Static-init cross-class order hazard** — declaration order; a
   read of a later class sees zero (documented).
4. **`??` after `?.` on a null object doesn't fire** — safe-nav yields
   a null value with a non-nullable type (proven, §7 above).
5. **`HttpServer.get` vs `getHandler` ABI trap** — typed handler
   through `HttpServer.get` segfaults the server; the guide uses
   `getHandler` and documents the trap; aligning the two is a stdlib
   design decision.
6. **Cross-function throw unwinding** — still deferred by design
   (needs its own design decision).
7. **`?.` flow narrowing + `!` unwrap** — still deferred (type-system
   feature).
8. **Doc-gate orphans** (timeout-killed servers stay listening and can
   false-pass a later example) + the misleading `spc` "Unsupported
   backend compiler" message on any backend failure (pre-existing).
9. **`main` doesn't drop its own locals; named Task handle box leak;
   `STDLIB_REFERENCE` has no HTTP section** (pre-existing).
10. **ADR-001–005 still pending** — ARC completion, in-process LLVM,
    WASM, self-hosted codegen untouched by design.
