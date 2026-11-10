# Phase 37 (pro-4.0.0) — Slice B: Container element release

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `ac8ff9a` (ADR-001–005 Accepted; Phase 7 done at `5ea6a69`)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, nothing
  committed or pushed from this worktree — parent merges)
- **Scope:** part of ADR-001 (Hybrid Value Semantics + ARC). Give
  containers a free path: when a container dies, release its retained
  reference-typed elements and free its storage. This closes the
  "container-retained shares leak forever" remainder from Phase 7
  slice C doc §6.
- **Status:** IN PROGRESS in worktree
  `~/workspace/sp-language-review/sp-language-p8-b` (detached at
  `ac8ff9a`).

**Repo AGENTS.md note:** the repo mandates a Graphify protocol
(`python -m graphify update . --force` before/after coding), but
`graphify-out/` does not exist in this worktree and `python -m
graphify` is not installed (verified 2026-11-10, as in Phases 5–7).
It cannot be run; nothing about it is fabricated here.

## 0. Design — written before the implementation

### 0.1 What is wrong today (verified by reading the code)

1. **Containers are not ARC objects.** `sp_rt_list_alloc`,
   `sp_rt_map_alloc`, `sp_rt_set_alloc`, `sp_rt_queue_alloc`,
   `sp_rt_pq_alloc` use plain `new SPListImpl()` etc.;
   `sp_rt_array_alloc` uses plain `sp_rt_alloc`; `sp_rt_channel_create`
   uses plain `new Channel`. None has an ARC header, a refcount, or
   a destructor. `sp_rt_release(container)` is a no-op on all of them
   (the magic check fails), and no `sp_rt_*_free` exists for
   List/Map/Set/Array/Queue/PQ (only `sp_rt_channel_destroy` exists,
   and it just `delete`s the Channel without touching queued items).
2. **Elements are retained into containers but never released.**
   Phase 7-C added `emitContainerElementRetain` (`sp_rt_retain` on a
   reference-typed element at every raw-bits store: push/add/enqueue,
   `set`, array-literal elements, indexed assignment, `Channel.send`).
   The container owns one share per stored reference element. Nothing
   ever gives it back: `sp_rt_list_clear` just sets `length = 0`,
   `sp_rt_list_remove_at` just memmoves, `sp_rt_map_remove_*` frees
   only the string key copy, `sp_rt_map_set_*` overwrites the old
   value without releasing it. Every retained share leaks forever.
3. **Scope drops already fire for most container locals — and do
   nothing.** `emitScopeDrops` releases every variable registered in
   `m_refCountedScopes`. A variable is registered when its initializer
   is a `NewExpr` (so `new List/Map/Set/Stack/Queue/PQ/Channel`
   locals ARE registered) or when its sema type `isReferenceType()`
   (Class/Interface — which covers `List<T>`, `Set<T>`, `Stack<T>`,
   `Queue<T>`, `PriorityQueue<T>`, `Channel`, all resolved to synthetic
   ClassTypes by `TypeContext::resolveType`). The release is emitted
   but is a no-op because the containers are not ARC objects.

### 0.2 Mechanism — ARC-allocate containers, flag the element kind, destruct

**Runtime (`runtime/src/Runtime.cpp`, `runtime/include/sp/runtime/Runtime.h`,
`compiler/src/ir/IR.cpp` for the declarations):**

1. **ARC allocation.** `sp_rt_list_alloc`, `sp_rt_map_alloc`,
   `sp_rt_set_alloc`, `sp_rt_queue_alloc`, `sp_rt_pq_alloc`,
   `sp_rt_array_alloc` allocate their impl struct via
   `sp_rt_arc_alloc(sizeof(Impl), <kind>_destructor)` instead of
   `new` / `sp_rt_alloc`. The impl structs are POD (ints + pointers),
   so `sp_rt_arc_alloc`'s zeroing is a correct initialization.
   The returned pointer is the payload (= the impl struct), exactly
   as today — no caller changes. `sp_rt_stack_alloc` already
   delegates to `sp_rt_list_alloc`, so Stack gets the List destructor
   for free.
2. **One flag per container: `elements_are_refs`.** The runtime
   cannot know the element kind: elements are raw i64 bits, and
   `sp_rt_release` on a small int would read memory at
   `(value - sizeof(header))` and segfault. Each impl struct gains a
   `bool elements_are_refs` field (default `false`). The destructor
   releases elements **only** when the flag is set; otherwise it only
   frees storage. A missing flag degrades to today's leak, never to a
   use-after-free.
