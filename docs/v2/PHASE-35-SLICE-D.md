# Phase 35 (pro-4.0.0) — Slice D: Verification Debt Part 3 (Doc Gate Re-run + Workaround Reverts)

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `7a44415` (Phase 6 slices A+B+C merged + parent merge fix)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, no ADR-001–005
  items touched, nothing committed or pushed)
- **Scope:** re-run the Phase 5-D doc gate against the merged A+B+C compiler,
  revert every Syntax Guide workaround that slices A/B/C fixed (proving each
  reverted example by running it), and verify the combined tree with a fresh
  build + full ctest.
- **Status:** all planned reverts done and proven; fresh build clean;
  full ctest **790/791 (99%)** — only `NetTest.UdpSocketSendRecv`
  (sandbox UDP EPERM). Doc gate **119/128 genuinely RUN-OK** (the gate
  prints 120/128 — see §1.1 for the one false pass). Two real gaps were
  found by proving, not by reading: a `??`-after-`?.` edge (§5.1) and the
  `HttpServer.get` vs `getHandler` ABI trap (§5.2). Both are documented
  here and left for a compiler/stdlib slice; neither was papered over.

## 1. Doc gate re-run

Scripts: `~/workspace/extract_doc_examples.py` +
`~/workspace/run_doc_gate.py` (Phase 5-D), against the merged compiler
(`~/workspace/build-phase6-parent/bin/spc`). Extraction covers
`docs/SP-Syntax-Guide.md` and `docs/v2/SP_2.0_LANGUAGE_SPEC.md`
(128 blocks). The gate was run three times: before edits (gate1),
after the first round of edits (gate2), and on the final docs (gate3).
All three runs agree on the accounting below.

### 1.1 Final numbers (gate3, final docs)

| | Count |
|---|---|
| Blocks extracted | 128 |
| `spc check` standalone-OK | 126 |
| `spc check` standalone-FAIL (fragments by design) | 2 |
| **RUN-OK as printed by the gate** | **120/128** |
| **Genuinely RUN-OK** | **119/128** |

The 9 non-genuine blocks are all accounted for, none is a doc bug:

- **4 timeouts by design** — ex007 (§7 Console I/O) and ex065
  (Calculator) wait on `readLine()` with no stdin in the gate
  (verified OK with piped stdin in Phase 5-D); ex106 (§85) and ex121
  (§100) are infinite serve loops (verified live with curl, §3).
- **3 no-main fragments** — ex043 (§38 `user.sp`), ex126 (spec §5.1),
  ex127 (spec §5.2): `spc run` fails at link time
  (`undefined reference to 'main'`). ex043 is verified via the §38
  project (§3.3); ex126 was run standalone with a `main` in Phase 5-D
  (`Fibonacci(10) = 55`).
- **2 check-fail skips** — ex044/ex045 (§38 service + main): cannot pass
  standalone (they import `myapp.models.user`); verified as a project
  (§3.3).

**The false pass (gate artifact, found while verifying ex121).**
The gate prints 120/128 because ex121 (§100) exits 0 in the gate run.
It does not really pass: the gate's earlier ex106 run times out after
10 s, but `spc run`'s timeout kill leaves the compiled server orphaned
(`sp_run_ex106_*` still listening on port 8080), so ex121's
`listen(8080)` fails to bind and its `wait()` returns immediately.
Killing the orphan and running ex121 alone gives the correct
`RUN-TIMEOUT(10s)` (rc=124, verified twice). The gate script kills
nothing on timeout, so any two server examples sharing a port will
always produce this artifact. Lesson: the gate's RUN-OK count for
server examples is only trustworthy after checking for orphaned
listeners (`ss -ltnp`).

**Misleading error message (pre-existing, not a regression).** The
three no-main fragments now fail with
`Compilation failed: Unsupported backend compiler '/usr/bin/clang++'.
GCC cannot compile LLVM IR...` — but the underlying error, visible in
the full output, is the link failure `undefined reference to 'main'`
(the same cause as in Phase 5-D). `spc` wraps *any* backend failure in
the "unsupported backend compiler" message, which misdirects. Not
fixed here (compiler message, out of scope); recorded for a later
slice.

**Comparison with Phase 5-D baseline:** 126/128 `spc check`, 119/128
genuine RUN-OK, same 9 accounted blocks. The gate count is unchanged
because the gate only measures "does the example run" — the slice's
real movement is that the examples themselves no longer contain
workarounds (see §2). The one genuine improvement in the raw count is
§73, which went from a ❌ section with no runnable singleton to a ✅
section whose example runs.

