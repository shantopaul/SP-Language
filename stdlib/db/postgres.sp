module std.db.postgres;

// ============================================================================
// Standard Library PostgreSQL Wire Protocol v3.0 Native Driver (SP 2.5)
// ============================================================================

extern "C" {
    fn sp_rt_pg_connect(string connStr) -> ptr;
    fn sp_rt_pg_close(ptr conn);
    fn sp_rt_pg_is_connected(ptr conn) -> bool;
    fn sp_rt_pg_last_error(ptr conn) -> string;
    fn sp_rt_pg_query(ptr conn, string sql) -> ptr;
    fn sp_rt_pg_ping(ptr conn) -> bool;
    fn sp_rt_pg_begin(ptr conn) -> bool;
    fn sp_rt_pg_commit(ptr conn) -> bool;
    fn sp_rt_pg_rollback(ptr conn) -> bool;

    fn sp_rt_pg_res_is_ok(ptr res) -> bool;
    fn sp_rt_pg_res_error(ptr res) -> string;
    fn sp_rt_pg_res_command_tag(ptr res) -> string;
    fn sp_rt_pg_res_affected_rows(ptr res) -> int64;
    fn sp_rt_pg_res_row_count(ptr res) -> int64;
    fn sp_rt_pg_res_col_count(ptr res) -> int64;
    fn sp_rt_pg_res_col_name(ptr res, int64 col) -> string;
    fn sp_rt_pg_res_get_value(ptr res, int64 row, int64 col) -> string;
    fn sp_rt_pg_res_is_null(ptr res, int64 row, int64 col) -> bool;
    fn sp_rt_pg_res_free(ptr res);
}

public class PgResult {
    public ptr handle;

    public fn isValid() -> bool {
        return this.handle != null;
    }

    public fn isOk() -> bool {
        if (this.handle == null) return false;
        return sp_rt_pg_res_is_ok(this.handle);
    }

    public fn error() -> string {
        if (this.handle == null) return "Invalid result handle";
        return sp_rt_pg_res_error(this.handle);
    }

    public fn commandTag() -> string {
        if (this.handle == null) return "";
        return sp_rt_pg_res_command_tag(this.handle);
    }

    public fn affectedRows() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_pg_res_affected_rows(this.handle);
    }

    public fn rowCount() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_pg_res_row_count(this.handle);
    }

    public fn columnCount() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_pg_res_col_count(this.handle);
    }

    public fn columnName(int64 col) -> string {
        if (this.handle == null) return "";
        return sp_rt_pg_res_col_name(this.handle, col);
    }

    public fn getValue(int64 row, int64 col) -> string {
        if (this.handle == null) return "";
        return sp_rt_pg_res_get_value(this.handle, row, col);
    }

    public fn isNull(int64 row, int64 col) -> bool {
        if (this.handle == null) return true;
        return sp_rt_pg_res_is_null(this.handle, row, col);
    }

    public fn free() {
        if (this.handle != null) {
            sp_rt_pg_res_free(this.handle);
            this.handle = null;
        }
    }
}

public class PgConnection {
    public ptr handle;

    public static fn connect(string connStr) -> PgConnection {
        let conn = new PgConnection();
        conn.handle = sp_rt_pg_connect(connStr);
        return conn;
    }

    public fn isValid() -> bool {
        return this.handle != null;
    }

    public fn isConnected() -> bool {
        if (this.handle == null) return false;
        return sp_rt_pg_is_connected(this.handle);
    }

    public fn lastError() -> string {
        if (this.handle == null) return "PostgreSQL not connected";
        return sp_rt_pg_last_error(this.handle);
    }

    public fn query(string sql) -> PgResult {
        let res = new PgResult();
        if (this.handle != null) {
            res.handle = sp_rt_pg_query(this.handle, sql);
        }
        return res;
    }

    public fn ping() -> bool {
        if (this.handle == null) return false;
        return sp_rt_pg_ping(this.handle);
    }

    public fn beginTransaction() -> bool {
        if (this.handle == null) return false;
        return sp_rt_pg_begin(this.handle);
    }

    public fn commit() -> bool {
        if (this.handle == null) return false;
        return sp_rt_pg_commit(this.handle);
    }

    public fn rollback() -> bool {
        if (this.handle == null) return false;
        return sp_rt_pg_rollback(this.handle);
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_pg_close(this.handle);
            this.handle = null;
        }
    }
}
