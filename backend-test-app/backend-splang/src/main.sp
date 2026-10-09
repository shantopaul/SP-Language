module main;

import std.io;
import std.web;
import config.env;
import database.connection;
import routes.router;

// ============================================================================
// SP Lang Professional Backend Service Entrypoint (backend-splang)
//
// Modular Architecture:
// - config.env               : Reads environment variables and .env configuration
// - database.connection      : Manages Neon Cloud PostgreSQL & SQLite fallback
// - models.item              : Domain models and JSON sanitization
// - middleware.auth          : API key and authorization security filters
// - controllers.health_controller : System health & OpenAPI 3.0 spec probes
// - controllers.item_controller   : Full REST CRUD actions with auth enforcement
// - routes.router            : Declarative routing pipeline
// ============================================================================

fn main() {
    println("==========================================================================");
    println("           SP Lang 3.0 Modular Backend Service (backend-splang)           ");
    println("==========================================================================");

    // 1. Load environment configuration (.env or OS environment variables)
    Env.load(".env");
    println("[✓] Loaded environment config. Port: " + Env.port() + ", App: " + Env.appName());

    // 2. Initialize persistent storage (Neon Cloud PostgreSQL & fallback)
    DatabaseManager.initSchema();
    println("[✓] Database initialized and verified.");

    // 3. Initialize HTTP Application Framework
    let app = new App();

    // Production middleware pipeline
    app.useLogger();
    app.useCors("*", "GET, POST, PUT, PATCH, DELETE, OPTIONS, HEAD", "Content-Type, Authorization, X-Requested-With, X-Request-ID, X-API-Key");
    app.useRecovery();

    // Interactive OpenAPI Documentation (Swagger UI & Redoc)
    app.swagger("/docs", "/openapi.json", "SP Lang Backend API");
    app.redoc("/redoc", "/openapi.json", "SP Lang Backend API");

    // 4. Register application routes
    AppRouter.registerRoutes(app);

    // 5. Start HTTP Engine
    println("\n[+] SP Lang backend server live at http://localhost:" + Env.port());
    println("    * REST API Root:   http://localhost:" + Env.port() + "/api/items");
    println("    * Health Probe:    http://localhost:" + Env.port() + "/api/health");
    println("    * Swagger UI Docs: http://localhost:" + Env.port() + "/docs");
    println("    * Redoc View:      http://localhost:" + Env.port() + "/redoc");
    println("    * Security:        API Key authentication enabled on write endpoints");
    println("==========================================================================");

    app.run(Env.port());
}
