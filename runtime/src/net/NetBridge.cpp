#include "sp/runtime/Net.h"
#include "sp/runtime/EventLoop.h"
#include "sp/runtime/Crypto.h"
#include "sp/runtime/Json.h"
#include "sp/runtime/Http.h"
#include "sp/runtime/Runtime.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <iostream>

namespace {

const char* alloc_str(const std::string& s) {
    char* res = static_cast<char*>(std::malloc(s.size() + 1));
    if (res) {
        std::memcpy(res, s.c_str(), s.size() + 1);
    }
    return res;
}

struct JsonWrapper {
    sp::rt::json::JsonValuePtr val;
};

} // anonymous namespace

extern "C" {

// ============================================================================
// Network C-ABI
// ============================================================================

void sp_rt_net_init() {
    sp::rt::net::initNetwork();
}

void sp_rt_net_shutdown() {
    sp::rt::net::shutdownNetwork();
}

const char* sp_rt_net_resolve(const char* host) {
    if (!host) return alloc_str("");
    return alloc_str(sp::rt::net::resolveHost(host));
}

// TCP Client
void* sp_rt_tcp_connect(const char* host, int64_t port, int64_t timeout_ms) {
    if (!host) return nullptr;
    auto sock = std::make_unique<sp::rt::net::TcpSocket>();
    if (!sock->connect(host, static_cast<int>(port), static_cast<int>(timeout_ms))) {
        return nullptr;
    }
    return sock.release();
}

int64_t sp_rt_tcp_write(void* sock, const char* str) {
    if (!sock || !str) return -1;
    auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
    return s->writeString(str);
}

const char* sp_rt_tcp_read(void* sock, int64_t max_bytes) {
    if (!sock || max_bytes <= 0) return alloc_str("");
    auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
    return alloc_str(s->readString(static_cast<size_t>(max_bytes)));
}

const char* sp_rt_tcp_read_all(void* sock, int64_t timeout_ms) {
    if (!sock) return alloc_str("");
    auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
    return alloc_str(s->readAll(static_cast<int>(timeout_ms)));
}

bool sp_rt_tcp_set_nonblocking(void* sock, bool nb) {
    if (!sock) return false;
    auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
    return s->setNonBlocking(nb);
}

bool sp_rt_tcp_set_timeout(void* sock, int64_t ms) {
    if (!sock) return false;
    auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
    return s->setTimeouts(static_cast<int>(ms), static_cast<int>(ms));
}

void sp_rt_tcp_close(void* sock) {
    if (sock) {
        auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
        delete s;
    }
}

// TCP Listener
void* sp_rt_tcp_listen(const char* host, int64_t port, int64_t backlog) {
    auto listener = std::make_unique<sp::rt::net::TcpListener>();
    std::string h = host ? host : "0.0.0.0";
    if (!listener->bindAndListen(h, static_cast<int>(port), static_cast<int>(backlog))) {
        return nullptr;
    }
    return listener.release();
}

void* sp_rt_tcp_accept(void* listener) {
    if (!listener) return nullptr;
    auto* l = static_cast<sp::rt::net::TcpListener*>(listener);
    auto client = l->accept();
    return client ? client.release() : nullptr;
}

void* sp_rt_tcp_accept_timeout(void* listener, int64_t timeout_ms) {
    if (!listener) return nullptr;
    auto* l = static_cast<sp::rt::net::TcpListener*>(listener);
    auto client = l->acceptTimeout(static_cast<int>(timeout_ms));
    return client ? client.release() : nullptr;
}

int64_t sp_rt_tcp_listener_port(void* listener) {
    if (!listener) return 0;
    auto* l = static_cast<sp::rt::net::TcpListener*>(listener);
    return l->getPort();
}

void sp_rt_tcp_listener_close(void* listener) {
    if (listener) {
        auto* l = static_cast<sp::rt::net::TcpListener*>(listener);
        delete l;
    }
}

// UDP Socket
void* sp_rt_udp_bind(const char* host, int64_t port) {
    auto sock = std::make_unique<sp::rt::net::UdpSocket>();
    std::string h = host ? host : "0.0.0.0";
    if (!sock->bind(h, static_cast<int>(port))) {
        return nullptr;
    }
    return sock.release();
}

int64_t sp_rt_udp_send_to(void* sock, const char* data, const char* host, int64_t port) {
    if (!sock || !data || !host) return -1;
    auto* s = static_cast<sp::rt::net::UdpSocket*>(sock);
    return s->sendString(data, host, static_cast<int>(port));
}

const char* sp_rt_udp_recv_from(void* sock, int64_t max_len, const char** out_sender_host, int64_t* out_sender_port) {
    if (!sock || max_len <= 0) return alloc_str("");
    auto* s = static_cast<sp::rt::net::UdpSocket*>(sock);
    std::string sHost;
    int sPort = 0;
    std::string data = s->recvString(static_cast<size_t>(max_len), sHost, sPort);
    if (out_sender_host) {
        *out_sender_host = alloc_str(sHost);
    }
    if (out_sender_port) {
        *out_sender_port = sPort;
    }
    return alloc_str(data);
}

void sp_rt_udp_close(void* sock) {
    if (sock) {
        auto* s = static_cast<sp::rt::net::UdpSocket*>(sock);
        delete s;
    }
}

// EventLoop
void* sp_rt_eventloop_create() {
    return new sp::rt::net::EventLoop();
}

bool sp_rt_eventloop_add(void* loop, void* sock, int64_t events) {
    if (!loop || !sock) return false;
    auto* el = static_cast<sp::rt::net::EventLoop*>(loop);
    auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
    return el->addSocket(s->getHandle(), events, sock);
}

bool sp_rt_eventloop_remove(void* loop, void* sock) {
    if (!loop || !sock) return false;
    auto* el = static_cast<sp::rt::net::EventLoop*>(loop);
    auto* s = static_cast<sp::rt::net::TcpSocket*>(sock);
    return el->removeSocket(s->getHandle());
}

int64_t sp_rt_eventloop_poll(void* loop, int64_t timeout_ms) {
    if (!loop) return -1;
    auto* el = static_cast<sp::rt::net::EventLoop*>(loop);
    return el->poll(static_cast<int>(timeout_ms));
}

void sp_rt_eventloop_destroy(void* loop) {
    if (loop) {
        auto* el = static_cast<sp::rt::net::EventLoop*>(loop);
        delete el;
    }
}

// ============================================================================
// Crypto C-ABI
// ============================================================================

const char* sp_rt_crypto_sha256(const char* data) {
    if (!data) return alloc_str("");
    return alloc_str(sp::rt::crypto::sha256(data));
}

const char* sp_rt_crypto_hmac_sha256(const char* key, const char* data) {
    if (!key || !data) return alloc_str("");
    return alloc_str(sp::rt::crypto::hmacSha256(key, data));
}

const char* sp_rt_crypto_base64_encode(const char* data) {
    if (!data) return alloc_str("");
    return alloc_str(sp::rt::crypto::base64Encode(data));
}

const char* sp_rt_crypto_base64_decode(const char* data) {
    if (!data) return alloc_str("");
    return alloc_str(sp::rt::crypto::base64Decode(data));
}

const char* sp_rt_crypto_random_hex(int64_t bytes) {
    if (bytes <= 0) return alloc_str("");
    return alloc_str(sp::rt::crypto::randomHex(static_cast<size_t>(bytes)));
}

const char* sp_rt_crypto_uuid() {
    return alloc_str(sp::rt::crypto::randomUuid());
}

// ============================================================================
// JSON C-ABI
// ============================================================================

void* sp_rt_json_parse(const char* json_str) {
    if (!json_str) return nullptr;
    std::string err;
    auto val = sp::rt::json::parse(json_str, &err);
    if (!val) return nullptr;
    return new JsonWrapper{val};
}

const char* sp_rt_json_stringify(void* handle, int64_t indent) {
    if (!handle) return alloc_str("null");
    auto* w = static_cast<JsonWrapper*>(handle);
    if (!w->val) return alloc_str("null");
    return alloc_str(w->val->stringify(static_cast<int>(indent)));
}

int64_t sp_rt_json_get_type(void* handle) {
    if (!handle) return -1;
    auto* w = static_cast<JsonWrapper*>(handle);
    if (!w->val) return -1;
    return static_cast<int64_t>(w->val->getType());
}

bool sp_rt_json_get_bool(void* handle) {
    if (!handle) return false;
    auto* w = static_cast<JsonWrapper*>(handle);
    return w->val ? w->val->asBool() : false;
}

int64_t sp_rt_json_get_int(void* handle) {
    if (!handle) return 0;
    auto* w = static_cast<JsonWrapper*>(handle);
    return w->val ? w->val->asInt() : 0;
}

double sp_rt_json_get_double(void* handle) {
    if (!handle) return 0.0;
    auto* w = static_cast<JsonWrapper*>(handle);
    return w->val ? w->val->asDouble() : 0.0;
}

const char* sp_rt_json_get_string(void* handle) {
    if (!handle) return alloc_str("");
    auto* w = static_cast<JsonWrapper*>(handle);
    if (!w->val) return alloc_str("");
    return alloc_str(w->val->asString());
}

int64_t sp_rt_json_arr_size(void* handle) {
    if (!handle) return 0;
    auto* w = static_cast<JsonWrapper*>(handle);
    return w->val ? static_cast<int64_t>(w->val->size()) : 0;
}

void* sp_rt_json_arr_get(void* handle, int64_t idx) {
    if (!handle || idx < 0) return nullptr;
    auto* w = static_cast<JsonWrapper*>(handle);
    if (!w->val) return nullptr;
    auto item = w->val->get(static_cast<size_t>(idx));
    return item ? new JsonWrapper{item} : nullptr;
}

void sp_rt_json_arr_add(void* handle, void* item_handle) {
    if (!handle) return;
    auto* w = static_cast<JsonWrapper*>(handle);
    if (!w->val) return;
    auto item = item_handle ? static_cast<JsonWrapper*>(item_handle)->val : sp::rt::json::JsonValue::makeNull();
    w->val->add(item);
}

void* sp_rt_json_obj_get(void* handle, const char* key) {
    if (!handle || !key) return nullptr;
    auto* w = static_cast<JsonWrapper*>(handle);
    if (!w->val) return nullptr;
    auto item = w->val->get(key);
    return item ? new JsonWrapper{item} : nullptr;
}

void sp_rt_json_obj_set(void* handle, const char* key, void* item_handle) {
    if (!handle || !key) return;
    auto* w = static_cast<JsonWrapper*>(handle);
    if (!w->val) return;
    auto item = item_handle ? static_cast<JsonWrapper*>(item_handle)->val : sp::rt::json::JsonValue::makeNull();
    w->val->set(key, item);
}

bool sp_rt_json_obj_has(void* handle, const char* key) {
    if (!handle || !key) return false;
    auto* w = static_cast<JsonWrapper*>(handle);
    return w->val ? w->val->has(key) : false;
}

void* sp_rt_json_new_null() {
    return new JsonWrapper{sp::rt::json::JsonValue::makeNull()};
}

void* sp_rt_json_new_bool(bool b) {
    return new JsonWrapper{sp::rt::json::JsonValue::makeBool(b)};
}

void* sp_rt_json_new_int(int64_t i) {
    return new JsonWrapper{sp::rt::json::JsonValue::makeInt(i)};
}

void* sp_rt_json_new_double(double d) {
    return new JsonWrapper{sp::rt::json::JsonValue::makeDouble(d)};
}

void* sp_rt_json_new_string(const char* s) {
    return new JsonWrapper{sp::rt::json::JsonValue::makeString(s ? s : "")};
}

void* sp_rt_json_new_array() {
    return new JsonWrapper{sp::rt::json::JsonValue::makeArray()};
}

void* sp_rt_json_new_object() {
    return new JsonWrapper{sp::rt::json::JsonValue::makeObject()};
}

void sp_rt_json_destroy(void* handle) {
    if (handle) {
        delete static_cast<JsonWrapper*>(handle);
    }
}

// ============================================================================
// HTTP C-ABI
// ============================================================================

void* sp_rt_http_get(const char* url, int64_t timeout_ms) {
    if (!url) return nullptr;
    auto resp = sp::rt::http::HttpClient::get(url, static_cast<int>(timeout_ms));
    return new sp::rt::http::HttpResponse(std::move(resp));
}

void* sp_rt_http_post(const char* url, const char* body, const char* content_type, int64_t timeout_ms) {
    if (!url) return nullptr;
    std::string b = body ? body : "";
    std::string ct = content_type ? content_type : "application/json";
    auto resp = sp::rt::http::HttpClient::post(url, b, ct, static_cast<int>(timeout_ms));
    return new sp::rt::http::HttpResponse(std::move(resp));
}

int64_t sp_rt_http_resp_status(void* resp) {
    if (!resp) return 0;
    return static_cast<sp::rt::http::HttpResponse*>(resp)->statusCode;
}

const char* sp_rt_http_resp_status_text(void* resp) {
    if (!resp) return alloc_str("");
    return alloc_str(static_cast<sp::rt::http::HttpResponse*>(resp)->statusText);
}

const char* sp_rt_http_resp_body(void* resp) {
    if (!resp) return alloc_str("");
    return alloc_str(static_cast<sp::rt::http::HttpResponse*>(resp)->body);
}

const char* sp_rt_http_resp_header(void* resp, const char* name) {
    if (!resp || !name) return alloc_str("");
    return alloc_str(static_cast<sp::rt::http::HttpResponse*>(resp)->getHeader(name));
}

void sp_rt_http_resp_destroy(void* resp) {
    if (resp) {
        delete static_cast<sp::rt::http::HttpResponse*>(resp);
    }
}

// HTTP Server
void* sp_rt_http_server_create() {
    return new sp::rt::http::HttpServer();
}

void sp_rt_http_server_route(void* srv, const char* method, const char* path, 
                             const char* (*handler_fn)(const char* req_method, const char* req_path, const char* req_body)) {
    if (!srv || !path || !handler_fn) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    std::string m = method ? method : "GET";
    std::string p = path;
    s->route(m, p, [handler_fn](const sp::rt::http::HttpRequest& req) {
        const char* resultStr = handler_fn(req.method.c_str(), req.path.c_str(), req.body.c_str());
        if (!resultStr) return sp::rt::http::HttpResponse::ok();
        std::string resContent = resultStr;
        if (resContent.rfind("HTTP/1.1", 0) == 0) {
            // Already raw HTTP response
            sp::rt::http::HttpResponse res;
            res.setBody(resContent);
            return res;
        }
        if (!resContent.empty() && (resContent.front() == '{' || resContent.front() == '[')) {
            return sp::rt::http::HttpResponse::json(resContent);
        }
        return sp::rt::http::HttpResponse::ok(resContent);
    });
}

bool sp_rt_http_server_listen(void* srv, const char* host, int64_t port) {
    if (!srv) return false;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    std::string h = host ? host : "0.0.0.0";
    return s->listen(h, static_cast<int>(port));
}

int64_t sp_rt_http_server_port(void* srv) {
    if (!srv) return 0;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    return s->getPort();
}

void sp_rt_http_server_stop(void* srv) {
    if (srv) {
        static_cast<sp::rt::http::HttpServer*>(srv)->stop();
    }
}

void sp_rt_http_server_destroy(void* srv) {
    if (srv) {
        auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
        s->stop();
        delete s;
    }
}

} // extern "C"
