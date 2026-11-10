# Phase 35 (pro-4.0.0) — Slice A: Codegen Crashers

- **Branch:** `feature/pro-4.0.0` (main untouched, still `d10e1c2`)
- **Base:** `0c5ebbe` (Phase 5 / Phase 34, pushed)
- **Status:** implemented, **uncommitted** — push once at the end of Phase 6
- **Version:** stays **3.1.0** (no bump; branch name is aspirational only)
- **Scope:** the 7 real compiler bugs found by Phase 5 slice D's doc-gate
  (`docs/v2/PHASE-34-SLICE-D.md` §3.1). Nothing here touches ADR-001–005,
  `.github/`, or the version source of truth.
- **Verification:** fresh build clean, full ctest **760/761 (99%)** — only
  `NetTest.UdpSocketSendRecv` fails (sandbox UDP EPERM, pre-existing).
  26 new tests (12 IR + 14 execution), all passing.

Every fix follows the same rule: a construct that is genuinely
unsupported fails **loudly with E1000** — never a silent fallback, never
a segfault. Where a fix changes a precedence (fix 4), sema stays the
authority and IRBuilder follows sema's resolved target.

---

## 1. Map iteration never terminated, garbage keys

### What was wrong

```sp
var ages = new Map<string, int>();
ages["Alice"] = 30;
for (key in ages) { println("key=${key}"); }   // garbage forever
```

`for (key in ages)` printed garbage keys (`key=3`, `key=-840506240`,
`key=16`, …) until killed by timeout. Int-key maps did the same.

### Root cause

`IRBuilder::visit(ForInStmt)` treated every non-List iterable as an
array: it called `sp_rt_array_length` on the map pointer, which read the
entry-table pointer as a length, and indexed it as an array. The
runtime map (`SPMapImpl`, open-addressing hash table, separate str/int
entry arrays, int64 values only) had `sp_rt_map_size` but **no key
accessor at all**.

### Fix

- Runtime: `sp_rt_map_str_key_at(map, idx)` /
  `sp_rt_map_int_key_at(map, idx)` — the idx-th occupied key in table
  order. Null map or out-of-range index returns null / 0.
  (`runtime/src/Runtime.cpp`, declared in `runtime/include/sp/runtime/Runtime.h`,
  emitted as externs in `compiler/src/ir/IR.cpp`.)
- IRBuilder: the `ForInStmt` map branch (detected via `sema::MapType`,
  key type from `getKeyType()`) uses `sp_rt_map_size` for the length
  and the new key accessors for the fetch. The fetch mirrors the
  index-assignment path: string keys via the str table, every other key
  via the i64 table.

Iteration order is **table order, unspecified** — do not rely on
insertion order, and do not mutate the map while iterating (a rehash
changes the order).

### Tests

- IR: `MapIterationLowersToMapKeyAccess` (expects `sp_rt_map_size` +
  `sp_rt_map_str_key_at`, NOT `sp_rt_array_length`),
  `MapIterationIntKeysLowers` (`sp_rt_map_int_key_at`).
- Execution: `MapIterationStringKeysTerminates`,
  `MapIterationIntKeysTerminates`, `MapIterationEmptyMap`.

### Live proof

```
key=Bob
key=Alice
key=Shanto
count=3
DONE
```

```
k=20
k=30
k=10
count=3 sum=60
DONE
```

---

## 2. Interface-typed method calls failed IR generation

### What was wrong

```sp
interface Drawable { fn draw(); fn getColor() -> string; }
class Circle implements Drawable { /* ... */ }
var d1 = new Circle(3, "red") as Drawable;   // (interface-typed variable)
d1.draw();                                    // error[E1000]: IR verification failed:
                                              //   Function 'main': call to undeclared function 'draw'
```

Any method call through an interface-typed receiver failed the same
way (`draw`, `getColor`, …), including calls through `List<Drawable>`.

### Root cause

