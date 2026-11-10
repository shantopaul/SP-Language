# Phase 36 (pro-4.0.0) — Slice D: Verification Debt Part 4 (Doc Gate Re-run + Workaround Reverts)

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `8c231db` (Phase 7 slices A+B+C merged + committed)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, no ADR-001–005
  items touched, nothing committed or pushed)
- **Scope:** re-run the Phase 5-D/6-D doc gate against the merged A+B+C
  compiler, revert the Syntax Guide §26 workaround now that slice A
  fixed `??` after `?.`, document the `get`/`getHandler` rule (slice B)
  where it belongs, and verify the combined tree with a fresh build +
  full ctest. Also reproduce slice C's reported `m.get("a")?.n` IR
  error on the merged binary and on the Phase 6 binary, to establish
  whether it is pre-existing.
- **Status:** §26 reverted and proven; doc gate **119/128 genuinely
  RUN-OK** (same accounting as Phase 6-D); fresh build clean; full
  ctest **805/806 (99%)** — only `NetTest.UdpSocketSendRecv`
  (sandbox UDP EPERM). The `m.get("a")?.n` error reproduces
  identically on both binaries — pre-existing, not fixed here.

## 1. Doc gate re-run

Scripts: `~/workspace/extract_doc_examples.py` +
`~/workspace/run_doc_gate.py` (Phase 5-D), against the merged
compiler (`~/workspace/build-phase7-docs/bin/spc`). Extraction covers
`docs/SP-Syntax-Guide.md` and `docs/v2/SP_2.0_LANGUAGE_SPEC.md`
(128 blocks). The gate was run twice: before edits (gate1, with the
parent's merged binary — run-only, nothing built into the parent
build dir) and on the final docs (gate2, with this slice's own
binary). Both runs agree on the accounting below.

### 1.1 Final numbers (gate2, final docs)

| | Count |
|---|---|
| Blocks extracted | 128 |
| `spc check` standalone-OK | 126 |
| `spc check` standalone-FAIL (fragments by design) | 2 |
| **RUN-OK as printed by the gate** | **120/128** |
| **Genuinely RUN-OK** | **119/128** |

The 9 non-genuine blocks are all accounted for, none is a doc bug:

- **4 timeouts by design** — ex007 (§7 Console I/O) and ex065
  (Calculator) wait on `readLine()` with no stdin in the gate;
  ex106 (§85) and ex121 (§100) are infinite serve loops (verified
  live with curl, §3).
- **3 no-main fragments** — ex043 (§38 `user.sp`), ex126 (spec §5.1),
  ex127 (spec §5.2): `spc run` fails at link time
  (`undefined reference to 'main'`). ex043 is verified via the §38
  project (§3.3); ex126 was run standalone with a `main` in Phase 5-D
  (`Fibonacci(10) = 55`).
- **2 check-fail skips** — ex044/ex045 (§38 service + main): cannot pass
  standalone (they import `myapp.models.user`); verified as a project
  (§3.3).

**The false pass (gate artifact, same as Phase 6-D §1.1).** The gate
prints 120/128 because ex121 (§100) exits 0 in the gate run. It does
not really pass: the gate's earlier ex106 run times out after 10 s,
but `spc run`'s timeout kill leaves the compiled server orphaned
(`sp_run_ex106_*` still listening on port 8080), so ex121's
`listen(8080)` fails to bind and its `wait()` returns immediately.
Verified twice in this slice: gate1's orphan (`sp_run_ex106_*`,
holding 0.0.0.0:8080) killed → ex121 alone times out (rc=124,
correct); gate2 left a fresh orphan on the same port, killed →
ex121 alone times out again (rc=124). After the kill no listener
remains.

**No change from Phase 6-D.** Gate1 (before edits) and gate2 (after
the §26 revert) print the identical 120/128 with the identical
non-OK list. The §26 edit introduced no regression: the edited
§26 block is ex031 — `spc check` rc 0, RUN-OK in gate2.

## 2. Workaround reverted: §26 `??` after `?.`

The Phase 6-D workaround was the comment plus the explicit null
check it taught:

```sp
// ?? supplies a fallback when the left side is null. It
// works on a nullable value — here a nullable field read
// through ?. — but not on the result of ?. itself when the
// OBJECT is null: that result is typed as the plain field
// type, so ?? does not see it as null. For a possibly-null
// object, check for null explicitly (below).
...
if (user3 == null) {
    println("Name 3: Unknown User");
} else {
    println("Name 3: ${user3?.name}");
}
```

Reverted (proven by running both versions with the merged compiler
first — original, then reverted, identical output except the
reverted line):

