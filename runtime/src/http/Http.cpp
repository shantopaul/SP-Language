#include "sp/runtime/Http.h"
#include "sp/runtime/Crypto.h"
#include "sp/runtime/WebSocket.h"
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <chrono>

namespace sp::rt::http {

namespace {

// Drain OpenSSL's error queue into a readable string (and clear it).
std::string opensslErrorString() {
    std::string out;
    unsigned long err = 0;
    while ((err = ERR_get_error()) != 0) {
        char buf[256];
        ERR_error_string_n(err, buf, sizeof(buf));
        if (!out.empty()) out += "; ";
        out += buf;
    }
    if (out.empty()) out = "unknown OpenSSL error";
    return out;
}

// Write every byte, through TLS when ssl != nullptr, plaintext otherwise.
bool writeAllBytes(net::TcpSocket& sock, SSL* ssl, const std::string& data) {
    size_t total = 0;
    while (total < data.size()) {
        int64_t n = 0;
        if (ssl) {
            n = SSL_write(ssl, data.data() + total, static_cast<int>(data.size() - total));
        } else {
            n = sock.write(data.data() + total, data.size() - total);
        }
        if (n <= 0) return false;
        total += static_cast<size_t>(n);
    }
    return true;
}

// Read until the peer closes the connection (the client sends
// "Connection: close", so the server closes after the response).
std::string readUntilClose(net::TcpSocket& sock, SSL* ssl) {
    std::string out;
    while (true) {
        std::string chunk;
        if (ssl) {
            std::string buf;
            buf.resize(8192);
            int n = SSL_read(ssl, buf.data(), 8192);
            if (n <= 0) break;
            buf.resize(static_cast<size_t>(n));
            chunk = std::move(buf);
        } else {
            chunk = sock.readString(8192);
            if (chunk.empty()) break;
        }
        out += chunk;
    }
    return out;
}

struct ParsedUrl {
    std::string host{"127.0.0.1"};
    int port{80};
    std::string path{"/"};
    bool useTls{false};
};

// Shared by HttpClient::get/post. https:// enables TLS and defaults to
// port 443; http:// (or no scheme) stays plaintext on port 80.
ParsedUrl parseClientUrl(const std::string& url) {
    ParsedUrl out;
    std::string clean = url;
    if (clean.rfind("https://", 0) == 0) {
        out.useTls = true;
        out.port = 443;
        clean = clean.substr(8);
    } else if (clean.rfind("http://", 0) == 0) {
        clean = clean.substr(7);
    }

    size_t slash = clean.find('/');
    std::string hostPort = (slash == std::string::npos) ? clean : clean.substr(0, slash);
    if (slash != std::string::npos) out.path = clean.substr(slash);

    size_t colon = hostPort.find(':');
    if (colon != std::string::npos) {
        out.host = hostPort.substr(0, colon);
        out.port = std::stoi(hostPort.substr(colon + 1));
    } else {
        out.host = hostPort;
    }
    return out;
}

// Transport abstraction so HttpServer::handleClient works identically over
// plaintext TCP and TLS. PlainConnection owns the socket; TlsConnection owns
// the socket plus the SSL object and shuts TLS down cleanly on close.
class Connection {
public:
    virtual ~Connection() = default;
    virtual std::string readString(size_t maxLength) = 0;
    virtual int64_t writeString(const std::string& str) = 0;
    virtual bool isValid() const = 0;
    virtual void close() = 0;
};

class PlainConnection final : public Connection {
public:
    explicit PlainConnection(std::unique_ptr<net::TcpSocket> sock) : m_sock(std::move(sock)) {}
    std::string readString(size_t maxLength) override {
        return m_sock ? m_sock->readString(maxLength) : std::string{};
    }
    int64_t writeString(const std::string& str) override {
        return m_sock ? m_sock->writeString(str) : -1;
    }
    bool isValid() const override { return m_sock && m_sock->isValid(); }
    void close() override { if (m_sock) m_sock->close(); }
private:
    std::unique_ptr<net::TcpSocket> m_sock;
};

class TlsConnection final : public Connection {
public:
    TlsConnection(std::unique_ptr<net::TcpSocket> sock, SSL* ssl)
        : m_sock(std::move(sock)), m_ssl(ssl) {}
    ~TlsConnection() override { close(); }
    std::string readString(size_t maxLength) override {
        if (!m_sock || !m_ssl) return {};
        std::string buf;
        buf.resize(maxLength);
        int n = SSL_read(m_ssl, buf.data(), static_cast<int>(maxLength));
        if (n <= 0) return {};
        buf.resize(static_cast<size_t>(n));
        return buf;
    }
    int64_t writeString(const std::string& str) override {
        if (!m_sock || !m_ssl || str.empty()) return 0;
        size_t total = 0;
        while (total < str.size()) {
            int n = SSL_write(m_ssl, str.data() + total, static_cast<int>(str.size() - total));
            if (n <= 0) return total == 0 ? -1 : static_cast<int64_t>(total);
            total += static_cast<size_t>(n);
        }
        return static_cast<int64_t>(total);
    }
    bool isValid() const override { return m_sock && m_sock->isValid() && m_ssl != nullptr; }
    void close() override {
        if (m_ssl) {
            SSL_shutdown(m_ssl);
            SSL_free(m_ssl);
            m_ssl = nullptr;
        }
        if (m_sock) m_sock->close();
    }
private:
    std::unique_ptr<net::TcpSocket> m_sock;
    SSL* m_ssl{nullptr};
};

// Adapts the HTTP server's (plaintext or TLS) Connection to the
// WebSocket frame loop's transport interface, so upgraded connections
// keep working over TLS (wss://) without WebSocket.cpp knowing about SSL.
class WsConnectionAdapter final : public ws::WsTransport {
public:
    explicit WsConnectionAdapter(Connection& conn) : m_conn(conn) {}
    std::string readSome(size_t maxLength) override { return m_conn.readString(maxLength); }
    int64_t writeString(const std::string& str) override { return m_conn.writeString(str); }
    bool isValid() const override { return m_conn.isValid(); }
private:
    Connection& m_conn;
};

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
    HttpClientOptions opts;
    opts.timeoutMs = timeoutMs;
    return send(req, host, port, opts);
}

HttpResponse HttpClient::send(const HttpRequest& req, const std::string& host, int port, const HttpClientOptions& options) {
    net::TcpSocket sock;
    if (!sock.connect(host, port, options.timeoutMs)) {
        return HttpResponse::internalError("Failed to connect to " + host + ":" + std::to_string(port));
    }

    // --- TLS setup (only when requested) -----------------------------------
    SSL_CTX* ctx = nullptr;
    SSL* ssl = nullptr;
    if (options.useTls) {
        OPENSSL_init_ssl(0, nullptr);
        ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) {
            sock.close();
            return HttpResponse::internalError("TLS: failed to create SSL context");
        }
        SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);

