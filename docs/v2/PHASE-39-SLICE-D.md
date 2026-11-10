# Phase 39 slice D — module consts bind (ADR-004)

## 0. Bug (verified 2026-11-10)

`import mini;` + `println("version ${MINI_VERSION}")` fails with
`error[E1002]: Undefined variable or symbol 'MINI_VERSION'` — and so
does the SAME-FILE case (`const MY_CONST: int32 = 42;` at top level,
used in `main`). The Phase 39 doc §0.3-D assumed the cause was
"`ModuleLoader` merges module-level `const` declarations" — that
assumption was **wrong**.

**Real root cause: no pass handles a top-level `VarDeclStmt` at all.**
`ModuleLoader::loadAndMergeAll` (`compiler/src/sema/ModuleLoader.cpp:138`)
already merges ALL non-Module/Import declarations — including the
consts — into the root program (merge loop ~lines 228–245). But:

1. `NameResolver::pass1_collectTopLevel` (`compiler/src/sema/NameResolver.cpp:11`)
   handles Import/Class/Interface/Struct/Enum/Function/BlockStmt only —
   no `VarDeclStmt` case, so the name is never registered.
2. `TypeChecker::visit(Program)` has no `VarDeclStmt` case either —
   the initializer is never checked and the symbol is never defined.
3. `IRBuilder::visit(Program)` collects top-level `VarDeclStmt`s into
   `topLevelStmts` and only executes them inside a *synthesized* main;
   with an explicit `main` they are silently dropped (proven: a
   top-level `println` never runs when an explicit `main` exists).

So "SP has no global variables" was true by accident, not by design:
the parser accepts top-level `const`/`let`/`var` (`Parser::parseVarDecl`,
`Parser.cpp:867`), `SymbolKind::Constant` exists (`Symbol.h:17`), and
bindgen emits module-level consts — but nothing downstream ever binds
them. The `<anonymous>:1:1` location on the E1002 is the interpolated
identifier's synthetic location, not the use site.

## 1. Design — real globals, mirroring class statics (Phase 6 slice B)

A top-level `const`/`let`/`var` is a **program-lifetime global**,
exactly like a `static` class field: an LLVM module global
(`IRModule::addGlobalVariable`, zero-initialized) plus one init
function that evaluates the initializers in declaration order and
stores them, run from `main` before any user code.

1. **Sema — `TypeChecker::visit(Program)` pre-pass.** Before the
   short-form pre-pass, visit every direct top-level `VarDeclStmt`.
   `visit(VarDeclStmt)` already does everything needed: resolves the
   annotation (`m_typeCtx.resolveType`), infers from the initializer
   for `var`/`let`/`const` without annotation, checks assignability
   and null safety, and defines a `VariableSymbol` (`Constant` kind
   when `isConst`) in the current — global — scope. The main loop
   deliberately gets NO `VarDeclStmt` case: each const is visited
   exactly once (a second visit would report a false E1004
   duplicate). The pre-pass must run before the short-form pre-pass:
   a short-form body (`fn getVersion() => MINI_VERSION;`) is checked
   there and needs the const defined already. Imported consts work
   unchanged — they are declarations of the merged program. No
   NameResolver change is needed: the E1002 is raised by the
   TypeChecker, and the pre-pass defines the symbol before any
   function/class body is checked.