## 2. Workarounds reverted

Every reverted example below was **run with the merged compiler**
before the doc text was changed to match it; the quoted outputs are
real outputs. Hash-order outputs (§15, §58) were run twice to confirm
they are deterministic for the fixed program before being pasted into
the `**Output:**` blocks.

### 2.1 §15 Map iteration (slice A fix 1) — `docs/SP-Syntax-Guide.md:1197`

The "iterate a list of known keys" workaround is gone: the example now
iterates the map directly (`for (string name in ages)` /
`for (string subject in scores)`), with a note that order is
hash-table order and the map must not be mutated during iteration.
Real output (order is one run's hash order, deterministic):

```
All ages:
  Charlie: 28
  Alice: 25
  Shanto: 22

All scores:
  📚 english → 92
  📚 science → 87
  📚 math → 95
```

### 2.2 §18 ctor calling a method + method order (slice A fixes 3–4) — `:1419`

The constructor now calls `this.generateId()` directly, and
`generateId` is declared *after* the constructor — proving order
independence. The "generateId declared FIRST" comment is gone; the
`renewId()` vehicle for the workaround is kept as an ordinary public
method. Proven by the gate (ex-block runs).

### 2.3 §20 interface dispatch (slice A fix 2) — `:1637`

The polymorphic loop is back: an interface-typed variable
(`Drawable first = shapes[0]`) and `for (Drawable s in shapes) {
s.draw(); s.getColor(); }` over a `List<Drawable>`. Real output:

```
Drawing ⭕ Circle (r=5, color=Red)
First color: Red
Drawing ⭕ Circle (r=5, color=Red)
  color: Red
Drawing ▬ Rectangle (4x6, color=Blue)
  color: Blue
```

(Caught while editing: an early draft called `s.getArea()` inside the
`Drawable` loop — `getArea` is a `Resizable` method, so sema would
reject it. Fixed before proving; the gate run confirms the final form.)

### 2.4 §22 statics (slice B fix 2) — `:1820`

The whole callout is rewritten: statics are real storage now. The
example gains a static mutable counter (`Counter.total`, bare
`total = total + 1` inside an instance method) and a static-const read
from outside the class (`MathUtils.PI`). The static-init-order hazard
(declaration order) is documented. Real output:

```
Count: 3
Total increments: 3
PI: 3.14159265358979
Circle area (r=5): 78.53981633974475
5² = 25
2^10 = 1024
```

### 2.5 §26 `?.` and `??` (slice A fixes 5–6) — `:2159`

`?.` is used directly in interpolation on a null object
(`user3?.name` prints empty, no segfault) and `??` supplies the
default for a nullable field read through `?.`
(`user2?.email ?? "no-email@default.com"`). The hand-written null
checks for `??` are gone. Real output:

```
User 1 name: Shanto
User 3 name:
Email 1: shanto@email.com
Email 2: no-email@default.com
```

The explicit `if (user3 == null)` check stays for the null-*object*
name default — `??` does not fire there (see §5.1). The "no flow
narrowing" note stays: it is still true (out of scope for Phase 6).

### 2.6 §38 Modules & Imports (slice B fix 1) — `:2760`

`UserService.users` is initialized at declaration
(`private List<User> users = new List<User>();`); the constructor that
existed only to build the list is gone, and the callout paragraph about
silently-dropped initializers is removed. Verified as a project (§3.3).

### 2.7 §46 Todo App + Banking System (slice A fix 4, slice B fix 1) — `:3312`, `:3409`

- **Todo App:** `tasks` initialized at declaration; the workaround ctor
  is gone.
- **Banking:** `history` initialized at declaration; `log` moved
  *after* the constructor (proving declaration order does not matter);
  the ctor and the deposit/withdraw/transfer methods now call bare
  `log(...)` — the exact shape slice A proved is not shadowed by the
  builtin `log(double)`. (`target.log(...)` stays qualified: it is
  another instance's method.)

### 2.8 §58 Set (slice B fix 4) — `:3979`

Sets print directly (`${setA}` → `{4, 3, 5, 2, 1}`) and iterate with
`for (x in unionSet)`. The membership-loop construction of
union/intersection/difference stays — there is still no
`Set.of` / `union` / `intersect` / `difference` in the stdlib, which
is an API gap, not a compiler bug. The old size-based output is
replaced with the real run output (hash order, verified deterministic
across two runs):

```
Set A: {4, 3, 5, 2, 1}
Set B: {4, 7, 8, 5, 6}
Union: {4, 7, 3, 8, 5, 6, 2, 1}
Intersection: {4, 5}
Difference: {3, 2, 1}
Union contains 7: true
Intersection contains 4: true
Difference contains 1: true
Difference contains 4: false
Union elements:
  4
  7
  3
  8
  5
  6
  2
  1
```

### 2.9 §68 `int.parse` (slice B fix 5) — `:4401`

`int.parse` is strict now — `int.parse("abc")` panics instead of
returning 0 — so `parsePort` validates first with a small `isDigit` /
`isAllDigits` helper (substring + `==` chains; SP has no string
`<`/`>` comparison, found while proving). The old comment ("returns 0
for non-numeric input") is gone. The `**Output:**` block is also
fixed: it previously showed only one `Configuration error` line
although two inputs are `Err`. Real output:

```
Server listening on port: 8080
Configuration error: Port must be between 1 and 65535
Configuration error: Port must be a number
```

### 2.10 §71 Builder (slice B fix 1) — `:4553`

All four `RequestBuilder` fields are initialized at declaration
(including `headers = new Map<string, string>()`); the workaround ctor
is gone. The default constructor runs the initializers.

### 2.11 §72 Factory (slice A fix 2) — `:4613`

The factory now returns the interface type
(`create(string channelName) -> NotificationService`) and the caller
dispatches through it. The "returns the concrete types" comment is
gone. (Proven separately before editing: a static method returning an
interface type and a direct concrete→interface assignment both work
without an `as` cast.)

### 2.12 §73 Singleton — `:4659` — ❌ → ✅

The eager singleton works, so the section is rewritten as working:
a static field initialized with `new` runs once before `main`, and
every `getInstance()` call returns the same object. Proven first with
a standalone program (see §3.4). The lazy form is still documented as
not writable (no flow narrowing, no `!` unwrap). The old section's
`**Output:**` block was also stale (it showed `cfg1`/`cfg2` lines the
old code never printed); the new block matches the new code.

### 2.13 §74 Observer (slice A fix 2, slice B fix 1) — `:4703`

One `List<Observer>` with interface dispatch in `emit`, instead of one
list per concrete listener type; the list is initialized at declaration
and the workaround ctor is gone.

### 2.14 §75 Strategy (slice A fix 4) — `:4758`

The public method now comes first and the private strategy methods
after it — the natural order, with the "methods come first" note
replaced by "declaration order does not matter".

### 2.15 §85 HTTP server (slice A fix 7, slice C fix 7) — `:5170`

A typed fn-pointer handler is added alongside the `routeJson` routes
(`app.getHandler("/hello", hello)`, `req.text("hi typed", 200)`).
See §5.2 for why it is `getHandler` and not `get`. Live proof in §3.1.
The `wait()` documentation from slice C is untouched.

### 2.16 §87 CLI args (slice B fix 8) — `:5253`

The parser now works on `OS.args()` instead of an explicit argument
list. `spc run` does not forward extra arguments (probed: `spc run
file.sp --verbose` → `argc=0`), so the doc tells the reader to run a
built binary and shows the output for that invocation. Live proof in
§3.2.

### 2.17 §95 reduce (slice B fix 6) — `:5605`

The summing loop is replaced by the 1-arg reduce
(`squares.reduce((int acc, int x) => acc + x)`). Output unchanged
(`Even Squares sum ...: 220`).

### 2.18 §97 Mutex (slice B fix 1) — `:5679`

`value` and `lock` are initialized at declaration; the workaround ctor
is gone. (The "Thread.spawn cannot be called from SP" note stays —
true and out of scope.)

### 2.19 §99 Key-Value Store (slice B fixes 1, 3) — `:5759`

`store` is initialized at declaration; the workaround ctor is gone,
together with the paragraph justifying `HashMap` over `Map` on the
grounds that a builtin Map index-read returns a nullable — both paths
now return the plain value type.

### 2.20 §100 REST microservice (slice A fix 7, slice C fix 7) — `:5828`

The "dynamic handlers segfault" comment is replaced with a pointer to
§85 (fn-pointer handlers work there). The service itself still
serializes once and serves static JSON — a named handler cannot see
`main`'s `service` local (no capturing lambdas), so `routeJson` remains
the right tool here, not a workaround. `catalog` is initialized at
declaration; the `wait()` note from slice C is untouched. Live proof
in §3.1.

### 2.21 STDLIB_REFERENCE.md

Small verified additions (the file has no HTTP section at all — noted
in slice C §9, still true): `OS.args()` in §7 (returns `argv[1..]`,
program name excluded; `spc run` does not forward args) and a
Set/Map note in §4 (Set prints `{a, b}` and iterates; Map index-read
returns the plain value type, missing key → zero value, use
`containsKey()`). No other claims in that file were touched — its
`ArrayList`/`put` examples were not re-verified here.

### 2.22 What was checked and left alone

- **SPEC (`docs/v2/SP_2.0_LANGUAGE_SPEC.md`):** no workaround mentions
  related to the Phase 6 fixes; nothing reverted.
- **§27 typed catches, §69/§70 catch filtering:** slice C fixed the
  debugger divergence, but the guide's §70 error-code-prefix pattern
  is still the documented approach for runtime catch filtering — the
  compiler's typed catches (Phase 5-B) are same-function-only and the
  guide's shape is not a workaround for a Phase 6 fix. Left alone.
- **§85/§100 `wait()`:** already done by slice C; verified, not redone.
- **§96 Channels, §31 Closures, §34/§35/§37, §40/§41, §101–§105:**
  genuinely unimplemented features; not Phase 6 scope.

## 3. Live proofs (run, not just checked)

All run with the merged compiler
(`~/workspace/build-phase6-parent/bin/spc`); proof programs in
`~/workspace/p6d-proofs/`.

### 3.1 §85 + §100 over curl

§85 (with the `getHandler` fix):

```
$ curl -s http://127.0.0.1:8080/
{"message": "Welcome to SP Web Framework!"}          HTTP 200
$ curl -s http://127.0.0.1:8080/api/health
{"status": "healthy", "version": "1.0.0"}             HTTP 200
$ curl -s http://127.0.0.1:8080/hello
hi typed                                             HTTP 200
$ curl -s http://127.0.0.1:8080/missing
No route matched GET /missing                         HTTP 404
```

§100:

```
$ curl -s http://127.0.0.1:8080/api/products
[{"id":1,"name":"SP Compiler Enterprise","price":499},{"id":2,"name":"Language Server Suite","price":199}]   HTTP 200
$ curl -s http://127.0.0.1:8080/health
{"status": "healthy"}                                 HTTP 200
```

Both servers stayed alive across all requests.

### 3.2 §87 built binary

```
$ spc build s87.sp -o s87bin
$ ./s87bin --verbose -o program.exe -O 3
Verbose: true
Output:  program.exe
Opt:     3
$ ./s87bin
Verbose: false
Output:  out.bin
Opt:     2
```

### 3.3 §38 project (after the initializer move)

`~/workspace/p6d-proofs/modproj/` (the three §38 files extracted from
the guide): `spc check src/main.sp` passes;
`spc run src/main.sp` prints `Found: Shanto, age 21`.

### 3.4 Singleton + interface probes (before editing §§72–73)

Eager singleton (`probe_singleton.sp`):

```
c1 env: Production
c2 env: Staging
same: true
```

Interface factory (`probe_iface.sp` — static method returning the
interface type, direct concrete→interface assignment, no `as`):

```
email: hello iface
email: direct assign
```

### 3.5 §68/§26 probe (`probe_68_26.sp`)

```
8080 digits: true
abc digits: false
999999 digits: true
empty digits: false
User 1 name: Shanto
User 3 name:
Email 1: shanto@email.com
Email 2: no-email@default.com
```

(`Name 3` via `??` after `?.` on the null object prints empty — the
§5.1 gap, not a probe error.)

## 4. Verification

**Fresh build (own dir).** `~/workspace/build-phase6-docs`, Release,
`-j2`, Clang 18.1.3, local googletest copy: **exit 0, 0 errors**;
`bin/spc` and `bin/sp_tests` present. (Environment note: the build
ran while the parent's ctest used the other build dir — no contention
issues; the Json benchmark passed this run.)

**Full ctest (own build dir, no concurrent builds): 790/791 passed
(99%).** Total 791 = 735 (Phase 5 baseline) + 26 (slice A) + 16
(slice B) + 14 (slice C). The single failure is the known
environmental one, `NetTest.UdpSocketSendRecv` (sandbox UDP EPERM).
`JsonTest.BenchmarkOneGigabytePerSecThroughput` passed.

## 5. Gaps found by proving (still broken / needs a follow-up slice)

### 5.1 `??` does not fire after `?.` on a null object

`?.` on a null object yields the null *value* of the field type, but
its static type is the plain (non-nullable) field type — so `??`,
which trusts the type, returns the null value instead of the fallback:

```sp
Box? b = null;
string s = b?.label ?? "default-s";   // s == "" (expected "default-s")
int v = b?.n ?? 99;                   // v == 0  (expected 99)
```

`??` on a genuinely nullable left side works
(`string? maybe = null; maybe ?? "default-a"` → `"default-a"`), and
`?.` on a non-null object works (`s2=real`, `v2=7`). So the gap is
exactly the `?.`-on-null-object → `??` combination: the safe-nav
result is null-valued but non-nullable-typed. Probe:
`~/workspace/p6d-proofs/probe_qq_safenav.sp`. The guide (§26) avoids
the combination and documents it; the compiler fix (make `?.`
results nullable-typed, or make `??` check the runtime value) is a
compiler slice, not a doc edit. **Not fixed in D.**

### 5.2 `HttpServer.get` vs `getHandler` — same name, different ABI

`App.get(path, handler)` routes to the typed-handler ABI
(`sp_rt_http_server_route_handler` → slice A's `callHandlerFn` with
the weak-symbol vtable wrap), but `HttpServer.get(path, handler)`
routes to the 3-string ABI (`sp_rt_http_server_route`, handler
signature `(method, path, body) -> const char*`). Passing a typed
`fn(HttpRequest) -> HttpResponse` to `HttpServer.get` segfaults the
server on the first request — the handler interprets a string pointer
as an `HttpRequest` object. Found by running the first draft of the
§85 rewrite (server log: `Segmentation fault` after `/` and
`/api/health` served fine). The guide now uses `getHandler` and
documents the trap. Whether `HttpServer.get` should be aligned with
`App.get` is a stdlib design decision — changing it could break
existing 3-string `route` users — so it is flagged here, not changed.
**Not fixed in D.**

### 5.3 `spc`'s backend-failure message misdirects

Any backend failure (including a plain link error such as a missing
`main`) is reported as `Unsupported backend compiler
'/usr/bin/clang++'...`, hiding the real error. Seen on the three
no-main fragments (§1.1). Pre-existing; needs a compiler-side fix to
surface the backend's own stderr. **Not fixed in D.**

### 5.4 Gate orphans

`spc run`'s timeout kill in the gate leaves compiled server processes
running (see §1.1). The gate script should kill the process group on
timeout, or server examples should use distinct ports. Harness fix,
not a doc fix. **Not fixed in D.**

## 6. Follow-ups and limitations (carried forward)

- **§5.1, §5.2, §5.3, §5.4** above.
- **Call-result temporaries still leak one share** (slice C §9):
  `takeFoo(makeFoo())`, `makeBox().n` — needs a temporary-release pass
  over ~235 `CallInst` sites; a slice of its own.
- **Base-class field initializers do not run** (no ctor chaining);
  **fields are never released at object death**; **static-init order
  is declaration order** (documented in §22, not silently reordered);
  **generic `T`-typed field initializers are skipped** in specialized
  ctors; **constructor defaults are not implemented**.
- **Cross-function throw unwinding** (same-function-only stays
  documented), **`?.` flow narrowing / `!` unwrap**, **lazy
  singleton** (see §2.12), and **ADR-001–005** remain out of scope for
  Phase 6, as planned.
- **STDLIB_REFERENCE still has no HTTP section** (slice C §9); the
  `ArrayList`/`put` examples in §4 were not re-verified here.
- **Pre-existing untracked files** in the repo root, `app.log` and
  `notes.txt` (dated 2026-11-09 17:36, created by the §47 file-system
  examples during an earlier gate run) — left alone; not created by
  this slice.
- **Graphify:** the repo `AGENTS.md` mandates
  `python -m graphify update . --force` after edits, but
  `graphify-out/` does not exist and `graphify` is not installed in
  this environment (verified again 2026-11-09) — the step could not be
  run and was not fabricated.

## Files changed (uncommitted)

```
docs/SP-Syntax-Guide.md    20 sections reverted (§§15, 18, 20, 22, 26,
                           38, 46 Todo + Banking, 58, 68, 71, 72, 73,
                           74, 75, 85, 87, 95, 97, 99, 100) + Output
                           blocks corrected to real runs (§§15, 58, 68)
docs/STDLIB_REFERENCE.md   OS.args() (§7) + Set/Map notes (§4)
docs/v2/PHASE-35-SLICE-D.md   this file
```

`docs/SP-Syntax-Guide.md`: 441 lines changed; `docs/STDLIB_REFERENCE.md`:
+11 lines. No code, no tests, no CMake, no version bump, no `.github/`
changes, no ADR items touched, nothing committed or pushed.
