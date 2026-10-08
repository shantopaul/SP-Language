import React from "react";
import Link from "next/link";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Sparkles, Cpu, Globe, Zap, ArrowRight, ShieldCheck, RefreshCw } from "lucide-react";

export const metadata = {
  title: "SP 2.0.0 Release Overview",
  description: "Comprehensive guide to SP 2.0.0 General Availability, key architectural evolutions, and new features.",
};

export default function Sp2OverviewPage() {
  const sampleCode = `import std.io;
import std.sync;

// 1. Compile-Time Metaprogramming
comptime fn computeLookup(int n) -> int {
    if (n <= 1) return 1;
    return n * computeLookup(n - 1);
}

// 2. Structured Asynchronous Coroutine
async fn processClusterMetrics(string clusterId) -> string {
    println("Polling telemetry nodes for \${clusterId}...");
    return "Cluster OK (p99 < 1.1ms)";
}

fn main() -> void {
    // Constant folded directly into LLVM IR immediate
    let factor = comptime computeLookup(8);
    println("Precalculated Factorial(8): \${factor}");

    // Cooperative coroutine dispatch
    let task = spawn processClusterMetrics("us-east-1");
    let status = await task;
    println("Telemetry Response: \${status}");
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs items={[{ title: "SP 2.0 Evolution", href: "/sp2/overview" }, { title: "Release Overview" }]} />

      {/* Hero Badge & Title */}
      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Sparkles className="w-3.5 h-3.5" />
          <span>Milestone Release &bull; October 2026</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          SP 2.0.0 General Availability
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          SP 2.0 marks the definitive milestone in the language’s evolution: combining bare-metal LLVM compilation, compile-time metaprogramming, structured asynchronous coroutines, first-class WebAssembly targeting, and forward-compatible Language Editions.
        </p>
      </div>

      <Callout type="tip" title="100% Continuous Verification Passing">
        The SP 2.0 toolchain passes all <strong>346 out of 346 CTest continuous integration test suites (100% pass rate)</strong> across compiler IR lowering, ARC memory safety, standard library modules, WASI targets, and language server capabilities.
      </Callout>

      {/* Code Preview */}
      <div>
        <h2 className="text-xl font-bold text-white mb-2">SP 2.0 Feature Preview</h2>
        <p className="text-xs text-slate-400 mb-3">
          Comptime constant folding and structured asynchronous coroutines working seamlessly together:
        </p>
        <CodeBlock code={sampleCode} filename="sp2_demo.sp" />
      </div>

      {/* Feature Pillar Grid */}
      <h2 className="text-xl font-bold text-white pt-2">Key Architectural Pillars</h2>
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
        <Link
          href="/sp2/editions"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-emerald-950/60 border border-emerald-800/60 flex items-center justify-center text-emerald-400 mb-3">
            <RefreshCw className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>Language Editions</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Evolve codebases without breaking existing modules. Seamlessly mix <code>2026</code> and <code>2028</code> editions in a single build.
          </p>
        </Link>

        <Link
          href="/sp2/comptime"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-purple-950/60 border border-purple-800/60 flex items-center justify-center text-purple-400 mb-3">
            <Zap className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>Compile-Time Execution (comptime)</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Execute arbitrary deterministic functions at build time. Constant fold heavy calculations directly into LLVM IR immediate values.
          </p>
        </Link>

        <Link
          href="/sp2/wasm"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-blue-950/60 border border-blue-800/60 flex items-center justify-center text-blue-400 mb-3">
            <Globe className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>WebAssembly (wasm32-wasi)</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            First-class cross-compilation target. Deploy sandboxed SP binaries onto Cloudflare Workers, Fastly Compute, Wasmer, and browsers.
          </p>
        </Link>

        <Link
          href="/sp2/async-await"
          className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
        >
          <div className="w-10 h-10 rounded-lg bg-cyan-950/60 border border-cyan-800/60 flex items-center justify-center text-cyan-400 mb-3">
            <Cpu className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white group-hover:text-cyan-400 transition-colors flex items-center justify-between">
            <span>Structured Coroutines</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Cooperative non-blocking multitasking using <code>async</code>, <code>await</code>, and <code>spawn</code> with zero OS thread overhead.
          </p>
        </Link>
      </div>

      {/* Migration Notice */}
      <div className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
        <div>
          <h3 className="text-base font-bold text-white">Upgrading from SP 1.x?</h3>
          <p className="text-xs text-slate-400 mt-1">
            Use the automated refactoring tool <code>spc fix</code> to seamlessly modernize your codebase with zero manual overhead.
          </p>
        </div>
        <Link
          href="/sp2/migration"
          className="inline-flex items-center gap-2 px-4 py-2 rounded-lg bg-cyan-600 hover:bg-cyan-500 text-white font-semibold text-xs transition-colors shrink-0"
        >
          <span>Migration Guide</span>
          <ArrowRight className="w-3.5 h-3.5" />
        </Link>
      </div>

      <PrevNextNav
        next={{
          title: "Language Editions (2026/2028)",
          href: "/sp2/editions",
        }}
      />
    </div>
  );
}
