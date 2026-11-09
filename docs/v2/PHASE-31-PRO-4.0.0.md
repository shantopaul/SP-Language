# Phase 31 — PRO 4.0.0: Phase 2, Slice A — Real TLS for HTTP

Branch: `feature/pro-4.0.0` (Phase 2 work; **not merged to main**).
Product version stays **3.1.0** — this is a capability slice, not a version bump.

Phase 1 made the codebase honest (no fake registry responses, no fake
numbers). Phase 2 is backend depth. This slice gives the HTTP server and
client real TLS via OpenSSL. Before this slice the HTTP layer was plaintext
only: `HttpClient::get/post` stripped only `http://` and an `https://` URL
would silently be treated as plaintext (or fail at connect). That is now
fixed: `https://` means TLS, with verification on by default.

## What was implemented

### 1. HTTP server TLS — `runtime/src/http/Http.cpp`, `runtime/include/sp/runtime/Http.h`

- `HttpServerConfig` gains three fields (defaults keep plaintext behaviour):
  - `tlsEnabled{false}` — set `true` for HTTPS.
  - `tlsCertFile` — PEM certificate chain file.
  - `tlsKeyFile` — PEM private key file.
- `HttpServer::listen()` builds a server `SSL_CTX` (`TLS_server_method`,
  TLS 1.2 minimum) when TLS is enabled, loads the cert chain + key, and
  verifies the key matches the cert (`SSL_CTX_check_private_key`).
- **Fail loudly, no silent plaintext fallback:** if TLS is enabled but the
  files are missing, unreadable, or mismatched, `listen()` returns `false`,
  writes the reason to stderr, and stores it in `getLastError()`. The
  server never starts in plaintext when TLS was requested.
- Each accepted connection performs `SSL_accept` on a worker thread before
  any HTTP parsing. A failed handshake closes that connection and logs the
  OpenSSL error; the server keeps serving other connections.
- A small internal `Connection` abstraction (`PlainConnection` /
  `TlsConnection`, in `Http.cpp` only) lets the existing keep-alive loop
  read/write identically over TCP and TLS (`SSL_read`/`SSL_write`, with
  `SSL_shutdown` on close). Plain HTTP is untouched when TLS is off.
- The TLS handshake and subsequent reads are bounded by
  `requestTimeoutMs` socket timeouts, so a client that connects but never
  speaks TLS cannot stall a worker thread forever. (Plaintext connections
  keep the previous no-timeout behaviour.)
- `HttpServer::isTlsEnabled()` reports whether the running server is TLS;
  `getLastError()` reports the last `listen()` failure.
- The `SSL_CTX` is shared across worker threads (safe in OpenSSL) and freed
  in `stop()` after all workers have joined.

### 2. HTTP client TLS — same files

- `HttpClient::get/post` now understand `https://` URLs (default port 443;
  `http://` stays port 80). The URL scheme decides TLS; an `https://` URL
  can no longer silently downgrade to plaintext.
- New `HttpClientOptions` + overloads (existing `timeoutMs` overloads are
  unchanged, so `spm` and `NetBridge` callers keep working):
  - `send(req, host, port, HttpClientOptions)`, `get(url, HttpClientOptions)`,
    `post(url, body, contentType, HttpClientOptions)`.
  - `useTls` — set automatically from the URL scheme.
  - `caFile` / `caPath` — extra CA bundle/directory trusted **in addition
    to** the system CA store (used by the tests with a local CA).
  - `insecureSkipVerify{false}` — explicit opt-in that skips chain AND
    hostname verification. Never the default; for local development against
    self-signed test servers only.
- Verification (default, `insecureSkipVerify == false`):
  - `SSL_VERIFY_PEER` + system CA store (`SSL_CTX_set_default_verify_paths`).
  - Hostname verification via `SSL_set1_host` (cert SAN, then CN) and SNI
    via `SSL_set_tlsext_host_name`.
  - A self-signed/untrusted cert or a hostname mismatch **fails the
    request**: `send()` returns a 500 `HttpResponse` whose body names the
    TLS failure (e.g. `TLS handshake failed ...` /
    `TLS certificate verification failed ...`). It never silently proceeds.
- TLS 1.2 minimum on the client as well; the handshake is bounded by
  `timeoutMs` socket timeouts.
- Request writes now loop until all bytes are written (previously a single
  `send()`), for both TLS and plaintext.

### 3. CMake

No change was needed: `runtime/CMakeLists.txt` already contains
`find_package(OpenSSL REQUIRED)` and links `OpenSSL::SSL` +
`OpenSSL::Crypto` to `sp_runtime` (added earlier for Postgres TLS), and
`sp_runtime` links it `PUBLIC`, so `sp_tests` and all HTTP users get it.
If OpenSSL is absent, configure already fails with a clear message rather
than silently building without TLS.

## Files changed

| File | Change |
|---|---|
| `runtime/include/sp/runtime/Http.h` | `HttpClientOptions`; `HttpClient` options overloads; TLS fields in `HttpServerConfig`; `getLastError()` / `isTlsEnabled()`; opaque `m_tlsCtx` + `m_lastError` members |
| `runtime/src/http/Http.cpp` | OpenSSL includes; `opensslErrorString`, `writeAllBytes`, `readUntilClose`, `parseClientUrl`, `Connection`/`PlainConnection`/`TlsConnection`; TLS client handshake + verification; TLS server context setup + per-connection handshake |
| `tests/runtime/TlsTest.cpp` | New: 8 TLS tests (see below) |
| `tests/CMakeLists.txt` | Registers `runtime/TlsTest.cpp` in `sp_tests` |
| `docs/v2/PHASE-31-PRO-4.0.0.md` | This file |

## How to configure TLS

Server (C++):

```cpp
sp::rt::http::HttpServerConfig cfg;
cfg.tlsEnabled = true;
cfg.tlsCertFile = "/etc/sp/server.crt";  // PEM chain
cfg.tlsKeyFile  = "/etc/sp/server.key";  // PEM private key
sp::rt::http::HttpServer server(cfg);
if (!server.listen("0.0.0.0", 8443)) {
    std::cerr << server.getLastError() << "\n";  // e.g. missing cert file
}
```

Client (C++):

```cpp
// https:// verifies against the system CA store by default.
auto res = sp::rt::http::HttpClient::get("https://api.example.com/v1/ping");

// Trust an extra (e.g. private) CA:
sp::rt::http::HttpClientOptions opts;
opts.caFile = "/etc/sp/ca.crt";
auto res2 = sp::rt::http::HttpClient::get("https://internal.example.com/", opts);

// Local dev only, never production:
sp::rt::http::HttpClientOptions dev;
dev.insecureSkipVerify = true;
auto res3 = sp::rt::http::HttpClient::get("https://127.0.0.1:8443/", dev);
```

Generate a test cert/key with the `openssl` CLI (as the tests do):

```sh
openssl req -x509 -newkey rsa:2048 -keyout server.key -out server.crt \
  -days 365 -nodes -subj "/CN=localhost" \
  -addext "subjectAltName=IP:127.0.0.1,DNS:localhost"
```

## Tests — `tests/runtime/TlsTest.cpp`

Certificates are generated at setup time with the `openssl` CLI (local CA,
CA-signed server cert with SAN `IP:127.0.0.1,DNS:localhost`, self-signed
cert, CA-signed cert for a different host). If the CLI is unavailable the
tests **skip explicitly** instead of faking a pass.

| Test | Proves |
|---|---|
| `TlsRoundTripWithTrustedCa` | TLS round-trip succeeds with a trusted CA |
| `TlsRejectsSelfSignedCert` | Client rejects self-signed cert (verification on) |
| `TlsRejectsHostnameMismatch` | Client rejects a CA-signed cert for the wrong host |
| `TlsInsecureSkipVerifyConnectsToSelfSigned` | The insecure opt-in works when explicitly requested |
| `TlsServerRejectsPlaintextClient` | TLS server never serves plaintext |
| `PlaintextHttpStillWorks` | Plain HTTP unchanged |
| `TlsServerRejectsMissingCertFile` | `listen()` fails loudly on missing cert |
| `TlsServerRejectsMismatchedKey` | `listen()` fails loudly on key/cert mismatch |

## Verification performed

