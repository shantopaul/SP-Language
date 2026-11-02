# SP Lang — Professional Backend Platform Implementation Plan

> **Document Version:** 1.0.0-GA  
> **Date:** October 8, 2026  
> **Status:** Architecture Review & Approval Phase  
> **Lead Maintainer:** Shanto Paul  
> **Primary Location:** [`Phases.md`](planning/PHASES.md) (Part III: Phases 19–28) & [`docs/BACKEND_PLATFORM_IMPLEMENTATION_PLAN.md`](BACKEND_PLATFORM_IMPLEMENTATION_PLAN.md)

---

## 1. Executive Summary & Purpose

SP Lang has achieved **SP 2.0.0 General Availability** with an in-process LLVM 18 compiler backend, deterministic ARC memory safety (zero GC pauses), and a 100% CTest pass rate (346/346 continuous verification tests).

This document outlines the **complete, production-grade technical roadmap** to transition SP Lang into an **industrial-strength backend programming language**.

### Target Architecture
```text
React / Next.js / Mobile Frontend
              │
              │ HTTPS / REST / JSON / WebSockets
              ▼
        ┌──────────────┐
        │   SP Lang    │
        │   Backend    │
        └──────┬───────┘
               │
       ┌───────┼────────┐
       ▼       ▼        ▼
   PostgreSQL Redis  External APIs
    / SQLite
```

---

## 2. Ground-Truth Codebase Audit Findings

A comprehensive empirical audit of the repository was conducted before authoring this plan:

1. **Compiler Pipeline (`compiler/`):**
   - **Lexer/Parser:** Hand-written recursive descent + Pratt precedence parser. Fully handles modern syntax (classes, inheritance, interfaces, generics, pattern matching, comptime, unsafe, extern C, and `?` operator).
   - **CRITICAL DISCOVERY — Single-File Compilation Only:** `NameResolver.cpp` records imported module strings in `m_importedModules`, but does **not** load or parse `.sp` files on disk. Standard library modules in `stdlib/` (like `stdlib/http/http.sp` and `stdlib/json/json.sp`) are not automatically loaded when user code writes `import std.http;`.
   - **CRITICAL DISCOVERY — Await Synchronous Lowering:** In `IRBuilder.cpp:6655`, `visit(const ast::AwaitExpr& node)` compiles `node.getExpr().accept(*this);` synchronously. Coroutine state machines and async event loop yields are not yet lowered to native tasks.
   - **Codegen & Linking:** High quality. LLVM 18 IR generation, `-O0` to `-O3` passes, object emission, and native linking via Clang/LLD with `-pthread` and `-lws2_32`.

2. **Native Runtime (`runtime/` & `libsp_runtime.a`):**
   - **Memory Model:** Deterministic ARC (`sp_rt_arc_alloc`, `sp_rt_retain`, `sp_rt_release`) with atomic strong/weak count headers. Instantaneous memory reclamation, no GC pauses, flat RAM usage.
   - **Threading:** Cross-platform work-stealing `ThreadPool` and lock-free typed `Channel`.
   - **Sockets & HTTP:** Sockets in `Net.cpp`, basic HTTP parser and `HttpServer` in `Http.cpp`, DOM JSON in `Json.cpp`, and SHA-256/HMAC/UUID in `Crypto.cpp`.

3. **Standard Library (`stdlib/`):**
   - Pure SP wrapper files exist (`http.sp`, `json.sp`, `net.sp`, `crypto.sp`, etc.), ready to be used once multi-file module loading is enabled in the compiler.

4. **Package Manager & CLI (`tools/spc/`, `tools/spm/`):**
   - `spc` CLI supports: `run`, `build`, `check`, `fmt`, `test`, `bench`, `doc`, `bindgen`, `doctor`, `fix`.
   - `spm` CLI supports: `init`, `add`, `remove`, `install`, `update`, `build`, `run`, `clean`, `verify`, `tree`, `cache`, `publish`, `search`.

---

