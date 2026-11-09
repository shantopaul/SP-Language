# Phase 36 (pro-4.0.0) — Slice B: HttpServer.get vs getHandler ABI alignment

- **Branch:** `feature/pro-4.0.0` (work only; never merge to `main`)
- **Base:** `328eee0` (Phase 6 / Phase 35 done, pushed)
- **Version:** 3.1.0, unchanged (no bump, no `.github/` changes, no ADR-001–005
  items touched, nothing committed or pushed)
- **Scope:** align `HttpServer.get/post/put/patch/delete/options/head` (and
  the same verbs on `RouteGroup`) with `App.get` — all of them now
  register **typed** handlers (`fn(HttpRequest) -> HttpResponse`, or a
  `(ptr req)` lambda) through `routeHandler`. The trap Phase 6 slice D
  found (§5.2) is gone.
- **Status:** fix done and proven both ways (segfault on the pre-fix
  binary, 200s on the fixed binary); 1 new test; full ctest **789/792
  (99%)** — the 3 failures are all attributed below, none caused by
  this slice.

## 1. What was wrong

Same method name, different ABI, per class:

| Class | `get(path, handler)` called | ABI | Handler signature |
|---|---|---|---|
| `App` (`stdlib/web/app.sp`) | `route()` → `server.routeHandler()` | typed | `fn(HttpRequest) -> HttpResponse` |
| `WebGroup` (`stdlib/web/app.sp`) | `route()` → `group.routeHandler()` | typed | same |
| `HttpServer` (`stdlib/http/http.sp`) | `route()` → `sp_rt_http_server_route` | **3-string** | `(method, path, body) -> const char*` |
| `RouteGroup` (`stdlib/http/http.sp`) | `sp_rt_http_group_route` | **3-string** | `(method, path, body) -> const char*` |

`getHandler` / `routeHandler` on both classes already used the typed
ABI, so the split was invisible until a typed handler went through
`get`.

**Why it segfaulted** (`runtime/src/net/NetBridge.cpp`). The 3-string
`sp_rt_http_server_route` invokes the handler as
`handler_fn(method, path, body)` — three `const char*` args — and
treats the return as a `const char*` string (`std::string resContent
= resultStr`). A typed `fn(HttpRequest) -> HttpResponse` expects ONE
pointer arg (the request object) and returns an object pointer. On
first request the handler read the method string as an `HttpRequest`
object (slot 1 = `handle` = garbage) and the runtime then read the
returned object pointer as a C string — segfault. The typed path
(`sp_rt_http_server_route_handler` → `callHandlerFn`, Phase 6 slice A)
wraps the request as a real SP `HttpRequest` object (slot 0 = vtable,
slot 1 = raw handle) and unwraps the returned `HttpResponse` via
`extractResponse`.

**Baseline proof (pre-fix binary `build-phase6-parent/bin/spc`,
port 18773).** The proof program (`~/workspace/p7b-proofs/server_get.sp`)
registers a typed `hello` via `HttpServer.get` plus `routeJson` routes:
```
curl /hello → (empty) HTTP 000
curl /      → (empty) HTTP 000
server log: Segmentation fault
server: DEAD
```
The first request killed the server — even the `routeJson` routes died
with it.

## 2. Caller audit (before changing anything)

Grep over `examples/`, `tests/`, `docs/` for `.get(` / `.post(` /
`.put(` / `.patch(` / `.deleteRoute(` / `.options(` / `.head(` /
`.route(` handler registrations:

- **Zero 3-arg handler lambdas anywhere** in `examples/`, `stdlib/`,
  `docs/` — no SP caller uses the 3-string ABI. No SP-level
  `.route(` calls at all.
- All SP handler registrations are 1-arg `(ptr req) => { let ctx =
  Context.create(req); ... }` lambdas (backend-crud `router.sp`,
  `web_app.sp`, `sp3_enterprise_showcase.sp`) or typed
  `fn(HttpRequest) -> HttpResponse` (guide §85). They run through
  `App.get` / `WebGroup.get` today — already the typed ABI — so the
  alignment changes nothing for them. (Under the typed ABI a
  `(ptr req)` handler receives the wrapped SP object and
  `Context.create` unwraps it via `sp_rt_http_req_unwrap`; that is
  the designed path since Phase 6 slice A.)
- `examples/router_middleware.sp` creates `RouteGroup`s via
  `server.group()` but registers **no routes** on them.
- `examples/http_server_v2.sp`, `examples/microservice.sp` register
  **no routes** at all (listen/stop only).
- **C++ level (unaffected — no runtime change):**
  `tests/runtime/HttpTest.cpp:641` calls `sp_rt_http_group_route`
  directly with a 3-string lambda; `OpenApiTest.cpp` uses C++
  `HttpServer::route`. Both are C++/runtime APIs, not the SP stdlib.
- **Docs:** only the §85 trap note mentioned the split.

**Conclusion: nothing depends on the 3-string behavior through
`get`.** The change is safe for every in-repo caller.

## 3. Fix

`stdlib/http/http.sp` only — no C++ change was needed (both ABIs
already exist in the runtime; the stdlib just picked the wrong one).

