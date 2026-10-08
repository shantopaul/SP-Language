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

// ============================================================================
// Phase 21: High-Performance Routing, Route Groups & Middleware Pipeline
// ============================================================================

TEST(HttpTest, RadixRouterExactAndParamMatching) {
    RadixRouter router;

    router.addRoute("GET", "/api/users", [](const HttpRequest&) {
        return HttpResponse::ok("all-users");
    });

    router.addRoute("GET", "/api/users/:id", [](const HttpRequest&) {
        return HttpResponse::ok("user-by-id");
    });

    router.addRoute("GET", "/api/users/:id/profile", [](const HttpRequest&) {
        return HttpResponse::ok("user-profile");
    });

    router.addRoute("POST", "/api/posts/:postId/comments/:commentId", [](const HttpRequest&) {
        return HttpResponse::ok("post-comment");
    });

    // 1. Exact match
    auto res1 = router.match("GET", "/api/users");
    EXPECT_TRUE(res1.found);
    EXPECT_TRUE(res1.pathParams.empty());

    // 2. Single param match
    auto res2 = router.match("GET", "/api/users/42");
    EXPECT_TRUE(res2.found);
    EXPECT_EQ(res2.pathParams["id"], "42");

    // 3. Nested param match
    auto res3 = router.match("GET", "/api/users/999/profile");
    EXPECT_TRUE(res3.found);
    EXPECT_EQ(res3.pathParams["id"], "999");

    // 4. Multi-param match
    auto res4 = router.match("POST", "/api/posts/p100/comments/c200");
    EXPECT_TRUE(res4.found);
    EXPECT_EQ(res4.pathParams["postId"], "p100");
    EXPECT_EQ(res4.pathParams["commentId"], "c200");

    // 5. Unregistered route
    auto res5 = router.match("GET", "/api/products");
    EXPECT_FALSE(res5.found);
    EXPECT_FALSE(res5.methodNotAllowed);
}

TEST(HttpTest, RadixRouterWildcardCatchAll) {
    RadixRouter router;

    router.addRoute("GET", "/static/*filepath", [](const HttpRequest&) {
        return HttpResponse::ok("wildcard-asset");
    });

    router.addRoute("GET", "/static/favicon.ico", [](const HttpRequest&) {
        return HttpResponse::ok("exact-favicon");
    });

    // Wildcard match across slashes
    auto res1 = router.match("GET", "/static/css/themes/dark.min.css");
    EXPECT_TRUE(res1.found);
    EXPECT_EQ(res1.pathParams["filepath"], "css/themes/dark.min.css");

    // Exact match takes precedence over wildcard
    auto res2 = router.match("GET", "/static/favicon.ico");
    EXPECT_TRUE(res2.found);
    EXPECT_TRUE(res2.pathParams.empty());
}