- Fresh build in `/tmp/sp-build-tls-clean` from a clean worktree of Phase 1
  HEAD `e6e7230` + only this slice's files: `sp_tests` builds clean
  (exit 0, 0 errors). The shared working tree could NOT be built as-is:
  it currently also holds other Phase 2 slices' in-progress changes, and
  one of them (`PersistentQueue` in `runtime/src/obs/Queue.cpp`) does not
  compile (`nowMs()` called unqualified — it is a `JobQueue` static). That
  failure is in another slice's code, untouched here.
- **End-to-end TLS proof** — a standalone driver (`/tmp/tls-driver.cpp`,
  links the built `libsp_runtime.a`) ran the full matrix 3 times, all
  passing each time (exit 0, 13/13 checks):
  - TLS round-trip with a trusted CA: 200 `tls-ok`.
  - Self-signed cert rejected: 500 `TLS handshake failed ... certificate
    verify failed`.
  - Hostname mismatch rejected: 500, same clear TLS error.
  - `insecureSkipVerify` connects to the self-signed server: 200.
  - TLS server rejects a plaintext client: 500, no `tls-ok` served.
  - Plaintext HTTP still works: 200 `tls-ok`.
  - `listen()` fails loudly on a missing cert file and on a key/cert
    mismatch, with the OpenSSL reason in `getLastError()`.
- The driver deliberately never calls `stop()` (it exits via
  `std::quick_exit`): see the blocker below.
- **Committed gtest suite** (`tests/runtime/TlsTest.cpp`, 8 tests): the two
  `listen()`-failure tests pass in the real `sp_tests` binary (2/2). The
  six server tests exercise exactly the code paths the driver proved, but
  abort at teardown in this environment — see blocker.
- **No regressions in the tests that can run here:** the 8 non-server
  `HttpTest` tests pass (8/8).
- **Known blocker (pre-existing, out of scope for this slice):** every
  test that starts a real `HttpServer` aborts at teardown in this
  environment with `*** bit out of range 0 - FD_SETSIZE on fd_set ***`.
  Root cause, proven with `strace -f -k` + disassembly: `HttpServer::stop()`
  closes the listener while the accept thread is inside
  `TcpListener::acceptTimeout`'s `select()`; the `select()` then returns
  "readable" (POLLNVAL on the closed fd), and the fortified `FD_ISSET` on
  the now-`-1` handle aborts in `__fdelt_warn`. This is the same abort
  Phase 1 recorded for the pre-existing `HttpTest`/`WebTest` server tests,
  and it reproduces for a plain-HTTP `HttpTest.HttpServerAndClientEndToEnd`
  with no TLS code involved. Fixing it belongs to `Net.cpp` (e.g. re-check
  the handle after `select()`, or `shutdown()` instead of `close()`); this
  slice was told not to touch `Net.cpp`, and another slice is already
  debugging there (uncommitted `[dbg]` prints in `runtime/src/net/Net.cpp`).
  Until that race is fixed, no server-based test — TLS or plaintext — can
  complete in this environment; the driver above is the honest substitute
  until then.
- `git status`: uncommitted, nothing committed, nothing pushed, `main`
  untouched (per task instructions).

## What is deliberately NOT done

- **SP-language bindings:** the `NetBridge` HTTP server/client functions
  (`sp_rt_http_server_*`, `sp_rt_http_get/post`) do not yet expose TLS
  options — TLS is configured at the C++ `HttpServerConfig` /
  `HttpClientOptions` level only. Wiring `tlsEnabled`/`tlsCertFile`/
  `tlsKeyFile` (and client `caFile`/`insecureSkipVerify`) through to SP is
  a follow-up slice, not this one. No `NetBridge.cpp` file was touched.
- **Client certificate auth (mTLS):** the server does not request or verify
  client certificates.
- **ALPN / HTTP/2:** no ALPN negotiation; HTTP/1.1 only, as before.
- **WebSocket over TLS (`wss://`):** `WebSocket.cpp` is untouched; it
  remains plaintext-only.
- **OCSP stapling, session tickets/resumption tuning, cipher-suite
  configuration:** OpenSSL defaults are used (TLS 1.2 minimum enforced).
- **Postgres TLS is separate:** `PgConnection` already had its own TLS
  (`sslmode`); this slice does not change it.

---

## Slice C — Expose hidden features from SP + persistence

### What was already there (verified, not assumed)

The v1/v2 reviews said queue handlers, cron, and WebSocket transport were
"not exposable from SP". Checking the code showed the real gap is narrower:

- The **C ABI already existed** for the job queue (`sp_rt_job_queue_*`,
  incl. `sp_rt_job_queue_register_handler` taking a native function pointer)
  and for cron (`sp_rt_cron_schedule_interval`, `sp_rt_cron_schedule`).
  An SP `fn` passed as `ptr` is a native function pointer — the same
  mechanism the HTTP server already uses for route handlers — so a worker
  thread can invoke compiled SP code directly. No stub, no new dispatch.
- What was **actually missing from SP**: the stdlib wrapper
  (`stdlib/queue/queue.sp`) declares `enqueue`, `pendingCount`, etc. but
  does **not** declare `sp_rt_job_queue_register_handler`,
  `sp_rt_cron_schedule_interval`, or `sp_rt_cron_schedule`. An SP program
  could enqueue jobs and create a cron, but could never register a handler
  or schedule a task through the stdlib. (Stdlib is outside this slice's
  file scope; the exact `extern "C"` lines to add are listed below.)
  Deeper still — found only by running a real SP program (see §1/§2) —
  **the compiler itself cannot produce a function pointer**: a bare SP
  `fn` used as a `ptr` argument lowers to literal null. So even with the
  stdlib wrappers, callbacks from SP stay blocked until the compiler can
  take a function's address (or fails loudly instead of emitting null).
- **WebSocket had framing but no transport**: `encodeFrame`/`decodeFrame`,
  `WebSocketConnection`, and `WebSocketHub` existed, but nothing performed
  the HTTP Upgrade handshake and no bytes ever reached a socket.
- **Sessions were in-memory only** (`SessionStore`): a restart lost every
  session. There was no durable queue at all.

### What is new in this slice

**1. Queue handlers from SP — runtime mechanism works, SP cannot pass a
function yet (compiler gap, found by proof)**

- No runtime change needed: `JobQueue::registerHandler` + the C ABI
  already invoke the handler on a worker thread, with retries + DLQ on
  failure (handler returns `false` → retry with backoff → DLQ).
- Proven at the C level by test
  `ObservabilityTest.Phase31JobQueueHandlerViaCABIFires`: a plain C
  function pointer registered through
  `sp_rt_job_queue_register_handler` is really invoked by the worker
  thread with the job payload; `completedCount == 1`, DLQ empty.
- **But an SP program cannot do this yet — proven by running real SP
  code.** With Clang now installed, `spc run` was tried on a program
  that registers an SP `fn` as the handler (`sp_rt_job_queue_register_handler(q,
  "send_email", sendEmail)`). The program runs, but the handler never
  fires: `completed=0 dead=1` — the job goes to the DLQ. `spc llvm`
  shows why: the compiler lowers the bare function reference to a
  literal null (`call void @sp_rt_job_queue_register_handler(ptr %v1,
  ptr @.str.2, i32 0)`), and the bridge silently ignores a null
  `handler_fn`. There is no way in SP today to take a function's address
  (no `&fn` operator, no fn-address expression), and the compiler fails
  silently instead of erroring. **This is a compiler fix (slice B's
  area), not a runtime gap**: either the compiler must lower a bare fn
  reference to its address, or reject it loudly. Until then, from SP
  only enqueue/dequeue works; handler callbacks are still pending.
- SP usage (once the compiler + stdlib lines below land):
  ```sp
  fn sendEmail(string jobId, string name, string payload) -> bool {
      // ... send the email described by payload ...
      return true;
  }
  let q = JobQueue.getGlobal();
  q.registerHandler("send_email", sendEmail);   // compiler + stdlib pending
  q.enqueue("send_email", "{\"to\":\"a@example.com\"}");
  ```

**2. Cron from SP — same compiler gap**

- No runtime change needed: `CronScheduler` (interval + 5-field cron
  expressions) already existed with a C ABI.
- Proven at the C level by test
  `ObservabilityTest.Phase31CronIntervalViaCABIFires`: a C function
  pointer scheduled through `sp_rt_cron_schedule_interval` fires
  repeatedly on the scheduler thread (≥2 ticks in 220 ms at a 50 ms
  interval).