3. **Per-kind destructors** (`SPDestructorFn`, run by `sp_rt_release`
   when the count hits 0). A destructor frees the container's
   **internal buffers only** — never the impl struct itself (the ARC
   machinery frees the payload):
   - **List/Stack:** if flag, `sp_rt_release` each `items[i]`;
     `sp_rt_free(items)`.
   - **Array:** if flag, `sp_rt_release` each element (needs
     `element_size` to stride — added to `SPArrayHeader`; all array
     access already goes through the struct, so the layout change is
     internal). No separate buffer: the data is inline after the
     header.
   - **Map:** if flag, `sp_rt_release` each occupied **value** (str
     and int tables); `sp_rt_free` every string-key copy; free both
     entry arrays. Int keys are never released (they are integers).
   - **Set:** a set IS a map, but its elements are the **int-table
     keys** (`sp_rt_set_add_i64` stores the element as the key with a
     dummy value `1`) and the str-table keys (strdup'd string copies).
     Its destructor therefore releases occupied **int keys** when the
     flag is set, frees str keys, frees both entry arrays. It is a
     different function from the map destructor.
   - **Queue:** if flag, `sp_rt_release` each queued item
     (`items[head .. head+count)`); free the ring buffer.
   - **PriorityQueue:** if flag, `sp_rt_release` each `items[i]`;
     free `items` and `priorities`.
   - **Nested containers work recursively:** an element whose static
     type is itself a container resolves to a ClassType (or
     Array/Map), so the outer flag is set and the outer destructor's
     `sp_rt_release` on an inner container runs the inner
     destructor, which releases ITS elements. A self-referential
     container (`xs.push(xs)`) keeps its own count ≥ 1 via the
     self-share, so its destructor cannot run while the cycle exists
     — cycles leak, exactly like class cycles today (no infinite
     recursion: the destructor only runs at count 0, and a share held
     by an element prevents that).
4. **How the flag gets set — compiler-emitted setter, not a signature
   change.** The alloc signatures stay unchanged (no churn in the ~20
   runtime-internal callers or the C++ tests). New setters
   (`sp_rt_list_mark_ref_elements`, `sp_rt_map_mark_ref_elements`,
   `sp_rt_set_mark_ref_elements`, `sp_rt_array_mark_ref_elements`,
   `sp_rt_queue_mark_ref_elements`, `sp_rt_pq_mark_ref_elements`)
   set the flag; the compiler emits the setter call immediately after
   the alloc call, **only when the element type is an owned type**
   (§0.3). The setter is emitted in the same block, before any store
   can happen, so no element can be stored unflagged. Runtime-internal
   allocations (e.g. `sp_rt_list_map` / `sp_rt_list_filter` results,
   string-split arrays) are never marked — see §0.6.
5. **Release on drop, not only on free.** The same flag lets the
   mutating operations release a dropped element's share instead of
   leaking it: `sp_rt_list_remove_at`, `sp_rt_list_clear`,
   `sp_rt_list_set_i64` (overwrite), `sp_rt_array_set_i64`
   (overwrite), `sp_rt_map_set_str` / `sp_rt_map_set_i64`
   (overwrite), `sp_rt_map_remove_str` / `sp_rt_map_remove_i64`,
   `sp_rt_map_clear`, `sp_rt_set_remove_i64` / `sp_rt_set_remove_str`
   / `sp_rt_set_clear`, `sp_rt_queue_clear`, and the PQ clear path.
   The retain-then-release order is already correct: the compiler
   retains the new value BEFORE the store call, so overwriting a slot
   with the same object (`m.set("a", x)` where `"a"` already holds
   `x`) nets to no change in count. `pop` / `dequeue` / `peek` are
   TRANSFERS and must NOT release — see §0.6.
6. **Channel is excluded from ARC and from scope-drop.** `Channel`
   is a C++ class with a mutex and condition variables — it cannot be
   `sp_rt_arc_alloc`'d (which memsets the payload), and its lifetime
   is inherently shared across threads: a lexical scope pop must not
   free a channel another task may be blocked on. Channels keep their
   explicit `sp_rt_channel_destroy` path (already called from SP
   `.destroy()`). As a small, contained addition, `destroy` drains the
   still-queued items and releases them when flagged (flag set from
   the compiler's `Channel.send` retain site via
   `sp_rt_channel_mark_ref_items`); items already received keep their
   shares and are unaffected. `recv` does NOT release (the receiver
   adopts the value through the normal retain rules — releasing on
   recv would free the object before a temporary use of it).

### 0.3 Which element types set the flag — one predicate

`isOwnedContainerElementType(TypePtr t)`: unwrap `Nullable`, then
`true` iff `t->isReferenceType()` (Class/Interface) **or**
`t->isArray()` **or** `t->isMap()`. (`List<T>` etc. are ClassTypes;
`Array<T>` / `T[]` are ArrayTypes; `Map<K,V>` is a MapType.)

The flag is set at the **allocation site**, where the element type is
known from the type system — not at store sites (a store site only
knows one element's type, and runtime-internal stores would never be
instrumented):

| Alloc site (compiler) | Element type source |
|---|---|
| `new List<Foo>()`, `new Set<Foo>()`, `new Stack<Foo>()`, `new Queue<Foo>()`, `new PriorityQueue<Foo>()` (`visit(NewExpr)`) | `node.getTypeArgs()[0]`, resolved via `m_sema.getTypeContext().resolveType(arg)` |
| `new Map<K, V>()` (`visit(NewExpr)`) | `node.getTypeArgs()[1]` (the **value** type; keys are strings/ints, never released) |
| Array literal `[e1, ...]` (`visit(ArrayExpr)`) | the literal's sema type (`ArrayType::getElementType()`); if the node type is missing/error, the flag stays `false` |
| Map literal `{...}` (`visit(MapExpr)`) | never marked — sema restricts literal values to integers |

### 0.4 The compiler must treat containers like classes everywhere — or not at all

Making containers ARC-allocated turns the existing scope-drop
`sp_rt_release` from a no-op into a real free. That is only sound if
every ownership rule that applies to classes also applies to
containers. Today those rules are keyed on `semaType->isReferenceType()`
(+ the `NewExpr` branch), and **ArrayType/MapType fall through all of
them**. Each fall-through is a live use-after-free once the free is
real, so each is widened in this slice with the same predicate as
§0.3 (a single helper, `isOwnedValueType(t)` =
`isReferenceType() || isArray() || isMap()`):

| Site (IRBuilder.cpp) | What it does for classes | Why Map/Array must join |
|---|---|---|
| `emitRefReturnRetain` (:654) | retains a borrowed return value so it survives `emitAllFunctionDrops` | `fn makeMap() { let m = new Map<..>(); return m; }` — without the retain, the drops free `m` and the caller receives a dangling pointer. **This is the load-bearing widening.** |
| `visit(VarDeclStmt)` (:1561) | registers the variable for scope-drop; retains a borrowed initializer | `let xs = [makeFoo()]` / `let m = getMap()` — without registration the container leaks (safe but the slice's point is the free path); without the retain a borrowed initializer would be freed at the pop while the source still owns it. |
| `visit(AssignExpr)` plain `=` (:4131) | retains the new value, releases the old value | `m = otherMap` — without the retain, the pop releases a share the variable never owned (double release when the source's own pop runs); without the old-value release, reassignment leaks the old container. |
| `visit(ThrowStmt)` payload-owns (:2847) | retains a thrown class object into the payload so unwinding drops cannot free it | same hazard for a thrown container (whether or not sema currently allows throwing one, the retain makes the path sound either way). |
| `emitFieldInitializers` (:3053) + static initializer (:2931) | retains a field value unless the initializer is `new` | a field holding a borrowed container must own a share; otherwise the source's pop frees it under the field. (Fields still never drop — no destructors yet, slice A's job — so the share leaks until then, as for classes.) |

**Array/map literals are +1 producers, exactly like `new`.** This
was forced by the counting, not chosen for elegance: an array literal
allocates a fresh container with count 1, but it is not a `NewExpr`,
so without this change a `VarDecl` would retain it (not +1 → retain)
and the scope pop would leave count 1 forever — the array and its
elements would never free. So `isFreshPlusOneExpr` returns true for
`ArrayExpr` and `MapExpr` as well as `NewExpr` and resolved calls,
and the temp-recording hook runs at the tails of `visit(ArrayExpr)`
and `visit(MapExpr)` (and `recordTempCallResult`'s type check widens
to the §0.3 predicate, so a resolved user call returning a Map/Array
used as a pure temporary is released like a class temporary). The
adopter discipline is unchanged: `VarDecl` / `Assign` / `Return` /
`Throw` / field-init adopt the raw value, so an adopted literal
transfers its +1; an unadopted literal is released at the statement
pop (after the call it was passed to — correct, it is dead there).
A literal used as a field initializer behaves like a factory call
(retain + pop-release = net one share in the field); a literal used
as a loop iterable or switch subject is adopted first (live for the
whole loop / every test block), exactly like the class cases in
Phase 7-C §0.4.

**Deliberately NOT widened:** temp recording for **unresolved** call
results returning Map/Array (runtime builtins, extern fns, fn-pointer
calls — convention unknown, the receiver retains per the old rule).
Those temporaries leak one share, as for classes today. A leaking
temporary cannot free a live value.

**Parameters are never registered** (only `VarDeclStmt` and catch
clauses register into `m_refCountedScopes`), so a borrowed container
passed to a function is never freed by the callee — the existing
convention already guarantees this, and the proofs pin it.

### 0.5 Why this cannot free a borrowed value — the safety argument

1. The destructor only runs when the container's own count hits 0.
   The count is owned exactly like a class object's: +1 at creation
   (`new` / alloc), adopted without retain by `VarDecl`/`Assign`/
   `Return`/`Throw`/field-init when the producer is +1, retained when
   the producer is borrowed, released once by exactly one owner
   (scope pop / reassignment / return-drop). The §0.4 widening makes
   the container follow the class rules at every site; a site that
   was missed would leak (a missing retain) or double-release (a
   missing registration) — the proofs in §3 are written to catch
   both directions (reads must stay valid; counts must hit 0).
2. Every stored reference element has a container-owned share
   (Phase 7-C retain, emitted before the store), so the destructor's
   release of an element drops exactly that share — never a share
   owned by a variable, field, return value, or another container.
   Overwrite-then-release order is retain-first (§0.2.5).
3. `sp_rt_release` is null-safe and magic-checked, so releasing a
   null element or an element slot that holds a non-ARC value is a
   no-op — the flag is the only gate, and the flag is set only from
   the static element type.
4. A container with no flag set is freed exactly like today (storage
   freed, elements untouched) — the destructor degrades to the old
   leak, never to a wrong free.

### 0.6 What deliberately still leaks (by design)

- **Derived containers** (`xs.map(f)`, `xs.filter(p)`,
  `arr.map/filter`, string-split arrays, …): allocated inside the
  runtime, never marked. For `map`, the result holds `fn(val)`
  results — which may be borrowed (identity map) or fresh +1s — and
  the runtime cannot tell them apart; marking the result and
  releasing on free would double-release the source's share for an
  identity map. They leak exactly as today (and their elements are
  never double-freed, because the destructor never runs: the
  unresolved-call rule retains an extra share on receipt, so the
  count never reaches 0).
- **`pop` / `dequeue` / `peek` transfers:** the container's share of
  a popped element is not released (the caller adopts the raw
  pointer; the caller's VarDecl retain covers the caller's share).
  One share leaks per popped reference element — pre-existing, same
  as the unresolved-call rule. Closing it needs the +1 convention
  extended to builtin pop/dequeue/recv — a follow-up, not this slice.
- **Unresolved-call Map/Array temporaries** (§0.4): not recorded by
  the temp pass — they leak one share, as for class temporaries from
  unresolved calls today. Fresh-allocation temporaries (literals,
  `new`, resolved user calls) ARE released at the statement pop.
- **Async-function temporaries and short-circuit/ternary temporaries:**
  unchanged from Phase 7-C §6 (recording off there).
- **Containers in fields, globals, statics, and `main` locals at
  process exit:** fields never drop (no `<Class>_destructor` yet —
  slice A, running in parallel, must NOT be depended on);
  globals/statics are never in a scope; `main`'s own locals are
  dropped only on the return paths that run `emitAllFunctionDrops`.
  A container that lives in one of these places leaks its storage
  and element shares — same as a class object there today.
- **Variables declared without an initializer, then assigned**
  (`let m: Map<..>; m = new Map<..>();`): never registered for
  scope-drop (same hole as for classes today) — the assigned
  container leaks. The assignment itself is sound (§0.4 table).
- **Self-referential cycles** (`xs.push(xs)`, mutual container
  cycles): the self-share keeps the count ≥ 1, so the destructor
  never runs — leaks, no crash, same as class cycles.
- **Channel:** not ARC, not scope-dropped (§0.2.6). Only the explicit
  `destroy()` drain releases queued items.

### 0.7 Prerequisite: two +1-convention thefts found while baselining (must be fixed in this slice)

Baselining the proofs exposed two places where the Phase 7-C
`+1` convention is **false** — a builtin returns a *borrowed* value
that every adopter treats as `+1` (no retain), so the adopter's
scope pop releases a share the adopter never owned. Today that only
steals the container's share (the container never releases, so the
count still bottoms out at the right place by accident); **once the
container destructor exists, the same theft double-releases** — the
adopter's pop frees the object and the destructor's release hits
freed memory. Both must be fixed before the destructor lands, or the
slice is unsound. Both are proven by IR dump + strong-count, not by
reading alone.

**Theft 1 — `peek` (Stack/Queue/PriorityQueue) is recorded as a
temporary and released at the statement pop, stealing the
container's share.** Proofs (parent binary, `spc run` +
`spc ir`): `q.peek().n` drops the element's strong count 2 → 1
(`probe_queue_retain.sp`); bare `q.peek();` drops 3 → 2
(`probe_bare_get.sp`); the IR dump shows
`%cast19 = call ptr @sp_rt_list_get_i64(...)` followed immediately
by `call void @sp_rt_release(ptr %cast19)` (`probe_get_theft.sp`
dump, line ~686–688). The same holds for `st.peek()` and
`pq.peek()`. `c_queue_drop` / `c_pq_drop` / `c_stack_drop` baselines
read `after block 0` — a Foo freed while the queue still held it —
because the proof itself reads through `peek().n`.

