module controllers.item_controller;

import std.web;
import std.db;
import std.db.postgres;
import std.json;
import std.string;
import models.item;
import database.connection;
import middleware.auth;

// ============================================================================
// Item Resource Controller
// Implements full REST CRUD with PostgreSQL Neon Cloud & SQLite fallback.
// Enforces API Key authorization on write operations (POST, PUT, DELETE).
// ============================================================================

public class ItemController {

    // [GET] /api/items — List all items
    public static fn listItems(Context ctx) -> HttpResponse {
        let pg = DatabaseManager.getCloudConn();
        if (pg.isConnected()) {
            let res = pg.query("SELECT id, title, description, status, priority, to_char(created_at, 'YYYY-MM-DD HH24:MI:SS') FROM items ORDER BY id DESC;");
            let itemsJson = "[";
            let first = true;
            if (res.isOk()) {
                let rows: int64 = res.rowCount();
                for (let i: int64 = 0; i < rows; i = i + 1) {
                    let id = res.getValue(i, 0);
                    let title = res.getValue(i, 1);
                    let desc = res.getValue(i, 2);
                    let status = res.getValue(i, 3);
                    let priority = res.getValue(i, 4);
                    let createdAt = res.getValue(i, 5);

                    if (!first) { itemsJson = itemsJson + ","; }
                    first = false;

                    itemsJson = itemsJson + ItemModel.serialize(id, title, desc, status, priority, createdAt);
                }
            }
            res.free();
            pg.close();
            itemsJson = itemsJson + "]";
            return ctx.json(itemsJson);
        }

        // Fallback SQLite
        let db = Database.openSqlite("items.db", 5000, true);
        let stmt = db.prepare("SELECT id, title, description, status, priority, created_at FROM items ORDER BY id DESC;");
        let itemsJson = "[";
        let first = true;
        while (stmt.next()) {
            let id = stmt.getString(0);
            let title = stmt.getString(1);
            let desc = stmt.getString(2);
            let status = stmt.getString(3);
            let priority = stmt.getString(4);
            let createdAt = stmt.getString(5);

            if (!first) { itemsJson = itemsJson + ","; }
            first = false;

            itemsJson = itemsJson + ItemModel.serialize(id, title, desc, status, priority, createdAt);
        }
        stmt.finalize();
        db.close();
        itemsJson = itemsJson + "]";
        return ctx.json(itemsJson);
    }

    // [GET] /api/items/:id — Get item by ID
    public static fn getItem(Context ctx) -> HttpResponse {
        let idStr = ctx.param("id");
        let targetId = int.parse(idStr);
        let pg = DatabaseManager.getCloudConn();

        if (pg.isConnected()) {
            let res = pg.query("SELECT id, title, description, status, priority, to_char(created_at, 'YYYY-MM-DD HH24:MI:SS') FROM items WHERE id = " + targetId + ";");
            if (res.isOk() && res.rowCount() > 0) {
                let id = res.getValue(0, 0);
                let title = res.getValue(0, 1);
                let desc = res.getValue(0, 2);
                let status = res.getValue(0, 3);
                let priority = res.getValue(0, 4);
                let createdAt = res.getValue(0, 5);
                res.free();
                pg.close();

                let itemJson = ItemModel.serialize(id, title, desc, status, priority, createdAt);
                return ctx.json(itemJson);
            }
            res.free();
            pg.close();
            return ctx.jsonStatus("{\"error\":\"Item not found\",\"id\":" + targetId + "}", 404);
        }

        // Fallback SQLite
        let db = Database.openSqlite("items.db", 5000, true);
        let stmt = db.prepare("SELECT id, title, description, status, priority, created_at FROM items WHERE id = " + targetId + ";");
        if (stmt.next()) {
            let id = stmt.getString(0);
            let title = stmt.getString(1);
            let desc = stmt.getString(2);
            let status = stmt.getString(3);
            let priority = stmt.getString(4);
            let createdAt = stmt.getString(5);
            stmt.finalize();
            db.close();

            let itemJson = ItemModel.serialize(id, title, desc, status, priority, createdAt);
            return ctx.json(itemJson);
        }
        stmt.finalize();
        db.close();
        return ctx.jsonStatus("{\"error\":\"Item not found\",\"id\":" + targetId + "}", 404);
    }

