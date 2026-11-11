module tests.auth_test;

import std.os;
import std.fs;
import std.io;
import std.json;
import std.http;
import std.web;
import db.schema;
import routes.router;
import services.auth_service;
import util.json;

fn cleanupDbFiles(string basePath) {
    if (exists(basePath)) { deleteFile(basePath); }
    if (exists(basePath + "-wal")) { deleteFile(basePath + "-wal"); }
    if (exists(basePath + "-shm")) { deleteFile(basePath + "-shm"); }
}

fn setupTestEnv(string dbPath) {
    cleanupDbFiles(dbPath);
    OS.setEnv("TODOLIST_DB_PATH", dbPath);
    OS.setEnv("JWT_SECRET", "test-only-jwt-secret-key-at-least-32-bytes");
    OS.setEnv("JWT_TTL_SECONDS", "3600");
    OS.setEnv("RATE_LIMIT_MAX", "1000");
    let ok = DatabaseManager.initSchema();
    if (!ok) {
        panic("Failed to initialize test SQLite schema");
    }
}

fn assertTrue(bool cond, string msg) {
    if (!cond) {
        panic("Assertion failed: " + msg);
    }
}

fn testLiveHttpHealthAndAuthRoutes() {
    let dbPath = "build/test_auth_http.db";
    setupTestEnv(dbPath);

    let app = Router.buildApp();
    let listening = app.listen(18081);
    assertTrue(listening, "Expected app.listen(18081) to succeed");
    let base = "http://127.0.0.1:18081";

    // 1. Health & probes over real HTTP
    let rHealth = HttpClient.get(base + "/api/health");
    assertTrue(rHealth.status() == 200, "GET /api/health must return 200");
    assertTrue(rHealth.body().contains("\"status\":\"healthy\""), "health status must be healthy");
    rHealth.destroy();

    let rLive = HttpClient.get(base + "/healthz");
    assertTrue(rLive.status() == 200, "GET /healthz must return 200");
    rLive.destroy();

    let rReady = HttpClient.get(base + "/readyz");
    assertTrue(rReady.status() == 200, "GET /readyz must return 200");
    rReady.destroy();

    // 2. Unauthenticated protected route must return 401
    let rUnauth = HttpClient.get(base + "/api/v1/todos");
    assertTrue(rUnauth.status() == 401, "GET /api/v1/todos without token must return 401");
    rUnauth.destroy();

    // 3. Invalid registration returns 422 Problem Details
    let rBadReg = HttpClient.post(
        base + "/api/v1/auth/register",
        "{\"name\":\"A\",\"email\":\"bad\",\"password\":\"123\"}"
    );
    assertTrue(rBadReg.status() == 422, "Invalid register must return 422");
    assertTrue(rBadReg.body().contains("\"errors\":["), "422 must include field errors");
    rBadReg.destroy();

    // 4. Register valid user returns 201 and JWT
    let rReg = HttpClient.post(
        base + "/api/v1/auth/register",
        "{\"name\":\"Alice Johnson\",\"email\":\"Alice@Example.com\",\"password\":\"StrongPass#123\"}"
    );
    assertTrue(rReg.status() == 201, "Valid register must return 201");
    let regBody = rReg.body();
    assertTrue(!regBody.contains("password_hash"), "Register must never return password_hash");
    assertTrue(regBody.contains("\"email\":\"alice@example.com\""), "Email must normalize to lowercase");
    assertTrue(regBody.contains("\"access_token\":\""), "Register must return JWT access_token");
    rReg.destroy();

    // 5. Duplicate email (case-insensitive) returns 409
    let rDup = HttpClient.post(
        base + "/api/v1/auth/register",
        "{\"name\":\"Alice Two\",\"email\":\"ALICE@example.com\",\"password\":\"StrongPass#123\"}"
    );
    assertTrue(rDup.status() == 409, "Duplicate register must return 409");
    rDup.destroy();

    // 6. Wrong password login returns 401
    let rBadLogin = HttpClient.post(
        base + "/api/v1/auth/login",
        "{\"email\":\"alice@example.com\",\"password\":\"WrongPassword!\"}"
    );
    assertTrue(rBadLogin.status() == 401, "Wrong password login must return 401");
    rBadLogin.destroy();

    // 7. Valid login returns 200 and verifiable JWT
    let rGoodLogin = HttpClient.post(
        base + "/api/v1/auth/login",
        "{\"email\":\"alice@example.com\",\"password\":\"StrongPass#123\"}"
    );
    assertTrue(rGoodLogin.status() == 200, "Valid login must return 200");
    let parsed = Json.parse(rGoodLogin.body());
    assertTrue(parsed.isValid(), "Login response must be valid JSON");
    let token = JsonUtil.getString(parsed, "access_token", "");
    parsed.destroy();
    rGoodLogin.destroy();
    assertTrue(token.length() > 20, "Login must issue non-empty JWT");

    let verifiedUser = AuthService.verifyBearerHeader("Bearer " + token);
    assertTrue(verifiedUser.found, "Issued JWT must verify cleanly");
    assertTrue(verifiedUser.email == "alice@example.com", "Verified user email must match");

    app.stop();
    app.close();
    cleanupDbFiles(dbPath);
    println("[PASS] live HTTP health, validation, register, login, and JWT verification");
}

fn testAuthServiceProfileAndNegativeCases() {
    let dbPath = "build/test_auth_service.db";
    setupTestEnv(dbPath);

    let reg = AuthService.register("Bob Builder", "bob@example.com", "BuilderSecret99");
    assertTrue(reg.ok && reg.statusCode == 201, "Service register should succeed");

    let profOk = AuthService.updateProfile(reg.user.id, "Robert Builder");
    assertTrue(profOk.ok && profOk.user.name == "Robert Builder", "Profile name should update");

    let profBad = AuthService.updateProfile(reg.user.id, "X");
    assertTrue(!profBad.ok && profBad.statusCode == 422, "Short name should fail with 422");

    let invalidTok = AuthService.verifyBearerHeader("Bearer invalid.jwt.token");
    assertTrue(!invalidTok.found, "Invalid JWT must not authenticate");

    cleanupDbFiles(dbPath);
    println("[PASS] AuthService profile update and token rejection");
}

fn main() -> int {
    testLiveHttpHealthAndAuthRoutes();
    testAuthServiceProfileAndNegativeCases();
    println("All auth tests passed (2/2).");
    return 0;
}