**Why peek, and not `get`?** Both resolve through the synthetic
methods on the container ClassTypes (`TypeContext::resolveType`),
and `isFreshPlusOneExpr` returns true for any call with a resolved
target — so both *claim* `+1`. Empirically, `get` is never recorded:
bare `xs.get(0)` leaves the count at 2, `takeFoo(xs.get(0))` leaves
it at 2, and `takeFoo(xs.get(i))` with an `int64` index also leaves
it at 2 (`probe_get_theft2.sp`, `probe_get_temp.sp`,
`probe_get_i64.sp`); the IR dump shows no release after
`sp_rt_list_get_i64`. The exact sema mechanism that keeps `get`
unrecorded while `peek` records was not fully pinned down (the
`get` call's sema type evidently fails `recordTempCallResult`'s
`isReferenceType()` check, or its target does not resolve in the
codegen path), so this slice does **not** rely on a mechanism
explanation — it relies on the proven observable rule below, and
the fix is written to match that rule exactly. **Do not retain on
`get`:** nothing releases a `get` result, so a retain there would
leak one share per call.

**Theft 2 — `List.reduce` returns the borrowed accumulator and is
treated as `+1`.** `let r = xs.reduce((Foo a, Foo b) => a, f)`:
the result IS `f` (borrowed, count 2: the variable + the container
share), but `VarDecl` adopts it without retaining — proven by the
count staying 2 after the assignment (`probe_reduce.sp`; if it were
not treated as `+1`, the `VarDecl` retain would have made it 3).
`r`'s scope pop then releases a share `r` never owned. Today the
container never releases, so the count still lands at 0; after this
slice the destructor's element release would be the third release
against two shares — a double-free.

