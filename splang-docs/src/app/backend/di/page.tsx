import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Boxes, Key, Zap, CheckCircle2 } from "lucide-react";

export const metadata = {
  title: "Dependency Injection (std.web.di)",
  description: "IoC Dependency Injection Container for clean backend architecture in SP 3.0.",
};

export default function BackendDiPage() {
  const diExampleCode = `import std.io;
import std.web;
import std.web.di;
import std.db;

// 1. Service Definition
class UserService {
    var pool: DbPool;

    fn UserService(DbPool p) {
        this.pool = p;
    }

    fn getUser(string id) -> string {
        // Query database via pooled connection
        let conn = this.pool.acquire();
        let res = conn.query("SELECT email FROM users WHERE id = '" + id + "'");
        this.pool.release(conn);
        return res;
    }
}

fn main() {
    let app = new App();
    let container = new Container();

    // 2. Bind Database Connection Pool as Singleton
    let dbPool = Database.sqlitePool("app.db", 10);
    container.bindValue("db_pool", dbPool);

    // 3. Register Service Factory
    container.bind("user_service", () => {
        let pool = container.resolveValue("db_pool") as DbPool;
        return new UserService(pool);
    });

    // 4. Attach Container to App
    app.setContainer(container);

    // 5. Injected Route Handler
    app.get("/api/v1/users/:id", (ptr req) => {
        let ctx = Context.create(req);
        let id = ctx.param("id");

        // Resolve service from IoC container
        let service = container.resolve("user_service") as UserService;
        let userJson = service.getUser(id);

        return ctx.json("{\\"id\\":\\"" + id + "\\",\\"data\\":\\"" + userJson + "\\"}");
    });

    app.listen(8080);
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 3.0 Backend Platform", href: "/backend/overview" },
          { title: "Dependency Injection (std.web.di)" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Boxes className="w-3.5 h-3.5" />
          <span>IoC Architecture &bull; std.web.di</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Dependency Injection Container
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          The <code>std.web.di</code> container decouples application route handlers from concrete infrastructure implementations. Register database pools, HTTP clients, and domain repositories once and resolve them safely inside routes.
        </p>
      </div>

      <Callout type="tip" title="Decoupled Clean Architecture">
        Dependency injection facilitates 100% testable code: during unit testing, swap production database pools or external payment APIs with in-memory test doubles without altering route handlers.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">IoC Container Setup &amp; Resolution</h2>
        <p className="text-xs text-slate-400 mb-3">
          Register singleton values and factory builders, then resolve them inside request handlers:
        </p>
        <CodeBlock code={diExampleCode} filename="src/di_app.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Container Methods</h3>
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
                <td className="py-2 text-cyan-400">container.bind(key, factoryFn)</td>
                <td className="py-2">Registers a lazy factory closure evaluated each time the dependency is resolved</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">container.bindValue(key, instance)</td>
                <td className="py-2">Registers an existing singleton object instance (e.g. database pool, config)</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">container.resolve(key)</td>
                <td className="py-2">Executes registered factory and returns the resolved object</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">container.resolveValue(key)</td>
                <td className="py-2">Returns bound singleton value without invoking factory</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">container.has(key)</td>
                <td className="py-2">Returns <code>true</code> if dependency key is registered</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Context & Ergonomics", href: "/backend/context" }}
        next={{ title: "OpenAPI 3.0 & Swagger UI", href: "/backend/openapi" }}
      />
    </div>
  );
}
