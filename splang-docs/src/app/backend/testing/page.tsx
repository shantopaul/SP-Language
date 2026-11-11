import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { CheckCircle2, Zap, ShieldCheck } from "lucide-react";

export const metadata = {
  title: "In-Memory HTTP Testing (std.test.http)",
  description: "Zero-socket, deterministic HTTP integration testing in SP 4.0.",
};

export default function BackendTestingPage() {
  const testExampleCode = `module tests.user_api_test;

import std.web;
import std.test.http;

test "User API Endpoint Integration Verification" {
    let app = new App();

    // Register middlewares & routes
    app.use(new Recovery());
    app.get("/api/v1/users/:id", (ptr req) => {
        let ctx = Context.create(req);
        let id = ctx.param("id");
        return ctx.json("{\\"id\\":\\"" + id + "\\",\\"status\\":\\"active\\"}");
    });

    app.post("/api/v1/users", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\\"created\\":true}", 201);
    });

    // 1. Create In-Memory Test Client (Zero network sockets allocated!)
    let client = TestClient.fromApp(app);

    // 2. Perform GET Request with fluent assertions
    let res = client.get("/api/v1/users/108");
    res.assertOk()
       .assertHeader("Content-Type", "application/json")
       .assertBodyContains("status")
       .assertBodyContains("108");
    res.destroy();

    // 3. Perform POST Request with body
    let postRes = client.post("/api/v1/users", "{\\"name\\":\\"Charlie\\"}");
    postRes.assertCreated()
           .assertBodyContains("created");
    postRes.destroy();

    // 4. Test 404 Route handling
    let notFoundRes = client.get("/api/unknown");
    notFoundRes.assertNotFound();
    notFoundRes.destroy();

    client.destroy();
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 4.0 Backend Platform", href: "/backend/overview" },
          { title: "In-Memory HTTP Testing (std.test.http)" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <CheckCircle2 className="w-3.5 h-3.5" />
          <span>Zero-Socket Test Harness &bull; std.test.http</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          In-Memory HTTP Testing Harness
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          Traditional web integration tests suffer from port collisions, slow TCP handshakes, and firewall alerts. SP's <code>std.test.http</code> harness invokes the entire HTTP server pipeline directly in-memory, running end-to-end integration tests in under <strong>1 millisecond</strong>.
        </p>
      </div>

      <Callout type="tip" title="Deterministic &amp; Fast">
        Because tests do not open OS network sockets, you can run thousands of HTTP integration tests concurrently across all CPU cores without port exhaustion or socket timeouts.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Fluent Integration Test Example</h2>
        <p className="text-xs text-slate-400 mb-3">
          Create a <code>TestClient</code> from your <code>App</code> instance and chain assertions fluently:
        </p>
        <CodeBlock code={testExampleCode} filename="tests/user_api_test.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Fluent Assertion Methods</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="border-b border-slate-800 text-slate-400">
              <tr>
                <th className="pb-2 font-sans font-semibold">Assertion Method</th>
                <th className="pb-2 font-sans font-semibold">Expected Behavior</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 text-slate-300">
              <tr>
                <td className="py-2 text-cyan-400">res.assertOk()</td>
                <td className="py-2">Asserts HTTP status code is <code>200 OK</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertCreated()</td>
                <td className="py-2">Asserts HTTP status code is <code>201 Created</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertNoContent()</td>
                <td className="py-2">Asserts HTTP status code is <code>204 No Content</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertBadRequest()</td>
                <td className="py-2">Asserts HTTP status code is <code>400 Bad Request</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertUnauthorized()</td>
                <td className="py-2">Asserts HTTP status code is <code>401 Unauthorized</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertForbidden()</td>
                <td className="py-2">Asserts HTTP status code is <code>403 Forbidden</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertNotFound()</td>
                <td className="py-2">Asserts HTTP status code is <code>404 Not Found</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertInternalError()</td>
                <td className="py-2">Asserts HTTP status code is <code>500 Internal Server Error</code></td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertStatus(expectedCode)</td>
                <td className="py-2">Asserts specific custom HTTP status code</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertBody(expectedString)</td>
                <td className="py-2">Exact matching on response body contents</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertBodyContains(subString)</td>
                <td className="py-2">Sub-string containment check in response body</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertHeader(name, value)</td>
                <td className="py-2">Case-insensitive header key and value verification</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">res.assertHeaderPresent(name)</td>
                <td className="py-2">Verifies presence of response header regardless of value</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "OpenAPI 3.0 & Swagger UI", href: "/backend/openapi" }}
        next={{ title: "Database & Pooling (std.db)", href: "/backend/database" }}
      />
    </div>
  );
}
