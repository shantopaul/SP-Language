#include <gtest/gtest.h>
#include "sp/runtime/Sqlite.h"
#include "sp/runtime/Postgres.h"
#include "sp/runtime/DbPool.h"
#include "sp/runtime/Runtime.h"
#include <chrono>
#include <thread>
#include <vector>
#include <filesystem>

using namespace sp::runtime;
namespace fs = std::filesystem;

// ============================================================================
// SQLite Unit & Integration Tests
// ============================================================================

TEST(SqliteTest, BasicOperationsAndLifecycle) {
    SqliteDatabase db;
    ASSERT_TRUE(db.open(":memory:"));
    EXPECT_TRUE(db.isOpen());

    int rc = db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT, age INTEGER);");
    EXPECT_GE(rc, 0);

    rc = db.execute("INSERT INTO users (name, age) VALUES ('Alice', 30);");
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(db.lastInsertRowId(), 1);
    EXPECT_EQ(db.changes(), 1);

    rc = db.execute("INSERT INTO users (name, age) VALUES ('Bob', 25);");
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(db.lastInsertRowId(), 2);

    db.close();
    EXPECT_FALSE(db.isOpen());
}

TEST(SqliteTest, PreparedStatementAndParameterizedBinding) {
    SqliteDatabase db;
    ASSERT_TRUE(db.open(":memory:"));

    db.execute("CREATE TABLE items (id INTEGER PRIMARY KEY, title TEXT, price REAL, note TEXT);");

    auto insertStmt = db.prepare("INSERT INTO items (id, title, price, note) VALUES (?, ?, ?, ?);");
    ASSERT_NE(insertStmt, nullptr);

    // Bind row 1
    EXPECT_TRUE(insertStmt->bindInt(1, 101));
    EXPECT_TRUE(insertStmt->bindString(2, "Mechanical Keyboard"));
    EXPECT_TRUE(insertStmt->bindDouble(3, 149.99));
    EXPECT_TRUE(insertStmt->bindNull(4));
    EXPECT_EQ(insertStmt->step(), 101); // SQLITE_DONE

    // Reset and bind row 2
    EXPECT_TRUE(insertStmt->reset());
    EXPECT_TRUE(insertStmt->clearBindings());
    EXPECT_TRUE(insertStmt->bindInt(1, 102));
    EXPECT_TRUE(insertStmt->bindString(2, "Gaming Mouse"));
    EXPECT_TRUE(insertStmt->bindDouble(3, 79.50));
    EXPECT_TRUE(insertStmt->bindString(4, "RGB wireless"));
    EXPECT_EQ(insertStmt->step(), 101);

    insertStmt->finalize();

    // Query rows back
    auto selectStmt = db.prepare("SELECT id, title, price, note FROM items ORDER BY id ASC;");
    ASSERT_NE(selectStmt, nullptr);
    EXPECT_EQ(selectStmt->columnCount(), 4);
    EXPECT_EQ(selectStmt->columnName(0), "id");
    EXPECT_EQ(selectStmt->columnName(1), "title");
    EXPECT_EQ(selectStmt->columnName(2), "price");
    EXPECT_EQ(selectStmt->columnName(3), "note");

    // First row
    EXPECT_EQ(selectStmt->step(), 100); // SQLITE_ROW
    EXPECT_EQ(selectStmt->columnInt(0), 101);
    EXPECT_EQ(selectStmt->columnString(1), "Mechanical Keyboard");
    EXPECT_DOUBLE_EQ(selectStmt->columnDouble(2), 149.99);
    EXPECT_TRUE(selectStmt->columnIsNull(3));

    // Second row
    EXPECT_EQ(selectStmt->step(), 100); // SQLITE_ROW
    EXPECT_EQ(selectStmt->columnInt(0), 102);
    EXPECT_EQ(selectStmt->columnString(1), "Gaming Mouse");
    EXPECT_DOUBLE_EQ(selectStmt->columnDouble(2), 79.50);
    EXPECT_FALSE(selectStmt->columnIsNull(3));
    EXPECT_EQ(selectStmt->columnString(3), "RGB wireless");

    // End of rows
    EXPECT_EQ(selectStmt->step(), 101); // SQLITE_DONE
    selectStmt->finalize();
}

