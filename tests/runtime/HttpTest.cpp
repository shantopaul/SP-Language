#include <gtest/gtest.h>
#include "sp/runtime/Http.h"
#include <thread>
#include <chrono>

using namespace sp::rt::http;

TEST(HttpTest, HttpRequestParsing) {
    std::string rawReq = 
        "POST /api/items?category=tech&sort=desc HTTP/1.1\r\n"
        "Host: api.example.com\r\n"
        "User-Agent: SP-Client/1.6\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 18\r\n"
        "\r\n"
        "{\"item\":\"laptop\"}";

    HttpRequest req = HttpRequest::parse(rawReq);
    EXPECT_EQ(req.method, "POST");
    EXPECT_EQ(req.path, "/api/items");
    EXPECT_EQ(req.getQueryParam("category"), "tech");
    EXPECT_EQ(req.getQueryParam("sort"), "desc");
    EXPECT_EQ(req.getHeader("host"), "api.example.com");
    EXPECT_EQ(req.getHeader("content-type"), "application/json");
    EXPECT_EQ(req.body, "{\"item\":\"laptop\"}");
}

TEST(HttpTest, HttpResponseSerialization) {
    HttpResponse resp;
    resp.statusCode = 200;
    resp.statusText = "OK";
    resp.setHeader("X-Powered-By", "SP-Language");
    resp.setJson("{\"success\":true}");

    std::string serialized = resp.serialize();
    EXPECT_TRUE(serialized.find("HTTP/1.1 200 OK\r\n") != std::string::npos);
    EXPECT_TRUE(serialized.find("X-Powered-By: SP-Language\r\n") != std::string::npos);
    EXPECT_TRUE(serialized.find("Content-Type: application/json; charset=utf-8\r\n") != std::string::npos);
    EXPECT_TRUE(serialized.find("Content-Length: 16\r\n") != std::string::npos);
    EXPECT_TRUE(serialized.find("{\"success\":true}") != std::string::npos);
}

TEST(HttpTest, HttpServerAndClientEndToEnd) {
    HttpServer server;

    server.get("/ping", [](const HttpRequest&) {
        return HttpResponse::ok("pong");
    });

    server.get("/users/:id", [](const HttpRequest& req) {
        std::string id = req.getPathParam("id");
        return HttpResponse::json("{\"userId\":\"" + id + "\"}");
    });

    server.post("/echo", [](const HttpRequest& req) {
        return HttpResponse::json(req.body);
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    ASSERT_GT(port, 0);

    // Give server thread moment to start listening
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Test 1: GET /ping
    auto pingResp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/ping");
    EXPECT_EQ(pingResp.statusCode, 200);
    EXPECT_EQ(pingResp.body, "pong");

    // Test 2: GET /users/999 (parameterized route)
    auto userResp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/users/999");
    EXPECT_EQ(userResp.statusCode, 200);
    EXPECT_EQ(userResp.body, "{\"userId\":\"999\"}");

    // Test 3: POST /echo
    std::string reqBody = "{\"message\":\"SP Microservice Engine\"}";
    auto echoResp = HttpClient::post("http://127.0.0.1:" + std::to_string(port) + "/echo", reqBody);
    EXPECT_EQ(echoResp.statusCode, 200);
    EXPECT_EQ(echoResp.body, reqBody);

    // Test 4: 404 Route
    auto notFoundResp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/nonexistent");
    EXPECT_EQ(notFoundResp.statusCode, 404);

    server.stop();
}
