# Phase 39 (ADR-004) — Slice A: struct-by-value FFI calls

Phase: 39 (pro-4.0.0) · Slice A · ADR-004 (FFI)
Base: Phase 39 doc (investigation) on `feature/pro-4.0.0`
Status: DESIGN (implementation follows this document)

## 0. Bug (verified 2026-11-10, `~/workspace/p10-probes/minilib/`)

`mini_scale(p, 2.0)` with `p: MiniPoint {x: 3, y: 4}` must return
`(3+4)*2.0 = 14.0`. It returns **2101050012** — the struct's storage
address read back as its value. Struct-by-pointer works
(`mini_sum_xy(&p)` → 7, `mini_fill` → `9 10`); struct-by-value does
not.

Root cause: `mapType` maps `Struct` → pointer (`IRBuilder.cpp:1999`),
and the extern declaration sites (`IRBuilder.cpp` ~2208, ~2230,
~2373) declare every body-less function with `mapType` param/return
types. The C side expects the struct in registers / on the stack by
value (System V ABI); SP passes the storage pointer instead.

The call site is NOT the bug: the general call path
(`IRBuilder.cpp` ~10230) already loads a struct value when the
declared param `isStruct()` — it just never triggers, because the
declaration says pointer.

## 0.1 Design (CORRECTED 2026-11-10 — the first design was wrong)

Declaring the extern fn with the struct VALUE type is NOT enough.
Verified by experiment: a minimal `.ll` with
`declare double @mini_scale(%struct.S, double)` + a struct-typed
call compiles to **two separate 32-bit loads into EDI/ESI** — LLVM's
backend flattens a struct-typed argument into its fields, while the
C callee (compiled by clang's frontend, which coerces the struct to
its ABI representation, `i64` in RDI) reads one 64-bit register.
Result: the callee saw `{x=3, y=0}` → `(3+0)*2.0 = 6.0`, not 14.0.
The C caller for the same call emits a single `mov %rdi` and gets
14.0. A 24-byte struct, by contrast, WORKS with the struct-typed
declaration (`mini_big_sum` → 21): MEMORY-class structs are
stack-passed correctly by the backend.

So the fix is **System V ABI coercion at extern call sites** (what
clang's frontend does): a struct of ≤16 bytes is passed as its
eightbyte register values — one LLVM value per eightbyte (INTEGER
eightbyte → `i64`, SSE eightbyte with a single double → `double`,
single float → `float`). The extern declaration uses the SAME
coerced types, so declare and call agree. Structs > 16 bytes keep
the struct-typed declaration/call (verified working).

1. **AST flag** — `FunctionDecl::m_isExternC` + `isExternC()` /
   `setExternC(bool)` (`Declaration.h`, same style as `m_isUnsafe`).
   The parser sets it on every `FunctionDecl` parsed inside an
   `extern "C" { ... }` block (`Parser.cpp` ~1266). Default false.
   A top-level body-less `fn` NOT in an extern block stays a plain
   external declaration (SP-internal pointer convention unchanged).
2. **Classifier** — `classifyExternStruct(sema struct type)`:
   flattens the layout's fields (recursively) into (offset, size,
   INTEGER/SSE) leaves using C layout rules, then classifies each
   eightbyte: any INTEGER leaf → `i64`; else a single double →
   `double`; a single float → `float`; two floats in one eightbyte
   (needs a vector type SP IR does not have) → NOT coercible.
   Returns the register types per eightbyte, or "not coercible".
   Structs > 16 bytes are not coerced (stack passing works).
3. **Declarations** — at the three no-body sites, gated on
   `fn->isExternC()`: each struct param expands into its coerced
   register types (1 or 2 LLVM params); the return type coerces the
   same way (one eightbyte → the register type; two → a struct of
   the two register types). Non-coercible struct params keep the
   struct type (the call site then reports a loud E1000).
4. **Call site** (general call path) — when the callee is extern-C:
   for each struct-typed SP arg, load the eightbyte values from the
   arg's storage (pointer arg: load directly; value arg: spill to a
   temp alloca first) and pass those instead of the struct value.
   Non-coercible struct arg → loud E1000 (never silently
   miscompile). Non-extern calls keep the existing behavior.
5. **Returns** — same coercion on the declare side; after the call,
   bitcast the coerced result back to the struct value type so the
   caller sees a normal struct value (`MiniPoint q = mini_make(5,
   6); q.x` must work).
6. **Scope** — direct free-function calls only. Fn-pointer calls to
   C functions with struct params stay a documented gap (the fn
   pointer's declared type still uses the pointer convention).

## 0.2 Verification (DONE 2026-11-10, build `~/workspace/build-phase10`)

- Live proofs (minilib, manual link until slice B lands) — all pass
  with the new binary:
  - `use_mini.sp` → `add 42 / scale 14 / sum 7 / filled 9 10`
    (was `scale 2101050012` pre-fix, `scale 6` with the first
    struct-typed design — the LLVM flattening bug, see §0.1).
  - `p5_struct_ret.sp` → `made 5 6 / added 6 8` (struct returns,
    incl. a call-result struct passed straight to another C fn —
    the spill path in `emitExternStructArg`).
  - `p6_big.sp` → `big 21` (24-byte struct, stack passing).
- IR tests (`tests/ir/ExternStructIRTest.cpp`, 5 tests, all pass):
  param coercion `(i64, i64, double)`, call passes register
  values, return coercion `{ i64, i64 }` + store/load conversion,
  24-byte struct keeps the struct type, plain SP fn still passes
  structs by pointer.
- Full ctest: **863/864** — only the standing UDP EPERM failure
  (`NetTest.UdpSocketSendRecv`); PG skipped (no server). Total
  grew 859 → 864 (+5 IR tests). Log
  `~/workspace/ctest-phase10-slice-a.log`.
- Phase 38 proofs re-run: `p9_try_list_dir.sp` and
  `p9_try_read_line.sp` byte-identical with the new binary (spot
  check; full Phase 38/39 proof sweep is part of the phase
  completion, slice E).

## 0.3 Findings during the slice

- **SP `int` is i32** (`IRBuilder::mapType`, line 2050 — also
  `VTableBuilder::mapType`). Earlier session notes saying "SP int
  = i64" are wrong; `int64` is the 64-bit type. The IR tests use
  `int64` fields for the 16-byte case. (The minilib's `MiniPoint`
  with `int32` fields matches C `int` — the live proof is
  unaffected.)
- **`ClassLayout::objectSize` overstates struct sizes**: it counts
  8 bytes per field (`VTableBuilder.cpp:769`), so a 2×i32 struct
  reports 16 bytes while the LLVM type is 8. It is only used for
  `new` heap-allocation sizes (over-allocation, harmless), but the
  classifier computes true C layout sizes from the element types
  instead of trusting it. Not fixed here (out of scope).
- **LLVM bitcast cannot convert aggregate ↔ scalar**: the first
  return-conversion attempt (`bitcast i64 → %struct.MiniPoint`)
  failed LLVM verification. Returns convert via memory (store the
  coerced result into a temp struct alloca, load the struct value).
- **Two floats in one eightbyte** is the only small-struct case
  with no register representation in SP IR (needs a vector type):
  it reports a loud E1000 at the call site instead of silently
  miscompiling. Same for any other non-coercible ≤16-byte struct.
- **Fn-pointer calls** to C functions with struct params are NOT
  fixed: a fn pointer's declared type still uses the pointer
  convention. Only direct free-function calls are coerced.
  Documented gap for a later slice.
