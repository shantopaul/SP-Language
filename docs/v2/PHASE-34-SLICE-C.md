# Phase 34 (pro-4.0.0) — Slice C: compiler robustness

> Slice C of Phase 5. Work is UNCOMMITTED (the parent commits part-by-part).
> Branch `feature/pro-4.0.0` @ `5228c14` (Phase 4 HEAD); nothing pushed,
> nothing merged to main; version stays 3.1.0; no `.github/` changes.

## What was built

### 1. TypeChecker null-safety audit

`SemanticAnalyzer::analyze()` runs NameResolver (declaration-only, already
safe) then `TypeChecker::check()`. The checker dereferenced AST children
that the parser's error recovery can leave null — `if;` segfaulted on the
null `IfStmt` condition, and 17 more programs segfaulted the same way.
Every such access is now null-guarded. The rule for every guard: a missing
child yields the error type (expressions) or is skipped (statements), and
**no new diagnostic is reported** — the parser's E1000 is the diagnostic.
This is safe because `Type::isAssignableTo` returns true when either side
is the error type (`compiler/src/sema/Type.cpp:120`), so error stand-ins
add no spurious errors. Guards trigger only on inputs that previously
crashed; valid programs never hit them.

Guard sites (26, each marked `Phase 5 slice C` in the code):

- Expressions: `BinaryExpr` (left+right), `UnaryExpr`, `CallExpr` (callee +
  null args as error-type placeholders, preserving arity for overload
  resolution), `MemberExpr`, `IndexExpr`, `ArrayExpr` / `MapExpr` (null
  elements as placeholders), `LambdaExpr` (body handled only when present;
  a null body falls back to `int`, matching the old fallthrough),
  `TernaryExpr`, `AssignExpr`, `NewExpr` (all 8 arg loops), `TryExpr`,
  `AwaitExpr`, `ExprStmt`.
- Statements: `IfStmt`, `WhileStmt`, `DoWhileStmt`, `ForStmt` (body),
  `ForInStmt` (iterable + body; the non-iterable E1001 is suppressed when
  the iterable type is error), `SwitchStmt` (subject + case values),
  `MatchStmt` (subject + arm bodies), `TryCatchStmt` (try block + catch
  bodies), `ThrowStmt`, `hasGuaranteedReturn` (null then-branch).
- Already guarded, no change: `VarDeclStmt`, `FunctionDecl`
  (`hasBody`/`isShortForm`), `ReturnStmt`, `BlockStmt`, `Program`,
  `ClassDecl`.

10 new `hasX()` accessors in the AST headers, following Phase 4 slice B's
pattern: `BinaryExpr::hasLeft`, `CallExpr::hasCallee`,
`MemberExpr::hasObject`, `IndexExpr::hasObject`, `TryExpr::hasExpr`,
`AwaitExpr::hasExpr`, `TernaryExpr::hasCondition`, `AssignExpr::hasTarget`,
`CatchClause::hasBody`, `TryCatchStmt::hasTryBlock`.

### 2. Parser recovery: no more phantom declarations

Root cause: `consume()` reports E1000 but does **not** advance, so a
failed name consume left the token stream untouched and the parser kept
building with an empty name — then re-parsed the leftover token as a
different declaration. `fn test()` produced a nameless `FunctionDecl`
(`test` is a reserved keyword, so it can never be a fn name) and then a
phantom `__sp_test_anonymous_test` from the leftover `test` token, plus
7 diagnostics for one broken line. Fixes:

- `parseFunctionDecl` / `parseClassDecl` / `parseInterfaceDecl` /
  `parseStructDecl` / `parseEnumDecl`: empty name → `synchronize()` +
  return `nullptr` (no nameless declaration; recovery skips to the next
  declaration boundary).
- `parseVarDecl`: empty name → return `nullptr` (no synchronize: the
  trailing `;` is skipped by `parseProgram`'s match, and any other
  following token is consumed by the next parse attempt).
- `parseTestDecl`: the `anonymous_test` fallback stays when a body IS
  present (`test { ... }` is a real test), but a test with neither a name
  nor a body is dropped (`synchronize()` + `nullptr`).
