import React from "react";
import Link from "next/link";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ArrowRight } from "lucide-react";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Developer Tooling Suite",
  description: "Complete developer toolchain: compiler, package manager, formatter, test runner, LSP, and DAP.",
};

const TOOLS = [
  { name: "Compiler CLI (spc)", href: "/tools/compiler-cli", desc: "Build, run, syntax check, cross-compile, auto-upgrade, and doctor diagnosis." },
  { name: "Package Manager (spm)", href: "/tools/package-manager", desc: "Initialize projects, resolve dependencies, and publish to the registry." },
  { name: "Code Formatter (spc fmt)", href: "/tools/formatter", desc: "Format source trees with deterministic idempotency." },
  { name: "Native Test Runner (spc test)", href: "/tools/test-runner", desc: "Execute embedded test blocks with assertion assertions and filters." },
  { name: "Doc Generator (spc doc)", href: "/tools/doc-generator", desc: "Generate Markdown and modern HTML documentation from /// comments." },
  { name: "Language Server (spls)", href: "/tools/lsp-server", desc: "JSON-RPC 2.0 language server providing autocomplete, diagnostics, and hover." },
  { name: "DAP Debugger (spd)", href: "/tools/debugger", desc: "Step through execution, set breakpoints, and inspect local variables." },
  { name: "VS Code Extension", href: "/tools/vscode-extension", desc: "Official editor integration with TextMate syntax highlighting and LSP." },
];

export default function ToolsOverviewPage() {
  return (
    <div className="space-y-6">
      <Breadcrumbs items={[{ title: "Developer Tools", href: "/tools" }]} />

      <h1 className="text-3xl font-extrabold text-white">SP Developer Toolchain</h1>
      <p className="text-sm text-slate-300 leading-relaxed">
        SP includes a first-class, fully native developer tools suite installed alongside the compiler, offering zero-config diagnostics, formatting, and package resolution.
      </p>

      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 pt-4">
        {TOOLS.map((tool) => (
          <Link
            key={tool.name}
            href={tool.href}
            className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
          >
            <div className="flex items-center justify-between mb-2">
              <h3 className="font-mono text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors">
                {tool.name}
              </h3>
              <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
            </div>
            <p className="text-xs text-slate-400 leading-relaxed">{tool.desc}</p>
          </Link>
        ))}
      </div>

      <PrevNextNav
        prev={{ title: "std.process Reference", href: "/stdlib/process" }}
        next={{ title: "Compiler CLI (spc)", href: "/tools/compiler-cli" }}
      />
    </div>
  );
}
