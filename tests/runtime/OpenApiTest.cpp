#include <gtest/gtest.h>
#include "sp/runtime/Http.h"
#include "sp/runtime/TestHttp.h"
#include "sp/runtime/OpenApi.h"
#include "sp/runtime/Runtime.h"
#include "doc/OpenApiGenerator.h"

using namespace sp::rt::http;

TEST(OpenApiTest, InMemoryTestClientGetRequest) {
    HttpServer server;
    server.route("GET", "/api/ping", [](const HttpRequest& /*req*/) {
        auto resp = HttpResponse::ok("pong");
        resp.setHeader("Content-Type", "text/plain");
        return resp;
    });

    void* client = sp_rt_test_client_create(&server);
    ASSERT_NE(client, nullptr);

    void* resp = sp_rt_test_client_get(client, "/api/ping");
    ASSERT_NE(resp, nullptr);

    EXPECT_EQ(sp_rt_test_response_status(resp), 200);
    EXPECT_STREQ(sp_rt_test_response_body(resp), "pong");
    EXPECT_STREQ(sp_rt_test_response_header(resp, "Content-Type"), "text/plain");
    EXPECT_EQ(sp_rt_test_response_has_header(resp, "Content-Type"), 1);
    EXPECT_EQ(sp_rt_test_response_has_header(resp, "X-Non-Existent"), 0);

    sp_rt_test_response_destroy(resp);
    sp_rt_test_client_destroy(client);
}

TEST(OpenApiTest, InMemoryTestClientPostRequestWithHeaders) {
    HttpServer server;
    server.route("POST", "/api/users", [](const HttpRequest& req) {
        HttpResponse resp;
        resp.statusCode = 201;
        resp.setBody("Created: " + req.body);
        resp.setHeader("Content-Type", "application/json");
        resp.setHeader("X-Created-By", "In-Memory Test");
        return resp;
    });

    void* client = sp_rt_test_client_create(&server);
    ASSERT_NE(client, nullptr);

    void* resp = sp_rt_test_client_post(client, "/api/users", "{\"name\":\"Bob\"}");
    ASSERT_NE(resp, nullptr);

    EXPECT_EQ(sp_rt_test_response_status(resp), 201);
    EXPECT_STREQ(sp_rt_test_response_body(resp), "Created: {\"name\":\"Bob\"}");
    EXPECT_STREQ(sp_rt_test_response_header(resp, "X-Created-By"), "In-Memory Test");

    sp_rt_test_response_destroy(resp);
    sp_rt_test_client_destroy(client);
}

TEST(OpenApiTest, InMemoryTestClientNotFound) {
    HttpServer server;
    void* client = sp_rt_test_client_create(&server);
    ASSERT_NE(client, nullptr);

    void* resp = sp_rt_test_client_get(client, "/non-existent-path");
    ASSERT_NE(resp, nullptr);

    EXPECT_EQ(sp_rt_test_response_status(resp), 404);

    sp_rt_test_response_destroy(resp);
    sp_rt_test_client_destroy(client);
}

TEST(OpenApiTest, InMemoryTestClientPathParameters) {
    HttpServer server;
    server.route("GET", "/users/:id", [](const HttpRequest& req) {
        auto it = req.pathParams.find("id");
        std::string id = (it != req.pathParams.end()) ? it->second : "unknown";
        return HttpResponse::ok("User:" + id);
    });

    void* client = sp_rt_test_client_create(&server);
    ASSERT_NE(client, nullptr);

    void* resp = sp_rt_test_client_get(client, "/users/999");
    ASSERT_NE(resp, nullptr);

    EXPECT_EQ(sp_rt_test_response_status(resp), 200);
    EXPECT_STREQ(sp_rt_test_response_body(resp), "User:999");

    sp_rt_test_response_destroy(resp);
    sp_rt_test_client_destroy(client);
}

TEST(OpenApiTest, SwaggerAndRedocHtmlGeneration) {
    const char* swaggerHtml = sp_rt_openapi_swagger_html("Payment API", "/api/v1/openapi.json");
    ASSERT_NE(swaggerHtml, nullptr);
    std::string sHtml(swaggerHtml);
    EXPECT_TRUE(sHtml.find("Payment API") != std::string::npos);
    EXPECT_TRUE(sHtml.find("/api/v1/openapi.json") != std::string::npos);
    EXPECT_TRUE(sHtml.find("swagger-ui") != std::string::npos);

    const char* redocHtml = sp_rt_openapi_redoc_html("Payment API", "/api/v1/openapi.json");
    ASSERT_NE(redocHtml, nullptr);
    std::string rHtml(redocHtml);
    EXPECT_TRUE(rHtml.find("Payment API") != std::string::npos);
    EXPECT_TRUE(rHtml.find("spec-url=\"/api/v1/openapi.json\"") != std::string::npos);
}

TEST(OpenApiTest, OpenApiGeneratorFromSourceCode) {
    std::string spSource = R"SP(
module app;

import std.web;

fn main() {
    let app = new App();

    /// @summary Health check probe
    /// @tag System
    /// @response 200 System is operational
    app.get("/health", (ptr req) => { ... });

    let api = app.group("/api/v1");

    /// @summary Retrieve user by ID
    /// @param id path integer Target user ID
    /// @response 200 User found
    /// @response 404 User not found
    api.get("/users/:id", (ptr req) => { ... });

    /// @summary Create a new user
    /// @body CreateUserRequest User creation payload
    /// @response 201 User created successfully
    api.post("/users", (ptr req) => { ... });
}
)SP";

    sp::doc::OpenApiOptions opts;
    opts.title = "Sample Service";
    opts.version = "2.1.0";
    opts.serverUrl = "https://api.example.com";

    sp::doc::OpenApiGenerator gen(opts);
    gen.scanSource(spSource, "main.sp");

    EXPECT_EQ(gen.getRoutes().size(), 3);

    std::string json = gen.generateJson();
    EXPECT_TRUE(json.find("\"openapi\": \"3.0.3\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"title\": \"Sample Service\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"version\": \"2.1.0\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"/health\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"/api/v1/users/{id}\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"name\": \"id\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"in\": \"path\"") != std::string::npos);
    EXPECT_TRUE(json.find("\"required\": true") != std::string::npos);
    EXPECT_TRUE(json.find("\"201\"") != std::string::npos);

    std::string yaml = gen.generateYaml();
    EXPECT_TRUE(yaml.find("openapi: 3.0.3") != std::string::npos);
    EXPECT_TRUE(yaml.find("Sample Service") != std::string::npos);
}