Method-call lowering computed the class layout from the receiver's
sema type; for an interface receiver `getClassLayout("Drawable")`
returned null (interfaces have no layout), so it fell to the
direct-call branch and emitted `Drawable_draw`, which does not exist.
Vtable slots are per-class declaration order, so a fixed slot index
cannot work across classes — the same method sits at a different
index in each implementing class.

### Fix — global interface-slot scheme

`VTableBuilder` now assigns **one global slot per (declaring
interface, method, signature)**, deterministically: interfaces sorted
by name, methods in declaration order, super-interfaces walked
recursively. Every class vtable is prepended with this interface
region (N slots); when a class method's key matches a global interface
key it is placed in that global slot instead of being appended (safe
even for classes that do not implement the interface — sema forbids
viewing them through it, so those slots are unreachable). Empty slots
emit `ptr null` in the vtable global. Each interface also gets a
layout (`{ptr}` struct type, slots whose `functionType` comes from the
interface method signature, no vtable global emitted), so the existing
vtable-dispatch path in IRBuilder works unchanged via `getClassLayout`
+ `getMethodVTableSlotByKey`.

Files: `compiler/include/sp/codegen/VTableBuilder.h`
(`InterfaceSlotInfo`, `buildInterfaceSlots()`,
`buildInterfaceLayout()`, `interfaceSlotsForMethod()`),
`compiler/src/codegen/VTableBuilder.cpp`.

### Tests

- IR: `InterfaceDispatchLowers` (module builds, `@_vtable_Circle`
  present), `InterfaceSlotIsStableAcrossDeclarationOrders` (Circle
  declares `draw()` first, Square declares `getColor()` first; asserts
  `@Circle_draw` and `@Square_draw` sit at the same offset inside
  their respective `@_vtable_* = constant` lines).
- Execution: `InterfaceDispatchDirectAndList` (direct + through
  `List<Drawable>`), `InterfaceDispatchSwappedDeclarationOrder`.

### Live proof

```
Circle r=3 color=red
color=red
Circle r=1 color=blue
Rect 2x3 color=green
DONE
```

---

## 3. Constructor calling a method segfaulted

### What was wrong

```sp
class User {
    fn User(string name) { this.id = this.generateId(); }  // segfault
    fn generateId() -> string { return "user-123"; }
}
```

`spc check` passed; `spc run` / `spc ir` segfaulted (exit 139). The
bare form (`this.id = generateId();`) crashed identically. gdb
backtrace (same for fix 4): SIGSEGV in `IRInstruction::format()` ←
`IRVerifier::verifyBlock` ← `verifyFunction` ← `verify` ←
`IRBuilder::build`.

### Root cause

Same as fix 4: vtable dispatch computed the call's return type from
`slotInfo.functionType` (never set for non-generic classes) or from
`m_module->getFunction(mangledName)` (only finds functions already
lowered — methods lower in declaration order, no pre-declaration
pass). A forward call to a value-returning method therefore got
return type void → `m_lastExprValue = nullptr` → a `StoreInst` with a
null value → the verifier's `format()` dereferenced the null operand.
(`IR.cpp` `format()` has no null checks; `StoreInst` accepts null
operands silently; the `BinaryInst` ctor dereferences
`lhs->getType()`.)

### Fix

