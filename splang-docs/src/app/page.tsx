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

      <Callout type="tip" title="SP 1.0.0 Production Release">
        SP 1.0.0 is officially released with LLVM native compilation, a standard library containing 8 modules, a built-in package manager, and full VS Code integration.
      </Callout>

      {/* Quick Start Card Grid */}
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4">
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
            <span>Standard Library</span>
            <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
          </h3>
          <p className="text-xs text-slate-400 mt-1 leading-relaxed">
            Explore 8 core native modules: I/O, Strings, Math, Collections, FS, and OS.
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
            Master the spc compiler, spm package manager, formatter, and debugger.
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
