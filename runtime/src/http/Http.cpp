#include "sp/runtime/Http.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <chrono>

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

int hexDigitValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

} // anonymous namespace

// ----------------------------------------------------------------------------
// URL Encoding / Decoding
// ----------------------------------------------------------------------------

std::string urlDecode(std::string_view str) {
    std::string res;
    res.reserve(str.size());
    for (size_t i = 0; i < str.size(); ++i) {
        if (str[i] == '+') {
            res += ' ';
        } else if (str[i] == '%' && i + 2 < str.size()) {
            int d1 = hexDigitValue(str[i + 1]);
            int d2 = hexDigitValue(str[i + 2]);
            if (d1 >= 0 && d2 >= 0) {
                res += static_cast<char>((d1 << 4) | d2);
                i += 2;
            } else {
                res += '%';
            }
        } else {
            res += str[i];
        }
    }
    return res;
}

std::string urlEncode(std::string_view str) {
    std::ostringstream ss;
    for (unsigned char c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            ss << static_cast<char>(c);
        } else {
            ss << '%' << "0123456789ABCDEF"[c >> 4] << "0123456789ABCDEF"[c & 0x0F];
        }
    }
    return ss.str();
}

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
    auto it = queryParams.find(name);
    return (it != queryParams.end()) ? it->second : "";
}

std::string HttpRequest::getPathParam(const std::string& name) const {
    auto it = pathParams.find(name);
    return (it != pathParams.end()) ? it->second : "";
}

std::string HttpRequest::getCookie(const std::string& name) const {
    auto it = cookies.find(name);
    return (it != cookies.end()) ? it->second : "";
}

bool HttpRequest::isKeepAlive() const {
    std::string conn = getHeader("Connection");
    if (!conn.empty()) {
        return !equalsIgnoreCase(conn, "close");
    }
    return true; // HTTP/1.1 defaults to keep-alive
}

int64_t HttpRequest::getContentLength() const {
    std::string cl = getHeader("Content-Length");
    if (cl.empty()) return 0;
    try {
        return std::stoll(cl);
    } catch (...) {
        return 0;
    }
}

std::string HttpRequest::getContentType() const {
    return getHeader("Content-Type");
}

