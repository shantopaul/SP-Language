# Phase 39 (pro-4.0.0) — ADR-004 FFI & `spc bindgen`

- **Branch:** `feature/pro-4.0.0` (branch only; remote `main` still `d10e1c2`)
- **Base:** `5b3b814` (Phase 38, ADR-002 Result/Option)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, nothing pushed)
- **Status:** IN PROGRESS — investigation done, design below; slices to follow.
- **ADR:** `docs/v2/DECISIONS/ADR-004-ffi.md` (Accepted 2026-11-10):
  direct C-ABI + `spc bindgen`; `extern "C"`, `#[repr(C)]`,
  `ptr<T>` in `unsafe` blocks; bindgen parses C headers via libclang.

## 0. Investigation — what exists, what is broken (verified 2026-11-10)

Probes in `~/workspace/p10-probes/` (minilib: a real C library
`libmini.so` built with clang; `~/workspace/build-phase9/bin/spc`).

### 0.1 What already works (no work needed)

- **`extern "C"` calls** — int/double/`ptr<T>` params, `void`
  returns, string literal → `ptr<char>`, `ptr<char>` prints as a
  C string. `mini_add(20, 22)` → **42**; `mini_sum_xy(&p)` → **7**;
  `mini_fill(&p, 9, 10)` → `9 10`.
- **`#[repr(C)]` layout matches C in memory** — `MiniMixed {int a;
  double b; int c}` read through a pointer by C returns `b = 2.5`;
  C reports `b` at offset 8, SP's layout agrees.
- **`ptr<T>` + `&` + `*` inside `unsafe`** — `deref 42`, `after 7`;
  sema rejects address-of and deref outside `unsafe` (E1001).
- **`spc bindgen` on small hand-written headers** — structs/enums/
  macros/functions generate clean SP; `spc check` passes on the
  output. CLI `spc bindgen <hdr.h> [-o <out.sp>] [--module <name>]`
  works. 5 existing `BindgenTest` cases pass.
- **Syntax Guide §42** runs as documented (`strlen("Hello")` → 5,
  `malloc`/`free` via bare `ptr`); libc is linked by default.
- **Linker mechanism exists** — `LinkerDriverOptions.additionalLibraries`
  emits `-l<lib>`; `-L`/`--lib-path` CLI flag exists.

### 0.2 What is broken — the Phase 10 scope