```sp
// ?? supplies a fallback when the left side is null —
// including the result of ?. when the OBJECT is null.
// (Phase 7 slice A: ?? now fires after ?. on a null object.)
...
string n3 = user3?.name ?? "Unknown User";
println("Name 3: ${n3}");       // Unknown User
```

**Live proof (merged binary, edited §26 block).**
```
User 1 name: Shanto
User 1 email: shanto@email.com
User 3 name:

Email 1: shanto@email.com
Email 2: no-email@default.com
Name 3: Unknown User

✅ Found user: Shanto
❌ User not found
```

**Value-type edge documented, not hidden.** Slice A §2 proved that
for value types null is represented as zero, so `?.` + `??` treats
a real `0` as null — exactly like plain `int?`. Verified on the
merged binary (`~/workspace/p7d-proofs/probe_qq_zero.sp`):
`null v=99 / zero w=99 / seven x=7`. The guide now says so in §26:
`user?.age ?? 99` yields 99 both when the user is null AND when age
is really 0; strings are unaffected (only a null pointer means
null, so a real empty string is never replaced by the fallback).

## 3. Live proofs (merged binary `~/workspace/build-phase7-docs/bin/spc`)

- **§26 reverted example** — see §2 above.
- **§85 curl** (B's aligned `get`): `/` 200
  `{"message": "Welcome to SP Web Framework!"}`,
  `/api/health` 200 `{"status": "healthy", "version": "1.0.0"}`,
  `/hello` (typed fn via `HttpServer.get`) 200 `hi typed`,
  `/missing` 404. Server stayed alive through all four requests.
- **§100 curl:** `/api/products` 200 (both products),
  `/health` 200 healthy, `/missing` 404.
- **§87 built binary:** `spc build` + `./sec87_bin --verbose -o
  program.exe -O 3` → `Verbose: true / Output: program.exe /
  Opt: 3`.
- **§38 project:** `spc check` passes, `spc run` →
  `Found: Shanto, age 21`.
- **Slice A re-confirmed on the merged binary:** the Phase 6-D probe
  (`~/workspace/p6d-proofs/probe_qq_safenav.sp`) →
  `a=default-a / s=default-s / v=99 / s2=real / v2=7`; the
  method-call proof (`~/workspace/p7a-proofs/probe_safenav_call.sp`)
  → `s=fallback-greet / v=99 / after-null-shout / s2=hi Shanto /
  v2=42 / SHOUT / s3=LOUD hi Mina`.
- **Slice C re-confirmed on the merged binary:**
  `temp_arg.sp` → `before 0 / take 42 / after 0`;
  `temp_member.sp` → `before 0 / n 5 / after 0`;
  `temp_chain.sp` → `before 0 / chain 9 / after chain 1 / arg 9 /
  after arg 2`.

## 4. The `get`/`getHandler` rule — where it is documented

Slice B already updated §85 completely: the example registers the
typed handler with `app.get("/hello", hello)`, and the comment
states `get` and `getHandler` both take typed handlers
(`fn(HttpRequest) -> HttpResponse`), `getHandler` remains the
explicit alias, and `route()` is the low-level 3-string entry
point — "Do not register a typed handler through `route()`."
Nothing was missing in the guide, so nothing was duplicated or
changed there.

`STDLIB_REFERENCE.md` was **not** extended, deliberately: it covers
only 7 modules (`std/io`, `std/math`, `std/string`,
`std/collections`, `std/fs`, `std/time`, `std/os` &
`std/process`) and has no HTTP section at all (pre-existing gap,
noted in Phase 6-C §9). Adding the rule there would mean creating
a whole `std/http` reference section, which is a separate docs
task, not a one-rule note. The rule lives in guide §85 and in the
code comments in `stdlib/http/http.sp`.

## 5. `m.get("a")?.n` — reproduced, pre-existing, not fixed

Slice C §3 reported that `m.get("a")?.n` on a map-loaded class
value fails with an IR type error (`%map_get9` defined as `i64`
but expected `ptr`). Reproduced in this slice with
`~/workspace/p7d-proofs/probe_map_safenav.sp` (a `Map<string, Box>`
holding `new Box(42)`, then `m.get("a")?.n ?? 99`):

- **Phase 6 binary** (`~/workspace/build-phase6-parent/bin/spc`):
  ```
  /tmp/sp_ir_...tmp.ll:676:57: error: '%map_get19' defined with type 'i64' but expected 'ptr'
      %fld_ptr20 = getelementptr inbounds %class.Box, ptr %map_get19, i32 0, i32 1
  ```
- **Merged Phase 7 binary** (`~/workspace/build-phase7-docs/bin/spc`):
  identical error, same shape (`%map_get19`, line 680).

So the bug is **pre-existing** (present before Phase 7), not a
regression from slice A or C, and not fixed by slice A: safe-nav
on a value loaded from a map still lowers the `i64` raw-bits map
value as if it were a pointer. The `?.` field access expects a
`ptr` base; `Map.get` returns the stored value as `i64`. It
belongs to slice A's territory (safe-nav completion) but was not
in A's scope. **Follow-up for the parent**, not a D fix. Also
re-confirmed in passing: the misleading `spc` "Unsupported backend
compiler" message wraps this real IR error (Phase 6-D §5.3, still
present).

