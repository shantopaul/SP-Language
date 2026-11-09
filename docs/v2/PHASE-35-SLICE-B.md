# Phase 35 (pro-4.0.0) — Slice B: Statics + Init Semantics

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `0c5ebbe` (Phase 5 / Phase 34 complete, `main` still `d10e1c2`)
- **Version:** 3.1.0, unchanged (`compiler/include/sp/Version.h` is still the
  single source of truth; no bump, no `.github/` changes, no ADR-001–005
  items touched)
- **Source:** Phase 5 slice D's doc-gate findings
  (`docs/v2/PHASE-34-SLICE-D.md` §3.1) — eight real semantics bugs the
  doc examples had tripped over, each previously worked around in the
  docs instead of fixed.
- **Status:** all 8 fixed, 16 new execution tests, full ctest **750/751**
  (only failure: `NetTest.UdpSocketSendRecv`, sandbox UDP EPERM).
  All changes uncommitted in the slice-B worktree.

## Summary

| # | Bug (baseline behaviour) | Fix | Proof |
|---|---|---|---|
| 1 | Field initializers with `new` silently dropped; field stays null, use panics `NullPointerException` | Initializers run in declaration order at the start of every constructor | `n=7`, `items size=2`, `items[0]=1` |
| 2 | Static field initializers dropped; static mutable state reads back zero; `MathUtils.PI` from outside the class segfaults | Statics live in module globals, initialized by a per-class `__sp_static_init_<Class>()` called from `main` | `PI=3.14159`, `bump1=1`, `bump2=2`, `counter=2` |
| 3 | `map[k]` returns nullable (`int?`) while `HashMap.get()` returns plain `V` | `map[k]` returns plain `V`; missing keys read back the zero value, like `HashMap.get()` | `a=42`, `missing=0`, `hb=7`, `hmissing=0` |
| 4 | Interpolating a `Set` prints raw pointer bytes; `Set` is not iterable | `Set` prints `{3, 2, 1}` and supports `for (x in s)` | `set={3, 2, 1}`, `elem=3/2/1` |
| 5 | `int.parse("abc")` silently returns `0` | Strict parse; invalid input panics naming the input | `SP Panic: int.parse: invalid integer 'abc'` |
| 6 | `reduce()`'s accumulator is typed `any`; 1-arg `List.reduce` has no overload | 1-arg `List.reduce` overload registered; accumulator is the element type | `sum=55`, `joined=abc` |
| 7 | Default arguments not applied at method call sites (`r.text("hello")` → E1001) | Sema attaches the method's AST decl to candidates; IR appends evaluated defaults at all 4 method-call sites | `hello 200`, `hello 404`, `world 200` |
| 8 | `OS.args()` returns an empty array even for a built binary run with arguments | `main` takes `(argc, argv)` and forwards them to the runtime | `argc=2`, `arg[0]=hello`, `arg[1]=world` |

Proof programs live in `~/workspace/p6b-proofs/` (`p1`–`p8` plus
variants `p1b`, `p1c`, `p2b`, `p2c`, `p4b`, `p6b`–`p6f`, `p7b`).

## Fix 1 — Field initializers run in every constructor

**What was wrong.** A field initializer was never evaluated. `sp_rt_arc_alloc`
zeroes the payload, the constructor body ran, and the field stayed null:

```sp
class Counter {
    public int n = 7;
    public List<int> items = new List<int>();
    public Counter() { this.items.add(1); this.items.add(2); }
}
fn main() {
    Counter c = new Counter();
    println("n=${c.n}");            // baseline: n=0
    println("items size=${c.items.size()}");  // baseline: SP Panic: NullPointerException: list is null, Aborted
}
```

**Fix.** New helper `IRBuilder::emitFieldInitializers(classDecl, layout,
thisVal)` (`compiler/src/ir/IRBuilder.cpp`, declared in
`compiler/include/sp/ir/IRBuilder.h`): for each non-static field with an
initializer, in declaration order, evaluate the initializer and store it
into the field slot (after the vtable store, before the constructor body).
Refcounting mirrors `visit(VarDeclStmt)`: a `new` transfers its owning
reference, anything else is retained (`sp_rt_retain`). Called from
`compileDefaultConstructor`, `compileConstructor`, and
`compileSpecializedConstructor` (see below).

