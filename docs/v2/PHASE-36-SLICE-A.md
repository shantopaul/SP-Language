# Phase 36 (pro-4.0.0) — Slice A: Safe-nav completion (`??` after `?.` + `?.` method calls)

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `328eee0` (Phase 6 / Phase 35 done, pushed)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, no ADR-001–005
  items touched, nothing committed or pushed)
- **Scope:** the two safe-nav gaps Phase 6 left behind — (1) `??` after
  `?.` on a null object did not fire, (2) `?.` method calls segfaulted
  on a null receiver. Both were reproduced before fixing (see §1).
- **Status:** both fixed and proven; fresh build clean; full ctest
  **794/795 (99%)** — only `NetTest.UdpSocketSendRecv` (sandbox UDP
  EPERM). 4 new execution tests, all passing.

## 1. What was wrong (both reproduced on the Phase 6 binary)

**Gap 1 — `??` after `?.` does not fire.** Probe
`~/workspace/p6d-proofs/probe_qq_safenav.sp` (Phase 6-D), run with the
Phase 6 binary:

```
a=default-a
s=              (should be "default-s")
v=0             (should be 99)
s2=real
v2=7
```

Plain `maybe ?? "default-a"` worked; non-null `b2?.label` worked. Only
the combination failed.

**Gap 2 — `?.` method calls segfault.** Proof program
`~/workspace/p7a-proofs/probe_safenav_call.sp`, run with the Phase 6
binary:

```
Segmentation fault
```

Nothing printed at all — the first `g?.greet()` on a null receiver
died in the vtable load. Phase 6-A's own code comment at the safe-nav
field-access site said so: "the member-call path in visit(CallExpr)
has no null check and still segfaults on a null receiver".

## 2. Fix 1 — `??` after `?.` (IRBuilder.cpp, visit(BinaryExpr))

### The mechanism

The `??` lowering (Phase 6 slice A) decides by the **static sema
type** of the LHS:

```cpp
auto semaLhsType = m_sema.getNodeType(&node.getLeft());
if (!semaLhsType || !semaLhsType->isNullable()) {
    // non-nullable LHS can never be null: ?? is a no-op, RHS unevaluated
    m_lastExprValue = emitCastIfNeeded(lhs, resType);
    ...
    return;
}
```

But `TypeChecker::visit(MemberExpr)` reports a safe-nav field access
with the field's **plain** type: it unwraps the nullable object type,
then sets `m_currentExprType = field.type` (`string`, not `string?`).
So `b?.label` has the null VALUE on the null branch (the field-access
fix stores `nullValueOf(fieldType)`) but a non-nullable static type —
and `??` took the no-op path.

### Decision: (b) over (a) — measured, not guessed

Two candidate fixes were measured before choosing:

- **(a) Make `?.` report a nullable result type in sema.**
  Rejected. `NullableType::isAssignableTo` returns false when the
  target is a plain type (`compiler/src/sema/Type.cpp:193-200`), so
  every plain use of `?.` would become a new `spc check` error:
  `string s = b?.label;` (VarDecl init check, TypeChecker.cpp:1577),
  `println(b?.label)` (argument check, :364),
  `return b?.label;` (return check, :1841). Fixing (a) would mean
  also relaxing all of those checks — a type-system change, not a
  one-line fix, and it would change `spc check` semantics for
  currently-valid programs.
- **(b) Make `??` treat a safe-nav LHS as nullable at runtime.**
  Chosen. It touches only the `??` lowering, changes no sema types,
  and `spc check` semantics are intact. The runtime null test already
  handles every IR type the safe-nav can yield (pointer `== null`,
  integer/bool `== 0`, float `oeq 0.0`), and safe-nav yields exactly
  those null representations on its null branch.

A blanket "always emit the runtime check" (the naive form of (b))
would be wrong: for a plain non-nullable `int x = 0`, `x ?? 99` must
stay `0` — zero is a legitimate value, not null. So the fix is
surgical: the no-op is skipped only when the LHS node itself is a
safe-nav access — a safe-nav `MemberExpr`, or a `CallExpr` whose
callee is one (so `obj?.method() ?? fallback` composes with fix 2):

