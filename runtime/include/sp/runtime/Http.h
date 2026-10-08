#pragma once

#include "sp/runtime/Net.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <atomic>
#include <thread>

namespace sp::rt::http {

struct Header {
    std::string name;
    std::string value;
};

class HttpRequest {
public:
    std::string method{"GET"};
    std::string path{"/"};
    std::string queryString;
    std::vector<Header> headers;
    std::string body;
    std::unordered_map<std::string, std::string> pathParams;

    std::string getHeader(const std::string& name) const;
    void setHeader(const std::string& name, const std::string& value);
    std::string getQueryParam(const std::string& name) const;
    std::string getPathParam(const std::string& name) const;

    static HttpRequest parse(const std::string& rawHttp);
};

class HttpResponse {
public:
    int statusCode{200};
    std::string statusText{"OK"};
    std::vector<Header> headers;
    std::string body;

    void setHeader(const std::string& name, const std::string& value);
    std::string getHeader(const std::string& name) const;
    void setBody(std::string content);
    void setJson(const std::string& jsonContent);

    std::string serialize() const;

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
        res.statusText = (code == 200) ? "OK" : "Status " + std::to_string(code);
        res.setJson(jsonContent);
        return res;
    }

    static HttpResponse notFound(std::string msg = "Not Found") {
        HttpResponse res;
        res.statusCode = 404;
        res.statusText = "Not Found";
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

class HttpServer {
public:
    HttpServer();
    ~HttpServer();

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    void get(const std::string& pattern, HttpHandler handler);
    void post(const std::string& pattern, HttpHandler handler);
    void put(const std::string& pattern, HttpHandler handler);
    void deleteRoute(const std::string& pattern, HttpHandler handler);
    void route(const std::string& method, const std::string& pattern, HttpHandler handler);

    void use(HttpMiddleware middleware);

    bool listen(const std::string& host, int port, int workerThreads = 4);
    void stop();
    bool isRunning() const { return m_running.load(); }
    int getPort() const { return m_port; }

private:
    struct Route {
        std::string method;
        std::string pattern;
        std::vector<std::string> paramNames;
        HttpHandler handler;
    };

    void serverLoop();
    void handleClient(std::unique_ptr<net::TcpSocket> client);
    HttpResponse dispatch(const HttpRequest& req);

    std::vector<Route> m_routes;
    std::vector<HttpMiddleware> m_middlewares;
    net::TcpListener m_listener;
    std::atomic<bool> m_running{false};
    int m_port{0};
    std::unique_ptr<std::thread> m_acceptThread;
};

} // namespace sp::rt::http
