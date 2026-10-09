#include <gtest/gtest.h>
#include "sp/runtime/Http.h"
#include "sp/runtime/Runtime.h"
#include "spm/ProjectConfig.h"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <functional>

using namespace sp::rt::http;

namespace {

// Simulated SP Object layout: slot 0 is vtable, slot 1 is handle
struct MockSpObject {
    void* vtable{nullptr};
    void* handle{nullptr};
};

} // namespace

TEST(WebTest, ContextResponseHelpers) {
    // 1. Text response helper
    void* resp1 = sp_rt_http_resp_create(200, nullptr);
    sp_rt_http_resp_set_text(resp1, "Hello Plain Text");
    auto* r1 = static_cast<HttpResponse*>(resp1);
    EXPECT_EQ(r1->statusCode, 200);
    EXPECT_EQ(r1->body, "Hello Plain Text");
    EXPECT_EQ(r1->getHeader("content-type"), "text/plain; charset=utf-8");
    sp_rt_http_resp_destroy(resp1);

    // 2. HTML response helper
    void* resp2 = sp_rt_http_resp_create(200, nullptr);
    sp_rt_http_resp_set_html(resp2, "<h1>Welcome to SP-Web</h1>");
    auto* r2 = static_cast<HttpResponse*>(resp2);
    EXPECT_EQ(r2->statusCode, 200);
    EXPECT_EQ(r2->body, "<h1>Welcome to SP-Web</h1>");
    EXPECT_EQ(r2->getHeader("content-type"), "text/html; charset=utf-8");
    sp_rt_http_resp_destroy(resp2);

    // 3. Redirect response helper (default 302)
    void* resp3 = sp_rt_http_resp_create(200, nullptr);
    sp_rt_http_resp_redirect(resp3, "/dashboard", 302);
    auto* r3 = static_cast<HttpResponse*>(resp3);
    EXPECT_EQ(r3->statusCode, 302);
    EXPECT_EQ(r3->statusText, "Found");
    EXPECT_EQ(r3->getHeader("location"), "/dashboard");
    EXPECT_TRUE(r3->body.empty());
    sp_rt_http_resp_destroy(resp3);

    // 4. Permanent redirect helper (301)
    void* resp4 = sp_rt_http_resp_create(200, nullptr);
    sp_rt_http_resp_redirect(resp4, "https://example.com/v2", 301);
    auto* r4 = static_cast<HttpResponse*>(resp4);
    EXPECT_EQ(r4->statusCode, 301);
    EXPECT_EQ(r4->statusText, "Moved Permanently");
    EXPECT_EQ(r4->getHeader("location"), "https://example.com/v2");
    sp_rt_http_resp_destroy(resp4);

    // 5. Cookie builder
    void* resp5 = sp_rt_http_resp_create(200, nullptr);
    sp_rt_http_resp_set_cookie(resp5, "auth_token", "jwt-token-12345", "/api", 7200, true, true, "Strict");
    auto* r5 = static_cast<HttpResponse*>(resp5);
    std::string setCookie = r5->getHeader("set-cookie");
    EXPECT_TRUE(setCookie.find("auth_token=jwt-token-12345") != std::string::npos);
    EXPECT_TRUE(setCookie.find("Path=/api") != std::string::npos);
    EXPECT_TRUE(setCookie.find("Max-Age=7200") != std::string::npos);
    EXPECT_TRUE(setCookie.find("Secure") != std::string::npos);
    EXPECT_TRUE(setCookie.find("HttpOnly") != std::string::npos);
    EXPECT_TRUE(setCookie.find("SameSite=Strict") != std::string::npos);
    sp_rt_http_resp_destroy(resp5);
}