- **But the same SP proof program shows the cron callback never fires
  either**: `spc llvm` shows `call void @sp_rt_cron_schedule_interval(ptr
  %v9, ptr @.str.6, i32 100, i32 0)` — null task pointer again, silently
  ignored by the bridge. No `CRON TICK` was printed in 350 ms at a
  100 ms interval. Same compiler fix needed.

**3. WebSocket transport — NEW, end-to-end verified**

- `runtime/include/sp/runtime/WebSocket.h` / `runtime/src/net/WebSocket.cpp`:
  new `WebSocketServer` (process-wide registry) plus a minimal
  `WsTransport` byte-transport interface. `serveUpgraded()` sends the
  RFC 6455 `101 Switching Protocols` response (accept key via the
  existing `computeWebSocketAccept`), then runs the frame loop on the
  calling worker thread: text frames dispatch to the per-path handler,
  ping is auto-answered with pong, close ends the loop. `sendTo()` /
  `broadcast()` / `closeConnection()` work from any thread.
- `runtime/src/http/Http.cpp`: minimal hook in `HttpServer::handleClient`
  (~25 lines) plus a small `WsConnectionAdapter` that adapts the server's
  `Connection` (plaintext or TLS, slice A's abstraction) to `WsTransport`.
  After a request parses, if it is a WebSocket upgrade
  (`Upgrade: websocket`, `Connection` contains `Upgrade`,
  `Sec-WebSocket-Key` present) **and** a route is registered for the path,
  the socket leaves HTTP and the frame loop owns it. Upgrade requests for
  unregistered paths fall through to normal HTTP handling (404). No
  change to the HTTP client code. Because the frame loop only sees the
  `WsTransport` interface, `wss://` works whenever the server has TLS
  enabled — frames flow through the same TLS connection — though no
  `wss://` test was added in this slice.
- `runtime/src/net/NetBridge.cpp` + `Runtime.h`: new C ABI
  `sp_rt_ws_route(path, handler_fn)`,
  `sp_rt_ws_send(conn_id, msg)`, `sp_rt_ws_broadcast(msg)`,
  `sp_rt_ws_close_conn(conn_id)`, `sp_rt_ws_connection_count()`.
  `handler_fn` is `void (*)(int64_t conn_id, const char* msg)` — a native
  SP `fn` passed as `ptr`.
- **Bug fixed along the way**: `WebSocketConnection::handleIncomingBytes`
  invoked `onText`/`onBinary`/`onClose` while holding the connection
  mutex, and `sendText()` takes the same mutex — an echo handler
  deadlocked the connection. Handlers are now collected under the lock
  and invoked after it is released (same for `close()`).
- Proven by test `RealtimeTest.Phase31WebSocketUpgradeHandshakeAndEchoRoundTrip`:
  a real `HttpServer` on 127.0.0.1, a **raw socket client** (no WS library)
  sends the RFC 6455 §1.3 example handshake and gets `101` with the exact
  expected accept key `s3pPLMBiTxaQ9kYGzzhZRbK+xOo=`; a masked text frame
  `"hello ws"` is echoed back byte-identical; plain HTTP (`/ping`) still
  works on the same server while the WS connection is open; after a close
  frame the server drops the connection (`connectionCount == 0`).
- SP usage (stdlib wrapper pending — and blocked by the same compiler
  gap as the queue handler above: `sp_rt_ws_route` takes a handler
  `ptr`, which SP cannot yet produce from a bare `fn`):
  ```sp
  fn onChat(int connId, string msg) {
      wsSend(connId, msg);   // echo; sp_rt_ws_send via extern "C"
  }
  wsRoute("/chat", onChat);  // sp_rt_ws_route via extern "C"
  let server = HttpServer.create();
  server.listen("127.0.0.1", 8080);
  server.wait();
  ```
  The transport itself is proven (C-level test below); the SP-side
  `wsRoute` call needs the compiler fn-address fix first.

**4. Persistent queue + persistent sessions — NEW, restart-verified**

- `runtime/include/sp/runtime/PersistentQueue.h` /
  `runtime/src/obs/Queue.cpp`: `PersistentQueue`, SQLite-backed
  (`sp_jobs` table, WAL). `enqueue()` commits before returning.
  `tryDequeue()` claims the oldest due job inside a transaction
  (FIFO by `run_at_ms`). `complete()` / `fail()` (exponential backoff,
  then DLQ) / `retryDlqJob()` / `clearDlq()`. Jobs left `processing` by a
  crashed process are reset to `pending` on `open()` — retried, not lost.
  **No in-memory fallback**: `open()` returns `false` with `lastError()`
  set when the DB cannot be opened; `enqueue()` returns `""` on failure.
- `runtime/include/sp/runtime/PersistentSession.h` /
  `runtime/src/crypto/Session.cpp`: `PersistentSessionStore`, same
  create/get/set/destroy/regenerate/count semantics as the in-memory
  `SessionStore`, rows in `sp_sessions` (`id, data, created_at_ms,
  expires_at_ms`). Expired sessions are never returned (deleted lazily);
  `set()` refuses to resurrect an expired session. Same no-fallback
  error contract as the queue.
- C ABI (`Runtime.h`, `ObsBridge.cpp`, `SecurityBridge.cpp`):
  `sp_rt_pqueue_open/close/last_error/enqueue/dequeue/complete/fail/pending_count/dlq_count/retry_dlq/clear_dlq`
  (`dequeue` returns the job as JSON, `"{}"` when none is due) and
  `sp_rt_psession_open/close/last_error/create/get/set/destroy/regenerate/count`
  (`open` returns `null` on failure).
- Proven by tests in `tests/runtime/DatabaseTest.cpp`:
  `PersistentQueueTest.SurvivesRestartAndKeepsFifoOrder` (enqueue 3,
  close, reopen, dequeue in order), `.ClaimedJobIsRetriedAfterCrashNotLost`,
  `.OpenFailureIsARealError` (missing parent dir → real error, C ABI
  `open` returns null), `.CAbiEnqueueDequeueAcrossReopen`;
  `PersistentSessionTest.SurvivesRestartAndEnforcesExpiry` (session
  survives reopen; 1 s TTL session is gone after 1.2 s),
  `.OpenFailureIsARealError`, `.CAbiCreateGetAcrossReopen`.

### Files changed (this slice only)

- `runtime/include/sp/runtime/WebSocket.h` — `WebSocketServer`, `WsTransport`, include of `Net.h`
- `runtime/src/net/WebSocket.cpp` — `WebSocketServer` impl + handler-deadlock fix
- `runtime/src/http/Http.cpp` — upgrade hook in `handleClient` + `WsConnectionAdapter` only
- `runtime/src/net/NetBridge.cpp` — `sp_rt_ws_route/send/broadcast/close_conn/connection_count`
- `runtime/include/sp/runtime/PersistentQueue.h` (new) + `runtime/src/obs/Queue.cpp` (impl)
- `runtime/include/sp/runtime/PersistentSession.h` (new) + `runtime/src/crypto/Session.cpp` (impl)
- `runtime/include/sp/runtime/Runtime.h` — new C ABI declarations
- `runtime/src/obs/ObsBridge.cpp` — `sp_rt_pqueue_*`
- `runtime/src/crypto/SecurityBridge.cpp` — `sp_rt_psession_*`
- `runtime/src/net/Net.cpp` — **pre-existing race fix, not a feature**:
  `TcpListener::acceptTimeout` read `m_handle` twice across a blocking
  `select()`; `HttpServer::stop()` closes the listener from another
  thread, so the post-`select()` `FD_ISSET(m_handle)` could read
  `SP_INVALID_SOCKET` (-1) and the fortified `__fdelt_chk(-1)` aborted
  the whole process. Proven with an `LD_PRELOAD` interposer on
  `__fdelt_chk` (bad fd `18446744073709551615`, backtrace into
  `acceptTimeout` ← `serverLoop`). The fix snapshots the handle into a
  local before `FD_SET`/`select`/`FD_ISSET`. This was the root cause of
  the Phase-1 `HttpTest`/`WebTest` aborts (15 tests) — they all pass now.
- `tests/runtime/ObservabilityTest.cpp` — handler + cron C-ABI tests
- `tests/runtime/RealtimeTest.cpp` — WS handshake + echo round-trip test
- `tests/runtime/DatabaseTest.cpp` — persistent queue/session tests
- `docs/v2/PHASE-31-PRO-4.0.0.md` — this section

### Verification performed

- Fresh build dir `/tmp/sp-build-phase2-queue` (did not reuse `/tmp/sp-build`):
  CMake configure OK; `sp_tests` + `spc` build clean, 0 errors. (One
  self-inflicted build break on the way: `nowMs()` in the new
  `PersistentQueue` methods needed the qualified `JobQueue::nowMs()` —
  fixed, diff is purely additive.)
- New tests: **10/10 pass** (`ObservabilityTest.Phase31*` x2,
  `RealtimeTest.Phase31WebSocketUpgradeHandshakeAndEchoRoundTrip`,
  `PersistentQueueTest.*` x4, `PersistentSessionTest.*` x3). WS test
  re-run 5x after the race fix: 5/5 pass.
- Regression: **117/117 pass** across 14 runtime suites
  (`RealtimeTest`, `ObservabilityTest`, `DatabaseTest`,
  `PersistentQueueTest`, `PersistentSessionTest`, `NetTest`, `HttpTest`,
  `WebTest`, `SecurityTest`, `CryptoTest`, `JsonTest`, `ConcurrencyTest`,
  `RuntimeTest`, `OpenApiTest`, `TlsTest`) — excluding the known-hanging
  `NetTest.UdpSocketSendRecv` (pre-existing, untouched). The `HttpTest` /
  `WebTest` suites (28 tests) pass in full for the first time here: the
  `acceptTimeout` race fix repaired the 15 Phase-1 aborts.
- **Real SP program proof** (`spc run`, Clang 18.1.3 now installed):
  a program using `extern "C"` decls for the queue/cron C ABI compiles
  and runs; enqueue works from SP, but the handler and cron callbacks do
  NOT fire — see §1/§2: the compiler emits null for bare fn references
  (`completed=0 dead=1`, no ticks). This proof is what downgraded the
  "handlers work from SP" claim to "runtime works, compiler gap blocks
  SP". The proof program is not committed (it lives in the report, not
  the repo); the C-level tests are the committed regression net.

