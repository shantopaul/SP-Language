module std.test.http;

import std.io;
import std.string;
import std.http;
import std.web.app;

// ============================================================================
// Standard Library HTTP In-Memory Test Harness (std.test.http) (SP 3.0)
// High-performance, zero-port-allocation integration testing for HTTP servers.
// Supports fluent assertions: assertStatus, assertOk, assertCreated,
// assertNotFound, assertBadRequest, assertBody, assertBodyContains, etc.
// ============================================================================

extern "C" {
    fn sp_rt_test_client_create(ptr serverHandle) -> ptr;
    fn sp_rt_test_client_request(ptr clientHandle, string method, string path, string body, string headersJson) -> ptr;
    fn sp_rt_test_client_get(ptr clientHandle, string path) -> ptr;
    fn sp_rt_test_client_post(ptr clientHandle, string path, string body) -> ptr;
    fn sp_rt_test_client_put(ptr clientHandle, string path, string body) -> ptr;
    fn sp_rt_test_client_delete(ptr clientHandle, string path) -> ptr;
    fn sp_rt_test_client_patch(ptr clientHandle, string path, string body) -> ptr;
    fn sp_rt_test_client_destroy(ptr clientHandle);

    fn sp_rt_test_response_status(ptr respHandle) -> int;
    fn sp_rt_test_response_body(ptr respHandle) -> string;
    fn sp_rt_test_response_header(ptr respHandle, string name) -> string;
    fn sp_rt_test_response_has_header(ptr respHandle, string name) -> int;
    fn sp_rt_test_response_destroy(ptr respHandle);

    fn sp_rt_str_from_int(int val) -> string;
    fn sp_rt_str_contains(string s, string sub) -> bool;
}

public class TestResponse {
    public ptr handle;

    public TestResponse(ptr h) {
        this.handle = h;
    }

    public fn status() -> int {
        if (this.handle == null) { return 0; }
        return sp_rt_test_response_status(this.handle);
    }

    public fn statusCode() -> int {
        return this.status();
    }

    public fn body() -> string {
        if (this.handle == null) { return ""; }
        return sp_rt_test_response_body(this.handle);
    }

    public fn header(string name) -> string {
        if (this.handle == null) { return ""; }
        return sp_rt_test_response_header(this.handle, name);
    }

    public fn hasHeader(string name) -> bool {
        if (this.handle == null) { return false; }
        return sp_rt_test_response_has_header(this.handle, name) == 1;
    }

    // Fluent assertions
    public fn assertStatus(int expected) -> TestResponse {
        let actual = this.status();
        if (actual != expected) {
            panic("HTTP assertion failed: expected status " + sp_rt_str_from_int(expected) + " but got " + sp_rt_str_from_int(actual));
        }
        return this;
    }

    public fn assertOk() -> TestResponse {
        return this.assertStatus(200);
    }

    public fn assertCreated() -> TestResponse {
        return this.assertStatus(201);
    }

    public fn assertNoContent() -> TestResponse {
        return this.assertStatus(204);
    }

    public fn assertBadRequest() -> TestResponse {
        return this.assertStatus(400);
    }

    public fn assertUnauthorized() -> TestResponse {
        return this.assertStatus(401);
    }

    public fn assertForbidden() -> TestResponse {
        return this.assertStatus(403);
    }

    public fn assertNotFound() -> TestResponse {
        return this.assertStatus(404);
    }

    public fn assertInternalError() -> TestResponse {
        return this.assertStatus(500);
    }

    public fn assertBody(string expected) -> TestResponse {
        let actual = this.body();
        if (actual != expected) {
            panic("HTTP assertion failed: body mismatch. Expected: '" + expected + "', got: '" + actual + "'");
        }
        return this;
    }

    public fn assertBodyContains(string substring) -> TestResponse {
        let actual = this.body();
        if (!sp_rt_str_contains(actual, substring)) {
            panic("HTTP assertion failed: body did not contain expected substring: '" + substring + "'");
        }
        return this;
    }

    public fn assertHeader(string name, string expected) -> TestResponse {
        let actual = this.header(name);
        if (actual != expected) {
            panic("HTTP assertion failed: header '" + name + "' mismatch. Expected: '" + expected + "', got: '" + actual + "'");
        }
        return this;
    }

    public fn assertHeaderContains(string name, string substring) -> TestResponse {
        let actual = this.header(name);
        if (!sp_rt_str_contains(actual, substring)) {
            panic("HTTP assertion failed: header '" + name + "' did not contain substring: '" + substring + "'");
        }
        return this;
    }

    public fn assertHeaderPresent(string name) -> TestResponse {
        if (!this.hasHeader(name)) {
            panic("HTTP assertion failed: expected header '" + name + "' to be present");
        }
        return this;
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_test_response_destroy(this.handle);
            this.handle = null;
        }
    }
}

public class TestClient {
    public ptr handle;

    public TestClient(ptr h) {
        this.handle = h;
    }

    public static fn fromServer(HttpServer server) -> TestClient {
        let h = sp_rt_test_client_create(server.handle);
        return new TestClient(h);
    }

    public static fn fromApp(App app) -> TestClient {
        let h = sp_rt_test_client_create(app.server.handle);
        return new TestClient(h);
    }

    public static fn create(App app) -> TestClient {
        return TestClient.fromApp(app);
    }

    public fn get(string path) -> TestResponse {
        let h = sp_rt_test_client_get(this.handle, path);
        return new TestResponse(h);
    }

    public fn post(string path, string body) -> TestResponse {
        let h = sp_rt_test_client_post(this.handle, path, body);
        return new TestResponse(h);
    }

    public fn postJson(string path, string jsonBody) -> TestResponse {
        let h = sp_rt_test_client_request(this.handle, "POST", path, jsonBody, "{\"Content-Type\":\"application/json\"}");
        return new TestResponse(h);
    }

    public fn put(string path, string body) -> TestResponse {
        let h = sp_rt_test_client_put(this.handle, path, body);
        return new TestResponse(h);
    }

    public fn putJson(string path, string jsonBody) -> TestResponse {
        let h = sp_rt_test_client_request(this.handle, "PUT", path, jsonBody, "{\"Content-Type\":\"application/json\"}");
        return new TestResponse(h);
    }

    public fn patch(string path, string body) -> TestResponse {
        let h = sp_rt_test_client_patch(this.handle, path, body);
        return new TestResponse(h);
    }

    public fn patchJson(string path, string jsonBody) -> TestResponse {
        let h = sp_rt_test_client_request(this.handle, "PATCH", path, jsonBody, "{\"Content-Type\":\"application/json\"}");
        return new TestResponse(h);
    }

    public fn delete(string path) -> TestResponse {
        let h = sp_rt_test_client_delete(this.handle, path);
        return new TestResponse(h);
    }

    public fn deleteRoute(string path) -> TestResponse {
        return this.delete(path);
    }

    public fn request(string method, string path, string body, string headersJson) -> TestResponse {
        let h = sp_rt_test_client_request(this.handle, method, path, body, headersJson);
        return new TestResponse(h);
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_test_client_destroy(this.handle);
            this.handle = null;
        }
    }
}