```cpp
static bool lhsMayBeNullViaSafeNav(const ast::ExpressionNode& lhs) {
    if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&lhs)) {
        return mem->isSafeNav();
    }
    if (const auto* call = dynamic_cast<const ast::CallExpr*>(&lhs)) {
        if (const auto* mem = dynamic_cast<const ast::MemberExpr*>(&call->getCallee())) {
            return mem->isSafeNav();
        }
    }
    return false;
}
```

and the early-out becomes:

```cpp
if ((!semaLhsType || !semaLhsType->isNullable()) &&
    !lhsMayBeNullViaSafeNav(node.getLeft())) { /* no-op */ }
```

### Known representation limit (shared with plain `int?`, not new)

For value types, null is represented as zero — so a safe-nav int
field that is really `0` on a non-null object is indistinguishable
from null: `b2?.n ?? 99` with `n == 0` yields `99`, exactly like
`int? m = 0; m ?? 99` yields `99` (proven in
`~/workspace/p7a-proofs/edge_plain_coalesce.sp`: `w=99`). For
strings this does not apply: a real `""` is a non-null pointer, so
only a null pointer means null. This is the language's existing
nullable-value-type convention, not something this fix introduces.

## 3. Fix 2 — `?.` method calls (IRBuilder.cpp, visit(CallExpr))

### Why the guard lives at the dispatch, not at the top

The member-call body (section 1b, `obj.method(...)`) is ~1,500 lines
of `if (...) { ...; return; }` sub-paths (super, static type methods,
Task/Result/Option/Duration/Arena/Thread/Mutex/RwLock/Semaphore
builtins, List/Map/Set builtins, then the vtable + direct/static
dispatch). A diamond around the whole body cannot work: every
sub-path C++-`return`s out of `visit()`, so no control would ever
reach a merge block. Extracting the body into a helper would be a
~1,500-line mechanical move for this slice.

So the guard wraps only the **vtable + direct/static dispatch**
(`if (layout && slotIdx >= 0) { ... } else { ... }`), which is the
path that segfaults for user-defined class/interface methods — the
vtable load dereferences the null receiver. The guard is emitted just
before the dispatch, after the receiver was evaluated once (`objVal`):

- Compute the merge result type from sema (async call → Task
  pointer; otherwise the mapped node type; `int32` fallback mirroring
  the direct branch).
- Emit the null compare, `nullBB` (store `nullValueOf(resType)`),
  `callBB` (the dispatch), `mergeBB` (a stack slot, same pattern as
  the field-access fix and `??`).
- The dispatch has 4 value-producing exits (async-ok + plain call, in
  each branch); each funnels through a local `safeNavFinish` lambda
  (store + branch to the merge block + merge load + return). Error
  exits (`reportUnsupported`) are left alone: they push to `m_errors`
  and `build()` fails the compilation, so the open diamond in the
  discarded module is harmless.
- Void methods need no slot: the exit just branches to the merge
  block and returns void.

The guard only applies when the receiver is a pointer (a
class/interface object); value receivers cannot be null.

### What the guard does NOT cover (measured, not assumed)

- **Builtin member-method paths are mostly unreachable for nullable
  receivers anyway.** The class-name-gated paths (`Mutex`, `Thread`,
  `Arena`, `Duration`) compare `semaObjType->toString()` against the
  bare name — for a `Mutex?` receiver the string is `"Mutex?"`, so
  the check fails and control falls through to the guarded dispatch
  (verified in the IR dump: 0 `sp_rt_mutex_lock` refs, 2
  `Mutex_lock` refs, identical on the Phase 6 binary). The
  List/Map/Set paths are gated on type predicates that fail for
  `NullableType`, so they fall through the same way.
- **`Task` methods are reachable but safe.** `Task<int>? t = null;
  t?.cancel()` takes the builtin path (`"Task<int>?"` starts with
  `"Task<"`) — but the runtime handles a null task handle gracefully:
  `cancelled=false / DONE` on both the Phase 6 and the new binary.
- **Generic `List`/`Map` methods fail IR verification identically on
  both binaries** (`call to undeclared function 'List_int_map'`,
  `'isEmpty'`) — even for plain non-null `.size()`/`.map()` calls.
  Pre-existing and independent of safe-nav; not a regression, not
  this slice.
