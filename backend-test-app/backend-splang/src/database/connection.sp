module database.connection;

import std.io;
import std.db;
import std.db.postgres;
import config.env;

// ============================================================================
// Database Connection & Schema Lifecycle Manager
// Seamlessly routes queries between Neon Cloud PostgreSQL & Local SQLite.
// ============================================================================

public class DatabaseManager {
    public static fn getCloudConn() -> PgConnection {
        return PgConnection.connect(Env.databaseUrl());
    }

    public static fn initSchema() {
        println("[+] Initializing database schemas...");
        let pg = DatabaseManager.getCloudConn();
        if (pg.isConnected()) {
            println("[✓] Connected to Neon Cloud PostgreSQL (AWS US-East-2)");
            let createRes = pg.query("CREATE TABLE IF NOT EXISTS items (id SERIAL PRIMARY KEY, title TEXT NOT NULL, description TEXT, status TEXT DEFAULT 'pending', priority TEXT DEFAULT 'normal', created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
            createRes.free();

            let countRes = pg.query("SELECT COUNT(*) FROM items;");
            let count = 0;
            if (countRes.isOk() && countRes.rowCount() > 0) {
                count = int.parse(countRes.getValue(0, 0));
            }
            countRes.free();

            if (count == 0) {
                let s1 = pg.query("INSERT INTO items (title, description, status, priority) VALUES ('Design Fullstack Architecture', 'Establish SP Lang native backend with Next.js App Router frontend', 'completed', 'high');");
                s1.free();
                let s2 = pg.query("INSERT INTO items (title, description, status, priority) VALUES ('Connect Neon Cloud PostgreSQL', 'Configure database pooling, connection strings, and fallback storage', 'completed', 'high');");
                s2.free();
                let s3 = pg.query("INSERT INTO items (title, description, status, priority) VALUES ('Implement Interactive Dashboard', 'Build responsive Next.js Tailwind UI with real-time CRUD actions', 'completed', 'normal');");
                s3.free();
                println("[+] Seeded initial demo tasks into Neon Cloud PostgreSQL.");
            }
            pg.close();
        } else {
            println("[!] Cloud unavailable, falling back to local SQLite storage...");
            let db = Database.openSqlite("items.db", 5000, true);
            db.execute("CREATE TABLE IF NOT EXISTS items (id INTEGER PRIMARY KEY AUTOINCREMENT, title TEXT NOT NULL, description TEXT, status TEXT DEFAULT 'pending', priority TEXT DEFAULT 'normal', created_at TEXT DEFAULT CURRENT_TIMESTAMP);");
            db.close();
        }
    }
}