**Generic classes.** The specialized-constructor path originally had no
access to the template's field initializers, and the first proof
(`p1c`: generic class with an explicit ctor) confirmed the gap
(`label=` — empty). Fixed by passing the template `ClassDecl` into
`compileSpecializedConstructor` and running the same helper with a
type-parameter filter: initializers for fields whose declared type
mentions a type parameter (`T`, `List<T>`) are skipped — their
expressions may depend on the type argument and there is no substitution
machinery for them. Non-generic-typed fields (`string`, `int`,
`List<int>`) initialize normally. A generic class with the *default*
constructor already went through `compileDefaultConstructor`, so it
worked without this change (`p1b`: `label=box`).

**Tests.** `StaticsExecutionTest.FieldInitializerWithNew`,
`.GenericClassFieldInitializerDefaultCtor`,
`.GenericClassFieldInitializerExplicitCtor`.

**Live proof (final binary).**
```
n=7
items size=2
items[0]=1
```
`p1b`: `label=box`. `p1c` (after the follow-up fix): `label=box`.

**Limits (pre-existing, not changed).** Base-class field initializers do
NOT run — there is no constructor chaining, so a derived object never
runs its base's initializer list. Reassigning a field in the constructor
body orphans the initial value (fields are never released at object
death — no destructor is generated).

## Fix 2 — Static fields are real storage

**What was wrong.** Static fields existed in sema only. They were skipped
in the instance layout (`VTableBuilder.cpp`: `if (varDecl.isStatic())
continue;`) and never allocated anywhere, so:

```sp
class MathUtils {
    public static const double PI = 3.14159;
    public static int counter = 0;
    public static fn bump() -> int {
        MathUtils.counter = MathUtils.counter + 1;
        return MathUtils.counter;
    }
}
fn main() {
    println("PI=${MathUtils.PI}");       // baseline: spc segfault, exit 139
    println("bump1=${MathUtils.bump()}"); // baseline: bump1=0 (read back zero)
}
```

The segfault's mechanism: `visit(IdentifierExpr)` lowers an unresolved
class name to `ConstantInt 0`, so `MathUtils.PI` read field 0 of address
0. (A related trap: §22's "static const works inside the class" was only
the hardcoded `Math.PI`/`Math.E`/`Math.TAU` fallback in
`visit(IdentifierExpr)` — a class's own `PI` constant never worked
either; the value `3.141592653589793` just happened to equal it.)

**Fix.**
- `compiler/include/sp/codegen/VTableBuilder.h` / `compiler/src/codegen/VTableBuilder.cpp`:
  new `StaticFieldInfo` (name, type, IR type, global name
  `@<Class>_<field>`, const flag) and `ClassLayout::staticFields`.
  `buildClassLayout` registers each static field as a module global
  (`IRModule::addGlobalVariable`, `zeroinitializer`, never `constant` —
  the static-init function must be able to store into it) and copies a
  base class's statics into a derived layout, so `Derived.baseVal`
  resolves to the base's global unless the derived class hides it.
- `IRBuilder::compileStaticInitializer(classDecl, layout)`: generates
  `__sp_static_init_<Class>()`, which stores every static field
  initializer into its global (same retain rule as fix 1), in
  declaration order. Generated right after the class's methods, so an
  initializer may call the class's own static methods. Recorded in
  `IRBuilder::m_staticInitFunctions`; `emitStaticInitCalls()` emits the
  calls from `main` (both the explicit-`main` and the synthesized-main
  prologues), before any user code runs.
- `IRBuilder::emitMemberPointer` intercepts static access *before*
  evaluating the object (`ClassName.field`, or `this.field` where the
  field is static) and returns the global address. A name that resolves
  to a local variable wins over a class of the same name.