- `VTableBuilder::buildClassLayout` now sets `slotInfo.functionType`
  from the method's sema signature — `(ptr %this, ...declared
  params) -> return` — mirroring `buildSpecializedClassLayout` (best
  `MemberMethod` found the same way `canonicalParamTypes` does; also
  updated on the override path). Builtin classes without a declaration
  keep today's module-lookup fallback.
- Companion hardening: `IRVerifier::verifyBlock` now checks every
  instruction's operands via `getOperands()` first, so any future null
  operand is a loud E1000, never a compiler SIGSEGV.
  (`compiler/src/ir/IRVerifier.cpp`.)

### Tests

- IR: `CtorCallingMethodLowers`.
- Execution: `CtorCallingMethod`, `CtorCallingMethodBare`.

### Live proof

```
id=user-123 name=Shanto
DONE
```

(both the `this.` and the bare form)

---

## 4. A method could only call earlier-declared methods; bare calls did not resolve

### What was wrong

```sp
class PaymentService {
    fn processPayment(int amount) -> bool {
        var ok = this.payByCard(amount);   // segfault (exit 139)
        return ok;
    }
    fn payByCard(int amount) -> bool { /* ... */ }   // declared LATER
}
```

The §75 doc-gate shape (public method first, private value-returning
callees after, result assigned) segfaulted with the same `format()`
backtrace as fix 3. (Void-only forward calls happened to pass, which
is why this looked order-dependent: only **value-returning** forward
calls crashed.) Separately, bare calls failed at sema:

```sp
class Banking {
    fn deposit(int amount) { log("deposited"); }   // E1001: No matching overload
    fn log(string msg) { println("log: ${msg}"); } //   for function 'log' with argument types (string)
}
```

### Root cause

- Forward calls: same as fix 3 — the vtable slot's `functionType`
  was unset, so the return type came out void.
- Bare calls: builtin free function `log(double)` is found by
  `lookupFunctions`, its overload resolution fails, and the
  free-function branch in `TypeChecker::visit(CallExpr)` returned that
  error immediately — the enclosing-class fallback below it never ran.
  `DiagnosticEngine` has no rollback API (`reportError` prints
  immediately; only `clear()` exists).

### Fix

- Sema (`compiler/src/sema/TypeChecker.cpp`): `resolveOverload`
  gained a defaulted `bool reportErrors = true` parameter. The
  free-function branch now calls it quietly (`false`) and falls
  through to the super/method/constructor checks when it fails; the
  method path keeps reporting, so exactly one error is reported if
  both fail. Free-function-first precedence is preserved — only
  currently-failing calls are rescued.
- Sema also marks method call targets `isMethod=true`
  (`FunctionSymbol` ctor, enclosing-class + member + interface
  branches): IRBuilder needs to tell a method apart from a
  same-named — even same-signature — free function or builtin.
- IRBuilder (`compiler/src/ir/IRBuilder.cpp`): the "method on
  'this'" block moved **before** the builtin special cases
  (`sqrt`/`log`/`panic`/…), so a same-named method is never shadowed
  by a builtin — but only when sema resolved the call to a method
  (`isMethod`); when sema picked the builtin, the builtin still wins.
  The plain-name fallback now runs only for calls sema left
  unresolved.
- `VTableBuilder::buildSpecializedClassLayout` also registers the
  overload key (`methodName + signature suffix`) in `methodSlotMap`,
  so the keyed lookup works for generic classes too (they were
  keyed by plain name only).
- The `buildClassLayout` `functionType` fix from §3 covers the
  forward-call return type (bare and `this.` alike).

### Tests

- IR: `LaterDeclaredValueMethodLowers` (expects `call i1`),
  `BareMethodCallLowers`, `BareValueMethodCallLowers` (expects
  `call i32`), `BareMethodCallNotShadowedByMathBuiltin` (no
  `call double @sp_rt_math_log`, `Banking_log` present),
  `BuiltinLogDoubleStillWinsWhenSemaPicksIt`
  (`call double @sp_rt_math_log` present). Note: the module prologue
  always declares the math builtins, so the assertions check for the
  *call*, not the declaration.
- Execution: `LaterDeclaredValueMethod`, `BareMethodCall`,
  `BareValueMethodCall`, `BuiltinLogDoubleStillWinsWhenSemaPicksIt`
  (`v=0.916…`, the method's `log:` line absent).

### Live proof

```
Initiating checkout of $150...
Charging $150 via Credit Card gateway.
Payment settled successfully.
Initiating checkout of $420...
Generating on-chain transaction for $420.
Payment settled successfully.
DONE
```

```
validated 50
log: deposited
DONE
```

```
v=0.9162907318741551
DONE
```

---

## 5. `??` generated invalid LLVM IR

### What was wrong

```sp
string? maybe = null;
string name = maybe ?? "default";
// /tmp/sp_ir_*.tmp.ll:611: error: invalid operand type for instruction
//   %b1 = add ptr %v0, @.str.0
```

### Root cause

`??` is a `BinaryExpr` with `QuestionQuestion` op (parser precedence
`NullCoalescing`; TypeChecker computes the common type of
underlying(lhs) and rhs) but `IRBuilder::visit(BinaryExpr)` had **no**
`QuestionQuestion` case — the switch's `default:` emitted `Add`,
invalid for pointers.

### Fix

A `??` case right after the `&&`/`||` short-circuit block, with the
same shape: evaluate LHS, null-check (`icmp eq null` for pointer
types; nullable scalars use their zero value), cond-branch to a
rhs block (evaluate RHS, store) vs a merge block; the result goes
through an entry-block alloca like the `&&`/`||` pattern (and
`TernaryExpr`). Result IR type = sema's common type. A struct-typed
LHS is a loud E1000 (structs have no null value in the IR).

Files: `compiler/src/ir/IRBuilder.cpp` (`visit(BinaryExpr)`).

### Tests

- IR: `NullCoalesceLowers` (no `add ptr`, has `icmp eq`).
- Execution: `NullCoalesce` (string + int cases).

### Live proof

```
name=default
name2=hello
v=42
DONE
```

Related null edges verified live: `p5b_null_int` (`n is null`,
`z(0) is null`, `s(7) is NOT null`, `t is null`), `p5c_null_ret`
(`x=9`, `n=5`), `p5d_null_ret2` (`v is NOT null`).

---

## 6. `?.` on a null object segfaulted

### What was wrong

```sp
User? user3 = findUser(999);              // null
println("User 3 name: ${user3?.name}");   // Segmentation fault
```

### Root cause

`?.` is a `MemberExpr` with `isSafeNav`, but
`IRBuilder::visit(MemberExpr)` called `emitMemberPointer`, which
GEPs/loads the field with **no null check**. TypeChecker sets the
safe-nav result type to the plain field type (e.g. `string`), not
nullable — so the runtime value for a null object is the null/zero
value of the field's IR type (nullptr for pointers, 0 / 0.0 / false
otherwise).

### Fix

In `visit(MemberExpr)`, when `node.isSafeNav()`, emit a null check on
the object → a null block storing the field type's null value
(new `IRBuilder::nullValueOf` helper), an access block loading the
field, and a merge block loading the result slot — the same
cond-branch pattern as `&&`/`||`/`??`. `?.` method calls got the
same treatment in the CallExpr member path
(`IRBuilder::emitMemberPointer` gained an optional pre-evaluated
object parameter). No flow narrowing: `?.` does not narrow the type
after the access (that is a type-system feature, out of scope —
see Follow-ups).

Files: `compiler/src/ir/IRBuilder.cpp`,
`compiler/include/sp/ir/IRBuilder.h`.

### Tests

- IR: `SafeNavLowersWithNullCheck` (has `icmp eq`).
- Execution: `SafeNavOnNullDoesNotCrash`.

### Live proof

```
User 1 name: Shanto
User 3 name:
DONE
```

---

## 7. fn-pointer HTTP handlers segfaulted the server on first request

### What was wrong

```sp
fn hello(HttpRequest req) -> HttpResponse { return req.text("hi typed", 200); }
var app = new App();
app.get("/hello", hello);
app.listen(18081);
// curl http://127.0.0.1:18081/hello  ->  exit 52, Segmentation fault in the server log
```

The `(ptr req)` form worked (`Context.create(req)` path, as used by
the registry backend); the typed `HttpRequest` form crashed on the
first request.

### Root cause

`sp_rt_http_server_route_handler` called `handler_fn(&req)`, passing
a raw C++ `HttpRequest*` — but a typed SP handler expects an SP
`HttpRequest` **object** (slot 0 = vtable ptr, slot 1 = handle); the
handler's `req.text()` did a vtable dispatch through slot 0 of the
C++ object → garbage vtable → segfault. A probe-object approach was
ruled out at sema (SP does not allow passing a class object where
`ptr` is expected), and no `-rdynamic`/`--export-dynamic` exists in
the linker code, so dlsym was not viable.

### Fix — weak-symbol vtable wrap

The linker uses precompiled static `libsp_runtime.a`, so a weak
undefined reference in the runtime resolves against the module's
global at final link time (or null if absent). In `NetBridge.cpp`:

- `extern "C" void* _vtable_HttpRequest __attribute__((weak));`
  (the vtable global is `@_vtable_HttpRequest`, external linkage,
  emitted by `VTableBuilder`).
- New `callHandlerFn()` helper, used by both
  `sp_rt_http_server_route_handler` and the group variant
  `sp_rt_http_group_route_handler`: if the weak symbol is present,
  allocate a 16-byte SP object via `sp_rt_alloc` ([vtable, &req]) and
  pass it, freed after the handler returns; otherwise fall back to
  the raw pointer, keeping `(ptr req)` handlers working (also when
  the program never imports std.http and the symbol is null).
- New `sp_rt_http_req_unwrap(req)`: a wrapped SP object yields its
  raw handle, a raw pointer passes through. `Context.init` /
  `Context.create` (`stdlib/web/context.sp`) pass the pointer through
  it, so `req.handle` is always the raw handle; the extern is declared
  in `stdlib/http/http.sp`.

The wrapper object lives only for the request — a handler must not
retain the request object past the request. (The other route ABI,
`sp_rt_http_server_route` — 3-string `const char* (*)(method, path,
body)`, used by `HttpServer.route` — is a separate path and is
unchanged. `App` has no port-taking constructor: `new App()`, then
`app.listen(port)`; `listen()` returns immediately, so the server
needs a sleep loop.)

Files: `runtime/src/net/NetBridge.cpp`,
`runtime/include/sp/runtime/Runtime.h`, `stdlib/http/http.sp`,
`stdlib/web/context.sp`.

### Tests

- Execution: `NamedFnHttpHandlerTypedRequest` (spawns the compiled
  server, polls curl, asserts body + 200, kills by PID; uncommon
  port, `timeout` bound; `std::system` precedent in
  `tests/runtime/WssTest.cpp`).

### Live proof

```
$ curl -s http://127.0.0.1:18080/hello        # (ptr req) named fn
hi from handler
HTTP 200