TEST(SqliteTest, TransactionsAndRollback) {
    SqliteDatabase db;
    ASSERT_TRUE(db.open(":memory:"));
    db.execute("CREATE TABLE ledger (account TEXT, balance INTEGER);");
    db.execute("INSERT INTO ledger VALUES ('acc1', 1000);");

    // 1. Transaction Rollback
    EXPECT_TRUE(db.beginTransaction());
    db.execute("UPDATE ledger SET balance = 500 WHERE account = 'acc1';");
    db.execute("INSERT INTO ledger VALUES ('acc2', 500);");
    EXPECT_TRUE(db.rollback());

    auto checkStmt = db.prepare("SELECT COUNT(*) FROM ledger WHERE account = 'acc2';");
    ASSERT_NE(checkStmt, nullptr);
    EXPECT_EQ(checkStmt->step(), 100);
    EXPECT_EQ(checkStmt->columnInt(0), 0); // Not inserted
    checkStmt->finalize();

    auto balanceStmt = db.prepare("SELECT balance FROM ledger WHERE account = 'acc1';");
    ASSERT_NE(balanceStmt, nullptr);
    EXPECT_EQ(balanceStmt->step(), 100);
    EXPECT_EQ(balanceStmt->columnInt(0), 1000); // Original balance maintained
    balanceStmt->finalize();

    // 2. Transaction Commit
    EXPECT_TRUE(db.beginTransaction());
    db.execute("UPDATE ledger SET balance = 800 WHERE account = 'acc1';");
    db.execute("INSERT INTO ledger VALUES ('acc3', 200);");
    EXPECT_TRUE(db.commit());

    auto committedStmt = db.prepare("SELECT balance FROM ledger WHERE account = 'acc1';");
    ASSERT_NE(committedStmt, nullptr);
    EXPECT_EQ(committedStmt->step(), 100);
    EXPECT_EQ(committedStmt->columnInt(0), 800);
    committedStmt->finalize();
}

TEST(SqliteTest, WalModeAndFileDatabase) {
    fs::path tempDb = fs::temp_directory_path() / "sp_test_wal.db";
    if (fs::exists(tempDb)) fs::remove(tempDb);

    {
        SqliteDatabase db;
        EXPECT_TRUE(db.open(tempDb.string(), 3000, true));
        db.execute("CREATE TABLE test_wal (v TEXT);");
        db.execute("INSERT INTO test_wal VALUES ('wal_mode_active');");
        db.close();
    }

    // Reopen and verify data persisted
    {
        SqliteDatabase db;
        EXPECT_TRUE(db.open(tempDb.string(), 3000, true));
        auto stmt = db.prepare("SELECT v FROM test_wal;");
        ASSERT_NE(stmt, nullptr);
        EXPECT_EQ(stmt->step(), 100);
        EXPECT_EQ(stmt->columnString(0), "wal_mode_active");
        stmt->finalize();
        db.close();
    }

    fs::remove(tempDb);
}

TEST(SqliteTest, SqlInjectionPreventionViaParameterizedQuery) {
    SqliteDatabase db;
    ASSERT_TRUE(db.open(":memory:"));
    db.execute("CREATE TABLE users (id INTEGER PRIMARY KEY, username TEXT, role TEXT);");
    db.execute("INSERT INTO users (username, role) VALUES ('admin', 'superuser');");
    db.execute("INSERT INTO users (username, role) VALUES ('guest', 'regular');");

    // Attacker input designed to break non-parameterized queries
    std::string maliciousInput = "guest' OR '1'='1";

    auto stmt = db.prepare("SELECT COUNT(*) FROM users WHERE username = ?;");
    ASSERT_NE(stmt, nullptr);
    stmt->bindString(1, maliciousInput);

    EXPECT_EQ(stmt->step(), 100);
    // Malicious input was treated strictly as a string literal, matching 0 rows
    EXPECT_EQ(stmt->columnInt(0), 0);
    stmt->finalize();
}