## 3. Backend Capability Matrix

| Capability | Current State | Missing Components | Difficulty | Priority |
|---|---|---|---|---|
| **Multi-File Module System** | AST parses `import`, but no file loader | Module loader, file resolution, circular import detection, multi-file AST merging | Medium | **P0 (Blocker)** |
| **HTTP Server** | C++ runtime `HttpServer` (basic routing) | HTTP/1.1 keep-alive, streaming body, multipart, connection pooling, graceful shutdown | Medium | **P0** |
| **HTTP Client** | Basic C++ client (`get`, `post`) | Custom headers, proxy, connection pool, TLS/HTTPS, timeouts, streaming | Low-Med | **P1** |
| **Router** | Basic string split `/path/:id` | Radix-tree router, route groups (`/api/v1`), nested routers, 405 Method Not Allowed | Medium | **P0** |
| **Middleware** | Basic C++ vector of callbacks | SP-level middleware abstraction, onion pipeline, `next()` execution, recovery | Medium | **P0** |
| **JSON** | C++ DOM parser/serializer | Struct reflection / typed schema mapping, fast zero-copy deserialization, validation errors | Medium | **P0** |
| **Async/Await & Event Loop** | Keywords parsed, lowered synchronously | State machine coroutine lowering or fiber/event-loop integration, async I/O dispatch | High | **P1** |
| **Concurrency** | Work-stealing `ThreadPool`, `Channel<T>` | High-level worker pipeline abstraction, structured cancellation | Low-Med | **P1** |
| **PostgreSQL** | Missing (only in docs) | Pure SP or libpq FFI driver, wire protocol / C binding, binary parameter encoding | High | **P0 (Tier 1 DB)** |
| **SQLite** | Bindgen tests exist for headers | Bundled sqlite3 C amalgamation, SP `SqliteConnection` wrapper, prepared statements | Medium | **P0 (Tier 2 DB)** |
| **MySQL / MariaDB** | Missing | Pure SP or libmysqlclient FFI driver | High | **P2** |
| **Connection Pool** | Missing | Generic connection pool (`Pool<T>`), max connections, idle timeouts, checkout/release | Medium | **P0** |
| **Transactions** | Missing | `BEGIN`, `COMMIT`, `ROLLBACK` wrappers, auto-rollback on error | Low-Med | **P0** |
| **Query Builder** | Missing | Fluent query builder (`select()`, `where()`, `join()`), SQL injection prevention | Medium | **P1** |
| **ORM / Data Models** | Missing | Struct mapping, relation loading, migration schema definition | High | **P2** |
| **Authentication** | Missing | Password hashing, token handling, session store | Medium | **P0** |
| **Password Hashing** | SHA-256 exists in runtime | Argon2id / bcrypt implementation or C binding (e.g. libsodium / argon2) | Medium | **P0** |
| **JWT** | Missing (mock token in tests) | HMAC-SHA256 JWT encoding, payload JSON parsing, signature verification, expiry check | Low-Med | **P0** |
| **Sessions & Cookies** | Headers can be set manually | `Set-Cookie` parser/builder, signed cookies, HttpOnly/SameSite/Secure, session store | Low-Med | **P0** |
| **CORS** | Missing | Standard CORS middleware (Allow-Origin, Allow-Methods, Allow-Headers, Preflight OPTIONS) | Low | **P0** |
| **CSRF** | Missing | CSRF token generation, header/cookie validation middleware | Low-Med | **P1** |
| **Rate Limiting** | Missing | Token bucket / sliding window rate limiting middleware | Low-Med | **P1** |
| **Validation** | Missing | Request payload validation schema, field errors reporting (RFC 7807) | Medium | **P1** |
| **File Upload** | Missing | Multipart/form-data parser, temp file storage, streaming upload | Medium | **P1** |
| **WebSocket** | Missing | RFC 6455 framing, upgrade handshake, event dispatch | High | **P2** |
| **SSE (Server-Sent Events)** | Missing | Chunked streaming HTTP response helper | Low-Med | **P1** |
| **Redis** | Missing | Redis RESP protocol parser & TCP socket client, connection pool | Medium | **P1** |
| **Background Jobs & Queue**| `ThreadPool` exists in runtime | Persistent queue, retry policy, dead letter queue, job scheduler | Medium | **P2** |
| **Logging** | Console `println` exists | Structured JSON logger, log levels (DEBUG/INFO/WARN/ERROR), timestamps, file output | Low-Med | **P0** |
| **Metrics & Tracing** | High-res timer exists | Prometheus `/metrics` exporter, request ID propagation, latency percentiles | Medium | **P1** |
| **OpenAPI** | Missing | Route introspection to OpenAPI 3.0 JSON/YAML specification | Medium | **P2** |
| **Testing** | CTest (346 tests), `spc test` | HTTP mock client, test harness for endpoints (`TestServer`, `TestClient`) | Medium | **P1** |
| **Production Build** | `spc build -O3`, `spm build --release` | Dockerfile template, scratch container build, musl/static link | Low-Med | **P0** |