2. **IR — `IRBuilder::visit(Program)`.** Before the class loop,
   collect direct top-level `VarDeclStmt`s; for each, compute the IR
   type (annotation → `mapType(string)`; no annotation → the
   initializer's sema type via `m_sema.getNodeType` → `mapType(Type*)`),
   register `@__sp_global_<name>` (`addGlobalVariable`, `isConstant=false`,
   `"zeroinitializer"`), and record it in the new `m_globalVars` map
   (name → `{irType, globalName, isWeak, isConst}`, mirroring
   `StaticFieldInfo`). If any decl has an initializer, emit
   `__sp_global_init` — a void function mirroring
   `compileStaticInitializer`: evaluate each initializer in
   declaration order, `sp_rt_retain` a borrowed owned value, adopt a
   `+1` `new` (Phase 7 slice C rule), `emitWeakCreate` for a `weak`
   slot (Phase 8 slice C rule), load-through-pointer for struct
   values, `emitCastIfNeeded`, store into the global. The init
   function name is inserted at the FRONT of
   `m_staticInitFunctions`, so consts initialize before class
   statics (a static initializer may reference a top-level const).
   `emitStaticInitCalls()` already runs at the start of both the
   synthesized and the explicit `main`.
3. **IR — exclude from `topLevelStmts`.** A `VarDeclStmt` is a
   declaration, not a statement to execute: add it to the exclusion
   list in the `topLevelStmts` collection, so the synthesized main
   does not also create a local alloca for it.
4. **IR — `visit(IdentifierExpr)` global load.** After the
   class-statics check, before the `PI`/`E`/`TAU` builtins (a user
   global wins over a builtin of the same name, same rule as class
   statics): look up `m_globalVars`; struct type → return the global
   pointer itself (mirrors the local-struct branch); otherwise emit
   a `LoadInst`; `weak` slot → `emitWeakUpgrade`.
5. **IR — `visit(AssignExpr)` global store.** After the
   class-statics branch: if the target name is in `m_globalVars`,
   `targetPtr` is the `GlobalVariable`. Sema already rejects
   assignment to a `const` symbol (`TypeChecker.cpp:1231`), so this
   only fires for `let`/`var` globals. Compound assignment
   (`+=` etc.) reuses the same `targetPtr` path.

## 2. Out of scope (documented)

- **Const-to-const forward references fail.** The pre-pass visits
  consts in declaration order, so `const B = A + 1;` needs `A`
  declared first. (Functions may reference any const regardless of
  order — the pre-pass runs before all function bodies are checked.)
- **Address-of a global (`&MINI_VERSION`) is not supported** —
  the unary-`&` path resolves through `lookupVariable` only and
  silently yields nothing for a global. A loud error here is a
  follow-up, not this slice.
- **`++`/`--` on a global** is not wired (same `lookupVariable`
  limitation as `&`).
- **Top-level `VarDeclStmt` inside a bare `BlockStmt`** (e.g. inside
  an `extern "C" {}` block) is not collected — direct declarations
  only, matching how the existing passes treat top-level blocks.
  Bindgen emits consts outside the extern block, so this does not
  affect the proof.
- **A `const` without an initializer** (`const X: int32;`) is allowed
  by sema and stays zero-initialized — same as a static field
  without an initializer.
- **Globals are never dropped** (program lifetime): an owned value
  stored in a global keeps its +1 share forever. That is correct,
  not a leak — and it is why the retain/adopt rules in the init
  function mirror the static-field rules exactly.

## 3. Verification (2026-11-10, slice-D binary)

- Same-file const: `spc run /tmp/same_file_const.sp` → `c 42`.
- Import proof (the original repro): `spc run use_const.sp` in
  `~/workspace/p10-probes/minilib/` → `version 3`.
- zlib consts proof: `spc run zlib_consts.sp` (imports the generated
  `zlib.sp`) → `zok 0 / zver 1.3 / zvernum 4864 / zmem 9`.
- Slice A/B regression with the slice-D binary, byte-identical:
  `use_mini.sp` → `add 42 / scale 14 / sum 7 / filled 9 10`;
  `p5_struct_ret.sp` → `made 5 6 / added 6 8`; `p6_big.sp` → `big 21`.
- 5 new tests in `tests/codegen/ExecutionTest.cpp`, all pass:
  `TopLevelConstValues` (int32/double/string/bool + uninferred
  `const INFERRED = 7` + use inside a non-`main` fn),
  `TopLevelLetGlobalReassigned` (`counter 2`, `counter2 12` via `=`
  and `+=`), `TopLevelConstChained` (`doubled 20`),
  `TopLevelConstForwardRef` (fn declared before the const, `late 99`),
  `TopLevelConstFromImport` (module file written to the test output
  dir, found via a new optional search-path parameter on
  `runSPProgram`; `version 3`, `scale ok true`).
- Full ctest **884/885** (log `~/workspace/ctest-phase10-slice-d.log`) —
  only the standing `NetTest.UdpSocketSendRecv` EPERM failure; PG
  skipped. Test total 880 → 885.

## 4. Finding during verification — builtin restatement is a no-op

The pre-pass exposed a latent collision the old code never saw:
`stdlib/math/math.sp` declares `public const double PI/E/TAU` with
exactly the builtin values — but `SymbolTable::initBuiltins`
(`SymbolTable.cpp:278-280`) already defines `PI`/`E`/`TAU` as
`Constant` doubles in the global scope, so the first full ctest run
failed `GoldenSemanticTest.AnalyzeStdlibMath` with three E1004
duplicates. Fix, in `Scope::define` (`SymbolTable.cpp`): a
variable/constant restating a `<built-in>` symbol with an **identical**
type returns `true` and keeps the builtin — the exact mirror of the
existing function branch, where a concrete implementation overriding
a built-in stub returns `true`. A conflicting type is still an E1004
duplicate. Consequence, verified by the passing test: a program that
imports `std.math` gets a `@__sp_global_PI` global holding the same
value, and a bare `PI` use loads it (the global branch in
`visit(IdentifierExpr)` runs before the `PI` constant branch) — same
value either way.
