import React from "react";
import { Terminal, CheckSquare, FileText, Monitor, Check } from "lucide-react";
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
      command: "code --install-extension sp-lang",
      features: ["Real-time diagnostics", "Variable inspection", "Step over/into/out debugger"],
    },
  ];

  return (
    <section className="py-24 relative bg-slate-950/40 border-t border-slate-900">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-16">
          <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-blue-950/60 border border-blue-800/60 text-blue-400 text-xs font-medium mb-3">
            <Monitor className="w-3.5 h-3.5" />
            <span>Developer Experience</span>
          </div>
          <h2 className="text-3xl sm:text-4xl font-bold text-white tracking-tight">
            Complete Tooling Out-of-the-Box
          </h2>
          <p className="mt-3 text-base text-slate-400">
            No need to configure 10 disparate third-party tools. Everything you need to format, test, document, and debug SP code comes installed in the standard binary distribution.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
          {tools.map((tool, idx) => (
            <div
              key={idx}
              className="rounded-2xl border border-slate-800 bg-slate-900/60 backdrop-blur-xl p-6 sm:p-8 flex flex-col justify-between hover:border-slate-700 transition-all shadow-xl"
            >
              <div>
                <div className="flex items-center justify-between mb-4">
                  <h3 className="text-xl font-bold text-white font-mono">{tool.title}</h3>
                  <span className="px-2.5 py-0.5 rounded-full text-xs font-semibold bg-cyan-950 text-cyan-400 border border-cyan-800/80">
                    {tool.badge}
                  </span>
                </div>
                <p className="text-sm text-slate-400 leading-relaxed mb-6">
                  {tool.description}
                </p>

                {/* Command Bar */}
                <div className="bg-black/60 rounded-xl px-4 py-2.5 border border-slate-800 font-mono text-xs text-emerald-400 mb-6 flex items-center gap-2">
                  <span className="text-slate-600 select-none">$</span>
                  <span>{tool.command}</span>
                </div>

                {/* Bullet Points */}
                <ul className="space-y-2 mb-6">
                  {tool.features.map((feat, fIdx) => (
                    <li key={fIdx} className="flex items-center gap-2 text-xs text-slate-300">
                      <Check className="w-4 h-4 text-cyan-400 shrink-0" />
                      <span>{feat}</span>
                    </li>
                  ))}
                </ul>
              </div>

              <div className="pt-4 border-t border-slate-800/80 flex items-center justify-between text-xs text-slate-500">
                <span>Included in SP v1.0.0</span>
                <a
                  href="https://docs.splang.shantopaul.com/tools"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="text-cyan-400 hover:text-cyan-300 font-medium"
                >
                  Read Docs &rarr;
                </a>
              </div>
            </div>
          ))}
        </div>
      </div>
    </section>
  );
};