// ============================================================================
// PostgreSQL Unit Tests
// ============================================================================

TEST(PostgresTest, ConnectionStringParser) {
    std::string uri = "postgres://sp_user:super_secret_pw@db.splang.io:5433/ecommerce_db?sslmode=disable";
    auto cfg = PgConfig::parseConnectionString(uri);

    EXPECT_EQ(cfg.host, "db.splang.io");
    EXPECT_EQ(cfg.port, 5433);
    EXPECT_EQ(cfg.user, "sp_user");
    EXPECT_EQ(cfg.password, "super_secret_pw");
    EXPECT_EQ(cfg.database, "ecommerce_db");
    EXPECT_EQ(cfg.sslmode, "disable");

    // Test key-value string format
    std::string kv = "host=192.168.1.50 port=5432 user=admin password=pass dbname=testdb sslmode=require";
    auto cfgKv = PgConfig::parseConnectionString(kv);

    EXPECT_EQ(cfgKv.host, "192.168.1.50");
    EXPECT_EQ(cfgKv.port, 5432);
    EXPECT_EQ(cfgKv.user, "admin");
    EXPECT_EQ(cfgKv.password, "pass");
    EXPECT_EQ(cfgKv.database, "testdb");
    EXPECT_EQ(cfgKv.sslmode, "require");
}

TEST(PostgresTest, ResultParsingAndAttributes) {
    PgResult res;
    res.setCommandTag("INSERT 0 15");
    EXPECT_EQ(res.affectedRows(), 15);

    res.setCommandTag("UPDATE 7");
    EXPECT_EQ(res.affectedRows(), 7);

    res.setCommandTag("DELETE 3");
    EXPECT_EQ(res.affectedRows(), 3);

    res.addColumn({"id", 23, 0});
    res.addColumn({"name", 25, 0});

    PgRow row1;
    row1.values = {"1", "Alice"};
    row1.isNull = {false, false};
    res.addRow(row1);

    PgRow row2;
    row2.values = {"2", ""};
    row2.isNull = {false, true};
    res.addRow(row2);

    EXPECT_EQ(res.rowCount(), 2);
    EXPECT_EQ(res.columnCount(), 2);
    EXPECT_EQ(res.columnName(0), "id");
    EXPECT_EQ(res.columnName(1), "name");
    EXPECT_EQ(res.getValue(0, 0), "1");
    EXPECT_EQ(res.getValue(0, 1), "Alice");
    EXPECT_FALSE(res.isNull(0, 1));
    EXPECT_TRUE(res.isNull(1, 1));
}

// ============================================================================
// Generic Connection Pool Tests
// ============================================================================

