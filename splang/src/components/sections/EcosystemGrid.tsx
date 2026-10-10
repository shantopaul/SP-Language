import React from "react";
import { BookOpen, Boxes, Code, ExternalLink, ArrowRight, Layers, Sparkles } from "lucide-react";

export const EcosystemGrid: React.FC = () => {
  const domains = [
    {
      title: "Official Documentation",
      url: "https://docs.splang.shantopaul.com",
      description:
        "Comprehensive language specification, enterprise backend guides, 46-feature syntax tour, and standard library API references.",
      icon: BookOpen,
      badge: "docs.splang.shantopaul.com",
      accent: "from-blue-600/15 via-[#081735] to-[#020D21]",
      border: "hover:border-cyan-400/50",
    },
    {
      title: "SP Package Registry",
      url: "https://registry.splang.shantopaul.com",
      description:
        "Discover, publish, and inspect packages with verified SHA-256 checksums and real-time integration with the spm CLI.",
      icon: Boxes,
      badge: "registry.splang.shantopaul.com",
      accent: "from-cyan-600/15 via-[#081735] to-[#020D21]",
      border: "hover:border-cyan-400/50",
    },
    {
      title: "VS Code Extension",
      url: "https://dl.splang.shantopaul.com/sp-vscode.vsix",
      description:
        "Official editor support featuring TextMate syntax highlighting, code snippets, LSP autocompletion, and DAP stepping debugger.",
      icon: Code,
      badge: "sp-vscode.vsix",
      accent: "from-indigo-600/15 via-[#081735] to-[#020D21]",
      border: "hover:border-cyan-400/50",
    },
    {
      title: "Standard Library (23 Modules)",
      url: "https://docs.splang.shantopaul.com/stdlib",
      description:
        "Production modules for web, net, db pooling, redis, crypto, metrics, json, queue, health, fs, io, math, time, sync, test, and more.",
      icon: Layers,
      badge: "23 stdlib modules",
      accent: "from-blue-500/15 via-[#081735] to-[#020D21]",
      border: "hover:border-cyan-400/50",
    },
  ];

  return (
    <section className="py-24 relative bg-slate-50 dark:bg-[#020D21] border-t border-slate-200 dark:border-[#0A1F44] transition-colors">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-16">
          <div className="inline-flex items-center gap-1.5 px-3.5 py-1 rounded-full bg-blue-50 dark:bg-[#081735] border border-blue-200 dark:border-cyan-500/40 text-[#0057FF] dark:text-cyan-300 text-xs font-semibold mb-3 shadow-sm dark:shadow-glow-sm">
            <Sparkles className="w-3.5 h-3.5 text-[#0057FF] dark:text-cyan-400" />
            <span>Developer Ecosystem</span>
          </div>
          <h2 className="text-3xl sm:text-5xl font-extrabold text-slate-900 dark:text-white tracking-tight">
            The Complete SP Ecosystem
          </h2>
          <p className="mt-4 text-base text-slate-600 dark:text-slate-300 leading-relaxed">
            A cohesive suite of portals, tools, and libraries engineered to maximize developer velocity and reliability.
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
                className={`group rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-gradient-to-br dark:${item.accent} p-8 flex flex-col justify-between transition-all duration-300 hover:-translate-y-1 hover:border-blue-300 dark:${item.border} shadow-sm hover:shadow-xl dark:shadow-xl dark:hover:shadow-[0_12px_35px_rgba(0,87,255,0.15)]`}
              >
                <div>
                  <div className="flex items-center justify-between mb-5">
                    <div className="w-12 h-12 rounded-xl bg-blue-50 dark:bg-[#020D21] border border-blue-200 dark:border-[#0A1F44] flex items-center justify-center shadow-sm">
                      <Icon className="w-6 h-6 text-[#0057FF] dark:text-cyan-400" />
                    </div>
                    <span className="font-mono text-xs px-3 py-1 rounded-full bg-slate-100 dark:bg-[#020D21]/90 border border-slate-200 dark:border-[#0A1F44] text-slate-700 dark:text-slate-300 flex items-center gap-1.5 shadow-sm">
                      {item.badge}
                      <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
                    </span>
                  </div>
                  <h3 className="text-xl font-bold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-cyan-300 transition-colors mb-2.5">
                    {item.title}
                  </h3>
                  <p className="text-sm text-slate-600 dark:text-slate-300/90 leading-relaxed">
                    {item.description}
                  </p>
                </div>

                <div className="mt-8 pt-4 border-t border-slate-100 dark:border-[#0A1F44] flex items-center justify-between text-xs text-[#0057FF] dark:text-cyan-400 font-semibold">
                  <span>Explore Resource</span>
                  <ArrowRight className="w-4 h-4 group-hover:translate-x-1.5 transition-transform" />
                </div>
              </a>
            );
          })}
        </div>
      </div>
    </section>
  );
};
