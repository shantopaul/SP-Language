module std.db.transaction;

import std.db.sqlite;
import std.db.postgres;

// ============================================================================
// Standard Library Safe Transaction Manager (SP 2.5)
// ============================================================================

public class Transaction {
    public SqliteConnection sqliteConn;
    public PgConnection pgConn;
    public string driver;
    public bool isFinished;

    public static fn forSqlite(SqliteConnection conn) -> Transaction {
        let tx = new Transaction();
        tx.sqliteConn = conn;
        tx.driver = "sqlite";
        tx.isFinished = false;
        conn.beginTransaction();
        return tx;
    }

    public static fn forPostgres(PgConnection conn) -> Transaction {
        let tx = new Transaction();
        tx.pgConn = conn;
        tx.driver = "postgres";
        tx.isFinished = false;
        conn.beginTransaction();
        return tx;
    }

    public fn execute(string sql) -> int64 {
        if (this.isFinished) return -1;
        if (this.driver == "sqlite") {
            return this.sqliteConn.execute(sql);
        } else if (this.driver == "postgres") {
            let res = this.pgConn.query(sql);
            if (res.isValid()) {
                if (res.isOk()) {
                    let affected = res.affectedRows();
                    res.free();
                    return affected;
                }
            }
            res.free();
            return -1;
        }
        return -1;
    }

    public fn commit() -> bool {
        if (this.isFinished) return false;
        this.isFinished = true;
        if (this.driver == "sqlite") {
            return this.sqliteConn.commit();
        } else if (this.driver == "postgres") {
            return this.pgConn.commit();
        }
        return false;
    }

    public fn rollback() -> bool {
        if (this.isFinished) return false;
        this.isFinished = true;
        if (this.driver == "sqlite") {
            return this.sqliteConn.rollback();
        } else if (this.driver == "postgres") {
            return this.pgConn.rollback();
        }
        return false;
    }
}