TEST(DbPoolTest, SqlitePoolAcquireReleaseAndStats) {
    DbPoolConfig cfg;
    cfg.minConnections = 2;
    cfg.maxConnections = 4;
    cfg.acquireTimeoutMs = 1000;

    auto factory = []() -> void* {
        auto db = new SqliteDatabase();
        db->open(":memory:");
        return db;
    };

    auto destroyer = [](void* ptr) {
        auto db = static_cast<SqliteDatabase*>(ptr);
        delete db;
    };

    auto pinger = [](void* ptr) -> bool {
        auto db = static_cast<SqliteDatabase*>(ptr);
        return db->execute("SELECT 1;") >= 0;
    };

    DbPool pool(cfg, factory, destroyer, pinger);
    EXPECT_TRUE(pool.initialize());

    EXPECT_EQ(pool.totalCount(), 2);
    EXPECT_EQ(pool.availableCount(), 2);
    EXPECT_EQ(pool.inUseCount(), 0);

    // Acquire first connection
    void* conn1 = pool.acquire();
    ASSERT_NE(conn1, nullptr);
    EXPECT_EQ(pool.totalCount(), 2);
    EXPECT_EQ(pool.availableCount(), 1);
    EXPECT_EQ(pool.inUseCount(), 1);

    // Acquire second connection
    void* conn2 = pool.acquire();
    ASSERT_NE(conn2, nullptr);
    EXPECT_EQ(pool.totalCount(), 2);
    EXPECT_EQ(pool.availableCount(), 0);
    EXPECT_EQ(pool.inUseCount(), 2);

    // Acquire third connection (dynamically grows up to max 4)
    void* conn3 = pool.acquire();
    ASSERT_NE(conn3, nullptr);
    EXPECT_EQ(pool.totalCount(), 3);
    EXPECT_EQ(pool.inUseCount(), 3);

    // Release connections
    pool.release(conn1);
    EXPECT_EQ(pool.availableCount(), 1);
    EXPECT_EQ(pool.inUseCount(), 2);

    pool.release(conn2);
    pool.release(conn3);
    EXPECT_EQ(pool.availableCount(), 3);
    EXPECT_EQ(pool.inUseCount(), 0);

    pool.closeAll();
    EXPECT_EQ(pool.totalCount(), 0);
}

TEST(DbPoolTest, PoolExhaustionAndTimeout) {
    DbPoolConfig cfg;
    cfg.minConnections = 1;
    cfg.maxConnections = 1;
    cfg.acquireTimeoutMs = 100; // 100 ms timeout

    auto factory = []() -> void* {
        auto db = new SqliteDatabase();
        db->open(":memory:");
        return db;
    };
    auto destroyer = [](void* ptr) {
        delete static_cast<SqliteDatabase*>(ptr);
    };

    DbPool pool(cfg, factory, destroyer);
    pool.initialize();

    void* conn1 = pool.acquire();
    ASSERT_NE(conn1, nullptr);

    // Attempting to acquire when max=1 and in-use=1 should time out
    auto start = std::chrono::steady_clock::now();
    void* conn2 = pool.acquire();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();

    EXPECT_EQ(conn2, nullptr);
    EXPECT_GE(elapsed, 80); // Waited approximately 100ms

    // Release conn1 and try again
    pool.release(conn1);
    void* conn3 = pool.acquire();
    EXPECT_NE(conn3, nullptr);
    pool.release(conn3);

    pool.closeAll();
}

