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

    static HttpResponse internalError(std::string msg = "Internal Server Error") {
        HttpResponse res;
        res.statusCode = 500;
        res.statusText = "Internal Server Error";
        res.setBody(std::move(msg));
        return res;
    }
};

class HttpClient {
public:
    static HttpResponse send(const HttpRequest& req, const std::string& host, int port, int timeoutMs = 5000);
    static HttpResponse get(const std::string& url, int timeoutMs = 5000);
    static HttpResponse post(const std::string& url, const std::string& body, 
                             const std::string& contentType = "application/json", int timeoutMs = 5000);
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;
using HttpMiddleware = std::function<bool(const HttpRequest&, HttpResponse&)>;

struct HttpServerConfig {
    int workerThreads{4};
    size_t maxBodySize{10 * 1024 * 1024};    // 10 MB limit
    size_t maxHeaderSize{16 * 1024};         // 16 KB header limit
    int idleTimeoutMs{15000};                // 15 seconds keep-alive timeout
    int requestTimeoutMs{30000};             // 30 seconds
    int maxKeepAliveRequests{1000};          // Requests before closing connection
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
    void route(const std::string& method, const std::string& pattern, HttpHandler handler);

    void use(HttpMiddleware middleware);

    bool listen(const std::string& host, int port, int workerThreads = 4);
    bool listen(const std::string& host, int port, const HttpServerConfig& config);
    void stop();
    [[nodiscard]] bool isRunning() const { return m_running.load(); }
    [[nodiscard]] int getPort() const { return m_port; }

    void setConfig(const HttpServerConfig& config) { m_config = config; }
    [[nodiscard]] const HttpServerConfig& getConfig() const noexcept { return m_config; }

private:
    struct Route {
        std::string method;
        std::string pattern;
        std::vector<std::string> paramNames;
        HttpHandler handler;
    };

    void serverLoop();
    void workerLoop();
    void handleClient(std::unique_ptr<net::TcpSocket> client);
    HttpResponse dispatch(const HttpRequest& req);

    HttpServerConfig m_config;
    std::vector<Route> m_routes;
    std::vector<HttpMiddleware> m_middlewares;
    net::TcpListener m_listener;
    std::atomic<bool> m_running{false};
    int m_port{0};

    std::unique_ptr<std::thread> m_acceptThread;
    std::vector<std::thread> m_workerThreads;
    std::deque<std::unique_ptr<net::TcpSocket>> m_connectionQueue;
    std::mutex m_queueMtx;
    std::condition_variable m_queueCv;
    std::atomic<int> m_activeRequests{0};
};

} // namespace sp::rt::http