- `visit(IdentifierExpr)`: a bare static field of the enclosing class
  (`PI`, `counter` inside a static method) loads its global — checked
  *before* the `PI`/`E`/`TAU` fallback, so a class's own static wins
  over the builtin constant.
- `visit(AssignExpr)`: assigning to a bare static field name inside the
  class stores into the global (compound `+=` etc. work too, via the
  shared load/store path).

**Tests.** `StaticsExecutionTest.StaticConstReadFromOutside`,
`.StaticMutableStatePersists`, `.BareStaticAccessInsideClass`,
`.InheritedStaticViaDerivedName`.

**Live proof (final binary).**
```
PI=3.14159
env=Production
bump1=1
bump2=2
counter=2
```
`p2b` (bare access + static-init-from-static `static int b = a + 5`):
`b=15`, `getB=15`, `incA=11`, `a=11`.
`p2c` (inheritance): `derivedBase=7`, `derivedTag=base`, `own=3`, `base=7`.

**Limits.** Static initialization order is class *declaration* order: if
class A's static initializer reads class B's static field and B is
declared after A, A's initializer sees B's field still zero (the same
static-init-order situation as C++). Cross-class static references must
respect declaration order. `super.field` for statics is not supported —
use the class name. `this` in a static initializer was not tested and
should be rejected by sema; it was not verified here.

## Fix 3 — `map[k]` returns the plain value type

**What was wrong.** Sema typed a map index-read as nullable (`int?` /
`CacheItem?`) while `HashMap.get()` returned the plain value type `V`.
The two ways of reading a map disagreed, and the nullable claim was
false: the runtime (`sp_rt_map_get_str/i64`) returns `0` for a missing
key and has no missing signal at all.

```sp
Map<string, int> m = new Map<string, int>();
m["a"] = 42;
int a = m["a"];          // baseline: E1001 (cannot assign int? to int)
```

**Fix (decision: plain `V` wins).** `TypeChecker::visit(IndexExpr)` now
types a map index-read as the plain value type, matching
`HashMap.get()`. The runtime behaviour is unchanged: a missing key reads
back the zero value (`0` / `""` / `false` / `null`). Use `containsKey()`
to test presence. `??` on a non-nullable left-hand side just returns the
common type (no error), so existing `??` uses keep working.

**Tests.** `StaticsExecutionTest.MapIndexReturnsPlainValue`.

**Live proof (final binary).**
```
a=42
missing=0
hb=7
hmissing=0
```

## Fix 4 — `Set` prints and iterates

**What was wrong.** Interpolating a `Set` printed the raw pointer bytes
of the backing table, and `for-in` over a `Set` was rejected:

```sp
Set<int> s = new Set<int>(); s.add(1); s.add(2); s.add(3);
println("set=${s}");        // baseline: set=M-pM-"^QM-wTd (pointer bytes)
for (int x in s) { ... }    // baseline: E1001 "For-in loop requires an iterable collection or array, got 'Set<int>'"
```

**Fix.**
- Runtime (`runtime/src/Runtime.cpp`, declared in
  `runtime/include/sp/runtime/Runtime.h`): new `sp_rt_set_get_i64` /
  `sp_rt_set_get_str` (index walk over occupied entries; out-of-range
  panics) and `sp_rt_set_to_string` (renders `{1, 2, 3}` / `{a, b}` /
  `{}` — ints first, then strings, in table order). A `Set` is backed by
  `SPMapImpl`; it stores pointers via string entries and everything else
  as `i64` (doubles are truncated — pre-existing).
- `IRBuilder::convertToStringValue`: a sema type of `Set<...>` now calls
  `sp_rt_set_to_string` (the `callReg`/`semaType` values were hoisted
  above the pointer early-return so the branch can see them).
- `TypeChecker::visit(ForInStmt)` + `IRBuilder::visit(ForInStmt)`: a
  `Set<T>` branch — length from `sp_rt_set_size`, element type from
  `Set<T>`, elements fetched with the new getters.