bool HttpRequest::parseRequest(const std::string& rawData, HttpRequest& outReq, size_t& outBytesConsumed, bool requireComplete) {
    outBytesConsumed = 0;
    if (rawData.empty()) return false;

    // Locate header-body delimiter
    size_t headerEnd = rawData.find("\r\n\r\n");
    size_t delimiterLen = 4;
    if (headerEnd == std::string::npos) {
        headerEnd = rawData.find("\n\n");
        delimiterLen = 2;
        if (headerEnd == std::string::npos) {
            return false; // Incomplete headers
        }
    }

    std::string headerPart = rawData.substr(0, headerEnd);
    std::istringstream stream(headerPart);
    std::string line;

    // 1. Request Line: GET /path?query HTTP/1.1
    if (!std::getline(stream, line)) {
        return false;
    }
    line = trim(line);
    std::istringstream lineStream(line);
    lineStream >> outReq.method;
    std::string fullPath;
    lineStream >> fullPath;

    size_t qPos = fullPath.find('?');
    if (qPos != std::string::npos) {
        outReq.path = urlDecode(fullPath.substr(0, qPos));
        outReq.queryString = fullPath.substr(qPos + 1);

        // Parse query params
        size_t pos = 0;
        while (pos < outReq.queryString.size()) {
            size_t nextAmp = outReq.queryString.find('&', pos);
            std::string pair = (nextAmp == std::string::npos) ? outReq.queryString.substr(pos) : outReq.queryString.substr(pos, nextAmp - pos);
            size_t eqPos = pair.find('=');
            if (eqPos != std::string::npos) {
                std::string k = urlDecode(pair.substr(0, eqPos));
                std::string v = urlDecode(pair.substr(eqPos + 1));
                outReq.queryParams[std::move(k)] = std::move(v);
            } else if (!pair.empty()) {
                outReq.queryParams[urlDecode(pair)] = "";
            }
            if (nextAmp == std::string::npos) break;
            pos = nextAmp + 1;
        }
    } else {
        outReq.path = urlDecode(fullPath.empty() ? "/" : fullPath);
        outReq.queryString.clear();
        outReq.queryParams.clear();
    }

    // 2. Headers
    outReq.headers.clear();
    outReq.cookies.clear();
    size_t contentLength = 0;
    bool isChunked = false;

    while (std::getline(stream, line)) {
        line = trim(line);
        if (line.empty()) continue;

        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string hName = trim(line.substr(0, colon));
            std::string hVal = trim(line.substr(colon + 1));

            if (equalsIgnoreCase(hName, "Content-Length")) {
                try { contentLength = static_cast<size_t>(std::stoul(hVal)); } catch (...) {}
            } else if (equalsIgnoreCase(hName, "Transfer-Encoding") && equalsIgnoreCase(hVal, "chunked")) {
                isChunked = true;
            } else if (equalsIgnoreCase(hName, "Cookie")) {
                // Parse Cookie: a=1; b=2
                size_t cPos = 0;
                while (cPos < hVal.size()) {
                    size_t semi = hVal.find(';', cPos);
                    std::string cPair = (semi == std::string::npos) ? hVal.substr(cPos) : hVal.substr(cPos, semi - cPos);
                    cPair = trim(cPair);
                    size_t cEq = cPair.find('=');
                    if (cEq != std::string::npos) {
                        outReq.cookies[trim(cPair.substr(0, cEq))] = trim(cPair.substr(cEq + 1));
                    }
                    if (semi == std::string::npos) break;
                    cPos = semi + 1;
                }
            }

            outReq.headers.push_back({std::move(hName), std::move(hVal)});
        }
    }

    size_t bodyOffset = headerEnd + delimiterLen;

    // 3. Body Handling
    if (isChunked) {
        // Chunked decoding: read chunks until 0\r\n\r\n
        std::string decodedBody;
        size_t cur = bodyOffset;
        while (true) {
            size_t crlf = rawData.find("\r\n", cur);
            if (crlf == std::string::npos) return false; // Need more data

            std::string hexLenStr = trim(rawData.substr(cur, crlf - cur));
            size_t chunkSize = 0;
            try {
                chunkSize = std::stoul(hexLenStr, nullptr, 16);
            } catch (...) {
                return false;
            }

            if (chunkSize == 0) {
                // Terminating chunk
                cur = crlf + 2;
                if (cur + 2 <= rawData.size() && rawData.substr(cur, 2) == "\r\n") {
                    cur += 2;
                }
                outReq.body = std::move(decodedBody);
                outBytesConsumed = cur;
                return true;
            }

            size_t dataStart = crlf + 2;
            if (dataStart + chunkSize + 2 > rawData.size()) {
                return false; // Incomplete chunk data
            }

            decodedBody.append(rawData.substr(dataStart, chunkSize));
            cur = dataStart + chunkSize + 2; // skip data + \r\n
        }
    } else {
        // Standard Content-Length
        if (requireComplete && rawData.size() < bodyOffset + contentLength) {
            return false; // Complete body has not yet arrived
        }
        size_t take = (contentLength > 0) ? std::min(contentLength, rawData.size() - bodyOffset) : (rawData.size() - bodyOffset);
        outReq.body = rawData.substr(bodyOffset, take);
        outBytesConsumed = bodyOffset + take;
        return true;
    }
}

