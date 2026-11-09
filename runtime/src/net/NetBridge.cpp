#include "sp/runtime/Net.h"
#include "sp/runtime/EventLoop.h"
#include "sp/runtime/Crypto.h"
#include "sp/runtime/Json.h"
#include "sp/runtime/Http.h"
#include "sp/runtime/WebSocket.h"
#include "sp/runtime/Sse.h"
#include "sp/runtime/Redis.h"
#include "sp/runtime/Runtime.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <unordered_map>
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

// Redis pool C-ABI ownership registry.
//
// RedisPool::acquire() returns a std::shared_ptr<RedisClient>, but the C
// ABI can only hand a raw RedisClient* back to SP code — and the pool
// drops its own reference when a client is checked out, so the local
// shared_ptr in sp_rt_redis_pool_acquire used to be the sole owner.
// Returning client.get() directly therefore dangled the moment that
// local died (use-after-free on first use), and the old no-op release()
// never returned the client to the pool either. This registry keeps one
// owning shared_ptr per checked-out raw pointer until release() hands it
// back to the pool. Clients MUST be released before their pool is
// destroyed; destroying a pool with checked-out clients drops those
// registry entries and invalidates the outstanding handles.
struct RedisPoolClientEntry {
    void* pool;
    std::shared_ptr<sp::rt::redis::RedisClient> client;
};
std::mutex g_redisPoolClientsMutex;
std::unordered_map<void*, RedisPoolClientEntry> g_redisPoolClients;

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

void sp_rt_json_register_class(const char* class_name, uint32_t object_size, bool has_vtable) {
    if (!class_name) return;
    sp::rt::json::ClassMetadata meta;
    meta.className = class_name;
    meta.objectSize = object_size;
    meta.hasVTable = has_vtable;
    sp::rt::json::ClassMetadataRegistry::instance().registerClass(std::move(meta));
}

void sp_rt_json_register_field(const char* class_name, const char* field_name, const char* json_key, 
                              int32_t type_kind, const char* type_name, uint32_t byte_offset, 
                              bool is_ignored, bool is_nullable) {
    if (!class_name || !field_name) return;
    auto* meta = const_cast<sp::rt::json::ClassMetadata*>(
        sp::rt::json::ClassMetadataRegistry::instance().findClass(class_name));
    if (!meta) {
        sp::rt::json::ClassMetadata newMeta;
        newMeta.className = class_name;
        sp::rt::json::ClassMetadataRegistry::instance().registerClass(newMeta);
        meta = const_cast<sp::rt::json::ClassMetadata*>(
            sp::rt::json::ClassMetadataRegistry::instance().findClass(class_name));
    }
    if (meta) {
        sp::rt::json::FieldMetadata f;
        f.fieldName = field_name;
        f.jsonKey = json_key && *json_key ? json_key : field_name;
        f.typeKind = static_cast<sp::rt::json::FieldTypeKind>(type_kind);
        f.typeName = type_name ? type_name : "";
        f.byteOffset = byte_offset;
        f.isIgnored = is_ignored;
        f.isNullable = is_nullable;
        meta->fields.push_back(std::move(f));
    }
}

void* sp_rt_json_decode_typed(const char* class_name, const char* json_str, char** out_err) {
    if (!class_name || !json_str) {
        if (out_err) *out_err = const_cast<char*>("Invalid null class name or JSON string");
        return nullptr;
    }
    std::string errStr;
    void* obj = sp::rt::json::decodeTypedObject(class_name, json_str, &errStr);
    if (!obj && out_err && !errStr.empty()) {
        *out_err = const_cast<char*>(alloc_str(errStr));
    }
    return obj;
}

const char* sp_rt_json_encode_typed(const char* class_name, const void* obj_ptr, int64_t indent) {
    if (!class_name || !obj_ptr) return alloc_str("null");
    std::string res = sp::rt::json::encodeTypedObject(class_name, obj_ptr, static_cast<int>(indent));
    return alloc_str(res);
}

