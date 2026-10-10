"use client";

import React, { useState } from "react";
import { CODE_SNIPPETS } from "@/data/code-snippets";
import { CodeHighlighter } from "@/components/ui/CodeHighlighter";
import { Play, Sparkles, Terminal, CheckCircle2 } from "lucide-react";

export const CodeShowcase: React.FC = () => {
  const [activeSnippetId, setActiveSnippetId] = useState(CODE_SNIPPETS[0].id);

  const activeSnippet =
    CODE_SNIPPETS.find((s) => s.id === activeSnippetId) || CODE_SNIPPETS[0];

  return (
    <section className="py-24 bg-white dark:bg-[#020D21]/90 border-t border-slate-200 dark:border-[#0A1F44] relative transition-colors">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-12">
          <div className="inline-flex items-center gap-1.5 px-3.5 py-1 rounded-full bg-blue-50 dark:bg-[#081735] border border-blue-200 dark:border-cyan-500/40 text-[#0057FF] dark:text-cyan-300 text-xs font-semibold mb-3 shadow-sm dark:shadow-glow-sm">
            <Sparkles className="w-3.5 h-3.5 text-[#0057FF] dark:text-cyan-400" />
            <span>Interactive Code Tour</span>
          </div>
          <h2 className="text-3xl sm:text-5xl font-extrabold text-slate-900 dark:text-white tracking-tight">
            Expressive Syntax. Zero Overhead.
          </h2>
          <p className="mt-4 text-base text-slate-600 dark:text-slate-300 max-w-2xl mx-auto leading-relaxed">
            Write clean, high-level code with ergonomic type inference that lowers directly to optimized native LLVM machine instructions.
          </p>
        </div>

        {/* Tab Buttons */}
        <div className="flex items-center justify-center gap-2 mb-8 flex-wrap">
          {CODE_SNIPPETS.map((snippet) => (
            <button
              key={snippet.id}
              onClick={() => setActiveSnippetId(snippet.id)}
              className={`px-4 py-2.5 rounded-xl text-xs sm:text-sm font-medium transition-all ${
                activeSnippetId === snippet.id
                  ? "bg-blue-50 dark:bg-[#081735] text-[#0057FF] dark:text-cyan-300 border border-blue-300 dark:border-cyan-400/60 shadow-sm font-semibold"
                  : "bg-white dark:bg-[#081735]/40 text-slate-600 dark:text-slate-400 border border-slate-200 dark:border-[#0A1F44] hover:text-slate-900 dark:hover:text-slate-200 hover:border-slate-300 dark:hover:border-slate-700 hover:bg-slate-50 dark:hover:bg-[#081735]"
              }`}
            >
              {snippet.title}
            </button>
          ))}
        </div>

        {/* Code + Output Viewer Grid */}
        <div className="grid grid-cols-1 lg:grid-cols-12 gap-6 items-start">
          {/* Main Code Editor */}
          <div className="lg:col-span-8 flex flex-col">
            <CodeHighlighter
              code={activeSnippet.code}
              filename={`${activeSnippet.id}.sp`}
              showLineNumbers={true}
            />
            <div className="mt-3 px-2 flex items-center justify-between text-xs text-slate-500 dark:text-slate-400">
              <span className="italic">{activeSnippet.subtitle}</span>
              <span className="font-mono text-[#0057FF] dark:text-cyan-400 font-semibold flex items-center gap-1">
                <CheckCircle2 className="w-3.5 h-3.5 text-[#0057FF] dark:text-cyan-400" />
                Compiled with spc -O3
              </span>
            </div>
          </div>

          {/* Execution Output Box */}
          <div className="lg:col-span-4 flex flex-col">
            <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/80 p-5 sm:p-6 shadow-sm dark:shadow-2xl flex flex-col h-full backdrop-blur-xl">
              <div className="flex items-center justify-between pb-3.5 mb-3.5 border-b border-slate-200 dark:border-[#0A1F44]">
                <div className="flex items-center gap-2">
                  <Play className="w-4 h-4 text-[#0057FF] dark:text-cyan-400" />
                  <span className="text-xs font-bold text-slate-900 dark:text-white uppercase tracking-wider font-mono">
                    Program Output
                  </span>
                </div>
                <span className="text-[11px] font-mono px-2.5 py-0.5 rounded-full bg-emerald-50 dark:bg-cyan-950/80 text-emerald-700 dark:text-cyan-300 border border-emerald-200 dark:border-cyan-700/60 font-semibold">
                  exit code 0
                </span>
              </div>

              <div className="flex-1 bg-[#0B1528] rounded-xl p-4 font-mono text-xs text-slate-200 overflow-x-auto whitespace-pre border border-slate-800 dark:border-[#0A1F44] min-h-[180px] leading-relaxed">
                {activeSnippet.output}
              </div>

              <div className="mt-4 pt-3.5 border-t border-slate-200 dark:border-[#0A1F44] flex items-center justify-between text-[11px] text-slate-500 dark:text-slate-400">
                <span className="flex items-center gap-1.5 font-mono">
                  <Terminal className="w-3.5 h-3.5 text-[#0057FF] dark:text-cyan-400" />
                  spc run {activeSnippet.id}.sp
                </span>
                <span className="font-mono text-[#0057FF] dark:text-cyan-300">Runtime: ~0.002s</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};