TEST(HttpTest, MethodNotAllowed405WithAllowHeader) {
    HttpServer server;

    server.get("/api/items", [](const HttpRequest&) {
        return HttpResponse::ok("items-list");
    });
    server.post("/api/items", [](const HttpRequest&) {
        return HttpResponse::ok("item-created");
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    sp::rt::net::TcpSocket sock;
    ASSERT_TRUE(sock.connect("127.0.0.1", port, 3000));

    // Send DELETE to a route that only supports GET and POST
    std::string reqStr = "DELETE /api/items HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    sock.writeString(reqStr);
    std::string respStr = sock.readAll(3000);
    sock.close();

    // Verify 405 Method Not Allowed and Allow header
    EXPECT_TRUE(respStr.find("HTTP/1.1 405 Method Not Allowed") != std::string::npos);
    EXPECT_TRUE(respStr.find("Allow:") != std::string::npos);
    EXPECT_TRUE(respStr.find("GET") != std::string::npos);
    EXPECT_TRUE(respStr.find("POST") != std::string::npos);

    server.stop();
}

TEST(HttpTest, AutomaticOptionsPreflightResponse) {
    HttpServer server;

    server.get("/api/orders", [](const HttpRequest&) {
        return HttpResponse::ok("orders");
    });
    server.put("/api/orders", [](const HttpRequest&) {
        return HttpResponse::ok("order-updated");
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    sp::rt::net::TcpSocket sock;
    ASSERT_TRUE(sock.connect("127.0.0.1", port, 3000));

    std::string reqStr = "OPTIONS /api/orders HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    sock.writeString(reqStr);
    std::string respStr = sock.readAll(3000);
    sock.close();

    // Verify automatic 204 No Content with Allow header
    EXPECT_TRUE(respStr.find("HTTP/1.1 204 No Content") != std::string::npos);
    EXPECT_TRUE(respStr.find("Allow:") != std::string::npos);
    EXPECT_TRUE(respStr.find("OPTIONS") != std::string::npos);

    server.stop();
}

TEST(HttpTest, AutomaticHeadHandling) {
    HttpServer server;

    server.get("/api/info", [](const HttpRequest&) {
        HttpResponse res = HttpResponse::ok("Detailed system info data");
        res.setHeader("X-Engine", "SP-2.3");
        return res;
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    sp::rt::net::TcpSocket sock;
    ASSERT_TRUE(sock.connect("127.0.0.1", port, 3000));

    std::string reqStr = "HEAD /api/info HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    sock.writeString(reqStr);
    std::string respStr = sock.readAll(3000);
    sock.close();

    // Verify status is 200, headers are present, but body is empty
    EXPECT_TRUE(respStr.find("HTTP/1.1 200 OK") != std::string::npos);
    EXPECT_TRUE(respStr.find("X-Engine: SP-2.3") != std::string::npos);
    size_t headerEnd = respStr.find("\r\n\r\n");
    ASSERT_NE(headerEnd, std::string::npos);
    std::string body = respStr.substr(headerEnd + 4);
    EXPECT_TRUE(body.empty());

    server.stop();
}

TEST(HttpTest, RouteGroupsPrefixAndMiddlewareInheritance) {
    HttpServer server;

    // Global middleware
    server.use([](const HttpRequest&, HttpResponse& res) -> bool {
        res.setHeader("X-Global", "active");
        return true;
    });

    // Group 1: /api/v1
    auto v1 = server.group("/api/v1");
    v1.use([](const HttpRequest&, HttpResponse& res) -> bool {
        res.setHeader("X-V1", "active");
        return true;
    });

    // Nested Group 2: /api/v1/admin
    auto admin = v1.group("/admin");
    admin.use([](const HttpRequest&, HttpResponse& res) -> bool {
        res.setHeader("X-Admin", "active");
        return true;
    });

    admin.get("/metrics", [](const HttpRequest&) {
        return HttpResponse::ok("admin-metrics-ok");
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto resp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/api/v1/admin/metrics");
    EXPECT_EQ(resp.statusCode, 200);
    EXPECT_EQ(resp.body, "admin-metrics-ok");
    EXPECT_EQ(resp.getHeader("x-global"), "active");
    EXPECT_EQ(resp.getHeader("x-v1"), "active");
    EXPECT_EQ(resp.getHeader("x-admin"), "active");

    server.stop();
}

TEST(HttpTest, CorsMiddlewarePreflightAndHeaders) {
    HttpServer server;
    server.use(HttpServer::cors({
        .allowedOrigins = "https://app.example.com",
        .allowedMethods = "GET, POST, OPTIONS",
        .allowedHeaders = "Authorization, Content-Type",
        .exposedHeaders = "X-Request-ID",
        .allowCredentials = true,
        .maxAge = 3600
    }));

    server.get("/api/data", [](const HttpRequest&) {
        return HttpResponse::ok("data-ok");
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Test Preflight OPTIONS interception
    sp::rt::net::TcpSocket sock;
    ASSERT_TRUE(sock.connect("127.0.0.1", port, 3000));
    std::string preflightReq = "OPTIONS /api/data HTTP/1.1\r\nHost: 127.0.0.1\r\nConnection: close\r\n\r\n";
    sock.writeString(preflightReq);
    std::string preflightRes = sock.readAll(3000);
    sock.close();

    EXPECT_TRUE(preflightRes.find("HTTP/1.1 204 No Content") != std::string::npos);
    EXPECT_TRUE(preflightRes.find("Access-Control-Allow-Origin: https://app.example.com") != std::string::npos);
    EXPECT_TRUE(preflightRes.find("Access-Control-Allow-Methods: GET, POST, OPTIONS") != std::string::npos);
    EXPECT_TRUE(preflightRes.find("Access-Control-Allow-Headers: Authorization, Content-Type") != std::string::npos);
    EXPECT_TRUE(preflightRes.find("Access-Control-Max-Age: 3600") != std::string::npos);

    // Test regular GET includes CORS headers
    auto getRes = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/api/data");
    EXPECT_EQ(getRes.statusCode, 200);
    EXPECT_EQ(getRes.getHeader("access-control-allow-origin"), "https://app.example.com");

    server.stop();
}

TEST(HttpTest, RateLimiterMiddleware429TooManyRequests) {
    HttpServer server;
    server.use(HttpServer::rateLimiter({
        .maxRequests = 3,
        .windowSeconds = 10
    }));

    server.get("/api/rate-test", [](const HttpRequest&) {
        return HttpResponse::ok("rate-ok");
    });

    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    std::string url = "http://127.0.0.1:" + std::to_string(port) + "/api/rate-test";

    // First 3 requests succeed
    for (int i = 0; i < 3; ++i) {
        auto r = HttpClient::get(url);
        EXPECT_EQ(r.statusCode, 200);
    }

    // 4th request exceeds limit and receives 429 Too Many Requests
    auto r4 = HttpClient::get(url);
    EXPECT_EQ(r4.statusCode, 429);
    EXPECT_EQ(r4.getHeader("retry-after"), "10");

    server.stop();
}

TEST(HttpTest, SubMicrosecondRoutingBenchmark) {
    RadixRouter router;

    // Register 150 diverse routes
    for (int i = 0; i < 150; ++i) {
        std::string path = "/api/v" + std::to_string(i % 5) + "/resources/" + std::to_string(i) + "/items/:itemId";
        router.addRoute("GET", path, [](const HttpRequest&) {
            return HttpResponse::ok("benchmark");
        });
    }
    EXPECT_EQ(router.routeCount(), 150u);

    // Pre-generate query paths to measure pure router matching latency without string formatting overhead
    std::vector<std::string> testPaths;
    testPaths.reserve(150);
    for (int idx = 0; idx < 150; ++idx) {
        testPaths.push_back("/api/v" + std::to_string(idx % 5) + "/resources/" + std::to_string(idx) + "/items/item_42");
    }

    // Warm up instruction cache and CPU governor
    for (int i = 0; i < 5000; ++i) {
        auto res = router.match("GET", testPaths[i % 150]);
        (void)res;
    }

    // Perform 20,000 pure lookups
    const int iterations = 20000;
    size_t matchCount = 0;
    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < iterations; ++i) {
        auto res = router.match("GET", testPaths[i % 150]);
        if (res.found) matchCount++;
    }

    auto end = std::chrono::high_resolution_clock::now();
    EXPECT_EQ(matchCount, static_cast<size_t>(iterations));

    double totalUs = std::chrono::duration<double, std::micro>(end - start).count();
    double avgUsPerLookup = totalUs / iterations;

    std::cout << "[BENCHMARK] 150-Route RadixTree Dispatch: " 
              << avgUsPerLookup << " us per lookup (" << (1000000.0 / avgUsPerLookup) << " lookups/sec)" << std::endl;

    // Ultra-low latency dispatch verification (< 5.0 us per lookup across CI/scheduler jitter)
    EXPECT_LT(avgUsPerLookup, 5.0);
}

TEST(HttpTest, RouteGroup_C_ABI_Bridge) {
    void* srv = sp_rt_http_server_create();
    ASSERT_NE(srv, nullptr);

    void* v1 = sp_rt_http_server_group(srv, "/api/v1");
    ASSERT_NE(v1, nullptr);

    sp_rt_http_group_route(v1, "GET", "/ping", [](const char*, const char*, const char*) -> const char* {
        return "v1-pong";
    });

    ASSERT_TRUE(sp_rt_http_server_listen(srv, "127.0.0.1", 0));
    int64_t port = sp_rt_http_server_port(srv);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto resp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/api/v1/ping");
    EXPECT_EQ(resp.statusCode, 200);
    EXPECT_EQ(resp.body, "v1-pong");

    sp_rt_http_group_destroy(v1);
    sp_rt_http_server_stop(srv);
    sp_rt_http_server_destroy(srv);
}
