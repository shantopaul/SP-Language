#pragma once

#include "sp/runtime/Net.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <deque>

namespace sp::rt::http {

// Utility functions for RFC 3986 URL Percent-Encoding / Decoding
std::string urlDecode(std::string_view str);
std::string urlEncode(std::string_view str);

struct Header {
    std::string name;
    std::string value;
};

struct Cookie {
    std::string name;
    std::string value;
    std::string path{"/"};
    std::string domain;
    int maxAge{-1};
    bool secure{false};
    bool httpOnly{false};
    std::string sameSite{"Lax"};
};

class HttpRequest {
public:
    std::string method{"GET"};
    std::string path{"/"};
    std::string queryString;
    std::vector<Header> headers;
    std::string body;
    std::unordered_map<std::string, std::string> pathParams;
    std::unordered_map<std::string, std::string> queryParams;
    std::unordered_map<std::string, std::string> cookies;

    [[nodiscard]] std::string getHeader(const std::string& name) const;
    void setHeader(const std::string& name, const std::string& value);
    [[nodiscard]] std::string getQueryParam(const std::string& name) const;
    [[nodiscard]] std::string getPathParam(const std::string& name) const;
    [[nodiscard]] std::string getCookie(const std::string& name) const;

    [[nodiscard]] bool isKeepAlive() const;
    [[nodiscard]] int64_t getContentLength() const;
    [[nodiscard]] std::string getContentType() const;

    // Parse single request from string
    static HttpRequest parse(const std::string& rawHttp);

    // Incremental parser for persistent keep-alive and pipelined connections:
    // Returns true if a complete request was parsed; outBytesConsumed indicates consumed bytes.
    static bool parseRequest(const std::string& rawData, HttpRequest& outReq, size_t& outBytesConsumed, bool requireComplete = true);
};

class HttpResponse {
public:
    int statusCode{200};
    std::string statusText{"OK"};
    std::vector<Header> headers;
    std::string body;
    bool isChunked{false};

    void setHeader(const std::string& name, const std::string& value);
    [[nodiscard]] std::string getHeader(const std::string& name) const;
    void setCookie(const std::string& name, const std::string& value, 
                   const std::string& path = "/", int maxAge = -1, 
                   bool secure = false, bool httpOnly = false,
                   const std::string& sameSite = "Lax");

    void setBody(std::string content);
    void setJson(const std::string& jsonContent);
    void setText(std::string textContent);
    void setHtml(std::string htmlContent);

    [[nodiscard]] std::string serialize(bool keepAlive = true) const;

    static HttpResponse ok(std::string body = "") {
        HttpResponse res;
        res.statusCode = 200;
        res.statusText = "OK";
        res.setBody(std::move(body));
        return res;
    }

    static HttpResponse json(const std::string& jsonContent, int code = 200) {
        HttpResponse res;
        res.statusCode = code;
        res.statusText = (code == 200) ? "OK" : ("Status " + std::to_string(code));
        res.setJson(jsonContent);
        return res;
    }

    static HttpResponse status(int code, std::string statusText = "") {
        HttpResponse res;
        res.statusCode = code;
        res.statusText = statusText.empty() ? ("Status " + std::to_string(code)) : std::move(statusText);
        return res;
    }

    static HttpResponse notFound(std::string msg = "Not Found") {
        HttpResponse res;
        res.statusCode = 404;
        res.statusText = "Not Found";
        res.setBody(std::move(msg));
        return res;
    }

    static HttpResponse payloadTooLarge(std::string msg = "Payload Too Large") {
        HttpResponse res;
        res.statusCode = 413;
        res.statusText = "Payload Too Large";
        res.setBody(std::move(msg));
        return res;
    }

    static HttpResponse noContent() {
        HttpResponse res;
        res.statusCode = 204;
        res.statusText = "No Content";
        return res;
    }

    static HttpResponse methodNotAllowed(const std::vector<std::string>& allowed) {
        HttpResponse res;
        res.statusCode = 405;
        res.statusText = "Method Not Allowed";
        std::string allowHeader;
        for (size_t i = 0; i < allowed.size(); ++i) {
            if (i > 0) allowHeader += ", ";
            allowHeader += allowed[i];
        }
        if (allowHeader.find("OPTIONS") == std::string::npos) {
            if (!allowHeader.empty()) allowHeader += ", ";
            allowHeader += "OPTIONS";
        }
        if (allowHeader.find("HEAD") == std::string::npos) {
            if (!allowHeader.empty()) allowHeader += ", ";
            allowHeader += "HEAD";
        }
        res.setHeader("Allow", allowHeader);
        res.setBody("Method Not Allowed");
        return res;
    }

    static HttpResponse tooManyRequests(std::string msg = "Too Many Requests", int retryAfter = 60) {
        HttpResponse res;
        res.statusCode = 429;
        res.statusText = "Too Many Requests";
        res.setHeader("Retry-After", std::to_string(retryAfter));
        res.setBody(std::move(msg));
        return res;
    }