const char* sp_rt_json_problem_details(const char* title, int32_t status, const char* detail, 
                                       const char* instance, const char* errors_json) {
    sp::rt::json::ProblemDetails pd;
    if (title && *title) pd.title = title;
    pd.status = status;
    if (detail && *detail) pd.detail = detail;
    if (instance && *instance) pd.instance = instance;
    if (errors_json && *errors_json) {
        std::string err;
        auto parsedErrs = sp::rt::json::parse(errors_json, &err);
        if (parsedErrs && parsedErrs->isArray()) {
            for (const auto& item : parsedErrs->getArrayItems()) {
                if (item && item->isObject()) {
                    auto fVal = item->get("field");
                    auto mVal = item->get("message");
                    pd.errors.push_back({
                        fVal ? std::string(fVal->asStringView()) : "",
                        mVal ? std::string(mVal->asStringView()) : ""
                    });
                }
            }
        }
    }
    return alloc_str(pd.toJsonString(0));
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

void* sp_rt_http_server_create_advanced(int64_t workers, int64_t max_body_bytes, int64_t max_header_bytes) {
    sp::rt::http::HttpServerConfig cfg;
    if (workers > 0) cfg.workerThreads = static_cast<int>(workers);
    if (max_body_bytes > 0) cfg.maxBodySize = static_cast<size_t>(max_body_bytes);
    if (max_header_bytes > 0) cfg.maxHeaderSize = static_cast<size_t>(max_header_bytes);
    return new sp::rt::http::HttpServer(cfg);
}

// Request inspection
const char* sp_rt_http_req_method(void* req) {
    if (!req) return alloc_str("GET");
    return alloc_str(static_cast<const sp::rt::http::HttpRequest*>(req)->method);
}

const char* sp_rt_http_req_path(void* req) {
    if (!req) return alloc_str("/");
    return alloc_str(static_cast<const sp::rt::http::HttpRequest*>(req)->path);
}

const char* sp_rt_http_req_body(void* req) {
    if (!req) return alloc_str("");
    return alloc_str(static_cast<const sp::rt::http::HttpRequest*>(req)->body);
}

const char* sp_rt_http_req_header(void* req, const char* name) {
    if (!req || !name) return alloc_str("");
    return alloc_str(static_cast<const sp::rt::http::HttpRequest*>(req)->getHeader(name));
}

const char* sp_rt_http_req_query(void* req, const char* name) {
    if (!req || !name) return alloc_str("");
    return alloc_str(static_cast<const sp::rt::http::HttpRequest*>(req)->getQueryParam(name));
}

const char* sp_rt_http_req_param(void* req, const char* name) {
    if (!req || !name) return alloc_str("");
    return alloc_str(static_cast<const sp::rt::http::HttpRequest*>(req)->getPathParam(name));
}

const char* sp_rt_http_req_cookie(void* req, const char* name) {
    if (!req || !name) return alloc_str("");
    return alloc_str(static_cast<const sp::rt::http::HttpRequest*>(req)->getCookie(name));
}

// Response construction
void* sp_rt_http_resp_create(int64_t status, const char* body) {
    auto* resp = new sp::rt::http::HttpResponse();
    resp->statusCode = static_cast<int>(status);
    resp->statusText = (status == 200) ? "OK" : ("Status " + std::to_string(status));
    if (body) resp->setBody(body);
    return resp;
}

void sp_rt_http_resp_set_status(void* resp, int64_t status) {
    if (!resp) return;
    auto* r = static_cast<sp::rt::http::HttpResponse*>(resp);
    r->statusCode = static_cast<int>(status);
    r->statusText = (status == 200) ? "OK" : ("Status " + std::to_string(status));
}

void sp_rt_http_resp_set_header(void* resp, const char* name, const char* val) {
    if (!resp || !name) return;
    static_cast<sp::rt::http::HttpResponse*>(resp)->setHeader(name, val ? val : "");
}

void sp_rt_http_resp_set_cookie(void* resp, const char* name, const char* val, 
                                const char* path, int64_t max_age, 
                                bool secure, bool http_only, const char* same_site) {
    if (!resp || !name) return;
    static_cast<sp::rt::http::HttpResponse*>(resp)->setCookie(
        name, val ? val : "", 
        path ? path : "/", static_cast<int>(max_age),
        secure, http_only, same_site ? same_site : "Lax"
    );
}

void sp_rt_http_resp_set_body(void* resp, const char* body) {
    if (!resp) return;
    static_cast<sp::rt::http::HttpResponse*>(resp)->setBody(body ? body : "");
}

void sp_rt_http_resp_set_json(void* resp, const char* json_str) {
    if (!resp) return;
    static_cast<sp::rt::http::HttpResponse*>(resp)->setJson(json_str ? json_str : "{}");
}

void sp_rt_http_resp_set_html(void* resp, const char* html) {
    if (!resp) return;
    auto* r = static_cast<sp::rt::http::HttpResponse*>(resp);
    r->setHeader("Content-Type", "text/html; charset=utf-8");
    r->setBody(html ? html : "");
}

void sp_rt_http_resp_redirect(void* resp, const char* url, int64_t status) {
    if (!resp) return;
    auto* r = static_cast<sp::rt::http::HttpResponse*>(resp);
    r->statusCode = (status > 0) ? static_cast<int>(status) : 302;
    r->statusText = (status == 301) ? "Moved Permanently" : "Found";
    r->setHeader("Location", url ? url : "/");
    r->setBody("");
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

void sp_rt_http_resp_set_text(void* resp, const char* text) {
    if (!resp) return;
    auto* r = static_cast<sp::rt::http::HttpResponse*>(resp);
    r->setHeader("Content-Type", "text/plain; charset=utf-8");
    r->setBody(text ? text : "");
}

static sp::rt::http::HttpResponse extractResponse(void* respHandle) {
    if (!respHandle) return sp::rt::http::HttpResponse::ok();

    // 1. Direct C++ HttpResponse pointer check (statusCode at offset 0: 100..599)
    auto* respPtr = static_cast<sp::rt::http::HttpResponse*>(respHandle);
    if (respPtr->statusCode >= 100 && respPtr->statusCode <= 599) {
        return *respPtr;
    }

    // 2. In SP Lang, HttpResponse object has:
    // slot 0 = vtable pointer
    // slot 1 = this.handle (pointer to C++ sp::rt::http::HttpResponse created by sp_rt_http_resp_create)
    void** spObjSlots = reinterpret_cast<void**>(respHandle);
    void* innerHandle = spObjSlots[1];
    if (innerHandle) {
        auto* innerPtr = static_cast<sp::rt::http::HttpResponse*>(innerHandle);
        if (innerPtr->statusCode >= 100 && innerPtr->statusCode <= 599) {
            return *innerPtr;
        }
    }

    // 3. Also check slot 0 if non-vtable layout
    void* slot0Handle = spObjSlots[0];
    if (slot0Handle && slot0Handle != innerHandle) {
        auto* slot0Ptr = static_cast<sp::rt::http::HttpResponse*>(slot0Handle);
        if (slot0Ptr->statusCode >= 100 && slot0Ptr->statusCode <= 599) {
            return *slot0Ptr;
        }
    }

    return sp::rt::http::HttpResponse::ok();
}

void sp_rt_http_server_route_handler(void* srv, const char* method, const char* path, 
                                     void* (*handler_fn)(void* req)) {
    if (!srv || !path || !handler_fn) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    std::string m = method ? method : "GET";
    std::string p = path;
    s->route(m, p, [handler_fn](const sp::rt::http::HttpRequest& req) {
        void* respHandle = handler_fn(const_cast<void*>(static_cast<const void*>(&req)));
        return extractResponse(respHandle);
    });
}

bool sp_rt_http_server_listen(void* srv, const char* host, int64_t port) {
    if (!srv) return false;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    std::string h = host ? host : "0.0.0.0";
    return s->listen(h, static_cast<int>(port));
}

void sp_rt_http_server_wait(void* srv) {
    if (!srv) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    s->wait();
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

// Phase 21: High-Performance Routing, Route Groups & Middleware Pipeline
void* sp_rt_http_server_group(void* srv, const char* prefix) {
    if (!srv) return nullptr;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    std::string p = prefix ? prefix : "";
    return new sp::rt::http::RouteGroup(s->group(p));
}

void* sp_rt_http_group_group(void* grp, const char* prefix) {
    if (!grp) return nullptr;
    auto* g = static_cast<sp::rt::http::RouteGroup*>(grp);
    std::string p = prefix ? prefix : "";
    return new sp::rt::http::RouteGroup(g->group(p));
}

void sp_rt_http_group_route(void* grp, const char* method, const char* path, 
                             const char* (*handler_fn)(const char* req_method, const char* req_path, const char* req_body)) {
    if (!grp || !path || !handler_fn) return;
    auto* g = static_cast<sp::rt::http::RouteGroup*>(grp);
    std::string m = method ? method : "GET";
    std::string p = path;
    g->route(m, p, [handler_fn](const sp::rt::http::HttpRequest& req) {
        const char* resultStr = handler_fn(req.method.c_str(), req.path.c_str(), req.body.c_str());
        if (!resultStr) return sp::rt::http::HttpResponse::ok();
        std::string resContent = resultStr;
        if (resContent.rfind("HTTP/1.1", 0) == 0) {
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

void sp_rt_http_group_route_handler(void* grp, const char* method, const char* path, 
                                     void* (*handler_fn)(void* req)) {
    if (!grp || !path || !handler_fn) return;
    auto* g = static_cast<sp::rt::http::RouteGroup*>(grp);
    std::string m = method ? method : "GET";
    std::string p = path;
    g->route(m, p, [handler_fn](const sp::rt::http::HttpRequest& req) {
        void* respHandle = handler_fn(const_cast<void*>(static_cast<const void*>(&req)));
        return extractResponse(respHandle);
    });
}

void sp_rt_http_group_destroy(void* grp) {
    if (grp) {
        delete static_cast<sp::rt::http::RouteGroup*>(grp);
    }
}

void sp_rt_http_server_use_cors(void* srv, const char* origins, const char* methods, const char* headers) {
    if (!srv) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    sp::rt::http::CorsOptions opts;
    if (origins && *origins) opts.allowedOrigins = origins;
    if (methods && *methods) opts.allowedMethods = methods;
    if (headers && *headers) opts.allowedHeaders = headers;
    s->use(sp::rt::http::HttpServer::cors(opts));
}

void sp_rt_http_server_use_logger(void* srv) {
    if (!srv) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    s->use(sp::rt::http::HttpServer::logger());
}

void sp_rt_http_server_use_recovery(void* srv) {
    if (!srv) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    s->use(sp::rt::http::HttpServer::recovery());
}

void sp_rt_http_server_use_request_id(void* srv) {
    if (!srv) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    s->use(sp::rt::http::HttpServer::requestId());
}

void sp_rt_http_server_use_rate_limiter(void* srv, int64_t max_reqs, int64_t window_secs) {
    if (!srv) return;
    auto* s = static_cast<sp::rt::http::HttpServer*>(srv);
    sp::rt::http::RateLimiterOptions opts;
    if (max_reqs > 0) opts.maxRequests = static_cast<int>(max_reqs);
    if (window_secs > 0) opts.windowSeconds = static_cast<int>(window_secs);
    s->use(sp::rt::http::HttpServer::rateLimiter(opts));
}

// ========================================================================
// Phase 26: Real-Time Web & Distributed Caching (WebSocket & Redis)
// ========================================================================

// WebSocket
const char* sp_rt_ws_accept_key(const char* client_key) {
    if (!client_key) return alloc_str("");
    return alloc_str(sp::rt::ws::computeWebSocketAccept(client_key));
}

void* sp_rt_ws_hub_create() {
    return new sp::rt::ws::WebSocketHub();
}

void sp_rt_ws_hub_destroy(void* hub) {
    if (hub) delete static_cast<sp::rt::ws::WebSocketHub*>(hub);
}

void sp_rt_ws_hub_broadcast(void* hub, const char* msg) {
    if (!hub || !msg) return;
    static_cast<sp::rt::ws::WebSocketHub*>(hub)->broadcast(msg);
}

void sp_rt_ws_hub_broadcast_room(void* hub, const char* room, const char* msg) {
    if (!hub || !room || !msg) return;
    static_cast<sp::rt::ws::WebSocketHub*>(hub)->broadcastTo(room, msg);
}

void sp_rt_ws_hub_join(void* hub, int64_t conn_id, const char* room) {
    if (!hub || !room) return;
    static_cast<sp::rt::ws::WebSocketHub*>(hub)->joinRoom(static_cast<uint64_t>(conn_id), room);
}

void sp_rt_ws_hub_leave(void* hub, int64_t conn_id, const char* room) {
    if (!hub || !room) return;
    static_cast<sp::rt::ws::WebSocketHub*>(hub)->leaveRoom(static_cast<uint64_t>(conn_id), room);
}

int64_t sp_rt_ws_hub_size(void* hub) {
    if (!hub) return 0;
    return static_cast<int64_t>(static_cast<sp::rt::ws::WebSocketHub*>(hub)->size());
}

int64_t sp_rt_ws_hub_room_size(void* hub, const char* room) {
    if (!hub || !room) return 0;
    return static_cast<int64_t>(static_cast<sp::rt::ws::WebSocketHub*>(hub)->roomSize(room));
}

// Server-Sent Events (SSE)
const char* sp_rt_sse_format(const char* event, const char* data, const char* id, int64_t retry) {
    return alloc_str(sp::rt::sse::formatEvent(
        event ? event : "",
        data ? data : "",
        id ? id : "",
        retry
    ));
}

const char* sp_rt_sse_heartbeat() {
    return alloc_str(sp::rt::sse::formatHeartbeat());
}

// Redis Client
void* sp_rt_redis_create(const char* host, int64_t port) {
    auto* client = new sp::rt::redis::RedisClient();
    std::string h = host ? host : "127.0.0.1";
    int p = (port > 0) ? static_cast<int>(port) : 6379;
    client->connect(h, p);
    return client;
}

void sp_rt_redis_destroy(void* client) {
    if (client) delete static_cast<sp::rt::redis::RedisClient*>(client);
}

bool sp_rt_redis_connect(void* client) {
    if (!client) return false;
    return static_cast<sp::rt::redis::RedisClient*>(client)->isConnected();
}

const char* sp_rt_redis_ping(void* client) {
    if (!client) return alloc_str("");
    return alloc_str(static_cast<sp::rt::redis::RedisClient*>(client)->ping());
}

const char* sp_rt_redis_get(void* client, const char* key) {
    if (!client || !key) return alloc_str("");
    auto val = static_cast<sp::rt::redis::RedisClient*>(client)->get(key);
    return alloc_str(val.value_or(""));
}

bool sp_rt_redis_set(void* client, const char* key, const char* val) {
    if (!client || !key) return false;
    return static_cast<sp::rt::redis::RedisClient*>(client)->set(key, val ? val : "");
}

bool sp_rt_redis_setex(void* client, const char* key, int64_t seconds, const char* val) {
    if (!client || !key) return false;
    return static_cast<sp::rt::redis::RedisClient*>(client)->setEx(key, seconds, val ? val : "");
}

bool sp_rt_redis_del(void* client, const char* key) {
    if (!client || !key) return false;
    return static_cast<sp::rt::redis::RedisClient*>(client)->del(key);
}

bool sp_rt_redis_exists(void* client, const char* key) {
    if (!client || !key) return false;
    return static_cast<sp::rt::redis::RedisClient*>(client)->exists(key);
}

int64_t sp_rt_redis_ttl(void* client, const char* key) {
    if (!client || !key) return -2;
    return static_cast<sp::rt::redis::RedisClient*>(client)->ttl(key);
}

int64_t sp_rt_redis_incr(void* client, const char* key) {
    if (!client || !key) return 0;
    return static_cast<sp::rt::redis::RedisClient*>(client)->incr(key);
}

int64_t sp_rt_redis_decr(void* client, const char* key) {
    if (!client || !key) return 0;
    return static_cast<sp::rt::redis::RedisClient*>(client)->decr(key);
}

const char* sp_rt_redis_hget(void* client, const char* key, const char* field) {
    if (!client || !key || !field) return alloc_str("");
    auto val = static_cast<sp::rt::redis::RedisClient*>(client)->hget(key, field);
    return alloc_str(val.value_or(""));
}

bool sp_rt_redis_hset(void* client, const char* key, const char* field, const char* val) {
    if (!client || !key || !field) return false;
    return static_cast<sp::rt::redis::RedisClient*>(client)->hset(key, field, val ? val : "");
}

bool sp_rt_redis_hdel(void* client, const char* key, const char* field) {
    if (!client || !key || !field) return false;
    return static_cast<sp::rt::redis::RedisClient*>(client)->hdel(key, field);
}

int64_t sp_rt_redis_lpush(void* client, const char* key, const char* val) {
    if (!client || !key) return 0;
    return static_cast<sp::rt::redis::RedisClient*>(client)->lpush(key, val ? val : "");
}

int64_t sp_rt_redis_rpush(void* client, const char* key, const char* val) {
    if (!client || !key) return 0;
    return static_cast<sp::rt::redis::RedisClient*>(client)->rpush(key, val ? val : "");
}

const char* sp_rt_redis_lpop(void* client, const char* key) {
    if (!client || !key) return alloc_str("");
    auto val = static_cast<sp::rt::redis::RedisClient*>(client)->lpop(key);
    return alloc_str(val.value_or(""));
}

const char* sp_rt_redis_rpop(void* client, const char* key) {
    if (!client || !key) return alloc_str("");
    auto val = static_cast<sp::rt::redis::RedisClient*>(client)->rpop(key);
    return alloc_str(val.value_or(""));
}

int64_t sp_rt_redis_llen(void* client, const char* key) {
    if (!client || !key) return 0;
    return static_cast<sp::rt::redis::RedisClient*>(client)->llen(key);
}

int64_t sp_rt_redis_publish(void* client, const char* channel, const char* message) {
    if (!client || !channel || !message) return 0;
    return static_cast<sp::rt::redis::RedisClient*>(client)->publish(channel, message);
}

// Redis Pool
void* sp_rt_redis_pool_create(const char* host, int64_t port, int64_t min_conn, int64_t max_conn) {
    sp::rt::redis::RedisPoolConfig cfg;
    if (host && *host) cfg.host = host;
    if (port > 0) cfg.port = static_cast<int>(port);
    if (min_conn > 0) cfg.minConnections = static_cast<size_t>(min_conn);
    if (max_conn > 0) cfg.maxConnections = static_cast<size_t>(max_conn);
    return new sp::rt::redis::RedisPool(cfg);
}

void sp_rt_redis_pool_destroy(void* pool) {
    if (!pool) return;
    {
        // Drop ownership of any still-checked-out clients of this pool so
        // a later release() cannot touch the deleted pool (those handles
        // are invalid once their pool is gone — release clients first).
        std::lock_guard<std::mutex> lock(g_redisPoolClientsMutex);
        for (auto it = g_redisPoolClients.begin(); it != g_redisPoolClients.end();) {
            if (it->second.pool == pool) {
                it = g_redisPoolClients.erase(it);
            } else {
                ++it;
            }
        }
    }
    delete static_cast<sp::rt::redis::RedisPool*>(pool);
}

void* sp_rt_redis_pool_acquire(void* pool) {
    if (!pool) return nullptr;
    auto client = static_cast<sp::rt::redis::RedisPool*>(pool)->acquire();
    if (!client) return nullptr;
    // Keep one owning reference in the registry: without it the local
    // shared_ptr dies here and the returned raw pointer dangles.
    void* raw = client.get();
    {
        std::lock_guard<std::mutex> lock(g_redisPoolClientsMutex);
        g_redisPoolClients[raw] = RedisPoolClientEntry{pool, std::move(client)};
    }
    return raw;
}

void sp_rt_redis_pool_release(void* pool, void* client) {
    if (!pool || !client) return;
    std::shared_ptr<sp::rt::redis::RedisClient> owned;
    {
        std::lock_guard<std::mutex> lock(g_redisPoolClientsMutex);
        auto it = g_redisPoolClients.find(client);
        if (it == g_redisPoolClients.end() || it->second.pool != pool) return;
        owned = std::move(it->second.client);
        g_redisPoolClients.erase(it);
    }
    static_cast<sp::rt::redis::RedisPool*>(pool)->release(std::move(owned));
}

int64_t sp_rt_redis_pool_available(void* pool) {
    if (!pool) return 0;
    return static_cast<int64_t>(static_cast<sp::rt::redis::RedisPool*>(pool)->available());
}

int64_t sp_rt_redis_pool_total(void* pool) {
    if (!pool) return 0;
    return static_cast<int64_t>(static_cast<sp::rt::redis::RedisPool*>(pool)->total());
}

} // extern "C"
