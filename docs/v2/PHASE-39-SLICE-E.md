# Phase 39 slice E — docs (Syntax Guide §42) + doc gate (ADR-004)

## 0. What changed

`docs/SP-Syntax-Guide.md` §42 was stale in three ways, all fixed:

1. **Wrong pointer syntax** — it said "Pointers are `ptr`, not
   `pointer`" and declared `fn strlen(string s) -> int;`. Current
   syntax is `ptr<T>` (`ptr<char>`, `ptr<uint8>`, `ptr<void>`), and
   `int` is 32-bit (C `long`/`size_t` need `int64`/`uint64`).
2. **Wrong variadic claim** — it said C variadics "cannot be
   declared". Since slice C they CAN be declared
   (`fn printf(fmt: ptr<char>,...) -> int32;`); what is still true
   is that calling one with extra arguments is a sema arity error.
3. **Missing everything Phase 39 built** — no struct-by-value, no
   `--link`/`sp.toml [native]`, no `spc bindgen`, no ownership
   paragraph.

New §42 (two runnable ` ```sp ` blocks, both verified before
writing — probes `~/workspace/p10-probes/sg42a.sp`,
`sg42b.sp`):

- **Basics**: `strlen` via `ptr<char>` (string literal converts
  implicitly), `malloc`/`free` via `ptr<void>`/`ptr<uint8>` inside
  `unsafe`, `spc` links libc by default.
- **Structs by value**: libc's `div` returns `div_t` by value —
  `#[repr(C)] struct DivT { quot: int32; rem: int32; }` +
  `fn div(numer: int32, denom: int32) -> DivT;` prints
  `quot 3 rem 2`. Small structs (≤16 bytes) travel in registers
  under the System V ABI; larger ones by address (slice A).
- **Linking**: `--link`/`-l` + `-L` (slice B), `sp.toml [native]`
  (`libs`, `lib-paths`; project mode only, bare `spc run` does not
  read it).
- **bindgen**: `spc bindgen /usr/include/zlib.h -I /usr/include
  --module zlib -o zlib.sp` + `spc run app.sp --link z` (slice C);
  the generated file is ordinary SP — `const` values bind as
  program constants, function pointers become `ptr<void>`.
- **Ownership**: ARC does not cross into C. A pointer SP passes
  stays SP-owned (C must not free it; SP must keep it alive). A
  fresh C allocation is the caller's to free with C `free`. Never
  free a string literal or a pointer into an SP-managed value.

The `import zlib` usage is prose with inline code, NOT a fenced
` ```sp ` block — the gate runs every fenced SP block standalone,
and the import cannot resolve in the examples directory.

## 1. Regression found by the gate — synthesized main for const-only programs

The first gate run came back **120/130** with two NEW failures
(ex053 §44, ex126 spec §3.1) that passed under the Phase 9 binary.
Both files consist of a single top-level declaration and nothing
else: `int age = 21;` (ex053) and
`const BUFFER_SIZE: int = comptime (1024 * 64);` (ex126). Slice D
excluded `VarDeclStmt` from `topLevelStmts`, so with no explicit
`main` and no remaining top-level statements, `IRBuilder` emitted
**no `main` at all** → the link failed (undefined `main`) → `spc
run` fell back to the legacy clang path with the misleading
"Unsupported backend compiler" error.

Fix (`compiler/src/ir/IRBuilder.cpp`, `visit(Program)`): synthesize
`main` when `!topLevelStmts.empty() || !m_staticInitFunctions.empty()`.
The synthesized main runs the init functions and exits. This also
fixes the same latent hole for a class with static initializers and
no `main`. Verified: ex053 and ex126 both exit 0 afterwards (ex126
prints only the expected `comptime`-experimental warning).

## 2. Verification (2026-11-10, slice-D/E binary)

- Doc gate **123/130 RUN-OK** (log
  `~/workspace/doc-gate-phase10-slice-e.log`; extract log
  `~/workspace/extract-phase10-slice-e.log`). Phase 9 baseline was
  121/129. The 7 non-OK map exactly to known baseline classes (by
  heading, since numbering shifted +1 after §42): ex045/ex046
  SKIP-CHECK-FAIL (§38), ex007/ex067 RUN-TIMEOUT (server examples),
  ex044/ex128/ex129 RUN-FAIL with the known misleading backend
  message (§38, spec §5.1/§5.2). The two new §42 blocks (ex050,
  ex051) are RUN-OK. ex108 (HTTP server) varies with orphan-server
  state on port 8080 — orphans were killed after the run (exact
  PIDs, never `pkill -f`).
- Full ctest **884/885** (log `~/workspace/ctest-phase10-slice-d.log`)
  — only the standing `NetTest.UdpSocketSendRecv` EPERM failure; PG
  skipped. Test total 880 → 885 (slice D's 5 new tests).
- Slice D proofs re-run with the final binary, byte-identical:
  same-file const `c 42`, import `version 3`, zlib consts
  `zok 0 / zver 1.3 / zvernum 4864 / zmem 9`, `use_mini`
  `add 42 / scale 14 / sum 7 / filled 9 10`, `p5` `made 5 6 /
  added 6 8`, `p6` `big 21`.
