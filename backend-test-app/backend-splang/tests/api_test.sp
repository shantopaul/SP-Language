module tests.api_test;

import std.io;
import std.web;
import std.test.http;

test "Health Endpoint Returns 200 OK" {
    let app = new App();
    app.get("/api/health", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\"status\":\"healthy\",\"service\":\"backend-splang\"}");
    });

    let client = TestClient.fromApp(app);
    let res = client.get("/api/health");
    res.assertOk();
    res.assertHeaderContains("Content-Type", "application/json");
    res.assertBodyContains("healthy");
    res.assertBodyContains("backend-splang");
    res.destroy();
    client.destroy();
}

test "OpenAPI Spec Endpoint Returns Valid JSON" {
    let app = new App();
    app.get("/openapi.json", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\"openapi\":\"3.0.3\",\"info\":{\"title\":\"SP Lang Backend API\"}}");
    });

    let client = TestClient.fromApp(app);
    let res = client.get("/openapi.json");
    res.assertOk();
    res.assertHeaderContains("Content-Type", "application/json");
    res.assertBodyContains("3.0.3");
    res.destroy();
    client.destroy();
}

test "Items Endpoint Returns JSON Array" {
    let app = new App();
    app.get("/api/items", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("[{\"id\":1,\"title\":\"Test Item\",\"status\":\"pending\"}]");
    });

    let client = TestClient.fromApp(app);
    let res = client.get("/api/items");
    res.assertOk();
    res.assertBodyContains("Test Item");
    res.destroy();
    client.destroy();
}

test "Create Item Endpoint Returns 201 Created" {
    let app = new App();
    app.post("/api/items", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.jsonStatus("{\"id\":2,\"title\":\"New Item\",\"status\":\"pending\"}", 201);
    });

    let client = TestClient.fromApp(app);
    let res = client.post("/api/items", "{\"title\":\"New Item\"}");
    res.assertCreated();
    res.assertBodyContains("New Item");
    res.destroy();
    client.destroy();
}