- `HttpServer.get/post/put/patch/deleteRoute/options/head`:
  `this.route(...)` → `this.routeHandler(...)`.
- `RouteGroup.get/post/put/patch/deleteRoute/options/head`:
  `sp_rt_http_group_route(...)` → `this.routeHandler(...)` (same
  trap one layer down; `WebGroup.get` was already typed).
- `getHandler/postHandler/...` unchanged (already `routeHandler`).
- `HttpServer.route` **kept** as the low-level 3-string entry point —
  it is the only way to register a `(method, path, body) ->
  const char*` handler from SP — with a comment stating the split:
  `route` = 3-string handlers; `get` / `routeHandler` / `getHandler`
  = typed handlers. Do not route a typed handler through `route()`.

**Guide (§85).** The trap note is gone. The example now registers
the typed handler with `app.get("/hello", hello)` (it used
`getHandler` to dodge the trap) and the comment says `getHandler`
remains as the explicit alias and `route()` is the low-level 3-string
API. The updated §85 example passes `spc check`. No other guide
section touched.

## 4. Tests

**New test** — `CodegenCrashersExecutionTest.HttpServerGetTypedHandler`
(`tests/codegen/ExecutionTest.cpp` was not touched; the test lives in
`tests/codegen/CodegenCrashersExecutionTest.cpp`, next to Phase 6-A's
`NamedFnHttpHandlerTypedRequest`, same shape: build the server exe,
start it, probe with curl, kill it). The SP program registers, on one
`HttpServer` (port 18772):

- typed `hello` via `HttpServer.get` (was: segfault),
- a `(ptr req)` lambda via `HttpServer.get` (backend-crud pattern),
- typed `hello` via `getHandler` (unchanged path),
- typed `hello` via `RouteGroup.get` (`app.group("/api")`),
- `routeJson` `/`.

The test curls `/hello`, `/echo`, `/hi2`, `/api/v1`, `/` (all 200 with
the expected bodies) and `/missing` (404), then kills the server.
**Passed (6.9 s)**; the pre-existing `NamedFnHttpHandlerTypedRequest`
(`App.get`) also passed (6.3 s). No new CMakeLists entry (test added
to an existing file).

## 5. Live proofs (fixed binary `build-phase7-http/bin/spc`, port 18773)

Same program as the baseline, `~/workspace/p7b-proofs/server_get.sp`:

```
curl /hello  (typed fn via HttpServer.get)    → hi typed                                  HTTP 200
curl /echo   ((ptr req) lambda via HttpServer.get) → {"echo":true}                        HTTP 200
curl /hi2    (typed fn via getHandler)        → hi typed                                  HTTP 200
curl /api/v1 (typed fn via RouteGroup.get)    → hi typed                                  HTTP 200
curl /       (routeJson)                      → {"message": "Welcome to SP Web Framework!"} HTTP 200
curl /missing                                →                                           HTTP 404
server: ALIVE afterwards
```

Pre-fix binary on the same program: `Segmentation fault` on the first
request, server dead (§1).

## 6. Verification

Own build dir `~/workspace/build-phase7-http` (Release, `-j2`, Clang
18.1.3, googletest bypass). Build clean, 0 errors. (One background
build session was lost mid-build at 30% with no error in the log and
no process left running; the build was restarted and completed
incrementally — no source change was involved.)

**Full ctest: 789/792 (99%)** — total 792 = 791 (Phase 6) + 1 new test.

| Test | Result | Cause |
|---|---|---|
| `NetTest.UdpSocketSendRecv` | Failed | Sandbox UDP EPERM (environmental, pre-existing) |
| `JsonTest.BenchmarkOneGigabytePerSecThroughput` | Failed | CPU contention: 229 vs 250 MB/s (validator), 11.08 vs 15 MB/s (DOM parser). **Proven pre-existing: fails identically on the pre-fix `build-phase6-parent` binary.** My change touches no JSON code (only `stdlib/http/http.sp`, the guide, one test file). |
| `ObservabilityTest.CronSchedulerIntervalExecution` | Failed in full run, **passed alone in 120 ms** | Timing flake under full-suite load (3 slices building/testing in parallel on shared CPUs). |

## 7. Files changed (all uncommitted in the worktree)

```
 docs/SP-Syntax-Guide.md                        | 12 ++-
 stdlib/http/http.sp                            | 49 ++++++---
 tests/codegen/CodegenCrashersExecutionTest.cpp | 133 +++++++++++++++++++++++++
 3 files changed, 175 insertions(+), 19 deletions(-)
```

## 8. Deliberately not done

- **No runtime change.** Both ABIs already exist in `NetBridge.cpp`;
  the fix is a stdlib routing choice.
- **`HttpServer.route` keeps the 3-string ABI** — it is the named
  low-level entry point; removing it would leave no way to register a
  3-string handler from SP. The guide + code comments now state the
  rule instead.
- **No `?.` method-call fix** (slice A's item) and no temp-release
  work (slice C) — out of this slice's scope.
- **Graphify not run:** the repo `AGENTS.md` mandates
  `python -m graphify update . --force`, but `graphify-out/` does
  not exist and `python -m graphify` is not installed (verified) —
  it cannot be run; nothing fabricated.