---

## 4. Phase-by-Phase Implementation Roadmap

The implementation is structured into 10 focused phases:

```text
Phase 19: Multi-File Compilation & Module System (SP 2.1)
   │
   ├── Phase 20: Production HTTP/1.1 Engine & Streaming (SP 2.2)
   │      │
   │      ├── Phase 21: High-Performance Routing, Groups & Middleware (SP 2.3)
   │      │
   │      ├── Phase 22: Zero-Copy JSON & Typed Serialization (SP 2.4)
   │      │
   │      ├── Phase 23: Tier-1 Databases & Connection Pooling (PostgreSQL & SQLite) (SP 2.5)
   │      │
   │      └── Phase 24: Security, Authentication & Cryptography (JWT, Argon2id, CORS) (SP 2.6)
   │
   └── Phase 25: Official Backend Framework & Context Ergonomics (SP 2.7)
          │
          ├── Phase 26: Real-Time Web & Distributed Caching (WebSocket & Redis) (SP 2.8)
          │
          ├── Phase 27: Observability, Background Workers & Production DevOps (SP 2.9)
          │
          └── Phase 28: Developer Experience, OpenAPI & TechEmpower Benchmarks (SP 3.0)
```

---

### Phase 19: Multi-File Compilation & Module System (SP 2.1)
- **Goal:** Enable multi-file project compilation and standard library import resolution (`import std.http;`, `import std.json;`, `import routes.users;`).
- **Key Deliverables:** `ModuleLoader` class, circular dependency detector, AST merger, and updated `spc build` / `spm build` to compile multi-file directory trees.

### Phase 20: Production HTTP/1.1 Engine & Streaming Server (SP 2.2)
- **Goal:** Transform the HTTP engine into an enterprise server with RFC 7230 compliance, keep-alive connection reuse, chunked streaming bodies, query string decoding, and graceful shutdown (`SIGINT`/`SIGTERM`).
- **Key Deliverables:** Hardened `runtime/src/http/Http.cpp` and `stdlib/http/http.sp`, 100k+ req/sec benchmark.

### Phase 21: High-Performance Routing, Route Groups & Middleware Pipeline (SP 2.3)
- **Goal:** Sub-microsecond Radix-Tree router with parameterized paths (`:id`), wildcard paths, route groups (`/api/v1`), and an onion-style middleware pipeline with `next()`.
- **Key Deliverables:** Pure SP Router, RouteGroup, and 5 built-in middlewares (`Cors`, `Logger`, `Recovery`, `RequestId`, `RateLimiter`).

### Phase 22: Zero-Copy JSON & Typed Data Serialization (SP 2.4)
- **Goal:** Direct SP class/struct JSON encoding and decoding, `@json` annotations, and request validation schema engine with RFC 7807 problem details error responses.
- **Key Deliverables:** Fast JSON serializer, `Validator<T>`, `@json_ignore`.