- **Chained safe-nav `a?.b.c` still segfaults** (verified on both
  binaries; non-null control works: `t=x`). The plain `.c` access on
  a possibly-null intermediate has no null check. Follow-up, not
  this slice.

## 4. Live proofs (new binary `~/workspace/build-phase7-safenav/bin/spc`)

**Probe (Phase 6-D) — `??` after `?.`:**
```
a=default-a
s=default-s
v=99
s2=real
v2=7
```

**Method-call proof — `~/workspace/p7a-proofs/probe_safenav_call.sp`:**
```
s=fallback-greet
v=99
after-null-shout
s2=hi Shanto
v2=42
SHOUT
s3=LOUD hi Mina
```

The null receiver yields the null value / fallback in every case
(string, int, and void `shout()` skipped silently); the non-null
receiver dispatches normally, including **virtual dispatch**
(`LoudGreeter` override: `LOUD hi Mina`).

**Plain-`??` no-op pinned — `edge_plain_coalesce.sp`:**
```
v=0
t=hi
w=99
DONE
```

## 5. Tests (4 new, `tests/codegen/CodegenCrashersExecutionTest.cpp`)

| Test | Proves |
|---|---|
| `CoalesceAfterSafeNavField` | Exact Phase 6-D probe outputs (`s=default-s`, `v=99`, controls `s2=real`, `v2=7`) |
| `PlainNonNullableCoalesceIsNoOp` | `int x = 0; x ?? 99` stays `0`; `"hi" ?? "bye"` stays `"hi"` |
| `SafeNavMethodCallOnNullDoesNotCrash` | Null receiver → fallbacks + void skip; non-null → real dispatch; derived override → virtual dispatch |
| `SafeNavAsyncMethodCall` | Null async call (never awaited) does not crash; non-null `await w2?.compute()` → `7` |

All 4 pass (see §6). No new test files, no CMakeLists change.

## 6. Verification

Fresh Release build in `~/workspace/build-phase7-safenav` (`-j2`,
Clang 18.1.3, googletest bypass
`-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=…/build-phase4-parent/_deps/googletest-src`).
Build clean, **0 errors**. (One compile error was caught and fixed
during the build: `safeNavResAlloca` was declared `IRValuePtr`, but
`addInstruction` needs the `AllocaInst` type — the field-access fix
keeps it as `std::shared_ptr<AllocaInst>`.)

Full ctest: **794/795 (99%)** — the only failure
is `NetTest.UdpSocketSendRecv` (sandbox UDP EPERM, environmental).
Expected total: 791 (Phase 6) + 4 new tests = **795**.

## 7. Files changed (all uncommitted)

| File | Change |
|---|---|
| `compiler/src/ir/IRBuilder.cpp` | `??` lowering: `lhsMayBeNullViaSafeNav` helper + no-op condition; safe-nav method-call guard + `safeNavFinish` lambda + 4 converted dispatch exits |
| `tests/codegen/CodegenCrashersExecutionTest.cpp` | 4 new execution tests (§5) |
| `docs/v2/PHASE-36-SLICE-A.md` | This doc |

Build dir: `~/workspace/build-phase7-safenav`. Proofs:
`~/workspace/p7a-proofs/` (`probe_safenav_call.sp`,
`edge_plain_coalesce.sp`, `edge_builtin_safenav.sp`,
`edge_builtin_map.sp`, `edge_builtin_map2.sp`,
`edge_builtin_mutex.sp`, `edge_builtin_task.sp`,
`edge_chained_safenav.sp`, `edge_chained_ok.sp`,
`edge_list_plain.sp`).

## 8. Deliberately not done

- **Chained safe-nav `a?.b.c`** still segfaults on a null
  intermediate (pre-existing, verified on both binaries).
- **Generic `List`/`Map` methods** (`List_int_map`, `isEmpty`,
  `List_int_size`) fail IR verification for plain calls too
  (pre-existing, identical on both binaries).
- **(a) from §2** — making `?.` report a nullable sema type — was
  measured and rejected; it would turn plain `?.` uses into new
  `spc check` errors.
- The repo `AGENTS.md` Graphify protocol (`python -m graphify update
  . --force`) could not be run: `graphify-out/` does not exist and
  `python -m graphify` is not installed (verified repeatedly in
  Phases 5–6). Nothing fabricated.
