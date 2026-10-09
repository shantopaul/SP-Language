module controllers.health_controller;

import std.web;
import std.db;
import std.db.postgres;
import config.env;
import database.connection;

// ============================================================================
// Health Probe & OpenAPI Specification Controller
// ============================================================================

public class HealthController {
    public static fn getHealth(Context ctx) -> HttpResponse {
        let pg = DatabaseManager.getCloudConn();
        let isCloud = pg.isConnected();
        let totalItems: int64 = 0;
        let dbName = "Neon PostgreSQL Cloud (AWS US-East-2)";

        if (isCloud) {
            let countRes = pg.query("SELECT COUNT(*) FROM items;");
            if (countRes.isOk() && countRes.rowCount() > 0) {
                totalItems = int.parse(countRes.getValue(0, 0));
            }
            countRes.free();
            pg.close();
        } else {
            dbName = "SQLite (Fallback)";
            let db = Database.openSqlite("items.db", 5000, true);
            let countStmt = db.prepare("SELECT COUNT(*) FROM items;");
            if (countStmt.next()) {
                totalItems = countStmt.getInt(0);
            }
            countStmt.finalize();
            db.close();
        }

        let jsonResp = "{\"status\":\"healthy\",\"service\":\"" + Env.appName() + "\",\"environment\":\"" + Env.appEnv() + "\",\"framework\":\"sp-web 3.0\",\"database\":\"" + dbName + "\",\"total_items\":" + totalItems + ",\"auth_required\":true,\"uptime\":\"online\"}";
        return ctx.json(jsonResp);
    }

    public static fn getOpenApiSpec(Context ctx) -> HttpResponse {
        let spec = "{\"openapi\":\"3.0.3\",\"info\":{\"title\":\"" + Env.appName() + "\",\"version\":\"3.0.0\",\"description\":\"Enterprise REST API powered by SP Lang 3.0 & Neon Cloud PostgreSQL\"},\"security\":[{\"ApiKeyAuth\":[]}],\"components\":{\"securitySchemes\":{\"ApiKeyAuth\":{\"type\":\"apiKey\",\"in\":\"header\",\"name\":\"X-API-Key\"}}},\"paths\":{\"/api/health\":{\"get\":{\"summary\":\"Health probe & database status\",\"responses\":{\"200\":{\"description\":\"OK\"}}}},\"/api/items\":{\"get\":{\"summary\":\"List items\",\"responses\":{\"200\":{\"description\":\"OK\"}}},\"post\":{\"summary\":\"Create item\",\"security\":[{\"ApiKeyAuth\":[]}],\"responses\":{\"201\":{\"description\":\"Created\"},\"401\":{\"description\":\"Unauthorized\"}}}},\"/api/items/{id}\":{\"get\":{\"summary\":\"Get item by ID\",\"responses\":{\"200\":{\"description\":\"OK\"}}},\"put\":{\"summary\":\"Update item\",\"security\":[{\"ApiKeyAuth\":[]}],\"responses\":{\"200\":{\"description\":\"OK\"},\"401\":{\"description\":\"Unauthorized\"}}},\"delete\":{\"summary\":\"Delete item\",\"security\":[{\"ApiKeyAuth\":[]}],\"responses\":{\"200\":{\"description\":\"OK\"},\"401\":{\"description\":\"Unauthorized\"}}}}}}";
        return ctx.json(spec);
    }
}