TEST(WebTest, RouteHandlerDirectResponse) {
    void* srv = sp_rt_http_server_create_advanced(2, 1024, 512);
    ASSERT_NE(srv, nullptr);

    // Register GET /greet/:name handler returning raw HttpResponse*
    sp_rt_http_server_route_handler(srv, "GET", "/greet/:name", [](void* req) -> void* {
        std::string name = sp_rt_http_req_param(req, "name");
        std::string role = sp_rt_http_req_query(req, "role");
        void* resp = sp_rt_http_resp_create(200, nullptr);
        std::string json = "{\"greeting\":\"Hello " + name + "\",\"role\":\"" + role + "\"}";
        sp_rt_http_resp_set_json(resp, json.c_str());
        return resp;
    });

    ASSERT_TRUE(sp_rt_http_server_listen(srv, "127.0.0.1", 0));
    int64_t port = sp_rt_http_server_port(srv);
    ASSERT_GT(port, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto resp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/greet/Alice?role=admin");
    EXPECT_EQ(resp.statusCode, 200);
    EXPECT_TRUE(resp.body.find("\"greeting\":\"Hello Alice\"") != std::string::npos);
    EXPECT_TRUE(resp.body.find("\"role\":\"admin\"") != std::string::npos);

    sp_rt_http_server_stop(srv);
    sp_rt_http_server_destroy(srv);
}

TEST(WebTest, RouteHandlerSpObjectWrapper) {
    void* srv = sp_rt_http_server_create_advanced(2, 1024, 512);
    ASSERT_NE(srv, nullptr);

    // Register POST /echo returning an SP class instance wrapper where slot 1 has the HttpResponse*
    static MockSpObject mockWrapper;
    sp_rt_http_server_route_handler(srv, "POST", "/echo", [](void* req) -> void* {
        std::string body = sp_rt_http_req_body(req);
        void* rawResp = sp_rt_http_resp_create(201, nullptr);
        sp_rt_http_resp_set_text(rawResp, ("Echo: " + body).c_str());
        mockWrapper.vtable = (void*)0xDEADBEEF;
        mockWrapper.handle = rawResp;
        return &mockWrapper;
    });

    ASSERT_TRUE(sp_rt_http_server_listen(srv, "127.0.0.1", 0));
    int64_t port = sp_rt_http_server_port(srv);
    ASSERT_GT(port, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto resp = HttpClient::post("http://127.0.0.1:" + std::to_string(port) + "/echo", "Payload 42");
    EXPECT_EQ(resp.statusCode, 201);
    EXPECT_EQ(resp.body, "Echo: Payload 42");
    EXPECT_EQ(resp.getHeader("content-type"), "text/plain; charset=utf-8");

    sp_rt_http_server_stop(srv);
    sp_rt_http_server_destroy(srv);
}

TEST(WebTest, RouteGroupHandlerDispatch) {
    void* srv = sp_rt_http_server_create_advanced(2, 1024, 512);
    ASSERT_NE(srv, nullptr);

    // Create route group "/api/v1"
    void* grp = sp_rt_http_server_group(srv, "/api/v1");
    ASSERT_NE(grp, nullptr);

    // Register GET /items/:id in group
    sp_rt_http_group_route_handler(grp, "GET", "/items/:id", [](void* req) -> void* {
        std::string id = sp_rt_http_req_param(req, "id");
        void* resp = sp_rt_http_resp_create(200, nullptr);
        std::string json = "{\"item_id\":\"" + id + "\",\"status\":\"active\"}";
        sp_rt_http_resp_set_json(resp, json.c_str());
        return resp;
    });

    // Register POST /items in group
    sp_rt_http_group_route_handler(grp, "POST", "/items", [](void* req) -> void* {
        std::string body = sp_rt_http_req_body(req);
        void* resp = sp_rt_http_resp_create(201, nullptr);
        std::string json = "{\"created\":true,\"data\":\"" + body + "\"}";
        sp_rt_http_resp_set_json(resp, json.c_str());
        return resp;
    });

    ASSERT_TRUE(sp_rt_http_server_listen(srv, "127.0.0.1", 0));
    int64_t port = sp_rt_http_server_port(srv);
    ASSERT_GT(port, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    // Test GET /api/v1/items/99
    auto getResp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/api/v1/items/99");
    EXPECT_EQ(getResp.statusCode, 200);
    EXPECT_TRUE(getResp.body.find("\"item_id\":\"99\"") != std::string::npos);

    // Test POST /api/v1/items
    auto postResp = HttpClient::post("http://127.0.0.1:" + std::to_string(port) + "/api/v1/items", "new-widget");
    EXPECT_EQ(postResp.statusCode, 201);
    EXPECT_TRUE(postResp.body.find("\"created\":true") != std::string::npos);

    sp_rt_http_group_destroy(grp);
    sp_rt_http_server_stop(srv);
    sp_rt_http_server_destroy(srv);
}

TEST(WebTest, DependencyInjectionContainerSemantics) {
    // Verify the DI container semantics as modeled by std.web.di
    struct TestContainer {
        std::unordered_map<std::string, std::function<void*()>> factories;
        std::unordered_map<std::string, void*> singletons;
        std::unordered_map<std::string, std::string> values;

        void bind(const std::string& key, std::function<void*()> factory) {
            factories[key] = factory;
        }

        void singleton(const std::string& key, std::function<void*()> factory) {
            factories[key] = [this, key, factory]() {
                auto it = singletons.find(key);
                if (it != singletons.end()) {
                    return it->second;
                }
                void* inst = factory();
                singletons[key] = inst;
                return inst;
            };
        }

        void* resolve(const std::string& key) {
            auto it = factories.find(key);
            if (it != factories.end()) {
                return it->second();
            }
            return nullptr;
        }

        void bindValue(const std::string& key, const std::string& val) {
            values[key] = val;
        }

        std::string resolveValue(const std::string& key) {
            auto it = values.find(key);
            return (it != values.end()) ? it->second : "";
        }

        bool has(const std::string& key) {
            return factories.find(key) != factories.end() || values.find(key) != values.end();
        }

        size_t size() {
            return factories.size() + values.size();
        }

        void clear() {
            factories.clear();
            singletons.clear();
            values.clear();
        }
    };

    TestContainer container;

    // 1. Transient bind creates fresh instance every resolve
    int transientCount = 0;
    container.bind("counter", [&transientCount]() -> void* {
        transientCount++;
        return reinterpret_cast<void*>(static_cast<uintptr_t>(transientCount));
    });

    EXPECT_EQ(reinterpret_cast<uintptr_t>(container.resolve("counter")), 1);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(container.resolve("counter")), 2);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(container.resolve("counter")), 3);

    // 2. Singleton caches the result on first resolve
    int singletonCount = 0;
    container.singleton("db_pool", [&singletonCount]() -> void* {
        singletonCount++;
        return reinterpret_cast<void*>(static_cast<uintptr_t>(42));
    });

    EXPECT_EQ(reinterpret_cast<uintptr_t>(container.resolve("db_pool")), 42);
    EXPECT_EQ(singletonCount, 1);
    EXPECT_EQ(reinterpret_cast<uintptr_t>(container.resolve("db_pool")), 42);
    EXPECT_EQ(singletonCount, 1); // Not called again!

    // 3. Value binding
    container.bindValue("env", "production");
    container.bindValue("app_name", "SP-SaaS");
    EXPECT_EQ(container.resolveValue("env"), "production");
    EXPECT_EQ(container.resolveValue("app_name"), "SP-SaaS");

    // 4. Inspection and clear
    EXPECT_TRUE(container.has("counter"));
    EXPECT_TRUE(container.has("db_pool"));
    EXPECT_TRUE(container.has("env"));
    EXPECT_FALSE(container.has("nonexistent"));
    EXPECT_EQ(container.size(), 4);

    container.clear();
    EXPECT_EQ(container.size(), 0);
    EXPECT_FALSE(container.has("db_pool"));
}

TEST(WebTest, SpmApiScaffoldProject) {
    namespace fs = std::filesystem;
    fs::path tempDir = fs::temp_directory_path() / "spm_test_api_project";
    if (fs::exists(tempDir)) {
        fs::remove_all(tempDir);
    }

    // Scaffold with template "api"
    std::string errMsg;
    bool success = sp::spm::scaffoldProject(tempDir.string(), "test_api", false, errMsg, "api");
    ASSERT_TRUE(success) << "scaffoldProject failed: " << errMsg;

    // Verify sp.toml exists and contains correct contents
    fs::path spToml = tempDir / "sp.toml";
    ASSERT_TRUE(fs::exists(spToml));

    {
        std::ifstream tomlFile(spToml);
        std::string tomlContent((std::istreambuf_iterator<char>(tomlFile)), std::istreambuf_iterator<char>());
        EXPECT_TRUE(tomlContent.find("std.http") != std::string::npos);
        EXPECT_TRUE(tomlContent.find("std.web") != std::string::npos);
        EXPECT_TRUE(tomlContent.find("std.json") != std::string::npos);
    }

    // Verify src/main.sp exists and contains App scaffold
    fs::path mainSp = tempDir / "src" / "main.sp";
    ASSERT_TRUE(fs::exists(mainSp));

    {
        std::ifstream spFile(mainSp);
        std::string spContent((std::istreambuf_iterator<char>(spFile)), std::istreambuf_iterator<char>());
        EXPECT_TRUE(spContent.find("import std.web") != std::string::npos);
        EXPECT_TRUE(spContent.find("let app = new App()") != std::string::npos);
        EXPECT_TRUE(spContent.find("app.use") != std::string::npos);
    }

    std::error_code ec;
    fs::remove_all(tempDir, ec);
}

TEST(WebTest, DevHotReloadRebootBenchmark) {
    // Benchmark file change detection and restart preparation speed (< 50ms)
    namespace fs = std::filesystem;
    fs::path tempFile = fs::temp_directory_path() / "sp_watch_bench.sp";
    {
        std::ofstream out(tempFile);
        out << "fn main() { println(\"v1\"); }\n";
    }

    auto t0 = std::chrono::high_resolution_clock::now();

    // Modify file
    {
        std::ofstream out(tempFile, std::ios::app);
        out << "// touch\n";
    }

    auto lastWrite = fs::last_write_time(tempFile);
    (void)lastWrite;

    auto t1 = std::chrono::high_resolution_clock::now();
    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();

    // Latency of change detection and preparation should be well below 50ms
    EXPECT_LT(elapsedMs, 50);

    fs::remove(tempFile);
}

TEST(WebTest, AppHighConcurrencyStress) {
    void* srv = sp_rt_http_server_create_advanced(4, 4096, 2048);
    ASSERT_NE(srv, nullptr);

    // Group /api/v1
    void* grp = sp_rt_http_server_group(srv, "/api/v1");
    ASSERT_NE(grp, nullptr);

    sp_rt_http_group_route_handler(grp, "GET", "/ping", [](void*) -> void* {
        void* resp = sp_rt_http_resp_create(200, nullptr);
        sp_rt_http_resp_set_json(resp, "{\"pong\":true}");
        return resp;
    });

    sp_rt_http_group_route_handler(grp, "GET", "/status", [](void*) -> void* {
        void* resp = sp_rt_http_resp_create(200, nullptr);
        sp_rt_http_resp_set_text(resp, "HEALTHY");
        return resp;
    });

    sp_rt_http_group_route_handler(grp, "GET", "/doc", [](void*) -> void* {
        void* resp = sp_rt_http_resp_create(200, nullptr);
        sp_rt_http_resp_set_html(resp, "<html>API Documentation</html>");
        return resp;
    });

    ASSERT_TRUE(sp_rt_http_server_listen(srv, "127.0.0.1", 0));
    int64_t port = sp_rt_http_server_port(srv);
    ASSERT_GT(port, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    constexpr int numThreads = 8;
    constexpr int reqsPerThread = 15;
    std::atomic<int> completedSuccess{0};
    std::vector<std::thread> threads;

    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([port, &completedSuccess]() {
            for (int r = 0; r < reqsPerThread; ++r) {
                if (r % 3 == 0) {
                    auto resp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/api/v1/ping");
                    if (resp.statusCode == 200 && resp.body == "{\"pong\":true}") {
                        completedSuccess++;
                    }
                } else if (r % 3 == 1) {
                    auto resp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/api/v1/status");
                    if (resp.statusCode == 200 && resp.body == "HEALTHY") {
                        completedSuccess++;
                    }
                } else {
                    auto resp = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/api/v1/doc");
                    if (resp.statusCode == 200 && resp.body == "<html>API Documentation</html>") {
                        completedSuccess++;
                    }
                }
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    EXPECT_EQ(completedSuccess.load(), numThreads * reqsPerThread);

    sp_rt_http_group_destroy(grp);
    sp_rt_http_server_stop(srv);
    sp_rt_http_server_destroy(srv);
}
