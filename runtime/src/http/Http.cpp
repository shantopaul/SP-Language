#include "sp/runtime/Http.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace sp::rt::http {

namespace {

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool equalsIgnoreCase(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// HttpRequest Methods
// ----------------------------------------------------------------------------

std::string HttpRequest::getHeader(const std::string& name) const {
    for (const auto& h : headers) {
        if (equalsIgnoreCase(h.name, name)) {
            return h.value;
        }
    }
    return "";
}

void HttpRequest::setHeader(const std::string& name, const std::string& value) {
    for (auto& h : headers) {
        if (equalsIgnoreCase(h.name, name)) {
            h.value = value;
            return;
        }
    }
    headers.push_back({name, value});
}

std::string HttpRequest::getQueryParam(const std::string& name) const {
    if (queryString.empty()) return "";
    size_t pos = 0;
    while (pos < queryString.size()) {
        size_t nextAmp = queryString.find('&', pos);
        std::string pair = (nextAmp == std::string::npos) ? queryString.substr(pos) : queryString.substr(pos, nextAmp - pos);
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            if (key == name) {
                return pair.substr(eqPos + 1);
            }
        } else if (pair == name) {
            return "";
        }
        if (nextAmp == std::string::npos) break;
        pos = nextAmp + 1;
    }
    return "";
}

std::string HttpRequest::getPathParam(const std::string& name) const {
    auto it = pathParams.find(name);
    return (it != pathParams.end()) ? it->second : "";
}

HttpRequest HttpRequest::parse(const std::string& rawHttp) {
    HttpRequest req;
    if (rawHttp.empty()) return req;

    std::istringstream stream(rawHttp);
    std::string line;

    // 1. Request Line: GET /path?query HTTP/1.1
    if (std::getline(stream, line)) {
        line = trim(line);
        std::istringstream lineStream(line);
        lineStream >> req.method;
        std::string fullPath;
        lineStream >> fullPath;

        size_t qPos = fullPath.find('?');
        if (qPos != std::string::npos) {
            req.path = fullPath.substr(0, qPos);
            req.queryString = fullPath.substr(qPos + 1);
        } else {
            req.path = fullPath.empty() ? "/" : fullPath;
        }
    }

    // 2. Headers
    size_t contentLength = 0;
    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) {
            break; // Header section ended
        }
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string hName = trim(line.substr(0, colon));
            std::string hVal = trim(line.substr(colon + 1));
            if (equalsIgnoreCase(hName, "Content-Length")) {
                try { contentLength = static_cast<size_t>(std::stoul(hVal)); } catch (...) {}
            }
            req.headers.push_back({std::move(hName), std::move(hVal)});
        }
    }

    // 3. Body
    size_t bodyStart = rawHttp.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        bodyStart += 4;
        req.body = rawHttp.substr(bodyStart, contentLength > 0 ? contentLength : std::string::npos);
    } else {
        bodyStart = rawHttp.find("\n\n");
        if (bodyStart != std::string::npos) {
            bodyStart += 2;
            req.body = rawHttp.substr(bodyStart, contentLength > 0 ? contentLength : std::string::npos);
        }
    }

    return req;
}

// ----------------------------------------------------------------------------
// HttpResponse Methods
// ----------------------------------------------------------------------------

void HttpResponse::setHeader(const std::string& name, const std::string& value) {
    for (auto& h : headers) {
        if (equalsIgnoreCase(h.name, name)) {
            h.value = value;
            return;
        }
    }
    headers.push_back({name, value});
}

std::string HttpResponse::getHeader(const std::string& name) const {
    for (const auto& h : headers) {
        if (equalsIgnoreCase(h.name, name)) {
            return h.value;
        }
    }
    return "";
}

void HttpResponse::setBody(std::string content) {
    body = std::move(content);
    setHeader("Content-Length", std::to_string(body.size()));
    if (getHeader("Content-Type").empty()) {
        setHeader("Content-Type", "text/plain; charset=utf-8");
    }
}

void HttpResponse::setJson(const std::string& jsonContent) {
    body = jsonContent;
    setHeader("Content-Length", std::to_string(body.size()));
    setHeader("Content-Type", "application/json; charset=utf-8");
}

std::string HttpResponse::serialize() const {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    for (const auto& h : headers) {
        ss << h.name << ": " << h.value << "\r\n";
    }
    if (getHeader("Connection").empty()) {
        ss << "Connection: close\r\n";
    }
    ss << "\r\n";
    ss << body;
    return ss.str();
}

