"use client";

import React, { useState } from "react";
import { CODE_SNIPPETS } from "@/data/code-snippets";
import { CodeHighlighter } from "@/components/ui/CodeHighlighter";
import { Play, Sparkles, Terminal } from "lucide-react";

export const CodeShowcase: React.FC = () => {
  const [activeSnippetId, setActiveSnippetId] = useState(CODE_SNIPPETS[0].id);

  const activeSnippet =
    CODE_SNIPPETS.find((s) => s.id === activeSnippetId) || CODE_SNIPPETS[0];

  return (
    <section className="py-20 bg-slate-950/60 border-t border-slate-900">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-12">
          <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-medium mb-3">
            <Sparkles className="w-3.5 h-3.5" />
            <span>Interactive Code Tour</span>
          </div>
          <h2 className="text-3xl sm:text-4xl font-bold text-white tracking-tight">
            Expressive Syntax. Zero Overhead.
          </h2>
          <p className="mt-3 text-base text-slate-400">
            Write clear, high-level code that lowers directly to optimized native machine instructions.
          </p>
        </div>

        {/* Tab Buttons */}
        <div className="flex items-center justify-center gap-2 mb-8 flex-wrap">
          {CODE_SNIPPETS.map((snippet) => (
            <button
              key={snippet.id}
              onClick={() => setActiveSnippetId(snippet.id)}
              className={`px-4 py-2 rounded-xl text-xs sm:text-sm font-medium transition-all ${
                activeSnippetId === snippet.id
                  ? "bg-cyan-500/20 text-cyan-300 border border-cyan-500/40 shadow-[0_0_15px_rgba(6,182,212,0.15)]"
                  : "bg-slate-900/60 text-slate-400 border border-slate-800/80 hover:text-slate-200 hover:border-slate-700"
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
            <div className="mt-3 px-2 flex items-center justify-between text-xs text-slate-400">
              <span className="italic">{activeSnippet.subtitle}</span>
              <span className="font-mono text-emerald-400">Compiled with spc -O3</span>
            </div>
          </div>

          {/* Execution Output Box */}
          <div className="lg:col-span-4 flex flex-col">
            <div className="rounded-xl border border-slate-800 bg-[#090d16] p-5 shadow-2xl flex flex-col h-full">
              <div className="flex items-center justify-between pb-3 mb-3 border-b border-slate-800">
                <div className="flex items-center gap-2">
                  <Play className="w-4 h-4 text-emerald-400" />
                  <span className="text-xs font-semibold text-slate-200 uppercase tracking-wider">
                    Program Output
                  </span>
                </div>
                <span className="text-[11px] font-mono px-2 py-0.5 rounded bg-emerald-950/60 text-emerald-400 border border-emerald-800/60">
                  exit code 0
                </span>
              </div>

              <div className="flex-1 bg-black/60 rounded-lg p-4 font-mono text-xs text-slate-200 overflow-x-auto whitespace-pre border border-slate-900 min-h-[160px]">
                {activeSnippet.output}
              </div>

              <div className="mt-4 pt-3 border-t border-slate-800/60 flex items-center justify-between text-[11px] text-slate-500">
                <span className="flex items-center gap-1 font-mono">
                  <Terminal className="w-3.5 h-3.5 text-slate-400" />
                  spc run {activeSnippet.id}.sp
                </span>
                <span>Runtime: ~0.02s</span>
              </div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};