**Tests.** `StaticsExecutionTest.SetInterpolationPrintsBraces`,
`.SetForInIteration`.

**Live proof (final binary).**
```
set={3, 2, 1}
size=3
elem=3
elem=2
elem=1
```

**Limits.** Iteration order is hash (table) order, **not** insertion
order — `{3, 2, 1}` above for the inserted `1, 2, 3`. Do not mutate a
set during iteration. `HashSet` (the SP class wrapping `Set`) was out of
scope and is unchanged.

## Fix 5 — `int.parse` fails loudly on invalid input

**What was wrong.** `int.parse("abc")` returned `0` silently — the same
value as a successful parse of `"0"`, so bad input was
indistinguishable from good input.

**Fix (decision: loud panic, keep returning plain `int`).** The runtime
parsers `sp_rt_str_to_int` / `_double` / `_float`
(`runtime/src/Runtime.cpp`) are now strict: trim whitespace on both
ends, base 10 only, full consumption required. Invalid input panics via
`sp_rt_panic` naming the input:
`SP Panic: int.parse: invalid integer 'abc'`. The old parser silently
parsed `0x..` hex / `0..` octal (base 0) and ignored trailing junk
(`42abc` → `42`) — all removed. Returning `int?` / `Option<int>` was
rejected: it would break every caller (`int.parse` is used in doc §§7
and 68, `examples/07_console_io.sp`, and the backend-crud-app
controllers), and scalar nullables have no runtime representation
anyway (`mapType(Nullable)` unwraps to the underlying type).
`bool.parse` is left as-is (it returns `false` for non-`"true"`/`"1"`,
which is a valid bool, not a silent error). `readLine()` strips the
newline, so strict parsing is safe for §7's console-input examples.

**Tests.** `StaticsExecutionTest.IntParseValid`,
`.IntParseInvalidPanics` (asserts the panic message is printed and the
line after the parse never runs; the child aborts with SIGABRT).

**Live proof (final binary).**
```
ok=42
SP Panic: int.parse: invalid integer 'abc'
Aborted
```
("after bad" is never printed; `spc run` reports the abort.)

**Doc impact — flag for slice D.** §68's `parsePort("abc")` example
currently expects an `Err` return; it will now panic instead and must
be rewritten.

## Fix 6 — `reduce()` typing

**What was wrong.** The accumulator parameter of `reduce()` was typed
`any`, so the standard numeric/string cases could not add it, and the
1-argument `List.reduce` overload did not exist at all:

```sp
int sum = numbers.reduce((int acc, int x) => acc + x, 0); // baseline: worked (2-arg, typed)
int sum = numbers.reduce((int acc, int x) => acc + x);    // baseline: E1001 "No matching overload"
```

(Baseline detail: 2-arg typed reduce already worked, including chained
`filter→map→reduce` and array reduce. The broken parts were the 1-arg
`List.reduce` and the `any` accumulator.)

**Fix.** `compiler/src/sema/Type.cpp`: a 1-arg `List.reduce` overload is
registered alongside the 2-arg one (`addMethod` pushes to a vector, so
overloads are supported). The IR lowering already handled the 1-arg
form (initial value `0`; `sp_rt_str_concat` treats null as `""`, so
1-arg string reduce works too). The accumulator must be the element
type.

**Tests.** `StaticsExecutionTest.ReduceOneArgList`,
`.ReduceTwoArgTyped` (regression guard).

**Live proof (final binary).**
```
sum=55
joined=abc
```
1-arg variants (`p6d`): `sum1=6`, `joined1=ab`. Chained
`filter→map→reduce` (`p6c`): `sum=220`. Array reduce (`p6e`/`p6f`):
`arrsum=10`, `arrsum1=10`.

**Limits.** A differently-typed accumulator (a true generic fold, e.g.
reducing `List<int>` into a `string`) is still unsupported. Untyped
lambda parameters still fail with E1001 (`Operator '+' cannot be
applied to types 'unknown' and 'unknown'`) — that is the general lambda
rule ("lambdas need typed params"), not a reduce bug, and is out of
scope for this slice (`p6b` still errors, deliberately).