    // [POST] /api/items — Create new item (Protected with API Key)
    public static fn createItem(Context ctx) -> HttpResponse {
        if (!AuthMiddleware.validateKey(ctx)) {
            return AuthMiddleware.unauthorizedResponse(ctx);
        }

        let body = ctx.body();
        let title = "Untitled Task";
        let desc = "";
        let status = "pending";
        let priority = "normal";

        if (body != "") {
            let j = Json.parse(body);
            if (j.isValid()) {
                if (j.has("title")) {
                    let t = j.get("title").asString();
                    if (t != "") { title = t; }
                }
                if (j.has("description")) {
                    desc = j.get("description").asString();
                }
                if (j.has("status")) {
                    let s = j.get("status").asString();
                    if (s != "") { status = s; }
                }
                if (j.has("priority")) {
                    let p = j.get("priority").asString();
                    if (p != "") { priority = p; }
                }
                j.destroy();
            }
        }

        let pg = DatabaseManager.getCloudConn();
        if (pg.isConnected()) {
            let sql = "INSERT INTO items (title, description, status, priority, created_at) VALUES ('" + escapeJson(title) + "', '" + escapeJson(desc) + "', '" + escapeJson(status) + "', '" + escapeJson(priority) + "', NOW()) RETURNING id;";
            let insRes = pg.query(sql);
            let newId = "0";
            if (insRes.isOk() && insRes.rowCount() > 0) {
                newId = insRes.getValue(0, 0);
            }
            if (newId == "") {
                newId = "0";
            }
            insRes.free();
            pg.close();

            let resJson = "{\"id\":" + newId + ",\"title\":\"" + escapeJson(title) + "\",\"description\":\"" + escapeJson(desc) + "\",\"status\":\"" + escapeJson(status) + "\",\"priority\":\"" + escapeJson(priority) + "\",\"message\":\"Item created in Neon Cloud PostgreSQL successfully\"}";
            return ctx.jsonStatus(resJson, 201);
        }

        // Fallback SQLite
        let db = Database.openSqlite("items.db", 5000, true);
        let sql = "INSERT INTO items (title, description, status, priority, created_at) VALUES ('" + escapeJson(title) + "', '" + escapeJson(desc) + "', '" + escapeJson(status) + "', '" + escapeJson(priority) + "', datetime('now'));";
        db.execute(sql);
        let newId = db.lastInsertRowId();
        db.close();

        let resJson = "{\"id\":" + newId + ",\"title\":\"" + escapeJson(title) + "\",\"description\":\"" + escapeJson(desc) + "\",\"status\":\"" + escapeJson(status) + "\",\"priority\":\"" + escapeJson(priority) + "\",\"message\":\"Item created successfully\"}";
        return ctx.jsonStatus(resJson, 201);
    }