1. **Struct-by-value FFI calls are broken (critical ABI bug).**
   `mini_scale(p, 2.0)` where `p: MiniPoint` should return
   `(3+4)*2.0 = 14.0`; it returns **2101050012** (garbage — the
   struct's address read back as its value). Root cause:
   `mapType` maps Struct → Pointer, and the extern declaration
   (`IRBuilder.cpp` ~2208) declares the C function with pointer
   params, so SP passes the storage address where C expects the
   struct in registers / on the stack by value. Struct-by-pointer
   works; struct-by-value does not. (Struct-by-value RETURNS are
   untested — same mechanism, likely the same bug.)
2. **No way to link a native library.** There is no `--link` /
   `-l` CLI flag and no sp.toml native-lib key;
   `additionalLibraries` is never populated from anywhere. `spc run`
   on a program calling libmini fails at link with `undefined
   reference to mini_add` (masked by the known misleading
   "Unsupported backend compiler" message). The proof above only
   ran via a manual `clang++ use_mini.o -L. -lmini ...` link.
3. **Bindgen-emitted module `const`s do not bind through import.**
   `MINI_VERSION` (a `const` in the generated `mini.sp`) fails with
   E1002 in the importer, while the generated structs and extern
   fns bind fine. Same class as the Phase 9 slice-E finding (only
   classes bind namespaces in file modules) — here it is specifically
   module-level `const` declarations not being merged by
   `ModuleLoader`.
4. **Bindgen cannot handle real headers.** `spc bindgen` on
   `/usr/include/zlib.h` yields **0 functions** and output that does
   not parse: a malformed `struct * {}` (from a `typedef struct
   X *Y;` misparse), unknown types (`z_off64_t` — a typedef the
   parser never resolved). Causes, all in the hand-written
   tokenizer: macro-wrapped prototypes (`ZEXPORT int ZEXPORT
   compress OF((...))` — `OF` defeats the function parser),
   calling-convention macros, function-pointer typedefs silently
   dropped (`typedef void (*MiniCallback)(int)` produces nothing,
   yet `mini_register(cb: MiniCallback)` references the undefined
   type), `#include`d types invisible (the tokenizer skips the
   directive; no `-I` flag in the CLI even though
   `BindgenOptions.includeDirs` exists), variadic `...` recorded
   (`isVariadic`) but never emitted, anonymous struct/union fields
   misparsed.
5. **Variadic extern declarations rejected by the parser.**
   `fn printf(fmt: ptr<char>, ...)` fails with E1000 "Expected
   parameter name". §42 documents this ("C variadics cannot be
   declared") — it is a real limitation, not just a doc note.

### 0.3 Design

- **A — struct-by-value ABI.** Extern `"C"` declarations must use
  the struct VALUE type for struct params (and struct returns),
  not the pointer `mapType` gives; call sites load the struct value
  before the call. The LLVM backend then applies the System V
  classification (register/stack) itself. Scope: params first,
  returns if the same mechanism covers them; nested structs follow
  the same path.
- **B — linking.** A `--link <lib>` / `-l<lib>` flag on `spc build`
  and `spc run` (repeatable) feeding
  `LinkerDriverOptions.additionalLibraries`; an sp.toml `[native]`
  section (`libs = [...]`, `lib-paths = [...]`) read by spm's build
  path. No new linker machinery — only the wiring.
- **C — bindgen on real headers.** Harden the existing tokenizer
  for the patterns real headers use: macro-wrapped prototypes
  (`OF(...)`, `ZEXPORT`/`ZEXTERN` and friends as skippable
  prefixes), function-pointer typedefs (emit as SP fn-pointer
  types — Phase 3 added those), `#include` following with `-I` /
  `--include-dir` wired through, variadic emission (needs the
  parser to accept `...` in extern decls — see slice C), typedef
  resolution for named types (`z_off64_t` etc.), anonymous
  struct/union fields. Target: `zlib.h` generates SP that parses
  and `spc check`s, with all its functions present; a
  compress/uncompress round-trip through the generated bindings
  proves it end to end.
- **D — module consts.** `ModuleLoader` merges module-level `const`
  declarations into the importer (like it already does for extern
  fns). Proof: `MINI_VERSION` resolves via `import mini`.
- **E — docs + doc gate.** Syntax Guide §42 rewritten to the real
  contract: struct-by-value example, `--link` usage, bindgen on a
  real header (zlib round-trip), `ptr<char>`/string notes, and an
  ownership paragraph (who frees what across the boundary — the
  ADR's own negative consequence). Doc gate re-run.

### 0.4 Decision — libclang vs the tokenizer

The ADR says bindgen parses "via libclang". No libclang headers
exist on this machine (only the runtime `libclang.so.1` in the
local clang extraction; no `clang-c/Index.h` anywhere, no
system `libclang-dev`). A libclang bindgen would add a build-time
dependency that gets wiped on VM replacement and a runtime `.so`
dependency for `spc`. **Decision: keep the tokenizer-based
bindgen and harden it for real-header patterns (slice C).** Zero
new dependencies; the observable goal — real headers generate
working bindings — is the same. If Shanto wants the libclang
rewrite, that is a one-line override (and needs `libclang-dev`
installed first).

### 0.5 Slices

- **A — Struct-by-value FFI calls.** Extern decls use struct value
  types; call sites load the value. Proofs: `mini_scale` → 14.0;
  struct return (if covered); nested struct; Phase 9 slice-D
  struct proofs unchanged.
- **B — Linking a native library.** `--link`/`-l` flag + sp.toml
  `[native]`; proof is `spc run use_mini.sp --link mini -L .`
  with NO manual link step.
- **C — Bindgen on real headers.** Tokenizer hardening (§0.3-C);
  proof is zlib.h → parseable, check-passing bindings with all
  functions + a live compress/uncompress round-trip.
- **D — Module consts bind.** `ModuleLoader` merges `const`s;
  proof: `MINI_VERSION` via import.
- **E — Docs + doc gate.** §42 rewrite + ownership paragraph;
  doc gate re-run (baseline 121/129 RUN-OK).

### 0.6 Expected outcomes

- ctest 859 → 859 + new tests (only UDP EPERM; PG skipped).
- All Phase 38/39 live proofs byte-identical (no ARC/Result regression).
- Doc gate 121/129 genuine (unchanged, plus any new §42 blocks).
- `docs/v2/PHASE-39-PRO-4.0.0.md` + slice docs A–E at the end.

### 0.7 Graphify protocol (repo AGENTS.md)

`python -m graphify update . --force` cannot run: `graphify-out/`
does not exist and `python -m graphify` is not installed (verified
2026-11-10, as in Phases 5–9). Not fabricated; the phase proceeds
without it.