HttpRequest HttpRequest::parse(const std::string& rawHttp) {
    HttpRequest req;
    size_t consumed = 0;
    parseRequest(rawHttp, req, consumed, /*requireComplete=*/false);
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

void HttpResponse::setCookie(const std::string& name, const std::string& value, 
                            const std::string& path, int maxAge, 
                            bool secure, bool httpOnly,
                            const std::string& sameSite) {
    std::string cookieStr = name + "=" + value + "; Path=" + path;
    if (maxAge >= 0) {
        cookieStr += "; Max-Age=" + std::to_string(maxAge);
    }
    if (secure) {
        cookieStr += "; Secure";
    }
    if (httpOnly) {
        cookieStr += "; HttpOnly";
    }
    if (!sameSite.empty()) {
        cookieStr += "; SameSite=" + sameSite;
    }
    setHeader("Set-Cookie", cookieStr);
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

void HttpResponse::setText(std::string textContent) {
    body = std::move(textContent);
    setHeader("Content-Length", std::to_string(body.size()));
    setHeader("Content-Type", "text/plain; charset=utf-8");
}

void HttpResponse::setHtml(std::string htmlContent) {
    body = std::move(htmlContent);
    setHeader("Content-Length", std::to_string(body.size()));
    setHeader("Content-Type", "text/html; charset=utf-8");
}

std::string HttpResponse::serialize(bool keepAlive) const {
    std::ostringstream ss;
    ss << "HTTP/1.1 " << statusCode << " " << statusText << "\r\n";
    for (const auto& h : headers) {
        ss << h.name << ": " << h.value << "\r\n";
    }
    if (getHeader("Connection").empty()) {
        ss << "Connection: " << (keepAlive ? "keep-alive" : "close") << "\r\n";
    }
    if (getHeader("Content-Length").empty() && !isChunked) {
        ss << "Content-Length: " << body.size() << "\r\n";
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

    std::string fullResp;
    while (true) {
        std::string chunk = sock.readString(8192);
        if (chunk.empty()) break;
        fullResp += chunk;
    }
    sock.close();

    if (fullResp.empty()) {
        return HttpResponse::internalError("Empty response from server");
    }

    HttpResponse res;
    std::istringstream respStream(fullResp);
    std::string line;

    if (std::getline(respStream, line)) {
        line = trim(line);
        std::istringstream statusLine(line);
        std::string httpVersion;
        statusLine >> httpVersion >> res.statusCode;
        std::getline(statusLine, res.statusText);
        res.statusText = trim(res.statusText);
    }

    size_t bodyLen = 0;
    while (std::getline(respStream, line)) {
        line = trim(line);
        if (line.empty()) break;
        size_t colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = trim(line.substr(0, colon));
            std::string val = trim(line.substr(colon + 1));
            if (equalsIgnoreCase(name, "Content-Length")) {
                try { bodyLen = std::stoul(val); } catch (...) {}
            }
            res.setHeader(name, val);
        }
    }

    size_t bodyPos = fullResp.find("\r\n\r\n");
    if (bodyPos != std::string::npos) {
        bodyPos += 4;
        res.body = fullResp.substr(bodyPos, bodyLen > 0 ? bodyLen : std::string::npos);
    }

    return res;
}

HttpResponse HttpClient::get(const std::string& url, int timeoutMs) {
    std::string host = "127.0.0.1";
    int port = 80;
    std::string path = "/";

    std::string clean = url;
    if (clean.rfind("http://", 0) == 0) clean = clean.substr(7);

    size_t slash = clean.find('/');
    std::string hostPort = (slash == std::string::npos) ? clean : clean.substr(0, slash);
    if (slash != std::string::npos) path = clean.substr(slash);

    size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        host = hostPort.substr(0, colon);
        port = std::stoi(hostPort.substr(colon + 1));
    } else {
        host = hostPort;
    }

    HttpRequest req;
    req.method = "GET";
    req.path = path;
    req.setHeader("User-Agent", "SP-Http-Client/2.2");
    return send(req, host, port, timeoutMs);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body, 
                              const std::string& contentType, int timeoutMs) {
    std::string host = "127.0.0.1";
    int port = 80;
    std::string path = "/";

    std::string clean = url;
    if (clean.rfind("http://", 0) == 0) clean = clean.substr(7);

    size_t slash = clean.find('/');
    std::string hostPort = (slash == std::string::npos) ? clean : clean.substr(0, slash);
    if (slash != std::string::npos) path = clean.substr(slash);

    size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        host = hostPort.substr(0, colon);
        port = std::stoi(hostPort.substr(colon + 1));
    } else {
        host = hostPort;
    }

    HttpRequest req;
    req.method = "POST";
    req.path = path;
    req.body = body;
    req.setHeader("Content-Type", contentType);
    req.setHeader("User-Agent", "SP-Http-Client/2.2");
    return send(req, host, port, timeoutMs);
}

// ----------------------------------------------------------------------------
// HttpServer Methods (Multi-Threaded & Keep-Alive RFC 7230)
// ----------------------------------------------------------------------------

HttpServer::HttpServer(HttpServerConfig config)
    : m_config(config) {}

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