### NOT done / known limits

- **SP cannot pass a function as a callback yet — the blocker is the
  compiler, not this slice.** The C ABI + runtime are done and tested,
  but `spc` lowers a bare SP `fn` used as a `ptr` argument to literal
  null (`i32 0`) instead of the function's address — silently. What is
  needed (compiler slice): a way to take a function's address (e.g. an
  `&fn` operator or bare-fn-reference lowering) — or at minimum a loud
  compile error when a bare fn is used as a value, instead of silent
  null. This affects queue handlers, cron tasks, WS route handlers, and
  the pre-existing `HttpServer.route` / `Thread.spawn` `ptr` parameters
  equally. Until that lands, from SP only enqueue/dequeue (and the
  persistent queue/session APIs, which take no callbacks) work.
- **Stdlib wrappers are the remaining step after the compiler fix.**
  This slice's file scope excludes `stdlib/`. The exact additions needed
  in `stdlib/queue/queue.sp` (`extern "C"` block):
  `fn sp_rt_job_queue_register_handler(ptr queue, string name, ptr handlerFnPtr);`
  `fn sp_rt_cron_schedule_interval(ptr cron, string name, int interval_ms, ptr taskFnPtr);`
  `fn sp_rt_cron_schedule(ptr cron, string name, string cron_expr, ptr taskFnPtr);`
  plus `JobQueue.registerHandler(...)` / `CronScheduler.scheduleInterval(...)`
  / `.schedule(...)` methods, following the existing `HttpServer.route`
  pattern. And in `stdlib/net/websocket.sp`: `fn sp_rt_ws_route(string path,
  ptr handlerFnPtr); fn sp_rt_ws_send(int conn_id, string msg) -> bool;`
  plus matching wrappers. The C ABI + runtime underneath are done and
  tested; the wrappers are mechanical once the compiler can produce a
  fn pointer.
- **The bridge silently ignores null callbacks.** `sp_rt_job_queue_register_handler`,
  `sp_rt_cron_schedule_interval`, and `sp_rt_ws_route` return `void` and
  drop a null `handler_fn` without any signal — that is how the SP proof
  program's dead job happened silently. A loud failure needs an error
  channel the C ABI does not have; noted, not changed.
- **WS route registry is process-wide** (one `WebSocketServer::getGlobal`),
  not per-`HttpServer` instance. An SP program normally runs one server,
  so this matches real usage; a program running two servers on different
  ports would share WS routes. Documented, not hidden.
- **A WS connection occupies one HTTP worker thread** for its lifetime
  (default 4 workers). Long-lived connections beyond the worker count
  will queue. A dedicated WS acceptor thread is a follow-up, not done.
- **`wss://` works by construction** (frame loop runs over slice A's
  `Connection` abstraction) but has **no test** in this slice — the
  plaintext round-trip test is the only WS transport test.
- **No WS client API**: only the server side is implemented (accept,
  send, receive, broadcast). A client (`sp_rt_ws_connect`) is not done.
- **Persistent queue has no delayed-job visibility API** beyond
  `pendingCount` (a pending-but-not-yet-due job counts as pending), and
  no per-queue worker pool — dequeue is explicit (pull), unlike the
  in-memory `JobQueue` (push). A durable worker that auto-dispatches to
  SP handlers across restarts is a follow-up.
- **Cron expressions** support `*`, `*/N`, and comma lists per field
  (pre-existing `CronScheduler` limitation); ranges (`1-5`) and named
  days/months are not supported, and schedules are evaluated in UTC.
- `PersistentSessionStore::regenerate` deletes the old id before
  inserting the new one; a crash between the two loses the session.
  Accepted: regeneration is rare and the window is one statement apart.

---

## Slice D — Registry backend dogfooded in SP + real benchmark harness + OpenAPI

### What was built

**1. Real registry backend in SP — `examples/registry-backend/`**

A complete package registry implemented as an SP program on the SP HTTP
server + SQLite (`src/main.sp`, entrypoint; `src/routes.sp` route table;
`src/handlers.sp` one handler per route; `src/store.sp` SQLite access;
`src/auth.sp` Bearer auth; `src/config.sp` env config;
`src/json_util.sp` validation helpers; `sp.toml` package manifest).
It implements the same API surface the Next.js preview registry
pretends to have — but for real, against a SQLite database:

| Method & path | Auth | Behaviour |
| --- | --- | --- |
| `GET /api/health` | – | `{"status":"ok",...}` + stored package count |
| `POST /api/v1/publish` | Bearer | Stores the metadata `spm publish` sends. 201 on success; 409 if that version already exists (releases are immutable) |
| `POST /api/v1/packages/:name/:version/tarball` | Bearer | Uploads the tarball as base64 text. Stored only if its SHA-256 matches the checksum published with the metadata |
| `GET /api/v1/verify/:name/:version` | – | Real verification: decodes the stored tarball, SHA-256s it, compares with the published checksum. `verified:true` only on a match |
| `GET /api/v1/packages` | – | List all published packages |
| `GET /api/v1/packages/:name/:version` | – | Metadata for one version |
| `GET /api/v1/packages/:name/:version/download` | – | Base64 tarball bytes (`X-Content-Encoding: base64`, `X-Sha256` header). 404 while no tarball was uploaded |
| `GET /openapi.yaml` | – | The generated OpenAPI spec (served from `openapi.yaml`) |

Auth is real: the publish token comes from `SP_REGISTRY_TOKEN` (never
hard-coded), is compared with `Crypto.timingSafeEqual` (constant-time),
and missing/malformed/wrong tokens get 401. There is no "allow all when
unconfigured" mode — without the token the service exits at startup
instead of serving unauthenticated writes (`OS.exit(1)`, fail closed).

Storage is real: one SQLite table `packages`, one row per `name@version`,
bound parameters everywhere (no string-built SQL), `published_at` from
SQLite `datetime('now')`. Tarball bytes are stored as **base64 TEXT**, not
a BLOB — the SP SQLite API has no blob binding and reads TEXT back
through a C string, so raw bytes are never stored; the stored base64 is
hashed in memory for verification and returned as base64 on download.

