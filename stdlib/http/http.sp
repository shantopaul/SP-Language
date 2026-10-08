module std.http;

// ============================================================================
// Standard Library HTTP/1.1 Engine, Client & Server Framework (SP 1.6)
// ============================================================================

extern "C" {
    fn sp_rt_http_get(string url, int timeoutMs) -> ptr;
    fn sp_rt_http_post(string url, string body, string contentType, int timeoutMs) -> ptr;
    fn sp_rt_http_resp_status(ptr resp) -> int;
    fn sp_rt_http_resp_status_text(ptr resp) -> string;
    fn sp_rt_http_resp_body(ptr resp) -> string;
    fn sp_rt_http_resp_header(ptr resp, string name) -> string;
    fn sp_rt_http_resp_destroy(ptr resp);

    fn sp_rt_http_server_create() -> ptr;
    fn sp_rt_http_server_route(ptr srv, string method, string path, ptr handlerFnPtr);
    fn sp_rt_http_server_listen(ptr srv, string host, int port) -> bool;
    fn sp_rt_http_server_port(ptr srv) -> int;
    fn sp_rt_http_server_stop(ptr srv);
    fn sp_rt_http_server_destroy(ptr srv);
}

public class HttpResponse {
    public ptr handle;

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
