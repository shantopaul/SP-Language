module std.http;

// ============================================================================
// Standard Library HTTP/1.1 Engine, Client & Server Framework (SP 2.2)
// ============================================================================

extern "C" {
    fn sp_rt_http_get(string url, int timeoutMs) -> ptr;
    fn sp_rt_http_post(string url, string body, string contentType, int timeoutMs) -> ptr;
    fn sp_rt_http_resp_status(ptr resp) -> int;
    fn sp_rt_http_resp_status_text(ptr resp) -> string;
    fn sp_rt_http_resp_body(ptr resp) -> string;
    fn sp_rt_http_resp_header(ptr resp, string name) -> string;
    fn sp_rt_http_resp_destroy(ptr resp);

    // Request inspection
    fn sp_rt_http_req_method(ptr req) -> string;
    fn sp_rt_http_req_path(ptr req) -> string;
    fn sp_rt_http_req_body(ptr req) -> string;
    fn sp_rt_http_req_header(ptr req, string name) -> string;
    fn sp_rt_http_req_query(ptr req, string name) -> string;
    fn sp_rt_http_req_param(ptr req, string name) -> string;
    fn sp_rt_http_req_cookie(ptr req, string name) -> string;

    // Response construction
    fn sp_rt_http_resp_create(int status, string body) -> ptr;
    fn sp_rt_http_resp_set_status(ptr resp, int status);
    fn sp_rt_http_resp_set_header(ptr resp, string name, string val);
    fn sp_rt_http_resp_set_cookie(ptr resp, string name, string val, string path, int maxAge, bool secure, bool httpOnly, string sameSite);
    fn sp_rt_http_resp_set_body(ptr resp, string body);
    fn sp_rt_http_resp_set_json(ptr resp, string jsonStr);

    // Server
    fn sp_rt_http_server_create() -> ptr;
    fn sp_rt_http_server_create_advanced(int workers, int maxBodyBytes, int maxHeaderBytes) -> ptr;
    fn sp_rt_http_server_route(ptr srv, string method, string path, ptr handlerFnPtr);
    fn sp_rt_http_server_route_handler(ptr srv, string method, string path, ptr handlerFnPtr);
    fn sp_rt_http_server_listen(ptr srv, string host, int port) -> bool;
    fn sp_rt_http_server_port(ptr srv) -> int;
    fn sp_rt_http_server_stop(ptr srv);
    fn sp_rt_http_server_destroy(ptr srv);
}

public class HttpRequest {
    public ptr handle;

    public fn method() -> string {
        return sp_rt_http_req_method(this.handle);
    }

    public fn path() -> string {
        return sp_rt_http_req_path(this.handle);
    }

    public fn body() -> string {
        return sp_rt_http_req_body(this.handle);
    }

    public fn header(string name) -> string {
        return sp_rt_http_req_header(this.handle, name);
    }

    public fn query(string name) -> string {
        return sp_rt_http_req_query(this.handle, name);
    }

    public fn param(string name) -> string {
        return sp_rt_http_req_param(this.handle, name);
    }

    public fn cookie(string name) -> string {
        return sp_rt_http_req_cookie(this.handle, name);
    }
}

public class HttpResponse {
    public ptr handle;

    public static fn create(int status, string body) -> HttpResponse {
        let res = new HttpResponse();
        res.handle = sp_rt_http_resp_create(status, body);
        return res;
    }

    public static fn ok(string body) -> HttpResponse {
        return HttpResponse.create(200, body);
    }

    public static fn json(string jsonContent) -> HttpResponse {
        let res = HttpResponse.create(200, jsonContent);
        sp_rt_http_resp_set_header(res.handle, "Content-Type", "application/json; charset=utf-8");
        return res;
    }

    public static fn notFound(string body) -> HttpResponse {
        return HttpResponse.create(404, body);
    }

    public fn status() -> int {
        return sp_rt_http_resp_status(this.handle);
    }

    public fn statusText() -> string {
        return sp_rt_http_resp_status_text(this.handle);
    }

    public fn body() -> string {
        return sp_rt_http_resp_body(this.handle);
    }

    public fn header(string name) -> string {
        return sp_rt_http_resp_header(this.handle, name);
    }

    public fn isOk() -> bool {
        let s = this.status();
        return s >= 200 && s < 300;
    }

    public fn setStatus(int status) {
        sp_rt_http_resp_set_status(this.handle, status);
    }

    public fn setHeader(string name, string val) {
        sp_rt_http_resp_set_header(this.handle, name, val);
    }

    public fn setCookie(string name, string val, string path, int maxAge, bool secure, bool httpOnly, string sameSite) {
        sp_rt_http_resp_set_cookie(this.handle, name, val, path, maxAge, secure, httpOnly, sameSite);
    }

    public fn setBody(string body) {
        sp_rt_http_resp_set_body(this.handle, body);
    }

    public fn setJson(string jsonStr) {
        sp_rt_http_resp_set_json(this.handle, jsonStr);
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_http_resp_destroy(this.handle);
            this.handle = null;
        }
    }
}

public class HttpClient {
    public static fn get(string url) -> HttpResponse {
        let resp = new HttpResponse();
        resp.handle = sp_rt_http_get(url, 5000);
        return resp;
    }

    public static fn getTimeout(string url, int timeoutMs) -> HttpResponse {
        let resp = new HttpResponse();
        resp.handle = sp_rt_http_get(url, timeoutMs);
        return resp;
    }

    public static fn post(string url, string body) -> HttpResponse {
        let resp = new HttpResponse();
        resp.handle = sp_rt_http_post(url, body, "application/json", 5000);
        return resp;
    }

    public static fn postWithContentType(string url, string body, string contentType, int timeoutMs) -> HttpResponse {
        let resp = new HttpResponse();
        resp.handle = sp_rt_http_post(url, body, contentType, timeoutMs);
        return resp;
    }
}

public class HttpServer {
    public ptr handle;

    public static fn create() -> HttpServer {
        let server = new HttpServer();
        server.handle = sp_rt_http_server_create();
        return server;
    }

    public static fn createWithWorkers(int workers, int maxBodyBytes) -> HttpServer {
        let server = new HttpServer();
        server.handle = sp_rt_http_server_create_advanced(workers, maxBodyBytes, 16384);
        return server;
    }

    public fn route(string method, string path, ptr handlerFnPtr) {
        sp_rt_http_server_route(this.handle, method, path, handlerFnPtr);
    }

    public fn get(string path, ptr handlerFnPtr) {
        this.route("GET", path, handlerFnPtr);
    }

    public fn post(string path, ptr handlerFnPtr) {
        this.route("POST", path, handlerFnPtr);
    }

    public fn put(string path, ptr handlerFnPtr) {
        this.route("PUT", path, handlerFnPtr);
    }

    public fn deleteRoute(string path, ptr handlerFnPtr) {
        this.route("DELETE", path, handlerFnPtr);
    }

    public fn listen(int port) -> bool {
        return sp_rt_http_server_listen(this.handle, "0.0.0.0", port);
    }

    public fn listenHost(string host, int port) -> bool {
        return sp_rt_http_server_listen(this.handle, host, port);
    }

    public fn port() -> int {
        return sp_rt_http_server_port(this.handle);
    }

    public fn stop() {
        if (this.handle != null) {
            sp_rt_http_server_stop(this.handle);
        }
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_http_server_destroy(this.handle);
            this.handle = null;
        }
    }
}