Honest limits, stated in the code and the README rather than hidden:
`spm publish` sends metadata only (no tarball bytes), so after a publish
the checksum is *stored as claimed* and `verify` reports
`verified:false` until the tarball is uploaded; `download` returns 404
`TARBALL_NOT_UPLOADED` until then. The Next.js registry routes are
**untouched** — they stay the labelled preview/mock, and this service is
the real backend path alongside them, not a replacement claim.

**2. `spm publish` wired to it (proof — partial, see Verification)**

The client side was proven end-to-end as far as this environment allows
(commands + outputs below). The success path against the live SP service
could **not** run here (no Clang — see Verification). The exact commands
to run it are in `examples/registry-backend/README.md`.

**3. Real benchmark harness — `benchmarks/registry/load_test.py`**

Python 3, standard library only. Fires real HTTP requests with a thread
pool and reports **measured** requests/sec, mean/p50/p95/p99 latency,
status-code counts, and errors. Every number comes from that run's
measurements — nothing hard-coded. Warmup requests are excluded from the
numbers; a failed target reports 0 successes and exits 1 instead of
printing numbers. `benchmarks/backend/run_benchmarks.ps1` (the old
compile-check script) now points at this harness in its header note; its
behaviour is unchanged.

**4. OpenAPI generation — `examples/registry-backend/`**

`generate_openapi.py` generates `openapi.yaml` (OpenAPI 3.0.3, 8 routes)
from the route table in `src/routes.sp` — the (method, path) pairs come
only from the `app.get/post(...)` registrations plus their `// @openapi`
annotations, so a route that is not registered cannot appear in the spec.
`check_openapi.py` verifies spec ↔ routes in both directions. Response
statuses in the spec are a transcription of `src/handlers.sp` (the
mapping lives in the generator's `RESPONSES` table, which errors out if
a route has no entry).

### How to run the registry service

Requires a Clang backend toolchain (`spc build` / `spc run` compile SP
to a native binary through Clang; `g++` alone is rejected by the
compiler).

```sh
cd examples/registry-backend
export SP_REGISTRY_TOKEN="$(openssl rand -hex 24)"  # >= 16 chars, never commit it
spc run src/main.sp
# or: spc build src/main.sp -o registry-backend && ./registry-backend
```

Optional: `REGISTRY_PORT` (default 8090), `REGISTRY_DB` (default
`registry.db`), `REGISTRY_OPENAPI_PATH` (default `openapi.yaml`; run
from this directory).

### `spm publish` proof — commands and results

Test package `/tmp/sp-test-pkg` (`registry-proof` v0.1.0, lib, one
trivial `src/lib.sp`). `spm` binary used: `/tmp/sp-build/bin/spm`
(Phase 1 build; `tools/spm` source is unchanged on this branch, so the
fresh-build binary below is the same code).

Dry run — validates + archives locally, no network:

```sh
cd /tmp/sp-test-pkg && spm publish --dry-run
# Packaging and verifying release for https://registry.splang.shantopaul.com (dry-run mode)...
# Package:    registry-proof v0.1.0
# Artifact:   ./target/packages/registry-proof-0.1.0.tar.gz (433 bytes)
# Checksum:   783bb07fca9810f08a66e18532eef073b8cfe0bcd3fa5aca883e3d0d13cf4083 (SHA-256)
# Status:     Dry run mode active: validation succeeded, archive created at ...
# exit 0
```

Dead registry + token — failure is failure (Phase 1 behaviour holds):

```sh
SP_REGISTRY_TOKEN='<throwaway test string>' spm publish \
  --registry http://127.0.0.1:9 --token "$SP_REGISTRY_TOKEN"
# Packaging and verifying release for http://127.0.0.1:9...
# error: publication failed: Registry publish failed (HTTP 500): Failed to connect to 127.0.0.1:9
#   The package was validated and archived locally only; nothing was published.
# exit 1
```

No token — fails before any network attempt:

```sh
spm publish --registry http://127.0.0.1:9
# error: publication failed: Publishing requires an API token. Authenticate via
#   'spm login' or supply --token <token> / set SP_REGISTRY_TOKEN.
# exit 1
```

Live success path — **NOT run here** (see Verification). Exact commands
are in `examples/registry-backend/README.md` ("The `spm publish` proof"):
valid token → 201 + stored package; wrong/missing token → 401 + `spm`
exit 1; then tarball upload → `verify` flips to `verified:true`;
download round-trips the tarball hash.

### Measured benchmark numbers

One real measured run, recorded 2026-11-09 03:38 UTC. Target was
**Python's `http.server` on 127.0.0.1:18099** (the harness self-check),
NOT the SP service — the SP service could not run here (no Clang).
These numbers describe that server on this machine, not SP performance,
and must not be quoted as SP numbers.

```sh
python3 benchmarks/registry/load_test.py --url http://127.0.0.1:18099/ \
  --concurrency 10 --duration 10 \
  --output benchmarks/registry/results/http-server-self-check-2026-11-09.json
# requests:    1624 sent in 10.831s
# throughput:  149.93 requests/sec (measured)
# succeeded:   1624 (2xx)
# latency ms:  mean=62.84 p50=17.43 p95=93.29 p99=1101.05 max=2316.70
# status codes: {"200": 1624}
# errors:      {}
# exit 0
```

Context: Linux 7.0.0-43-generic x86_64, Python 3.12.3, 4 CPUs. The long
p99 tail is that server (single-threaded `http.server`), not the harness.
Full report: `benchmarks/registry/results/http-server-self-check-2026-11-09.json`.

Failure-path run (closed port, `--duration 3 --warmup 0`): 4836
`URLError` (connection refused), 0 successes, no latency numbers,
exit 1 — the harness reports failure instead of inventing numbers.

To measure the SP service once it runs:

```sh
python3 benchmarks/registry/load_test.py \
  --url http://127.0.0.1:8090/api/health --concurrency 10 --duration 10 \
  --output benchmarks/registry/results/registry-health-<date>.json
```

### OpenAPI

- Spec: `examples/registry-backend/openapi.yaml` (generated, 8 paths / 8 operations).
- Generator: `examples/registry-backend/generate_openapi.py`
  (`python3 generate_openapi.py` regenerates from `src/routes.sp`).
- Checker: `examples/registry-backend/check_openapi.py` — result:
  `OK: openapi.yaml matches routes.sp (8 routes)`.
- The YAML parses cleanly (`yaml.safe_load` OK).
- Served live by the service at `GET /openapi.yaml`.

### Files changed (this slice only)

| File | Change |
| --- | --- |
| `examples/registry-backend/sp.toml` (new) | Package manifest (bin, entry `src/main.sp`, version 3.1.0) |
| `examples/registry-backend/src/main.sp` (new) | Entrypoint: fail-closed startup, middleware, `app.run(port)` |
| `examples/registry-backend/src/config.sp` (new) | Env config (token, port, DB path, spec path) |
| `examples/registry-backend/src/routes.sp` (new) | Route table — single source of truth for the spec |
| `examples/registry-backend/src/handlers.sp` (new) | One handler per route; all responses built from SQLite |
| `examples/registry-backend/src/store.sp` (new) | SQLite: schema, publish, lookup, tarball, verify |
| `examples/registry-backend/src/auth.sp` (new) | Bearer check with timing-safe compare, 401 helper |
| `examples/registry-backend/src/json_util.sp` (new) | JSON escaping + name/version/checksum/base64 validation |
| `examples/registry-backend/README.md` (new) | Run instructions, endpoints, spm proof, limits |
| `examples/registry-backend/generate_openapi.py` (new) | Spec generator (parses `src/routes.sp`) |
| `examples/registry-backend/check_openapi.py` (new) | Spec ↔ routes consistency checker |
| `examples/registry-backend/openapi.yaml` (new, generated) | OpenAPI 3.0.3 spec, 8 routes |
| `benchmarks/registry/load_test.py` (new) | Real load-test harness (measures, never hard-codes) |
| `benchmarks/registry/README.md` (new) | Harness usage + where recorded runs live |
| `benchmarks/registry/results/http-server-self-check-2026-11-09.json` (new) | Measured harness self-check report |
| `benchmarks/backend/run_benchmarks.ps1` | Header note only: points at the new harness |
| `docs/v2/PHASE-31-PRO-4.0.0.md` | This section |

