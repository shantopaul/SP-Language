# Phase 38 (ADR-002) — Slice D: Payload width — boxed struct / multi-payload / nested tagged-union payloads

Phase: 38 (pro-4.0.0) · Slice D · ADR-002 (Result/Option)
Base: Slice C commit `64a53ee` on `feature/pro-4.0.0`
Status: DESIGN (implementation follows this document)

## 0. Problem

Slices A–C made a tagged union a `{i32 tag, i64 payload}` struct whose
single slot holds one 64-bit payload. That representation cannot hold:

1. **Struct payloads** — `Result<Point, string>`: a `Point` is 16 bytes
   (two i64 fields). The slot holds 8.
2. **Multi-payload variants** — `enum Shape { Rect(int, int) }`: two
   payloads, one slot. Construction already rejects them at sema with
   E1001 ("not supported yet (Phase 38 slice D)"); match binding
   rejects them the same way (slice B).
3. **Nested tagged unions** — `Result<Result<int, string>, string>`:
   the inner value is itself 16 bytes (its own `{i32, i64}`).

The pre-D probe `p6_struct_payload.sp` was inconclusive (wrong struct
syntax in the probe); the representation limit itself is proven by
`emitPopulateTaggedUnion` (`IRBuilder.cpp:43`): it stores
`emitCastIfNeeded(payloadVal, IRType::getInt64())` into field 1.

## 0.1 Design: heap-box the wide payload

A **wide** payload is stored in a heap box; the slot holds the box
pointer (one i64) instead of the payload bits. The box is an ARC
object allocated with `sp_rt_arc_alloc(size, dtor)` (`Runtime.h:75-77`,
`Runtime.cpp:581`), so the whole Slice A ownership rule carries over
unchanged: the TU owns exactly one share of the box; construction
adopts/retains, copy retains, drop releases — and the box's destructor
releases whatever the box itself owns. The release paths
(`emitScopeDrops`, `emitRetainTaggedUnionPayload`,
`emitTaggedUnionConstructionRetain`, temp pops) operate on slot bits
only, so they need no change: releasing slot bits that happen to be a
box pointer frees the box and runs its destructor.