$ curl -s http://127.0.0.1:18081/hello        # typed HttpRequest named fn
hi typed
HTTP 200

$ curl -s http://127.0.0.1:18081/missing
No route matched GET /missing
HTTP 404
```

---

## Tests added

| File | Tests |
|---|---|
| `tests/ir/CodegenCrashersIRTest.cpp` (new) | 12 |
| `tests/codegen/CodegenCrashersExecutionTest.cpp` (new) | 14 |
| **Total new** | **26** |

Registered in `tests/CMakeLists.txt` (the only CMake edit). Both
files use the established harness patterns (`compileToIR` + `dumpIR` +
`hasInstruction` from `TryCatchTest.cpp`; `runSPProgram` from
`TryCatchExecutionTest.cpp`, with `ModuleLoader::loadAndMergeAll`).

## Verification

- Fresh build clean in `~/workspace/build-phase6-codegen` (Release,
  `-j2`; local googletest via
  `-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=…/build-phase4-parent/_deps/googletest-src`).
  One trigraph warning (`??'` in a diagnostic string) found and
  fixed by rewording.
- Full ctest: **760/761 passed (99%)**, total time 113.83 s.
  Only failure: `NetTest.UdpSocketSendRecv` (sandbox UDP EPERM,
  pre-existing). The 3 ExecutionTest collection tests that were flaky
  under full-suite load at the Phase 5 baseline (731/735) all passed
  this run.
