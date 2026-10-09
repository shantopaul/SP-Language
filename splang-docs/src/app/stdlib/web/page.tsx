import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.web Module Reference",
  description: "Official web application framework, HTTP routing, and context ergonomics in SP.",
};

export default function StdWebPage() {
  const exampleCode = `import std.web;

fn main() {
    let app = new App();
    app.use(new Logger());
    app.use(new Cors());

    app.get("/api/v1/ping", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\\"status\\":\\"ok\\"}");
    });

    app.listen(8080);
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.web" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.web Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.web</code> module provides the core high-level backend application framework, Radix tree HTTP routing, middleware pipeline, and the ergonomic <code>Context</code> abstraction.
      </p>

      <CodeBlock code={exampleCode} filename="web_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Core Classes &amp; APIs</h2>

      <ApiCard
        name="App"
        signature="class App"
        description="Core web application instance managing route table, middlewares, and socket server."
        returns="App instance"
      />

      <ApiCard
        name="Context.create"
        signature="static fn create(ptr rawReq) -> Context"
        description="Creates an ergonomic Context wrapper around the low-level native HTTP request."
        parameters={[{ name: "rawReq", type: "ptr", description: "Pointer to the native request handle." }]}
        returns="Context instance"
      />

      <ApiCard
        name="ctx.param"
        signature="fn param(string name) -> string"
        description="Extracts named path parameter (e.g., :id) with sub-microsecond zero-copy lookup."
        parameters={[{ name: "name", type: "string", description: "Parameter name without colon." }]}
        returns="string value of the parameter"
      />

      <ApiCard
        name="ctx.json"
        signature="fn json(string jsonStr, int status = 200) -> string"
        description="Constructs an HTTP response with Content-Type: application/json."
        parameters={[
          { name: "jsonStr", type: "string", description: "Valid JSON payload string." },
          { name: "status", type: "int", description: "HTTP status code (default: 200)." },
        ]}
        returns="Formatted HTTP response string"
      />

      <PrevNextNav
        prev={{ title: "Stdlib Overview", href: "/stdlib" }}
        next={{ title: "std.db Reference", href: "/stdlib/db" }}
      />
    </div>
  );
}