## Fix 7 — Default arguments applied at method call sites

**What was wrong.** Free functions honoured defaults (sema's
`resolveOverload` counts non-default parameters via the AST decl, and
the IR filled them in), but methods did not — a method call with fewer
arguments than parameters was rejected outright:

```sp
class Responder {
    public fn text(string content, int status = 200) -> string { return content; }
}
r.text("hello");   // baseline: E1001 "No matching overload for function 'text' with argument types (string)"
```

**Fix — two parts.**
- *Sema.* `MemberMethod` (`compiler/include/sp/sema/Type.h`) gains an
  `astDecl` pointer (null for builtins, which keep exact-arity
  matching). `NameResolver` sets it when registering class and interface
  methods, and `TypeChecker` attaches it to the candidate
  `FunctionSymbol`s at all three method-resolution sites
  (enclosing-class methods, class methods, interface methods), so the
  existing `resolveOverload` default logic (`minParams`) now applies to
  methods exactly as it does to free functions.
- *IR.* New helper `IRBuilder::appendDefaultArgs(node, loweredArgs,
  declArgOffset, irParams)` (`compiler/src/ir/IRBuilder.cpp`): for each
  omitted trailing parameter with a default, evaluate the default
  expression at the call site and cast it to the declared parameter
  type (IR param type when available; the sema type as fallback when
  the callee's IR function is not in the module yet, i.e. a forward
  reference to a later-declared class; the sema type for `async`
  callees, whose body params are `(task, ctx)`). Applied at all four
  method-call lowering sites in `visit(CallExpr)`: `super.method(...)`
  (offset 1 — `this` first), vtable dispatch (offset 1), direct/static
  call (offset 1 iff the callee takes `%this`, else 0), and unqualified
  this-method calls (offset 1).

**Tests.** `StaticsExecutionTest.MethodDefaultArgumentApplied` — the
default value is *observed* in the result (`hello 200` vs `hello 404`),
and an unqualified call from inside the class also picks it up
(`world 200`).

**Live proof (final binary).**
```
hello 200
hello 404
world 200
```
(`p7`: `hi Shanto!`, `hi Shanto?`, `hello`, `hello` — the free-function
defaults still work alongside.)

**Limits.** Defaults must be trailing (same rule as free functions — a
default followed by a required parameter binds positionally, as before).
A default that references another parameter fails loudly via the
existing E1000 path (the name is unresolved at the call site).
Constructor defaults (`new C(...)` / `super(...)`) are **not** covered:
sema gives constructor calls no call target and performs no overload
resolution for them, so there is nothing for the IR to fill from — that
needs a sema change of its own.

## Fix 8 — `OS.args()` returns the real argv

**What was wrong.** `sp_rt_os_args` (non-Windows branch) always returned
an empty array, even for a built binary run with arguments:

```sp
string[] a = OS.args();
println("argc=${a.length}");   // baseline: argc=0, run as `prog hello world`
```

**Fix.** The runtime keeps a deep copy of the process arguments
(`sp_rt_set_args(argc, argv)` + `g_sp_argc`/`g_sp_argv`,
`runtime/src/Runtime.cpp`, declared in
`runtime/include/sp/runtime/Runtime.h`); `sp_rt_os_args` now builds the
array from them. Generated `main` — both the explicit-`main` path and
the synthesized top-level-statements path — now takes
`(i32 %argc, ptr %argv)` and calls `sp_rt_set_args` first (the
param-alloca loop was changed to iterate declared params with a
`paramOffset` of 2 for `main`, fixing an out-of-bounds read — `main`
declares no params). The declarations were added to
`compiler/src/ir/IR.cpp`. Program name is excluded: `OS.args()` returns
`argv[1..]` (document this in the stdlib docs).

**Tests.** `StaticsExecutionTest.OsArgsReturnsArgv`.