- Live proofs: all 19 proof programs in `~/workspace/p6a-proofs/`
  re-run on the final binary (map string/int/empty, interface direct
  + List + swapped order, ctor `this.` + bare, method order §75 exact
  shape, bare calls incl. `log` shadowing, `??` string/int/null
  edges, `?.` null + non-null, HTTP named-fn `(ptr req)` + typed
  `HttpRequest` via curl). Exact outputs are quoted per fix above.

## Files changed (uncommitted)

```
compiler/include/sp/codegen/VTableBuilder.h   interface slots + keyed generic layout
compiler/src/codegen/VTableBuilder.cpp        global interface slots, functionType in
                                              buildClassLayout, keyed entries in
                                              buildSpecializedClassLayout
compiler/include/sp/sema/TypeChecker.h        resolveOverload reportErrors param
compiler/src/sema/TypeChecker.cpp             quiet free-fn fallback, isMethod targets
compiler/include/sp/ir/IRBuilder.h           nullValueOf, emitMemberPointer param
compiler/src/ir/IRBuilder.cpp                 map for-in, ??, ?., method-block move,
                                              interface dispatch via layouts
compiler/src/ir/IR.cpp                        map key-accessor externs
compiler/src/ir/IRVerifier.cpp                null-operand guard
runtime/include/sp/runtime/Runtime.h          sp_rt_map_*_key_at, sp_rt_http_req_unwrap
runtime/src/Runtime.cpp                       sp_rt_map_str/int_key_at
runtime/src/net/NetBridge.cpp                 weak-symbol vtable wrap, callHandlerFn,
                                              sp_rt_http_req_unwrap
stdlib/http/http.sp                           sp_rt_http_req_unwrap extern
stdlib/web/context.sp                         unwrap in Context.init / create
tests/CMakeLists.txt                          register the 2 new test files
tests/ir/CodegenCrashersIRTest.cpp            new, 12 tests
tests/codegen/CodegenCrashersExecutionTest.cpp new, 14 tests
docs/v2/PHASE-35-SLICE-A.md                   this file
```

