module main;

import std.io;
import std.web;
import std.os;
import config;
import routes;
import store;

// ============================================================================
// SP Registry Service — the registry backend dogfooded in SP (Phase 2, slice D).
//
// What it is: a real package registry (publish / verify / info / download)
// implemented as an SP program on the SP HTTP server + SQLite. It replaces
// nothing by itself: the Next.js registry stays the labelled preview/mock,
// and this service is the real backend path (see README.md).
//
// How to run:
//   SP_REGISTRY_TOKEN=<long random secret> spc run src/main.sp
//   (or build first: spc build src/main.sp -o registry-backend)
//   Optional: REGISTRY_PORT=8090 (default), REGISTRY_DB=registry.db (default),
//   REGISTRY_OPENAPI_PATH=openapi.yaml (default; run from this directory).
//
// The service refuses to start without SP_REGISTRY_TOKEN (fail closed).
// ============================================================================

fn main() {
    println("=================================================================");
    println("   SP Registry Service 3.1.0 — registry backend dogfooded in SP  ");
    println("=================================================================");

    let token = RegistryConfig.token();
    if (token == "") {
        println("[!] SP_REGISTRY_TOKEN is not set. Refusing to start without a publish token.");
        println("    Set SP_REGISTRY_TOKEN to a long random secret before starting.");
        OS.exit(1);
    }
    if (token.length() < 16) {
        println("[!] SP_REGISTRY_TOKEN is too short (minimum 16 characters). Refusing to start.");
        OS.exit(1);
    }

    let dbPath = RegistryConfig.dbPath();
    if (!RegistryStore.initSchema(dbPath)) {
        println("[!] Could not initialize registry database at: " + dbPath);
        OS.exit(1);
    }
    println("[✓] Registry database ready: " + dbPath);

    let app = new App();
    app.useLogger();
    app.useRecovery();
    app.useRequestId();
    app.useRateLimiter(300, 60);

    RegistryRoutes.registerRoutes(app);

    let port = RegistryConfig.port();
    println("[+] SP Registry listening on http://127.0.0.1:" + port);
    println("    * Health:    GET  /api/health");
    println("    * Publish:   POST /api/v1/publish (Bearer token required)");
    println("    * Tarball:   POST /api/v1/packages/:name/:version/tarball (Bearer, base64 body)");
    println("    * Verify:    GET  /api/v1/verify/:name/:version");
    println("    * List:      GET  /api/v1/packages");
    println("    * Info:      GET  /api/v1/packages/:name/:version");
    println("    * Download:  GET  /api/v1/packages/:name/:version/download");
    println("    * OpenAPI:   GET  /openapi.yaml");
    println("=================================================================");

    app.run(port);
}