**Live proof (final binary, built with `spc build`, run as
`./p8_args_bin hello world`).**
```
argc=2
arg[0]=hello
arg[1]=world
```

## Tests added

New file `tests/codegen/StaticsExecutionTest.cpp` (16 tests), registered
in `tests/CMakeLists.txt` — the only CMake edit. Its runner mirrors
`ExecutionTest::runSPProgram` but also passes argv to the child and
reports the child exit status (needed for the `OS.args()` test and the
`int.parse` panic test, where the child dies from SIGABRT).

| Test | Covers |
|---|---|
| `FieldInitializerWithNew` | fix 1 |
| `StaticConstReadFromOutside` | fix 2 (was segfault) |
| `StaticMutableStatePersists` | fix 2 |
| `BareStaticAccessInsideClass` | fix 2 (bare read/assign, static-init-from-static) |
| `InheritedStaticViaDerivedName` | fix 2 |
| `GenericClassFieldInitializerDefaultCtor` | fix 1, generic classes |
| `GenericClassFieldInitializerExplicitCtor` | fix 1, generic classes + explicit ctor |
| `MapIndexReturnsPlainValue` | fix 3 |
| `SetInterpolationPrintsBraces` | fix 4 |
| `SetForInIteration` | fix 4 |
| `IntParseValid` | fix 5 |
| `IntParseInvalidPanics` | fix 5 |
| `ReduceOneArgList` | fix 6 |
| `ReduceTwoArgTyped` | fix 6 (regression guard) |
| `MethodDefaultArgumentApplied` | fix 7 |
| `OsArgsReturnsArgv` | fix 8 |

All 16 pass.

### Existing tests updated (intentional behaviour change)

Fix 8 changed `main`'s signature to
`define i32 @main(i32 %argc, ptr %argv) {`, so four tests that asserted
the old `define i32 @main() {` were updated:

- `tests/ir/IRBuilderTest.cpp`: `HelloSPGeneratesMainAndPrintlnCall`
  (also now asserts the `sp_rt_set_args` call),
  `ArithmeticAndVariables`.
- `tests/codegen/LLVMGeneratorTest.cpp`:
  `GenerateLLVMIRHeaderAndBody`, `EmitLLVMIRToFile`.

Not changed (verified by inspection): `tests/ir/IRTest.cpp:111`,
`DebugInfoGeneratorTest.cpp:64`, `LinkerDriverTest.cpp:66` — those build
or feed hand-written IR with a zero-arg `main` as *input* to the IR
printer / linker, not SP-compiler output, and they pass unchanged.

## Verification

- Fresh Release build (`~/workspace/build-phase6-statics`, `-j2`,
  local googletest copy, no downloads): clean, 0 errors.
- Full ctest: **750/751 passed (99%)**. The single failure is the
  known environmental one, `NetTest.UdpSocketSendRecv` (sandbox UDP
  EPERM). `JsonTest.BenchmarkOneGigabytePerSecThroughput` failed on an
  earlier run under CPU contention (sibling slices building) and passed
  on the final run — it is flaky under load, not a regression.
- Baseline for comparison: Phase 5 parent ctest was 734/735; this run
  has 751 tests (735 + the 16 new ones).
- All 8 proof programs re-run with the final binaries; outputs are in
  the per-fix sections above. The `p6b` untyped-lambda program still
  errors with E1001, deliberately (see fix 6 limits).

## Files changed (uncommitted)

