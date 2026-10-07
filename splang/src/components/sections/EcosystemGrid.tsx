import React from "react";
import { BookOpen, Boxes, Code, ExternalLink, ArrowRight, Layers } from "lucide-react";

export const EcosystemGrid: React.FC = () => {
  const domains = [
    {
      title: "Official Documentation",
      url: "https://docs.splang.shantopaul.com",
      description:
        "Comprehensive language specification, getting-started guides, 46-feature syntax tour, and standard library API references.",
      icon: BookOpen,
      badge: "docs.splang.shantopaul.com",
      accent: "from-emerald-500/20 to-teal-500/5",
      border: "hover:border-emerald-500/50",
    },
    {
      title: "SP Package Registry",
      url: "https://registry.splang.shantopaul.com",
      description:
        "Discover, publish, and inspect packages with verified SHA-256 checksums and real-time integration with the spm CLI.",
      icon: Boxes,
      badge: "registry.splang.shantopaul.com",
      accent: "from-cyan-500/20 to-blue-500/5",
      border: "hover:border-cyan-500/50",
    },
    {
      title: "VS Code Extension",
      url: "https://dl.splang.shantopaul.com/sp-vscode.vsix",
      description:
        "Official editor support featuring TextMate syntax highlighting, code snippets, LSP autocompletion, and DAP stepping debugger.",
      icon: Code,
      badge: "sp-vscode.vsix",
      accent: "from-blue-500/20 to-indigo-500/5",
      border: "hover:border-blue-500/50",
    },
    {
      title: "Standard Library (8 Modules)",
      url: "https://docs.splang.shantopaul.com/stdlib",
      description:
        "Rich native modules for I/O, strings, mathematics, collections (List, Map, Set), filesystem, clocks, and child processes.",
      icon: Layers,
      badge: "share/sp/stdlib",
      accent: "from-purple-500/20 to-pink-500/5",
      border: "hover:border-purple-500/50",
    },
  ];

  return (
    <section className="py-24 relative border-t border-slate-900">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-16">
          <h2 className="text-3xl sm:text-4xl font-bold text-white tracking-tight">
            The Complete SP Ecosystem
          </h2>
          <p className="mt-3 text-base text-slate-400">
            A cohesive suite of portals and tools built to empower developer productivity.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-6">
          {domains.map((item, idx) => {
            const Icon = item.icon;
            return (
              <a
                key={idx}
                href={item.url}
                target={item.url.startsWith("http") ? "_blank" : undefined}
                rel={item.url.startsWith("http") ? "noopener noreferrer" : undefined}
                className={`group rounded-2xl border border-slate-800 bg-gradient-to-br ${item.accent} p-8 flex flex-col justify-between transition-all duration-300 hover:-translate-y-1 ${item.border} shadow-xl`}
              >
                <div>
                  <div className="flex items-center justify-between mb-4">
                    <div className="w-12 h-12 rounded-xl bg-slate-900 border border-slate-700/80 flex items-center justify-center">
                      <Icon className="w-6 h-6 text-cyan-400" />
                    </div>
                    <span className="font-mono text-xs px-2.5 py-1 rounded-full bg-slate-900/90 border border-slate-700 text-slate-300 flex items-center gap-1">
                      {item.badge}
                      <ExternalLink className="w-3 h-3 text-slate-500" />
                    </span>
                  </div>
                  <h3 className="text-xl font-bold text-white group-hover:text-cyan-400 transition-colors mb-2">
                    {item.title}
                  </h3>
                  <p className="text-sm text-slate-400 leading-relaxed">
                    {item.description}
                  </p>
                </div>

                <div className="mt-6 pt-4 border-t border-slate-800/80 flex items-center justify-between text-xs text-cyan-400 font-medium">
                  <span>Visit Resource</span>
                  <ArrowRight className="w-4 h-4 group-hover:translate-x-1 transition-transform" />
                </div>
              </a>
            );
          })}
        </div>
      </div>
    </section>
  );
};