**Which payloads are wide** (per variant, from the variant's payload
types read off the subject's specialized `EnumType`):

| payload kind | wide? | box contents |
|---|---|---|
| scalar / pointer / class / array / map / string | no | — (slot holds the bits, as today) |
| struct type (`isStruct`) | yes | the struct bytes |
| tagged-union type (`isEnum` + `isTaggedUnion`) | yes | the inner `{i32, i64}` struct bytes (16) |
| variant with > 1 payload types | yes | one i64 per part, laid out part 0..N-1 at offsets 0, 8, 16, … |

A variant is wide when **any** of its payload types is wide, or when it
has more than one payload type. A multi-payload variant is supported
only when **every** part fits in one i64 (scalar / pointer / class /
array / map / string / enum-tag); a part that is itself wide (struct or
nested TU) is a loud E1000 at the construction site — boxing a part
would need a box-of-boxes layout that this slice does not build.

**Box layout and size**

- Struct payload: `asyncStructSizeBytes(mapType(payloadType))` bytes
  (alignment-aware, rounds up — it already over-allocates safely;
  `IRBuilder.cpp:11197`). The struct bytes are stored whole.
- Nested TU payload: 16 bytes (the inner struct, `{i32, i64}`).
- Multi-payload: `maxParts × 8` bytes, where `maxParts` is the
  maximum part count over **all** variants of the enum. The box is
  sized uniformly so that **one** destructor (releasing `maxParts`
  slots) is correct for every variant; parts the constructed variant
  does not have are **zeroed** at construction (a zeroed slot is a
  safe no-op for `sp_rt_release`).

**Box destructors** (generated into the module like
`emitClassDestructors`, `IRBuilder.cpp:184`; null is legal —
`sp_rt_release` skips a null destructor, `Runtime.cpp:634`):

- Struct payload: **null** destructor. The struct's fields are not
  tracked — the same documented gap as Phase 8-A struct fields
  holding refs (a `Point` holding a class would leak that one share;
  a leak, not a UAF).
- Nested TU payload: one shared destructor `__sp_tu_box_dtor_nested` —
  loads the i64 at offset 8 and calls `sp_rt_release` on it. Used only
  when some variant of the inner TU has an owned payload; otherwise
  null (all releases would be no-ops). Recursive: if the inner
  payload is itself a box pointer (nested-nested), the release drops
  that box's count and runs *its* destructor.
- Multi-payload: one shared destructor per part count
  `__sp_tu_box_dtor_multi_<N>` — releases each of the N i64 slots
  unconditionally. Used only when some part (in any variant) is owned
  (`isOwnedValueType`); otherwise null. Unused (zeroed) slots and
  scalar slots are no-ops, so no tag dispatch is needed.

**Why no tag is stored in the box:** the destructor never needs the
variant. A nested-TU box holds exactly one payload (whichever variant
was constructed); a multi-payload box holds `maxParts` slots of which
the unused ones are zero. Both release unconditionally and safely.

## 0.2 Construction (boxing point)

New helper `emitTaggedUnionPayloadSlot(tuType, variantName,
payloadVals, payloadTypes)` — called by the three construction sites
(member `E.Ok(..)` / `E.Err(..)` / `E.Some(..)`, bare `Ok(..)` /
`Err(..)` / `Some(..)`, user-enum variant construction) **after** the
per-argument retain (see §0.3) and **before** `emitCreateTaggedUnion`.
It returns the slot value: the single payload bits for a narrow
payload, the box pointer (cast to i64) for a wide one.

- Struct payload: allocate `sp_rt_arc_alloc(size, null)`, store the
  struct bytes into the box (load the struct first when the lowered
  argument is a pointer to it), return the box pointer.
- Nested TU payload: allocate `sp_rt_arc_alloc(16, dtor)`, store the
  inner struct bytes, return the box pointer.
- Multi-payload: allocate `sp_rt_arc_alloc(maxParts*8, dtor)`, store
  each part (cast to i64) at offset `i*8`, store zero into the
  remaining slots, return the box pointer.
- The payload types come from the call's sema type for Result/Option
  (`m_sema.getNodeType(&node)` → specialized `Result<T,E>` /
  `Option<T>` → `getVariantPayload(variant)`), and from
  `findUserType(targetName)` for user enums.

`emitCreateTaggedUnion` itself is unchanged — it already stores the
given value as i64 into field 1, so a box pointer flows through as
`ptrtoint`.

## 0.3 Ownership at construction (extends Slice A)

`emitTaggedUnionConstructionRetain` gains a tagged-union branch: when
the payload type is itself a TU with an owned payload, adopt the
argument's temp record (`adoptTempValue`) and, when the argument is
borrowed (`!isFreshPlusOneExpr(arg)`), retain its inner payload via
`emitRetainTaggedUnionPayload`. Without this, a TU temp used as a
payload would be released by the statement pop while the box still
points at its payload (UAF). For a multi-payload construction the
helper runs once per argument with that argument's type (a class part
is retained; a scalar part is a no-op). Struct arguments are not
owned (`isOwnedValueType` excludes structs), so they need nothing.

## 0.4 Unboxing points