```
compiler/include/sp/codegen/VTableBuilder.h   StaticFieldInfo + ClassLayout::staticFields
compiler/src/codegen/VTableBuilder.cpp        static-global registration, base-static inheritance
compiler/include/sp/ir/IRBuilder.h           emitFieldInitializers / compileStaticInitializer /
                                              emitStaticInitCalls / appendDefaultArgs decls
compiler/src/ir/IRBuilder.cpp                fixes 1, 2, 4, 7, 8 (422 lines changed)
compiler/src/ir/IR.cpp                       sp_rt_set_args / set getters / set to-string decls
compiler/include/sp/sema/Type.h              MemberMethod::astDecl
compiler/src/sema/NameResolver.cpp           attach AST decl to class/interface methods
compiler/src/sema/TypeChecker.cpp            map index plain V, Set for-in, method astDecl attach
compiler/src/sema/Type.cpp                   1-arg List.reduce overload
runtime/include/sp/runtime/Runtime.h         sp_rt_set_args, set getter/to-string decls
runtime/src/Runtime.cpp                      strict parse, argv storage, set getters/to-string
tests/CMakeLists.txt                          register StaticsExecutionTest.cpp (only CMake edit)
tests/codegen/StaticsExecutionTest.cpp        new, 16 tests
tests/codegen/LLVMGeneratorTest.cpp           main-signature expectations (fix 8)
tests/ir/IRBuilderTest.cpp                   main-signature expectations (fix 8)
```

14 files changed, +712/−30, plus the new test file. No version bump, no
`.github/` changes, no ADR items touched, nothing committed or pushed.

## Doc impact — for Phase 6 slice D

The docs currently work around these bugs; the workarounds are now
wrong and the examples need rewriting:

- **§68** — `parsePort("abc")` now panics instead of returning `Err`.
  Rewrite the example to show the panic, or to validate with a guard
  before parsing.
- **§22** — statics are real now: static const/mutable fields, bare
  access inside the class, inheritance, static-init order (declaration
  order). The "static const works inside the class" note was the
  hardcoded `Math.PI` coincidence — remove it.
- **§58** — `Set` prints `{a, b}` and is iterable with `for (x in s)`;
  document hash order (not insertion order).
- **§87** — `OS.args()` returns `argv[1..]` (program name excluded).
- **§95** — 1-arg `List.reduce` works; accumulator must be the element
  type.
- **§99** — `map[k]` returns plain `V` now; the paragraph justifying the
  nullable index-read (and its contrast with `HashMap.get()`) is
  obsolete — both paths agree.
- **§38 / Todo / Banking / §71 / §74 / §97** — field initializers
  (including `new`) now run in every constructor, including generic
  classes; remove the workarounds that initialized fields in the ctor
  body instead.

## Honest follow-ups and limitations

- **Base-class field initializers do not run.** There is no constructor
  chaining, so a derived object never runs its base's initializer list.
  Pre-existing; needs a design decision (chaining semantics), not a
  bugfix.
- **Field values are never released at object death.** Reassigning a
  field in the ctor body orphans the initial value; no destructor is
  generated. Pre-existing ARC gap.
- **Static-init order is declaration order.** A static initializer that
  reads another class's static field sees zero if that class is declared
  later. Same hazard class as C++ static-init order; document it, do not
  silently reorder.
- **Generic `T`-typed field initializers are skipped** in specialized
  constructors (no substitution machinery). Non-generic-typed fields
  initialize fine.
- **Constructor defaults are not implemented** (`new C(...)` /
  `super(...)`): sema gives ctor calls no call target. Needs its own
  sema work.
- **Defaults must be trailing;** a default naming another parameter
  fails loudly (E1000) at the call site.
- **`Set` order is hash order;** do not mutate during iteration; doubles
  stored in a `Set` are truncated (pre-existing storage limitation).
- **`bool.parse` unchanged** (returns `false` for non-`"true"`/`"1"`).
- **1-arg `reduce` requires an element-typed accumulator;** a true
  generic fold to a different type is unsupported.
- **Untyped lambda parameters still E1001** (general lambda rule).
- **Missing map keys read back the zero value;** use `containsKey()` to
  test presence.
- **Cross-function throw unwinding, `?.` flow narrowing / `!` unwrap,
  and ADR-001–005 items** remain out of scope for Phase 6, as planned.
- **Graphify:** the repo `AGENTS.md` requires
  `python -m graphify update . --force` after coding, but `graphify`
  is not installed in this environment and `graphify-out/` does not
  exist, so that step could not be run. Nothing was fabricated in its
  place.
