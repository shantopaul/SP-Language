import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Layers, ArrowRight, Code, Key } from "lucide-react";

export const metadata = {
  title: "HTTP Context & Request Ergonomics (ctx)",
  description: "Ergonomic HTTP Request and Response Context API in SP 4.0 (std.web.context).",
};

export default function BackendContextPage() {
  const contextExampleCode = `import std.web;

app.post("/api/v1/posts/:category", (ptr req) => {
    let ctx = Context.create(req);

    // 1. Radix-tree Path Parameter
    let category = ctx.param("category");

    // 2. Query String Parameter (?author=alice)
    let author = ctx.query("author");

    // 3. HTTP Request Header lookup (case-insensitive)
    let authHeader = ctx.header("Authorization");
    if (authHeader == "") {
        return ctx.json("{\\"error\\":\\"Missing Authorization Header\\"}", 401);
    }

    // 4. Request Body reading
    let bodyJson = ctx.body();

    // 5. State sharing from middleware
    let currentUserId = ctx.get("user_id");

    // 6. Set response cookie
    ctx.setCookie("session_track", "active", 3600, true);

    // 7. Typed JSON Response (201 Created)
    return ctx.json("{\\"id\\":42,\\"category\\":\\"" + category + "\\",\\"author\\":\\"" + author + "\\"}", 201);
});`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 4.0 Backend Platform", href: "/backend/overview" },
          { title: "Context & Ergonomics (ctx)" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Layers className="w-3.5 h-3.5" />
          <span>Ergonomics &bull; std.web.context</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          HTTP Context &amp; Request Ergonomics
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          The <code>Context</code> (commonly named <code>ctx</code>) provides an ergonomic facade wrapping the raw native HTTP request and response streams, delivering sub-microsecond parameter lookups and fluent response helpers.
        </p>
      </div>

      <Callout type="tip" title="Zero-Copy Parameter Extraction">
        Path parameters (e.g. <code>:category</code>, <code>:id</code>) and query string values are extracted using zero-copy string views directly mapped into the request buffer, minimizing garbage collection and memory copies.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Context in Action</h2>
        <p className="text-xs text-slate-400 mb-3">
          Comprehensive handler utilizing parameters, headers, cookies, state sharing, and status codes:
        </p>
        <CodeBlock code={contextExampleCode} filename="src/routes/posts.sp" />
      </div>

      <div className="space-y-6">
        <h2 className="text-xl font-bold text-white">Context API Reference</h2>

        <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
          <h3 className="text-base font-bold text-white mb-3">Request Helpers</h3>
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
                  <td className="py-2 text-cyan-400">ctx.param(name: string)</td>
                  <td className="py-2">Returns matched path parameter from route pattern (e.g. <code>:id</code>)</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.query(name: string)</td>
                  <td className="py-2">Returns query string parameter (e.g. <code>?page=2</code>) or empty string</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.header(name: string)</td>
                  <td className="py-2">Case-insensitive HTTP request header lookup</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.cookie(name: string)</td>
                  <td className="py-2">Retrieves cookie value from incoming <code>Cookie</code> header</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.body()</td>
                  <td className="py-2">Returns complete raw request payload as string</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.method()</td>
                  <td className="py-2">Returns HTTP verb: <code>GET</code>, <code>POST</code>, <code>PUT</code>, etc.</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.path()</td>
                  <td className="py-2">Returns normalized request URL path</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>

        <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
          <h3 className="text-base font-bold text-white mb-3">Response Helpers</h3>
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
                  <td className="py-2 text-cyan-400">ctx.json(jsonStr, status=200)</td>
                  <td className="py-2">Sets <code>Content-Type: application/json</code> and returns response</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.text(str, status=200)</td>
                  <td className="py-2">Sets <code>Content-Type: text/plain; charset=utf-8</code> and returns response</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.html(htmlStr, status=200)</td>
                  <td className="py-2">Sets <code>Content-Type: text/html; charset=utf-8</code> and returns response</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.redirect(url, status=302)</td>
                  <td className="py-2">Emits <code>Location</code> header redirect response</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.status(code)</td>
                  <td className="py-2">Sets HTTP status code without immediate body return</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.setCookie(name, val, maxAge, secure)</td>
                  <td className="py-2">Appends <code>Set-Cookie</code> header with security attributes</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>

        <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
          <h3 className="text-base font-bold text-white mb-3">Context State Sharing</h3>
          <p className="text-xs text-slate-400 mb-3">
            Middlewares can store authenticated user IDs, tenant scopes, or request IDs for downstream handlers:
          </p>
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
                  <td className="py-2 text-cyan-400">ctx.set(key: string, val: string)</td>
                  <td className="py-2">Stores contextual key-value string in the request scope</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.get(key: string)</td>
                  <td className="py-2">Retrieves stored value or empty string if not present</td>
                </tr>
                <tr>
                  <td className="py-2 text-cyan-400">ctx.has(key: string)</td>
                  <td className="py-2">Returns <code>true</code> if the key exists in current context</td>
                </tr>
              </tbody>
            </table>
          </div>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Web Framework (std.web)", href: "/backend/framework" }}
        next={{ title: "Dependency Injection (Container)", href: "/backend/di" }}
      />
    </div>
  );
}