## Follow-ups / honest limitations

- **Interface inheritance has no parser syntax.** `interface A
  extends B { … }` fails loudly at parse time (E1000). The
  interface-slot code already walks super-interfaces recursively,
  so the runtime side is ready — the parser is the missing piece.
  Out of scope for slice A.
- **Generic class → interface assignment is not supported.**
  `Drawable d = new Box<int>(…)` fails loudly at sema (E1001 type
  mismatch). Interface dispatch itself works; the assignment side
  needs a generic-implements check. Out of scope.
- **Map iteration order is unspecified** (hash-table order) and the
  map must not be mutated during iteration. Documented in the
  runtime comment; no loud error is possible cheaply here.
- **`??` RHS await in async fns** is the same untested edge as the
  existing `&&`/`||` entry-alloca pattern — the pre-scan may not
  cover it. Worth a test if async `??` use appears.
- **`?.` does not narrow the type** after the access, and there is
  no `!` unwrap — both are type-system features needing a design
  decision (explicitly out of scope for Phase 6).
- **HTTP wrapper lifetime:** the wrapped request object is freed
  after the handler returns; retaining it past the request is a
  use-after-free. No loud guard exists yet.
- **Cross-function throw unwinding** (throw in a called function
  caught by the caller's `try`) is still same-function-only and
  documented — needs a design decision, explicitly out of scope.
- **Capturing lambdas** remain deliberately unsupported (loud
  E1000), unchanged.
- Repo `AGENTS.md` mandates `python -m graphify update . --force`
  after edits, but `graphify-out/` does not exist and `graphify` is
  not installed in this environment — the step could not be run and
  was not fabricated.