## 6. Verification

Fresh Release build in `~/workspace/build-phase7-docs` (`-j2`,
Clang 18.1.3, googletest bypass
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=…/build-phase4-parent/_deps/googletest-src`).
Build clean, **0 errors**. (Build took ~23 min wall-clock — the
parent was building in `build-phase7-parent` in parallel on shared
1.5 CPUs.)

**Full ctest: 805/806 (99%)** — total 806 = 791 (Phase 6) + 4
(slice A) + 1 (slice B) + 10 (slice C).

| Test | Result | Cause |
|---|---|---|
| `NetTest.UdpSocketSendRecv` | Failed | Sandbox UDP EPERM (environmental, pre-existing) |
| `PostgresB1.PgLiveStringArrayRoundTrip` | Skipped in the full run, **PASSED alone in 303 ms** | No server was running when ctest ran: this VM had been restarted since Phase 6, so PostgreSQL was installed but not started and the `sp_test` DB/user did not exist. Started the service, created the scratch DB/user (throwaway local credentials, never committed), re-ran the single test — PASSED. The task brief's "PostgreSQL is accepting connections" was stale for this VM. |

`JsonTest.BenchmarkOneGigabytePerSecThroughput` passed (no
contention failure in this run). No other failures.

## 7. Still broken / found by proving (not fixed — out of D scope)

1. **`m.get("a")?.n` IR type error** (§5) — safe-nav on a
   map-loaded class value (`%map_get19` i64 vs ptr). Pre-existing,
   reproduced on both binaries.
2. **Chained safe-nav `a?.b.c`** still segfaults on a null
   intermediate (slice A §8 — verified by A on both binaries, not
   re-run here).
3. **Generic `List`/`Map` methods** (`List_int_map`, `isEmpty`)
   fail IR verification for plain calls too (slice A §8,
   pre-existing, identical on both binaries).
4. **Gate orphans** (§1.1) — a timed-out server example leaves its
   compiled server listening, which can false-pass a later example
   on the same port. Same mechanism as Phase 6-D, reproduced twice
   here.
5. **Misleading `spc` backend message** (§5) — any backend failure
   (missing `main`, IR type error) is wrapped in "Unsupported
   backend compiler '/usr/bin/clang++'" (pre-existing).
6. Slice C's remaining leak sites (its doc §6): async temporaries,
   short-circuit RHS / ternary-arm temporaries, field shares of
   dropped temporaries (no destructors), container-retained shares,
   self-referential cycles, `main`'s locals — all deliberately not
   done, per that doc.

## 8. Files changed (all uncommitted in the main tree)

| File | Change |
|---|---|
| `docs/SP-Syntax-Guide.md` | §26 only: `??`-after-`?.` workaround reverted (comment + `if (user3 == null)` → `user3?.name ?? "Unknown User"`), value-type zero edge documented (+12/−11) |
| `docs/v2/PHASE-36-SLICE-D.md` | This doc (new) |

Pre-existing untracked `app.log` / `notes.txt` (from an earlier
gate run) left alone. The repo `AGENTS.md` Graphify protocol
(`python -m graphify update . --force`) could not be run:
`graphify-out/` does not exist and `python -m graphify` is not
installed (verified repeatedly in Phases 5–7). Nothing fabricated.

## 9. Follow-ups for the parent

- `m.get("a")?.n` IR type error (§5) — safe-nav needs to handle
  map-loaded (i64 raw-bits) class values; slice-A territory.
- Gate-orphan false pass (§1.1) — the gate should kill the whole
  process group on timeout, or run server examples on distinct
  ports.
- The misleading `spc` backend message (§5) — report the real
  backend error instead of "Unsupported backend compiler".
- A `std/http` section for `STDLIB_REFERENCE.md` (§4) — the
  reference covers only 7 modules; `get`/`getHandler`/`route`
  currently live only in guide §85 and code comments.
