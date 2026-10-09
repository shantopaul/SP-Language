import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.db Module Reference",
  description: "Native SQLite and PostgreSQL database pooling and query execution in SP.",
};

export default function StdDbPage() {
  const exampleCode = `import std.db;

fn main() {
    let pool = Database.sqlitePool("app.db", 10);
    let conn = pool.acquire();

    conn.execute("CREATE TABLE IF NOT EXISTS users (id INT, name TEXT);");
    let rows = conn.query("SELECT * FROM users;");
    println("Fetched users: " + rows);

    pool.release(conn);
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.db" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.db Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.db</code> module provides thread-safe connection pooling, automated connection leasing, transactions, and native drivers for SQLite and PostgreSQL.
      </p>

      <CodeBlock code={exampleCode} filename="db_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="Database.sqlitePool"
        signature="static fn sqlitePool(string path, int maxConns) -> DbPool"
        description="Creates a thread-safe connection pool for SQLite."
        parameters={[
          { name: "path", type: "string", description: "Database file path or ':memory:'." },
          { name: "maxConns", type: "int", description: "Maximum concurrent active connections." },
        ]}
        returns="DbPool instance"
      />

      <ApiCard
        name="Database.postgresPool"
        signature="static fn postgresPool(string connStr, int maxConns) -> DbPool"
        description="Creates a high-performance PostgreSQL connection pool."
        parameters={[
          { name: "connStr", type: "string", description: "PostgreSQL DSN connection URI." },
          { name: "maxConns", type: "int", description: "Maximum concurrent active connections." },
        ]}
        returns="DbPool instance"
      />

      <ApiCard
        name="pool.acquire"
        signature="fn acquire() -> DbConnection"
        description="Borrows an available connection from the pool. Blocks until a connection is freed."
        returns="DbConnection instance"
      />

      <ApiCard
        name="pool.release"
        signature="fn release(DbConnection conn) -> void"
        description="Returns a leased connection back into the pool for subsequent requests."
        parameters={[{ name: "conn", type: "DbConnection", description: "Active connection to return." }]}
        returns="void"
      />

      <PrevNextNav
        prev={{ title: "std.web Reference", href: "/stdlib/web" }}
        next={{ title: "std.redis Reference", href: "/stdlib/redis" }}
      />
    </div>
  );
}