    static HttpResponse internalError(std::string msg = "Internal Server Error") {
        HttpResponse res;
        res.statusCode = 500;
        res.statusText = "Internal Server Error";
        res.setBody(std::move(msg));
        return res;
    }
};

struct HttpClientOptions {
    int timeoutMs{5000};
    // TLS (Phase 2): set automatically by HttpClient::get/post for https:// URLs.
    bool useTls{false};
    // Trust this extra CA bundle (PEM file) or CA directory in addition to the
    // system CA store. Used by tests with a locally generated CA.
    std::string caFile;
    std::string caPath;
    // DANGER: skips certificate-chain AND hostname verification. Opt-in only,
    // never the default. Intended for local development against self-signed
    // test servers only; must never be enabled in production.
    bool insecureSkipVerify{false};
};

class HttpClient {
public:
    static HttpResponse send(const HttpRequest& req, const std::string& host, int port, int timeoutMs = 5000);
    static HttpResponse send(const HttpRequest& req, const std::string& host, int port, const HttpClientOptions& options);
    static HttpResponse get(const std::string& url, int timeoutMs = 5000);
    static HttpResponse get(const std::string& url, const HttpClientOptions& options);
    static HttpResponse post(const std::string& url, const std::string& body,
                             const std::string& contentType = "application/json", int timeoutMs = 5000);
    static HttpResponse post(const std::string& url, const std::string& body,
                             const std::string& contentType, const HttpClientOptions& options);
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;
using HttpMiddleware = std::function<bool(const HttpRequest&, HttpResponse&)>;

// ----------------------------------------------------------------------------
// Radix-Tree Router & Route Groups (SP 2.3)
// ----------------------------------------------------------------------------

struct RouteMatchResult {
    bool found{false};
    bool methodNotAllowed{false};
    std::vector<std::string> allowedMethods;
    HttpHandler handler;
    std::vector<HttpMiddleware> middlewares;
    std::unordered_map<std::string, std::string> pathParams;
};

class RadixRouter {
public:
    enum class NodeType {
        Exact,
        Param,    // :id
        Wildcard  // *filepath
    };

    struct StringHash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept {
            return std::hash<std::string_view>{}(sv);
        }
        size_t operator()(const std::string& s) const noexcept {
            return std::hash<std::string_view>{}(s);
        }
    };

    struct StringEqual {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const noexcept {
            return a == b;
        }
    };

    struct Node {
        NodeType type{NodeType::Exact};
        std::string segment;
        std::string paramName;
        std::unordered_map<std::string, HttpHandler, StringHash, StringEqual> handlers;
        std::vector<HttpMiddleware> middlewares;
        std::vector<std::unique_ptr<Node>> children;

        // O(1) indexed children
        std::unordered_map<std::string, Node*, StringHash, StringEqual> exactChildren;
        Node* paramChild{nullptr};
        Node* wildcardChild{nullptr};

        Node* findChild(NodeType t, std::string_view seg) const;
    };

    RadixRouter();

    void addRoute(const std::string& method, const std::string& pattern, 
                  HttpHandler handler, std::vector<HttpMiddleware> middlewares = {});

    [[nodiscard]] RouteMatchResult match(const std::string& method, const std::string& path) const;

    [[nodiscard]] size_t routeCount() const noexcept { return m_routeCount; }

private:
    std::unique_ptr<Node> m_root;
    size_t m_routeCount{0};

    static std::vector<std::string_view> splitPath(std::string_view path);
    void matchRecursive(const Node* node, const std::vector<std::string_view>& segments, size_t segIdx,
                        const std::string& method,
                        std::vector<HttpMiddleware>& accumulatedMiddlewares,
                        std::vector<std::pair<std::string_view, std::string>>& accumulatedParams,
                        RouteMatchResult& bestResult) const;
};

class RouteGroup {
public:
    RouteGroup(std::string prefix, RadixRouter* router, std::vector<HttpMiddleware> middlewares = {});

    RouteGroup group(const std::string& subPrefix);
    void use(HttpMiddleware middleware);

    void get(const std::string& pattern, HttpHandler handler);
    void post(const std::string& pattern, HttpHandler handler);
    void put(const std::string& pattern, HttpHandler handler);
    void patch(const std::string& pattern, HttpHandler handler);
    void deleteRoute(const std::string& pattern, HttpHandler handler);
    void options(const std::string& pattern, HttpHandler handler);
    void head(const std::string& pattern, HttpHandler handler);
    void route(const std::string& method, const std::string& pattern, HttpHandler handler);

    [[nodiscard]] const std::string& getPrefix() const noexcept { return m_prefix; }

private:
    std::string m_prefix;
    RadixRouter* m_router{nullptr};
    std::vector<HttpMiddleware> m_middlewares;

