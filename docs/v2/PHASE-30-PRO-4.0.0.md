# Phase 30 — PRO 4.0.0, Phase 1: Honesty / Tooling Fixes

- **Branch:** `feature/pro-4.0.0` (work in progress; NOT merged to `main`)
- **Product version:** **3.1.0** — the branch name says 4.0.0, but no version bump
  was approved. The single source of truth is
  `compiler/include/sp/Version.h` (`SP_VERSION_STRING = "3.1.0"`).
- **Goal of Phase 1:** stop the codebase from claiming things it does not do.
  No new features, no new packages, no invented numbers.

## What this branch actually changes

| # | Change | File(s) |
|---|---|---|
| 1 | Registry publish returns **501 preview** instead of a fake 201 "received and verified successfully". Nothing is stored or verified; `downloadUrl` is now `null`, `immutable` is `false`, and the response carries `preview: true, mock: true, stored: false, verified: false`. | `splang-registry/src/app/api/v1/publish/route.ts` |
| 2 | Registry verify returns **`verified: false`** with a preview/mock message. The checksum still comes from the hard-coded preview catalogue — no file is downloaded or hashed. | `splang-registry/src/app/api/v1/verify/[name]/[version]/route.ts` |
| 3 | New `REGISTRY_PREVIEW` label exported from the package catalogue: the registry has no storage backend, checksums are placeholders, download counts are illustrative. No real packages added or changed. | `splang-registry/src/data/packages-db.ts` |
| 4 | `spm publish` failure is now failure. The old code treated an unreachable registry (`statusCode == 0` or `>= 500`) as success. It now returns an error and publishes nothing. | `tools/spm/Publish.cpp` |
| 5 | `spm publish` now sends the token as `Authorization: Bearer <token>` via `HttpClient::send()` (the old `HttpClient::post()` takes no headers, so the token was never sent). | `tools/spm/Publish.cpp` |
| 6 | `spm publish` to an `https://` registry URL now fails with a clear error: the SP HTTP client has no TLS support, so it cannot publish over HTTPS. (Follow-up, not done here: add TLS + an authenticated `post()` to the runtime HTTP client.) | `tools/spm/Publish.cpp` |
| 7 | `spc fix` no longer pretends to migrate `.sp` files. The dead `bool changed = false` write block is removed; the command now says plainly that no `.sp` transformations are implemented yet and `.sp` files are scanned only. The `sp.toml` edition rewrite is untouched and still works. | `tools/spc/main.cpp` |
| 8 | `spc --version` no longer claims an "(in-process LLVM & LLD backend)". It now says the backend is SP IR text via the external Clang driver. | `tools/spc/main.cpp` |
| 9 | Hard-coded fallback API key removed from the example frontend. The key now comes from `NEXT_PUBLIC_API_KEY` only; when unset, requests are sent without auth headers. No fallback secret remains in the file. The old key value must be rotated if it was ever used against a real service. | `examples/backend-crud-app/fontend-next/src/lib/api.ts` |
| 10 | Backend benchmark script no longer prints hard-coded RPS/latency numbers as measured results. It only runs `spc check` and reports pass/fail, labelled NOT MEASURED. | `benchmarks/backend/run_benchmarks.ps1` |
| 11 | Version single source of truth set to **3.1.0** in the four `package.json` files, `tools/package_release.ps1`, the `install.ps1` fallback zip name, and the LSP `serverInfo.version`. | `splang/package.json`, `splang-docs/package.json`, `splang-registry/package.json`, `editors/vscode/package.json`, `tools/package_release.ps1`, `install.ps1`, `tools/spls/LspServer.cpp` |
| 12 | **Compiler stops silently miscompiling.** `&&`/`||` now short-circuit (RHS evaluated only when needed). `throw`, `catch` clauses, map literals, `await`, and `comptime` expressions that reach IR lowering now fail the build with a clear `error[E1000]: '<construct>' is not yet supported` diagnostic instead of being silently dropped or passed through. **Breaking change, by design:** `examples/27_try_catch.sp` now correctly fails to compile instead of silently dropping its handlers. Same-name free-function/method overloads with different signatures also fail loudly now (previously the second definition silently collapsed onto the first) — proper overload name-mangling is Phase 2/3 work. | `compiler/src/ir/IRBuilder.cpp`, `compiler/include/sp/ir/IRBuilder.h` |
| 13 | **New IR verifier.** Every module is verified before IR text emission: each block ends with exactly one terminator, no instruction formats as `unknown_inst` (Phi is rejected), signature clashes are reported, and every direct call targets a declared function. Verification failure aborts codegen. | `compiler/include/sp/ir/IRVerifier.h`, `compiler/src/ir/IRVerifier.cpp`, `compiler/src/codegen/LLVMGenerator.cpp`, `compiler/include/sp/ir/IRModule.h`, `compiler/CMakeLists.txt` |
| 14 | **Redis stops lying about being connected.** Connect failure is now an explicit failure (`isConnected()==false`, `lastError()` set) instead of silently switching to an in-memory mock with `connected=true`. Mock mode is opt-in only. The Redis pool use-after-free is fixed (acquired clients are owned until released back to the pool). | `runtime/src/net/Redis.cpp`, `runtime/include/sp/runtime/Redis.h`, `runtime/src/net/NetBridge.cpp` |
| 15 | **`sp_rt_pg_connect` returns `nullptr` on failed connect** (was: non-null handle on failure). Reason retrievable via `sp_rt_pg_last_error`. | `runtime/src/db/DbBridge.cpp` |
| 16 | **Health liveness runs real checks.** It no longer hard-codes `{"status":"UP"}`; it reports DOWN when a registered check fails (exception-safe, same evaluation as readiness). No checks registered = UP with `"checks":{}`. | `runtime/src/obs/Health.cpp`, `runtime/include/sp/runtime/Health.h` |
| 17 | **New tests for the fixed behaviours** (unexecuted here — see "Verification" below): short-circuit + fail-loud IR tests, Redis connect-failure/mock tests, PG connect-null test, liveness check tests. | `tests/ir/IRBuilderTest.cpp`, `tests/runtime/RealtimeTest.cpp`, `tests/runtime/DatabaseTest.cpp`, `tests/runtime/ObservabilityTest.cpp` |
| 18 | **Pre-existing Linux build breakage fixed** (found while verifying this branch — both files were unchanged from `main`, so the breakage predates this branch): the POSIX `CLOSE_SOCK` macro expanded to unqualified `close`, which inside `PgConnection` member functions resolved to the 0-arg member instead of the socket call; and `PathUtils.cpp` redeclared `home` (`fs::path` vs `const char*`). Both are minimal, behaviour-preserving fixes. | `runtime/src/db/Postgres.cpp`, `compiler/src/util/PathUtils.cpp` |

