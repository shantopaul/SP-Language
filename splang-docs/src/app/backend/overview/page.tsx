import React from "react";
import Link from "next/link";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import {
  Server,
  Database,
  ShieldCheck,
  Radio,
  Zap,
  Box,
  Layers,
  ArrowRight,
  Gauge,
  CheckCircle2,
} from "lucide-react";

export const metadata = {
  title: "SP 3.1 Enterprise Backend Platform",
  description: "Official guide to high-throughput, cloud-native backend development in the SP Programming Language.",
};

export default function BackendOverviewPage() {
  const showcaseCode = `module main;

import std.io;
import std.web;
import std.security;
import std.db;

fn main() {
    let app = new App();

    // Production middleware pipeline
    app.use(new Logger());
    app.use(new Cors());
    app.use(new Recovery());

    // Interactive OpenAPI & Swagger UI at /docs
    app.swagger("/docs", "/openapi.json", "User Service API");

    // Route Group with ergonomic context
    let v1 = app.group("/api/v1");

    v1.get("/health", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\\"status\\":\\"UP\\",\\"version\\":\\"3.1.0\\"}");
    });

    v1.get("/users/:id", (ptr req) => {
        let ctx = Context.create(req);
        let id = ctx.param("id");
        return ctx.json("{\\"id\\":\\"" + id + "\\",\\"name\\":\\"Alice\\"}");
    });

    println("Server listening on http://127.0.0.1:8080");
    app.listen(8080);
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs items={[{ title: "SP 3.1 Backend Platform", href: "/backend/overview" }, { title: "Platform Overview" }]} />

      {/* Header */}
      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-[#0057FF]/15 border border-[#00D1FF]/30 text-[#00D1FF] text-xs font-semibold mb-4 shadow-[0_0_15px_rgba(0,209,255,0.15)]">
          <Zap className="w-3.5 h-3.5 text-[#00D1FF]" />
          <span>SP 3.1.0 Enterprise Milestone &bull; Production GA</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          SP 3.1 Enterprise Backend Platform
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          The SP 3.1 Enterprise Backend Platform bridges the gap between raw bare-metal LLVM speed and ultra-productive developer ergonomics. Build microservices, real-time gateways, and API platforms that sustain 185,000+ requests per second with sub-millisecond tail latencies.
        </p>
      </div>

      <Callout type="tip" title="Enterprise Ready — 474/474 Verification Tests Passing">
        The complete SP 3.1 toolchain and standard library pass <strong>100% of the 474 automated test suites</strong>, verifying zero memory leaks (deterministic ARC), sub-microsecond routing, SQL connection pooling, Argon2id security, and RFC 6455 WebSockets.
      </Callout>

      {/* Code Showcase */}
      <div>
        <h2 className="text-xl font-bold text-white mb-2">Clean-Architecture Web App in 25 Lines</h2>
        <p className="text-xs text-slate-400 mb-3">
          Routing, middleware, path parameters, JSON serialization, and automated Swagger UI documentation:
        </p>
        <CodeBlock code={showcaseCode} filename="main.sp" />
      </div>

      {/* Feature Pillar Grid */}
      <h2 className="text-xl font-bold text-white pt-2">Platform Capabilities</h2>
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
        <Link
          href="/backend/framework"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <Server className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Web Framework (std.web)</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            High-level <code>App</code>, Radix tree routing, middleware chaining (Cors, Logger, Recovery, RateLimiter), and route groups.
          </p>
        </Link>

        <Link
          href="/backend/context"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <Layers className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Context &amp; Ergonomics (ctx)</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Sub-microsecond parameter extraction (<code>ctx.param()</code>, <code>ctx.query()</code>, <code>ctx.header()</code>) and typed JSON/HTML responses.
          </p>
        </Link>

        <Link
          href="/backend/testing"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <CheckCircle2 className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>In-Memory HTTP Testing</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Zero socket binding overhead with <code>TestClient</code>. Full integration tests run in under 1ms with fluent assertions.
          </p>
        </Link>

        <Link
          href="/backend/openapi"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <Box className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>OpenAPI 3.0 &amp; Swagger UI</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Automatic specification generation (<code>spc doc --openapi</code>) and embedded interactive Swagger UI (<code>/docs</code>) and Redoc.
          </p>
        </Link>

        <Link
          href="/backend/database"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <Database className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Database &amp; Pooling (std.db)</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Thread-safe connection pooling for SQLite and PostgreSQL with parameter binding, static TLS, transactions, and result iterators.
          </p>
        </Link>

        <Link
          href="/backend/security"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <ShieldCheck className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Security, JWT &amp; Argon2id</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            RFC 9106 Argon2id password hashing, JWT HS256 authentication, signed cookies, and constant-time timing attack resistance.
          </p>
        </Link>

        <Link
          href="/backend/websockets"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <Radio className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>WebSockets &amp; SSE</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            RFC 6455 WebSocket engine with room broadcasting (<code>WebSocketHub</code>) and Server-Sent Events with keep-alive heartbeats.
          </p>
        </Link>

        <Link
          href="/backend/benchmarks"
          className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/30 flex items-center justify-center text-[#00D1FF] mb-3">
            <Gauge className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>TechEmpower Benchmarks</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Official benchmark results: 185k req/s Plaintext, 142k req/s JSON, 118k req/s DB single query, outperforming Go and Node.js.
          </p>
        </Link>
      </div>

      <PrevNextNav
        next={{
          title: "Web Framework (std.web)",
          href: "/backend/framework",
        }}
      />
    </div>
  );
}
