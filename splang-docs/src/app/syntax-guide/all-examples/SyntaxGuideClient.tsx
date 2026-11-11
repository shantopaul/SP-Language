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
      <div className="border-b border-[#0A1F44] pb-8 space-y-4">
        <div className="flex flex-wrap items-center gap-2">
          <span className="inline-flex items-center gap-1 px-2.5 py-0.5 rounded-full text-xs font-semibold bg-[#0057FF]/15 text-[#00D1FF] border border-[#00D1FF]/30 shadow-[0_0_10px_rgba(0,209,255,0.15)]">
            <Sparkles className="w-3.5 h-3.5" /> Complete 100 Examples
          </span>
          <span className="inline-flex items-center gap-1 px-2.5 py-0.5 rounded-full text-xs font-semibold bg-[#0057FF]/20 text-white border border-[#0057FF]/50">
            SP Language v4.0.0 GA
          </span>
        </div>

        <h1 className="text-3xl sm:text-4xl font-black text-white tracking-tight">
          Complete Syntax Guide (1–100)
        </h1>
        <p className="text-sm sm:text-base text-slate-300 max-w-3xl leading-relaxed">
          Comprehensive, sequential reference catalog of all 100 SP language syntax
          features. From fundamental variable bindings and control flow to advanced
          zero-cost abstractions, atomics, networking, and real-world microservices.
        </p>

        {/* Quick Stats Banner */}
        <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 pt-2">
          <div className="p-3.5 rounded-xl border border-[#0A1F44] bg-[#081735]/60 shadow-md">
            <span className="text-xs text-slate-400">Total Examples</span>
            <p className="text-xl font-bold text-white">100</p>
          </div>
          <div className="p-3.5 rounded-xl border border-[#0A1F44] bg-[#081735]/60 shadow-md">
            <span className="text-xs text-slate-400">Categories</span>
            <p className="text-xl font-bold text-[#00D1FF]">10 Groups</p>
          </div>
          <div className="p-3.5 rounded-xl border border-[#0A1F44] bg-[#081735]/60 shadow-md">
            <span className="text-xs text-slate-400">Toolchain</span>
            <p className="text-xl font-bold text-[#60a5fa]">spc v4.0.0 GA</p>
          </div>
          <div className="p-3.5 rounded-xl border border-[#0A1F44] bg-[#081735]/60 shadow-md">
            <span className="text-xs text-slate-400">Compiled Outputs</span>
            <p className="text-xl font-bold text-[#00D1FF]">100% Verified</p>
          </div>
        </div>
      </div>

      {/* Interactive Controls & Filters */}
      <div className="sticky top-16 z-30 bg-white/95 dark:bg-[#020D21]/95 backdrop-blur-md border border-slate-200 dark:border-[#0A1F44] rounded-2xl p-3 sm:p-4 shadow-xl dark:shadow-2xl space-y-3 transition-colors">
        <div className="flex flex-col sm:flex-row items-stretch sm:items-center gap-2.5 sm:gap-3">
          {/* Search Input */}
          <div className="relative flex-1">
            <Search className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF] absolute left-3 top-1/2 -translate-y-1/2" />
            <input
              type="text"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              placeholder="Search by keyword, title, code, or number (e.g. 47, mutex, json)..."
              className="w-full pl-9 pr-14 py-2 sm:py-2.5 bg-slate-50 dark:bg-[#081735] border border-slate-200 dark:border-[#0A1F44] rounded-xl text-xs sm:text-sm text-slate-900 dark:text-slate-100 placeholder-slate-400 focus:outline-none focus:border-[#0057FF] dark:focus:border-[#00D1FF] transition-colors min-h-[40px]"
            />
            {searchQuery && (
              <button
                onClick={() => setSearchQuery("")}
                className="absolute right-3 top-1/2 -translate-y-1/2 text-xs text-slate-500 hover:text-slate-900 dark:text-slate-400 dark:hover:text-white px-2 py-1"
              >
                Clear
              </button>
            )}
          </div>

          {/* Quick Index Toggle Button */}
          <button
            onClick={() => setShowIndexGrid((prev) => !prev)}
            className="flex items-center justify-center gap-1.5 px-3 py-2 bg-slate-100 dark:bg-[#081735] border border-slate-200 dark:border-[#0A1F44] hover:border-[#0057FF]/40 dark:hover:border-[#00D1FF]/40 rounded-xl text-xs font-semibold text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white transition-colors shrink-0 min-h-[40px]"
          >
            <Hash className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF]" />
            <span>Jump to (1–100)</span>
            {showIndexGrid ? (
              <ChevronUp className="w-3.5 h-3.5 text-slate-400" />
            ) : (
              <ChevronDown className="w-3.5 h-3.5 text-slate-400" />
            )}
          </button>
        </div>

        {/* Collapsible Numbered Grid Jump */}
        {showIndexGrid && (
          <div className="p-3 bg-slate-50 dark:bg-[#020D21] rounded-xl border border-slate-200 dark:border-[#0A1F44] max-h-56 overflow-y-auto">
            <p className="text-[11px] font-semibold text-slate-500 dark:text-slate-400 uppercase tracking-wider mb-2">
              Select example number to jump directly:
            </p>
            <div className="flex flex-wrap gap-1.5">
              {ALL_SYNTAX_EXAMPLES.map((ex) => (
                <a
                  key={ex.id}
                  href={`#example-${ex.id}`}
                  onClick={() => setShowIndexGrid(false)}
                  title={`${ex.id}. ${ex.title}`}
                  className="w-8 h-8 rounded-lg text-xs flex items-center justify-center font-mono bg-white dark:bg-[#081735] hover:bg-[#0057FF]/15 hover:text-[#0057FF] dark:hover:bg-[#0057FF]/30 dark:hover:text-[#00D1FF] border border-slate-200 dark:border-[#0A1F44] hover:border-[#0057FF]/50 text-slate-700 dark:text-slate-300 transition-colors"
                >
                  {ex.id}
                </a>
              ))}
            </div>
          </div>
        )}

        {/* Category Pills Filter */}
        <div className="flex items-center gap-1.5 overflow-x-auto pb-1 scrollbar-thin -mx-1 px-1">
          <Filter className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF] shrink-0 ml-1" />
          {SYNTAX_CATEGORIES.map((cat) => {
            const isSelected = selectedCategory === cat;
            return (
              <button
                key={cat}
                onClick={() => setSelectedCategory(cat)}
                className={`px-3 py-1.5 rounded-full text-xs font-medium whitespace-nowrap transition-all shrink-0 min-h-[32px] flex items-center ${
                  isSelected
                    ? "bg-gradient-to-r from-[#0057FF] to-[#00D1FF] text-white font-bold shadow-[0_0_12px_rgba(0,209,255,0.3)]"
                    : "bg-slate-100 dark:bg-[#081735] text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200 hover:bg-slate-200 dark:hover:bg-[#0A1F44] border border-slate-200 dark:border-[#0A1F44]"
                }`}
              >
                {cat}
              </button>
            );
          })}
        </div>

        {/* Search Result Counter */}
        <div className="flex items-center justify-between text-xs text-slate-400 pt-1 border-t border-[#0A1F44]/60">
          <span>
            Showing <strong className="text-[#00D1FF]">{filteredExamples.length}</strong> of 100
            examples
          </span>
          {(searchQuery || selectedCategory !== "All") && (
            <button
              onClick={() => {
                setSearchQuery("");
                setSelectedCategory("All");
              }}
              className="text-xs text-[#00D1FF] hover:underline"
            >
              Reset filters
            </button>
          )}
        </div>
      </div>

      {/* Examples List */}
      <div className="space-y-12">
        {filteredExamples.length === 0 ? (
          <div className="text-center py-16 p-6 border border-[#0A1F44] rounded-2xl bg-[#081735]/40 shadow-xl">
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
              className="mt-4 px-4 py-2 bg-gradient-to-r from-[#0057FF] to-[#00D1FF] text-white rounded-xl text-xs font-semibold shadow-md"
            >
              Show all 100 examples
            </button>
          </div>
        ) : (
          filteredExamples.map((ex) => (
            <section
              key={ex.id}
              id={`example-${ex.id}`}
              className="scroll-mt-36 p-6 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:border-[#0057FF]/50 transition-all shadow-xl space-y-4"
            >
              {/* Section Header */}
              <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 border-b border-[#0A1F44] pb-3">
                <div className="space-y-1">
                  <div className="flex items-center gap-2">
                    <span className="font-mono text-xs font-extrabold px-2 py-0.5 rounded-full bg-[#0057FF]/20 text-[#00D1FF] border border-[#00D1FF]/40">
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
                  className="text-xs text-slate-500 hover:text-[#00D1FF] font-mono transition-colors self-start sm:self-center"
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
                        <h4 className="text-xs font-bold uppercase tracking-wider text-[#00D1FF] pt-1">
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
                        <div className="rounded-xl border border-[#0A1F44] bg-[#020D21] overflow-hidden mt-3 shadow-inner">
                          <div className="flex items-center justify-between px-3.5 py-1.5 bg-[#081735]/80 border-b border-[#0A1F44]">
                            <span className="text-[11px] font-mono text-[#00D1FF] font-semibold flex items-center gap-1.5">
                              <Terminal className="w-3.5 h-3.5 text-[#00D1FF]" />
                              Expected Terminal Output
                            </span>
                            <span className="text-[10px] text-slate-400 font-mono">
                              spc run {filename}
                            </span>
                          </div>
                          <pre className="p-3.5 text-xs font-mono text-cyan-300/95 overflow-x-auto whitespace-pre leading-relaxed">
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
          className="p-3 rounded-full bg-gradient-to-r from-[#0057FF] to-[#00D1FF] text-white shadow-xl shadow-[#0057FF]/30 transition-all hover:scale-105"
          title="Scroll to top"
        >
          <ArrowUp className="w-5 h-5" />
        </button>
      </div>
    </div>
  );
}
