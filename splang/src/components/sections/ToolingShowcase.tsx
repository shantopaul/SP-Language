import React from "react";
import { Terminal, CheckSquare, FileText, Monitor, Check, ArrowRight } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const ToolingShowcase: React.FC = () => {
  const tools = [
    {
      title: "spc fmt",
      badge: "Formatter",
      description:
        "Standardized, deterministic code formatting with strict idempotency (fmt(fmt(x)) == fmt(x)). Eliminates bike-shedding on team codebases.",
      command: "spc fmt src/ --write",
      features: ["Custom indent width", "Preserves docstrings", "Blazing fast AST formatter"],
    },
    {
      title: "spc test",
      badge: "Test Runner",
      description:
        "First-class test blocks embedded right in your source files or tests/ directory. Zero boilerplate setup, test filtering, and colored reporting.",
      command: "spc test tests/ --filter math",
      features: ["test \"name\" { ... } blocks", "Assert & panic interception", "Detailed failure diffs"],
    },
    {
      title: "spc doc",
      badge: "Documentation",
      description:
        "Generates clean Markdown and modern responsive HTML documentation directly from your /// doc comments and function signatures.",
      command: "spc doc src/ --out docs/",
      features: ["Live search index", "Dark mode web UI", "Symbol reference hierarchy"],
    },
    {
      title: "spls & spd",
      badge: "LSP & DAP",
      description:
        "Official JSON-RPC 2.0 Language Server and Debug Adapter. Autocomplete, hover tooltips, go-to-definition, and breakpoints in VS Code.",
      command: "irm https://dl.splang.shantopaul.com/sp-vscode.vsix -OutFile $env:TEMP/sp.vsix; code --install-extension $env:TEMP/sp.vsix",
      features: ["Real-time diagnostics", "Variable inspection", "Step over/into/out debugger"],
    },
  ];

  return (
    <section className="py-24 relative bg-[#020D21] border-t border-[#0A1F44]">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-16">
          <div className="inline-flex items-center gap-1.5 px-3.5 py-1 rounded-full bg-[#081735] border border-blue-500/40 text-blue-300 text-xs font-semibold mb-3 shadow-glow-sm">
            <Monitor className="w-3.5 h-3.5 text-cyan-400" />
            <span>Developer Experience</span>
          </div>
          <h2 className="text-3xl sm:text-5xl font-extrabold text-white tracking-tight">
            Complete Tooling Out-of-the-Box
          </h2>
          <p className="mt-4 text-base text-slate-300 leading-relaxed">
            No need to configure disparate third-party tools. Everything you need to format, test, document, and debug SP code comes pre-bundled in the binary distribution.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
          {tools.map((tool, idx) => (
            <div
              key={idx}
              className="rounded-2xl border border-[#0A1F44] bg-[#081735]/70 backdrop-blur-xl p-6 sm:p-8 flex flex-col justify-between hover:border-cyan-400/40 hover:shadow-[0_12px_35px_rgba(0,87,255,0.12)] transition-all"
            >
              <div>
                <div className="flex items-center justify-between mb-4">
                  <h3 className="text-xl font-bold text-white font-mono">{tool.title}</h3>
                  <span className="px-3 py-0.5 rounded-full text-xs font-semibold bg-cyan-950/80 text-cyan-300 border border-cyan-700/60 shadow-glow-sm">
                    {tool.badge}
                  </span>
                </div>
                <p className="text-sm text-slate-300 leading-relaxed mb-6">
                  {tool.description}
                </p>

                {/* Command Bar */}
                <div className="bg-[#020D21] rounded-xl px-4 py-3 border border-[#0A1F44] font-mono text-xs text-cyan-300 mb-6 flex items-center gap-2 overflow-x-auto">
                  <span className="text-slate-600 select-none">$</span>
                  <span className="whitespace-nowrap">{tool.command}</span>
                </div>

                {/* Bullet Points */}
                <ul className="space-y-2.5 mb-6">
                  {tool.features.map((feat, fIdx) => (
                    <li key={fIdx} className="flex items-center gap-2.5 text-xs text-slate-300">
                      <Check className="w-4 h-4 text-cyan-400 shrink-0" />
                      <span>{feat}</span>
                    </li>
                  ))}
                </ul>
              </div>

              <div className="pt-4 border-t border-[#0A1F44] flex items-center justify-between text-xs text-slate-400">
                <span className="font-mono text-cyan-400">Included in SP v3.1.0 GA</span>
                <a
                  href="https://docs.splang.shantopaul.com/tools"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-cyan-300 hover:text-white font-medium flex items-center gap-1 transition-colors"
                >
                  <span>Read Docs</span>
                  <ArrowRight className="w-3.5 h-3.5" />
                </a>
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
};
