"use client";

import React, { useState, useMemo } from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import {
  ALL_SYNTAX_EXAMPLES,
  SYNTAX_CATEGORIES,
} from "@/data/syntaxExamples";
import {
  Search,
  Terminal,
  Hash,
  BookOpen,
  Filter,
  ArrowUp,
  Sparkles,
  ChevronDown,
  ChevronUp,
} from "lucide-react";

export function SyntaxGuideClient() {
  const [searchQuery, setSearchQuery] = useState("");
  const [selectedCategory, setSelectedCategory] = useState<string>("All");
  const [showIndexGrid, setShowIndexGrid] = useState(false);

  // Filtered examples
  const filteredExamples = useMemo(() => {
    return ALL_SYNTAX_EXAMPLES.filter((ex) => {
      // Category filter
      if (selectedCategory !== "All" && ex.category !== selectedCategory) {
        return false;
      }

      // Search filter
      if (!searchQuery.trim()) return true;

      const q = searchQuery.toLowerCase().trim();
      const idMatch = ex.id.toString() === q || `#${ex.id}` === q;
      const titleMatch = ex.title.toLowerCase().includes(q);
      const descMatch = ex.description.toLowerCase().includes(q);
      const codeMatch = ex.snippets.some(
        (s) =>
          s.code.toLowerCase().includes(q) ||
          (s.subtitle && s.subtitle.toLowerCase().includes(q))
      );

      return idMatch || titleMatch || descMatch || codeMatch;
    });
  }, [searchQuery, selectedCategory]);

  const scrollToTop = () => {
    window.scrollTo({ top: 0, behavior: "smooth" });
  };

  return (
    <div className="space-y-8 pb-16">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Complete Syntax Guide (1–100)" },
        ]}
      />

      {/* Hero Header */}
      <div className="border-b border-slate-800 pb-8 space-y-4">
        <div className="flex flex-wrap items-center gap-2">
          <span className="inline-flex items-center gap-1 px-2.5 py-0.5 rounded-full text-xs font-semibold bg-cyan-500/10 text-cyan-400 border border-cyan-500/30">
            <Sparkles className="w-3.5 h-3.5" /> Complete 100 Examples
          </span>
          <span className="inline-flex items-center gap-1 px-2.5 py-0.5 rounded-full text-xs font-semibold bg-emerald-500/10 text-emerald-400 border border-emerald-500/30">
            SP Language v1.0
          </span>
        </div>

        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Complete Syntax Guide (1–100)
        </h1>
        <p className="text-sm sm:text-base text-slate-300 max-w-3xl leading-relaxed">
          Comprehensive, sequential reference catalog of all 100 SP language syntax
          features. From fundamental variable bindings and control flow to advanced
          zero-cost abstractions, atomics, networking, and real-world microservices.
        </p>

        {/* Quick Stats Banner */}
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 pt-2">
          <div className="p-3 rounded-lg border border-slate-800 bg-slate-900/40">
            <span className="text-xs text-slate-400">Total Examples</span>
            <p className="text-xl font-bold text-white">100</p>
          </div>
          <div className="p-3 rounded-lg border border-slate-800 bg-slate-900/40">
            <span className="text-xs text-slate-400">Categories</span>
            <p className="text-xl font-bold text-cyan-400">10 Groups</p>
          </div>
          <div className="p-3 rounded-lg border border-slate-800 bg-slate-900/40">
            <span className="text-xs text-slate-400">Toolchain</span>
            <p className="text-xl font-bold text-purple-400">spc v1.0.0</p>
          </div>
          <div className="p-3 rounded-lg border border-slate-800 bg-slate-900/40">
            <span className="text-xs text-slate-400">Compiled Outputs</span>
            <p className="text-xl font-bold text-emerald-400">Verified</p>
          </div>
        </div>
      </div>

      {/* Interactive Controls & Filters */}
      <div className="sticky top-16 z-30 bg-[#020617]/95 backdrop-blur-md border border-slate-800 rounded-xl p-4 shadow-xl space-y-3">
        <div className="flex flex-col sm:flex-row items-stretch sm:items-center gap-3">
          {/* Search Input */}
          <div className="relative flex-1">
            <Search className="w-4 h-4 text-slate-400 absolute left-3 top-1/2 -translate-y-1/2" />
            <input
              type="text"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder="Search by keyword, title, code, or number (e.g. 47, mutex, json, defer)..."
              className="w-full pl-9 pr-4 py-2 bg-slate-900/90 border border-slate-700/80 rounded-lg text-xs sm:text-sm text-slate-100 placeholder-slate-500 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-colors"
            />
            {searchQuery && (
              <button
                onClick={() => setSearchQuery("")}
                className="absolute right-3 top-1/2 -translate-y-1/2 text-xs text-slate-400 hover:text-white"
              >
                Clear
              </button>
            )}
          </div>

          {/* Quick Index Toggle Button */}
          <button
            onClick={() => setShowIndexGrid((prev) => !prev)}
            className="flex items-center justify-center gap-1.5 px-3 py-2 bg-slate-900 border border-slate-800 hover:border-slate-700 rounded-lg text-xs font-semibold text-slate-300 hover:text-white transition-colors shrink-0"
          >
            <Hash className="w-3.5 h-3.5 text-cyan-400" />
            <span>Jump to Number (1–100)</span>
            {showIndexGrid ? (
              <ChevronUp className="w-3.5 h-3.5 text-slate-400" />
            ) : (
              <ChevronDown className="w-3.5 h-3.5 text-slate-400" />
            )}
          </button>
        </div>

        {/* Collapsible Numbered Grid Jump */}
        {showIndexGrid && (
          <div className="p-3 bg-slate-950/80 rounded-lg border border-slate-800 max-h-56 overflow-y-auto">
            <p className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider mb-2">
              Select example number to jump directly:
            </p>
            <div className="flex flex-wrap gap-1.5">
              {ALL_SYNTAX_EXAMPLES.map((ex) => (
                <a
                  key={ex.id}
                  href={`#example-${ex.id}`}
                  onClick={() => setShowIndexGrid(false)}
                  title={`${ex.id}. ${ex.title}`}
                  className="w-8 h-8 rounded text-xs flex items-center justify-center font-mono bg-slate-900 hover:bg-cyan-950 hover:text-cyan-300 border border-slate-800 hover:border-cyan-700 text-slate-300 transition-colors"
                >
                  {ex.id}
                </a>
              ))}
            </div>
          </div>
        )}

        {/* Category Pills Filter */}
        <div className="flex items-center gap-1.5 overflow-x-auto pb-1 scrollbar-thin">
          <Filter className="w-3.5 h-3.5 text-slate-500 shrink-0 ml-1" />
          {SYNTAX_CATEGORIES.map((cat) => {
            const isSelected = selectedCategory === cat;
            return (
              <button
                key={cat}
                onClick={() => setSelectedCategory(cat)}
                className={`px-2.5 py-1 rounded-full text-xs font-medium whitespace-nowrap transition-colors shrink-0 ${
                  isSelected
                    ? "bg-cyan-500 text-slate-950 font-bold shadow-sm"
                    : "bg-slate-900/60 text-slate-400 hover:text-slate-200 hover:bg-slate-800 border border-slate-800/80"
                }`}
              >
                {cat}
              </button>
            );
          })}
        </div>

        {/* Search Result Counter */}
        <div className="flex items-center justify-between text-xs text-slate-400 pt-1 border-t border-slate-800/60">
          <span>
            Showing <strong className="text-cyan-400">{filteredExamples.length}</strong> of 100
            examples
          </span>
          {(searchQuery || selectedCategory !== "All") && (
            <button
              onClick={() => {
                setSearchQuery("");
                setSelectedCategory("All");
              }}
              className="text-xs text-cyan-400 hover:underline"
            >
              Reset filters
            </button>
          )}
        </div>
      </div>

      {/* Examples List */}
      <div className="space-y-12">
        {filteredExamples.length === 0 ? (
          <div className="text-center py-16 p-6 border border-slate-800 rounded-xl bg-slate-900/20">
            <BookOpen className="w-10 h-10 text-slate-600 mx-auto mb-3" />
            <h3 className="text-base font-semibold text-white">No matching syntax examples</h3>
            <p className="text-xs text-slate-400 mt-1 max-w-md mx-auto">
              No results found for &ldquo;{searchQuery}&rdquo;. Try another keyword, category,
              or section number.
            </p>
            <button
              onClick={() => {
                setSearchQuery("");
                setSelectedCategory("All");
              }}
              className="mt-4 px-4 py-1.5 bg-cyan-600 hover:bg-cyan-500 text-white rounded-lg text-xs font-semibold"
            >
              Show all 100 examples
            </button>
          </div>
        ) : (
          filteredExamples.map((ex) => (
            <section
              key={ex.id}
              id={`example-${ex.id}`}
              className="scroll-mt-36 p-6 rounded-2xl border border-slate-800/90 bg-[#0a0f1d]/70 hover:border-slate-700 transition-colors shadow-lg space-y-4"
            >
              {/* Section Header */}
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-slate-800/80 pb-3">
                <div className="space-y-1">
                  <div className="flex items-center gap-2">
                    <span className="font-mono text-xs font-extrabold px-2 py-0.5 rounded bg-cyan-950/80 text-cyan-300 border border-cyan-800/60">
                      #{ex.id}
                    </span>
                    <span className="text-[11px] font-medium text-slate-400">
                      {ex.category}
                    </span>
                  </div>
                  <h2 className="text-lg sm:text-xl font-bold text-white tracking-tight">
                    {ex.id}. {ex.title}
                  </h2>
                </div>
                <a
                  href={`#example-${ex.id}`}
                  className="text-xs text-slate-500 hover:text-cyan-400 font-mono transition-colors self-start sm:self-center"
                  title="Permanent anchor link"
                >
                  #example-{ex.id}
                </a>
              </div>

              {/* Description */}
              {ex.description && (
                <p className="text-xs sm:text-sm text-slate-300 leading-relaxed">
                  {ex.description}
                </p>
              )}

              {/* Snippets */}
              <div className="space-y-5">
                {ex.snippets.map((snippet, sIdx) => {
                  const filename = `ex_${ex.id.toString().padStart(2, "0")}${
                    ex.snippets.length > 1 ? `_${sIdx + 1}` : ""
                  }.sp`;

                  return (
                    <div key={sIdx} className="space-y-2">
                      {snippet.subtitle && (
                        <h4 className="text-xs font-bold uppercase tracking-wider text-cyan-400/90 pt-1">
                          {snippet.subtitle}
                        </h4>
                      )}

                      {snippet.description && (
                        <p className="text-xs text-slate-400 leading-relaxed">
                          {snippet.description}
                        </p>
                      )}

                      {/* Code Block with line numbers & copy button */}
                      <CodeBlock
                        code={snippet.code}
                        filename={filename}
                        language="sp"
                      />

                      {/* Terminal Expected Output */}
                      {snippet.output && (
                        <div className="rounded-xl border border-slate-800 bg-[#060a14] overflow-hidden mt-3 shadow-inner">
                          <div className="flex items-center justify-between px-3.5 py-1.5 bg-slate-900/70 border-b border-slate-800/80">
                            <span className="text-[11px] font-mono text-emerald-400 font-semibold flex items-center gap-1.5">
                              <Terminal className="w-3.5 h-3.5 text-emerald-400" />
                              Expected Terminal Output
                            </span>
                            <span className="text-[10px] text-slate-500 font-mono">
                              spc run {filename}
                            </span>
                          </div>
                          <pre className="p-3.5 text-xs font-mono text-emerald-300/95 overflow-x-auto whitespace-pre leading-relaxed">
                            {snippet.output}
                          </pre>
                        </div>
                      )}
                    </div>
                  );
                })}
              </div>
            </section>
          ))
        )}
      </div>

      {/* Floating Scroll To Top */}
      <div className="fixed bottom-6 right-6 z-40">
        <button
          onClick={scrollToTop}
          className="p-3 rounded-full bg-cyan-600 hover:bg-cyan-500 text-white shadow-xl shadow-cyan-900/40 transition-all hover:scale-105"
          title="Scroll to top"
        >
          <ArrowUp className="w-5 h-5" />
        </button>
      </div>
    </div>
  );
}