    // [PUT] /api/items/:id — Update existing item (Protected with API Key)
    public static fn updateItem(Context ctx) -> HttpResponse {
        if (!AuthMiddleware.validateKey(ctx)) {
            return AuthMiddleware.unauthorizedResponse(ctx);
        }

        let idStr = ctx.param("id");
        let targetId = int.parse(idStr);
        let body = ctx.body();

        let pg = DatabaseManager.getCloudConn();
        if (pg.isConnected()) {
            let chk = pg.query("SELECT id FROM items WHERE id = " + targetId + ";");
            let exists = chk.isOk() && chk.rowCount() > 0;
            chk.free();

            if (!exists) {
                pg.close();
                return ctx.jsonStatus("{\"error\":\"Item not found\",\"id\":" + targetId + "}", 404);
            }

            if (body != "") {
                let j = Json.parse(body);
                if (j.isValid()) {
                    if (j.has("title")) {
                        let title = j.get("title").asString();
                        if (title != "") {
                            let u = pg.query("UPDATE items SET title = '" + escapeJson(title) + "' WHERE id = " + targetId + ";");
                            u.free();
                        }
                    }
                    if (j.has("description")) {
                        let desc = j.get("description").asString();
                        let u = pg.query("UPDATE items SET description = '" + escapeJson(desc) + "' WHERE id = " + targetId + ";");
                        u.free();
                    }
                    if (j.has("status")) {
                        let status = j.get("status").asString();
                        if (status != "") {
                            let u = pg.query("UPDATE items SET status = '" + escapeJson(status) + "' WHERE id = " + targetId + ";");
                            u.free();
                        }
                    }
                    if (j.has("priority")) {
                        let priority = j.get("priority").asString();
                        if (priority != "") {
                            let u = pg.query("UPDATE items SET priority = '" + escapeJson(priority) + "' WHERE id = " + targetId + ";");
                            u.free();
                        }
                    }
                    j.destroy();
                }
            }
            pg.close();

            let resJson = "{\"id\":" + targetId + ",\"message\":\"Item updated in Neon Cloud PostgreSQL successfully\"}";
            return ctx.json(resJson);
        }

        // Fallback SQLite
        let db = Database.openSqlite("items.db", 5000, true);
        let checkStmt = db.prepare("SELECT id FROM items WHERE id = " + targetId + ";");
        let exists = checkStmt.next();
        checkStmt.finalize();

        if (!exists) {
            db.close();
            return ctx.jsonStatus("{\"error\":\"Item not found\",\"id\":" + targetId + "}", 404);
        }

        if (body != "") {
            let j = Json.parse(body);
            if (j.isValid()) {
                if (j.has("title")) {
                    let title = j.get("title").asString();
                    if (title != "") {
                        db.execute("UPDATE items SET title = '" + escapeJson(title) + "' WHERE id = " + targetId + ";");
                    }
                }
                if (j.has("description")) {
                    let desc = j.get("description").asString();
                    db.execute("UPDATE items SET description = '" + escapeJson(desc) + "' WHERE id = " + targetId + ";");
                }
                if (j.has("status")) {
                    let status = j.get("status").asString();
                    if (status != "") {
                        db.execute("UPDATE items SET status = '" + escapeJson(status) + "' WHERE id = " + targetId + ";");
                    }
                }
                if (j.has("priority")) {
                    let priority = j.get("priority").asString();
                    if (priority != "") {
                        db.execute("UPDATE items SET priority = '" + escapeJson(priority) + "' WHERE id = " + targetId + ";");
                    }
                }
                j.destroy();
            }
        }
        db.close();

        let resJson = "{\"id\":" + targetId + ",\"message\":\"Item updated successfully\"}";
        return ctx.json(resJson);
    }

    // [DELETE] /api/items/:id — Delete item (Protected with API Key)
    public static fn deleteItem(Context ctx) -> HttpResponse {
        if (!AuthMiddleware.validateKey(ctx)) {
            return AuthMiddleware.unauthorizedResponse(ctx);
        }

        let idStr = ctx.param("id");
        let targetId = int.parse(idStr);

        let pg = DatabaseManager.getCloudConn();
        if (pg.isConnected()) {
            let delRes = pg.query("DELETE FROM items WHERE id = " + targetId + " RETURNING id;");
            let deleted = delRes.isOk() && delRes.rowCount() > 0;
            delRes.free();
            pg.close();

            if (!deleted) {
                return ctx.jsonStatus("{\"error\":\"Item not found\",\"id\":" + targetId + "}", 404);
            }

            let resJson = "{\"success\":true,\"deleted_id\":" + targetId + ",\"message\":\"Item deleted from Neon Cloud PostgreSQL\"}";
            return ctx.json(resJson);
        }

        // Fallback SQLite
        let db = Database.openSqlite("items.db", 5000, true);
        let checkStmt = db.prepare("SELECT id FROM items WHERE id = " + targetId + ";");
        let exists = checkStmt.next();
        checkStmt.finalize();

        if (!exists) {
            db.close();
            return ctx.jsonStatus("{\"error\":\"Item not found\",\"id\":" + targetId + "}", 404);
        }

        db.execute("DELETE FROM items WHERE id = " + targetId + ";");
        db.close();

        let resJson = "{\"success\":true,\"deleted_id\":" + targetId + ",\"message\":\"Item deleted successfully\"}";
        return ctx.json(resJson);
    }
}
