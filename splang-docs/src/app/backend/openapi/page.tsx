import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { FileCode, Globe, Terminal, CheckCircle2 } from "lucide-react";

export const metadata = {
  title: "OpenAPI 3.0 & Swagger UI",
  description: "Automated OpenAPI 3.0.3 generation, embedded Swagger UI, and Redoc in SP 3.0.",
};

export default function BackendOpenApiPage() {
  const annotatedRouteCode = `import std.web;

let app = new App();

// Interactive Documentation Viewers
app.swagger("/docs", "/openapi.json", "Storefront API");
app.redoc("/redoc", "/openapi.json", "Storefront API");

/// @route GET /api/v1/products/{id}
/// @summary Get product details by ID
/// @tag Products
/// @param id path integer Product primary ID
/// @response 200 Product fetched successfully
/// @response 404 Product not found
app.get("/api/v1/products/:id", (ptr req) => {
    let ctx = Context.create(req);
    let id = ctx.param("id");
    return ctx.json("{\\"id\\":\\"" + id + "\\",\\"name\\":\\"Mechanical Keyboard\\"}");
});

/// @route POST /api/v1/products
/// @summary Create a new catalog product
/// @tag Products
/// @response 201 Product created
/// @response 400 Invalid product schema
app.post("/api/v1/products", (ptr req) => {
    let ctx = Context.create(req);
    return ctx.json("{\\"created\\":true}", 201);
});

app.listen(8080);`;

  const cliCommands = `# 1. Generate OpenAPI 3.0.3 Specification (JSON)
spc doc src/main.sp --openapi -o openapi.json

# 2. Generate Standalone Swagger UI HTML
spc doc src/main.sp --swagger -o docs/

# 3. Generate Standalone Redoc HTML
spc doc src/main.sp --redoc -o docs/`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 3.0 Backend Platform", href: "/backend/overview" },
          { title: "OpenAPI 3.0 & Swagger UI" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Globe className="w-3.5 h-3.5" />
          <span>Automated API Specs &bull; OpenAPI 3.0.3</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          OpenAPI 3.0 &amp; Interactive Documentation
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          SP 3.0 introduces native OpenAPI 3.0.3 introspection. Generate contract specifications directly from your code comments and route definitions, and serve interactive Swagger UI and Redoc directly from your running binary.
        </p>
      </div>

      <Callout type="tip" title="Zero Dependencies">
        The Swagger UI and Redoc viewer endpoints are served directly by the native binary with CDN asset fallbacks. No external Node.js dependencies, npm packages, or build plugins are required.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Code Annotations &amp; Embedded UI</h2>
        <p className="text-xs text-slate-400 mb-3">
          Annotate routes with doc tags to auto-populate summaries, parameters, tags, and status codes:
        </p>
        <CodeBlock code={annotatedRouteCode} filename="src/main.sp" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">CLI Generation Commands</h2>
        <p className="text-xs text-slate-400 mb-3">
          Generate static contract documents for CI/CD pipelines, SDK generation, and developer portals:
        </p>
        <CodeBlock code={cliCommands} filename="Terminal" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Supported Doc Comment Directives</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="border-b border-slate-800 text-slate-400">
              <tr>
                <th className="pb-2 font-sans font-semibold">Directive</th>
                <th className="pb-2 font-sans font-semibold">Example</th>
                <th className="pb-2 font-sans font-semibold">Description</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 text-slate-300">
              <tr>
                <td className="py-2 text-cyan-400">/// @route</td>
                <td className="py-2"><code>/// @route GET /users/&#123;id&#125;</code></td>
                <td className="py-2">Explicit HTTP verb and path mapping</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">/// @summary</td>
                <td className="py-2"><code>/// @summary Fetch user by ID</code></td>
                <td className="py-2">Short summary for endpoint title in Swagger UI</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">/// @tag</td>
                <td className="py-2"><code>/// @tag Users</code></td>
                <td className="py-2">Groups endpoints into categorized navigation accordion</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">/// @param</td>
                <td className="py-2"><code>/// @param id path integer User ID</code></td>
                <td className="py-2">Defines name, location (path/query/header), type, and description</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">/// @response</td>
                <td className="py-2"><code>/// @response 200 Success</code></td>
                <td className="py-2">Document expected HTTP status code and response contract</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Dependency Injection", href: "/backend/di" }}
        next={{ title: "In-Memory HTTP Testing", href: "/backend/testing" }}
      />
    </div>
  );
}
