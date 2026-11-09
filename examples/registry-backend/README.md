# SP Registry Service — registry backend dogfooded in SP

A real package registry implemented as an **SP program** on the SP HTTP
server + SQLite (Phase 2, slice D). It is the real backend path for the
registry. The Next.js registry (`splang-registry/`) stays the labelled
preview/mock — this service does not change it, and the mock's 501 publish
/ `verified:false` behaviour is still correct for the mock.

## Run it

Requires a Clang backend toolchain (`spc build` / `spc run` compile SP to
a native binary through Clang; `g++` alone is rejected by the compiler).

```sh
cd examples/registry-backend

# 1. Set a publish token (required; the service refuses to start without one)
export SP_REGISTRY_TOKEN="$(openssl rand -hex 24)"   # >= 16 chars, never commit it

# 2. Run directly, or build first
spc run src/main.sp
# or: spc build src/main.sp -o registry-backend && ./registry-backend
```

Optional environment:

| Variable | Default | Meaning |
| --- | --- | --- |
| `SP_REGISTRY_TOKEN` | *(required)* | Bearer token checked on publish + tarball upload |
| `REGISTRY_PORT` | `8090` | Listen port |
| `REGISTRY_DB` | `registry.db` | SQLite file (created in the working directory) |
| `REGISTRY_OPENAPI_PATH` | `openapi.yaml` | Path of the spec served at `GET /openapi.yaml` |

## Endpoints

| Method & path | Auth | What it does |
| --- | --- | --- |
| `GET /api/health` | – | `{"status":"ok", ...}` + stored package count |
| `POST /api/v1/publish` | Bearer | Store package metadata (what `spm publish` sends). 201 on success, 409 if that version already exists. |
| `POST /api/v1/packages/:name/:version/tarball` | Bearer | Upload the tarball as base64 text. Stored only if its SHA-256 matches the published checksum. |
| `GET /api/v1/verify/:name/:version` | – | Real verification: decodes the stored tarball, SHA-256s it, compares with the published checksum. `verified:true` only on a match. |
| `GET /api/v1/packages` | – | List all published packages. |
| `GET /api/v1/packages/:name/:version` | – | Metadata for one version. |
| `GET /api/v1/packages/:name/:version/download` | – | Base64 tarball bytes (`X-Content-Encoding: base64`, `X-Sha256` header). 404 while no tarball was uploaded. |
| `GET /openapi.yaml` | – | This service's OpenAPI spec (generated, see below). |

## The `spm publish` proof

`spm publish` sends metadata only — JSON
`{name, version, description, license, checksum, archiveSizeBytes}` with an
`Authorization: Bearer <token>` header to `{registry}/api/v1/publish`. It
does **not** upload the tarball bytes, so after a successful publish the
package's checksum is *stored as claimed*, and `verify` reports
`verified:false` until the tarball is uploaded:

```sh
# 1. Start the service (token set as above), then publish a test package:
cd /tmp/sp-test-pkg            # any dir with sp.toml + src/
spm publish --registry http://127.0.0.1:8090 --token "$SP_REGISTRY_TOKEN"
# -> 201, package stored. (spm prints the registry's response.)

# 2. Same command with a wrong token -> the service answers 401 and spm
#    fails (exit code 1). Same with no token at all.

# 3. Verify before tarball upload -> verified:false (honest: nothing to hash yet)
curl http://127.0.0.1:8090/api/v1/verify/<name>/<version>

# 4. Upload the tarball spm archived (base64), then verify again -> verified:true
B64="$(base64 -w0 target/packages/<name>-<version>.tar.gz)"
curl -X POST http://127.0.0.1:8090/api/v1/packages/<name>/<version>/tarball \
  -H "Authorization: Bearer $SP_REGISTRY_TOKEN" -d "$B64"
curl http://127.0.0.1:8090/api/v1/verify/<name>/<version>

# 5. Download it back and compare hashes:
curl http://127.0.0.1:8090/api/v1/packages/<name>/<version>/download \
  | base64 -d | sha256sum     # must equal the published checksum
```