TEST(DbPoolTest, MultiThreadedConcurrentAcquisition) {
    DbPoolConfig cfg;
    cfg.minConnections = 2;
    cfg.maxConnections = 4;
    cfg.acquireTimeoutMs = 2000;

    auto factory = []() -> void* {
        auto db = new SqliteDatabase();
        db->open(":memory:");
        db->execute("CREATE TABLE counter (val INTEGER);");
        db->execute("INSERT INTO counter VALUES (0);");
        return db;
    };
    auto destroyer = [](void* ptr) {
        delete static_cast<SqliteDatabase*>(ptr);
    };

    DbPool pool(cfg, factory, destroyer);
    pool.initialize();

    const int numThreads = 8;
    const int opsPerThread = 50;
    std::vector<std::thread> workers;

    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&pool, opsPerThread]() {
            for (int i = 0; i < opsPerThread; ++i) {
                void* conn = pool.acquire();
                if (conn) {
                    auto db = static_cast<SqliteDatabase*>(conn);
                    db->execute("UPDATE counter SET val = val + 1;");
                    std::this_thread::yield();
                    pool.release(conn);
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(pool.inUseCount(), 0);
    pool.closeAll();
}

// ============================================================================
// C-ABI Runtime Bridge Tests
// ============================================================================

TEST(DbBridgeTest, SqliteCAbiWorkflow) {
    void* db = sp_rt_sqlite_open(":memory:", 5000, false);
    ASSERT_NE(db, nullptr);

    int64_t rc = sp_rt_sqlite_exec(db, "CREATE TABLE products (id INTEGER PRIMARY KEY, name TEXT, price REAL);");
    EXPECT_GE(rc, 0);

    rc = sp_rt_sqlite_exec(db, "INSERT INTO products (name, price) VALUES ('Monitor', 299.99);");
    EXPECT_EQ(rc, 1);
    EXPECT_EQ(sp_rt_sqlite_last_insert_rowid(db), 1);

    void* stmt = sp_rt_sqlite_prepare(db, "SELECT id, name, price FROM products WHERE id = ?;");
    ASSERT_NE(stmt, nullptr);

    EXPECT_TRUE(sp_rt_sqlite_bind_int(stmt, 1, 1));
    EXPECT_EQ(sp_rt_sqlite_step(stmt), 100); // SQLITE_ROW

    EXPECT_EQ(sp_rt_sqlite_col_count(stmt), 3);
    EXPECT_STREQ(sp_rt_sqlite_col_name(stmt, 1), "name");
    EXPECT_EQ(sp_rt_sqlite_col_int(stmt, 0), 1);
    EXPECT_STREQ(sp_rt_sqlite_col_string(stmt, 1), "Monitor");
    EXPECT_DOUBLE_EQ(sp_rt_sqlite_col_double(stmt, 2), 299.99);

    EXPECT_EQ(sp_rt_sqlite_step(stmt), 101); // SQLITE_DONE
    sp_rt_sqlite_finalize(stmt);

    sp_rt_sqlite_close(db);
}

TEST(DbBridgeTest, DbPoolCAbiWorkflow) {
    void* pool = sp_rt_db_pool_create_sqlite(":memory:", 2, 4, 1000);
    ASSERT_NE(pool, nullptr);

    EXPECT_EQ(sp_rt_db_pool_total(pool), 2);
    EXPECT_EQ(sp_rt_db_pool_available(pool), 2);
    EXPECT_EQ(sp_rt_db_pool_in_use(pool), 0);

    void* conn = sp_rt_db_pool_acquire(pool);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(sp_rt_db_pool_in_use(pool), 1);

    int64_t execRc = sp_rt_sqlite_exec(conn, "SELECT 1;");
    EXPECT_GE(execRc, 0);

    sp_rt_db_pool_release(pool, conn);
    EXPECT_EQ(sp_rt_db_pool_in_use(pool), 0);

    sp_rt_db_pool_close(pool);
}

// ============================================================================
// High-Throughput Database Benchmark (50,000 queries/sec requirement)
// ============================================================================

TEST(SqliteTest, HighThroughputBenchmark) {
    SqliteDatabase db;
    ASSERT_TRUE(db.open(":memory:"));
    db.execute("CREATE TABLE benchmark (id INTEGER PRIMARY KEY, val INTEGER);");

    const int totalInserts = 25000;
    auto start = std::chrono::steady_clock::now();

    db.beginTransaction();
    auto stmt = db.prepare("INSERT INTO benchmark (val) VALUES (?);");
    for (int i = 0; i < totalInserts; ++i) {
        stmt->bindInt(1, i);
        stmt->step();
        stmt->reset();
    }
    stmt->finalize();
    db.commit();

    auto end = std::chrono::steady_clock::now();
    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    if (durationMs == 0) durationMs = 1;

    double qps = (double)totalInserts / ((double)durationMs / 1000.0);
    std::cout << "[ BENCHMARK ] Executed " << totalInserts << " prepared INSERT queries in "
              << durationMs << " ms (" << static_cast<int>(qps) << " queries/sec)\n";

    EXPECT_GT(qps, 50000.0); // Exceeds 50k queries/sec
}
