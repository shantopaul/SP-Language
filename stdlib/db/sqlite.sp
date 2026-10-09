module std.db.sqlite;

// ============================================================================
// Standard Library SQLite3 Native Driver (SP 2.5)
// ============================================================================

extern "C" {
    fn sp_rt_sqlite_open(string path, int64 busyTimeoutMs, bool walMode) -> ptr;
    fn sp_rt_sqlite_close(ptr db);
    fn sp_rt_sqlite_exec(ptr db, string sql) -> int64;
    fn sp_rt_sqlite_errmsg(ptr db) -> string;
    fn sp_rt_sqlite_last_insert_rowid(ptr db) -> int64;
    fn sp_rt_sqlite_changes(ptr db) -> int64;
    fn sp_rt_sqlite_begin(ptr db) -> bool;
    fn sp_rt_sqlite_commit(ptr db) -> bool;
    fn sp_rt_sqlite_rollback(ptr db) -> bool;

    fn sp_rt_sqlite_prepare(ptr db, string sql) -> ptr;
    fn sp_rt_sqlite_bind_int(ptr stmt, int64 index, int64 val) -> bool;
    fn sp_rt_sqlite_bind_double(ptr stmt, int64 index, double val) -> bool;
    fn sp_rt_sqlite_bind_string(ptr stmt, int64 index, string val) -> bool;
    fn sp_rt_sqlite_bind_null(ptr stmt, int64 index) -> bool;
    fn sp_rt_sqlite_param_index(ptr stmt, string name) -> int64;
    fn sp_rt_sqlite_bind_named_int(ptr stmt, string name, int64 val) -> bool;
    fn sp_rt_sqlite_bind_named_double(ptr stmt, string name, double val) -> bool;
    fn sp_rt_sqlite_bind_named_string(ptr stmt, string name, string val) -> bool;
    fn sp_rt_sqlite_bind_named_null(ptr stmt, string name) -> bool;
    fn sp_rt_sqlite_step(ptr stmt) -> int64;
    fn sp_rt_sqlite_col_count(ptr stmt) -> int64;
    fn sp_rt_sqlite_col_name(ptr stmt, int64 index) -> string;
    fn sp_rt_sqlite_col_type(ptr stmt, int64 index) -> int64;
    fn sp_rt_sqlite_col_int(ptr stmt, int64 index) -> int64;
    fn sp_rt_sqlite_col_double(ptr stmt, int64 index) -> double;
    fn sp_rt_sqlite_col_string(ptr stmt, int64 index) -> string;
    fn sp_rt_sqlite_col_is_null(ptr stmt, int64 index) -> bool;
    fn sp_rt_sqlite_reset(ptr stmt) -> bool;
    fn sp_rt_sqlite_clear_bindings(ptr stmt) -> bool;
    fn sp_rt_sqlite_finalize(ptr stmt);
}

public class SqliteStatement {
    public ptr handle;

    public fn isValid() -> bool {
        return this.handle != null;
    }

    public fn bindInt(int index, int64 val) -> bool {
        return sp_rt_sqlite_bind_int(this.handle, index, val);
    }

    public fn bindDouble(int index, double val) -> bool {
        return sp_rt_sqlite_bind_double(this.handle, index, val);
    }

    public fn bindString(int index, string val) -> bool {
        return sp_rt_sqlite_bind_string(this.handle, index, val);
    }

    public fn bindNull(int index) -> bool {
        return sp_rt_sqlite_bind_null(this.handle, index);
    }

    public fn parameterIndex(string name) -> int64 {
        return sp_rt_sqlite_param_index(this.handle, name);
    }

    public fn bindNamedInt(string name, int64 val) -> bool {
        return sp_rt_sqlite_bind_named_int(this.handle, name, val);
    }

    public fn bindNamedDouble(string name, double val) -> bool {
        return sp_rt_sqlite_bind_named_double(this.handle, name, val);
    }

    public fn bindNamedString(string name, string val) -> bool {
        return sp_rt_sqlite_bind_named_string(this.handle, name, val);
    }

    public fn bindNamedNull(string name) -> bool {
        return sp_rt_sqlite_bind_named_null(this.handle, name);
    }

    public fn step() -> int64 {
        return sp_rt_sqlite_step(this.handle);
    }

    public fn next() -> bool {
        // 100 corresponds to SQLITE_ROW
        return sp_rt_sqlite_step(this.handle) == 100;
    }

    public fn columnCount() -> int64 {
        return sp_rt_sqlite_col_count(this.handle);
    }

    public fn columnName(int index) -> string {
        return sp_rt_sqlite_col_name(this.handle, index);
    }

    public fn columnType(int index) -> int64 {
        return sp_rt_sqlite_col_type(this.handle, index);
    }

    public fn getInt(int index) -> int64 {
        return sp_rt_sqlite_col_int(this.handle, index);
    }

    public fn getDouble(int index) -> double {
        return sp_rt_sqlite_col_double(this.handle, index);
    }

    public fn getString(int index) -> string {
        return sp_rt_sqlite_col_string(this.handle, index);
    }

    public fn isNull(int index) -> bool {
        return sp_rt_sqlite_col_is_null(this.handle, index);
    }

    public fn reset() -> bool {
        return sp_rt_sqlite_reset(this.handle);
    }

    public fn clear() -> bool {
        return sp_rt_sqlite_clear_bindings(this.handle);
    }

    public fn finalize() {
        if (this.handle != null) {
            sp_rt_sqlite_finalize(this.handle);
            this.handle = null;
        }
    }
}

public class SqliteConnection {
    public ptr handle;

    public static fn open(string path, int busyTimeoutMs = 5000, bool walMode = true) -> SqliteConnection {
        let conn = new SqliteConnection();
        conn.handle = sp_rt_sqlite_open(path, busyTimeoutMs, walMode);
        return conn;
    }

    public static fn inMemory() -> SqliteConnection {
        return SqliteConnection.open(":memory:", 5000, false);
    }

    public fn isOpen() -> bool {
        return this.handle != null;
    }

    public fn execute(string sql) -> int64 {
        if (this.handle == null) return -1;
        return sp_rt_sqlite_exec(this.handle, sql);
    }

    public fn prepare(string sql) -> SqliteStatement {
        let stmt = new SqliteStatement();
        if (this.handle != null) {
            stmt.handle = sp_rt_sqlite_prepare(this.handle, sql);
        }
        return stmt;
    }

    public fn lastInsertRowId() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_sqlite_last_insert_rowid(this.handle);
    }

    public fn changes() -> int64 {
        if (this.handle == null) return 0;
        return sp_rt_sqlite_changes(this.handle);
    }

    public fn beginTransaction() -> bool {
        if (this.handle == null) return false;
        return sp_rt_sqlite_begin(this.handle);
    }

    public fn commit() -> bool {
        if (this.handle == null) return false;
        return sp_rt_sqlite_commit(this.handle);
    }

    public fn rollback() -> bool {
        if (this.handle == null) return false;
        return sp_rt_sqlite_rollback(this.handle);
    }

    public fn lastError() -> string {
        if (this.handle == null) return "Database not open";
        return sp_rt_sqlite_errmsg(this.handle);
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_sqlite_close(this.handle);
            this.handle = null;
        }
    }
}
