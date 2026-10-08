#include "sp/runtime/Sqlite.h"
#include "sqlite3.h"
#include <iostream>

namespace sp::runtime {

// --- SqliteStmt ---

SqliteStmt::SqliteStmt(sqlite3_stmt* stmt)
    : m_stmt(stmt) {}

SqliteStmt::~SqliteStmt() {
    finalize();
}

SqliteStmt::SqliteStmt(SqliteStmt&& other) noexcept
    : m_stmt(other.m_stmt) {
    other.m_stmt = nullptr;
}

SqliteStmt& SqliteStmt::operator=(SqliteStmt&& other) noexcept {
    if (this != &other) {
        finalize();
        m_stmt = other.m_stmt;
        other.m_stmt = nullptr;
    }
    return *this;
}

void SqliteStmt::finalize() {
    if (m_stmt) {
        sqlite3_finalize(m_stmt);
        m_stmt = nullptr;
    }
}

bool SqliteStmt::bindInt(int index, int64_t val) {
    if (!m_stmt) return false;
    return sqlite3_bind_int64(m_stmt, index, val) == SQLITE_OK;
}

bool SqliteStmt::bindDouble(int index, double val) {
    if (!m_stmt) return false;
    return sqlite3_bind_double(m_stmt, index, val) == SQLITE_OK;
}

bool SqliteStmt::bindString(int index, const std::string& val) {
    if (!m_stmt) return false;
    return sqlite3_bind_text(m_stmt, index, val.c_str(), static_cast<int>(val.size()), SQLITE_TRANSIENT) == SQLITE_OK;
}

bool SqliteStmt::bindNull(int index) {
    if (!m_stmt) return false;
    return sqlite3_bind_null(m_stmt, index) == SQLITE_OK;
}

int SqliteStmt::step() {
    if (!m_stmt) return SQLITE_MISUSE;
    return sqlite3_step(m_stmt);
}

bool SqliteStmt::isRow() const {
    return m_stmt != nullptr;
}

bool SqliteStmt::isDone() const {
    return m_stmt != nullptr;
}

int SqliteStmt::columnCount() const {
    if (!m_stmt) return 0;
    return sqlite3_column_count(m_stmt);
}

std::string SqliteStmt::columnName(int index) const {
    if (!m_stmt) return "";
    const char* name = sqlite3_column_name(m_stmt, index);
    return name ? name : "";
}

SqliteType SqliteStmt::columnType(int index) const {
    if (!m_stmt) return SqliteType::Null;
    return static_cast<SqliteType>(sqlite3_column_type(m_stmt, index));
}

int64_t SqliteStmt::columnInt(int index) const {
    if (!m_stmt) return 0;
    return sqlite3_column_int64(m_stmt, index);
}

double SqliteStmt::columnDouble(int index) const {
    if (!m_stmt) return 0.0;
    return sqlite3_column_double(m_stmt, index);
}

std::string SqliteStmt::columnString(int index) const {
    if (!m_stmt) return "";
    const unsigned char* text = sqlite3_column_text(m_stmt, index);
    return text ? reinterpret_cast<const char*>(text) : "";
}

bool SqliteStmt::columnIsNull(int index) const {
    if (!m_stmt) return true;
    return sqlite3_column_type(m_stmt, index) == SQLITE_NULL;
}

bool SqliteStmt::reset() {
    if (!m_stmt) return false;
    return sqlite3_reset(m_stmt) == SQLITE_OK;
}

bool SqliteStmt::clearBindings() {
    if (!m_stmt) return false;
    return sqlite3_clear_bindings(m_stmt) == SQLITE_OK;
}

// --- SqliteDatabase ---

SqliteDatabase::~SqliteDatabase() {
    close();
}

SqliteDatabase::SqliteDatabase(SqliteDatabase&& other) noexcept
    : m_db(other.m_db) {
    other.m_db = nullptr;
}

SqliteDatabase& SqliteDatabase::operator=(SqliteDatabase&& other) noexcept {
    if (this != &other) {
        close();
        m_db = other.m_db;
        other.m_db = nullptr;
    }
    return *this;
}

bool SqliteDatabase::open(const std::string& path, int busyTimeoutMs, bool walMode) {
    close();

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    int rc = sqlite3_open_v2(path.c_str(), &m_db, flags, nullptr);
    if (rc != SQLITE_OK) {
        return false;
    }

    if (busyTimeoutMs > 0) {
        sqlite3_busy_timeout(m_db, busyTimeoutMs);
    }

    if (walMode && path != ":memory:" && !path.empty()) {
        execute("PRAGMA journal_mode = WAL;");
        execute("PRAGMA synchronous = NORMAL;");
    }

    return true;
}

void SqliteDatabase::close() {
    if (m_db) {
        sqlite3_close_v2(m_db);
        m_db = nullptr;
    }
}

std::string SqliteDatabase::lastError() const {
    if (!m_db) return "Database not open";
    const char* err = sqlite3_errmsg(m_db);
    return err ? err : "";
}

int SqliteDatabase::lastErrorCode() const {
    if (!m_db) return -1;
    return sqlite3_errcode(m_db);
}

int SqliteDatabase::execute(const std::string& sql) {
    if (!m_db) return -1;
    char* errmsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        if (errmsg) {
            sqlite3_free(errmsg);
        }
        return -1;
    }
    return sqlite3_changes(m_db);
}

int64_t SqliteDatabase::lastInsertRowId() const {
    if (!m_db) return 0;
    return sqlite3_last_insert_rowid(m_db);
}

int SqliteDatabase::changes() const {
    if (!m_db) return 0;
    return sqlite3_changes(m_db);
}

bool SqliteDatabase::beginTransaction() {
    return execute("BEGIN TRANSACTION;") >= 0;
}

bool SqliteDatabase::commit() {
    return execute("COMMIT;") >= 0;
}

bool SqliteDatabase::rollback() {
    return execute("ROLLBACK;") >= 0;
}

std::shared_ptr<SqliteStmt> SqliteDatabase::prepare(const std::string& sql) {
    if (!m_db) return nullptr;
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
    if (rc != SQLITE_OK || !stmt) {
        return nullptr;
    }
    return std::make_shared<SqliteStmt>(stmt);
}

} // namespace sp::runtime
