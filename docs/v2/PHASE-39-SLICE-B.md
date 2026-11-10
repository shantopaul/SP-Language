# Phase 39 slice B — Linking a native library (ADR-004)

## 0. Bug (verified 2026-11-10)

There was no way to link a native library. No `--link`/`-l` CLI
flag (the CLI arg parser at `tools/spc/main.cpp` ~690 handles `-L`/
`--lib-path` only), no sp.toml native-lib key, and
`LinkerDriverOptions.additionalLibraries` was never populated from
anywhere — even though `LinkerDriver` already emits `-l<lib>` for
each entry (GNU path, `LinkerDriver.cpp:158`). `spc run` on a
libmini program failed at link with `undefined reference to
mini_add` (masked by the known misleading "Unsupported backend
compiler" message).

## 0.1 Design

1. **`CodegenOptions.additionalLibraries`**
   (`compiler/include/sp/codegen/LLVMGenerator.h`) — plumbed in
   both link paths of `compileToExecutable`:
   - Fast path (default, `useDirectLinker`): `linkOpts.additionalLibraries
     = m_options.additionalLibraries` (LinkerDriver already emits
     `-l<lib>` per entry).
   - Legacy clang path: `-l<lib>` appended after the `-L` flags.
2. **CLI** (`tools/spc/main.cpp`): `--link <lib>` / `--link=<lib>`
   / `-l <lib>` / `-l<lib>` (repeatable) → `codegenOpts.additionalLibraries`.
   `-l` takes a bare library name (`mini`, not `libmini.so` and
   not `-lmini`): the linker adds the `lib` prefix and `.so`/`.a`
   suffix itself, exactly like `-l` in clang/gcc. Help text lists
   the new flags next to `-L`/`--lib-path`.
3. **sp.toml `[native]`** (`tools/spm/ProjectConfig.{h,cpp}`):
   `libs = ["mini"]` and `lib-paths = ["./vendor/lib"]`
   (single-line and multi-line arrays, same parser style as the
   workspace `members` array). Getters `getNativeLibs()` /
   `getNativeLibPaths()`; `toTomlString()` round-trips the section
   when non-empty.
4. **spm build path** (`tools/spm/BuildOrchestrator.cpp`):
   after loading the project config, `codegenOpts.additionalLibraries
   = config.getNativeLibs()` and the config's lib-paths are
   appended to `codegenOpts.librarySearchPaths`. CLI flags and
   sp.toml compose: `spc build/run` (single-file mode) uses CLI
   flags only; `spm build` uses the project's `[native]` section.
   A bare `spc run file.sp` in a project directory does NOT read
   sp.toml — that is `spm build`/`spm run`'s job.

## 0.2 Verification (DONE 2026-11-10, build `~/workspace/build-phase10`)

- Live proof (minilib, `~/workspace/p10-probes/minilib/`): `spc run
  use_mini.sp --link mini -L.` prints `add 42 / scale 14 / sum 7 /
  filled 9 10` with NO manual link step. `spc build use_mini.sp
  --link mini -L. -o use_mini_b_bin` also works; the binary prints
  the same four lines.
- spm proof (`~/workspace/p10-probes/spm-mini/`): a scratch project
  with `sp.toml` `[native] libs = ["mini"]`, `lib-paths =
  ["/home/hatch/workspace/p10-probes/minilib"]` builds via `spm
  build` to `target/debug/mini_app`, which prints the same four
  lines.
- New tests: `ProjectConfigTest.NativeLibsParseAndRoundTrip` and
  `ProjectConfigTest.NativeLibsAbsentByDefault` — all 8
  ProjectConfig tests pass.
- Full ctest: **865/866** — only the standing UDP EPERM failure
  (`NetTest.UdpSocketSendRecv`); PG skipped (no server). Total
  grew 864 → 866 (+2 tests). Log
  `~/workspace/ctest-phase10-slice-b.log`.
- Note: copying the `spc` binary out of its build dir breaks the
  fast link path (the runtime lib is found relative to the binary),
  and the build then falls through to the legacy clang path, where
  `clang++` trips the "GCC cannot compile LLVM IR" check — the
  `g++` substring matches `clang++`. That is a pre-existing
  misleading error (the ex043/ex126/ex127 doc-gate category), not
  a slice-B regression; use the in-place binary.

## 0.3 Out of scope

- `pkg-config` integration, `.pc` files, or auto-discovery of
  system libraries (zlib etc. still need an explicit `--link z`
  / `[native] libs = ["z"]` plus the system `-L` path if any).
- `spc run` reading a neighboring sp.toml (deliberate: single-file
  mode stays self-contained; project mode is `spm`).
- Windows `.lib` handling is LinkerDriver's existing code path
  (untouched).
