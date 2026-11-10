# Phase 37 (pro-4.0.0) — Slice E: EscapeAnalysis wiring + value-semantics/Arena audit + doc gate

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `c68ba28` (slices A–D merged; parent fix `d5e5ea2` included)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, nothing pushed)
- **Scope:** last slice of ADR-001 (Hybrid Value Semantics + ARC).
  Wire the existing-but-unused escape analysis into the compiler
  pipeline, audit the value-semantics model and the Arena as built,
  and re-run the doc gate.
- **Status:** IN PROGRESS in the main tree (parent implements this slice).

## 0. Design — written before the implementation

### 0.1 What exists and what is missing (verified by reading the code)

1. **EscapeAnalysis exists, compiles, and is never run.**
   `compiler/src/sema/EscapeAnalysis.cpp` (472 lines) + header are
   in the build (`compiler/CMakeLists.txt:17`), but nothing
   instantiates the class — no IRBuilder, SemanticAnalyzer, test,
   or `spc` command references it. Its API is complete and usable:
   `analyze(program)` / `analyzeFunction(fnDecl)` build per-function
   `FunctionEscapeSummary`s; `escapes(func, var)` /
   `getEscapeState(func, var)` / `getSummary(func)` query them.
   It is unit-tested (`tests/sema/EscapeAnalysisTest.cpp`, 6 tests)
   but nothing in the compiler pipeline instantiates it. Escape
   triggers are conservative: return, store into a field /
   collection index / outer scope or global, aliasing to an escaping
   variable, return from a lambda body.
2. **The Arena is real.** `std.mem.Arena` (`stdlib/mem/mem.sp`)
   calls `sp_rt_arena_create` / `sp_rt_arena_alloc` /
   `sp_rt_arena_reset` / `sp_rt_arena_destroy`, all of which exist
   in the runtime (`Runtime.cpp:749–833`, declared in `Runtime.h`),
   and `ExecutionTest.MemoryArenaNativeExecution` passes in ctest.
   The audit below verifies rather than assumes this.
3. **The doc gate scripts exist** (`~/workspace/extract_doc_examples.py`,
   `~/workspace/run_doc_gate.py`); the last recorded result was
   119/128 RUN-OK with one known false pass (orphaned ex106 server).

### 0.2 What "wiring" means here — and what it deliberately does NOT do

**Wiring (this slice):**
- Run the analysis in `IRBuilder::build()` — after layouts, before
  `visit(program)` — and keep the `EscapeAnalysis` object on the
  builder for the build, with a getter for future phases and tests.
  It needs only the TypeChecker, available as
  `m_sema.getTypeChecker()`. The analysis is advisory: it returns
  void and cannot fail a build.
- Make the verdicts observable: extend `spc sema`
  (`tools/spc/main.cpp:885`, which already prints "Semantic
  analysis passed" + global symbols) to also print per-function
  escape summaries (`var -> LocalNonEscaping / GlobalEscaping`
  + the recorded reasons).
- Pin the verdicts with unit tests (`tests/sema/EscapeAnalysisTest.cpp`
  — the file already exists with 6 tests covering local / returned /
  field-store / index-store / aliasing; this slice ADDS the two
  uncovered triggers: global/outer-scope store and return from a
  lambda body).

**Deliberately NOT done: retain/release elision.** The analysis's
own comment says non-escaping values are "eligible for ARC
optimization", and the obvious optimization is eliding the
retain/release pair on a non-escaping local. That elision is NOT
sound under the +1 convention, and this slice does not do it:
- Init from a +1 value adopts the share and the scope drop releases
  it — the pair IS the object's lifetime management. Eliding both
  leaks (nothing would ever free it); the release is the free path.
- Init from a borrowed value retains so the local survives its
  source (which may die first — e.g. `var x = getFoo().child`,
  where the outer temp's death releases the child). The retain is
  load-bearing; escape analysis cannot prove the source outlives
  the local (that is liveness, not escape).
- The only elision that works is stack allocation (the object never
  reaches the heap, so no retain/release exists at all). That is a
  Phase-9-sized feature (alloca'd objects, destructor at scope end,
  address-never-escapes proof), not this slice.
- Consequence: a wrong verdict today costs only a wrong diagnostic
  line in `spc sema` — nothing consumes the verdicts for
  optimization yet. Any future phase that does MUST re-audit
  soundness first; this is stated here so the wiring is not mistaken
  for a safety proof.

### 0.3 Value-semantics audit (as built, after slices A–D)

Verified against the code during implementation (2026-11-10):
- Structs are by-value (copied on assignment/pass, no ARC header).
  `emitClassDestructors` resolves only `ClassType` by mangled name,
  so structs get no destructor — struct fields holding references
  leak (known gap from slice A, unchanged by this slice).
- Classes / interfaces / containers / arrays / maps are reference
  types under ARC (+1 returns, scope drops, destructors A+B,
  weak refs C, temp scopes D).
- Strings are bare `const char*` the caller keeps (never ARC, never
  freed by the runtime) — accessors must return fresh copies.
- `std.mem.Arena` is a real bump allocator, verified end to end:
  stdlib class (`stdlib/mem/mem.sp`: create/alloc/reset/destroy/
  allocatedBytes) maps to runtime fns (`Runtime.cpp:749–833`,
  declared `Runtime.h:90–95`); the stdlib's 2-arg
  `sp_rt_arena_alloc(int, int)` binds to the 3-arg runtime fn via
  the IR declaration (`IR.cpp:577–578`, default alignment); the
  `MemoryArenaNativeExecution` test passes (alloc 128+256 →
  `allocatedBytes() >= 384`, reset/destroy work). No code change.

### 0.4 Doc gate

Re-run 2026-11-10 on the merged A+B+C+D+E tree (port 8080 free
before the run): **RUN-OK 120/128 printed, 119/128 genuine** —
unchanged from the Phase 5–7 baseline. The 8 non-OK are the same
set: ex044/ex045 SKIP-CHECK-FAIL (the 2 real check failures),
ex007/ex065/ex106 RUN-TIMEOUT (servers that never exit),
ex043/ex126/ex127 RUN-FAIL on the known misleading "Unsupported
backend compiler" message. ex121's printed pass is the known
false pass: it is itself a server (times out alone, rc=124 with
port 8080 free) and passes in the gate only because ex106's
orphan server already holds port 8080 — killed pid 41093 after
the run. Slice E changed no codegen semantics, so no gate change
is expected or observed. (Gate script fix: `run_doc_gate.py` now
passes `errors='replace'` — a program printing non-UTF-8 bytes
previously crashed the script with UnicodeDecodeError.)

### 0.5 Expected outcomes

- `spc sema` on a program with a non-escaping local, a returned
  value, and a field-stored value prints the three verdicts with
  reasons.
- New `EscapeAnalysisTest` cases pass; full ctest stays at the
  merged level (836 + new tests; only UDP EPERM; PG skipped).
- Arena: no code change expected (audit only) unless the audit
  finds a gap.
- No change to codegen semantics: the analysis runs but no
  retain/release is elided and no IR changes (verify: the
  `temp_chain` / `dtor_nested` / `weak_self` / stress proofs are
  byte-identical before and after this slice).
