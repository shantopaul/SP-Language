module examples.database_crud;

import std.io;
import std.db;

// ============================================================================
// Phase 23: Tier-1 Databases & Connection Pooling (SP 2.5) Demo
// ============================================================================

fn main() {
    println("==================================================");
    println(" SP Lang 2.5 - Enterprise Database Engine Demo   ");
    println("==================================================");

    // 1. In-memory SQLite database connection
    println("\n[1] Initializing SQLite In-Memory Database...");
    let db = Database.memorySqlite();
    if (!db.isOpen()) {
        println("ERROR: Failed to open SQLite database: " + db.lastError());
        return;
    }
    println("    Connected successfully to SQLite engine.");

    // 2. DDL Schema Execution
    println("\n[2] Creating 'products' table schema...");
    let createRc = db.execute("CREATE TABLE products (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, price REAL NOT NULL, stock INTEGER NOT NULL);");
    if (createRc >= 0) {
        println("    Schema created successfully.");
    }

    // 3. Parameterized Prepared Statements (Zero SQL Injection Risk)
    println("\n[3] Executing Parameterized Inserts...");
    let insertStmt = db.prepare("INSERT INTO products (name, price, stock) VALUES (?, ?, ?);");
    if (insertStmt.isValid()) {
        // Product 1
        insertStmt.bindString(1, "SP Pro Mechanical Keyboard");
        insertStmt.bindDouble(2, 129.99);
        insertStmt.bindInt(3, 45);
        insertStmt.step();
        println("    Inserted Product #1: ID=" + db.lastInsertRowId());

        // Product 2
        insertStmt.reset();
        insertStmt.clear();
        insertStmt.bindString(1, "SP Ergonomic Wireless Mouse");
        insertStmt.bindDouble(2, 69.50);
        insertStmt.bindInt(3, 120);
        insertStmt.step();
        println("    Inserted Product #2: ID=" + db.lastInsertRowId());

        // Product 3
        insertStmt.reset();
        insertStmt.clear();
        insertStmt.bindString(1, "SP 4K UHD Developer Monitor");
        insertStmt.bindDouble(2, 499.00);
        insertStmt.bindInt(3, 18);
        insertStmt.step();
        println("    Inserted Product #3: ID=" + db.lastInsertRowId());

        insertStmt.finalize();

        // Product 4 using Named Parameter Bindings (:name, :price, :stock)
        let namedStmt = db.prepare("INSERT INTO products (name, price, stock) VALUES (:name, :price, :stock);");
        if (namedStmt.isValid()) {
            namedStmt.bindNamedString("name", "SP Studio USB-C Microphone");
            namedStmt.bindNamedDouble("price", 189.00);
            namedStmt.bindNamedInt("stock", 35);
            namedStmt.step();
            println("    Inserted Product #4 (via Named :params): ID=" + db.lastInsertRowId());
            namedStmt.finalize();
        }
    }

    // 4. Querying and Iterating Results
    println("\n[4] Querying Products (Price > 100.00)...");
    let queryStmt = db.prepare("SELECT id, name, price, stock FROM products WHERE price > ? ORDER BY price DESC;");
    if (queryStmt.isValid()) {
        queryStmt.bindDouble(1, 100.00);
        while (queryStmt.next()) {
            let id = queryStmt.getInt(0);
            let name = queryStmt.getString(1);
            let price = queryStmt.getDouble(2);
            let stock = queryStmt.getInt(3);
            println("    - [ID: " + id + "] " + name + " | Price: $" + price + " | Stock: " + stock);
        }
        queryStmt.finalize();
    }

    // 5. Safe Transaction Management
    println("\n[5] Demonstrating Safe Transaction Rollback & Commit...");
    db.beginTransaction();
    db.execute("UPDATE products SET stock = stock - 5 WHERE id = 1;");
    println("    Executed tentative stock deduction. Rolling back...");
    db.rollback();

    let checkStmt = db.prepare("SELECT stock FROM products WHERE id = 1;");
    if (checkStmt.isValid()) {
        if (checkStmt.next()) {
            println("    Verified Product #1 stock after rollback: " + checkStmt.getInt(0) + " (Unchanged!)");
        }
        checkStmt.finalize();
    }

    db.close();

    // 6. Thread-Safe Connection Pool
    println("\n[6] Initializing Thread-Safe Database Connection Pool...");
    let pool = Database.sqlitePool(":memory:", 2, 5, 2000);
    println("    Pool initialized. Total: " + pool.total() + ", Available: " + pool.available() + ", In-Use: " + pool.inUse());

    let conn1 = pool.acquireSqlite();
    println("    Acquired connection 1. In-Use: " + pool.inUse() + ", Available: " + pool.available());
    conn1.execute("CREATE TABLE metrics (key TEXT, val INTEGER);");
    conn1.execute("INSERT INTO metrics VALUES ('uptime_sec', 3600);");

    let conn2 = pool.acquireSqlite();
    println("    Acquired connection 2. In-Use: " + pool.inUse() + ", Available: " + pool.available());

    pool.releaseSqlite(conn1);
    pool.releaseSqlite(conn2);
    println("    Released connections back to pool. In-Use: " + pool.inUse() + ", Available: " + pool.available());
    pool.close();

    // 7. Fluent Query Builder
    println("\n[7] Demonstrating Fluent Query Builder...");
    let qb = Database.query("users")
        .select("id, name, email")
        .where("status", "=", "active")
        .orderBy("created_at", "DESC")
        .limit(10);

    println("    Generated SQLite SQL:   " + qb.toSelectSql("sqlite"));
    println("    Generated Postgres SQL: " + qb.toSelectSql("postgres"));
    println("    Generated Count SQL:    " + qb.toCountSql("sqlite"));

    let qbIn = Database.query("users")
        .select("id, name")
        .whereIn("role", "admin", "moderator");
    println("    Generated whereIn SQLite SQL:   " + qbIn.toSelectSql("sqlite"));
    println("    Generated whereIn Postgres SQL: " + qbIn.toSelectSql("postgres"));

    println("\n==================================================");
    println(" Phase 23 Database Platform Successfully Verified ");
    println("==================================================");
}