- `parseIfStatement` / `parseWhileStatement`: null condition → return
  `nullptr` with **no** synchronize — the failed expression parse already
  advanced past the offending token, so synchronizing would eat the next
  real statement (verified: `if; println("hi");` keeps `println`).
- `parseExprStatement`: null expression → return `nullptr` (a lone `;`
  used to produce an `ExprStmt` with a null expression).
- `parseBlock`, switch case/default loops, class member loop: skip null
  statements/members instead of pushing null entries.
- Interface methods, struct fields, enum variants: skip entries whose
  name consume failed (empty name).

### 3. TryCatchExecutionTest harness loads stdlib imports

Mirrored the Phase 4 parent fix for `ExecutionTest.cpp`:
`#include "sp/sema/ModuleLoader.h"` + `ModuleLoader::loadAndMergeAll`
before sema in `runSPProgram`. The file is shared with slice B (typed
catches); this slice touched only the helper, not the test bodies.

## Verification

Build dir: `~/workspace/build-phase5-robust` (Release, `-j2`, Clang
18.1.3). Fresh build clean, 0 errors. (A VM replacement mid-task wiped
`/tmp` and killed the first build at ~38 min in; the build was restarted
and finished clean. `/tmp` is a 512M tmpfs — never build there.)

### Broken-program sweep — `tests/sema/BrokenProgramTest.cpp` (29 tests, NEW)

Each test lexes + parses + calls `SemanticAnalyzer::analyze()` on a
broken program — the exact call shape the LSP/debugger use — and asserts
a parser diagnostic exists and `analyze()` returns. Before/after, per
test run against the baseline binary (`--gtest_filter`, one TEST at a
time; a segfault is exit 139):

| Program | Before (baseline) | After |
|---|---|---|
| `if;` | segfault | clean return |
| `while;` | segfault | clean return |
| `do {..} while;` | segfault | clean return |
| `for (int i=0; i< ; i++)` | segfault | clean return |
| `let x = 1 + ;` | segfault | clean return |
| `let x = -;` | segfault | clean return |
| `foo(,);` | segfault | clean return |
| `a[]` | segfault | clean return |
| `await;` (async fn) | segfault | clean return |
| `true ? 1 : ;` | segfault | clean return |
| `x = ;` | segfault | clean return |
| `[1, , 3]` | segfault | clean return |
| `{"a": 1, "b": }` | segfault | clean return |
| `new Foo(,);` | segfault | clean return |
| `throw;` | segfault | clean return |
| `;` (in fn) | segfault | clean return |
| `case :` (switch) | segfault | clean return |
| `match x { 1 => , }` | segfault | clean return |
| `(int x) => ;` (lambda) | no crash (see note) | clean return |
| unbalanced braces | no crash (see note) | clean return |
| `fn test()` | 2 decls `[fn:, fn:__sp_test_anonymous_test]`, 7 errors | **0 decls, 1 error** |
| `fn () {..}` | 1 decl `[fn:]` | **0 decls** |
| `class { int x; }` | 1 decl `[class:]` | **0 decls** |
| `let ;` | 1 decl | **0 decls** |
| `fn main() {..} }` (stray `}`) | 2 decls `[fn:main, exprstmt]` | **1 decl `[fn:main]`** |
| `fn test() {..} fn main() {..}` | 3 decls (2 phantoms + main) | **1 decl `[fn:main]`** |
| valid: `for (;;) { break; }`, `fn main() {}`, try/catch | pass | pass (unchanged) |

**Result: 29/29 pass.** (18 of the 20 null-child cases segfaulted on the
baseline; the other 2 never crashed but are covered by the same guards.)

Note on slice B's "5 decls" for `fn test()`: this slice measured **2
top-level declarations** for the exact input `fn test()` (nameless fn +
phantom test fn). The other 3 items in slice B's report (2 null ExprStmts
+ top-level block) did not reproduce as top-level declarations — possibly
nested nodes or a different probe input. After the fix the count is 0
either way.

