#include "sp/runtime/Runtime.h"
#include "sp/runtime/Sqlite.h"
#include "sp/runtime/Postgres.h"
#include "sp/runtime/DbPool.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <cstdlib>

using namespace sp::runtime;

namespace {
const char* alloc_str(const std::string& s) {
    char* copy = static_cast<char*>(malloc(s.size() + 1));
    if (copy) {
        std::memcpy(copy, s.c_str(), s.size() + 1);
    }
    return copy;
}
} // namespace

extern "C" {

// ============================================================================
// SQLite C-ABI Bindings
// ============================================================================

void* sp_rt_sqlite_open(const char* path, int64_t busy_timeout_ms, bool wal_mode) {
    if (!path) return nullptr;
    auto db = new SqliteDatabase();
    if (!db->open(path, static_cast<int>(busy_timeout_ms), wal_mode)) {
        delete db;
        return nullptr;
    }
    return db;
}

void sp_rt_sqlite_close(void* db) {
    if (!db) return;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    sqlite->close();
    delete sqlite;
}

int64_t sp_rt_sqlite_exec(void* db, const char* sql) {
    if (!db || !sql) return -1;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    return sqlite->execute(sql);
}

const char* sp_rt_sqlite_errmsg(void* db) {
    if (!db) return "Invalid database handle";
    auto sqlite = static_cast<SqliteDatabase*>(db);
    static thread_local std::string err;
    err = sqlite->lastError();
    return err.c_str();
}

int64_t sp_rt_sqlite_last_insert_rowid(void* db) {
    if (!db) return 0;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    return sqlite->lastInsertRowId();
}

int64_t sp_rt_sqlite_changes(void* db) {
    if (!db) return 0;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    return sqlite->changes();
}

bool sp_rt_sqlite_begin(void* db) {
    if (!db) return false;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    return sqlite->beginTransaction();
}

bool sp_rt_sqlite_commit(void* db) {
    if (!db) return false;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    return sqlite->commit();
}

bool sp_rt_sqlite_rollback(void* db) {
    if (!db) return false;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    return sqlite->rollback();
}

void* sp_rt_sqlite_prepare(void* db, const char* sql) {
    if (!db || !sql) return nullptr;
    auto sqlite = static_cast<SqliteDatabase*>(db);
    auto stmt = sqlite->prepare(sql);
    if (!stmt) return nullptr;
    return new std::shared_ptr<SqliteStmt>(stmt);
}

bool sp_rt_sqlite_bind_int(void* stmt, int64_t index, int64_t val) {
    if (!stmt) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindInt(static_cast<int>(index), val);
}

bool sp_rt_sqlite_bind_double(void* stmt, int64_t index, double val) {
    if (!stmt) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindDouble(static_cast<int>(index), val);
}

bool sp_rt_sqlite_bind_string(void* stmt, int64_t index, const char* val) {
    if (!stmt || !val) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindString(static_cast<int>(index), val);
}

bool sp_rt_sqlite_bind_null(void* stmt, int64_t index) {
    if (!stmt) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindNull(static_cast<int>(index));
}

int64_t sp_rt_sqlite_param_index(void* stmt, const char* name) {
    if (!stmt || !name) return 0;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s ? s->parameterIndex(name) : 0;
}

bool sp_rt_sqlite_bind_named_int(void* stmt, const char* name, int64_t val) {
    if (!stmt || !name) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindNamedInt(name, val);
}

bool sp_rt_sqlite_bind_named_double(void* stmt, const char* name, double val) {
    if (!stmt || !name) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindNamedDouble(name, val);
}

bool sp_rt_sqlite_bind_named_string(void* stmt, const char* name, const char* val) {
    if (!stmt || !name || !val) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindNamedString(name, val);
}

bool sp_rt_sqlite_bind_named_null(void* stmt, const char* name) {
    if (!stmt || !name) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->bindNamedNull(name);
}

int64_t sp_rt_sqlite_step(void* stmt) {
    if (!stmt) return -1;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s ? s->step() : -1;
}

int64_t sp_rt_sqlite_col_count(void* stmt) {
    if (!stmt) return 0;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s ? s->columnCount() : 0;
}

const char* sp_rt_sqlite_col_name(void* stmt, int64_t index) {
    if (!stmt) return "";
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    if (!s) return "";
    static thread_local std::string name;
    name = s->columnName(static_cast<int>(index));
    return name.c_str();
}

int64_t sp_rt_sqlite_col_type(void* stmt, int64_t index) {
    if (!stmt) return 5; // Null
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s ? static_cast<int64_t>(s->columnType(static_cast<int>(index))) : 5;
}

int64_t sp_rt_sqlite_col_int(void* stmt, int64_t index) {
    if (!stmt) return 0;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s ? s->columnInt(static_cast<int>(index)) : 0;
}

double sp_rt_sqlite_col_double(void* stmt, int64_t index) {
    if (!stmt) return 0.0;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s ? s->columnDouble(static_cast<int>(index)) : 0.0;
}

const char* sp_rt_sqlite_col_string(void* stmt, int64_t index) {
    if (!stmt) return "";
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    if (!s) return "";
    static thread_local std::string str;
    str = s->columnString(static_cast<int>(index));
    return str.c_str();
}

bool sp_rt_sqlite_col_is_null(void* stmt, int64_t index) {
    if (!stmt) return true;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s ? s->columnIsNull(static_cast<int>(index)) : true;
}

bool sp_rt_sqlite_reset(void* stmt) {
    if (!stmt) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->reset();
}

bool sp_rt_sqlite_clear_bindings(void* stmt) {
    if (!stmt) return false;
    auto s = *static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    return s && s->clearBindings();
}

void sp_rt_sqlite_finalize(void* stmt) {
    if (!stmt) return;
    auto p = static_cast<std::shared_ptr<SqliteStmt>*>(stmt);
    delete p;
}

// ============================================================================
// PostgreSQL C-ABI Bindings
// ============================================================================

static thread_local std::string g_lastPgConnectError;

void* sp_rt_pg_connect(const char* conn_str) {
    if (!conn_str) return nullptr;
    auto conn = new PgConnection();
    if (!conn->connect(conn_str)) {
        g_lastPgConnectError = conn->lastError();
        // Honest failure: return nullptr instead of a non-null handle to
        // a dead connection. The reason stays retrievable via
        // sp_rt_pg_last_error(nullptr); callers must check the handle
        // (stdlib PgConnection::isValid()/isConnected() already do).
        delete conn;
        return nullptr;
    }
    return conn;
}

void sp_rt_pg_close(void* conn) {
    if (!conn) return;
    auto pg = static_cast<PgConnection*>(conn);
    pg->close();
    delete pg;
}

bool sp_rt_pg_is_connected(void* conn) {
    if (!conn) return false;
    return static_cast<PgConnection*>(conn)->isConnected();
}

const char* sp_rt_pg_last_error(void* conn) {
    if (!conn) {
        if (!g_lastPgConnectError.empty()) return g_lastPgConnectError.c_str();
        return "Invalid connection handle";
    }
    static thread_local std::string err;
    err = static_cast<PgConnection*>(conn)->lastError();
    return err.c_str();
}

void* sp_rt_pg_query(void* conn, const char* sql) {
    if (!conn || !sql) return nullptr;
    auto pg = static_cast<PgConnection*>(conn);
    auto res = pg->query(sql);
    if (!res) return nullptr;
    return new std::shared_ptr<PgResult>(res);
}

void* sp_rt_pg_query_params(void* conn, const char* sql, const char** params, int64_t param_count) {
    if (!conn || !sql) return nullptr;
    auto pg = static_cast<PgConnection*>(conn);
    std::vector<std::string> pList;
    if (params && param_count > 0) {
        pList.reserve(param_count);
        for (int64_t i = 0; i < param_count; ++i) {
            pList.emplace_back(params[i] ? params[i] : "");
        }
    }
    auto res = pg->queryParams(sql, pList);
    if (!res) return nullptr;
    return new std::shared_ptr<PgResult>(res);
}

bool sp_rt_pg_ping(void* conn) {
    if (!conn) return false;
    return static_cast<PgConnection*>(conn)->ping();
}

bool sp_rt_pg_begin(void* conn) {
    if (!conn) return false;
    return static_cast<PgConnection*>(conn)->beginTransaction();
}

bool sp_rt_pg_commit(void* conn) {
    if (!conn) return false;
    return static_cast<PgConnection*>(conn)->commit();
}

bool sp_rt_pg_rollback(void* conn) {
    if (!conn) return false;
    return static_cast<PgConnection*>(conn)->rollback();
}

bool sp_rt_pg_res_is_ok(void* res) {
    if (!res) return false;
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    return r && r->isOk();
}

const char* sp_rt_pg_res_error(void* res) {
    if (!res) return "Invalid result handle";
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    if (!r) return "Null result";
    static thread_local std::string err;
    err = r->errorMessage();
    return err.c_str();
}

const char* sp_rt_pg_res_command_tag(void* res) {
    if (!res) return "";
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    if (!r) return "";
    static thread_local std::string tag;
    tag = r->commandTag();
    return tag.c_str();
}

int64_t sp_rt_pg_res_affected_rows(void* res) {
    if (!res) return 0;
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    return r ? r->affectedRows() : 0;
}

int64_t sp_rt_pg_res_row_count(void* res) {
    if (!res) return 0;
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    return r ? r->rowCount() : 0;
}

int64_t sp_rt_pg_res_col_count(void* res) {
    if (!res) return 0;
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    return r ? r->columnCount() : 0;
}

const char* sp_rt_pg_res_col_name(void* res, int64_t col) {
    if (!res) return alloc_str("");
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    if (!r) return alloc_str("");
    return alloc_str(r->columnName(static_cast<int>(col)));
}

const char* sp_rt_pg_res_get_value(void* res, int64_t row, int64_t col) {
    if (!res) return alloc_str("");
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    if (!r) return alloc_str("");
    return alloc_str(r->getValue(static_cast<int>(row), static_cast<int>(col)));
}

bool sp_rt_pg_res_is_null(void* res, int64_t row, int64_t col) {
    if (!res) return true;
    auto r = *static_cast<std::shared_ptr<PgResult>*>(res);
    return r ? r->isNull(static_cast<int>(row), static_cast<int>(col)) : true;
}

void sp_rt_pg_res_free(void* res) {
    if (!res) return;
    delete static_cast<std::shared_ptr<PgResult>*>(res);
}

// ============================================================================
// Connection Pool C-ABI Bindings
// ============================================================================

void* sp_rt_db_pool_create_sqlite(const char* path, int64_t min_conn, int64_t max_conn, int64_t acquire_timeout_ms) {
    std::string dbPath = path ? path : ":memory:";
    DbPoolConfig cfg;
    cfg.minConnections = static_cast<size_t>(min_conn > 0 ? min_conn : 1);
    cfg.maxConnections = static_cast<size_t>(max_conn > 0 ? max_conn : 10);
    cfg.acquireTimeoutMs = static_cast<int>(acquire_timeout_ms > 0 ? acquire_timeout_ms : 5000);
    cfg.testOnAcquire = true;

    auto factory = [dbPath]() -> void* {
        auto db = new SqliteDatabase();
        if (!db->open(dbPath)) {
            delete db;
            return nullptr;
        }
        return db;
    };

    auto destroyer = [](void* ptr) {
        if (ptr) {
            auto db = static_cast<SqliteDatabase*>(ptr);
            db->close();
            delete db;
        }
    };

    auto pinger = [](void* ptr) -> bool {
        if (!ptr) return false;
        auto db = static_cast<SqliteDatabase*>(ptr);
        return db->execute("SELECT 1;") >= 0;
    };

    auto pool = new DbPool(cfg, factory, destroyer, pinger);
    pool->initialize();
    return pool;
}

void* sp_rt_db_pool_create_pg(const char* conn_str, int64_t min_conn, int64_t max_conn, int64_t acquire_timeout_ms) {
    std::string cs = conn_str ? conn_str : "";
    DbPoolConfig cfg;
    cfg.minConnections = static_cast<size_t>(min_conn > 0 ? min_conn : 1);
    cfg.maxConnections = static_cast<size_t>(max_conn > 0 ? max_conn : 10);
    cfg.acquireTimeoutMs = static_cast<int>(acquire_timeout_ms > 0 ? acquire_timeout_ms : 5000);
    cfg.testOnAcquire = true;

    auto factory = [cs]() -> void* {
        auto pg = new PgConnection();
        if (!pg->connect(cs)) {
            delete pg;
            return nullptr;
        }
        return pg;
    };

    auto destroyer = [](void* ptr) {
        if (ptr) {
            auto pg = static_cast<PgConnection*>(ptr);
            pg->close();
            delete pg;
        }
    };

    auto pinger = [](void* ptr) -> bool {
        if (!ptr) return false;
        return static_cast<PgConnection*>(ptr)->ping();
    };

    auto pool = new DbPool(cfg, factory, destroyer, pinger);
    pool->initialize();
    return pool;
}

void* sp_rt_db_pool_acquire(void* pool) {
    if (!pool) return nullptr;
    return static_cast<DbPool*>(pool)->acquire();
}

void sp_rt_db_pool_release(void* pool, void* conn) {
    if (!pool || !conn) return;
    static_cast<DbPool*>(pool)->release(conn);
}

void sp_rt_db_pool_invalidate(void* pool, void* conn) {
    if (!pool || !conn) return;
    static_cast<DbPool*>(pool)->invalidate(conn);
}

void sp_rt_db_pool_close(void* pool) {
    if (!pool) return;
    auto p = static_cast<DbPool*>(pool);
    p->closeAll();
    delete p;
}

int64_t sp_rt_db_pool_total(void* pool) {
    if (!pool) return 0;
    return static_cast<int64_t>(static_cast<DbPool*>(pool)->totalCount());
}

int64_t sp_rt_db_pool_available(void* pool) {
    if (!pool) return 0;
    return static_cast<int64_t>(static_cast<DbPool*>(pool)->availableCount());
}

int64_t sp_rt_db_pool_in_use(void* pool) {
    if (!pool) return 0;
    return static_cast<int64_t>(static_cast<DbPool*>(pool)->inUseCount());
}

} // extern "C"
