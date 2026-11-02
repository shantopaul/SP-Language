module routes.router;

import std.web;
import controllers.health_controller;
import controllers.item_controller;

// ============================================================================
// Central Application Route Table
// Maps HTTP endpoints to controller handler actions.
// ============================================================================

public class AppRouter {
    public static fn registerRoutes(App app) {
        // OpenAPI Specification & Health Probes
        app.get("/openapi.json", (ptr req) => {
            let ctx = Context.create(req);
            return HealthController.getOpenApiSpec(ctx);
        });

        app.get("/api/health", (ptr req) => {
            let ctx = Context.create(req);
            return HealthController.getHealth(ctx);
        });

        // REST Resource: Items
        app.get("/api/items", (ptr req) => {
            let ctx = Context.create(req);
            return ItemController.listItems(ctx);
        });

        app.get("/api/items/:id", (ptr req) => {
            let ctx = Context.create(req);
            return ItemController.getItem(ctx);
        });

        app.post("/api/items", (ptr req) => {
            let ctx = Context.create(req);
            return ItemController.createItem(ctx);
        });

        app.put("/api/items/:id", (ptr req) => {
            let ctx = Context.create(req);
            return ItemController.updateItem(ctx);
        });

        app.deleteRoute("/api/items/:id", (ptr req) => {
            let ctx = Context.create(req);
            return ItemController.deleteItem(ctx);
        });
    }
}