        if (options.insecureSkipVerify) {
            // Explicit opt-in only: no chain check, no hostname check.
            SSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, nullptr);
        } else {
            // Verify the chain against the system CA store by default, plus
            // any extra CA bundle/path the caller supplied (e.g. a test CA).
            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
            SSL_CTX_set_default_verify_paths(ctx);
            if (!options.caFile.empty() || !options.caPath.empty()) {
                if (SSL_CTX_load_verify_locations(ctx,
                        options.caFile.empty() ? nullptr : options.caFile.c_str(),
                        options.caPath.empty() ? nullptr : options.caPath.c_str()) != 1) {
                    std::string err = opensslErrorString();
                    SSL_CTX_free(ctx);
                    sock.close();
                    return HttpResponse::internalError("TLS: failed to load CA file/path: " + err);
                }
            }
        }

        ssl = SSL_new(ctx);
        if (!ssl) {
            std::string err = opensslErrorString();
            SSL_CTX_free(ctx);
            sock.close();
            return HttpResponse::internalError("TLS: failed to create SSL object: " + err);
        }
        if (SSL_set_fd(ssl, static_cast<int>(sock.getHandle())) != 1) {
            std::string err = opensslErrorString();
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            sock.close();
            return HttpResponse::internalError("TLS: failed to attach socket: " + err);
        }
        // SNI so virtual-hosted servers present the right certificate.
        SSL_set_tlsext_host_name(ssl, host.c_str());
        if (!options.insecureSkipVerify) {
            // Hostname verification: OpenSSL checks the cert's SAN (then CN)
            // against this name during SSL_connect.
            if (SSL_set1_host(ssl, host.c_str()) != 1) {
                std::string err = opensslErrorString();
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                sock.close();
                return HttpResponse::internalError("TLS: failed to set verification hostname: " + err);
            }
            SSL_set_hostflags(ssl, 0);
        }

        // Bound the handshake so a silent peer cannot stall the client.
        sock.setTimeouts(options.timeoutMs, options.timeoutMs);
        if (SSL_connect(ssl) <= 0) {
            std::string err = opensslErrorString();
            SSL_free(ssl);
            SSL_CTX_free(ctx);
            sock.close();
            return HttpResponse::internalError("TLS handshake failed with " + host + ": " + err);
        }
        if (!options.insecureSkipVerify) {
            long verifyResult = SSL_get_verify_result(ssl);
            if (verifyResult != X509_V_OK) {
                std::string err = X509_verify_cert_error_string(verifyResult);
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                sock.close();
                return HttpResponse::internalError("TLS certificate verification failed for " + host + ": " + err);
            }
        }
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

    if (!writeAllBytes(sock, ssl, reqStream.str())) {
        if (ssl) { SSL_free(ssl); SSL_CTX_free(ctx); }
        sock.close();
        return HttpResponse::internalError("Failed to send request to " + host + ":" + std::to_string(port));
    }

    std::string fullResp = readUntilClose(sock, ssl);
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
        SSL_CTX_free(ctx);
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
    HttpClientOptions opts;
    opts.timeoutMs = timeoutMs;
    return get(url, opts);
}

