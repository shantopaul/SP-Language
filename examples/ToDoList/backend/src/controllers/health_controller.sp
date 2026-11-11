module controllers.health_controller;

import std.web;
import std.http;
import std.health;
import db.schema;

public class HealthController {
    public static fn health(Context ctx) -> HttpResponse {
        let dbOk = DatabaseManager.ping();
        Health.setStatus("database", dbOk, "SQLite WAL primary store");
        let dbState = "up";
        let statusStr = "healthy";
        let code = 200;
        if (!dbOk) {
            dbState = "down";
            statusStr = "unhealthy";
            code = 503;
        }
        let body = "{\"status\":\"" + statusStr
            + "\",\"service\":\"todolist-backend\""
            + ",\"version\":\"1.0.0\""
            + ",\"database\":\"" + dbState + "\""
            + "}";
        return ctx.jsonStatus(body, code);
    }

    public static fn liveness(Context ctx) -> HttpResponse {
        return ctx.jsonStatus(Health.liveness(), 200);
    }

    public static fn readiness(Context ctx) -> HttpResponse {
        let dbOk = DatabaseManager.ping();
        Health.setStatus("database", dbOk, "SQLite readiness probe");
        let code = 200;
        if (!dbOk) {
            code = 503;
        }
        return ctx.jsonStatus(Health.readiness(), code);
    }
}
