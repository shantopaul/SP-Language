# SP 3.0 Enterprise Backend Platform Guide

The definitive production guide to building ultra-high-throughput, cloud-native backend services, microservices, and distributed real-time systems using the SP Programming Language (SP 3.0 Milestone).

---

## 1. Architectural Philosophy

SP Lang combines **zero-cost native performance** (LLVM ahead-of-time compilation, sub-microsecond routing, lock-free concurrency) with the **developer ergonomics of modern scripting languages** (Go/TypeScript-level expressiveness, dependency injection, and fluent assertions).

Key pillars:
- **Zero-Allocation HTTP Context & Routing**: Radix tree path indexing with sub-microsecond resolution.
- **In-Memory Testing Harness**: Zero socket port allocation, complete middleware stack integration in `< 1ms`.
- **Integrated OpenAPI 3.0.3**: Automated doc generation from code and comments, embedded Swagger UI and Redoc.
- **Hardened Cryptography**: Argon2id password hashing, JWT HMAC-SHA256, constant-time equality.
- **Enterprise Observability**: Prometheus `/metrics` registry, structured JSON logging, Kubernetes `/healthz` & `/readyz`.
- **Minimal Containerization**: Static binaries linking against musl libc producing `< 15MB` scratch images.

---

## 2. High-Level Backend Framework (`std.web`)

The official high-level backend application framework is centered around the `App` and `Context` classes.

### Minimal Application Example

```sp
module main;

import std.io;
import std.web;

fn main() {
    let app = new App();

    // Production middleware pipeline
    app.use(new Logger());
    app.use(new Cors());
    app.use(new Recovery());

    // Interactive OpenAPI & Swagger UI
    app.swagger("/docs", "/openapi.json", "E-Commerce API");
    app.redoc("/redoc", "/openapi.json", "E-Commerce API");

    // Route groups
    let v1 = app.group("/api/v1");

    v1.get("/health", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\"status\":\"UP\"}");
    });

    v1.get("/users/:id", (ptr req) => {
        let ctx = Context.create(req);
        let userId = ctx.param("id");
        return ctx.json("{\"id\":\"" + userId + "\",\"name\":\"Alice\"}");
    });

    app.listen(8080);
}
```

### Context (`ctx`) Ergonomics

| Method | Description |
|---|---|
| `ctx.method()` | Returns HTTP method (`GET`, `POST`, etc.) |
| `ctx.path()` | Returns request URL path |
| `ctx.body()` | Returns raw request body |
| `ctx.param(name)` | Radix-tree path parameter extraction (e.g. `:id`) |
| `ctx.query(name)` | Query string parameter (e.g. `?limit=10`) |
| `ctx.header(name)` | Case-insensitive request header lookup |
| `ctx.cookie(name)` | Cookie value lookup |
| `ctx.json(jsonStr, status=200)` | Returns JSON HTTP response |
| `ctx.text(str, status=200)` | Returns plain text HTTP response |
| `ctx.html(htmlStr, status=200)` | Returns HTML HTTP response |
| `ctx.sse(data)` | Returns Server-Sent Events stream chunk |
| `ctx.redirect(url, status=302)` | Returns redirect response |

---

## 3. In-Memory Integration Testing (`std.test.http`)

Unit testing real HTTP servers usually suffers from TCP port collisions, firewall popups, and slow socket setup. SP provides a native **in-memory HTTP test harness** that invokes the HTTP server dispatch pipeline directly in memory without binding sockets.

### Fluent Test Example

```sp
module tests.user_api_test;

import std.web;
import std.test.http;

test "User API Endpoint Verification" {
    let app = new App();
    app.get("/api/v1/users/:id", (ptr req) => {
        let ctx = Context.create(req);
        let id = ctx.param("id");
        return ctx.json("{\"id\":\"" + id + "\",\"status\":\"active\"}");
    });

    // Zero socket binding: tests run directly in memory in < 1ms
    let client = TestClient.fromApp(app);

    let res = client.get("/api/v1/users/42");
    res.assertOk()
       .assertHeader("Content-Type", "application/json")
       .assertBodyContains("status")
       .assertBodyContains("42");

    res.destroy();
    client.destroy();
}
```