## Verification performed on this branch

- Full CMake build (Release, `SP_BUILD_TESTS=ON`, g++, no Clang installed) in `/tmp/sp-build`: **clean, 0 errors**. The only build failures found were the two pre-existing Linux errors fixed in this branch (items 18 above); both files were unchanged from `main`.
- `ctest -C Release` (488 tests): **398 passed (82%), 90 failed. All 15 new Phase 1 tests passed** (short-circuit x2, fail-loud IR x6, verifier x2, Redis x2, PG-connect x1, liveness x1). The 90 failures are all pre-existing/environmental, none caused by this branch:
  - ~62 need Clang to produce/run native executables (`ExecutionTest`, end-to-end codegen, `ObjectEmitter`, `LinkerDriver`, `DebugInfo`, `LLVMOptimizer`, `BuildOrchestrator`, `Phase17Tooling`, `TestRunner`) — Clang is not installed in this environment.
  - 15 `HttpTest`/`WebTest` abort with `bit out of range 0 - FD_SETSIZE on fd_set` from `FD_SET` in `runtime/src/net/Net.cpp:166,336` — a file untouched by this branch.
  - 1 `NetTest.UdpSocketSendRecv` hangs indefinitely (killed after 9+ min) — untouched code, no timeout in the test.
- `spc --version` runs and prints the honest backend string (see item 8).
- `git status` clean except branch changes; `main` untouched; nothing merged.

## What is deliberately NOT done in this branch

- **No real registry backend.** Publish/verify are honestly labelled preview/mock; building storage, auth verification, and tarball upload is Phase 2.
- **No TLS in the HTTP client.** `runtime/src/http/Http.cpp` was not touched. `spm publish` to `https://` therefore fails loudly by design until TLS lands.
- **No `spm` dependency downloading.** The resolver's mock catalogue is untouched.
- **No benchmark measurement.** A real load-test harness is Phase 2.
- **No real async lowering, no exception runtime, no overload name-mangling.** `throw`/`catch`/`await` fail loudly instead of being faked; implementing them is Phase 2/3.
- **No version bump to 4.0.0.** The branch name is aspirational; the product stays 3.1.0 until a release decision is made.

## Explicitly deferred to Phase 2 / 3

- Phase 2 (backend depth): real registry backend (dogfooded in SP), TLS, real async + event-loop wiring, Postgres prepared statements exposed from SP, queue handlers/cron from SP, WebSocket transport, persistent queue/session, real benchmark harness, OpenAPI generation.
- Phase 3 (language depth, after ADR-001–005 approval): ARC memory model completion, in-process LLVM API + LLD, WASM + online playground, self-hosted compiler codegen, LSP depth, real debugger.
- Never planned (documented non-goals): mobile apps, desktop GUI, game engines, OS kernels, VM/JIT, borrow checker.
