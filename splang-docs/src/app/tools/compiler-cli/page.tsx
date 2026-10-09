import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Compiler CLI (spc)",
  description: "Command-line interface reference for the SP native compiler (spc).",
};

export default function CompilerCliPage() {
  const basicUsage = `# 1. Run file directly
spc run main.sp

# 2. Compile native executable with -O3
spc build main.sp -o my_app -O3

# 3. Check syntax and types without linking
spc check main.sp

# 4. Cross-compile for target triple
spc build main.sp -o my_app_linux --target x86_64-unknown-linux-gnu`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Developer Tools", href: "/tools" },
          { title: "Compiler CLI (spc)" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">SP Compiler CLI (`spc`)</h1>
      <p className="text-sm text-slate-300">
        The <code>spc</code> binary is the primary native compiler executable. It coordinates parsing, semantic type analysis, LLVM IR translation, and Clang machine code linking.
      </p>

      <CodeBlock code={basicUsage} language="bash" />

      <h2 className="text-xl font-bold text-white pt-4">Subcommands</h2>
      <div className="space-y-3 text-xs sm:text-sm">
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc build &lt;file&gt; [-o &lt;out&gt;] [-O0..-O3]</code>
          <p className="text-slate-400 mt-1">Compiles the source file into a native binary (.exe on Windows, ELF on Linux, Mach-O on macOS).</p>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc run &lt;file&gt; [-- &lt;args...&gt;]</code>
          <p className="text-slate-400 mt-1">Compiles into temporary cache and immediately executes with optional program arguments.</p>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc check &lt;file&gt;</code>
          <p className="text-slate-400 mt-1">Performs Lexer, Parser, and TypeChecker passes without emitting binary files. Useful for fast CI validation.</p>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc build &lt;file&gt; --static [-o &lt;out&gt;]</code>
          <p className="text-slate-400 mt-1">Statically links runtime routines against musl libc on Linux, producing a self-contained single binary with zero external shared libraries.</p>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc run &lt;file&gt; --watch</code>
          <p className="text-slate-400 mt-1">Monitors workspace directory for source modifications and auto-restarts development servers in under 50ms.</p>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc doctor</code>
          <p className="text-slate-400 mt-1">Diagnoses the SP installation, runtime library integrity, OpenSSL linkage, and backend LLVM toolchain health.</p>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc setup</code>
          <p className="text-slate-400 mt-1">Automatically checks and provisions a portable LLVM Clang compiler and LLD linker if missing on the computer.</p>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <code className="text-cyan-400 font-bold">spc uninstall [-y]</code>
          <p className="text-slate-400 mt-1">Completely removes SP Language binaries, portable toolchains, SP_HOME, and user PATH entries with zero leftover files.</p>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Tools Overview", href: "/tools" }}
        next={{ title: "Package Manager (spm)", href: "/tools/package-manager" }}
      />
    </div>
  );
}
