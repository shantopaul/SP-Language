# Phase 38 (pro-4.0.0) — Slice C: async + Result

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `3100474` (Phase 38 slice B)
- **Version:** 3.1.0, unchanged
- **Scope:** `async fn` may return `Result<T, E>` / `Option<T>` (and
  user tagged-union enums); `await` on `Task<Result<...>>` yields
  the Result; `?` inside an async fn completes the task with the
  Err/None instead of hanging. No Result as an async ARGUMENT, no
  `Task<Task<...>>`, no `Task<Map<...>>` (still loud E1000).
- **Status:** DONE — committed on `feature/pro-4.0.0` (see §0.7).
  Verified: proofs + 4 new tests + full ctest 849/850 (only UDP EPERM).

## 0.7 Verification (2026-11-10, build `~/workspace/build-phase9`, Release)

- Proofs (`~/workspace/p9-probes/`, new `spc`):
  `p2_async_try` → `ok 43`; `p2b_async_try_err` → `err boom`
  (the `?` err path completes the task — no hang); slices A/B
  proofs byte-identical (`p1` circle 5, `p4` ok 5 / err division
  by zero, `p3` 0/1/7/1, `p5` 11/-1).
- New tests (4): `AsyncResultPayload`, `AsyncResultTryOperator`,
  `AsyncResultClassPayloadLiveCounts` (live in scope 1, after
  scope 0 — the awaited payload is owned and dropped exactly
  once), `AsyncOptionPayload` — all pass.
- Full ctest: **849/850** (846 + 4 new; only
  `NetTest.UdpSocketSendRecv` fails — EPERM, environmental; PG
  skipped, no server).
- Two build fixes during the slice (both mine):
  `getCallTarget` takes a `const CallExpr*`, not a reference;
  the TryExpr err-box must be a `shared_ptr<CallInst>`, not an
  `IRValuePtr`, for `addInstruction`.
- Known gaps left: Result/Option as an async fn ARGUMENT (spawn
  arg types unchanged); `Task<Task<...>>`, `Task<Map<...>>`,
  plain-enum task payloads (still loud E1000); the `?` operand
  vs enclosing-return type mismatch (pre-existing sema hole,
  sync fns too).

## 0. Design — written before the implementation

### 0.1 What the probes showed (Phase 38 investigation)

- `async fn work() -> Result<int, string>` fails at the
  declaration with E1000: "the return type of this 'async fn'
  cannot be carried by a Task (only void, int, bool, string,
  double, array and struct payloads are supported)".
- Root cause: `taskInnerType` rejects ANY inner type containing
  `'<'` ("Nested Task<...> / parameterized payloads are not
  carried"), so `Task<Result<int, string>>` is unrepresentable.
- The struct-kind machinery already handles tagged unions
  end-to-end for the BITS: `asyncStructTypeFor` has an enum branch
  ("Tagged-union enums (Result/Option) lower to a struct value"),
  the return boxes the struct, the finish uses
  `complete_ptr_owned`, the awaiter loads it by value from the
  box. What is missing is (a) the KIND decision, (b) payload
  OWNERSHIP across the task boundary, and (c) the `?` err path
  inside async fns (today a plain `ReturnInst` in the void resume
  fn — the task would never complete).

### 0.2 New payload kind: "taggedunion"

`taskPayloadKind` / `asyncReturnKind` gain a kind `"taggedunion"`
when the (inner) return type is a tagged-union EnumType. The
kind flows through the existing struct-shaped paths:

- **Declaration:** `taskInnerType` becomes a const member (it
  needs `findUserType`, which is const). The `'<'` rejection is
  kept, except when the inner type's BASE name resolves to a
  tagged-union enum (`Result`/`Option`/user payload enum) — then
  the inner string is returned. `Task<Task<...>>`, `Task<Map<...>>`
  and plain-enum payloads still reject.
- **Return (async fn body):** like kind `"struct"` (box the
  `{i32, i64}` bits with `sp_rt_alloc`), PLUS retain the payload
  when owned and borrowed (`!isFreshPlusOneExpr(value)`), before
  the scope drops — the box then holds one share. A fresh +1
  return is adopted into the box (no second retain).
- **Finish block:** like `"struct"` (`complete_ptr_owned`) — and
  NO payload release (unlike `"string"`): the share transfers to
  the task's result, it is not copied out and freed.
- **Await:** like `"struct"` in `asyncSuspendAndLoad`,
  `emitAwaitTask`, `asyncAwaitedIRType`, and both AwaitExpr sites
  (`asyncStructTypeForTask` already returns the TU struct type).

### 0.3 Await-result ownership (the subtle part)

The box holds exactly one payload share. Who owns it after the
await?

- **Form 2 — `await worker()` (direct call to an async fn):** the
  temporary task is destroyed after the await, so the box's share
  TRANSFERS to the await result: the result is +1.
  `isFreshPlusOneExpr` gains an AwaitExpr branch (true only for a
  direct async call whose await type is a TU with an owned
  payload), and `visit(AwaitExpr)` records the result's payload
  as a temp (refactored `recordTempTaggedUnion` → type-based
  helper, also used by `recordTempCallResult`).
- **Form 3 — `await t` (an existing Task value):** the task is NOT
  destroyed and can be awaited twice — the same share cannot be
  owned twice, so the result is BORROWED (adopters retain by the
  copy rule; no temp record). Awaiting the same TU task twice and
  assigning both results is safe: each assignment retains its
  own share.
- The box free on task destroy never touches the payload (plain
  `sp_rt_free` of 16 bytes), so the transferred share survives.

### 0.4 `?` inside an async fn

`visit(TryExpr)`'s err block, when `inAsyncSM()`: instead of a
plain `ReturnInst` (which returns void from the resume fn and
leaves the task hanging), box the operand like an async return
(retain the payload when borrowed), store the box in the frame's
retVal slot, flush pending temps except the operand's payload
(slice A), drop the frame locals, and branch to the finish block.
The task completes with the Err/None Result. (Sema already
requires the enclosing fn's declared return to be Result/Option —
`m_currentFunctionReturnType` is the DECLARED return, not the
Task — so no sema change is needed here.)

### 0.5 Out of scope

- Result/Option as an async fn ARGUMENT (spawn arg types unchanged).
- `Task<Task<...>>`, `Task<Map<...>>`, plain-enum task payloads
  (still loud E1000).
- `?` operand type vs enclosing return type mismatch (pre-existing
  sema hole, sync fns too — the err path would return a
  differently-typed Result; the IR struct types coincide so it
  does not crash, but it is semantically wrong).

### 0.6 Expected outcomes

- Proof (`~/workspace/p9-probes/p2_async_try.sp`, rewritten as a
  driver): `async fn work() -> Result<int, string>` with `?`
  inside, awaited by an async runner, prints `ok 43` (and an Err
  variant prints `err ...`).
- New tests: `AsyncResultPayload`, `AsyncResultTryOperator`,
  `AsyncResultClassPayloadLiveCounts` (live count 0 after the
  awaited Result is dropped), `AsyncOptionPayload`.
- Full ctest 846 + new tests (only UDP EPERM; PG skipped).
- All Phase 37 + slice A/B live proofs byte-identical.