### Phase 23: Tier-1 Databases & Connection Pooling (PostgreSQL & SQLite) (SP 2.5)
- **Goal:** Bundled SQLite3 C amalgamation driver, asynchronous PostgreSQL wire protocol/libpq driver, thread-safe connection pooling, safe transaction manager (`db.transaction((tx) => ...)`), and parameterized SQL query builder.
- **Key Deliverables:** `std.db.sqlite`, `std.db.postgres`, `std.db.pool.ConnectionPool<T>`, `std.db.sql`.

### Phase 24: Security, Authentication & Cryptography (SP 2.6)
- **Goal:** Production auth stack: Argon2id password hashing, JWT creation & validation (HS256/RS256), HMAC signed cookies (`HttpOnly`, `SameSite=Strict`), and session stores.
- **Key Deliverables:** `std.crypto.password`, `std.security.jwt`, `std.security.session`, security headers middleware.

### Phase 25: Official Backend Framework & Context Ergonomics (SP 2.7)
- **Goal:** Unify router, middleware, and database into an ergonomic, modern framework:
  ```sp
  import std.web;

  fn main() {
      let app = new App();
      app.use(new Logger());
      app.use(new Cors());

      app.get("/api/users", (ctx) => ctx.json({ "users": [] }));
      app.listen(8080);
  }
  ```
- **Key Deliverables:** `std.web.App`, `Context` (`ctx.json`, `ctx.param`, `ctx.query`), `sp dev` hot-reload runner.

### Phase 26: Real-Time Web & Distributed Caching (WebSocket & Redis) (SP 2.8)
- **Goal:** RFC 6455 WebSocket server, Server-Sent Events (SSE) streaming, and high-performance Redis client with RESP parsing, connection pool, and Pub/Sub.
- **Key Deliverables:** `std.net.ws`, `std.web.sse`, `std.redis`.

### Phase 27: Observability, Background Workers & Production DevOps (SP 2.9)
- **Goal:** Structured JSON logging, Prometheus `/metrics` exporter, in-process background worker queues with exponential retries, and sub-15MB minimal static Docker containers (`musl` build).
- **Key Deliverables:** `std.log`, `std.metrics`, `std.queue`, static production Dockerfile.

### Phase 28: Developer Experience, OpenAPI & Production Benchmarks (SP 3.0)
- **Goal:** Enterprise GA: automated OpenAPI 3.0 documentation generator (`spc doc --openapi`), in-memory HTTP endpoint test client (`std.test.http`), and certified TechEmpower benchmark comparisons against Go and Rust.
- **Key Deliverables:** SP 3.0 Enterprise Milestone, Swagger UI integration, official backend tutorials.

---

## 5. Risk Analysis & Mitigation

1. **Backwards Compatibility:** All 346 existing CTest targets must continue to compile and pass with 100% success rate throughout all phases.
2. **Memory Safety:** Every request lifecycle uses deterministic ARC; continuous 24-hour load testing under AddressSanitizer (ASan) will ensure zero memory growth over time.
3. **Cryptographic Integrity:** Standard audited implementations (Argon2id, OpenSSL/libsodium primitives) will be linked rather than custom implementations.
4. **SQL Injection Defense:** All database drivers will mandate parameterized statements (`?` or `$1`), strictly forbidding raw query interpolation.

---

## 6. Definition of Done ("Professional Backend Ready")

SP Lang will be officially certified **Professional Backend Ready** when:
1. A developer can build a complete SaaS API with **Next.js frontend communication** (JWT authentication, PostgreSQL/SQLite CRUD, CORS, JSON validation).
2. The entire project compiles across multiple source files via `spm build` into a single standalone binary.
3. The server handles **85,000+ requests/second** with sub-2ms latency, flat memory footprint, and zero GC pauses.
4. Deployable into an isolated **sub-15MB Docker container** with Prometheus telemetry and structured JSON logs.
5. All 346+ continuous integration tests pass with 100% success rate.