### Fluent Assertions

- `res.assertStatus(expected)`
- `res.assertOk()` (200)
- `res.assertCreated()` (201)
- `res.assertNoContent()` (204)
- `res.assertBadRequest()` (400)
- `res.assertUnauthorized()` (401)
- `res.assertForbidden()` (403)
- `res.assertNotFound()` (404)
- `res.assertInternalError()` (500)
- `res.assertBody(expectedString)`
- `res.assertBodyContains(subString)`
- `res.assertHeader(headerName, expectedValue)`
- `res.assertHeaderContains(headerName, subString)`
- `res.assertHeaderPresent(headerName)`

---

## 4. OpenAPI 3.0.3 & Swagger UI

SP comes with an automated OpenAPI 3.0.3 generator tool (`spc doc --openapi` and `spc doc --swagger`) and runtime UI viewers.

### Command Line Generation

```bash
# Generate OpenAPI 3.0.3 JSON
spc doc src/main.sp --openapi -o docs/openapi.json

# Generate Swagger UI viewer HTML
spc doc src/main.sp --swagger -o docs/

# Generate Redoc viewer HTML
spc doc src/main.sp --redoc -o docs/
```

### Doc Comment Annotations

Routes and parameters can be documented directly above handlers:

```sp
/// @route GET /api/v1/users/{id}
/// @summary Get user profile by ID
/// @tag Users
/// @param id path integer User primary key
/// @response 200 User profile returned successfully
/// @response 404 User not found
app.get("/api/v1/users/:id", (ptr req) => { ... });
```

---

## 5. Security & Authentication Architecture

Production backends require defense-in-depth:
- **Argon2id (RFC 9106)**: `std.crypto.password.Password.hash(pwd)` and `Password.verify(pwd, hash)`.
- **JWT HS256**: `std.security.jwt.Jwt.sign(payload, secret)` and `Jwt.verify(token, secret)`.
- **Constant-Time Comparison**: `sp_rt_crypto_timing_safe_equal(a, b)` preventing side-channel attacks.
- **Cookie Security**: Signed cookies via HMAC preventing client-side tampering.
- **Middleware**: `JwtAuthMiddleware`, `RoleAuthMiddleware`, `SecurityHeadersMiddleware`.

---

## 6. Observability, Telemetry & Background Workers

### Prometheus Registry (`std.metrics`)
Scrape endpoint standard `/metrics` exposing:
- `Counter`: HTTP request counts, DB errors
- `Gauge`: Connection pool in-use, active WebSocket sessions
- `Histogram`: Request latency p50, p90, p99

### Structured Logging (`std.log`)
JSON structured logs with severity filtering, timestamp ISO 8601, rotating file appender, and console output.

### Background Queue (`std.queue`)
In-process thread-pool workers, exponential backoff retries, and dead-letter queue.

---

## 7. Static Compilation & Docker Scratch Container

SP compiles down to native machine code via LLVM. Linking statically against musl libc creates a self-contained single binary with zero runtime dependencies.

### Multi-stage Dockerfile
```dockerfile
FROM debian:bookworm-slim AS builder
WORKDIR /app
COPY . .
RUN spc build src/main.sp --static --release -o server

FROM scratch
WORKDIR /app
COPY --from=builder /app/server /app/server
EXPOSE 8080
ENTRYPOINT ["/app/server"]
```
Total final image footprint: **~12.5 MB**.

---

## 8. TechEmpower Benchmarks Summary

| Benchmark | SP 3.0 Throughput | p50 Latency | p99 Latency |
|---|---|---|---|
| Plaintext | 185,000 req/s | 0.28 ms | 0.85 ms |
| JSON Serialization | 142,000 req/s | 0.36 ms | 0.98 ms |
| Single Query | 118,000 req/s | 0.45 ms | 1.20 ms |
| Multiple Queries (20) | 24,500 req/s | 1.65 ms | 3.40 ms |
| Fortunes (DB+HTML) | 68,000 req/s | 0.78 ms | 1.95 ms |

Reproduce locally via:
```powershell
.\benchmarks\backend\run_benchmarks.ps1
```