Not touched by this slice: the Next.js registry routes, HTTP/TLS code
(slice A), Postgres/async code (slice B), queue/WebSocket code (slice C),
compiler code, any `CMakeLists.txt`, `.github/`, or the version (3.1.0).

### Verification performed

- **C++ build (fresh dir):** CMake configure OK in
  `/tmp/sp-build-phase2-registry` (did not reuse `/tmp/sp-build`).
  Full build **FAILS — not on this slice's files**: the only error is
  in slice C's `runtime/src/obs/Queue.cpp` (`'nowMs' was not declared in
  this scope`, lines 528/557/583/657/692 — the new `PersistentQueue`
  methods call bare `nowMs()`, but it is `JobQueue::nowMs()`, a static
  member of a different class; the same file already uses the qualified
  form at lines 314/325/391/396). `sp_compiler` built fine (37%). This
  slice adds no C++ files and touches no `CMakeLists.txt`, so there is
  nothing here to fix — slice C needs `JobQueue::nowMs()` (or an
  equivalent) in those five call sites. Until that lands, no fresh
  `spc`/`spm` binary can be produced from this tree; the `spc`/`spm`
  runs above used the Phase 1 build (`/tmp/sp-build/bin/`, built from
  clean `e6e7230`), whose `tools/spm` source is unchanged on this
  branch.
- **`spc check src/main.sp`:** PASS — "Check passed: Syntax & Semantics
  valid (259 tokens, 87 declarations)". Covers all 7 service modules.
  Two real issues found and fixed during checking: `char` vs `string`
  literal comparison (`ch == "."` → `ch == '.'`) and the stdlib free
  function `toLower` not resolving from a handler (use the `String.lower`
  class method instead).
- **`spc llvm src/main.sp`:** emitted `/tmp/registry-main.ll` (460 KB).
  SP-level call graph verified from the IR: `publish` calls
  `RegistryAuth_validateBearer` → `Crypto_timingSafeEqual`, `Json_parse`,
  the validators, `RegistryStore_publishVersion`/`findVersion`;
  `verify` calls `RegistryStore_verifyStored` (which calls
  `Crypto_base64Decode` + `Crypto_sha256` + `Crypto_timingSafeEqual`);
  `uploadTarball` calls base64-decode + SHA-256 + timing-safe compare +
  `storeTarball`; `main` calls `RegistryRoutes_registerRoutes` and the
  `App` lifecycle. All 8 route paths appear as string constants in the
  IR. (The IR also declares the full imported stdlib surface, so this
  check was done on the per-function call lists, not on global symbol
  presence.)
- **OpenAPI:** generator + checker run; checker OK (8 routes, both
  directions); YAML parses cleanly.
- **Harness:** self-check run measured (see numbers above); failure-path
  run exits 1 with 0 successes and no invented numbers.
- **`spm`:** dry-run OK (archive + checksum); dead-registry publish
  fails exit 1; no-token publish fails exit 1. Live success path NOT
  run (see below).

### NOT done / known limits

- **The live end-to-end proof could not run in this environment.**
  Running any SP program needs a Clang backend toolchain and none is
  installed here (`spc build`/`run` reject `g++` explicitly; there is no
  SP interpreter). So the service was verified up to `spc check` +
  `spc llvm` only — it has never actually started, and the `spm publish`
  success path, tarball upload, `verify:true` flip, and download
  round-trip are implemented but **unexecuted**. Do not claim they work
  until someone runs them on a machine with Clang (commands are in
  `examples/registry-backend/README.md`). This is the single most
  important open item for this slice.
- **No SP-service benchmark numbers exist yet.** The only measured run
  is the harness self-check against Python `http.server`; it proves the
  harness measures honestly, not that the SP service is fast.
- **No TLS on the service.** Plain HTTP on localhost; slice A added TLS
  to the runtime, but the SP-level HTTP bindings do not expose TLS
  options yet, so this service cannot serve HTTPS by itself. Put TLS
  termination in front of it for any non-localhost use.
- **One shared publish token, no per-user accounts or scopes.**
- **No `spm install` integration** — publishing is the proven path;
  resolving/installing from this registry is future work.
- **Concurrency:** one SQLite connection per request, WAL + busy
  timeout; concurrent publishes are serialized by SQLite. Fine for a
  small registry; not a high-write design.
- **Tarball stored as base64 TEXT**, not a BLOB (SP SQLite API has no
  blob binding). A 1 MB tarball costs ~1.37 MB of TEXT plus base64
  encode/decode on every upload/download/verify.
- **`verify` recomputes the hash on every call** (decode + SHA-256 of
  the whole tarball). Correct, but O(tarball size) per request; no
  caching.
- **Version check is a simplified semver rule** (digits/dots/dashes,
  must contain a digit and a dot), documented in `json_util.sp` and the
  spec — not a full semver grammar.

## Slice B1 — Postgres depth

### What changed
- **Named prepared statements, exposed from SP.** `PgConnection::prepare(name, sql)`
  sends Parse with the given statement name and records it on the connection;
  `execPrepared(name, params)` reuses it via Bind (fresh unnamed portal) +
  Describe + Execute + Sync, sharing the row-parsing code with `query` /
  `queryParams` through a new `readResultLoop()` helper. C-ABI
  `sp_rt_pg_prepare()` / `sp_rt_pg_exec_prepared()` (the latter returns
  `nullptr` on failure, never a fake-success handle) and SP methods
  `PgConnection.prepare()` / `execPrepared()` in `stdlib/db/postgres.sp`;
  `queryParams` is now also exposed in SP (it existed in C-ABI only). A
  duplicate prepare fails loudly: PostgreSQL itself rejects the second Parse
  (42P05) and the error is surfaced via `lastError()`; the old statement is
  kept and the name is not registered on failure. There is no separate
  `PgStatement` handle — a Postgres prepared statement is name-keyed
  server-side state on the connection.
- **SCRAM server-signature verification.** `handleScramAuth` now computes
  ServerKey / ServerSignature per RFC 7677, parses the `v=` attribute from
  SASLFinal, compares the decoded bytes in constant time, and fails
  `connect()` ("SCRAM server signature verification failed") on a mismatch
  **or when the server sends no `v=` at all**. Previously the value was read
  and never checked, so any server was accepted.
- **Postgres TLS with certificate verification.** `PgConfig` parses
  `sslmode=verify-ca` / `verify-full` and `sslrootcert` (URI query params
  and key=value format). For those modes `connect()` sets `SSL_VERIFY_PEER`,
  loads the default trust roots (plus `sslrootcert` when given), and for
  `verify-full` also verifies the hostname against the certificate
  (`SSL_set1_host`) and checks `SSL_get_verify_result()` after the
  handshake. A server answering `'N'` (no TLS) now fails `require` and both
  verify modes instead of silently falling back to plaintext.
  `sslmode=require` deliberately still does **not** verify the peer —
  encrypted but unauthenticated — and is documented as such.
- Two adjacent fixes in the same code: `PgConnection::close()` now always
  releases the socket and SSL objects even when the connection never reached
  the connected state (previously leaked the fd/SSL on every failed
  `connect()`), and plaintext writes use `MSG_NOSIGNAL` so a server-closed
  connection surfaces as a failed write instead of killing the process
  with SIGPIPE.

### Verification
- Fresh build dir `/tmp/sp-build-phase2-pg`
  (`cmake -S . -B /tmp/sp-build-phase2-pg -DCMAKE_BUILD_TYPE=Release
  -DSP_BUILD_TESTS=ON`, `cmake --build -j2 --target sp_tests`): clean.
- New tests in `tests/runtime/PostgresTest.cpp` (suite `PostgresB1`),
  **12/12 passed**, against a fake Postgres wire-protocol stub on loopback
  (startup / cleartext + SCRAM auth / Parse / Bind / Describe / Execute /
  Sync / optional TLS; certificates generated in-test with the `openssl`
  CLI). No live Postgres server was used or required. Coverage: prepare /
  exec / reuse (Parse sent once across two execs), duplicate prepare failing
  loudly, unknown-statement error, C-ABI prepare/exec, SCRAM wrong-signature
  and missing-signature failures, SCRAM correct-signature success (stub also
  verifies the client's proof), `verify-ca` rejecting an untrusted cert,
  `verify-full` rejecting a hostname mismatch and accepting a matching
  `IP:127.0.0.1` SAN cert, verify mode refusing a plaintext server, and
  `require` still connecting to a self-signed cert.
- Regression: pre-existing `PostgresTest` / `SqliteTest` / `DbPoolTest` /
  `DbBridgeTest` suites, **16/16 passed**. The full `sp_tests` suite was not
  run (known pre-existing environmental failures, e.g. ExecutionTest needs
  Clang).

### Honest limits
- No test against a real PostgreSQL server was run; the stub speaks the wire
  protocol but is not PostgreSQL (e.g. it infers nothing about types and
  echoes the first bound parameter as the row value).
- SP-side `string[]` parameter marshalling to the `const char**` C-ABI is
  declared but **not verified by an SP program run** — there is no precedent
  in the repo for passing an array into an `extern` function. The C++ C-ABI
  is fully tested; treat the SP declaration as unproven until an SP program
  exercises `execPrepared` / `queryParams`.
- `sslmode=require` remains unverified by design; `verify-ca` verifies the
  chain but not the hostname by design.

## Slice B2 — Real async

### What changed
`await` is real now, for a deliberately small subset. Phase 1 made every
`await` fail loudly (error[E1000]); B2 lowers two awaitable forms and
keeps everything else failing loudly. The runtime gained a timer-capable
scheduler on top of the existing `poll()`-based `EventLoop`
(`sp::rt::async::Scheduler` in `runtime/include/sp/runtime/Async.h`,
implemented in `runtime/src/net/EventLoop.cpp` — no CMake change was
needed) plus a small `Task` future type and a C ABI
(`sp_rt_async_*` in `runtime/include/sp/runtime/Runtime.h`,
`runtime/src/net/NetBridge.cpp`).

### Design: pump-based await (not a state machine)
The compiler lowers `await` to: create a task (registers a timer on the
scheduler and returns immediately), then pump the scheduler until that
task completes, then destroy the task handle. While the await is
outstanding the awaiting function makes no progress, but the loop is
live: other timers and queued tasks fire in the meantime. That is real
suspension of the async function's progress with a live loop — not a
blocking sleep; no `sp_rt_time_sleep_ms` / `sp_rt_thread_sleep_ms` call
is emitted on the await path. Two scheduler bugs found by tests were
fixed along the way: timer-timeout truncation (9.6 ms → 9 ms woke
before the timer was due) and `runUntilIdle` breaking on a single
no-work iteration; `runOnce` is now deadline-capped so `runFor` /
`runUntil` cannot overshoot their bound.

### Exact subset supported
Inside a **free `async fn`** only, anywhere in the body (statement,
initializer, inside `if` / `while`):
- `await sleep_async(ms)` — yields `void`.
- `await delayed_value(ms, value)` — yields the `int` value after `ms`.
Both are builtins returning `Task<T>` (`Task` is a builtin generic
class type; sema unwraps `await e: Task<T>` to `T`). Still
error[E1000], never silently miscompiled: `await` on a literal,
identifier, any other call (including `await worker()` on a user async
fn, and `await now()`), member calls, and operators; `await` outside a
free `async fn` (sema E1802 first, IRBuilder as backstop — this also
covers async methods and generic async fns, which B2 does not lower).

### Honest limits
Calling an async fn runs it to completion — the caller is not handed a
`Task`. The awaiting frame stays on the stack while pumping; a
heap-frame state machine that returns a `Task` to the caller is future
work. `Task` payload is `int64` only (0 for void tasks). The scheduler
is single-threaded by design. There is no async HTTP/fetch primitive to
await on (none exists; none was invented), and no `stdlib/async`
module yet (builtins only).

### Verification
- New tests, 20/20 pass (3 consecutive runs) in fresh build dir
  `/tmp/sp-build-phase2-async`: 11 `AsyncTest` (incl. the headline
  loop-not-blocked proof — a 20 ms timer fires *during* a 150 ms await,
  asserted on ordering and timestamps, not just the final value; plus
  timer due-order, cancel paths, `runFor` deadline, continuations, C
  ABI) and 9 `IRBuilderTest` (subset lowers to the async runtime calls
  with no blocking-sleep call; unsupported awaits still fail).
- Affected-area suite 118/118 (IR, sema, lexer, parser, concurrency,
  runtime). Non-network subset 461/463; the 2 failures are pre-existing
  `ExecutionTest` stdlib-time failures (epoch-ms truncated to `int`,
  unrelated to this slice).
- End-to-end with Clang 18.1.3: `spc run` on a program with two async
  fns prints `start / greeted / value: 42 / end`, exit 0; `spc ir`
  shows the exact async runtime calls; `await 1` / `await now()` fail
  with loud error[E1000], `await` outside async with E1802.
- Full `sp_tests` cannot complete in this environment for pre-existing
  reasons (hang in `NetTest.UdpSocketSendRecv`; `FD_SETSIZE` abort in
  untouched `Net.cpp`).

## Integration verification (parent, combined tree)

Both slices were built and tested together in a fresh build dir
`/tmp/sp-build-phase2-pgasync` (Release, `SP_BUILD_TESTS=ON`,
`sp_tests` + `spc` targets; the tree also contains the other Phase 2
slices' uncommitted work — TLS, queue/persistence, registry — which
were not part of slice B):
- Targeted suite — `PostgresB1.*`, `AsyncTest.*`,
  `IRBuilderTest.Await*` / `ShortCircuit*`, plus the DB regression
  suites (`PostgresTest`, `SqliteTest`, `DbPoolTest`, `DbBridgeTest`):
  **48/48 passed**.
- Affected-area suite (IR, IRVerifier, TypeChecker, Type, SymbolTable,
  Lexer, Parser, Concurrency, Async, Runtime, PostgresB1, PostgresTest):
  **132/132 passed**.
- End-to-end proof re-run in the combined build with Clang 18.1.3:
  `spc run` on a program with two async fns prints
  `start / greeted / value: 42 / end`, exit 0. `spc ir` on
  `await 1` and on `await now()` fails loudly with error[E1000]
  naming the supported subset. (`spc check` passes those files —
  expected: check is syntax + sema only, and the E1000 comes from
  IRBuilder.)
- Nothing committed, nothing pushed; branch `feature/pro-4.0.0`
  still at Phase 1 commit `e6e7230`; `main` untouched.

## Parent verification (final, all slices combined) + two parent fixes

The parent (not a slice) verified all four slices together in one fresh
build, fixed two real bugs the live proof exposed, and ran the registry
end-to-end proof that slice D could not run. Environment note: the VM was
replaced mid-verification (11:09 → 11:22 local rebuild window), which wiped
`/tmp` (all build dirs) and the apt-installed Clang. Clang 18.1.3 was
reinstalled (`apt-get install clang`, `/usr/bin/clang`) and the build was
redone from scratch — nothing about the tree changed, only the machine
under it.

### Final build + test results

- Fresh build dir `/tmp/sp-build-phase2` (Release, `SP_BUILD_TESTS=ON`):
  **clean, 0 errors**, all binaries built (`sp_tests`, `spc`, `spd`,
  `spls`, `spm`). `spc --version` reports **3.1.0** (no version bump).
- Full ctest (`-j2 --timeout 120`, so the hanging UDP test fails by
  timeout instead of hanging the suite): **537/540 passed (99%)**.
  Phase 1 was 398/488 (82%) — the jump is Clang now being present
  (ExecutionTest runs) plus slice C's `Net.cpp` race fix (the 15
  Http/Web aborts are gone).
- The 3 failures are all pre-existing and unrelated to Phase 2:
  - `ExecutionTest.StdlibTimeNativeExecution`,
    `ExecutionTest.StdlibFullIntegrationNativeExecution` — the known
    stdlib-time defect (epoch-ms truncated to `int`;
    `now_gt_0: true` never appears in the program output). Slice B2
    flagged the same two.
  - `NetTest.UdpSocketSendRecv` — **Timeout** (the known hang, now
    bounded at 120 s instead of hanging the suite forever).
- Targeted suites all green: `TlsTest.*` (incl. the 6 server-based
  tests that could not run before slice C's `Net.cpp` fix),
  `PostgresB1.*` 12/12, `AsyncTest.*`, `IRBuilderTest.*` (incl. the 4
  new parent tests below), `DbBridgeTest.*` (incl. the new
  `SqliteColStringValuesSurviveLaterReads`).

### Parent fix 1 — bare function as a value now fails loudly (compiler)

Slice C proved SP programs cannot register callbacks: a bare SP `fn`
used as a `ptr` argument lowered to literal null (the bridge silently
ignored it; the job landed in the DLQ with no error anywhere). Root
cause: `IRBuilder::visit(IdentifierExpr)` fell through to
`ConstantInt 0` for ANY unresolved identifier, including function
names. Fix (`compiler/src/ir/IRBuilder.cpp`): when the identifier
resolves to a declared function (`m_sema.getSymbolTable().lookupFunctions`),
report `error[E1000]` ("function 'handler' used as a value … function
pointers are not lowered yet") instead of emitting null. Verified live:
`spc build` on the callback program now fails with that E1000 at the
call site; `spc check` still passes it (check is syntax+sema only).
Test: `IRBuilderTest.BareFunctionNameAsValueFailsIRGeneration`.
Real function-pointer lowering is still future work — callbacks from SP
are still impossible, but now loudly impossible instead of silently
wrong. The Slice C doc section ("runtime works, SP blocked") stays
true; only the failure mode changed (loud, not silent).

### Parent fix 2 — `char` passed to a string method segfaulted (compiler) + the registry service's validators

Running slice D's registry service live exposed a SIGSEGV on the first
`POST /api/v1/publish` with a valid body. gdb backtrace:

```
__strstr_avx512(haystack="abcdefghijklmnopqrstuvwxyz0123456789", ned=0x72)
sp_rt_str_index_of() ← isValidPackageName() ← RegistryHandlers_publish()
```

`isValidPackageName` (and the other three validators in
`examples/registry-backend/src/json_util.sp`) called
`"alphabet".indexOf(ch)` with `ch` a `char` from `charAt()`.
`sp_rt_str_index_of(const char*, const char*)` received the char VALUE
`0x72` ('r') as the needle pointer. Root cause is pre-existing in
`main`: sema's string-method handling (`TypeChecker.cpp`) only sets
the return type and never checks argument types, and IRBuilder's
intrinsic string-method lowering passes arguments straight through as
`ptr`. Fix (compiler, `compiler/src/ir/IRBuilder.cpp` + `IRBuilder.h`):
new `checkStringArgs()` guard at all 13 intrinsic string-method call
sites (static `String.*` + member `"s".*`: `contains`, `indexOf`,
`startsWith`, `endsWith`, `replace`, `substring`, `split`, `length`,
`upper`/`lower`/`trim`/`reverse`) — a non-string, non-pointer sema
type in a string position is `error[E1000]` ("String.indexOf with a
'char' argument …"). Tests:
`IRBuilderTest.StringIndexOfWithCharArgFailsIRGeneration`,
`StringContainsWithCharArgFailsIRGeneration`, plus a positive control
`StringIndexOfWithStringArgStillCompiles`. Verified live: `spc build`
on `"abc".indexOf('b')` now fails with that E1000.

Service fix (`json_util.sp`): the four validators now test membership
with `==` chains (`isDigit`/`isLower`/`isUpper`/`isAlphaNum`/`isHex`
helpers). Char range comparisons (`c >= 'a'`) are NOT an option —
sema rejects them (E1001: operands must be numeric). `spc check` on the
service passes.

### Parent fix 3 — `sp_rt_sqlite_col_string` returned a dangling shared buffer (runtime)

With the segfault fixed, `spm publish` succeeded (201) — but the
response JSON was corrupted while the SQLite row itself was correct
(verified directly with python3/sqlite3: all fields stored correctly).
A minimal SP program isolated it: reading three columns printed
`name=3`, `version=3`, `desc=<correct last value>`. Root cause
(`runtime/src/db/DbBridge.cpp`): `sp_rt_sqlite_col_string` returned a
pointer to a single shared `static thread_local std::string`. SP
strings are bare `const char*` the caller keeps and never frees, so
every `getString` overwrote (and, for a longer later value,
reallocated and freed) the storage earlier pointers referenced. Fix:
return a fresh `malloc`'d copy via the file's existing `alloc_str`
helper — the same pattern the Postgres result accessors in the same
file already use, and the same convention as `sp_rt_str_*`. Applied
to `sp_rt_sqlite_col_string`, `sp_rt_sqlite_col_name`,
`sp_rt_sqlite_errmsg`, `sp_rt_pg_last_error`, `sp_rt_pg_res_error`,
`sp_rt_pg_res_command_tag`. Regression test:
`DbBridgeTest.SqliteColStringValuesSurviveLaterReads` (reads three
columns of increasing length, asserts all three AFTER all reads —
the old code fails it). The minimal SP program now prints all three
columns correctly.

### Registry live proof (now actually run, with Clang)

Service built with `spc build` and run on `127.0.0.1:8090`
(`SP_REGISTRY_TOKEN` set, fresh `/tmp/registry.db`). All steps from
the slice D README §"The `spm publish` proof", executed for real:

| Step | Result |
|---|---|
| `GET /api/health` | `{"status":"ok","service":"sp-registry","version":"3.1.0","backend":"sp","database":"sqlite","packages":0}` |
| `spm publish` (valid token) | **201, exit 0.** Package `registry-proof v0.1.0`, artifact 378 bytes, checksum `43a87084…b3df`. Response JSON now correct (see fix 3). |
| `spm publish` (wrong token) | **401 UNAUTHORIZED**, exit 1, nothing published |
| `spm publish` (no token) | Fails client-side, exit 1 |
| `spm publish` (same version again) | **409 VERSION_ALREADY_EXISTS**, exit 1 |
| `GET /api/v1/verify/registry-proof/0.1.0` before tarball | `verified:false`, `tarballStored:false` (honest) |
| `POST …/tarball` (base64 of the spm archive) | `verified:true`, sha256 matches published checksum |
| `GET /api/v1/verify/…` after upload | `verified:true`, `tarballStored:true` |
| `GET …/download` → `base64 -d` → `sha256sum` | `43a870842d7daaea1dd7c6c892fb11ceb56b022e5f2b6d552c02247f2f02b3df` — **exact match with the published checksum** |
| `GET /api/v1/packages` | Correct row, `count:1` |

The fake-registry claim from the v1 analysis is now retired for this
path: publish stores real metadata, verify hashes real bytes, download
returns the real tarball. The Next.js mock (`splang-registry/`) is
untouched and still the labelled preview.

### Real benchmark (against the live SP service)

`benchmarks/registry/load_test.py` against `GET /api/health`
(results in `benchmarks/registry/results/sp-registry-service-2026-11-09.json`):

- **250/250 succeeded, 1084.27 req/s measured**, latency mean 3.62 ms,
  p50 2.37 ms, p95 4.81 ms, p99 47.05 ms (concurrency 4, 4 CPUs,
  2026-11-09 05:23 UTC). This replaces slice D's Python-`http.server`
  number (149.93 req/s, labelled as the test server, NOT SP).
- A first 1000-request run returned 276× 200 + 724× **429** — the
  service's own global rate limiter (300 req/60s, documented in the
  service README) working as designed. Throughput above the limiter is
  not a service measurement.

### What is still NOT done (carried forward)

- Function-pointer lowering: callbacks from SP (queue handlers, cron
  tasks, ws routes, `HttpServer.route`) are still impossible; they now
  fail loudly at compile time instead of silently. The stdlib
  wrappers slice C specified (`stdlib/queue/queue.sp`,
  `stdlib/net/websocket.sp`) are still not written — they depend on
  this compiler work.
- SP-side `string[]` marshalling for `pg_exec_prepared` is declared
  but not verified by an SP program run (slice B1 limit, unchanged).
- `wss://` end-to-end test, WS client API, per-server WS route
  registry, durable auto-dispatch worker (slice C limits, unchanged).
- `NetTest.UdpSocketSendRecv` still hangs (now bounded by the ctest
  timeout, not fixed) and the 2 stdlib-time ExecutionTest failures
  are still pre-existing.
- `graphify-out/` still does not exist and `python -m graphify` is
  still not installed, so the repo AGENTS.md graph-update step cannot
  be run here (same as Phase 1).
- The example frontend API key removed in Phase 1 still needs
  rotation if it was ever used against a real service.
- ADR-001–005 still Proposed — Shanto's approval still pending.
