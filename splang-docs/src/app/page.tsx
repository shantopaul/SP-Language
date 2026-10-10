import React from "react";
import Link from "next/link";
import Image from "next/image";
import { BookOpen, Terminal, Code2, Boxes, Sparkles, ArrowRight, ShieldCheck, Flame, Cpu, Zap } from "lucide-react";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { Callout } from "@/components/ui/Callout";

export default function DocsLandingPage() {
  const quickHello = `import std.io;

fn main() -> void {
    println("Hello, SP World!");
}`;

  const quickWeb = `import std.web;

fn handle_root(ctx: &web.Context) -> web.Response {
    return ctx.json({"status": "healthy", "engine": "SP std.web", "version": "3.1.0"});
}

fn main() -> void {
    let app = web.new_server();
    app.get("/health", handle_root);
    app.listen(8080);
}`;

  return (
    <div className="space-y-12 relative">
      {/* Hero Banner */}
      <div className="relative">
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-blue-50 dark:bg-[#0057FF]/15 border border-blue-200 dark:border-[#00D1FF]/30 text-[#0057FF] dark:text-[#00D1FF] text-xs font-semibold mb-4 shadow-sm dark:shadow-[0_0_15px_rgba(0,209,255,0.15)]">
          <BookOpen className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF]" />
          <span>Official SP Documentation (v3.1.0 GA)</span>
        </div>
        
        <h1 className="text-3xl sm:text-5xl font-black text-slate-900 dark:text-white tracking-tight leading-tight">
          SP Language <span className="text-transparent bg-clip-text bg-gradient-to-r from-blue-600 via-sky-600 to-[#0057FF] dark:from-white dark:via-slate-100 dark:to-[#00D1FF]">Documentation</span>
        </h1>
        
        <p className="mt-4 text-base sm:text-lg text-slate-600 dark:text-slate-300 leading-relaxed max-w-3xl">
          Welcome to the official developer documentation for the SP Programming Language. Explore comprehensive language tours, standard library APIs, enterprise web platform guides, and native LLVM toolchain manuals.
        </p>
      </div>

      <Callout type="tip" title="SP 3.1.0 Enterprise Backend Platform GA Now Available">
        <p>
          SP 3.1.0 GA is officially released! Featuring the production <strong>Web Framework</strong> (<code>std.web</code> delivering 185k req/s), <strong>Database Connection Pooling</strong> (SQLite &amp; PostgreSQL with static TLS), <strong>Redis Caching &amp; Pub/Sub</strong>, <strong>RFC 6455 WebSockets</strong>, <strong>Argon2id &amp; JWT Security</strong>, <strong>Prometheus Metrics</strong>, <strong>Universal Auto-Upgrade</strong> (<code>spc upgrade</code>), and <strong>Diagnostic Toolchain Doctor</strong> (<code>spc doctor</code>).{" "}
          <Link href="/backend/overview" className="text-[#0057FF] dark:text-[#00D1FF] underline font-semibold hover:text-blue-700 dark:hover:text-white">
            Explore the SP 3.1 Backend Platform &rarr;
          </Link>
        </p>
      </Callout>

      {/* Quick Start Card Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
        <Link
          href="/backend/overview"
          className="p-5 rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 hover:bg-slate-50 dark:hover:bg-[#081735] hover:border-blue-300 dark:hover:border-[#00D1FF]/50 transition-all group sm:col-span-2 shadow-sm dark:shadow-xl relative overflow-hidden"
        >
          <div className="w-10 h-10 rounded-xl bg-blue-50 dark:bg-[#0057FF]/20 border border-blue-200 dark:border-[#00D1FF]/30 flex items-center justify-center text-[#0057FF] dark:text-[#00D1FF] mb-3 shadow-sm dark:shadow-[0_0_15px_rgba(0,209,255,0.2)]">
            <Sparkles className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>SP 3.1 Enterprise Backend Platform</span>
            <ArrowRight className="w-4 h-4 text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-300 mt-1 leading-relaxed">
            Build production-grade microservices and real-time APIs sustaining 185,000 req/s with sub-microsecond routing, SQL connection pooling, static OpenSSL TLS, and zero-socket integration testing.
          </p>
        </Link>

        <Link
          href="/getting-started/installation"
          className="p-5 rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/50 hover:bg-slate-50 dark:hover:bg-[#081735] hover:border-blue-300 dark:hover:border-[#00D1FF]/40 transition-all group shadow-sm dark:shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-blue-50 dark:bg-[#0057FF]/15 border border-blue-200 dark:border-[#00D1FF]/30 flex items-center justify-center text-[#0057FF] dark:text-[#00D1FF] mb-3">
            <Terminal className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Installation Guide</span>
            <ArrowRight className="w-4 h-4 text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-600 dark:text-slate-400 mt-1 leading-relaxed">
            Install SP on Windows (1-click PowerShell), Linux, or macOS with automated multi-drive detection and toolchain verification.
          </p>
        </Link>

        <Link
          href="/syntax-guide"
          className="p-5 rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/50 hover:bg-slate-50 dark:hover:bg-[#081735] hover:border-blue-300 dark:hover:border-[#00D1FF]/40 transition-all group shadow-sm dark:shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-blue-50 dark:bg-[#0057FF]/15 border border-blue-200 dark:border-[#00D1FF]/30 flex items-center justify-center text-[#0057FF] dark:text-[#00D1FF] mb-3">
            <Code2 className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Language Tour &amp; Syntax</span>
            <ArrowRight className="w-4 h-4 text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-600 dark:text-slate-400 mt-1 leading-relaxed">
            Master the syntax: variables, control flow, functions, OOP, zero-cost generics, and deterministic ARC memory management.
          </p>
        </Link>

        <Link
          href="/stdlib"
          className="p-5 rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/50 hover:bg-slate-50 dark:hover:bg-[#081735] hover:border-blue-300 dark:hover:border-[#00D1FF]/40 transition-all group shadow-sm dark:shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-blue-50 dark:bg-[#0057FF]/15 border border-blue-200 dark:border-[#00D1FF]/30 flex items-center justify-center text-[#0057FF] dark:text-[#00D1FF] mb-3">
            <Boxes className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Standard Library (23 Modules)</span>
            <ArrowRight className="w-4 h-4 text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-600 dark:text-slate-400 mt-1 leading-relaxed">
            Explore all 23 native modules: Web, DB, Redis, Crypto, WebSockets, Metrics, Health, IO, Sync, and FS.
          </p>
        </Link>

        <Link
          href="/tools/compiler-cli"
          className="p-5 rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/50 hover:bg-slate-50 dark:hover:bg-[#081735] hover:border-blue-300 dark:hover:border-[#00D1FF]/40 transition-all group shadow-sm dark:shadow-lg"
        >
          <div className="w-10 h-10 rounded-xl bg-blue-50 dark:bg-[#0057FF]/15 border border-blue-200 dark:border-[#00D1FF]/30 flex items-center justify-center text-[#0057FF] dark:text-[#00D1FF] mb-3">
            <Cpu className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
            <span>Compiler &amp; Toolchain (spc)</span>
            <ArrowRight className="w-4 h-4 text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-600 dark:text-slate-400 mt-1 leading-relaxed">
            Complete CLI manual: spc build, spc run, spc upgrade, spc doctor, spc fmt, spc test, and spm package manager.
          </p>
        </Link>
      </div>

      {/* Code Examples Previews */}
      <div className="space-y-6 pt-4">
        <div>
          <h2 className="text-xl font-bold text-slate-900 dark:text-white mb-1">Hello World in SP</h2>
          <p className="text-xs text-slate-600 dark:text-slate-400 mb-3">
            Save this to <code>hello.sp</code> and execute natively with <code>spc run hello.sp</code>:
          </p>
          <CodeBlock code={quickHello} filename="hello.sp" />
        </div>

        <div>
          <h2 className="text-xl font-bold text-slate-900 dark:text-white mb-1">Production Web Server with std.web</h2>
          <p className="text-xs text-slate-600 dark:text-slate-400 mb-3">
            Build and deploy microservices with zero external dependencies:
          </p>
          <CodeBlock code={quickWeb} filename="server.sp" />
        </div>
      </div>
    </div>
  );
}