Note on `analyze()`'s return value: `SemanticAnalyzer::analyze()` reports
whether *sema added new errors*, so it returns `true` for a program whose
only errors are parse errors. The sweep test therefore does **not**
assert `analyze() == false` for broken programs — the assertion is that
`analyze()` returns at all (no segfault) and a parser diagnostic exists.
Callers must also check `hasErrors()`, as `spc` does. The two
non-crashing baseline cases (`(int x) => ;`, unbalanced braces) failed
only on that over-strict assertion in the first draft of the test; the
assertion was corrected, not the code.

### Full ctest — 705/707 (99%)

Total test time 148.81 s. The only 2 failures are the pre-existing
environmental ones, unchanged since Phase 4:

- `NetTest.UdpSocketSendRecv` — fails in 0.01 s (sandbox UDP `sendto`
  returns EPERM; Phase 4 slice D proved it; the test fails fast now).
- `JsonTest.BenchmarkOneGigabytePerSecThroughput` — throughput assertion
  under CPU contention; passes when the machine is quieter. Not weakened.

`BrokenProgramTest` + `TryCatchExecutionTest` via ctest: **60/60 pass**
(29 + 31, the latter including slice B's new typed-catch tests — the
ModuleLoader change did not break any of them).

### Live proof — final `spc` binary, all 29 sweep programs

All 26 broken programs fail cleanly with an E1000 diagnostic (exit 1,
no segfault — a crash would be exit 139); all 3 valid controls pass
`spc check` (exit 0). Highlights:

- `fn test()`: **7 errors → 1 error** (`Expected function name` only).
- `if;`: **3 errors → 2 errors** (the spurious `Expected ')' after if
  condition`, which pointed at the *next* statement's line, is gone
  because the broken `if` no longer swallows it as a then-branch).
- `fn main() { println("ok"); }`: still `Check passed`.

## Files changed (this slice only)

- `compiler/include/sp/ast/Expression.h` — 8 new `hasX()` accessors (+21)
- `compiler/include/sp/ast/Statement.h` — 2 new `hasX()` accessors (+18)
- `compiler/src/sema/TypeChecker.cpp` — 26 null-guard sites (file also
  carries slice B's `setResolvedTypeName` block from parallel work; the
  parent splits by hunk)
- `compiler/src/parser/Parser.cpp` — recovery fixes (+102/−6)
- `tests/sema/BrokenProgramTest.cpp` — NEW, 409 lines, 29 tests
- `tests/CMakeLists.txt` — 1 line: register the new test
- `tests/codegen/TryCatchExecutionTest.cpp` — include + ModuleLoader block
  only (file also carries slice B's typed-catch tests; untouched by this
  slice)

## Deliberately not done

- `analyze()`'s return contract (returns `true` when sema adds no new
  errors, even with pre-existing parse errors) was **not** changed — it
  is the existing contract, `spc` already pairs it with `hasErrors()`,
  and changing it is a separate decision. See "Known follow-ups".
- No new error codes; no diagnostics added for missing children — the
  parser's E1000 is the diagnostic.
- No valid-program behaviour or diagnostic changed (proven by the 3
  valid controls + the full 707-test suite, including the golden
  parser/sema tests).
- The `parseGroupedOrLambda` / `parseNewExpr` / `parseStringLiteral`
  null paths were audited for the no-infinite-loop argument (every null
  return from `parseExpression` consumed ≥1 token) but not changed.
- Graphify protocol from repo `AGENTS.md` could not be run:
  `graphify-out/` does not exist and `python -m graphify` is not
  installed (same as Phases 1–4; verified again). Nothing was fabricated.

## Known follow-ups (for the parent / a later slice)

- `analyze()` returning `true` for parse-error programs (when sema adds
  nothing) is a footgun for callers that check only the return value.
  Options: make it return `false` when the engine already has errors on
  entry, or document that callers must also check `hasErrors()`.
- `try { } catch () { }` and similar malformed catch clauses were not
  added to the sweep (catch bodies come from `parseBlock()`, which never
  returns null, so no crash vector was found there).
- The `do; while;` / `for (;;) ;` style constructs keep their node with
  null children (checker skips them); a stricter parser could drop them
  entirely, as it now does for `if;`/`while;`.
- Slice B's "5 decls" figure for `fn test()` could not be reproduced as
  top-level declarations (measured: 2); if the parent needs the exact
  reconciliation, the probe input slice B used would have to be re-run.