HttpResponse HttpClient::get(const std::string& url, const HttpClientOptions& options) {
    ParsedUrl p = parseClientUrl(url);
    HttpClientOptions opts = options;
    opts.useTls = p.useTls;  // URL scheme decides; https:// means TLS.
    HttpRequest req;
    req.method = "GET";
    req.path = p.path;
    req.setHeader("User-Agent", "SP-Http-Client/2.2");
    return send(req, p.host, p.port, opts);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                              const std::string& contentType, int timeoutMs) {
    HttpClientOptions opts;
    opts.timeoutMs = timeoutMs;
    return post(url, body, contentType, opts);
}

HttpResponse HttpClient::post(const std::string& url, const std::string& body,
                              const std::string& contentType, const HttpClientOptions& options) {
    ParsedUrl p = parseClientUrl(url);
    HttpClientOptions opts = options;
    opts.useTls = p.useTls;  // URL scheme decides; https:// means TLS.
    HttpRequest req;
    req.method = "POST";
    req.path = p.path;
    req.body = body;
    req.setHeader("Content-Type", contentType);
    req.setHeader("User-Agent", "SP-Http-Client/2.2");
    return send(req, p.host, p.port, opts);
}

// ----------------------------------------------------------------------------
// ----------------------------------------------------------------------------
// Radix-Tree Router & Route Groups (SP 2.3)
// ----------------------------------------------------------------------------

std::vector<std::string_view> RadixRouter::splitPath(std::string_view path) {
    std::vector<std::string_view> segments;
    segments.reserve(8);
    size_t start = 0;
    while (start < path.size()) {
        while (start < path.size() && path[start] == '/') {
            start++;
        }
        if (start >= path.size()) break;
        size_t end = path.find('/', start);
        if (end == std::string_view::npos) {
            segments.push_back(path.substr(start));
            break;
        }
        segments.push_back(path.substr(start, end - start));
        start = end + 1;
    }
    return segments;
}

RadixRouter::Node* RadixRouter::Node::findChild(NodeType t, std::string_view seg) const {
    if (t == NodeType::Exact) {
        auto it = exactChildren.find(seg);
        if (it != exactChildren.end()) return it->second;
        return nullptr;
    } else if (t == NodeType::Param) {
        return paramChild;
    } else if (t == NodeType::Wildcard) {
        return wildcardChild;
    }
    return nullptr;
}

RadixRouter::RadixRouter() {
    m_root = std::make_unique<Node>();
}

void RadixRouter::addRoute(const std::string& method, const std::string& pattern, 
                           HttpHandler handler, std::vector<HttpMiddleware> middlewares) {
    if (!m_root) {
        m_root = std::make_unique<Node>();
    }
    std::string cleanPattern = pattern;
    size_t qPos = cleanPattern.find('?');
    if (qPos != std::string::npos) {
        cleanPattern = cleanPattern.substr(0, qPos);
    }

    auto segments = splitPath(cleanPattern);
    Node* curr = m_root.get();

    for (const auto& seg : segments) {
        NodeType t = NodeType::Exact;
        std::string pName;
        std::string sVal(seg);

        if (!seg.empty() && seg[0] == '*') {
            t = NodeType::Wildcard;
            pName = std::string(seg.substr(1));
        } else if (!seg.empty() && seg[0] == ':') {
            t = NodeType::Param;
            pName = std::string(seg.substr(1));
        }

        Node* child = curr->findChild(t, sVal);
        if (!child) {
            auto newNode = std::make_unique<Node>();
            newNode->type = t;
            newNode->segment = sVal;
            newNode->paramName = pName;
            child = newNode.get();
            if (t == NodeType::Exact) {
                curr->exactChildren[sVal] = child;
            } else if (t == NodeType::Param) {
                if (!curr->paramChild) curr->paramChild = child;
            } else if (t == NodeType::Wildcard) {
                if (!curr->wildcardChild) curr->wildcardChild = child;
            }
            curr->children.push_back(std::move(newNode));
        }
        curr = child;

        if (t == NodeType::Wildcard) {
            break; // Wildcard absorbs remainder
        }
    }

    std::string upperMethod = method;
    std::transform(upperMethod.begin(), upperMethod.end(), upperMethod.begin(), ::toupper);

    curr->handlers[upperMethod] = std::move(handler);
    if (!middlewares.empty()) {
        curr->middlewares.insert(curr->middlewares.end(), middlewares.begin(), middlewares.end());
    }
    m_routeCount++;
}