A new helper `emitLoadTaggedUnionPayload(structPtr, tuBaseName,
payloadType)` loads a payload value from a TU struct pointer: narrow
→ load field 1 as i64 and cast to the payload type (today's behaviour);
wide struct / nested TU → load field 1 as a box pointer and load the
payload from the box (struct bytes / inner struct); multi-payload →
no single value exists — the caller reports it.

- **`unwrap` / `unwrapErr` / `unwrapOr`** (`IRBuilder.cpp:6765+`):
  the receiver's enum type + the method's variant (tag 0 for unwrap,
  tag 1 for unwrapErr) give the payload type from the call's sema
  type; a multi-payload variant there is a loud E1000. `unwrapOr`'s
  default value is unaffected (it is only used on the None/Err path).
- **`?` ok path** (`visit(TryExpr)`): `?` is sema-restricted to
  Result/Option, whose variants are single-payload, so only the
  struct / nested-TU unbox applies.
- **Match binding** (slice B, `IRBuilder.cpp:3013+`): single boxed
  payload → load from the box; multi-payload → each binding loads
  its part from the box at offset `i*8`, cast to that part's IR type
  (`mapType(payloadTypes[i])`). Bindings stay BORROWED (the subject
  outlives the arm body). The slice-B sema rule that rejects
  multi-payload patterns is lifted; the "multiple payload bindings"
  E1001 in the IR is removed. Nested patterns stay out of scope.
- **`.payload` member**: if its lowering is the same field-1 load,
  route it through the same helper; if it is not, leave it as-is and
  note it (it returns `any`).

No change is needed at: `isOk`/`isErr`/`isSome`/`isNone` (tag only),
scope drops, copy retains, the async box (slice C — the async box
holds the TU *struct*, whose slot bits are the payload box pointer;
the payload retain in the async ReturnStmt already retains the slot
bits), or the `?` err path (it copies the struct, retaining the box).

## 0.5 What stays loud (E1000)

- A multi-payload variant whose part is itself wide (struct or nested
  TU) — construction reports it.
- `unwrap` / `unwrapErr` on a multi-payload variant.
- (Pre-existing, unchanged) nested match patterns, literal-argument
  patterns, exhaustiveness checking.

## 0.6 Out of scope (documented, not fixed)

- Struct payloads whose fields hold owned values: the box destructor
  is null, so those shares leak when the TU dies (same class as the
  Phase 8-A struct-field gap; leak, not UAF).
- Class fields holding a Result/Option (Slice A gap) — unchanged.
- Result/Option as an async fn argument; `Task<Task<...>>` (Slice C).
- The `?` operand-vs-enclosing-return type mismatch (pre-existing
  sema hole).
- Retain/release elision / stack allocation (needs the escape-analysis
  soundness re-audit first).

## 0.7 Verification

New tests in `ExecutionTest.cpp` — 7 tests, all passing. Full ctest
**856/857** (only the standing UDP EPERM failure; PG skipped without
a server). Log `~/workspace/ctest-phase9-slice-d.log`.

1. `ResultStructPayloadRoundTrip` — `Result<Point, string>` with
   `Point p; p.x = 3; p.y = 4;` constructed, unwrapped, fields read
   back (`3 / 4`).
2. `ResultStructPayloadTryOperator` — `?` on a struct payload, both
   the Ok path (`x 9 y 10`) and the Err path (`err true no point`).
3. `OptionStructPayloadUnwrapOr` — `unwrapOr` on `Some` and on
   `None` (the guarded unbox; the None path must not dereference
   the null slot).
4. `MatchMultiPayloadBindsParts` — `Shape.Rect(3, 4)` constructed and
   match-bound to `w, h` → `rect 3 4`; `Circle(5)` → `circle 5`.
5. `ResultNestedPayloadRoundTrip` — `Result<Result<int, string>,
   string>` constructed, unwrapped twice → `42`.
6. `ResultNestedClassPayloadLiveCounts` — `Result<Result<Foo, string>,
   string>` where `Foo` is a class with a live counter: live count
   2 after construction (Foo + outer box), 0 after the outer TU
   drops (proves the nested box destructor runs).
7. `MatchMultiPayloadClassPartLiveCounts` — `Pair.Both(Foo, int)`:
   `both 5 3`, live 2 in scope, 0 after scope (proves the multi
   box destructor runs); `Pair.Single(9)` → `single 9`, 0 after.

Live proofs in `~/workspace/p9d-proofs/` (same harness as A–C):
`p6_struct_payload.sp` (rewritten with correct struct syntax),
`p7_multi_payload.sp`, `p8_nested_result.sp`, `p8b_nested_live.sp`,
`p8c_try_struct.sp`, `p8d_multi_class.sp`, plus re-runs of the
Slice A–C proofs byte-identical — except `p1b_multi_reject.sp`,
which used to expect the multi-payload E1001 and now prints `rect`
(the rejection is lifted; p7 supersedes it as the multi-payload
proof).

## 0.8 Findings during implementation (two real bugs)

1. **Box leak — `taggedUnionHasOwnedPayload` too narrow.** The
   predicate only counted class/array/map payloads, so a TU whose
   payload is itself a TU (or a struct, or a multi-payload variant)
   was never registered for a scope drop and its box leaked
   (p8b printed `after scope 1`). Fixed by widening the predicate:
   a variant's payload needs a release when it is owned, when it is
   wide (struct / nested TU — the slot holds a box pointer), or
   when the variant is multi-payload (the slot holds a box pointer).
   The retain/release/temp/async paths all operate on slot bits, so
   they free a box exactly like a class pointer once the predicate
   admits it.
2. **Struct-payload UAF — the yielded struct aliased the box.**
   `?`/`unwrap` returned the box pointer itself, but the statement
   pop frees the box before the *next* statement reads the struct
   bytes (p8c printed `x 0 y 0` for `var p = ...?` then
   `Result.Ok(p)`). Fixed by snapshot-copying a struct payload into
   a fresh entry-block alloca at every yield point
   (`emitUnboxTaggedUnionPayload`) — structs are values, so the
   yielded value must not alias the box. A struct payload holding
   owned refs still aliases them (the Phase 8-A struct-field gap,
   unchanged).
