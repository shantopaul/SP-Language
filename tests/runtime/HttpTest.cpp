#include <gtest/gtest.h>
#include "sp/runtime/Http.h"
#include "sp/runtime/Runtime.h"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>

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

TEST(HttpTest, UrlPercentEncodingAndDecoding) {
    EXPECT_EQ(urlEncode("hello world"), "hello%20world");
    EXPECT_EQ(urlDecode("hello%20world"), "hello world");
    EXPECT_EQ(urlDecode("hello+world"), "hello world");
    EXPECT_EQ(urlDecode("a%2Fb%26c%3Dd"), "a/b&c=d");

    // Test URL decoding inside HttpRequest
    std::string raw = 
        "GET /search?query=SP+Lang%202%2E2&author=Antigravity HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    EXPECT_EQ(req.path, "/search");
    EXPECT_EQ(req.getQueryParam("query"), "SP Lang 2.2");
    EXPECT_EQ(req.getQueryParam("author"), "Antigravity");
}

TEST(HttpTest, CookieParsingAndSerialization) {
    std::string raw = 
        "GET /dashboard HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "Cookie: session_id=xyz987; user=alice; theme=dark\r\n"
        "\r\n";
    HttpRequest req = HttpRequest::parse(raw);
    EXPECT_EQ(req.getCookie("session_id"), "xyz987");
    EXPECT_EQ(req.getCookie("user"), "alice");
    EXPECT_EQ(req.getCookie("theme"), "dark");

    HttpResponse resp;
    resp.setCookie("token", "secret42", "/api", 3600, true, true, "Strict");
    std::string s = resp.serialize();
    EXPECT_TRUE(s.find("Set-Cookie: token=secret42; Path=/api; Max-Age=3600; Secure; HttpOnly; SameSite=Strict") != std::string::npos);
}

TEST(HttpTest, ChunkedTransferEncodingParsing) {
    std::string chunkedReq = 
        "POST /stream HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        "Mozilla\r\n"
        "9\r\n"
        "Developer\r\n"
        "7\r\n"
        "Network\r\n"
        "0\r\n"
        "\r\n";

    HttpRequest req;
    size_t consumed = 0;
    bool ok = HttpRequest::parseRequest(chunkedReq, req, consumed, true);
    ASSERT_TRUE(ok);
    EXPECT_EQ(req.body, "MozillaDeveloperNetwork");
    EXPECT_EQ(consumed, chunkedReq.size());
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

TEST(HttpTest, KeepAlivePersistentConnectionReuse) {
    HttpServer server;
    std::atomic<int> counter{0};

    server.get("/count", [&counter](const HttpRequest&) {
        int val = ++counter;
        return HttpResponse::ok(std::to_string(val));
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    sp::rt::net::TcpSocket sock;
    ASSERT_TRUE(sock.connect("127.0.0.1", port, 3000));

    // Send 5 sequential requests over the single persistent socket
    for (int i = 1; i <= 5; ++i) {
        std::string reqStr = "GET /count HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: keep-alive\r\n\r\n";
        EXPECT_EQ(sock.writeString(reqStr), static_cast<int64_t>(reqStr.size()));

        std::string respStr;
        while (respStr.find("\r\n\r\n") == std::string::npos) {
            std::string chunk = sock.readString(1024);
            if (chunk.empty()) break;
            respStr += chunk;
        }

        EXPECT_TRUE(respStr.find("HTTP/1.1 200 OK") != std::string::npos);
        EXPECT_TRUE(respStr.find("Connection: keep-alive") != std::string::npos);
        EXPECT_TRUE(respStr.find(std::to_string(i)) != std::string::npos);
    }

    sock.close();
    server.stop();
}

TEST(HttpTest, PayloadTooLargeEnforcement) {
    HttpServerConfig cfg;
    cfg.maxBodySize = 64; // Small limit: 64 bytes
    HttpServer server(cfg);

    server.post("/upload", [](const HttpRequest& req) {
        return HttpResponse::ok("uploaded " + std::to_string(req.body.size()));
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Send small payload (within limit)
    std::string smallBody = "small data";
    auto okResp = HttpClient::post("http://127.0.0.1:" + std::to_string(port) + "/upload", smallBody);
    EXPECT_EQ(okResp.statusCode, 200);

    // Send large payload (exceeding 64 bytes)
    std::string largeBody(128, 'Z');
    auto errResp = HttpClient::post("http://127.0.0.1:" + std::to_string(port) + "/upload", largeBody);
    EXPECT_EQ(errResp.statusCode, 413);

    server.stop();
}

TEST(HttpTest, MultiThreadedWorkerPoolConcurrency) {
    HttpServerConfig cfg;
    cfg.workerThreads = 4;
    HttpServer server(cfg);

    server.get("/fast", [](const HttpRequest&) {
        return HttpResponse::ok("fast-response");
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    constexpr int clientThreads = 8;
    constexpr int reqsPerThread = 5;
    std::vector<std::thread> workers;
    std::atomic<int> successCount{0};

    for (int t = 0; t < clientThreads; ++t) {
        workers.emplace_back([port, &successCount]() {
            for (int r = 0; r < reqsPerThread; ++r) {
                auto resp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/fast");
                if (resp.statusCode == 200 && resp.body == "fast-response") {
                    successCount++;
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(successCount.load(), clientThreads * reqsPerThread);
    server.stop();
}

TEST(HttpTest, C_ABI_BridgeInspection) {
    // Test response creation & builders
    void* resp = sp_rt_http_resp_create(201, "Created Record");
    EXPECT_EQ(sp_rt_http_resp_status(resp), 201);
    EXPECT_STREQ(sp_rt_http_resp_body(resp), "Created Record");

    sp_rt_http_resp_set_header(resp, "X-Custom", "SP-Test");
    EXPECT_STREQ(sp_rt_http_resp_header(resp, "x-custom"), "SP-Test");

    sp_rt_http_resp_set_cookie(resp, "sid", "sess123", "/api", 1800, false, true, "Lax");
    EXPECT_STREQ(sp_rt_http_resp_header(resp, "set-cookie"), "sid=sess123; Path=/api; Max-Age=1800; HttpOnly; SameSite=Lax");

    sp_rt_http_resp_set_json(resp, "{\"id\":101}");
    EXPECT_STREQ(sp_rt_http_resp_body(resp), "{\"id\":101}");

    sp_rt_http_resp_destroy(resp);

    // Test advanced server creation & teardown
    void* srv = sp_rt_http_server_create_advanced(2, 1024, 512);
    ASSERT_NE(srv, nullptr);
    ASSERT_TRUE(sp_rt_http_server_listen(srv, "127.0.0.1", 0));
    int64_t port = sp_rt_http_server_port(srv);
    EXPECT_GT(port, 0);
    sp_rt_http_server_stop(srv);
    sp_rt_http_server_destroy(srv);
}
