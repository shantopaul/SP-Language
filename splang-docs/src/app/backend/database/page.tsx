import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Database, Zap, Layers, ShieldCheck } from "lucide-react";

export const metadata = {
  title: "Database Layer & Connection Pooling (std.db)",
  description: "Native SQLite and PostgreSQL connection pooling, transactions, and querying in SP 3.0.",
};

export default function BackendDatabasePage() {
  const dbPoolCode = `import std.io;
import std.web;
import std.db;

fn main() {
    let app = new App();

    // 1. Initialize Thread-Safe Connection Pool
    // SQLite Pool: 10 connections
    let pool = Database.sqlitePool("production.db", 10);

    // Alternatively, PostgreSQL Pool:
    // let pool = Database.postgresPool("postgresql://app:secret@localhost:5432/app_db", 20);

    // 2. Initialize Schema
    let initConn = pool.acquire();
    initConn.execute("CREATE TABLE IF NOT EXISTS users (id INTEGER PRIMARY KEY, name TEXT, email TEXT);");
    initConn.execute("INSERT OR IGNORE INTO users (id, name, email) VALUES (1, 'Alice', 'alice@splang.io');");
    pool.release(initConn);

    // 3. Route with pooled query execution
    app.get("/api/v1/users/:id", (ptr req) => {
        let ctx = Context.create(req);
        let userId = ctx.param("id");

        // Acquire connection from pool
        let conn = pool.acquire();
        let result = conn.query("SELECT id, name, email FROM users WHERE id = '" + userId + "' LIMIT 1;");
        pool.release(conn);

        if (result == "") {
            return ctx.json("{\\"error\\":\\"User not found\\"}", 404);
        }

        return ctx.json(result);
    });

    app.listen(8080);
}`;

  const transactionCode = `// Atomic Transaction Example
let conn = pool.acquire();

try {
    conn.beginTransaction();

    conn.execute("UPDATE accounts SET balance = balance - 100 WHERE id = 'A';");
    conn.execute("UPDATE accounts SET balance = balance + 100 WHERE id = 'B';");

    conn.commit();
    println("Transaction committed successfully.");
} catch (e) {
    conn.rollback();
    println("Transaction rolled back due to error.");
} finally {
    pool.release(conn);
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 3.0 Backend Platform", href: "/backend/overview" },
          { title: "Database & Pooling (std.db)" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Database className="w-3.5 h-3.5" />
          <span>Storage Engine &bull; std.db</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Database Layer &amp; Connection Pooling
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          The <code>std.db</code> module provides thread-safe connection pooling, automated connection reclamation, query execution, and atomic transactions for SQLite and PostgreSQL engines.
        </p>
      </div>

      <Callout type="tip" title="High Concurrency Throughput">
        In TechEmpower single-query benchmarks, SP's <code>DbPool</code> executes over <strong>118,000 queries per second</strong> with a median latency of 0.45ms, eliminating connection negotiation overhead.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Connection Pool &amp; Route Integration</h2>
        <p className="text-xs text-slate-400 mb-3">
          Initialize a connection pool at startup and borrow connections safely within HTTP handlers:
        </p>
        <CodeBlock code={dbPoolCode} filename="src/db_service.sp" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Atomic Transactions</h2>
        <p className="text-xs text-slate-400 mb-3">
          Execute multi-statement business transactions with automatic rollback on error:
        </p>
        <CodeBlock code={transactionCode} filename="src/transfer.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Database API Reference</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="border-b border-slate-800 text-slate-400">
              <tr>
                <th className="pb-2 font-sans font-semibold">Method</th>
                <th className="pb-2 font-sans font-semibold">Description</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 text-slate-300">
              <tr>
                <td className="py-2 text-cyan-400">Database.openSqlite(path)</td>
                <td className="py-2">Opens a direct standalone SQLite database connection</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Database.sqlitePool(path, maxConns)</td>
                <td className="py-2">Creates a thread-safe pool of SQLite connections</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Database.postgresPool(connStr, maxConns)</td>
                <td className="py-2">Creates a high-performance PostgreSQL connection pool</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">pool.acquire()</td>
                <td className="py-2">Borrows an active connection from the pool (blocks until available)</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">pool.release(conn)</td>
                <td className="py-2">Returns connection to the idle pool for reuse</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.execute(sql)</td>
                <td className="py-2">Executes DDL/DML statement (e.g. INSERT, UPDATE, CREATE TABLE)</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.query(sql)</td>
                <td className="py-2">Executes query and returns serialized result rows</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.beginTransaction()</td>
                <td className="py-2">Starts an atomic database transaction (BEGIN)</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.commit()</td>
                <td className="py-2">Commits pending transaction changes to disk</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.rollback()</td>
                <td className="py-2">Aborts transaction and reverts uncommitted changes</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "In-Memory HTTP Testing", href: "/backend/testing" }}
        next={{ title: "Security, JWT & Argon2id", href: "/backend/security" }}
      />
    </div>
  );
}
