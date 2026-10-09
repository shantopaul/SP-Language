module std.web.context;

import std.http;
import std.collections;

// ============================================================================
// Standard Library Ergonomic HTTP Context (SP 2.7)
// Provides unified request inspection, response construction, cookie helpers,
// and request-lifecycle state sharing.
// ============================================================================

public class Context {
    public ptr reqHandle;
    public HttpRequest req;
    public HttpResponse res;
    public HashMap<string, string> state;

    public Context() {
        this.reqHandle = null;
        this.req = new HttpRequest();
        this.res = HttpResponse.ok("");
        this.state = new HashMap<string, string>();
    }

    public fn init(ptr reqHandle) {
        this.reqHandle = reqHandle;
        this.req = new HttpRequest();
        this.req.handle = reqHandle;
        this.res = HttpResponse.ok("");
        this.state = new HashMap<string, string>();
    }

    public static fn create(ptr reqHandle) -> Context {
        let ctx = new Context();
        ctx.reqHandle = reqHandle;
        ctx.req.handle = reqHandle;
        return ctx;
    }

    // Request Inspection Helpers
    public fn method() -> string {
        return this.req.method();
    }

    public fn path() -> string {
        return this.req.path();
    }

    public fn body() -> string {
        return this.req.body();
    }

    public fn header(string name) -> string {
        return this.req.header(name);
    }

    public fn query(string name) -> string {
        return this.req.query(name);
    }

    public fn param(string name) -> string {
        return this.req.param(name);
    }

    public fn cookie(string name) -> string {
        return this.req.cookie(name);
    }

    public fn getCookie(string name) -> string {
        return this.req.cookie(name);
    }

    // Response Construction Helpers
    public fn json(string jsonContent) -> HttpResponse {
        this.res.setStatus(200);
        this.res.setJson(jsonContent);
        return this.res;
    }

    public fn jsonStatus(string jsonContent, int status) -> HttpResponse {
        this.res.setStatus(status);
        this.res.setJson(jsonContent);
        return this.res;
    }

    public fn text(string textContent) -> HttpResponse {
        this.res.setStatus(200);
        this.res.setText(textContent);
        return this.res;
    }

    public fn textStatus(string textContent, int status) -> HttpResponse {
        this.res.setStatus(status);
        this.res.setText(textContent);
        return this.res;
    }

    public fn html(string htmlContent) -> HttpResponse {
        this.res.setStatus(200);
        this.res.setHtml(htmlContent);
        return this.res;
    }

    public fn htmlStatus(string htmlContent, int status) -> HttpResponse {
        this.res.setStatus(status);
        this.res.setHtml(htmlContent);
        return this.res;
    }

    public fn sse(string sseData) -> HttpResponse {
        this.res.setStatus(200);
        this.res.setHeader("Content-Type", "text/event-stream; charset=utf-8");
        this.res.setHeader("Cache-Control", "no-cache");
        this.res.setHeader("Connection", "keep-alive");
        this.res.setBody(sseData);
        return this.res;
    }

    public fn redirect(string url) -> HttpResponse {
        this.res.redirect(url, 302);
        return this.res;
    }

    public fn redirectStatus(string url, int status) -> HttpResponse {
        this.res.redirect(url, status);
        return this.res;
    }

    public fn status(int code) -> Context {
        this.res.setStatus(code);
        return this;
    }

    public fn setHeader(string name, string val) -> Context {
        this.res.setHeader(name, val);
        return this;
    }

    public fn setCookie(string name, string val, string path = "/", int maxAge = 86400, bool secure = false, bool httpOnly = true, string sameSite = "Lax") -> Context {
        this.res.setCookie(name, val, path, maxAge, secure, httpOnly, sameSite);
        return this;
    }

    // Request-Lifecycle State Sharing
    public fn set(string key, string val) {
        this.state.set(key, val);
    }

    public fn get(string key) -> string {
        if (this.state.contains(key)) {
            return this.state.get(key);
        }
        return "";
    }

    public fn has(string key) -> bool {
        return this.state.contains(key);
    }
}
