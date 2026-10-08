#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>

struct sqlite3;
struct sqlite3_stmt;

namespace sp::runtime {

enum class SqliteType {
    Integer = 1,
    Float = 2,
    Text = 3,
    Blob = 4,
    Null = 5
};

class SqliteStmt {
public:
    explicit SqliteStmt(sqlite3_stmt* stmt);
    ~SqliteStmt();

    SqliteStmt(const SqliteStmt&) = delete;
    SqliteStmt& operator=(const SqliteStmt&) = delete;

    SqliteStmt(SqliteStmt&& other) noexcept;
    SqliteStmt& operator=(SqliteStmt&& other) noexcept;

    bool bindInt(int index, int64_t val);
    bool bindDouble(int index, double val);
    bool bindString(int index, const std::string& val);
    bool bindNull(int index);

    int step(); // 100 = SQLITE_ROW, 101 = SQLITE_DONE, else error code
    bool isRow() const;
    bool isDone() const;

    int columnCount() const;
    std::string columnName(int index) const;
    SqliteType columnType(int index) const;
    int64_t columnInt(int index) const;
    double columnDouble(int index) const;
    std::string columnString(int index) const;
    bool columnIsNull(int index) const;

    bool reset();
    bool clearBindings();
    void finalize();

    sqlite3_stmt* raw() const { return m_stmt; }

private:
    sqlite3_stmt* m_stmt{nullptr};
};

class SqliteDatabase {
public:
    SqliteDatabase() = default;
    ~SqliteDatabase();

    SqliteDatabase(const SqliteDatabase&) = delete;
    SqliteDatabase& operator=(const SqliteDatabase&) = delete;

    SqliteDatabase(SqliteDatabase&& other) noexcept;
    SqliteDatabase& operator=(SqliteDatabase&& other) noexcept;

    bool open(const std::string& path, int busyTimeoutMs = 5000, bool walMode = true);
    void close();
    bool isOpen() const { return m_db != nullptr; }

    std::string lastError() const;
    int lastErrorCode() const;

    // Executes DDL/DML, returns number of changed rows or -1 on error
    int execute(const std::string& sql);

    int64_t lastInsertRowId() const;
    int changes() const;

    bool beginTransaction();
    bool commit();
    bool rollback();

    std::shared_ptr<SqliteStmt> prepare(const std::string& sql);

    sqlite3* raw() const { return m_db; }

private:
    sqlite3* m_db{nullptr};
};

} // namespace sp::runtime