**Fixes (both in `IRBuilder.cpp`, both proven by the counts in §1):**

1. **`peek` — make the `+1` claim true: retain on return.**
   In the shared Stack/Queue/PQ `peek` lowering (~line 6947),
   emit `sp_rt_retain` on the returned element when the call's
   sema type is an owned type (§0.3 predicate, no nullable
   unwrap — mirroring `recordTempCallResult`'s check exactly).
   `peek` never creates: the result is always a borrowed element,
   so the retain is always correct. Then the temp release consumes
   exactly the fresh retain (pure-temp use), and an adopting
   `VarDecl`/`Return`/`Assign` owns a real `+1` (adopted use) —
   and the container's own share stays intact for the destructor.
   The condition must mirror the recording predicate exactly:
   retain iff the temp pass would record/release it, otherwise
   the retain leaks (array/map element peeks: the retain is
   emitted only because `recordTempCallResult` is widened to the
   §0.3 predicate in the same slice, so the temp release consumes
   it there too).
2. **`reduce` — make the `+1` claim false: exclude it.**
   `isFreshPlusOneExpr` returns false for a member call named
   `reduce` on a `List<…>` / array receiver (receiver-type check,
   so a user class's own `reduce()` method — which returns `+1`
   by the convention — is unaffected). Then `VarDecl`/`Return`/
   `Assign`/`Throw` retain the result (they own a real share) and
   the temp pass never records it (a pure-temp use releases
   nothing). A fold that returns a *fresh* object still leaks one
   share (the fold's `new` is `+1`, the result is treated as
   borrowed, the extra retain is never released) — a leak, not a
   UAF, in an exotic shape; noted in §0.6.
3. **`pop` / `dequeue` / `recv` — no change.** `pop`/`dequeue`
   resolve (no-arg) and are recorded — that is the *transfer*:
   the container's share moves to the result, the temp release /
   adoption consumes it, and the container slot is cleared by
   the runtime, so the destructor never releases it again
   (proven: `let x = xs.pop()` leaves the count at 2,
   `probe_pop_transfer.sp`). A retain on `pop` would leak the
   container's share. `Channel.recv` returns `int64` in sema
   (synthetic method, `Type.cpp` ~line 700), so it is never
   recorded and never released — the channel's share of a
   received item simply leaks (pre-existing, §0.6).

**Related pre-existing bug, NOT this slice:** a `.get()` result
cannot be field-read — `xs.get(0).n` and `m.get("a").n` fail IR
verification (`%v15 defined with type 'i64' but expected 'ptr'`:
the `get` lowering leaves `m_lastExprType = Int64`). Index reads
(`xs[0].n`, `m["a"].n`) work. Same family as the `m.get("a")?.n`
bug from Phase 7; slice-A territory. Proofs in this slice read
through indexes, never through `.get().field`.

**Also corrected while baselining:** `isFreshPlusOneExpr` must
recognise the builtin fresh-array producers, or the §0.4 widening
leaks them: `String.split` and array `map`/`filter` return a fresh
array with **no** resolved call target (TypeChecker string/array
branches type them without recording a target), so today they are
treated as borrowed — `VarDecl` would retain a fresh count-1
array → count 2 → drop → 1 → leak. They are added to
`isFreshPlusOneExpr` by (member name, receiver type):
`split` on String, `map`/`filter` on ArrayType. (`List.map`/
`filter` already resolve via the synthetic methods, so they are
already `+1`.) With that, a split result is adopted without
retain (owned count 1) and a pure-temp split is released at the
statement pop by the widened temp recording — both correct.

### 0.8 Boundaries of the owned predicate (nullable)

`isOwnedValueType(t)` does **not** unwrap `Nullable`:
`Foo?`, `List<Foo>?`, `Foo[]?` are not owned types. A nullable
variable is never registered for scope-drop and never retained —
exactly as today, so a nullable container leaks its elements
(safe direction). The flag for `List<Foo?>` is likewise not set
(the element type `Foo?` is not owned): its elements are never
released by the destructor. Nullable containers and nullable
elements are out of scope for this slice; they leak by design,
and no case in this slice relies on releasing one.

### 0.9 Prerequisite: Set<Foo> never stored its elements (pre-existing lowering bug)

While baselining `c_set_drop`, the IR dump showed
`sp_rt_set_add_str` for `s.add(f)` — a Foo stored as a **string**
key. The Set add/contains/remove/for-in lowerings dispatched on
the argument's **IR type** being a pointer, so every pointer-typed
element (class, array, map) was stored as a bogus string key (the
Foo's bytes read as a C string, `strlen`/`strcmp` over object
memory) instead of as an int key holding the pointer bits. That
made `Set<Foo>` unreleasable (the destructor frees str keys; only
int keys are released) and made `for (x in s)` yield garbage
(`n 0` in the baseline). `contains` only worked by accident (the
same bogus comparison on both sides).

**Fix (in this slice, because the set proof cannot pass without
it):** the four Set lowerings dispatch on the argument/element's
**sema type** now — `_str` only when it `isString()`, `_i64`
otherwise. `Set<string>` is unaffected (same `_str` path; proven
by `probe_set_forin.sp`). The Map key lowerings use the same
`isPointer()` pattern, but Map keys are strings/ints by design
(§0.3), so they are correct for every in-scope key type; a Map
with a class key would hit the same bug and is a follow-up, not
this slice.

**Related, same principle — `Channel.trySend`:** Phase 7-C never
retained into `trySend` (unlike `send`), so a reference sent via
`trySend` was borrowed. That alone is a pre-existing bug, but it
becomes a *new* UAF in this slice if `send` marks the channel
and `destroy()` then releases a `trySend`'d item that was never
retained (mixed send/trySend use). So `trySend` gets the retain
**and** the mark, exactly like `send`. `recv` is unchanged.

**Clarification on the reduce exclusion (§0.7):** the exclusion
matters where the reduce result becomes a *pointer* variable
(return, typed `let r2: Foo = …`, assignment to a Foo var) — there
the adopter retains and the count is correct (proven: `strong
after typed reduce assign 3`). A plain `let r = xs.reduce(…)`
holds the result as raw **i64** bits (the runtime returns i64),
so `r` is not a refcounted variable and no share is owned or
released there — `strong after reduce assign 2` is the correct
expectation, not a sign the exclusion failed.

## 1. Proofs — before (parent binary at `ac8ff9a`) vs after (this slice)

All proofs live in `~/workspace/p8b-proofs/`. `Foo` has `n = 42`;
`makeFoo()` is a factory; `sp_rt_mem_live_objects()` counts live
ARC objects; `sp_rt_strong_count(p)` counts one object's shares.
**Reading the counts:** containers are ARC objects now, so an
"inside" count includes the container itself (a List holding one
Foo reads 2, not 1). That is the intended change, not noise — the
"after block" line is the verdict in every drop proof.

| Proof | Before (parent) | After (this slice) |
|---|---|---|
| `c_list_drop` | `inside 1`, `after block 1` | `inside 2`, `n 42`, `after block 0` |
| `c_array_drop` | `inside 1`, `after block 1` | `inside 2`, `n 42`, `after block 0` |
| `c_map_drop` | `inside 1`, `after block 1` | `inside 2`, `n 42` (`m["a"].n`), `after block 0` |
| `c_set_drop` | `inside 1`, `after block 1` (for-in read `n 0`) | `inside 2`, `contains true`, `f strong 2`, `after block 0` |
| `c_queue_drop` | `inside 1`, `after block 1` (peek theft: `after block 0` was a free, not a leak — see §0.7) | `inside 2`, `f strong 2`, `n 42`, `after block 0` |
| `c_stack_drop` | same as queue | `inside 2`, `f strong 2`, `n 42`, `after block 0` |
| `c_pq_drop` | same as queue | `inside 2`, `f strong 2`, `n 42`, `after block 0` |
| `c_nested` (`List<List<Foo>>`) | `inside 1`, `after block 1` | `inside 3`, `outer n 42`, `after block 0` |
| `c_nested_array` (`List<Foo[]>`) | — (new) | `inside 3`, `inner strong 2`, `outer n 42`, `after block 0` |
| `c_borrowed` | `held 1`, `after call 1`, `f strong after call 2`, `n 42` | `held 2`, `after call 2` (the list now counts), `f strong after call 2` (unchanged), `n 42` — borrowed, no UAF |
| `c_returned` | `after list 1`, `after map 2`, `after arr 3` | `after list 2`, `after map 4`, `after arr 6` (containers now count); all three reads `42` — no UAF, the `emitRefReturnRetain` widening holds |
| `c_reassign` | `before reassign 2`, `after reassign 2` | `before reassign 3`, `after reassign 1` (old list + 2 Foos freed), `size 0` |
| `c_remove` | `f1 after removeAt 2`, `f2 after clear 2`, `f1 after mapset 3`, `f1 after overwrite 3`, `f2 after overwrite 4`, `f2 after mapremove 4` | `f1 after removeAt 1`, `f2 after clear 1`, `f1 after mapset 2`, `f1 after overwrite 1`, `f2 after overwrite 3`, `f2 after mapremove 2` — every release path fires exactly once |
| `c_channel_destroy` | `after send 1`, `after destroy 1` | `after send 1`, `after destroy 0` |
| `c_peek_retain` | `strong after peek temp 1` (theft) | `strong after peek temp 2`, `take 42`, `live 2` |
| `c_reduce_borrowed` | `strong after reduce assign 2`, `after block 1` | `strong after reduce assign 2` (i64 var, correct), `strong after typed reduce assign 3` (exclusion pins), `after block 0` |

Full after-outputs (exact, new binary):

```
c_list_drop:    before 0 / inside 2 / n 42 / after block 0
c_array_drop:   before 0 / inside 2 / n 42 / after block 0
c_map_drop:     before 0 / inside 2 / n 42 / after block 0
c_set_drop:     before 0 / inside 2 / contains true / f strong 2 / n 42 / after block 0
c_queue_drop:   before 0 / inside 2 / f strong 2 / n 42 / after block 0
c_stack_drop:   before 0 / inside 2 / f strong 2 / n 42 / after block 0
c_pq_drop:      before 0 / inside 2 / f strong 2 / n 42 / after block 0
c_nested:       before 0 / inside 3 / n 42 / outer n 42 / after block 0
c_nested_array: before 0 / inside 3 / inner strong 2 / outer n 42 / after block 0
c_borrowed:     before 0 / f strong 1 / f strong after push 2 / held 2 / in fn 42 /
                f strong after call 2 / after call 2 / n 42
c_returned:     before 0 / list n 42 / after list 2 / map n 42 / after map 4 /
                arr n 42 / after arr 6
c_reassign:     before 0 / before reassign 3 / after reassign 1 / size 0
c_remove:       before 0 / f1 strong 2 / f2 strong 2 / f1 strong after removeAt 1 /
                f2 strong after removeAt 2 / n 42 / f2 strong after clear 1 / size 0 /
                f1 strong after mapset 2 / f1 strong after overwrite 1 /
                f2 strong after overwrite 3 / f2 strong after mapremove 2
c_channel_destroy: before 0 / after send 1 / after destroy 0
c_peek_retain:  strong after enqueue 2 / take 42 / strong after peek temp 2 / q n 42 / live 2
c_reduce_borrowed: before 0 / strong after push 2 / strong after reduce assign 2 /
                strong after typed reduce assign 3 / f n 42 / r2 n 42 / live 2 / after block 0
```

Probes (also in `~/workspace/p8b-proofs/`, baseline-only except
where noted):

- `probe_set_forin.sp` — Set<Foo> for-in + remove + Set<string>:
  new binary `forin n 42` (baseline `n 0`), `f strong after
  remove 1` (baseline 2), `str size 2`, `str contains true`,
  both string for-in values print. The Set lowering fix works
  and `Set<string>` is unaffected.
- `probe_queue_retain.sp`, `probe_bare_get.sp`,
  `probe_get_temp.sp`, `probe_get_theft2.sp`, `probe_map_get.sp`,
  `probe_pop_transfer.sp`, `probe_get_i64.sp` — the baselining
  probes for §0.7 (peek theft vs get/pop behaviour). See §0.7
  for the numbers; they are the evidence for the retain/no-retain
  decisions, not regression guards.

## 2. Implementation

### Runtime (`runtime/src/Runtime.cpp`, `runtime/include/sp/runtime/Runtime.h`)

- `SPArrayHeader` gains `element_size` (the destructor's stride)
  and `elements_are_refs`. `SPListImpl`, `SPQueueImpl`,
  `SPPQImpl` gain `elements_are_refs`; `SPMapImpl` gains
  `elements_are_refs` (values are refs — Map) and
  `ref_elements_are_keys` (int keys are refs — Set's backing map).
- All six allocs use `sp_rt_arc_alloc` with a per-kind
  destructor (`sp_array_destructor`, `sp_list_destructor`,
  `sp_map_destructor`, `sp_set_destructor`,
  `sp_queue_destructor`, `sp_pq_destructor`). A destructor frees
  internal buffers only (never the impl struct), and releases
  elements only when flagged. `sp_rt_stack_alloc` delegates to
  `sp_rt_list_alloc`, so Stack gets the List destructor free.
- Setters `sp_rt_*_mark_ref_elements` (6) + the declarations in
  `Runtime.h`. Alloc signatures unchanged.
- Release on drop: `sp_rt_list_set_i64` / `sp_rt_array_set_i64`
  (overwrite), `sp_rt_list_remove_at`, `sp_rt_list_clear`,
  `sp_rt_map_set_str` / `sp_rt_map_set_i64` (overwrite),
  `sp_rt_map_remove_str` / `sp_rt_map_remove_i64`,
  `sp_rt_map_clear`, `sp_rt_queue_clear`, `sp_rt_pq_clear`.
  The map functions key the release on the right flag, so a Set's
  backing map releases keys (not the dummy value 1) and a Map
  releases values (not int keys).
- Channel (`runtime/include/sp/runtime/Channel.h`,
  `runtime/src/threading/Channel.cpp`,
  `runtime/src/threading/Concurrency.cpp`): `m_ref_items` flag +
  `mark_ref_items()` / `ref_items()` / `drain_items()` on the
  class; `sp_rt_channel_mark_ref_items` wrapper;
  `sp_rt_channel_destroy` drains and releases still-queued items
  when flagged, then `delete`s the channel. No ARC for Channel
  (mutex + condvars cannot be memset).
- **`sp_rt_free` is ARC-aware** (moved below `sp_rt_release`):
  an ARC pointer routes to `sp_rt_release`, a plain pointer to
  `std::free`. This was forced by the first full ctest run
  (§5): `sp_rt_array_alloc` now returns a payload pointer 16
  bytes past the malloc base, so `std::free`/`sp_rt_free` on it
  aborted with "free(): invalid pointer" in 2 RuntimeTest array
  tests and 10 AsyncTest C-ABI tests (the combinator tests free
  both the input handle array and the results array the caller
  owns). The fix keeps `sp_rt_free` as the runtime's universal
  free — plain pointers (strings, buffers, async frames) are
  unaffected, and a shared ARC object is only decremented, never
  freed under live shares. The AsyncTest CAbi tests now free
  their arrays with `sp_rt_free` (13 sites); the C-string frees
  stay `std::free`.

### Compiler (`compiler/src/ir/IRBuilder.cpp`, `compiler/include/sp/ir/IRBuilder.h`, `compiler/src/ir/IR.cpp`)

- `isOwnedValueType(t)` = `isReferenceType() || isArray() ||
  isMap()` (no nullable unwrap). Widened at:
  `emitRefReturnRetain`, `recordTempCallResult`,
  `recordTempNewResult`, `emitContainerElementRetain`, VarDecl
  registration, AssignExpr `isRefAssignment`, ThrowStmt
  payload-owns, `emitFieldInitializers`, static init.
- `isFreshPlusOneExpr`: `ArrayExpr`/`MapExpr` are +1;
  List/array `reduce` excluded (receiver-type check, so a user
  class's own `reduce()` is unaffected); `String.split` and
  array `map`/`filter` are +1 (fresh arrays with no resolved
  target).
- `recordTempLiteralResult(node)` (new) — the temp-recording
  hook at the tails of `visit(ArrayExpr)` and `visit(MapExpr)`.
- `emitContainerMarkRefElements(markFn, container, elemType)`
  (new) — emits the setter call only when the element type is
  owned. Called at every container alloc site: `new List/Set/
  Stack/Queue/PriorityQueue` (typeArgs[0]), `new Map`
  (typeArgs[1], the value type), array literal (the literal's
  element type). Map literals are never marked (int values only).
- `peek` (Stack/Queue/PQ shared branch): `sp_rt_retain` on the
  returned element when the call's sema type is owned —
  mirroring `recordTempCallResult`'s check exactly. No retain on
  `get`/`pop`/`dequeue`/`recv` (§0.7).
- `Channel.send` / `trySend`: `sp_rt_channel_mark_ref_items`
  when the sent value is owned; `trySend` also gains the missing
  `emitContainerElementRetain` (§0.9).
- Set add/contains/remove/for-in: `_str` vs `_i64` dispatch on
  the argument/element's **sema type** (`isString()`), not the IR
  type being a pointer (§0.9).
- `IR.cpp`: declarations for the six mark functions and
  `sp_rt_channel_mark_ref_items`.

### What was deliberately NOT changed

- Container `new` branches in `visit(NewExpr)` still return
  early without `recordTempNewResult` — a `new List<Foo>()`
  passed straight to a call (`foo(new List<Foo>())`) is not
  recorded and leaks (safe direction; same as an unresolved-call
  temporary). Only literals got the new recording hook.
- No `<Class>_destructor` generation (slice A, parallel — not
  depended on). Element release is `sp_rt_release` per element
  only.
- No version bump, no `.github/` changes, nothing pushed, main
  untouched.

## 3. Tests

12 new execution tests in `tests/codegen/TryCatchExecutionTest.cpp`
(same `runSPProgram` harness as the Phase 7-C temp tests; no new
test infra), plus one updated expectation in the pre-existing
Phase 7-C test:

- `ContainerListDropReleasesElements`
- `ContainerArrayAndMapDropReleaseElements`
- `ContainerSetDropReleasesElements` (also pins the Set.add fix:
  `contains true`, `forin n 42`)
- `ContainerQueueStackPQDropReleasesElements`
- `ContainerNestedListReleasesRecursively` (List-in-List and
  List-of-array)
- `ContainerBorrowedPassedToFunctionIsNotFreed`
- `ContainerReturnedFromFunctionSurvives` (List, Map, array)
- `ContainerRemoveAndClearReleaseElements`
- `ContainerReassignReleasesOldContainer`
- `ContainerChannelDestroyDrainsQueuedItems`
- `ContainerPeekDoesNotStealShare`
- `ContainerReduceBorrowedResultIsRetainedOnAssign`
- `ContainerStoresRetainTempElements` (pre-existing, Phase 7-C):
  expectations updated `1→2`, `2→3`, `3→5`, `4→7` — containers
  are ARC objects now, so they count in
  `sp_rt_mem_live_objects()`. The reads (`n 42` everywhere) are
  unchanged; this is a count change, not a behaviour change.

All 13 container tests pass; the whole `TryCatchExecutionTest`
file (55 tests) passes.

## 4. Files changed

| File | Change |
|---|---|
| `docs/v2/PHASE-37-SLICE-B.md` | This doc (design §0–§0.9, proofs §1, implementation §2, tests §3, files §4, ctest §5, still-leaks §6) |
| `runtime/src/Runtime.cpp` | ARC allocs, 6 destructors, 2 flag fields (array/list/map/set/queue/pq), 6 setters, release-on-drop in 11 mutating functions, ARC-aware `sp_rt_free` |
| `runtime/include/sp/runtime/Runtime.h` | 6 mark declarations + `sp_rt_channel_mark_ref_items` |
| `runtime/include/sp/runtime/Channel.h` | `m_ref_items`, `mark_ref_items()`, `ref_items()`, `drain_items()` |
| `runtime/src/threading/Channel.cpp` | `mark_ref_items` / `ref_items` / `drain_items` implementations |
| `runtime/src/threading/Concurrency.cpp` | `sp_rt_channel_mark_ref_items`, destroy drain |
| `compiler/include/sp/ir/IRBuilder.h` | `isOwnedValueType`, `recordTempLiteralResult`, `emitContainerMarkRefElements` declarations |
| `compiler/src/ir/IRBuilder.cpp` | Owned predicate + 8 widenings, `isFreshPlusOneExpr` (+literals, −reduce, +split/array-map/filter), literal temp hook, 7 setter call sites, peek retain, channel marks, Set `_str`/`_i64` discriminator fix |
| `compiler/src/ir/IR.cpp` | 7 new runtime function declarations |
| `tests/codegen/TryCatchExecutionTest.cpp` | 12 new container execution tests + 1 updated expectation (Phase 7-C live counts) |
| `tests/runtime/AsyncTest.cpp` | CAbi tests free their arrays with `sp_rt_free` (13 sites) + comment on the ARC free contract; C-string frees unchanged |

## 5. ctest

First full run (before the `sp_rt_free` fix): **805/818** —
12 aborts, all one root cause: `free(): invalid pointer` on
ARC array payloads (2 `RuntimeTest.Array*` via `sp_rt_free`,
10 `AsyncTest.CAbi*` via `std::free` on the input handle array
and the caller-owned results array). The fix is in §2
(ARC-aware `sp_rt_free` + `sp_rt_free` in the CAbi tests).

Second full run (after the fix): **817/818 (99%)** — total time
98.36 s. The single failure is the known pre-existing
environmental one:

- `NetTest.UdpSocketSendRecv` — `sent = -1`, the sandbox blocks
  UDP (EPERM), same as Phase 7 (804/806).

The Json-benchmark flake from Phase 7 did not occur this run
(`JsonTest.BenchmarkOneGigabytePerSecThroughput` passed).
Phase 7 baseline was 804/806; this slice adds 12 tests
(818 total) and loses none.

## 6. What deliberately still leaks (by design)

- **Derived containers** (`xs.map(f)`, `xs.filter(p)`,
  array `map`/`filter`, string-split arrays): runtime-internal,
  never marked — see §0.6.
- **`pop` / `dequeue` transfers:** one share leaks per popped
  reference element (the caller's VarDecl retain covers the
  caller's share; the container's share is the transfer). Closing
  it needs the +1 convention extended to builtin pop/dequeue —
  follow-up, not this slice.
- **Unresolved-call Map/Array temporaries:** not recorded — leak
  one share, as for class temporaries from unresolved calls.
- **Container `new` as a pure temporary** (`foo(new
  List<Foo>())`): the `new` branches return early without
  `recordTempNewResult`, so the fresh container leaks. (A
  literal in the same position IS released — the hook exists
  only for literals.)
- **Async-function temporaries and short-circuit/ternary
  temporaries:** unchanged from Phase 7-C §6.
- **Containers in fields, globals, statics, and `main` locals
  at process exit:** fields never drop (no `<Class>_destructor`
  yet — slice A); globals/statics are never in a scope.
- **Variables declared without an initializer, then assigned:**
  never registered for scope-drop (same hole as for classes).
- **Self-referential cycles:** the self-share keeps the count
  ≥ 1 — leaks, no crash.
- **Nullable containers / nullable elements** (§0.8): not owned,
  never flagged, leak by design.
- **A fold that returns a fresh object** (`xs.reduce((a, b) =>
  new Foo(), …)`): the result is treated as borrowed, so the
  extra share leaks one (safe direction).
- **Channel `recv`'d items:** the channel's share of a received
  item is never released (recv returns i64 in sema, never
  recorded) — the receiver holds a borrowed pointer kept alive
  by the leaked share. Sound, leaks one share per received
  reference item.
- **Map with a class key** (`Map<Foo, int>`): the Map key
  lowerings still dispatch on IR pointer-ness, so a class key
  would be stored as a bogus string key (same bug as Set had —
  see §0.9). Out of scope; keys are strings/ints by design.
- **C-ABI contract change (consequence, not a leak):** any C/C++
  caller that frees a runtime array (or any container) must use
  `sp_rt_free`, not `std::free` — the payload pointer is 16
  bytes past the malloc base and `std::free` on it aborts. This
  is now the documented contract in the AsyncTest CAbi comment
  and in `sp_rt_free` itself.
