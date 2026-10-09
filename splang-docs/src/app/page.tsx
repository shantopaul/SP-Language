import React from "react";
import Link from "next/link";
import { BookOpen, Terminal, Code2, Boxes, Sparkles, ArrowRight, ShieldCheck, Flame } from "lucide-react";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { Callout } from "@/components/ui/Callout";

export default function DocsLandingPage() {
  const quickHello = `import std.io;

fn main() -> void {
    println("Hello, SP World!");
}`;

  return (
    <div className="space-y-10">
      {/* Hero Banner */}
      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <BookOpen className="w-3.5 h-3.5" />
          <span>Official SP Documentation</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight leading-tight">
          SP Language Documentation
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          Welcome to the official developer documentation for the SP Programming Language. Here you will find comprehensive guides, grammar references, standard library documentation, and tooling manuals.
        </p>
      </div>

      <Callout type="tip" title="SP 3.0 Enterprise Backend Platform GA Now Available">
        <p>
          SP 3.0 is officially released! Featuring the official <strong>Web Framework</strong> (<code>std.web</code>), <strong>Radix Tree Routing</strong>, <strong>In-Memory HTTP Testing</strong> (<code>std.test.http</code>), <strong>Database Pooling</strong> (SQLite &amp; PostgreSQL), <strong>Redis Caching &amp; Pub/Sub</strong>, <strong>RFC 6455 WebSockets</strong>, <strong>Argon2id &amp; JWT Security</strong>, <strong>Prometheus Metrics</strong>, and <strong>Automated OpenAPI 3.0 Docs</strong>.{" "}
          <Link href="/backend/overview" className="underline font-semibold hover:text-white">
            Explore the SP 3.0 Backend Platform &rarr;
          </Link>
        </p>
      </Callout>

      {/* Quick Start Card Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
        <Link
          href="/backend/overview"
          className="p-5 rounded-xl border border-cyan-800/60 bg-cyan-950/20 hover:bg-cyan-950/40 hover:border-cyan-500 transition-all group sm:col-span-2"
        >
          <div className="w-10 h-10 rounded-lg bg-cyan-900/60 border border-cyan-700/60 flex items-center justify-center text-cyan-400 mb-3">
            <Sparkles className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>SP 3.0 Enterprise Backend Platform</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-300 mt-1 leading-relaxed">
            Build production-grade web services, microservices, and real-time APIs sustaining 185k req/s with sub-microsecond routing, SQL pooling, and zero-socket integration testing.
          </p>
        </Link>

        <Link
          href="/getting-started/installation"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-emerald-950/60 border border-emerald-800/60 flex items-center justify-center text-emerald-400 mb-3">
            <Terminal className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>Installation Guide</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Install the SP toolchain on Windows, Linux, or macOS in under a minute.
          </p>
        </Link>

        <Link
          href="/syntax-guide"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-cyan-950/60 border border-cyan-800/60 flex items-center justify-center text-cyan-400 mb-3">
            <Code2 className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>Language Tour</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Learn the syntax: variables, control flow, functions, OOP, and generics.
          </p>
        </Link>

        <Link
          href="/stdlib"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-blue-950/60 border border-blue-800/60 flex items-center justify-center text-blue-400 mb-3">
            <Boxes className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>Standard Library (17 Modules)</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Explore 17 native modules: Web, DB, Redis, Crypto, WebSockets, Logging, and Metrics.
          </p>
        </Link>

        <Link
          href="/tools/compiler-cli"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-purple-950/60 border border-purple-800/60 flex items-center justify-center text-purple-400 mb-3">
            <Terminal className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>Compiler & Toolchain</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Master the spc compiler, spm package manager, formatter, test runner, and debugger.
          </p>
        </Link>
      </div>

      {/* Quick Hello World Preview */}
      <div>
        <h2 className="text-xl font-bold text-white mb-2">Hello World in SP</h2>
        <p className="text-xs text-slate-400 mb-3">
          Save this to <code>hello.sp</code> and run with <code>spc run hello.sp</code>:
        </p>
        <CodeBlock code={quickHello} filename="hello.sp" />
      </div>
    </div>
  );
}