    std::string fullPath(const std::string& pattern) const;
};

struct HttpServerConfig {
    int workerThreads{4};
    size_t maxBodySize{10 * 1024 * 1024};    // 10 MB limit
    size_t maxHeaderSize{16 * 1024};         // 16 KB header limit
    int idleTimeoutMs{15000};                // 15 seconds keep-alive timeout
    int requestTimeoutMs{30000};             // 30 seconds
    int maxKeepAliveRequests{1000};          // Requests before closing connection

    // TLS (Phase 2): when tlsEnabled is true the server speaks HTTPS.
    // tlsCertFile is a PEM certificate chain file, tlsKeyFile the matching PEM
    // private key. Plain HTTP keeps working unchanged when tlsEnabled is false.
    // listen() FAILS (returns false, see getLastError()) if TLS is enabled but
    // the files are missing, unreadable, or the key does not match the cert —
    // the server never silently falls back to plaintext.
    bool tlsEnabled{false};
    std::string tlsCertFile;
    std::string tlsKeyFile;
};

struct CorsOptions {
    std::string allowedOrigins{"*"};
    std::string allowedMethods{"GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD"};
    std::string allowedHeaders{"Content-Type, Authorization, X-Requested-With, X-Request-ID"};
    std::string exposedHeaders{"Content-Length, X-Request-ID"};
    bool allowCredentials{true};
    int maxAge{86400};
};

struct RateLimiterOptions {
    int maxRequests{100};
    int windowSeconds{60};
};

class HttpServer {
public:
    explicit HttpServer(HttpServerConfig config = {});
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void get(const std::string& pattern, HttpHandler handler);
    void post(const std::string& pattern, HttpHandler handler);
    void put(const std::string& pattern, HttpHandler handler);
    void patch(const std::string& pattern, HttpHandler handler);
    void deleteRoute(const std::string& pattern, HttpHandler handler);
    void options(const std::string& pattern, HttpHandler handler);
    void head(const std::string& pattern, HttpHandler handler);
    void route(const std::string& method, const std::string& pattern, HttpHandler handler);

    RouteGroup group(const std::string& prefix);

    void use(HttpMiddleware middleware);

    bool listen(const std::string& host, int port, int workerThreads = 4);
    bool listen(const std::string& host, int port, const HttpServerConfig& config);
    void wait();
    void stop();
    [[nodiscard]] bool isRunning() const { return m_running.load(); }
    [[nodiscard]] int getPort() const { return m_port; }

    void setConfig(const HttpServerConfig& config) { m_config = config; }
    [[nodiscard]] const HttpServerConfig& getConfig() const noexcept { return m_config; }
    // Last fatal error from listen() (e.g. TLS misconfiguration). Empty if none.
    [[nodiscard]] const std::string& getLastError() const noexcept { return m_lastError; }
    [[nodiscard]] bool isTlsEnabled() const noexcept { return m_tlsCtx != nullptr; }

    [[nodiscard]] RadixRouter& getRouter() noexcept { return m_router; }
    [[nodiscard]] const RadixRouter& getRouter() const noexcept { return m_router; }

    // In-memory request handling (e.g. for testing and direct routing)
    HttpResponse handle(const HttpRequest& req);
    HttpResponse dispatch(const HttpRequest& req);

    // Built-in Middleware factories
    static HttpMiddleware cors(CorsOptions opts = {});
    static HttpMiddleware logger();
    static HttpMiddleware recovery();
    static HttpMiddleware requestId();
    static HttpMiddleware rateLimiter(RateLimiterOptions opts = {});

private:
    void serverLoop();
    void workerLoop();
    void handleClient(std::unique_ptr<net::TcpSocket> client);

    HttpServerConfig m_config;
    RadixRouter m_router;
    std::vector<HttpMiddleware> m_middlewares;
    net::TcpListener m_listener;
    std::atomic<bool> m_running{false};
    int m_port{0};

    std::unique_ptr<std::thread> m_acceptThread;
    // Phase 6 slice C: guards the accept-thread join. wait() and
    // stop() may both join it (wait() blocks in it while another
    // thread calls stop()) — joining the same std::thread twice is
    // undefined behaviour and hung in practice, so exactly one of
    // them joins and the other sees a non-joinable thread. The mutex
    // is held across the join: that cannot deadlock, because stop()
    // clears m_running (which is what lets the accept thread exit)
    // BEFORE taking this mutex.
    std::mutex m_acceptJoinMtx;
    std::vector<std::thread> m_workerThreads;
    std::deque<std::unique_ptr<net::TcpSocket>> m_connectionQueue;
    std::mutex m_queueMtx;
    std::condition_variable m_queueCv;
    std::atomic<int> m_activeRequests{0};

    // TLS server context (SSL_CTX*, opaque here so this header does not need
    // OpenSSL includes). Non-null only when TLS is enabled and listen()
    // succeeded. Owned by the server; freed in stop().
    void* m_tlsCtx{nullptr};
    std::string m_lastError;
};

} // namespace sp::rt::http
