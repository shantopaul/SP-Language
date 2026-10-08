module std.db;

import std.db.sqlite;
import std.db.postgres;
import std.db.pool;
import std.db.sql;
import std.db.transaction;

// ============================================================================
// Standard Library Database System Facade (SP 2.5)
// ============================================================================

public class Database {
    public static fn openSqlite(string path, int busyTimeoutMs = 5000, bool walMode = true) -> SqliteConnection {
        return SqliteConnection.open(path, busyTimeoutMs, walMode);
    }

    public static fn memorySqlite() -> SqliteConnection {
        return SqliteConnection.inMemory();
    }

    public static fn connectPostgres(string connStr) -> PgConnection {
        return PgConnection.connect(connStr);
    }

    public static fn sqlitePool(string path, int minConn = 1, int maxConn = 10, int timeoutMs = 5000) -> DbPool {
        return DbPool.sqlite(path, minConn, maxConn, timeoutMs);
    }

    public static fn postgresPool(string connStr, int minConn = 1, int maxConn = 10, int timeoutMs = 5000) -> DbPool {
        return DbPool.postgres(connStr, minConn, maxConn, timeoutMs);
    }

    public static fn query(string table) -> QueryBuilder {
        return QueryBuilder.from(table);
    }
}
