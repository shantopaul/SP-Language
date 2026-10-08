module std.db.pool;

import std.db.sqlite;
import std.db.postgres;

// ============================================================================
// Standard Library Generic Thread-Safe Database Connection Pool (SP 2.5)
// ============================================================================

extern "C" {
    fn sp_rt_db_pool_create_sqlite(string path, int64 minConn, int64 maxConn, int64 acquireTimeoutMs) -> ptr;
    fn sp_rt_db_pool_create_pg(string connStr, int64 minConn, int64 maxConn, int64 acquireTimeoutMs) -> ptr;
    fn sp_rt_db_pool_acquire(ptr pool) -> ptr;
    fn sp_rt_db_pool_release(ptr pool, ptr conn);
    fn sp_rt_db_pool_invalidate(ptr pool, ptr conn);
    fn sp_rt_db_pool_close(ptr pool);
    fn sp_rt_db_pool_total(ptr pool) -> int64;
    fn sp_rt_db_pool_available(ptr pool) -> int64;
    fn sp_rt_db_pool_in_use(ptr pool) -> int64;
}

public class DbPool {
    public ptr handle;
    public string driver;

    public static fn sqlite(string path, int minConn = 1, int maxConn = 10, int timeoutMs = 5000) -> DbPool {
        let pool = new DbPool();
        pool.driver = "sqlite";
        pool.handle = sp_rt_db_pool_create_sqlite(path, minConn, maxConn, timeoutMs);
        return pool;
    }

    public static fn postgres(string connStr, int minConn = 1, int maxConn = 10, int timeoutMs = 5000) -> DbPool {
        let pool = new DbPool();
        pool.driver = "postgres";
        pool.handle = sp_rt_db_pool_create_pg(connStr, minConn, maxConn, timeoutMs);
        return pool;
    }

    public fn acquireSqlite() -> SqliteConnection {
        let conn = new SqliteConnection();
        if (this.handle != null) {
            conn.handle = sp_rt_db_pool_acquire(this.handle);
        }
        return conn;
    }

    public fn releaseSqlite(SqliteConnection conn) {
        if (this.handle != null) {
            if (conn.handle != null) {
                sp_rt_db_pool_release(this.handle, conn.handle);
                conn.handle = null; // Detach handle so caller cannot reuse directly
            }
        }
    }

    public fn acquirePostgres() -> PgConnection {
        let conn = new PgConnection();
        if (this.handle != null) {
            conn.handle = sp_rt_db_pool_acquire(this.handle);
        }
        return conn;
    }

    public fn releasePostgres(PgConnection conn) {
        if (this.handle != null) {
            if (conn.handle != null) {
                sp_rt_db_pool_release(this.handle, conn.handle);
                conn.handle = null;
            }
        }
    }

    public fn total() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_db_pool_total(this.handle);
    }

    public fn available() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_db_pool_available(this.handle);
    }

    public fn inUse() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_db_pool_in_use(this.handle);
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_db_pool_close(this.handle);
            this.handle = null;
        }
    }
}
