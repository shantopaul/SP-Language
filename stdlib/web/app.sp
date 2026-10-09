module std.web.app;

import std.http;
import std.web.context;
import std.web.di;

// ============================================================================
// Standard Library Backend Application Framework (SP 2.7)
// High-level App class unifying routing, route groups, dependency injection,
// and composable middleware pipelines.
// ============================================================================

extern "C" {
    fn sp_rt_openapi_swagger_html(string title, string openApiJsonUrl) -> string;
    fn sp_rt_openapi_redoc_html(string title, string openApiJsonUrl) -> string;
}

public class Cors {
    public string origins;
    public string methods;
    public string headers;

    public Cors() {
        this.origins = "*";
        this.methods = "GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD";
        this.headers = "Content-Type, Authorization, X-Requested-With, X-Request-ID";
    }

    public static fn create(string origins, string methods, string headers) -> Cors {
        let c = new Cors();
        c.origins = origins;
        c.methods = methods;
        c.headers = headers;
        return c;
    }
}

public class Logger {
    public Logger() {}
}

public class Recovery {
    public Recovery() {}
}

public class RequestId {
    public RequestId() {}
}

public class RateLimiter {
    public int maxRequests;
    public int windowSeconds;

    public RateLimiter(int maxRequests = 100, int windowSeconds = 60) {
        this.maxRequests = maxRequests;
        this.windowSeconds = windowSeconds;
    }

    public static fn create(int maxRequests, int windowSeconds) -> RateLimiter {
        let rl = new RateLimiter(maxRequests, windowSeconds);
        return rl;
    }
}

public class WebGroup {
    public RouteGroup group;
    public Container container;

    public fn route(string method, string path, ptr handlerFnPtr) {
        this.group.routeHandler(method, path, handlerFnPtr);
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

    public fn patch(string path, ptr handlerFnPtr) {
        this.route("PATCH", path, handlerFnPtr);
    }

    public fn delete(string path, ptr handlerFnPtr) {
        this.route("DELETE", path, handlerFnPtr);
    }

    public fn deleteRoute(string path, ptr handlerFnPtr) {
        this.route("DELETE", path, handlerFnPtr);
    }

    public fn options(string path, ptr handlerFnPtr) {
        this.route("OPTIONS", path, handlerFnPtr);
    }

    public fn head(string path, ptr handlerFnPtr) {
        this.route("HEAD", path, handlerFnPtr);
    }

    public fn group(string subPrefix) -> WebGroup {
        let child = new WebGroup();
        child.group = this.group.group(subPrefix);
        child.container = this.container;
        return child;
    }

    public fn destroy() {
        if (this.group.handle != null) {
            this.group.destroy();
        }
    }
}

public class App {
    public HttpServer server;
    public Container container;

    public App() {
        this.server = HttpServer.create();
        this.container = new Container();
    }

    public fn init() {
        this.server = HttpServer.create();
        this.container = new Container();
    }

    public static fn create() -> App {
        return new App();
    }

    public static fn createWithWorkers(int workers, int maxBodyBytes) -> App {
        let app = new App();
        app.server = HttpServer.createWithWorkers(workers, maxBodyBytes);
        return app;
    }

    // Routing
    public fn route(string method, string path, ptr handlerFnPtr) {
        this.server.routeHandler(method, path, handlerFnPtr);
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

    public fn patch(string path, ptr handlerFnPtr) {
        this.route("PATCH", path, handlerFnPtr);
    }

    public fn delete(string path, ptr handlerFnPtr) {
        this.route("DELETE", path, handlerFnPtr);
    }

    public fn deleteRoute(string path, ptr handlerFnPtr) {
        this.route("DELETE", path, handlerFnPtr);
    }

    public fn options(string path, ptr handlerFnPtr) {
        this.route("OPTIONS", path, handlerFnPtr);
    }

    public fn head(string path, ptr handlerFnPtr) {
        this.route("HEAD", path, handlerFnPtr);
    }

    // Route Groups
    public fn group(string prefix) -> WebGroup {
        let g = new WebGroup();
        g.group = this.server.group(prefix);
        g.container = this.container;
        return g;
    }

    // Composable Middleware Helpers
    public fn useCorsConfig(Cors c) {
        this.server.useCors(c.origins, c.methods, c.headers);
    }

    // Explicit Middleware helpers
    public fn useCors(string origins = "*", string methods = "GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD", string headers = "Content-Type, Authorization, X-Requested-With, X-Request-ID") {
        this.server.useCors(origins, methods, headers);
    }

    public fn useCorsDefault() {
        this.server.useCorsDefault();
    }

    public fn useLogger() {
        this.server.useLogger();
    }

    public fn useRecovery() {
        this.server.useRecovery();
    }

    public fn useRequestId() {
        this.server.useRequestId();
    }

    public fn useRateLimiter(int maxRequests = 100, int windowSeconds = 60) {
        this.server.useRateLimiter(maxRequests, windowSeconds);
    }

    // Server Lifecycle
    public fn listen(int port) -> bool {
        return this.server.listen(port);
    }

    public fn listenHost(string host, int port) -> bool {
        return this.server.listenHost(host, port);
    }

    public fn port() -> int {
        return this.server.port();
    }

    public fn stop() {
        this.server.stop();
    }

    public fn close() {
        this.server.destroy();
    }

    // Phase 28: OpenAPI / Swagger UI / Redoc Documentation Endpoints
    public fn swagger(string path = "/docs", string openApiJsonUrl = "/openapi.json", string title = "API Documentation") {
        let htmlContent = sp_rt_openapi_swagger_html(title, openApiJsonUrl);
        this.get(path, (ptr req) => {
            let ctx = Context.create(req);
            return ctx.html(htmlContent);
        });
    }

    public fn redoc(string path = "/redoc", string openApiJsonUrl = "/openapi.json", string title = "API Documentation") {
        let htmlContent = sp_rt_openapi_redoc_html(title, openApiJsonUrl);
        this.get(path, (ptr req) => {
            let ctx = Context.create(req);
            return ctx.html(htmlContent);
        });
    }

    public fn openApi(string path = "/openapi.json", string specJson = "{}") {
        this.get(path, (ptr req) => {
            let ctx = Context.create(req);
            return ctx.json(specJson);
        });
    }
}