void HttpServer::patch(const std::string& pattern, HttpHandler handler) {
    route("PATCH", pattern, std::move(handler));
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

bool HttpServer::listen(const std::string& host, int port, int workerThreads) {
    HttpServerConfig cfg = m_config;
    if (workerThreads > 0) cfg.workerThreads = workerThreads;
    return listen(host, port, cfg);
}

bool HttpServer::listen(const std::string& host, int port, const HttpServerConfig& config) {
    m_config = config;
    if (m_config.workerThreads <= 0) {
        m_config.workerThreads = std::max(2u, std::thread::hardware_concurrency());
    }

    if (!m_listener.bindAndListen(host, port, 1024)) {
        return false;
    }
    m_port = m_listener.getPort();
    m_running.store(true);

    // Start worker threads pool
    m_workerThreads.clear();
    for (int i = 0; i < m_config.workerThreads; ++i) {
        m_workerThreads.emplace_back(&HttpServer::workerLoop, this);
    }

    // Start accept thread
    m_acceptThread = std::make_unique<std::thread>(&HttpServer::serverLoop, this);
    return true;
}

void HttpServer::stop() {
    if (m_running.exchange(false)) {
        m_listener.close();

        // Wake all worker threads
        m_queueCv.notify_all();

        // Wait for accept thread
        if (m_acceptThread && m_acceptThread->joinable()) {
            m_acceptThread->join();
        }
        m_acceptThread.reset();

        // Wait for workers to finish in-flight connections
        for (auto& t : m_workerThreads) {
            if (t.joinable()) {
                t.join();
            }
        }
        m_workerThreads.clear();

        // Drain remaining queued connections
        std::lock_guard<std::mutex> lock(m_queueMtx);
        while (!m_connectionQueue.empty()) {
            m_connectionQueue.pop_front();
        }
    }
}

void HttpServer::serverLoop() {
    while (m_running.load()) {
        auto client = m_listener.acceptTimeout(100);
        if (client && client->isValid()) {
            std::lock_guard<std::mutex> lock(m_queueMtx);
            m_connectionQueue.push_back(std::move(client));
            m_queueCv.notify_one();
        }
    }
}

void HttpServer::workerLoop() {
    while (m_running.load()) {
        std::unique_ptr<net::TcpSocket> client;
        {
            std::unique_lock<std::mutex> lock(m_queueMtx);
            m_queueCv.wait(lock, [this]() {
                return !m_running.load() || !m_connectionQueue.empty();
            });

            if (!m_connectionQueue.empty()) {
                client = std::move(m_connectionQueue.front());
                m_connectionQueue.pop_front();
            } else if (!m_running.load()) {
                break;
            }
        }

        if (client && client->isValid()) {
            handleClient(std::move(client));
        }
    }
}

void HttpServer::handleClient(std::unique_ptr<net::TcpSocket> client) {
    std::string buffer;
    int requestCount = 0;

    // Persistent HTTP/1.1 keep-alive loop
    while (m_running.load() && client && client->isValid()) {
        std::string chunk = client->readString(16384);
        if (chunk.empty()) {
            if (buffer.empty()) {
                break; // Client closed connection cleanly
            }
        } else {
            buffer += chunk;
        }

        HttpRequest req;
        size_t bytesConsumed = 0;
        if (HttpRequest::parseRequest(buffer, req, bytesConsumed)) {
            m_activeRequests++;
            buffer.erase(0, bytesConsumed);
            requestCount++;

            HttpResponse res;

            // Enforce body limit
            if (req.body.size() > m_config.maxBodySize) {
                res = HttpResponse::payloadTooLarge();
            } else {
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
            }

            bool keepAlive = req.isKeepAlive() && 
                             (requestCount < m_config.maxKeepAliveRequests) && 
                             m_running.load();

            std::string rawRes = res.serialize(keepAlive);
            client->writeString(rawRes);
            m_activeRequests--;

            if (!keepAlive) {
                break;
            }
        } else {
            // Check for oversized header DOS attack
            if (buffer.size() > m_config.maxHeaderSize && buffer.find("\r\n\r\n") == std::string::npos) {
                HttpResponse res = HttpResponse::status(431, "Request Header Fields Too Large");
                client->writeString(res.serialize(false));
                break;
            }
            if (chunk.empty()) {
                // Incomplete request and client closed connection
                break;
            }
        }
    }

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
