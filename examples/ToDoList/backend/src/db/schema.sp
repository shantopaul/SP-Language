module db.schema;

import std.db;
import std.time;
import config.env;
import util.logger;

public class DatabaseManager {
    /// Open a configured SQLite connection with WAL mode and foreign keys enabled.
    public static fn open() -> SqliteConnection {
        Env.ensureDataDir();
        let db = Database.openSqlite(Env.dbPath(), 5000, true);
        if (db.isOpen()) {
            db.execute("PRAGMA foreign_keys = ON;");
        }
        return db;
    }

    /// Return current UTC timestamp in ISO-8601 format (YYYY-MM-DDTHH:MM:SSZ).
    public static fn utcNow(SqliteConnection db) -> string {
        if (db.isOpen()) {
            let stmt = db.prepare("SELECT strftime('%Y-%m-%dT%H:%M:%SZ', 'now');");
            if (stmt.isValid()) {
                if (stmt.next()) {
                    let ts = stmt.getString(0);
                    stmt.finalize();
                    if (ts.length() > 0) {
                        return ts;
                    }
                }
                stmt.finalize();
            }
        }
        return Time.format(Time.now());
    }

    /// Return current UTC calendar date (YYYY-MM-DD).
    public static fn utcToday(SqliteConnection db) -> string {
        if (db.isOpen()) {
            let stmt = db.prepare("SELECT strftime('%Y-%m-%d', 'now');");
            if (stmt.isValid()) {
                if (stmt.next()) {
                    let d = stmt.getString(0);
                    stmt.finalize();
                    if (d.length() > 0) {
                        return d;
                    }
                }
                stmt.finalize();
            }
        }
        return "";
    }

    /// Verify database connectivity for readiness checks.
    public static fn ping() -> bool {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            return false;
        }
        let stmt = db.prepare("SELECT 1;");
        if (!stmt.isValid()) {
            db.close();
            return false;
        }
        let ok = stmt.next();
        stmt.finalize();
        db.close();
        return ok;
    }

    /// Idempotently initialize tables, foreign keys, and indexes.
    public static fn initSchema() -> bool {
        let db = DatabaseManager.open();
        if (!db.isOpen()) {
            AppLog.error("db.schema", "Failed to open SQLite database at " + Env.dbPath());
            return false;
        }

        let migrationsSql = "CREATE TABLE IF NOT EXISTS schema_migrations ("
            + "version INTEGER PRIMARY KEY, "
            + "applied_at TEXT NOT NULL"
            + ");";
        if (db.execute(migrationsSql) < 0) {
            AppLog.error("db.schema", "Failed to create schema_migrations: " + db.lastError());
            db.close();
            return false;
        }

        let usersSql = "CREATE TABLE IF NOT EXISTS users ("
            + "id TEXT PRIMARY KEY, "
            + "email TEXT NOT NULL UNIQUE, "
            + "name TEXT NOT NULL, "
            + "password_hash TEXT NOT NULL, "
            + "created_at TEXT NOT NULL, "
            + "updated_at TEXT NOT NULL"
            + ");";
        if (db.execute(usersSql) < 0) {
            AppLog.error("db.schema", "Failed to create users table: " + db.lastError());
            db.close();
            return false;
        }

        let categoriesSql = "CREATE TABLE IF NOT EXISTS categories ("
            + "id TEXT PRIMARY KEY, "
            + "user_id TEXT NOT NULL, "
            + "name TEXT NOT NULL, "
            + "color TEXT NOT NULL DEFAULT '#3B82F6', "
            + "sort_order INTEGER NOT NULL DEFAULT 0, "
            + "created_at TEXT NOT NULL, "
            + "updated_at TEXT NOT NULL, "
            + "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
            + "UNIQUE (user_id, name)"
            + ");";
        if (db.execute(categoriesSql) < 0) {
            AppLog.error("db.schema", "Failed to create categories table: " + db.lastError());
            db.close();
            return false;
        }

        let todosSql = "CREATE TABLE IF NOT EXISTS todos ("
            + "id TEXT PRIMARY KEY, "
            + "user_id TEXT NOT NULL, "
            + "category_id TEXT NULL, "
            + "title TEXT NOT NULL, "
            + "description TEXT NOT NULL DEFAULT '', "
            + "status TEXT NOT NULL DEFAULT 'todo', "
            + "priority TEXT NOT NULL DEFAULT 'medium', "
            + "due_date TEXT NULL, "
            + "completed_at TEXT NULL, "
            + "sort_order INTEGER NOT NULL DEFAULT 0, "
            + "created_at TEXT NOT NULL, "
            + "updated_at TEXT NOT NULL, "
            + "FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE, "
            + "FOREIGN KEY (category_id) REFERENCES categories(id) ON DELETE SET NULL"
            + ");";
        if (db.execute(todosSql) < 0) {
            AppLog.error("db.schema", "Failed to create todos table: " + db.lastError());
            db.close();
            return false;
        }

        db.execute("CREATE INDEX IF NOT EXISTS idx_categories_user_sort ON categories(user_id, sort_order, name);");
        db.execute("CREATE INDEX IF NOT EXISTS idx_todos_user_status ON todos(user_id, status);");
        db.execute("CREATE INDEX IF NOT EXISTS idx_todos_user_priority ON todos(user_id, priority);");
        db.execute("CREATE INDEX IF NOT EXISTS idx_todos_user_due ON todos(user_id, due_date);");
        db.execute("CREATE INDEX IF NOT EXISTS idx_todos_category ON todos(category_id);");

        let now = DatabaseManager.utcNow(db);
        let migStmt = db.prepare("INSERT OR IGNORE INTO schema_migrations (version, applied_at) VALUES (?, ?);");
        if (migStmt.isValid()) {
            migStmt.bindInt(1, 1);
            migStmt.bindString(2, now);
            migStmt.step();
            migStmt.finalize();
        }

        db.close();
        return true;
    }
}