> **Status (updated 2026-11-09, parent verification):** steps 1–5 above
> HAVE now been executed live against the real SP service (Clang 18.1.3
> installed during Phase 2 verification). `spm publish` → 201, wrong/no
> token → 401/fail, duplicate → 409, verify `false` before tarball and
> `true` after upload, download round-trip SHA-256 matches the published
> checksum exactly. Two bugs found by the live run were fixed in the same
> branch: a compiler segfault on `indexOf(char)` (now a loud E1000) and
> corrupted SQLite read-back from a shared buffer in
> `sp_rt_sqlite_col_string` (now a fresh copy per call). Full results:
> `docs/v2/PHASE-31-PRO-4.0.0.md`, "Parent verification (final, all
> slices combined)". Real benchmark: 250/250 succeeded, 1084.27 req/s
> (`benchmarks/registry/results/sp-registry-service-2026-11-09.json`).

## Auth

- Publish and tarball upload require `Authorization: Bearer <token>`.
- The token is compared with `Crypto.timingSafeEqual` (constant-time).
- Missing, malformed, or wrong tokens get `401`. There is no "allow all
  when unconfigured" mode: without `SP_REGISTRY_TOKEN` the service exits
  at startup instead of serving unauthenticated writes.
- The token is never written to the database, the logs, or any file in
  this repo. Rotate it by restarting the service with a new value.
  (Existing stored packages are unaffected — the token is not per-package.)

## Storage

- SQLite, one table `packages`, one row per `name@version`.
- Releases are immutable: re-publishing the same version returns `409`.
- Tarball bytes are stored as **base64 TEXT**, not a BLOB: the SP SQLite
  API has no blob binding and reads TEXT back through a C string, so raw
  bytes are never stored. The stored base64 is hashed in memory for
  verification; download returns the same base64 text.
- One connection is opened per request and closed before responding
  (same pattern as `examples/backend-crud-app`). Concurrent publishes are
  serialized by SQLite's WAL + busy timeout; this is a small registry, not
  a high-write service.

## OpenAPI

`openapi.yaml` is **generated** from the route table — it cannot drift
from the service without the checker noticing:

```sh
python3 generate_openapi.py   # regenerate from src/routes.sp
python3 check_openapi.py      # verify spec <-> routes.sp, both directions
```

## Files

| File | What it is |
| --- | --- |
| `sp.toml` | Package manifest (bin, entry `src/main.sp`) |
| `src/main.sp` | Entrypoint: fail-closed startup, middleware, `app.run(port)` |
| `src/config.sp` | Env config (`SP_REGISTRY_TOKEN`, `REGISTRY_PORT`, `REGISTRY_DB`, `REGISTRY_OPENAPI_PATH`) |
| `src/routes.sp` | The route table — single source of truth for the spec |
| `src/handlers.sp` | One handler per route; all responses built from SQLite |
| `src/store.sp` | SQLite access: schema, publish, lookup, tarball, verify |
| `src/auth.sp` | Bearer check with timing-safe compare, 401 helper |
| `src/json_util.sp` | JSON escaping + name/version/checksum/base64 validation |
| `generate_openapi.py` | Spec generator (parses `src/routes.sp`) |
| `check_openapi.py` | Spec <-> routes consistency checker |
| `openapi.yaml` | Generated OpenAPI 3.0.3 spec (8 routes) |

## Not done (deliberate)

- **No TLS.** The service listens on plain HTTP on localhost. Publishing
  over a network needs TLS termination in front of it (slice A work) or
  the runtime HTTP client/server TLS support — not this slice.
- **No per-user accounts.** One shared publish token, not per-publisher
  credentials or scopes. Multi-tenant auth is out of scope.
- **No rate limiting per token.** A global 300 req/60s limiter is on; the
  Next.js mock's validation rules (name/version/checksum shape) are
  mirrored, but there is no CAPTCHA, no upload size cap beyond the HTTP
  server's body limit, and no virus scanning of tarballs.
- **No Postgres backend.** SQLite only, by design for this dogfood step.
- **No `spm install` integration.** `spm publish` is the proven path;
  resolving/installing from this registry is future work.
