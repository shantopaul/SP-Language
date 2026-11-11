import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Server, Shield, Layers, FastForward } from "lucide-react";

export const metadata = {
  title: "Web Framework (std.web.app)",
  description: "High-level backend application framework with Radix tree routing and middleware pipeline in SP.",
};

export default function WebFrameworkPage() {
  const basicAppCode = `module main;

import std.io;
import std.web;

fn main() {
    let app = new App();

    // Register global middlewares
    app.use(new Logger());
    app.use(new Cors());
    app.use(new Recovery());

    // Basic route handlers
    app.get("/", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.text("Welcome to the SP Web Platform!");
    });

    app.get("/api/ping", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\\"pong\\":true}");
    });

    // Start server on port 8080
    app.listen(8080);
}`;

  const routeGroupsCode = `// Route Groups for modular API design
let api = app.group("/api/v1");

// Attach group-specific middleware
api.use(new RateLimiter(100, 60)); // 100 requests per minute

api.get("/status", (ptr req) => {
    let ctx = Context.create(req);
    return ctx.json("{\\"status\\":\\"operational\\"}");
});

// Nested groups
let users = api.group("/users");

users.get("/", (ptr req) => {
    let ctx = Context.create(req);
    return ctx.json("[{\\"id\\":1,\\"name\\":\\"Alice\\"},{\\"id\\":2,\\"name\\":\\"Bob\\"}]");
});

users.get("/:id", (ptr req) => {
    let ctx = Context.create(req);
    let userId = ctx.param("id");
    return ctx.json("{\\"id\\":\\"" + userId + "\\"}");
});`;

  const middlewareCode = `// Built-in Production Middlewares

// 1. Request Logger with response timing
app.use(new Logger());

// 2. CORS configuration (Cross-Origin Resource Sharing)
let cors = new Cors();
cors.allowOrigin("*");
cors.allowMethods("GET, POST, PUT, DELETE, OPTIONS");
cors.allowHeaders("Content-Type, Authorization");
app.use(cors);

// 3. Crash Recovery (Catches panic/unhandled exceptions & returns 500)
app.use(new Recovery());

// 4. In-Memory Token Bucket Rate Limiting
app.use(new RateLimiter(500, 60)); // 500 requests per 60 seconds`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 4.0 Backend Platform", href: "/backend/overview" },
          { title: "Web Framework (std.web.app)" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Server className="w-3.5 h-3.5" />
          <span>Core Framework &bull; std.web.app</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Web Framework (std.web)
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          The <code>std.web</code> framework delivers modern, clean-architecture web service development. It combines zero-allocation Radix tree routing with high-level composable middlewares and sub-microsecond HTTP dispatch.
        </p>
      </div>

      <Callout type="tip" title="Sub-Microsecond Routing">
        SP's HTTP router uses a lock-free prefix radix tree that matches parameterized paths (e.g. <code>/api/v1/users/:id/orders/:orderId</code>) in under <strong>45 nanoseconds</strong> with zero heap allocations during the dispatch phase.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Creating an Application</h2>
        <p className="text-xs text-slate-400 mb-3">
          Instantiate the <code>App</code> class, attach middlewares, declare route handlers, and start listening:
        </p>
        <CodeBlock code={basicAppCode} filename="src/main.sp" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Route Groups &amp; Scoping</h2>
        <p className="text-xs text-slate-400 mb-3">
          Route groups allow grouping common path prefixes and applying middlewares only to specific subsets of routes:
        </p>
        <CodeBlock code={routeGroupsCode} filename="src/routes/api.sp" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Built-in Middlewares</h2>
        <p className="text-xs text-slate-400 mb-3">
          SP ships with production-grade middlewares ready out-of-the-box:
        </p>
        <CodeBlock code={middlewareCode} filename="src/middleware.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">App Class Methods</h3>
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
                <td className="py-2 text-cyan-400">app.use(middleware)</td>
                <td className="py-2">Mounts global middleware in the execution chain</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.get(path, handler)</td>
                <td className="py-2">Registers HTTP GET handler</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.post(path, handler)</td>
                <td className="py-2">Registers HTTP POST handler</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.put(path, handler)</td>
                <td className="py-2">Registers HTTP PUT handler</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.delete(path, handler)</td>
                <td className="py-2">Registers HTTP DELETE handler</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.group(prefix)</td>
                <td className="py-2">Creates a new prefixed route group</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.swagger(path, specUrl, title)</td>
                <td className="py-2">Mounts interactive Swagger UI documentation at path</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.redoc(path, specUrl, title)</td>
                <td className="py-2">Mounts interactive Redoc documentation at path</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">app.listen(port)</td>
                <td className="py-2">Binds socket and starts concurrent threadpool event loop</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Backend Overview", href: "/backend/overview" }}
        next={{ title: "Context & Ergonomics (ctx)", href: "/backend/context" }}
      />
    </div>
  );
}