RouteMatchResult RadixRouter::match(const std::string& method, const std::string& path) const {
    RouteMatchResult result;
    if (!m_root) return result;

    std::string upperMethod;
    bool needsUpper = false;
    for (char c : method) {
        if (std::islower(static_cast<unsigned char>(c))) {
            needsUpper = true;
            break;
        }
    }
    if (needsUpper) {
        upperMethod = method;
        for (char& c : upperMethod) {
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
    }
    const std::string& queryMethod = needsUpper ? upperMethod : method;

    auto segments = splitPath(path);
    std::vector<HttpMiddleware> accumulatedMws;
    accumulatedMws.reserve(8);
    std::vector<std::pair<std::string_view, std::string>> accumulatedParams;
    accumulatedParams.reserve(8);

    matchRecursive(m_root.get(), segments, 0, queryMethod, accumulatedMws, accumulatedParams, result);
    return result;
}

void RadixRouter::matchRecursive(const Node* node, const std::vector<std::string_view>& segments, size_t segIdx,
                                 const std::string& method,
                                 std::vector<HttpMiddleware>& accumulatedMiddlewares,
                                 std::vector<std::pair<std::string_view, std::string>>& accumulatedParams,
                                 RouteMatchResult& bestResult) const {
    if (!node) return;

    size_t prevMwCount = accumulatedMiddlewares.size();
    if (!node->middlewares.empty()) {
        accumulatedMiddlewares.insert(accumulatedMiddlewares.end(), node->middlewares.begin(), node->middlewares.end());
    }

    if (segIdx == segments.size()) {
        if (!node->handlers.empty()) {
            auto it = node->handlers.find(method);
            if (it != node->handlers.end()) {
                bestResult.found = true;
                bestResult.handler = it->second;
                bestResult.middlewares = accumulatedMiddlewares;
                for (const auto& [k, v] : accumulatedParams) {
                    bestResult.pathParams[std::string(k)] = v;
                }
                return;
            } else {
                bestResult.methodNotAllowed = true;
                for (const auto& [m, _] : node->handlers) {
                    if (std::find(bestResult.allowedMethods.begin(), bestResult.allowedMethods.end(), m) == bestResult.allowedMethods.end()) {
                        bestResult.allowedMethods.push_back(m);
                    }
                }
                bestResult.middlewares = accumulatedMiddlewares;
                for (const auto& [k, v] : accumulatedParams) {
                    bestResult.pathParams[std::string(k)] = v;
                }
            }
        }
        if (node->wildcardChild) {
            accumulatedParams.emplace_back(node->wildcardChild->paramName, "");
            auto it = node->wildcardChild->handlers.find(method);
            if (it != node->wildcardChild->handlers.end()) {
                bestResult.found = true;
                bestResult.handler = it->second;
                bestResult.middlewares = accumulatedMiddlewares;
                if (!node->wildcardChild->middlewares.empty()) {
                    bestResult.middlewares.insert(bestResult.middlewares.end(), 
                                                  node->wildcardChild->middlewares.begin(), 
                                                  node->wildcardChild->middlewares.end());
                }
                for (const auto& [k, v] : accumulatedParams) {
                    bestResult.pathParams[std::string(k)] = v;
                }
                return;
            } else if (!node->wildcardChild->handlers.empty()) {
                bestResult.methodNotAllowed = true;
                for (const auto& [m, _] : node->wildcardChild->handlers) {
                    if (std::find(bestResult.allowedMethods.begin(), bestResult.allowedMethods.end(), m) == bestResult.allowedMethods.end()) {
                        bestResult.allowedMethods.push_back(m);
                    }
                }
                bestResult.middlewares = accumulatedMiddlewares;
                for (const auto& [k, v] : accumulatedParams) {
                    bestResult.pathParams[std::string(k)] = v;
                }
            }
            accumulatedParams.pop_back();
        }
        if (accumulatedMiddlewares.size() > prevMwCount) {
            accumulatedMiddlewares.resize(prevMwCount);
        }
        return;
    }

    std::string_view seg = segments[segIdx];

    // 1. Try Exact match child (O(1) lookup)
    auto itExact = node->exactChildren.find(seg);
    if (itExact != node->exactChildren.end()) {
        matchRecursive(itExact->second, segments, segIdx + 1, method, accumulatedMiddlewares, accumulatedParams, bestResult);
        if (bestResult.found) return;
    }

    // 2. Try Param match child
    if (node->paramChild) {
        accumulatedParams.emplace_back(node->paramChild->paramName, std::string(seg));
        matchRecursive(node->paramChild, segments, segIdx + 1, method, accumulatedMiddlewares, accumulatedParams, bestResult);
        if (bestResult.found) return;
        accumulatedParams.pop_back();
    }

    // 3. Try Wildcard match child
    if (node->wildcardChild) {
        std::string wildcardTail;
        for (size_t i = segIdx; i < segments.size(); ++i) {
            if (i > segIdx) wildcardTail += "/";
            wildcardTail += segments[i];
        }
        accumulatedParams.emplace_back(node->wildcardChild->paramName, std::move(wildcardTail));
        auto it = node->wildcardChild->handlers.find(method);
        if (it != node->wildcardChild->handlers.end()) {
            bestResult.found = true;
            bestResult.handler = it->second;
            bestResult.middlewares = accumulatedMiddlewares;
            if (!node->wildcardChild->middlewares.empty()) {
                bestResult.middlewares.insert(bestResult.middlewares.end(), 
                                              node->wildcardChild->middlewares.begin(), 
                                              node->wildcardChild->middlewares.end());
            }
            for (const auto& [k, v] : accumulatedParams) {
                bestResult.pathParams[std::string(k)] = v;
            }
            return;
        } else if (!node->wildcardChild->handlers.empty()) {
            bestResult.methodNotAllowed = true;
            for (const auto& [m, _] : node->wildcardChild->handlers) {
                if (std::find(bestResult.allowedMethods.begin(), bestResult.allowedMethods.end(), m) == bestResult.allowedMethods.end()) {
                    bestResult.allowedMethods.push_back(m);
                }
            }
            bestResult.middlewares = accumulatedMiddlewares;
            for (const auto& [k, v] : accumulatedParams) {
                bestResult.pathParams[std::string(k)] = v;
            }
        }
        accumulatedParams.pop_back();
    }

    if (accumulatedMiddlewares.size() > prevMwCount) {
        accumulatedMiddlewares.resize(prevMwCount);
    }
}

// ----------------------------------------------------------------------------
// RouteGroup Implementation
// ----------------------------------------------------------------------------

RouteGroup::RouteGroup(std::string prefix, RadixRouter* router, std::vector<HttpMiddleware> middlewares)
    : m_prefix(std::move(prefix)), m_router(router), m_middlewares(std::move(middlewares)) {}

std::string RouteGroup::fullPath(const std::string& pattern) const {
    if (m_prefix.empty()) return pattern;
    std::string cleanPre = m_prefix;
    if (cleanPre.back() == '/') cleanPre.pop_back();
    std::string cleanPat = pattern;
    if (!cleanPat.empty() && cleanPat.front() == '/') cleanPat = cleanPat.substr(1);
    return cleanPre + "/" + cleanPat;
}

RouteGroup RouteGroup::group(const std::string& subPrefix) {
    return RouteGroup(fullPath(subPrefix), m_router, m_middlewares);
}

void RouteGroup::use(HttpMiddleware middleware) {
    m_middlewares.push_back(std::move(middleware));
}

void RouteGroup::get(const std::string& pattern, HttpHandler handler) {
    route("GET", pattern, std::move(handler));
}

void RouteGroup::post(const std::string& pattern, HttpHandler handler) {
    route("POST", pattern, std::move(handler));
}

void RouteGroup::put(const std::string& pattern, HttpHandler handler) {
    route("PUT", pattern, std::move(handler));
}

void RouteGroup::patch(const std::string& pattern, HttpHandler handler) {
    route("PATCH", pattern, std::move(handler));
}

void RouteGroup::deleteRoute(const std::string& pattern, HttpHandler handler) {
    route("DELETE", pattern, std::move(handler));
}

void RouteGroup::options(const std::string& pattern, HttpHandler handler) {
    route("OPTIONS", pattern, std::move(handler));
}

void RouteGroup::head(const std::string& pattern, HttpHandler handler) {
    route("HEAD", pattern, std::move(handler));
}

void RouteGroup::route(const std::string& method, const std::string& pattern, HttpHandler handler) {
    if (m_router) {
        m_router->addRoute(method, fullPath(pattern), std::move(handler), m_middlewares);
    }
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

void HttpServer::options(const std::string& pattern, HttpHandler handler) {
    route("OPTIONS", pattern, std::move(handler));
}

void HttpServer::head(const std::string& pattern, HttpHandler handler) {
    route("HEAD", pattern, std::move(handler));
}

void HttpServer::route(const std::string& method, const std::string& pattern, HttpHandler handler) {
    m_router.addRoute(method, pattern, std::move(handler));
}

RouteGroup HttpServer::group(const std::string& prefix) {
    return RouteGroup(prefix, &m_router);
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
    m_lastError.clear();
    if (m_config.workerThreads <= 0) {
        m_config.workerThreads = std::max(2u, std::thread::hardware_concurrency());
    }

    // --- TLS setup: fail loudly, never fall back to plaintext ---------------
    SSL_CTX* tlsCtx = nullptr;
    if (m_config.tlsEnabled) {
        if (m_config.tlsCertFile.empty() || m_config.tlsKeyFile.empty()) {
            m_lastError = "TLS enabled but tlsCertFile/tlsKeyFile is not set";
            std::cerr << "HttpServer: " << m_lastError << "\n";
            return false;
        }
        OPENSSL_init_ssl(0, nullptr);
        tlsCtx = SSL_CTX_new(TLS_server_method());
        if (!tlsCtx) {
            m_lastError = "TLS: failed to create SSL context: " + opensslErrorString();
            std::cerr << "HttpServer: " << m_lastError << "\n";
            return false;
        }
        SSL_CTX_set_min_proto_version(tlsCtx, TLS1_2_VERSION);
        if (SSL_CTX_use_certificate_chain_file(tlsCtx, m_config.tlsCertFile.c_str()) != 1) {
            m_lastError = "TLS: failed to load certificate file '" + m_config.tlsCertFile
                + "': " + opensslErrorString();
            SSL_CTX_free(tlsCtx);
            std::cerr << "HttpServer: " << m_lastError << "\n";
            return false;
        }
        if (SSL_CTX_use_PrivateKey_file(tlsCtx, m_config.tlsKeyFile.c_str(), SSL_FILETYPE_PEM) != 1) {
            m_lastError = "TLS: failed to load private key file '" + m_config.tlsKeyFile
                + "': " + opensslErrorString();
            SSL_CTX_free(tlsCtx);
            std::cerr << "HttpServer: " << m_lastError << "\n";
            return false;
        }
        if (SSL_CTX_check_private_key(tlsCtx) != 1) {
            m_lastError = "TLS: private key '" + m_config.tlsKeyFile
                + "' does not match certificate '" + m_config.tlsCertFile
                + "': " + opensslErrorString();
            SSL_CTX_free(tlsCtx);
            std::cerr << "HttpServer: " << m_lastError << "\n";
            return false;
        }
    }

    if (!m_listener.bindAndListen(host, port, 1024)) {
        if (tlsCtx) SSL_CTX_free(tlsCtx);
        return false;
    }
    m_tlsCtx = tlsCtx;
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

void HttpServer::wait() {
    if (m_acceptThread && m_acceptThread->joinable()) {
        m_acceptThread->join();
    }
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

    // Workers are joined by now, so no connection can still use the context.
    if (m_tlsCtx) {
        SSL_CTX_free(static_cast<SSL_CTX*>(m_tlsCtx));
        m_tlsCtx = nullptr;
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
    // TLS handshake first when enabled; a failed handshake closes the
    // connection without serving anything (no plaintext fallback).
    std::unique_ptr<Connection> conn;
    if (m_tlsCtx) {
        SSL* ssl = SSL_new(static_cast<SSL_CTX*>(m_tlsCtx));
        if (!ssl) {
            std::cerr << "HttpServer: TLS: failed to create SSL object: " << opensslErrorString() << "\n";
            client->close();
            return;
        }
        if (SSL_set_fd(ssl, static_cast<int>(client->getHandle())) != 1) {
            std::cerr << "HttpServer: TLS: failed to attach socket: " << opensslErrorString() << "\n";
            SSL_free(ssl);
            client->close();
            return;
        }
        // Bound the handshake (and later reads) so a client that never
        // speaks TLS cannot stall a worker thread forever.
        client->setTimeouts(m_config.requestTimeoutMs, m_config.requestTimeoutMs);
        if (SSL_accept(ssl) <= 0) {
            std::cerr << "HttpServer: TLS handshake failed: " << opensslErrorString() << "\n";
            SSL_free(ssl);
            client->close();
            return;
        }
        conn = std::make_unique<TlsConnection>(std::move(client), ssl);
    } else {
        conn = std::make_unique<PlainConnection>(std::move(client));
    }

    std::string buffer;
    int requestCount = 0;

    // Persistent HTTP/1.1 keep-alive loop
    while (m_running.load() && conn && conn->isValid()) {
        std::string chunk = conn->readString(16384);
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

            // RFC 6455 upgrade: if a WebSocket route is registered for this
            // path, the connection leaves HTTP and the frame loop owns the
            // socket until the peer closes. No route -> normal HTTP handling.
            if (equalsIgnoreCase(req.getHeader("Upgrade"), "websocket")) {
                std::string connHdr = req.getHeader("Connection");
                std::string key = req.getHeader("Sec-WebSocket-Key");
                bool wantsUpgrade = false;
                {
                    std::string lower = connHdr;
                    std::transform(lower.begin(), lower.end(), lower.begin(),
                                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
                    wantsUpgrade = lower.find("upgrade") != std::string::npos;
                }
                if (wantsUpgrade && !key.empty() &&
                    ws::WebSocketServer::getGlobal().hasRoute(req.path)) {
                    m_activeRequests--;
                    WsConnectionAdapter transport(*conn);
                    ws::WebSocketServer::getGlobal().serveUpgraded(transport, req.path, key, buffer);
                    break;
                }
            }

            HttpResponse res;

            // Enforce body limit
            if (req.body.size() > m_config.maxBodySize) {
                res = HttpResponse::payloadTooLarge();
            } else {
                // Apply global middlewares
                HttpResponse mwRes;
                bool proceed = true;
                for (const auto& mw : m_middlewares) {
                    if (!mw(req, mwRes)) {
                        proceed = false;
                        res = std::move(mwRes);
                        break;
                    }
                }

                if (proceed) {
                    res = dispatch(req);
                    for (const auto& h : mwRes.headers) {
                        if (res.getHeader(h.name).empty()) {
                            res.setHeader(h.name, h.value);
                        }
                    }
                }
            }

            bool keepAlive = req.isKeepAlive() && 
                             (requestCount < m_config.maxKeepAliveRequests) && 
                             m_running.load();

            std::string rawRes = res.serialize(keepAlive);
            conn->writeString(rawRes);
            m_activeRequests--;

            if (!keepAlive) {
                break;
            }
        } else {
            // Check for oversized header DOS attack
            if (buffer.size() > m_config.maxHeaderSize && buffer.find("\r\n\r\n") == std::string::npos) {
                HttpResponse res = HttpResponse::status(431, "Request Header Fields Too Large");
                conn->writeString(res.serialize(false));
                break;
            }
            if (chunk.empty()) {
                // Incomplete request and client closed connection
                break;
            }
        }
    }

    conn->close();
}

HttpResponse HttpServer::handle(const HttpRequest& req) {
    HttpResponse res;
    HttpResponse mwRes;
    bool proceed = true;
    for (const auto& mw : m_middlewares) {
        if (!mw(req, mwRes)) {
            proceed = false;
            res = std::move(mwRes);
            break;
        }
    }
    if (proceed) {
        res = dispatch(req);
        for (const auto& h : mwRes.headers) {
            if (res.getHeader(h.name).empty()) {
                res.setHeader(h.name, h.value);
            }
        }
    }
    return res;
}

HttpResponse HttpServer::dispatch(const HttpRequest& req) {
    std::string upperMethod = req.method;
    std::transform(upperMethod.begin(), upperMethod.end(), upperMethod.begin(), ::toupper);

    auto match = m_router.match(upperMethod, req.path);

    if (match.found) {
        HttpRequest matchedReq = req;
        matchedReq.pathParams = std::move(match.pathParams);

        // Run group-level middlewares
        HttpResponse groupMwRes;
        for (const auto& mw : match.middlewares) {
            if (!mw(matchedReq, groupMwRes)) {
                return groupMwRes; // short-circuited by group middleware
            }
        }

        HttpResponse finalRes;
        if (upperMethod == "HEAD") {
            finalRes = match.handler(matchedReq);
            finalRes.body.clear();
        } else {
            finalRes = match.handler(matchedReq);
        }

        // Merge group middleware headers
        for (const auto& h : groupMwRes.headers) {
            if (finalRes.getHeader(h.name).empty()) {
                finalRes.setHeader(h.name, h.value);
            }
        }
        return finalRes;
    }

    if (match.methodNotAllowed) {
        if (upperMethod == "OPTIONS") {
            // Automatic OPTIONS preflight response with Allow header
            auto res = HttpResponse::noContent();
            std::string allowHeader;
            for (size_t i = 0; i < match.allowedMethods.size(); ++i) {
                if (i > 0) allowHeader += ", ";
                allowHeader += match.allowedMethods[i];
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
            return res;
        }

        if (upperMethod == "HEAD") {
            // Check if GET is allowed
            auto it = std::find(match.allowedMethods.begin(), match.allowedMethods.end(), "GET");
            if (it != match.allowedMethods.end()) {
                auto getMatch = m_router.match("GET", req.path);
                if (getMatch.found) {
                    HttpRequest matchedReq = req;
                    matchedReq.pathParams = std::move(getMatch.pathParams);
                    auto getRes = getMatch.handler(matchedReq);
                    getRes.body.clear();
                    return getRes;
                }
            }
        }

        return HttpResponse::methodNotAllowed(match.allowedMethods);
    }

    // Path not found in router
    if (upperMethod == "OPTIONS") {
        return HttpResponse::noContent();
    }

    return HttpResponse::notFound("No route matched " + req.method + " " + req.path);
}

// Built-in Middleware factories
HttpMiddleware HttpServer::cors(CorsOptions opts) {
    return [opts](const HttpRequest& req, HttpResponse& res) -> bool {
        res.setHeader("Access-Control-Allow-Origin", opts.allowedOrigins);
        res.setHeader("Access-Control-Allow-Methods", opts.allowedMethods);
        res.setHeader("Access-Control-Allow-Headers", opts.allowedHeaders);
        res.setHeader("Access-Control-Expose-Headers", opts.exposedHeaders);
        if (opts.allowCredentials) {
            res.setHeader("Access-Control-Allow-Credentials", "true");
        }
        if (opts.maxAge > 0) {
            res.setHeader("Access-Control-Max-Age", std::to_string(opts.maxAge));
        }
        if (equalsIgnoreCase(req.method, "OPTIONS")) {
            res.statusCode = 204;
            res.statusText = "No Content";
            res.body.clear();
            return false; // Intercept preflight OPTIONS
        }
        return true;
    };
}

HttpMiddleware HttpServer::logger() {
    return [](const HttpRequest& /*req*/, HttpResponse& /*res*/) -> bool {
        return true;
    };
}

HttpMiddleware HttpServer::recovery() {
    return [](const HttpRequest& /*req*/, HttpResponse& /*res*/) -> bool {
        return true;
    };
}

HttpMiddleware HttpServer::requestId() {
    return [](const HttpRequest& req, HttpResponse& res) -> bool {
        std::string id = req.getHeader("x-request-id");
        if (id.empty()) {
            id = crypto::randomUuid();
        }
        res.setHeader("X-Request-ID", id);
        return true;
    };
}

HttpMiddleware HttpServer::rateLimiter(RateLimiterOptions opts) {
    struct RateLimitState {
        std::mutex mtx;
        std::unordered_map<std::string, std::vector<std::chrono::steady_clock::time_point>> clients;
    };
    auto state = std::make_shared<RateLimitState>();

    return [opts, state](const HttpRequest& req, HttpResponse& res) -> bool {
        std::string key = req.getHeader("x-forwarded-for");
        if (key.empty()) key = req.getHeader("authorization");
        if (key.empty()) key = "default-client";

        auto now = std::chrono::steady_clock::now();
        auto cutoff = now - std::chrono::seconds(opts.windowSeconds);

        std::lock_guard<std::mutex> lock(state->mtx);
        auto& timestamps = state->clients[key];
        std::vector<std::chrono::steady_clock::time_point> valid;
        for (const auto& tp : timestamps) {
            if (tp >= cutoff) {
                valid.push_back(tp);
            }
        }
        timestamps = std::move(valid);

        if (timestamps.size() >= static_cast<size_t>(opts.maxRequests)) {
            res = HttpResponse::tooManyRequests("Rate limit exceeded. Try again later.", opts.windowSeconds);
            return false;
        }

        timestamps.push_back(now);
        return true;
    };
}

} // namespace sp::rt::http
