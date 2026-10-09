#include "sp/runtime/TestHttp.h"
#include "sp/runtime/Runtime.h"
#include <cstring>
#include <sstream>
#include <algorithm>

namespace sp::rt::net {

namespace {

char* alloc_c_str(const std::string& str) {
    char* copy = static_cast<char*>(malloc(str.size() + 1));
    if (copy) {
        std::memcpy(copy, str.c_str(), str.size() + 1);
    }
    return copy;
}

std::map<std::string, std::string> parseJsonHeaders(const std::string& json) {
    std::map<std::string, std::string> headers;
    if (json.empty() || json[0] != '{') return headers;

    size_t i = 1;
    while (i < json.size()) {
        while (i < json.size() && (json[i] == ' ' || json[i] == ',' || json[i] == '\t' || json[i] == '\n' || json[i] == '\r')) i++;
        if (i >= json.size() || json[i] == '}') break;

        if (json[i] == '"') {
            i++;
            size_t kStart = i;
            while (i < json.size() && json[i] != '"') i++;
            std::string key = json.substr(kStart, i - kStart);
            if (i < json.size()) i++; // skip closing quote

            while (i < json.size() && (json[i] == ' ' || json[i] == ':')) i++;
            if (i < json.size() && json[i] == '"') {
                i++;
                size_t vStart = i;
                while (i < json.size() && json[i] != '"') i++;
                std::string val = json.substr(vStart, i - vStart);
                if (i < json.size()) i++;
                headers[key] = val;
            }
        } else {
            i++;
        }
    }
    return headers;
}

} // namespace

TestHttpClient::TestHttpClient(sp::rt::http::HttpServer* server)
    : m_server(server) {}

std::unique_ptr<TestResponseHandle> TestHttpClient::request(
    const std::string& method,
    const std::string& fullPath,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {

    auto resp = std::make_unique<TestResponseHandle>();
    if (!m_server) {
        resp->statusCode = 500;
        resp->statusText = "Internal Server Error";
        resp->body = "TestHttpClient: null server instance";
        return resp;
    }

    sp::rt::http::HttpRequest req;
    req.method = method;

    // Split path and query string
    size_t qPos = fullPath.find('?');
    if (qPos != std::string::npos) {
        req.path = fullPath.substr(0, qPos);
        req.queryString = fullPath.substr(qPos + 1);

        // Parse query params
        std::stringstream ss(req.queryString);
        std::string pair;
        while (std::getline(ss, pair, '&')) {
            size_t eqPos = pair.find('=');
            if (eqPos != std::string::npos) {
                req.queryParams[pair.substr(0, eqPos)] = pair.substr(eqPos + 1);
            } else if (!pair.empty()) {
                req.queryParams[pair] = "";
            }
        }
    } else {
        req.path = fullPath;
    }

    req.body = body;
    for (const auto& [k, v] : headers) {
        req.setHeader(k, v);
    }
    if (!body.empty() && req.getHeader("Content-Length").empty()) {
        req.setHeader("Content-Length", std::to_string(body.size()));
    }

    // Execute in-memory through full middleware pipeline + routing
    sp::rt::http::HttpResponse serverRes = m_server->handle(req);

    resp->statusCode = serverRes.statusCode;
    resp->statusText = serverRes.statusText;
    resp->body = serverRes.body;
    for (const auto& h : serverRes.headers) {
        resp->headers[h.name] = h.value;
    }

    return resp;
}

} // namespace sp::rt::net

extern "C" {

void* sp_rt_test_client_create(void* srv_handle) {
    if (!srv_handle) return nullptr;
    auto* srv = static_cast<sp::rt::http::HttpServer*>(srv_handle);
    return new sp::rt::net::TestHttpClient(srv);
}

void sp_rt_test_client_destroy(void* client_handle) {
    if (client_handle) {
        delete static_cast<sp::rt::net::TestHttpClient*>(client_handle);
    }
}

void* sp_rt_test_client_request(void* client_handle, const char* method, const char* path,
                               const char* body, const char* headers_json) {
    if (!client_handle || !path) return nullptr;
    auto* client = static_cast<sp::rt::net::TestHttpClient*>(client_handle);
    std::string m = method ? method : "GET";
    std::string p = path;
    std::string b = body ? body : "";
    std::map<std::string, std::string> h;
    if (headers_json && *headers_json) {
        h = sp::rt::net::parseJsonHeaders(headers_json);
    }

    auto res = client->request(m, p, b, h);
    return res.release();
}

void* sp_rt_test_client_get(void* client_handle, const char* path) {
    return sp_rt_test_client_request(client_handle, "GET", path, nullptr, nullptr);
}

void* sp_rt_test_client_post(void* client_handle, const char* path, const char* body) {
    return sp_rt_test_client_request(client_handle, "POST", path, body, nullptr);
}

void* sp_rt_test_client_put(void* client_handle, const char* path, const char* body) {
    return sp_rt_test_client_request(client_handle, "PUT", path, body, nullptr);
}

void* sp_rt_test_client_delete(void* client_handle, const char* path) {
    return sp_rt_test_client_request(client_handle, "DELETE", path, nullptr, nullptr);
}

void* sp_rt_test_client_patch(void* client_handle, const char* path, const char* body) {
    return sp_rt_test_client_request(client_handle, "PATCH", path, body, nullptr);
}

int32_t sp_rt_test_response_status(void* resp_handle) {
    if (!resp_handle) return 0;
    return static_cast<sp::rt::net::TestResponseHandle*>(resp_handle)->statusCode;
}

const char* sp_rt_test_response_body(void* resp_handle) {
    if (!resp_handle) return sp::rt::net::alloc_c_str("");
    return sp::rt::net::alloc_c_str(static_cast<sp::rt::net::TestResponseHandle*>(resp_handle)->body);
}

const char* sp_rt_test_response_header(void* resp_handle, const char* name) {
    if (!resp_handle || !name) return sp::rt::net::alloc_c_str("");
    auto* r = static_cast<sp::rt::net::TestResponseHandle*>(resp_handle);
    auto it = r->headers.find(name);
    if (it != r->headers.end()) {
        return sp::rt::net::alloc_c_str(it->second);
    }
    // Case-insensitive fallback
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    for (const auto& [k, v] : r->headers) {
        std::string lk = k;
        std::transform(lk.begin(), lk.end(), lk.begin(), ::tolower);
        if (lk == lowerName) {
            return sp::rt::net::alloc_c_str(v);
        }
    }
    return sp::rt::net::alloc_c_str("");
}

int32_t sp_rt_test_response_has_header(void* resp_handle, const char* name) {
    if (!resp_handle || !name) return 0;
    auto* r = static_cast<sp::rt::net::TestResponseHandle*>(resp_handle);
    if (r->headers.count(name)) return 1;
    std::string lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(), ::tolower);
    for (const auto& [k, v] : r->headers) {
        std::string lk = k;
        std::transform(lk.begin(), lk.end(), lk.begin(), ::tolower);
        if (lk == lowerName) return 1;
    }
    return 0;
}

void sp_rt_test_response_destroy(void* resp_handle) {
    if (resp_handle) {
        delete static_cast<sp::rt::net::TestResponseHandle*>(resp_handle);
    }
}

} // extern "C"