// ----------------------------------------------------------------------------
// HttpClient Methods
// ----------------------------------------------------------------------------

HttpResponse HttpClient::send(const HttpRequest& req, const std::string& host, int port, int timeoutMs) {
    net::TcpSocket sock;
    if (!sock.connect(host, port, timeoutMs)) {
        return HttpResponse::internalError("Failed to connect to " + host + ":" + std::to_string(port));
    }

    std::ostringstream reqStream;
    std::string pathWithQuery = req.path;
    if (!req.queryString.empty()) {
        pathWithQuery += "?" + req.queryString;
    }
    reqStream << req.method << " " << pathWithQuery << " HTTP/1.1\r\n";
    reqStream << "Host: " << host << ":" << port << "\r\n";
    for (const auto& h : req.headers) {
        reqStream << h.name << ": " << h.value << "\r\n";
    }
    if (req.getHeader("Content-Length").empty() && !req.body.empty()) {
        reqStream << "Content-Length: " << req.body.size() << "\r\n";
    }
    if (req.getHeader("Connection").empty()) {
        reqStream << "Connection: close\r\n";
    }
    reqStream << "\r\n";
    reqStream << req.body;

    std::string reqStr = reqStream.str();
    sock.writeString(reqStr);

    std::string rawResp = sock.readAll(timeoutMs);
    sock.close();

    HttpResponse resp;
    if (rawResp.empty()) {
        return HttpResponse::internalError("Empty response from server");
    }

    std::istringstream respStream(rawResp);
    std::string line;
    if (std::getline(respStream, line)) {
        line = trim(line);
        std::istringstream statusLine(line);
        std::string httpVer;
        statusLine >> httpVer >> resp.statusCode;
        std::getline(statusLine, resp.statusText);
        resp.statusText = trim(resp.statusText);
    }

    size_t contentLength = 0;
    while (std::getline(respStream, line)) {
        line = trim(line);
        if (line.empty()) break;
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string hName = trim(line.substr(0, colon));
            std::string hVal = trim(line.substr(colon + 1));
            if (equalsIgnoreCase(hName, "Content-Length")) {
                try { contentLength = static_cast<size_t>(std::stoul(hVal)); } catch (...) {}
            }
            resp.headers.push_back({std::move(hName), std::move(hVal)});
        }
    }

    size_t bodyStart = rawResp.find("\r\n\r\n");
    if (bodyStart != std::string::npos) {
        bodyStart += 4;
        resp.body = rawResp.substr(bodyStart, contentLength > 0 ? contentLength : std::string::npos);
    }

    return resp;
}

HttpResponse HttpClient::get(const std::string& url, int timeoutMs) {
    // Parse http://host:port/path
    std::string host = "127.0.0.1";
    int port = 80;
    std::string path = "/";

    std::string u = url;
    if (u.rfind("http://", 0) == 0) {
        u = u.substr(7);
    }
    size_t slashPos = u.find('/');
    std::string hostPort = (slashPos != std::string::npos) ? u.substr(0, slashPos) : u;
    if (slashPos != std::string::npos) {
        path = u.substr(slashPos);
    }
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
        host = hostPort.substr(0, colonPos);
        port = std::stoi(hostPort.substr(colonPos + 1));
    } else {
        host = hostPort;
    }

    HttpRequest req;
    req.method = "GET";
    size_t qPos = path.find('?');
    if (qPos != std::string::npos) {
        req.path = path.substr(0, qPos);
        req.queryString = path.substr(qPos + 1);
    } else {
        req.path = path;
    }
    req.setHeader("User-Agent", "SP-Language/1.6");
    req.setHeader("Accept", "*/*");

    return send(req, host, port, timeoutMs);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, 
                              const std::string& contentType, int timeoutMs) {
    std::string host = "127.0.0.1";
    int port = 80;
    std::string path = "/";

    std::string u = url;
    if (u.rfind("http://", 0) == 0) {
        u = u.substr(7);
    }
    size_t slashPos = u.find('/');
    std::string hostPort = (slashPos != std::string::npos) ? u.substr(0, slashPos) : u;
    if (slashPos != std::string::npos) {
        path = u.substr(slashPos);
    }
    size_t colonPos = hostPort.find(':');
    if (colonPos != std::string::npos) {
        host = hostPort.substr(0, colonPos);
        port = std::stoi(hostPort.substr(colonPos + 1));
    } else {
        host = hostPort;
    }

    HttpRequest req;
    req.method = "POST";
    size_t qPos = path.find('?');
    if (qPos != std::string::npos) {
        req.path = path.substr(0, qPos);
        req.queryString = path.substr(qPos + 1);
    } else {
        req.path = path;
    }
    req.setHeader("User-Agent", "SP-Language/1.6");
    req.setHeader("Content-Type", contentType);
    req.body = body;

    return send(req, host, port, timeoutMs);
}

