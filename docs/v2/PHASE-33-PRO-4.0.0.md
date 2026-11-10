# Phase 33 (pro-4.0.0) — Ecosystem (spm / package registry), Part 1

> Branch `feature/pro-4.0.0`. Production quality. Branch-only push, never
> merge to main. Product version stays **3.1.0** (`compiler/include/sp/Version.h`
> is the single source of truth). This file is written slice by slice; each
> slice owns its own section. (Slice A created this file; other slices add
> theirs.)

---

## Slice A — spm / package ecosystem end-to-end (real registry backend)

### What was verified as already working (live, before any fix)

Baseline run with the pre-slice binary (`af13f06` + in-progress slices),
registry service built from `examples/registry-backend/` on
`127.0.0.1:8090`, throwaway token, fresh DB. All outputs below are observed,
not inferred.

1. **Registry service itself works.** `GET /api/health` →
   `{"status":"ok","service":"sp-registry","version":"3.1.0","backend":"sp","database":"sqlite","packages":0}`.
2. **`spm publish` metadata POST works** (201, `verified:false`,
   `tarballStored:false`). `greet v0.1.0`, archive 386 bytes, checksum
   `0fd716bed0e965bef2761cfb15e3f8363794afab05e4e856308c7a0f7379cef7`
   (SHA-256, deterministic — same value in every run); `greet v0.2.0`,
   393 bytes, `69427414d1ce5f75f18a0464c5ad40f1fece99214a9f6dbd79379c1b37d2bee8`.
   `GET /api/v1/packages` lists both versions.
3. **SemVer constraints are honored by the resolver.** `^0.1.0` correctly
   resolves to `0.1.0` only — for a 0-major version, caret pins the minor
   (`^0.1.0` excludes `0.2.0`; this is correct semver, not a bug). The
   diamond test already proved highest-compatible selection (`1.3.0`).
4. **`spm add` records the dep and auto-resolves** (`greet = "^0.1.0"` in
   `sp.toml`, sp.lock written).
5. **Registry releases are immutable.** Re-publishing `greet@0.1.0` →
   HTTP 409 `VERSION_ALREADY_EXISTS` (loud, correct).

### What was broken (live proof, pre-slice binary)

1. **Install never fetched from the registry.** `spm install` resolved
   only against the hardcoded mock list + local cache and wrote `sp.lock`
   — it downloaded nothing, and `.spm/packages` was never created.
2. **`spm build` of a program importing the dependency failed:**
   `error[E1009]: Module 'greet' could not be resolved from path` — the
   round-trip publish → install → build was broken at the build step.
3. **sp.lock pinned a metadata hash, not content.** Checksum was the
   double-FNV-1a of `name@version(source)`
   (`a5d4d92316e926ff404856fceafebdebe59c8fdffc179b14e61d302001e7e4ea`) —
   never the tarball SHA-256 (`0fd716be…`). `verify()` only recomputed
   that same FNV, so tampered package files were undetectable.
4. **`spm publish` sent metadata only.** No tarball upload, so the
   registry could never verify a checksum (`verified:false` forever).
5. **`search` was a hardcoded static list**, never querying the registry.
6. **Publish refused `https://` URLs** with a stale "no TLS support"
   message — Phase 2 added TLS to the HTTP client, the refusal was never
   removed.

### What was built

New files (all under `tools/spm/`):

- **`RegistryClient.h/.cpp`** — `IRegistryClient` interface +
  `HttpRegistryClient` over `sp::rt::http::HttpClient` (TLS via
  `HttpClientOptions` for `https://` URLs; peer+hostname verification by
  default). Endpoints: `listPackages` (GET `/api/v1/packages`),
  `getMetadata` (GET `/api/v1/packages/{name}/{version}`),
  `download` (GET `…/download`, base64 body + `X-Sha256` header),
  `publishMetadata` (POST `/api/v1/publish`),
  `uploadTarball` (POST `/api/v1/packages/{name}/{version}/tarball`,
  base64 body, Bearer). Base URL resolution: explicit arg >
  `SP_REGISTRY_URL` env > `http://127.0.0.1:8090`. JSON parsing is via
  `sp::rt::json::parse` (pure helpers `parsePackageList` /
  `parsePackageObject`, unit-tested directly).
- **`PackageArchive.h/.cpp`** — strict parser/extractor for the
  `SP_PACKAGE_ARCHIVE_V1` format written by `PackagePublisher`
  (header key=value lines, `---MANIFEST---` with raw sp.toml,
  `---FILES---` with `FILE:<relpath>:<size>` + exact bytes). Rejects
  garbage, truncation, bad sizes, and unsafe (`..`, absolute) paths.
  Vendor layout: entry file (`src/lib.sp`) → `<dest>/mod.sp` (so
  `import <name>;` resolves), other `src/<rest>` → `<dest>/<rest>` (so
  `import <name>.foo;` resolves), README.md kept at top level. Writes a
  `.sp-checksum` marker holding the lock checksum.

Changed files:

- **`DependencyResolver.h/.cpp`** — new `RegistryProvider`
  (`IPackageProvider`): registry versions UNION mock + cache versions
  (descending); registry metadata checksum via new virtual
  `IPackageProvider::getPackageChecksum(name, version)` (default
  `nullopt`); when the registry is unreachable it falls back to
  mock+cache with `registryUnavailable()` / `lastRegistryError()` set.
  `getPackageConfig` for a registry version downloads the tarball
  (SHA-256 verified by the client) into the cache first, because the
  registry metadata carries no dependency list — transitive deps live in
  the package's sp.toml inside the archive. New virtual
  `IPackageProvider::lastError()`: the resolver now **refuses to
  silently install an older version** when the newest matching version
  cannot be fetched ("Refusing to install an older version of 'X':
  version 0.2.0 matches the requirement but could not be fetched… Pin
  the older version explicitly if that is intended."). The resolver
  stores the provider's content checksum in `ResolvedPackage.checksum`
  when one is known, else the legacy metadata hash. `getPackageChecksum`
  also returns the cached archive's own SHA-256 when the registry is
  unreachable, so an offline resolve still pins a content hash (this
  was found by the live offline proof — see Verification).
- **`Cache.h/.cpp`** — `verifyChecksum` now hashes the cached archive
  bytes when an archive file exists (content check); only when no
  archive exists does it fall back to comparing the recorded
  `.sp-checksum` string (legacy cache; detects a changed record, not
  tampered files). New `storeArchive` (writes archive bytes +
  `.sp-checksum` + extracts sp.toml + src/ into the cache dir, refusing
  bytes whose SHA-256 disagrees with the claimed checksum),
  `getCachedArchivePath`, `readCachedArchive`.
- **`BuildOrchestrator.h/.cpp`** — new `install(projectDir,
  registryUrl, forceResolve, frozen)` (plus a test seam overload taking
  a `shared_ptr<IRegistryClient>`): resolve with `RegistryProvider` →
  write sp.lock with content hashes → `ensureVendored`. New
  `ensureVendored(projectDir, lock, …)`: for each registry entry, skip
  when the vendored `.sp-checksum` marker matches the lock; else take
  archive bytes from the cache (hash-verified) or download from the
  registry (hash must equal the lock checksum, else loud refusal),
  parse + extract into `.spm/packages/<name>/`. Up-to-date locks still
  vendor (a fresh checkout has sp.lock but no `.spm/packages`). Legacy
  lockfiles (registry checksums = metadata hashes) are re-locked with
  real tarball SHA-256 values **without changing versions** (hash from
  registry metadata, else from cached archive bytes; fails loudly when
  neither source is available). `build()` now resolves through the
  `RegistryProvider` and vendors before compiling. `verify()` now:
  legacy metadata checksum for a registry entry → loud failure ("uses a
  legacy metadata checksum, not a content hash. Run 'spm install' to
  re-lock…"); content hash → verified against the cached archive bytes
  (missing archive → loud failure; hash mismatch → "Checksum mismatch /
  tampering detected"). Path/git entries keep the FNV check unchanged.
  `search()` queries the real registry first; on failure it prints an
  explicit warning and falls back to the built-in offline index.
  `defaultRegistryUrl()` helper (explicit > `SP_REGISTRY_URL` >
  `http://127.0.0.1:8090`).
- **`Publish.cpp`** — after the metadata POST (201), uploads the
  tarball (base64) to the tarball endpoint with Bearer auth (one retry
  on transport failure only). A failed upload is a loud failure with
  manual-recovery instructions (re-running `spm publish` cannot fix it:
  the registry rejects duplicate versions as immutable). The stale
  `https://` refusal is removed (TLS now works). The printed Status now
  appends "Tarball uploaded and verified against the published checksum
  (verified:true)." — the metadata response alone still says
  `verified:false`, which was misleading. Dry-run behavior unchanged
  (no network, archive + cache only).
- **`main.cpp`** — `install` / `update` route through
  `BuildOrchestrator::install` (with `--registry <url>` and `--frozen`);
  `add` auto-installs and warns loudly (instead of silently skipping)
  when the install fails; `search` accepts `--registry <url>`; help
  lists `--registry <url>`.
- **`tools/CMakeLists.txt`** — `RegistryClient.cpp`,
  `PackageArchive.cpp` added to `spm_core` (no new link deps:
  `spm_core` already links `sp_runtime`).

### Exact semantics

- **Resolution order:** registry index (when reachable) ∪ mock ∪ cache,
  highest version satisfying ALL accumulated constraints wins.
  `^0.1.0` matches `0.1.x` only (0-major caret); `*` picks the highest
  published version; unsatisfiable constraints fail loudly
  ("No compatible version found…"), never install a wrong version.
- **Lockfile checksums:** registry entries pin the tarball SHA-256
  (online: registry metadata; offline: cached archive bytes). Mock /
  path / git entries keep the legacy metadata hash.
- **Vendoring:** `.spm/packages/<name>/mod.sp` (+ other `src/*.sp`,
  README.md, `.sp-checksum` marker). Re-vendoring is skipped when the
  marker matches the lock. A cached archive whose hash disagrees with
  the lock is never trusted — `install` re-downloads it from the
  registry (self-healing); `verify` reports it.
- **Offline install (honest):** registry unreachable → resolve from
  cache with a printed note
  ("[spm] note: registry at … unreachable …; resolved from the local
  package cache only."). Works only when the needed versions are cached
  with verifiable archives; otherwise fails loudly. Offline does NOT
  pretend the registry was consulted.
- **Publish:** metadata POST then tarball POST; the package is
  `verified:true` only after the tarball upload. Metadata-published but
  tarball-failed leaves the package `verified:false` and fails the
  command loudly (manual upload path printed).
- **Search:** registry-first; unreachable registry → explicit stderr
  warning + built-in offline index (which is still only the 7 mock
  packages — it is a fallback, not the registry).

### Tests

New file `tests/spm/RegistryEcosystemTest.cpp` (12 tests, all passing;
registered in `tests/CMakeLists.txt`):

| Test | Proves |
|---|---|
| `ArchiveRoundTripPreservesFilesAndVendorLayout` | parse + extract, entry → `mod.sp`, `src/util.sp` → `util.sp`, checksum marker |
| `ArchiveParseRejectsGarbageAndTruncation` | garbage / header-only / truncated / bad-size archives fail |
| `ClientParsesListAndMetadataJson` | list + metadata JSON parsing (incl. `tarballStored`, sizes); malformed JSON fails |
| `ClientResolvesBaseUrlExplicitThenEnvThenDefault` | explicit > `SP_REGISTRY_URL` > `http://127.0.0.1:8090` |
| `ProviderPicksHighestMatchingVersionWithContentChecksum` | `*` → 0.2.0 with real tarball SHA-256 in the resolution; `^0.1.0` → 0.1.0; `^9.0.0` fails loudly |
| `ProviderRefusesSilentDowngradeWhenNewestUnfetchable` | 0.2.0 unfetchable + `*` → resolution FAILS ("Refusing to install an older version"), no silent 0.1.0 |
| `ProviderFallsBackToCacheWhenRegistryIsDown` | registry down → resolves 0.1.0 from cache, lock pins the archive hash, `registryUnavailable()` true |
| `CacheArchiveHashDetectsTampering` | 1 flipped byte in the cached archive → `verifyChecksum` false (recorded string unchanged) |
| `InstallVendorsFromRegistryAndVerifyPasses` | `install()` vs stub registry: sp.lock pins tarball SHA-256, `mod.sp` vendored, `verify()` passes |
| `InstallVendorsAgainOnFreshCheckout` | lock up-to-date + `.spm` deleted → second install re-vendors (no no-op) |
| `InstallRelocksLegacyChecksumWithoutChangingVersion` | FNV lock → install re-locks to the content hash, version stays 0.1.0 |
| `VerifyFailsLegacyLockfile` | FNV lock → `verify()` fails with "legacy" |

Updated `tests/spm/BuildOrchestratorTest.cpp` (pre-existing tests, new
semantics): `SearchRegistry` pins `SP_REGISTRY_URL` to an unreachable
address so the test exercises the offline fallback regardless of
whether a real registry is listening on localhost:8090;
`VerifyProjectIntegrityAndTamperDetection` v3 now builds a valid lock
from a real dry-run-published archive (content hash) instead of an FNV
hash, and adds v3b (legacy FNV lock → verify fails with "legacy"). The
v2 step ("verify should fail because sp.lock is missing") still
expects failure — with the new code it fails for a different, honest
reason: the self-healed auto-generated lock carries an FNV checksum
for the mock `sp_math` package, which `verify()` now rejects as legacy.

### Verification

- **Full Release build** in `/tmp/sp-build-phase4-spm`
  (`-DSP_BUILD_TESTS=ON`): clean, 0 errors. (One pre-existing warning
  in `compiler/src/util/PathUtils.cpp:596`, unused `isMsvc` — untouched.)
- **spm-area suites: 33/33 pass** (`RegistryEcosystemTest`,
  `SemVerTest`, `DependencyResolverTest`, `LockfileTest`,
  `BuildOrchestratorTest`, `Phase17ToolingTest`).
- **Full ctest: 636/643 pass (99%).** 7 failures, none in this slice's
  area (see Blockers):
  `NetTest.UdpSocketSendRecv` (pre-existing, Phase 3);
  `JsonTest.BenchmarkOneGigabytePerSecThroughput` (147 MB/s vs 1 GB/s
  required — throughput assertion on a throttled 1.5-core VM; no JSON
  code touched);
  `SplsPhase17Test.InlayTypeHintsInfersOmittedTypes` (SegFault in
  `sp::lsp::SymbolIndex::walkExpr`);
  `LspDepthTest.SemanticTokensBasic`;
  `DapServerTest.StandardFlowStopsAtBreakpoint`,
  `DapServerTest.LoopIterationValues`, `DapServerTest.SetVariableWorks`.
- **Live round-trip, final binary, clean cache** (registry service on
  `127.0.0.1:8090`, fresh DB, `SP_CACHE_DIR` empty):
  `spm publish greet v0.1.0` → 201 + "Tarball uploaded and verified
  against the published checksum (verified:true).";
  `GET /api/v1/verify/greet/0.1.0` → `verified:true,
  tarballStored:true`; `spm add greet@^0.1.0` → resolves 0.1.0, sp.lock
  checksum `0fd716be…` (real tarball SHA-256); `spm build` →
  "Finished debug target: ./target/debug/hello"; run →
  `Hello, world from greet!`; `spm verify` → "Verified: package
  integrity and lockfile checksums valid (1 package verified, 0
  tampering detected)."
- **Live constraint proofs:** `greet@*` → resolves 0.2.0, runs
  `Hello, world from greet v0.2.0!`; `greet@^9.0.0` → loud failure
  ("No compatible version found…"); `spm search greet` → 2 matches
  (0.1.0, 0.2.0) from the real registry.
- **Live tamper proof:** 1 flipped byte in the cached archive →
  `spm verify` fails exit 1: "Checksum mismatch / tampering detected
  for package 'greet' (cached archive SHA-256 does not match sp.lock)".
  (`spm install` would instead re-download the pristine bytes from the
  registry — self-healing; `verify` is the reporter.)
- **Live offline proof:** registry stopped, cache populated →
  `spm add greet@^0.1.0` resolves from cache with the printed
  "[spm] note: registry … unreachable …; resolved from the local
  package cache only.", sp.lock pins `0fd716be…`, build + run +
  verify all pass.
- **Live fresh-checkout proof:** `rm -rf .spm` with sp.lock present →
  `spm install` re-vendors ("Installed 1 package(s) into
  .spm/packages/."), verify passes.

### Blockers / findings outside this slice (not fixed — other slices' areas)

1. **Shared tree did not build at slice start** (other slice's
   half-finished LSP edit): `tools/spls/LspServer.cpp` referenced
   `sp::lsp::SymbolIndex::resolveAt` / `referencesOf` while
   `tools/spls/SymbolIndex.cpp` was not yet linked into `spls`
   (undefined references, `spls` link failed; `spc`/`spm` built fine).
   It links again now — the LSP slice has since completed that part —
   but the new code segfaults: `SplsPhase17Test
   .InlayTypeHintsInfersOmittedTypes` crashes in
   `SymbolIndex::walkExpr` ← `SymbolIndex::build` ←
   `LspServer::handleInlayHint` (gdb backtrace; also fails ctest).
   `LspDepthTest.SemanticTokensBasic` also fails. Not touched, per the
   slice rules.
2. **Debugger slice's new tests fail:** `DapServerTest
   .StandardFlowStopsAtBreakpoint`, `.LoopIterationValues`,
   `.SetVariableWorks`. Not touched.
3. **Committed Phase 3 regression blocks building the registry
   service with the repo stdlib:** `stdlib/web/app.sp:271`
   (`App::openApi`) captures `specJson` in a lambda, which Phase 3
   made a loud compile error — so **any** program importing `std.web`
   fails to build (`error[E1000]: 'lambda captures variable
   'specJson''`). The live proof therefore built the service with
   `spc --stdlib` pointing at a `/tmp` copy of stdlib in which ONLY
   that dead-for-the-service method was neutralized (empty-spec
   literal); the service never calls `App::openApi` (it registers
   `/openapi.yaml` via `app.get` directly). The repo tree was NOT
   modified for this. A real fix needs either capturing lambdas or an
   `App::openApi` redesign — follow-up work, not this slice.
4. **Third VM replacement mid-slice** wiped `/tmp` (build dir, proof
   dirs, registry DB) and Clang; Clang 18.1.3 was present again on the
   new VM (os-intent replay) and the build + proofs were redone from
   scratch. Baseline proof outputs in this document were recorded
   before the wipe and re-verified after it.
5. `examples/registry-backend/src/handlers.sp:20` still comments that
   "spm publish sends metadata only" — now stale (publish uploads the
   tarball). Left untouched (registry-backend is slice D's area); the
   service code itself is correct either way.

### Deliberately not done

- **No git dependencies over the network.** `git = "…"` deps still
  resolve through the local provider only; cloning/fetching a git
  remote was not in this slice.
- **No registry search endpoint.** `search` lists (`GET
  /api/v1/packages`) and filters client-side; the service has no
  query endpoint to use.
- **No signature / provenance beyond SHA-256.** The tarball hash pins
  bytes; there is no author signing, no checksum transparency log.
- **No `spm login` / token storage change.** Publish still reads the
  token from `--token` / `SP_REGISTRY_TOKEN` env / `~/.sp/credentials`
  — unchanged.
- **No version bump.** Product stays **3.1.0**; the branch name is
  aspirational only.
- **No changes to the registry service itself.** Its handlers already
  implemented metadata + tarball + verify + download correctly; only
  the client was missing.
- **Mock registry kept.** `DefaultPackageProvider`'s hardcoded mock
  list (`sp_math` etc.) is unchanged and still participates in
  resolution as a fallback — removing it would break existing tests
  and any workflow that scaffolds against it. A real registry package
  with the same name would shadow the mock only by version union
  (both are listed; the highest matching version wins).

---

## Slice D — Verification debt + docs sync

> Scope: close the verification debt left by Phases 2-3 (the 3
> pre-existing full-suite failures, the unverified PG `string[]`
> marshalling, the missing wss:// e2e test) and sync the living docs
> with what Phases 1-3 actually made real. All outputs below are
> observed, not inferred. No commit / push / merge by this slice (the
> parent commits part-by-part and pushes once at the end). Product
> version stays **3.1.0**.

### D1. stdlib-time epoch-ms truncation — root cause: sema + stdlib, not the runtime

Failing tests: `ExecutionTest.StdlibTimeNativeExecution`
(`now_gt_0: true` missing) and `StdlibFullIntegrationNativeExecution`
(`time_now_gt_0: true` missing).

Root cause: the runtime returns `int64_t` (`sp_rt_time_now_ms`,
~1.7e12 ms; `runtime/src/Runtime.cpp:1763`, declared i64 in
`compiler/src/ir/IR.cpp:808`), and IRBuilder already special-cases
`now`/`nowSeconds`/`nowNanos`/`formatTime`/`parseTime` to i64
(`compiler/src/ir/IRBuilder.cpp:3852-3870, 6411-6466`). But sema
declared the builtins `now`/`nowSeconds` as `-> int` (32-bit) and
`formatTime(int)->string` / `parseTime(string)->int`
(`compiler/src/sema/SymbolTable.cpp:341-342, 371-372`), and stdlib
`stdlib/time/time.sp` declared the externs + wrappers the same way.
1.7e12 truncated to 32 bits is negative, so `now() > 0` was false.
The actually-wrong layer was sema + stdlib, not the runtime.

Fix (`compiler/src/sema/SymbolTable.cpp`, `stdlib/time/time.sp`):
`now`/`nowSeconds`/`nowNanos` return `int64`; `formatTime` takes
`int64`; `parseTime` returns `int64` (incl. the `Time.*` static
methods). `sleep`/`sleepSeconds` stay `int`; `Duration` stays `int`
(out of scope). Tests updated to `int64` variables
(`tests/codegen/ExecutionTest.cpp`); examples updated
(`examples/stdlib_time.sp`, `examples/38_modules_imports.sp`).

Verification: both tests pass; `spc run examples/stdlib_time.sp`
prints `Current epoch ms > 0: true`, `Formatted 1700000000:
2023-11-14T22:13:20Z`, `Parsed back: 1700000000`;
`examples/38_modules_imports.sp` prints `Time.now() ms:
1794217331936` (full int64 epoch-ms, previously truncated negative).

### D2. UDP hang — environmental EPERM + a real unbounded-receive defect

Failing test: `NetTest.UdpSocketSendRecv` hung until ctest's 120 s
timeout. Two layers, proven separately:

1. Environmental: `sendto` fails with `EPERM (Operation not
   permitted)` in this VM -- proven by `strace` on the test binary
   (`sendto(4, "UDP SP Message", 14, ...) = -1 EPERM`) and by an
   independent Python UDP `sendto` to 127.0.0.1 failing the same way
   (TCP works fine). This sandbox blocks UDP sendto; on a normal
   machine the send succeeds.
2. Real code defect: `UdpSocket::recvFrom` (`runtime/src/net/Net.cpp`)
   was a plain blocking `::recvfrom` with **no timeout** (unlike
   `TcpSocket`, which has `setTimeouts`). When the send failed, no
   datagram ever arrived and the test hung forever instead of failing.

Fix: added `UdpSocket::setTimeouts(recvTimeoutMs, sendTimeoutMs)`
(`runtime/include/sp/runtime/Net.h`, `runtime/src/net/Net.cpp`),
mirroring `TcpSocket::setTimeouts`. Test now uses `ASSERT_EQ(sent,
14)` (fail fast -- never proceed to a recv that cannot succeed) and
bounds the receive to 5 s (`tests/runtime/NetTest.cpp`).

Verification: the test now fails in **0.01 s** with a clear
`sent == -1` instead of hanging 120 s. In this sandbox it stays red
**only** because of the EPERM environment (proven above); where UDP
loopback is permitted it passes. The hang is fixed; the failure is
honest and visible, not masked.

### D3. ObjectEmitter flakiness — shared fixed temp dir (same defect in two more fixtures)

Failing tests: `ObjectEmitterTest.EmitsNativeObjectFromIRText` /
`EmitsObjectFromIRModule` failed in full `-j2` runs but passed alone
(5 of 6 `-j2` runs failed before the fix).

Root cause (proven with clang's own message): the fixture used one
fixed shared dir `/tmp/sp_obj_emitter_test` for every parallel ctest
process, and `SetUp`/`TearDown` did `remove_all` on it:
`error: unable to open output file
'/tmp/sp_obj_emitter_test/compute.o': 'No such file or directory'`
-- one process deleted the directory while another process's clang
was opening its output file inside it.

Fix: unique temp dir per test process + test name (pid + test name,
the pattern `TlsTest` already uses) in
`tests/codegen/ObjectEmitterTest.cpp`. Hardened the emitter itself too
(`compiler/src/codegen/ObjectEmitter.cpp`): the temp IR path now
includes the pid (the old `steady_clock + per-process nonce` scheme
could collide between two processes started at the same time), and
`emitObjectToMemory` / `emitObjectFromIRToMemory` use pid + atomic
nonce instead of unseeded `std::rand()`.

The full-suite run then exposed the **same defect** in two more
fixtures, fixed with the same pattern:
`tests/codegen/LinkerDriverTest.cpp` (`/tmp/sp_linker_driver_test`,
failed in the first full run, passes alone) and
`tests/codegen/DebugInfoGeneratorTest.cpp` (`/tmp/sp_dbg_info_test`,
same pattern, fixed proactively).

Verification: `ctest -R 'ObjectEmitterTest' -j2` 6/6 runs at 100%
(before: 5/6 runs failed); `ctest -R
'LinkerDriverTest|DebugInfoGeneratorTest|ObjectEmitterTest' -j2`
5/5 runs at 13/13. Also fixed a stale comment in
`tests/runtime/TlsTest.cpp`: the "KNOWN ENVIRONMENT BLOCKER"
(FD_SETSIZE abort at HttpServer teardown) is gone since Phase 2
slice C fixed the `Net.cpp` acceptTimeout race -- all 8 TlsTest cases
pass now.

### D4. PG `string[]` marshalling — a REAL ABI bug, not just unverified

Phase 2 left `pg_exec_prepared` with `string[]` params declared but
never verified by an SP run. It was worse than unverified.

Proof (before fix, `spc llvm` IR dump of an SP program calling
`queryParams` + `execPrepared` with a `string[]`):
```
declare ptr @sp_rt_pg_query_params(ptr, ptr, ptr)
declare ptr @sp_rt_pg_exec_prepared(ptr, ptr, ptr)
%c13 = call ptr @sp_rt_pg_query_params(ptr %mem_val10, ptr %v11, ptr %v12)
```
The C ABI is 4 args (`runtime/include/sp/runtime/Runtime.h:556-558`,
`runtime/src/db/DbBridge.cpp:280-322`): `(conn, sql, const char**
params, int64_t param_count)`. The emitted call passed only 3 args,
so the callee read a **garbage param_count** and the array header
pointer where `const char**` was expected.

Fix (stdlib-level marshalling, no compiler change --
`stdlib/db/postgres.sp`): the extern decls now use the 4-arg C ABI,
and `queryParams` / `execPrepared` expand the array with
`sp_rt_array_data(params)` (element storage pointer -- for `string[]`
that is `const char**`, `runtime/src/Runtime.cpp:805`) +
`sp_rt_array_length(params)`. The helpers are declared with
`string[]` params so sema accepts an SP array (a `ptr` param was
rejected with `E1001`; verified by probe); they lower to the same
`ptr` signatures `IR.cpp:908-914` predeclares, so no signature clash.
No other callers exist outside `postgres.sp`.

Proof (after fix, same IR dump):
```
declare ptr @sp_rt_pg_query_params(ptr, ptr, ptr, i64)
%c13 = call ptr @sp_rt_array_data(ptr %v12)
%c15 = call i64 @sp_rt_array_length(ptr %v14)
%c16 = call ptr @sp_rt_pg_query_params(ptr %mem_val10, ptr %v11, ptr %c13, i64 %c15)
```
4-arg calls matching the C ABI exactly, for both functions.

C-ABI-level test (new `PostgresB1.PgSpArrayMarshallingRoundTrip` in
`tests/runtime/PostgresTest.cpp`, reusing the Phase 2 fake
PostgreSQL wire-protocol stub -- **no real Postgres in this
environment**: `which psql/pg_ctl/postgres` empty, no
`/usr/lib/postgresql/`): builds an SP string array with
`sp_rt_array_alloc`, marshals it exactly like the fixed stdlib,
drives the pair through `sp_rt_pg_exec_prepared` and
`sp_rt_pg_query_params` against the stub, and asserts the server
received **both** params in order (`srv.lastBindParams() == {"42",
"hello"}`) plus the empty-array edge (length 0, simple-query
fallback, one row `"1"`).

Verification: the new test passes, with the neighboring
`PgCAbiPrepareAndExecPrepared` / `PreparedStatementPrepareExecAndReuse`
tests (3/3). `spc run` on the proof program prints `done` (connect
fails honestly with no server; the `handle != null` guards skip the
queries -- no crash).

### D5. wss:// end-to-end test

New `tests/runtime/WssTest.cpp` (registered in
`tests/CMakeLists.txt`): TLS `HttpServer` + the global
`WebSocketServer` echo route (`/wss-echo`), and a minimal in-test WS
client over raw OpenSSL (same cert-generation pattern as `TlsTest`:
CA + CA-signed server cert with SAN `IP:127.0.0.1,DNS:localhost`,
unique temp dir, explicit SKIP if the `openssl` CLI is missing).
Two cases: (a) `WssEchoRoundTrip` -- TLS handshake with the test CA
trusted (real verification, IP SAN checked, no skip-verify), RFC 6455
upgrade (101 + `Sec-WebSocket-Accept` checked against
`computeWebSocketAccept`), masked client text frame, server echo
`hello-wss` over the same TLS connection; (b)
`WssRejectsUntrustedCert` -- the same handshake fails without the
test CA (verification is on, not silently skipped).

Verification: 2/2 pass.

### D6. Comptime double-recursion folded WRONG — found by the docs sync, fixed

While verifying the spec §3.2 example (`comptime fibonacci(10)`),
the evaluator returned **-80** instead of 55 (fibonacci(5) gave
**-15**). Runtime fibonacci was correct (55/5); comptime factorial
was correct (3628800). Only double recursion was wrong.

Root cause (`compiler/src/sema/ComptimeEvaluator.cpp:83-94`):
`setVar()` searched ALL enclosing scopes and overwrote the first
existing binding. A recursive call's parameter binding therefore
clobbered the CALLER's parameter: `fibonacci(5)` called
`fibonacci(4)`, which overwrote the outer `n` with 4, so after the
first recursive call returned, `fibonacci(n - 2)` read the corrupted
`n`. Single-recursion `factorial` was immune (it never reads `n`
again after a recursive call returns) -- which is why the Phase 3
tests never caught it.

Fix: new `defineVar()` (always a fresh binding in the innermost
scope) used for function parameters and `let` declarations; `setVar()`
(nearest-existing-binding update) kept only for true assignments
(`AssignExpr`). New tests in `tests/sema/ComptimeEvaluatorTest.cpp`:
`ComptimeFnDoubleRecursionFibonacci` (5 and 55) and
`InnerLetShadowsOuterVariable` (inner `let x = 99` must not clobber
outer `x = 10`).

Verification: `comptime fibonacci(5) = 5`, `comptime fibonacci(10)
= 55`; spec §3.2 example prints `Fibonacci(10) = 55`;
`ComptimeEvaluatorTest` 12/12 pass.

### D7. Docs sync — stale claims corrected (all verified empirically with `spc`)

Living docs only; phase docs and dated analysis reports are history
and were not touched. Every corrected example in the docs was run
through `spc check`/`spc run` before being written; every removed
claim lists the observed error. No ADR approval invented anywhere.

| Doc | Stale claim | Reality (observed) | Fix |
|---|---|---|---|
| `docs/SP-Syntax-Guide.md` §27 | Typed catches (`catch (ArithmeticException e)`), `e.message`, `throw Exception("...")`, `100 / 0` caught | `e.message` fails `E1002` (binder is `any`); `throw Exception(...)` fails `E1003` (no such function); `100 / 0` kills the process with `SIGFPE`, never caught; a throw is caught only in the **same** function (throw in a callee panics: `SP Panic: ... / Aborted`) | Rewrote §27 to the real semantics: catch binds the thrown scalar (`catch (string e)` / `catch (int e)`), verified example runs |
| Same guide §28 | `Ok(...)` / `Error(...)` constructors + `match` on Result | `Error(...)` fails `E1003`; `match` on Result fails `E1001` | Rewrote §28 to the real API: `Result.Ok(...)` / `Result.Err(...)` + `isOk()` / `isErr()` / `unwrap()` / `unwrapErr()` / `unwrapOr()` (the passing `ExecutionTest.ResultAndOptionZeroAllocTaggedUnionExecution` test) |
| Same guide §33 | Method overloading | **TRUE now** (Phase 3 A) -- verified: `Printer.print` overloads print `Integer: 42 / Double: 3.14 / String: hi / n: 7` | No change |
| Same guide §34 | Operator overloading ✅ (`operator +`) | Parse error `E1000: Expected field name in struct` | Heading changed to ❌ Not Implemented; example replaced with a verified named-method version (`class Vector2` + `add()` / `equals()`, `new Vector2(...)`) |
| Same guide §39 | `await http.get(...)` (async I/O) | `http` undefined (`E1002`); no async I/O exists | Rewrote to the real awaitables: async fns + `sleep_async` / `delayed_value`; verified example runs (`Result 1: 20 / Result 2: 42`) |
| Same guide §101 | comptime factorial | **TRUE** -- verified again (`comptime factorial(10)` = 3628800) | No change |
| Same guide §103 | `spawn`, `time.sleepMs(20)` | `spawn` fails to parse (`E1000`); `Time.sleepMs` fails `E1003` | Rewrote with `Task<T>` variables + `await` and `sleep_async(20)`; verified (`Node primary healthy / Node replica healthy`); also corrected the "zero OS thread context-switch penalty" claim (pump-based, frame stays on stack) |
| `docs/STDLIB_REFERENCE.md` §6 | `Time.nowMillis()` / `Time.sleepMillis(100)` | Both fail `E1003` (do not exist) | Rewrote to `Time.now()` (int64 epoch-ms) + `Time.sleep(ms)`; verified (`Elapsed: 100 ms`) |
| `docs/v2/SP_2.0_LANGUAGE_SPEC.md` §3.1 | `comptime { for...; tbl.push(...); return tbl; }` block | Does not compile (declaration fails `E1002`/`E1001`) | Corrected to the supported subset (comptime expressions + `comptime fn` calls incl. recursion); block example kept only as a quoted non-working example |
| Same spec §3.2 | `comptime fibonacci(10)` = 55 | **TRUE after the D6 fix** (was -80 before it) | No change (the fix is documented in D6) |
| Same spec §3.3 | `comptime { assert(...); }` block | Fails to parse (`E1000: Expected ';' after expression`) | Corrected: `assert` works **inside a `comptime fn`** (verified); block form removed |
| Same spec §5 intro | "without runtime overhead" | False (pump-based; awaiting frame stays on stack) | Corrected to pump-based with the heap-frame state machine still missing |
| Same spec §5.1 | `await client.get(url)` | No async I/O (same as §39) | Corrected to `await sleep_async(10)` |
| Same spec §5.3 | await outside async = `E1801` | Actual error is **`E1802`** (`'await' can only be used inside an 'async' function`, verified) | Fixed the error code |

### Files changed (Slice D only)

- `compiler/src/sema/SymbolTable.cpp` -- time builtins to int64
- `compiler/src/sema/ComptimeEvaluator.cpp` + `compiler/include/sp/sema/ComptimeEvaluator.h` -- `defineVar()` fix (D6)
- `compiler/src/codegen/ObjectEmitter.cpp` -- pid in temp IR / mem-obj names
- `stdlib/time/time.sp` -- epoch API to int64
- `stdlib/db/postgres.sp` -- PG marshalling to the 4-arg C ABI
- `runtime/include/sp/runtime/Net.h`, `runtime/src/net/Net.cpp` -- `UdpSocket::setTimeouts`
- `tests/codegen/ExecutionTest.cpp` -- int64 time vars
- `tests/codegen/ObjectEmitterTest.cpp`, `tests/codegen/LinkerDriverTest.cpp`, `tests/codegen/DebugInfoGeneratorTest.cpp` -- unique temp dirs
- `tests/runtime/NetTest.cpp` -- fail-fast send + 5 s receive bound
- `tests/runtime/PostgresTest.cpp` -- `PgSpArrayMarshallingRoundTrip`
- `tests/runtime/WssTest.cpp` (new) + `tests/CMakeLists.txt` (registered)
- `tests/sema/ComptimeEvaluatorTest.cpp` -- fibonacci + shadowing regression tests
- `tests/runtime/TlsTest.cpp` -- removed stale FD_SETSIZE blocker comment
- `examples/stdlib_time.sp`, `examples/38_modules_imports.sp` -- int64 epoch vars
- `docs/SP-Syntax-Guide.md`, `docs/STDLIB_REFERENCE.md`, `docs/v2/SP_2.0_LANGUAGE_SPEC.md` -- corrections in D7
- This file -- this section

### Verification

- Full Release build in `/tmp/sp-build-phase4-debt` (own build dir):
  **clean, exit 0, 0 errors, all targets** (including other slices'
  `sp_lsp_server` / debugger / spm targets -- the tree builds again
  after the other slice fixed its `LspServer.cpp` errors; this slice
  did not touch that code).
- Full ctest (`ctest -j2 --timeout 120`): **641/650 pass (99%)**,
  9 failed. First full run was 640/650 (LinkerDriverTest flaked);
  after the D3 follow-up fix it is 641/650.
- Slice D's own tests all pass: time 2/2, ObjectEmitter 5/5 (6/6
  `-j2` runs), LinkerDriver + DebugInfo + ObjectEmitter 13/13 (5/5
  `-j2` runs), PG marshalling 1/1 (+ neighbors 3/3), WssTest 2/2,
  ComptimeEvaluatorTest 12/12, TlsTest 8/8.
- Remaining 9 failures, each accounted for:
  - `NetTest.UdpSocketSendRecv` -- environmental: this VM's sandbox
    returns `EPERM` for UDP `sendto` (proven by strace + independent
    Python probe). Fails fast in 0.01 s now; passes where UDP loopback
    is permitted.
  - `JsonTest.BenchmarkOneGigabytePerSecThroughput` -- environmental
    CPU contention: fails at load average 5.43 on 1.5 shared CPUs
    (308 MB/s vs 500 required), **passes with the same binary when
    the machine is quieter** (679 MB/s vs 500; 52.6 MB/s vs 25). No
    JSON code changed in this slice. The test was not weakened.
  - `SplsPhase17Test.InlayTypeHintsInfersOmittedTypes` (SEGFAULT),
    `LspDepthTest.SemanticTokensBasic`, `DapServerTest.*` x5
    (StandardFlowStopsAtBreakpoint, LoopIterationValues,
    SetVariableWorks, TryCatchCatchesThrow,
    SecondLaunchInSameSession) -- **other Phase 4 slices'
    in-progress work** (LSP depth / debugger). Re-run confirms they
    fail consistently (6/18 in their subset). Not touched by this
    slice; the owning slices must fix them.
- Environment notes: a VM replacement during this slice wiped `/tmp`
  (build dir + Clang). Clang 18.1.3 was reinstalled; the repo tree
  (`~/workspace`) survived intact. The full build at the start of the
  slice failed only on another slice's half-finished `LspServer.cpp`
  (4 errors: `s.kind` on a pointer, `searchTextForSymbols` arity
  mismatch x2, unmatched definition); that slice fixed it during the
  slice and this slice did not touch it. The repo `AGENTS.md`
  Graphify protocol (`python -m graphify update . --force`) cannot
  be run (`graphify-out/` missing, module not installed) -- not
  fabricated.

### Deliberately not done

- No live-Postgres proof: no Postgres server exists in this
  environment, so the PG proof is IR + C-ABI-against-stub (D4),
  stated honestly. A real-server round-trip still needs a Postgres
  instance (Shanto's PC or CI).
- No `sp_rt_pg_*` compiler special-casing: the stdlib-level
  marshalling (D4) fixes the ABI with no compiler change; a
  `visit(CallExpr)` special case was considered and rejected as
  unnecessary.
- No fix for the other slices' failing tests (SplsPhase17 SEGFAULT,
  LspDepth, DapServer x5) and no fix for the JsonTest benchmark
  thresholds (environmental, not a code defect).
- No `Time.sleepMillis` / `nowMillis` aliases added: the docs were
  corrected to the real API instead of adding API surface to match
  wrong docs.
- No comptime statement-block support (`comptime { for...; }`): the
  docs were corrected to the supported subset instead of building a
  new evaluator feature in a verification-debt slice.
- No version bump (stays 3.1.0), no `.github/` changes, no merge to
  main, no ADR approval invented.

---

## Slice C — Debugger depth (real DAP session over a real interpreter)

### What the live session showed before any fix

Baseline: pristine `af13f06` `spd`, driven over stdio with a small Python
DAP client (`/tmp/sp-p4-dbg/dap_probe.py`, `dap_probe2.py`; durable copies
in `~/workspace/sp-language-review/phase4-slice-c/`). Probe program
`probe.sp`: `fn add(int a, int b) -> int`, `fn main` with `x=10`, `y=20`,
`total=add(x,y)`, `name="Shanto"`, `flag=true`, `pi=3.5`, a `for` loop
summing `0..2` into `sum`, two `println`s. `spc run` prints `30` then `3`
(exit 0) — the reference values.

| Step | Observed (before) |
| --- | --- |
| launch → setBreakpoints → configurationDone | Program ran to completion at launch, before any breakpoint was set; no `stopped` event ever |
| setBreakpoints blank line 4 | `verified=true`, silently moved to line 5 |
| setBreakpoints `}` line 18 | `verified=true` at 18 |
| setBreakpoints line 99 (past EOF) | `verified=true` at 99 |
| stackTrace after launch | line 17, all variables "already set" |
| variables | `total="add(x, y)"`, `sum="sum + i"` (values regex-guessed from source text) |
| output events | none, for either `println` |
| stepIn at a call | landed on line 1 (`fn add` definition), not the first body statement |
| stepOut from `add` | landed on line 2 (still inside `add`, not the caller line) |
| evaluate `x + y` / `total` / `nosuchvar` | echoed the expression back as its own value in all three cases |
| loop stop at line 14 | `sum="sum + i"`, no `i` variable at all (`for`-init invisible to the regex) |
| launch of a missing file | "succeeded", then immediately terminated |
| `setVariable` | advertised in `initialize` capabilities, no handler at all |

Root cause: `DebugSession` was a line-walking simulator, not a debugger.
It never executed the program: it walked "executable lines" (a heuristic
— non-empty, non-comment, non-`{`/`}` lines), regex-guessed variable
values from source text, ran loops once, never bound call parameters, and
`evaluate` echoed its input. There is no interpreter anywhere in the repo
(only `ComptimeEvaluator`, a compile-time expression interpreter), so a
real one had to be built for the debugger to be real.

### What was built

- **`tools/debugger/Interpreter.h` / `Interpreter.cpp` (new, ~1,900 lines)**
  — a tree-walking interpreter on the compiler's own AST, running the
  same front-end pipeline as `spc`: Lexer → Parser →
  `ModuleLoader::loadAndMergeAll` → `ComptimeEvaluator::processProgram` →
  `SemanticAnalyzer::analyze`. Variables are real (computed values),
  loops really iterate, calls really bind parameters and return, with a
  real call stack. Value model: Void/Int/Float/Bool/String/Char/Array/
  Map/Null/Range, with `display()` (strings quoted) and `typeName()`.
  Control flow via exceptions (`InterpReturn`/`InterpBreak`/`InterpContinue`/
  `InterpThrow`/`InterpPause`/`InterpError`, the last a loud runtime error
  with line, never a silent wrong value).
  - **Pause/resume is replay-based**: on pause, snapshot interpreter
    state and unwind the C++ stack; on resume, re-execute from the entry,
    fast-forwarding (no pauses, no output) to the paused statement, then
    continue in the requested mode. Consequence, stated honestly:
    programs must be deterministic across replays (no time/random/input).
    `println` output is captured and re-emitted exactly once (replay
    output discarded). Program output never goes to the DAP stdio stdout
    (it would corrupt framing) — it leaves as DAP `output` events.
  - Pause snapshot is taken in `beforeStatement`, before the `InterpPause`
    unwind — scope/frame RAII guards pop during unwinding, so a snapshot
    taken in the catch would lose block/loop scopes and locals.
  - On termination the pause snapshot is cleared — otherwise evaluate/
    variables would silently answer from a stale snapshot after the
    program ended (found live by probe 2).
  - Top-level `let` declarations bind into module globals (visible from
    `main`), not the script frame's scopes.
  - `setVariable` overrides are re-applied at the pause point on resume,
    keyed by call-chain signature (`"main>add"`, plus `"<globals>"`).
  - `evaluateInFrame` runs with pauses disabled, output suppressed, and
    saved/restored frames/statement-serial/step count, so evaluation
    cannot disturb the resume point.
  - Frame ids are depth-based (1 = outermost), not sequential — replay
    would otherwise give the same logical frame a new id on every resume
    and break clients caching frame ids (found live by probe 2).
  - Guards: 1,000,000-step limit (infinite loops), 1,000-frame recursion
    limit. Frames are held by reference across nested calls, so
    `m_frames.reserve(kMaxCallDepth + 8)` is done before any push.
  - Supported subset (anything else = loud `InterpError`): let/const/var,
    assignment (+= etc.), if/else, while, do-while, for, for-in over
    arrays and int ranges (`..` → Range) and strings, return/break/
    continue, try/catch/finally + throw (scalar payloads, stringified),
    ternary, arrays, maps, indexing, string concat, `.length`, string
    methods (`upper`/`lower`/`trim`/`reverse`/`repeat`/`contains`/
    `startsWith`/`endsWith`/`isEmpty`/`indexOf`/`replace`/`substring`/
    `charAt`/`split`), array methods (`push`/`add`/`pop`/`removeLast`/
    `get`/`set`/`contains`/`indexOf`/`isEmpty`/`clear`/`removeAt`,
    `.length`/`.size`), map methods (`get`/`set`/`remove`/`clear`/
    `contains`/`containsKey`/`isEmpty`, `.length`/`.size`), builtins
    (println/print, assert, panic, len, math). Method names are the REAL
    SP API, verified against `compiler/src/sema/TypeChecker.cpp` — the
    first draft's `toUpper`/`toLower`/`keys()` aliases were rejected by
    `spc` (`E1003`) and removed.
  - `Interpreter::load` resets all run state: a second `launch` in the
    same session starts fresh instead of returning the first program's
    terminal state at once (bug found live by probe 3, see below).
  - Try/catch: the first catch clause always matches — the payload is
    always stringified, so there is no runtime type to filter on; the
    type name is a binding annotation only (same rule as the compiler's
    try/catch lowering, Phase 3 slice B). The first draft matched only
    `string`/`Error`/`any`, so `catch (Exception e)` never caught
    (bug found live by probe 3, see below).
- **`tools/debugger/DebugSession.h` / `DebugSession.cpp` (rewritten)** on
  top of the interpreter (same public API shape, plus `launch()`,
  `startExecution()`, `getLoadError()`, `getLastError()`, `drainOutput()`,
  `evaluate(frameId, expr, out, error)` returning bool).
- **`tools/debugger/DapServer.h` / `DapServer.cpp` (rewritten handlers)**:
  `handleLaunch` fails loudly when `loadProgram` fails (missing file now
  returns `success=false` with the load error); launch does NOT auto-run
  before `configurationDone` (execution begins at `configurationDone`;
  `stopOnEntry` pauses at the first statement immediately); `cwd`
  (resolve relative program path) and `noDebug` (run to completion
  ignoring breakpoints) honoured; captured output drained after each run
  slice as DAP `output` events (stdout); runtime errors sent as `output`
  events (stderr) with `exited` exitCode 1; `handleEvaluate` returns
  `success=false` for unevaluatable expressions (no more echo); unknown
  commands return `success=false` (was `success=true` with empty body);
  `setVariable` handler added (was advertised but unhandled).
  `handleMessage` turns a handler-set `m_lastError` into `success=false`
  + message.
- **Breakpoint verification (AST-based, per file)**: executable lines =
  statement start lines from the AST (function bodies + top-level
  statements; `fn` definition lines and `}` lines are NOT breakable).
  Requested line matching a statement start line → `verified=true`, line
  unchanged. Otherwise move to the next statement start line
  (`verified=true`, client sees the moved line — standard behavior). If
  no statement exists at/after the requested line → `verified=false`
  (loud/honest, instead of the old silent never-firing `verified=true`).
  Breakpoints are keyed by normalized absolute path
  (`std::filesystem::absolute().lexically_normal()`), so a breakpoint in
  one file never fires for a same-numbered line in another file.
  `setBreakpoints` before launch lazy-loads the program from the
  breakpoint file so it can be verified; if the file cannot load, every
  breakpoint is honestly `verified=false`.
- **`tools/CMakeLists.txt`** — one line: added `debugger/Interpreter.cpp`
  to `sp_debugger` only.

### What the live session shows after the fix

Probe 1 (standard flow) and probe 2 (breakpoints before launch +
`configurationDone`, then stepIn/stepOut/evaluate/continues), against the
final `spd`:

| Step | Observed (after) |
| --- | --- |
| setBreakpoints [4, 18, 99] | 4 → verified at moved line 6; 18 → `verified=false`; 99 → `verified=false` |
| setBreakpoints [8, 14, 16] | all verified at their lines |
| configurationDone | `stopped` event, reason `breakpoint`, stackTrace `main` at line 8 |
| Locals at line 8 | `x=10`, `y=20`, `total` absent (line 8 not yet executed) |
| stepIn at the call | line 2 in `add`, frames [add line 2, main line 8], `a=10`, `b=20` |
| stepOut | line 9 in `main`, `total=30` |
| evaluate `total` / `x + y` | `"30"` (int) / `"30"` |
| evaluate `nosuchvar` | `success=false` ("evaluate failed: Undefined variable 'nosuchvar'") |
| loop stops at line 14 | (i, sum) = (0, 0), (1, 0), (2, 1) |
| output events | `"30\n"` + `"3\n"`; `terminated` + `exited` exitCode 0 |

Probe 3 (arrays/maps/try-catch + runtime error + missing file, against the
final `spd`, after the method-name fixes) found two more real bugs live,
both fixed and re-verified in the same session:

1. **Session reuse**: after the divzero program terminated, launching
   `probe2.sp` in the same session returned the first program's terminal
   state at once (no stop at the breakpoint). Cause: `Interpreter::load`
   did not reset run state (`m_started`/`m_terminated`). Fixed by
   resetting all run state in `load()`.
2. **`catch (Exception e)` never caught**: continuing from the `throw`
   line produced `Runtime error: Uncaught throw: boom` (exitCode 1)
   instead of running the catch body. Cause: the catch matcher only
   accepted `string`/`Error`/`any`. Fixed: first catch always wins
   (see above).

After both fixes, probe 3 (fresh session and re-used session) shows, at
the breakpoint on line 16 (`throw "boom";`): `total=60`, `first=10`,
`v=1`, `n=5`, `has=true`, `up="HELLO"`, `r=0`, `s="hello"`,
`nums=[10, 20, 30]`, `m={"a": 1, "b": 2}`; map children expand to `a=1`,
`b=2`; array children expand to `[0]=10`, `[1]=20`, `[2]=30`; evaluate
`nums[1] + v` → `21`; continue → output events `60`, `10`, `1`, `5`,
`true`, `HELLO`, `7` — exactly `spc run`'s output (the catch ran,
`r=7`). Missing-file launch → `success=false`, message "launch failed:
Cannot open program file '...'". Division by zero → stderr output event
"Runtime error: Division by zero", `terminated`, `exited` exitCode 1.
`probe2.sp` initially failed launch with real diagnostics (parse error
for `catch (e)`; sema `E1001` for `int? + int?`; sema `E1003` for
`toUpper`) — honest loud failures, same as `spc`.

### Tests (`tests/debugger/DapServerTest.cpp`, 17 TESTs, all passing)

No new test file, so no `tests/CMakeLists.txt` change. 6 existing tests
updated (stopOnEntry now pauses at line 2, the first executable statement,
not line 1; launch alone runs nothing — `configurationDone` added;
`ScopesVariablesAndEvaluation` restructured so `name` is asserted absent
before its line executes), plus 11 new tests:

| Test | Covers |
| --- | --- |
| `LaunchMissingProgramFails` | missing file → `success=false` + message |
| `LoadInvalidProgramFails` | `let x =;` → load false + non-empty error |
| `BreakpointVerification` | line 4 → verified line 6; lines 18, 99 → `verified=false`; line 8 → verified |
| `StandardFlowStopsAtBreakpoint` | full DAP flow: launch → setBreakpoints [8] → configurationDone → paused at 8, x=10, y=20, total absent; stepIn → line 2 in `add`, frames [add, main], a=10, b=20; stepOut → line 9, total=30; continue → terminated; output {"30", "3"} |
| `LoopIterationValues` | bp at 14; three stops (i, sum) = (0,0), (1,0), (2,1); output {"30", "3"} |
| `EvaluateRealExpressions` | `x + y` → 30, `total` → 30, `nosuchvar` → `success=false` |
| `RuntimeErrorIsLoud` | `10 / 0` → terminated, last error contains "Division by zero" |
| `SetVariableWorks` | pause at 8, setVariable x=99 → variables show 99; continue → output {"119", "3"} (override survives replay) |
| `UnknownCommandFails` | `frobnicate` → `success=false` |
| `TryCatchCatchesThrow` | `throw "boom"` caught by `catch (Exception e)`, `println(r)` → "7" |
| `SecondLaunchInSameSession` | divzero terminates, then a second launch in the same session stops at its breakpoint (line 8) and outputs {"30", "3"} |

Test-side bug found by the isolated run (not a product bug): the 5
output-asserting tests first read `session.drainOutput()`, but the
server drains the session's pending output into `output` events after
every run slice, so the session was already empty and the tests failed
with 0 lines. Fixed with a `stdoutOutputEvents(wire)` helper that reads
the stdout `output` events off the `out` wire stream (stderr filtered
out by category). This was the "DapServer x5" failure Slice D's section
reports — test-side, fixed here, not a product defect.

### Verification

- Shared tree does **not** build in full: another slice's half-finished
  edit breaks `tools/spls/LspServer.cpp` (4 errors: `LspServer.cpp:560`
  `s.kind` on a `const Symbol*`; `:888,905` `searchTextForSymbols` called
  with 5 args, candidate expects 4; `:1151` definition (5 params) does
  not match declaration in `LspServer.h:83` (4 params)). Slice C did not
  touch it and did not fix it. `spd` + `spc` build clean in the shared
  build dir (`/tmp/sp-build-phase4-dbg`); no errors, no warnings in the
  final builds.
- **Isolated full verification**: worktree at `af13f06` + only slice C's
  7 files (`tools/CMakeLists.txt` reverted to HEAD + only the
  Interpreter.cpp line), build dir `/tmp/sp-isolated-build`. Full Release
  build clean (first attempt failed on `ar: unable to copy file
  '../lib/libspm_core.a'` — `/tmp` was 100% full at 512M tmpfs, not a
  code error; baseline worktree removed, retry clean).
- `ctest -E "NetTest.UdpSocketSendRecv" -j2`: **610/614 passed, 4 failed**
  — `ExecutionTest.StdlibTimeNativeExecution`,
  `ExecutionTest.StdlibFullIntegrationNativeExecution` (2 stdlib-time
  epoch-ms, pre-existing), `JsonTest.BenchmarkOneGigabytePerSecThroughput`
  (pre-existing), `ObjectEmitterTest.EmitsNativeObjectFromIRText`
  (flaky under parallel ctest, **passes alone** — pre-existing, same as
  Phase 3). Effective: **611/614**, 3 pre-existing failures.
- `NetTest.UdpSocketSendRecv` was excluded: it hangs (22+ minutes, no
  output) — pre-existing, same as Phase 3. The first full `ctest` run
  (615 tests) was stopped after 606 passed / 8 failed with only that
  test still running; the 5 debugger failures in that run were the
  test-side `drainOutput` bug above, fixed since.
- `DapServerTest.*` run directly: **17/17 passed**.
- Python DAP probes re-run against the final binary (shared `spd` and
  isolated `spd`): probes 1, 2, 3 all pass; transcripts saved in
  `~/workspace/sp-language-review/phase4-slice-c/` (`after_transcript.txt`,
  `after2_transcript.txt`, `after3_transcript.txt`,
  `after3_isolated.txt`, plus the two baseline transcripts and the three
  probe scripts).
- `spc run` on both probe programs matches the interpreter exactly
  (`30`/`3` and `60`/`10`/`1`/`5`/`true`/`HELLO`/`7`).

### Deliberately not done

- No classes / `new`, no lambdas as values (so no `map`/`filter`/`reduce`
  on arrays), no async/await, no `expr?`, no switch/match, no I/O
  builtins (readln, fs), no extern "C" calls, no stdlib functions that
  need the native runtime, no fn-pointer values, no double/array/struct
  Task payloads — the interpreter fails loudly (`InterpError`) on all of
  these instead of returning a wrong value.
- No real-time stepping: pause/resume is replay-based, so programs must
  be deterministic across replays (no time/random/input). A native
  breakpoint-patching debugger (compiled code + patched breakpoints) is
  a different, larger slice; the replay interpreter is the honest scope
  for this one.
- No fix for the other slices' code (LSP build errors, LSP test
  SEGFAULT, JSON benchmark thresholds, stdlib-time epoch-ms tests).
- No version bump (stays 3.1.0), no `.github/` changes, no merge to
  main, no commit/push from this slice.

## Slice B — LSP depth (real index, references, documentSymbol, semantic tokens)

LSP depth for `tools/spls/` (the spls server) + `editors/vscode/` wiring.
Worked uncommitted on the shared tree; no commit/push/merge from this
slice. No version bump (stays 3.1.0), no `.github/` changes.

### Method

A real LSP session was driven over stdio with a small Python JSON-RPC
client (`/tmp/sp-p4-lsp-probe.py`, Content-Length framing, reader thread)
against the built `spls` binary — every capability below was proven by
that session, not just by unit tests. Probe program (`file:///tmp/lsp-probe/main.sp`,
0-based lines): `import std.io;` (0), `/// Adds two numbers` (2),
`fn add(a: int, b: int) -> int` (3), `class Greeter` (7) with
`public fn greet(name: string) -> string` (8), `fn helper() -> int` (13)
with `let count = 5; return count;`, `fn main()` (18) with
`let total = add(2, 3);` (19), `let g = new Greeter();` (20),
`let msg = g.greet("World");` (21), `let count = 0; count = count + 1;`
(22–23), `let bad: int = "oops";` (24, deliberate type error),
`println(total);` (25).

### Baseline (Phase 3 binary, before any slice B change)

| Capability | Observed |
|---|---|
| initialize | advertised completion/hover/definition/formatting/rename/workspaceSymbol/inlayHint; `spls 3.1.0` |
| publishDiagnostics on didOpen | REAL — one E1001 `Type mismatch: cannot initialize variable of type 'int' with value of type 'string'` at line 24; `import std.io` produced no spurious diagnostic |
| completion @ (19,10) | 52 items, unfiltered (same 52 everywhere) |
| completion @ (21,16) after `g.` | 52 items, includes `greet` — no member-context filtering |
| hover | works for top-level fn/method/class (with doc comment); NULL for locals (`total`, `count`) |
| definition | works for fn/method/class; NULL for locals |
| references / documentSymbol / semanticTokens / documentHighlight | `null` (not implemented, not advertised) |
| formatting | 1 edit, but CORRUPTS string literals: `return "Hello, " + name;` became `return Hello, + name;` (quotes stripped) |
| rename `count` @ main → `total2` | 5 edits (WRONG) — helper's `count` (lines 14–15) included; no scope awareness |
| rename `add` @ def | 2 edits (def + use) — correct |
| workspaceSymbol | works — "add" → 1, "Gre" → 2, "" → 11 (regex line scan, open docs only) |
| inlayHint full range | 5 hints — `: int` correct for literal inits, `: any` ×3 for call results (`add(2,3)`, `new Greeter()`, `g.greet(...)`) |
| didChange full / incremental | works — fixing the error → `[]`; reintroducing it → E1001 again |
| shutdown → exit | client got a broken pipe (server exited on `shutdown`, before `exit`) |

### What was built

1. **AST symbol index** — new `tools/spls/SymbolIndex.h/.cpp`
   (~1,170 lines), added to the `sp_lsp_server` library. Built
   per-request from source (Lexer → Parser → SemanticAnalyzer,
   diagnostics ignored): a scope tree (top-level, per-function,
   per-block), symbols (Function/Method/Class/Interface/Struct/Enum/
   EnumVariant/Variable/Parameter/Field with name, kind, type,
   signature, 1-based line/col/endLine, scope, parent class, doc
   comment) and uses (name, position, scope, member flag, receiver
   expr). Resolution walks the scope chain (last decl with line ≤ use
   line; top-level ignores line order for forward refs); member uses
   resolve via the sema type of the receiver, with fallbacks
   (identifier's own decl type; unique member name across classes).
   Powers hover/definition for locals+params+members, scope-aware
   rename, references, documentSymbol, semantic tokens, real-type
   inlay hints, and member-context completion.
2. **New providers** — `textDocument/references`,
   `textDocument/documentSymbol` (top-level + class members only,
   gopls/rust-analyzer convention — no locals), `textDocument/
   semanticTokens/full` (lexer + index, legend advertised in
   `initialize`), all dispatched in `LspServer.cpp` (+ `DocumentSymbol`
   struct in `Protocol.h`).
3. **Index-first hover/definition/completion/inlayHint/rename** —
   hover/definition resolve via the index (DocGenerator fallback kept,
   so `///` doc comments still show); completion after `.` offers only
   the receiver's members, elsewhere the symbols visible at the cursor
   (locals/params included); inlay hints use sema types with a regex
   fallback only when the index is invalid; rename is scope-aware
   (token scan only as fallback for unresolved names).
4. **Workspace indexing** — `initialize` stores rootUri/
   workspaceFolders and indexes `*.sp` under the root (bounded: skips
   dot-dirs/`node_modules`/`.git`/`target`/`build`, cap 500 files);
   `workspace/didChangeWatchedFiles` updates the cache;
   `workspace/symbol` searches open docs (index) + workspace cache
   (index, regex fallback), deduplicated.
5. **Formatter string-literal fix** — root cause: the lexer returns
   string/char lexemes DECODED (quotes stripped, escapes processed)
   and the formatter emitted `tok.lexeme` verbatim. Fix in
   `tools/fmt/Formatter.cpp`: re-quote StringLiteral/CharLiteral at
   emit time (`escapeForDoubleQuoted`/`escapeForChar` helpers,
   `\`/`"`/control chars escaped, non-ASCII bytes passed through for
   UTF-8). The lexer is untouched (the compiler relies on the decoded
   lexeme).
6. **VS Code wiring** — `sp.format.onSave` defaulted to `true` in
   `package.json` but the extension never implemented it (the setting
   did nothing, and its description said "using spc fmt" while the
   `sp.format` command ran the LSP formatter). Fix in
   `editors/vscode/src/extension.ts`: an `onWillSaveTextDocument`
   handler honoring the setting via
   `vscode.executeFormatDocumentProvider` (LSP formatter, same engine
   as `spc fmt`); setting description corrected. The server
   command/path wiring (`sp.languageServer.path`, default
   `/usr/local/bin/spls` / `spls` on PATH) was verified correct and
   left alone. Not packaged/run inside VS Code here.

### Bugs found by the live probe and fixed (all in this slice)

1. **Shutdown/exit broken pipe (pre-existing).** The server exited
   immediately on `shutdown` (`m_running=false`), so a spec-compliant
   client sending `exit` afterwards got a broken pipe. Fix: `shutdown`
   only sets a flag; `exit` stops the loop; post-shutdown requests get
   JSON-RPC error -32600. Probe now exits 0.
2. **Declaration locations point at the keyword, not the name.**
   `Parser::parseFunctionDecl` uses the `fn` token's location, so
   definition ranges covered `fn` and `resolveAt` missed def sites.
   Fix: `fixNameCol()` moves each decl's column onto its name via word
   search (functions, classes, interfaces, structs, enums, fields);
   doc comments are captured separately (`docCommentAbove()`), so hover
   at a def site resolves via the index AND keeps its doc comment.
3. **documentSymbol leaks.** Methods appeared both as class children
   AND top-level, and method parameters leaked as class children. Fix:
   top-level loop skips symbols with a non-empty `parent`; children
   loop only includes Method/Field/EnumVariant.
4. **Hover range pointed at the declaration, not the hovered use.**
   Fix: `SymbolIndex::rangeAt()`; hover uses the use-site range.
5. **Use-after-move killed all inlay hints.** The `let` hint was
   recorded AFTER `addSymbol(std::move(sym))`, which pilfers
   `sym.type`/`sym.name`, so every hint type was empty and no hint was
   ever pushed. Fix: record the hint BEFORE the move.
6. **Segfault in `SymbolIndex::walkExpr` on `fn test()` (reported by
   slice A as `SplsPhase17Test.InlayTypeHintsInfersOmittedTypes`).**
   Root cause, found with a -O0 reproducer + gdb: the parser's error
   recovery on a keyword used as a function name builds a garbage
   program — a nameless `FunctionDecl`, a phantom
   `__sp_test_anonymous_test` test decl, and `ExprStmt` nodes whose
   expression is null — and `ExprStmt::getExpr()` is `*m_expr`, so the
   walker dereferenced null. `parseExpression()` returns `nullptr` for
   any invalid prefix, so null children can appear at ANY expression
   position after error recovery (the parser constructs
   `BinaryExpr`/`UnaryExpr`/`AssignExpr`/`TernaryExpr`/`IndexExpr`/
   `AwaitExpr`/`CallExpr` args/`ArrayExpr` elements etc. without
   null-checking). Fixes, all in this slice:
   - 19 one-line `hasX()` const accessors added to
     `compiler/include/sp/ast/Statement.h` (+37) and
     `compiler/include/sp/ast/Expression.h` (+19), following the
     existing `hasInitializer()`/`hasElseBranch()`/`hasFinally()`/
     `hasExpr()` convention; every unguarded walker dereference in
     `walkStmt`/`walkExpr` is now guarded (vectors and `ReturnStmt`/
     `ForStmt` pointer getters were already guarded).
   - `SymbolIndex::build()` no longer runs sema on a program with
     parse errors: `TypeChecker` itself dereferences null children
     (`visit(IfStmt)` → `checkExpr` on a null condition segfaults —
     reproduced with `fn main() { if ; }`), and `spc` never hits this
     because it bails on parse errors before sema. The index does the
     same; `m_sema` is reset first so a stale analyzer (whose node
     map points into the previous, destroyed program) can never be
     used. Without sema the index falls back to structural inference
     (literals, `new`, calls), so hints keep working on broken code.
   - `inferType()` no longer trusts sema's Error type: `getNodeType()`
     returns the Error type for nodes sema never visited, whose
     `toString()` is `"<type-error>"` — the old string comparison
     checked for `"error"` and leaked it into hint labels. Now the
     TYPE KIND is checked (`isError()`/`isUnknown()`), falling through
     to structural inference.
   - Regression tests: `LspDepthTest.InlayHintOnKeywordNamedFunction`
     (the exact crashing program; asserts the 4 real-type hints and no
     `type-error` label) and
     `LspDepthTest.MalformedProgramsDoNotCrashIndex` (15 broken
     programs — null if/while condition, null binary RHS, null unary
     operand, null ternary branch, null index, null await operand,
     null throw, null assign value, null switch-case value, null
     match-arm body, null for-in iterable, null lambda body, null
     call arg, null array element — each must answer, not segfault).
7. **Semantic-token string length wrong.** The lexer returns decoded
   string lexemes, so `len = lexeme.size()` gave 7 for `"Hello, "`
   (9 source chars) and 3 for `"a\nb"` (6 source chars) — editors
   would highlight the wrong range. Fix in `handleSemanticTokens`:
   string/char token lengths are measured from the source text
   (opening quote to matching closing quote, honoring `\` escapes),
   falling back to the lexeme length for unterminated literals.
   Regression test: `SemanticTokensStringLengthCountsSourceSpan`.

### Found but NOT fixed here (for the parent / a later slice)

- **Parser error recovery builds phantom declarations.** `fn test(): void { ... }`
  produces FIVE top-level declarations: a nameless `FunctionDecl`, a
  phantom `__sp_test_anonymous_test` `FunctionDecl` (from
  `parseTestDecl` firing on the leftover `test` token), two `ExprStmt`s
  with null expressions (from the leftover `: void`), and the `{ ... }`
  block as a top-level `BlockStmt`. `spc` reports E1000 errors cleanly
  and refuses to compile, so this is not a compiler crash — but the
  recovery is noisy and the phantom decls are real AST nodes any tool
  walking a failed parse will see. Fixing `parseFunctionDecl`/
  `parseTestDecl` recovery is compiler-core work, out of this slice.
- **TypeChecker is not null-safe.** Any tool that runs
  `SemanticAnalyzer::analyze()` on a program with parse errors can
  segfault (proven: `fn main() { if ; }` → `visit(IfStmt)` →
  `checkExpr(null)`). `spc` is safe because it bails first. A
  compiler-side audit (every `visit` method guarding null children)
  is a separate slice; this slice's index works around it by skipping
  sema on parse errors (see bug 6 above).

### After-probe results (final binary, probe exit 0, log `~/workspace/tmp/sp-p4lsp-probe-final3.log`)

| Capability | After |
|---|---|
| initialize | + `referencesProvider`, `documentSymbolProvider`, `semanticTokensProvider` (full, with legend). `spls 3.1.0` |
| completion @ (19,10) | 53 items — keywords/types + visible locals (`total`, `g`, `msg`) |
| completion @ (21,16) after `g.` | **1 item: `greet` only** (was 52 unfiltered) |
| hover `total` @ (25,13) | `let total: int`, use-site range (was null) |
| hover `greet` @ (21,17) | `fn greet(name: string) -> string`, `method \`greet\` of \`Greeter\`` |
| hover `add` @ def (3,5) | index format + `Adds two numbers` doc comment, range (3,3)–(3,6) |
| definition `add` @ (19,17) | (3,3)–(3,6) — the name (was (3,0)–(3,3), covering `fn`) |
| definition `total` | (19,8)–(19,13) |
| references `add` @ (19,17) | 2 refs (decl + use) (was null) |
| documentSymbol | 4 top-level (`add`, `Greeter`, `helper`, `main`); `Greeter` has exactly 1 child (`greet`) (was null) |
| semanticTokens/full | 120 tokens; `fn` → keyword (13), `add` → function (11) + declaration bit, `a`/`b` → parameter (7) + declaration, `int` → type (1), `Greeter` → class (2) + declaration, `greet` → method (12) + declaration, `"Hello, "` → string (15), `///` → comment (14) + documentation modifier (16). String lengths now count the source span (was null) |
| documentHighlight | still null — not implemented (see deliberately-not-done) |
| formatting | quotes preserved — `return "Hello, " + name;` survives (was `return Hello, + name;`) |
| rename `count` @ main → `total2` | **3 edits, lines 22–23 only** (was 5 incl. helper's `count`) |
| rename `add` @ def → `sum` | 2 edits |
| workspaceSymbol | "add" → 1, "Gre" → 2, "" → 14 (was 11; now includes params/locals with correct name spans) |
| inlayHint full range | **5 hints, real types, no `: any`**: `: int` (lines 14, 19, 22), `: Greeter` (20), `: string` (21) (was `[]` after the use-after-move regression; baseline had 5 hints with `: any` ×3) |
| didChange / diagnostics | unchanged, correct (fix → `[]`, reintroduce → E1001 at line 24) |
| shutdown → exit | probe exits 0 (was BrokenPipeError) |

### Tests

| Test | Result |
|---|---|
| `LspDepthTest.*` (18 tests: hover/definition locals+params+methods, scope-aware references/rename, documentSymbol, semantic tokens incl. escaped-string span, member completion, locals completion, real-type inlay hints, initialize providers, workspace symbol incl. watched-file delete, keyword-named-function segfault regression, malformed-program sweep) | 18/18 |
| `SplsPhase17Test.*` (incl. `InlayTypeHintsInfersOmittedTypes` — the reported segfault) | all pass |
| `LspServerTest.*` | all pass |
| `FormatterTest.*` (incl. 3 new: StringLiteralQuotesPreserved, StringEscapesPreserved, CharLiteralQuotesPreserved) | all pass |
| LSP + formatter suites together | **37/37** |

### Verification

- Full Release build in `~/workspace/build-phase4-lsp` (own build dir;
  the original `/tmp/sp-build-phase4-lsp` was abandoned — /tmp is a
  512M tmpfs and filled up from the other slices' build dirs plus the
  parent's, failing the `sp_tests` link with "No space left on device";
  the home fs has 147G free; other slices' dirs were NOT deleted):
  clean, 0 errors (`spls`, `sp_tests`, `spc` targets).
- Full ctest (`-j2 --timeout 120`): **653/655 (99%)**. Only failures:
  `NetTest.UdpSocketSendRecv` (pre-existing hang, Phase 3) and
  `JsonTest.BenchmarkOneGigabytePerSecThroughput` (147 MB/s vs 1 GB/s
  on the throttled VM — also reported by slice A). The 3 LSP failures
  slice A reported are all fixed; the DAP failures slice A reported
  are fixed by slice C.
- Python LSP probe re-run against the final binary: exit 0, all
  capabilities as in the after-table above
  (`~/workspace/tmp/sp-p4lsp-probe-final3.log`, 634 lines). The
  segfault reproducers (`/tmp/repro-inlay.cpp`,
  `/tmp/repro-malformed.cpp`, `/tmp/dump-tokens.cpp`) are throwaway
  /tmp scratch, not committed.
- `spc build` on both segfault reprograms (`fn test(): void {...}`
  and `fn main() { if ; }`) reports E1000 errors cleanly, no crash —
  confirming the compiler driver is safe and the crash was specific
  to running sema on a failed parse.

### Files changed (this slice only)

- New: `tools/spls/SymbolIndex.h` (172 lines), `tools/spls/SymbolIndex.cpp`
  (997 lines), `tests/spls/LspDepthTest.cpp` (421 lines, 18 tests).
- Modified: `tools/spls/LspServer.cpp` (+676/−47),
  `tools/spls/LspServer.h` (+30), `tools/spls/Protocol.h` (+27),
  `tools/fmt/Formatter.cpp` (+64/−1), `tools/CMakeLists.txt`
  (SymbolIndex.cpp in `sp_lsp_server`), `tests/CMakeLists.txt`
  (LspDepthTest registration), `tests/fmt/FormatterTest.cpp` (+27),
  `compiler/include/sp/ast/Statement.h` (+37),
  `compiler/include/sp/ast/Expression.h` (+19),
  `editors/vscode/src/extension.ts` (+25),
  `editors/vscode/package.json` (1 line — setting description).
- 11 tracked files +913/−49 plus the 3 new files above.

### Deliberately not done

- `documentHighlight` not implemented (not advertised either).
- Imports unresolved in semantic tokens (`std` stays variable).
- No `range` semantic tokens (only `full`).
- Inlay hints only for `let` without annotation.
- Workspace scan is one-time at initialize + file-event updates; no
  multi-root beyond the first folder.
- VS Code extension not packaged/run inside VS Code here (no VS Code
  binary in this environment); the `onWillSaveTextDocument` fix uses
  only the documented `vscode.executeFormatDocumentProvider` API.
- No parser-recovery fix and no TypeChecker null-safety audit (see
  "Found but NOT fixed" — compiler-core work for a later slice).
- No ADR-001–005 decisions; no language semantics invented for the LSP.
- No version bump, no `.github/` changes, no merge to main, no
  commit/push from this slice.

## Parent verification + parent fix

> The parent (not a slice) verified the combined tree, fixed one Phase 3
> regression the slices exposed, and committed part-by-part. Nothing was
> merged to main; the branch was pushed once, at the end.

### P1. Phase 3 regression: `std.web` import broke every program that called `App::openApi()`

Slice A found it live: `stdlib/web/app.sp:271` (`App::openApi`) registered
a lambda that captured the `specJson` parameter. Capturing lambdas are
deliberately unsupported — they fail loudly with E1000 since Phase 3 — so
any program importing `std.web` and calling `openApi()` failed to build.
No test covered it (see P2 for why).

Fix (no `/tmp` workaround, no stdlib behaviour change): the spec is now
served by a new runtime static-JSON route. `sp_rt_http_server_route_json`
(`runtime/src/net/NetBridge.cpp`, declared in
`runtime/include/sp/runtime/Runtime.h`) registers a route whose handler
returns `HttpResponse::json(body)`; the body string is copied into the
route, so the caller's string may be freed afterwards. `HttpServer`
gained a `routeJson(method, path, json)` method (`stdlib/http/http.sp`,
extern decl alongside the other `sp_rt_http_server_*` decls), and
`App::openApi` now calls `this.server.routeJson("GET", path, specJson)`
— same endpoint, same body, no lambda at all. A `this`-based field lookup
was considered and rejected after reading the lowering: `visit(LambdaExpr)`
(`compiler/src/ir/IRBuilder.cpp`) swaps out all enclosing scopes and only
re-declares the lambda params, so `this` is not reachable inside a lambda.
(`swagger`/`redoc` do not capture — they use hardcoded strings — so they
were left untouched; note their `title`/`openApiJsonUrl` parameters are
ignored by the hardcoded HTML, a pre-existing quirk, out of scope.)

Tests: `WebTest.RouteJsonServesStaticDocument` (C++ level: 200, exact
body, `application/json; charset=utf-8`) and
`ExecutionTest.StdWebOpenApiNoCapturingLambda` (SP level: `import std.web`,
`new App()`, `app.openApi(...)`, prints `openapi_registered: true`).
Negative control (run by the parent, fix temporarily reverted): the new
SP test fails with exactly
`error[E1000]: 'lambda captures variable 'specJson''` at `app.sp:271`,
then passes again with the fix restored.

### P2. The execution-test harness never loaded stdlib imports

Proving P1 exposed a second, older gap: `runSPProgram`
(`tests/codegen/ExecutionTest.cpp`) went straight from Parser to
`SemanticAnalyzer` and never ran `ModuleLoader::loadAndMergeAll`, the
step `spc` performs before sema. So `import std.web` contributed
nothing in the harness — the SP test failed with
`error[E1002]: Undefined class 'App'` while the real compiler accepted
the same program (`spc check` + `spc build` + run all passed). That is
why the Phase 3 regression shipped uncaught: the harness could only
exercise builtins (`now()`, `List`/`Map`, `Duration` are all sema/IR
builtins; `std.time`/`std.io`/… are on the loader's own skip list).

Fix: `runSPProgram` now calls `loadAndMergeAll` exactly like `spc`
(before sema). Blast radius checked: the full `ExecutionTest` suite
passes **63/63** with the loader wired in, including the six
`std.collections` tests. Pure-builtin modules are still skipped by the
loader itself, so builtin-only tests are unaffected.

### P3. Final verification (combined tree, parent build dir `~/workspace/build-phase4-parent`)

- Fresh Release build: **clean, exit 0, 0 errors** (all targets).
- Full ctest (`-j2 --timeout 120`): **653/655 (99%)**. The only 2
  failures are the pre-existing environmental ones, both proven by
  slice D: `NetTest.UdpSocketSendRecv` (this sandbox returns EPERM for
  UDP `sendto`; fails fast in 0.01 s now) and
  `JsonTest.BenchmarkOneGigabytePerSecThroughput` (throughput assertion
  under CPU contention; the same binary passes when the machine is
  quieter). No test was weakened or skipped.
- Live proofs re-run by the parent against the final binaries:
  - **Registry round-trip** (fresh service built with the final `spc`,
    throwaway token, fresh dirs): `/api/health` ok → `spm publish`
    201 + tarball uploaded, `verified:true` (checksum
    `0fd716be…`, the real tarball SHA-256) → `spm add greet@^0.1.0`
    resolves + writes `sp.lock` → `spm install` vendors 1 package →
    `spm build` ok → run prints `Hello, world from greet!` →
    `spm verify` reports "1 package verified, 0 tampering detected" →
    registry `verify` endpoint `verified:true`.
  - **std.web**: `spc build` of a program calling `app.openApi(...)`
    succeeds with the final binary; run prints
    `openapi_registered: true`.
  - **LSP**: slice B's stdio probe (`/tmp/sp-p4-lsp-probe.py`) against
    the final `spls` runs every section to completion (completion,
    hover, definition, references, documentSymbol, semanticTokens,
    formatting, rename, workspaceSymbol, inlayHint, diagnostics);
    workspaceSymbol counts match slice B's report (1/2/14). The
    programmatic checks live in `LspDepthTest` (18 tests, in the
    passing ctest run).
  - **Debugger**: slice C's DAP probe (`dap_probe.py`) against the
    final `spd` exits 0; transcript shows breakpoints verified and
    `stopped` events with reason `breakpoint` (twice) plus stack
    frames. The programmatic checks live in `DapServerTest` (17/17,
    in the passing ctest run).

### P4. Commits (part-by-part, branch `feature/pro-4.0.0` only)

The combined diff was split mechanically (`~/workspace/split-p4.py`,
168 hunks, 0 unassigned) and applied in slice order, then the parent
fix (two commits: static-JSON route, then the harness loader +
regression test), then this doc:

1. Slice A — spm install from the real registry
2. Slice B — LSP depth (SymbolIndex, references, semantic tokens)
3. Slice C — real debugger (tree-walking interpreter, DAP rewrite)
4. Slice D — verification debt + docs sync
5. Parent fix — `std.web` openApi via static-JSON route
6. Parent fix — execution harness loads stdlib imports + openApi test
7. Doc — this file (Phase 33)

Push: single push of `feature/pro-4.0.0` at the end; remote `main`
still `d10e1c2`, never merged. Version stays **3.1.0**; `.github/`
untouched (GitHub Actions stay removed).

### Found but NOT fixed (follow-up candidates, not Phase 4 regressions)

- **TypeChecker is not null-safe on parse-error programs**: slice B
  proved `if ;` segfaults `analyze()` (null condition). The LSP index
  works around it by skipping sema when the parse has errors. A
  compiler-core audit is a separate slice's work; do not fold it into
  an ecosystem phase.
- **Parser error recovery builds phantom declarations**: `fn test()`
  yields 5 declarations (an unnamed fn, a phantom
  `__sp_test_anonymous_test`, 2 null `ExprStmt`s, a top-level block).
  `spc` refuses cleanly with E1000 and does not crash, but the
  recovery is noisy.
- No live-Postgres round-trip (no Postgres in this environment; the PG
  proof is IR + C-ABI-against-stub — slice D, stated honestly).
- The registry `splang-registry/` Next.js mock is untouched and still
  correctly labelled preview/mock.
