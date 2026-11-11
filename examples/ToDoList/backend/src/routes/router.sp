module routes.router;

import std.web;
import std.http;
import std.fs;
import config.env;
import controllers.health_controller;
import controllers.auth_controller;
import controllers.category_controller;
import controllers.todo_controller;

public class Router {
    /// Register middleware, OpenAPI docs, and all API routes on an App instance.
    public static fn configureApp(App app) {
        app.useRecovery();
        app.useRequestId();
        app.useLogger();
        app.useCors(
            Env.corsOrigins(),
            "GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD",
            "Content-Type, Authorization, X-Requested-With, X-Request-ID"
        );
        app.useRateLimiter(Env.rateLimitMax(), Env.rateLimitWindowSeconds());

        Router.registerRoutes(app);

        if (exists("openapi.json")) {
            let spec = readText("openapi.json");
            if (spec.trim().length() > 0) {
                app.openApi("/openapi.json", spec);
            }
        }
        app.swagger("/docs", "/openapi.json", "ToDoList API v1");
        app.redoc("/redoc", "/openapi.json", "ToDoList API v1");
    }

    public static fn buildApp() -> App {
        let app = App.create();
        Router.configureApp(app);
        return app;
    }

    public static fn registerRoutes(App app) {
        // Health & Kubernetes-style Probes
        // @openapi GET /api/health Database-backed health check
        app.get("/api/health", (ptr req) => {
            let ctx = Context.create(req);
            return HealthController.health(ctx);
        });
        // @openapi GET /healthz Liveness probe
        app.get("/healthz", (ptr req) => {
            let ctx = Context.create(req);
            return HealthController.liveness(ctx);
        });
        // @openapi GET /readyz Readiness probe
        app.get("/readyz", (ptr req) => {
            let ctx = Context.create(req);
            return HealthController.readiness(ctx);
        });

        // Authentication & Profile
        // @openapi POST /api/v1/auth/register Register user account
        app.post("/api/v1/auth/register", (ptr req) => {
            let ctx = Context.create(req);
            return AuthController.register(ctx);
        });
        // @openapi POST /api/v1/auth/login Authenticate user and issue JWT
        app.post("/api/v1/auth/login", (ptr req) => {
            let ctx = Context.create(req);
            return AuthController.login(ctx);
        });
        // @openapi GET /api/v1/auth/me Get current authenticated user profile
        app.get("/api/v1/auth/me", (ptr req) => {
            let ctx = Context.create(req);
            return AuthController.me(ctx);
        });
        // @openapi PATCH /api/v1/auth/me Update current user display name
        app.patch("/api/v1/auth/me", (ptr req) => {
            let ctx = Context.create(req);
            return AuthController.updateProfile(ctx);
        });

        // Categories
        // @openapi GET /api/v1/categories List user categories with task counts
        app.get("/api/v1/categories", (ptr req) => {
            let ctx = Context.create(req);
            return CategoryController.list(ctx);
        });
        // @openapi POST /api/v1/categories Create a category
        app.post("/api/v1/categories", (ptr req) => {
            let ctx = Context.create(req);
            return CategoryController.create(ctx);
        });
        // @openapi PATCH /api/v1/categories/:id Update category
        app.patch("/api/v1/categories/:id", (ptr req) => {
            let ctx = Context.create(req);
            return CategoryController.update(ctx);
        });
        // @openapi PUT /api/v1/categories/:id Replace category
        app.put("/api/v1/categories/:id", (ptr req) => {
            let ctx = Context.create(req);
            return CategoryController.update(ctx);
        });
        // @openapi DELETE /api/v1/categories/:id Delete category
        app.deleteRoute("/api/v1/categories/:id", (ptr req) => {
            let ctx = Context.create(req);
            return CategoryController.remove(ctx);
        });

        // Todos (static sub-routes registered BEFORE parameterized :id routes)
        // @openapi GET /api/v1/todos/stats Aggregated workspace task statistics
        app.get("/api/v1/todos/stats", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.stats(ctx);
        });
        // @openapi POST /api/v1/todos/bulk Execute bulk operation across selected todos
        app.post("/api/v1/todos/bulk", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.bulk(ctx);
        });
        // @openapi GET /api/v1/todos List paginated, filtered, sorted todos
        app.get("/api/v1/todos", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.list(ctx);
        });
        // @openapi POST /api/v1/todos Create todo task
        app.post("/api/v1/todos", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.create(ctx);
        });
        // @openapi GET /api/v1/todos/:id Get single todo by ID
        app.get("/api/v1/todos/:id", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.getOne(ctx);
        });
        // @openapi PATCH /api/v1/todos/:id Partially update todo
        app.patch("/api/v1/todos/:id", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.update(ctx);
        });
        // @openapi PUT /api/v1/todos/:id Update todo
        app.put("/api/v1/todos/:id", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.update(ctx);
        });
        // @openapi DELETE /api/v1/todos/:id Delete todo
        app.deleteRoute("/api/v1/todos/:id", (ptr req) => {
            let ctx = Context.create(req);
            return TodoController.remove(ctx);
        });
    }
}