// ----------------------------------------------------------------------------
// HttpServer Implementation
// ----------------------------------------------------------------------------

HttpServer::HttpServer() {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::get(const std::string& pattern, HttpHandler handler) {
    route("GET", pattern, std::move(handler));
}

void HttpServer::post(const std::string& pattern, HttpHandler handler) {
    route("POST", pattern, std::move(handler));
}

void HttpServer::put(const std::string& pattern, HttpHandler handler) {
    route("PUT", pattern, std::move(handler));
}

void HttpServer::deleteRoute(const std::string& pattern, HttpHandler handler) {
    route("DELETE", pattern, std::move(handler));
}

void HttpServer::route(const std::string& method, const std::string& pattern, HttpHandler handler) {
    Route r;
    r.method = method;
    r.pattern = pattern;
    r.handler = std::move(handler);

    // Extract :param names
    std::istringstream pStream(pattern);
    std::string part;
    while (std::getline(pStream, part, '/')) {
        if (!part.empty() && part.front() == ':') {
            r.paramNames.push_back(part.substr(1));
        }
    }
    m_routes.push_back(std::move(r));
}

void HttpServer::use(HttpMiddleware middleware) {
    m_middlewares.push_back(std::move(middleware));
}

bool HttpServer::listen(const std::string& host, int port, int /*workerThreads*/) {
    if (!m_listener.bindAndListen(host, port, 256)) {
        return false;
    }
    m_port = m_listener.getPort();
    m_running.store(true);

    m_acceptThread = std::make_unique<std::thread>(&HttpServer::serverLoop, this);
    return true;
}

void HttpServer::stop() {
    if (m_running.exchange(false)) {
        m_listener.close();
        if (m_acceptThread && m_acceptThread->joinable()) {
            m_acceptThread->join();
        }
        m_acceptThread.reset();
    }
}

void HttpServer::serverLoop() {
    while (m_running.load()) {
        auto client = m_listener.acceptTimeout(200);
        if (client && client->isValid()) {
            handleClient(std::move(client));
        }
    }
}

void HttpServer::handleClient(std::unique_ptr<net::TcpSocket> client) {
    std::string rawReq = client->readString(65536);
    if (rawReq.empty()) return;

    HttpRequest req = HttpRequest::parse(rawReq);
    HttpResponse res;

    // Apply middlewares
    bool proceed = true;
    for (const auto& mw : m_middlewares) {
        if (!mw(req, res)) {
            proceed = false;
            break;
        }
    }

    if (proceed) {
        res = dispatch(req);
    }

    std::string rawRes = res.serialize();
    client->writeString(rawRes);
    client->close();
}

HttpResponse HttpServer::dispatch(const HttpRequest& req) {
    for (const auto& route : m_routes) {
        if (!route.method.empty() && !equalsIgnoreCase(route.method, req.method)) {
            continue;
        }

        // Exact match
        if (route.pattern == req.path) {
            return route.handler(req);
        }

        // Path parameter match e.g. /users/:id
        if (route.pattern.find(':') != std::string::npos) {
            std::istringstream patStream(route.pattern);
            std::istringstream reqStream(req.path);
            std::string patPart, reqPart;
            bool match = true;
            std::unordered_map<std::string, std::string> params;

            while (std::getline(patStream, patPart, '/') && std::getline(reqStream, reqPart, '/')) {
                if (patPart.empty() && reqPart.empty()) continue;
                if (!patPart.empty() && patPart.front() == ':') {
                    params[patPart.substr(1)] = reqPart;
                } else if (patPart != reqPart) {
                    match = false;
                    break;
                }
            }

            if (match && !patStream.good() && !reqStream.good()) {
                HttpRequest matchedReq = req;
                matchedReq.pathParams = std::move(params);
                return route.handler(matchedReq);
            }
        }
    }

    return HttpResponse::notFound("No route matched " + req.method + " " + req.path);
}

} // namespace sp::rt::http
