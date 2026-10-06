"use client";

import React, { useState, useEffect, useRef } from "react";
import { useRouter } from "next/navigation";
import { Search, X, ArrowRight, CornerDownLeft, BookOpen } from "lucide-react";
import { searchDocs } from "@/lib/search";
import { SearchItem } from "@/data/search-items";

interface SearchModalProps {
  isOpen: boolean;
  onClose: () => void;
}

export const SearchModal: React.FC<SearchModalProps> = ({ isOpen, onClose }) => {
  const [query, setQuery] = useState("");
  const [results, setResults] = useState<SearchItem[]>([]);
  const [selectedIndex, setSelectedIndex] = useState(0);
  const router = useRouter();
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (isOpen) {
      setTimeout(() => inputRef.current?.focus(), 50);
    } else {
      setQuery("");
      setResults([]);
      setSelectedIndex(0);
    }
  }, [isOpen]);

  useEffect(() => {
    const res = searchDocs(query);
    setResults(res);
    setSelectedIndex(0);
  }, [query]);

  // Handle keyboard navigation
  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === "ArrowDown") {
      e.preventDefault();
      setSelectedIndex((prev) => (prev + 1 < results.length ? prev + 1 : 0));
    } else if (e.key === "ArrowUp") {
      e.preventDefault();
      setSelectedIndex((prev) => (prev > 0 ? prev - 1 : results.length - 1));
    } else if (e.key === "Enter") {
      e.preventDefault();
      if (results[selectedIndex]) {
        router.push(results[selectedIndex].href);
        onClose();
      }
    } else if (e.key === "Escape") {
      onClose();
    }
  };

  if (!isOpen) return null;

  return (
    <div className="fixed inset-0 z-50 flex items-start justify-center pt-20 px-4 bg-black/70 backdrop-blur-md animate-in fade-in duration-150">
      <div
        className="w-full max-w-2xl rounded-2xl border border-slate-800 bg-slate-950 shadow-2xl overflow-hidden flex flex-col max-h-[75vh]"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Search Input Bar */}
        <div className="flex items-center gap-3 px-4 py-3.5 border-b border-slate-800 bg-slate-900/50">
          <Search className="w-5 h-5 text-slate-400 shrink-0" />
          <input
            ref={inputRef}
            type="text"
            value={query}
            onChange={(e) => setQuery(e.target.value)}
            onKeyDown={handleKeyDown}
            placeholder="Search documentation, syntax, stdlib, tools..."
            className="w-full bg-transparent text-sm sm:text-base text-white placeholder-slate-500 focus:outline-none"
          />
          {query && (
            <button
              onClick={() => setQuery("")}
              className="p-1 rounded-md text-slate-500 hover:text-white"
            >
              <X className="w-4 h-4" />
            </button>
          )}
          <kbd className="hidden sm:inline-flex items-center px-1.5 py-0.5 rounded border border-slate-700 bg-slate-900 text-[10px] font-mono text-slate-400">
            ESC
          </kbd>
        </div>

        {/* Search Results List */}
        <div className="flex-1 overflow-y-auto p-2 divide-y divide-slate-900">
          {query.trim() === "" ? (
            <div className="py-12 text-center text-xs text-slate-500">
              Type a keyword like <code className="text-cyan-400 font-mono">match</code>,{" "}
              <code className="text-cyan-400 font-mono">std.io</code>, or{" "}
              <code className="text-cyan-400 font-mono">spm build</code> to search.
            </div>
          ) : results.length === 0 ? (
            <div className="py-12 text-center text-xs text-slate-500">
              No results found for &ldquo;{query}&rdquo;.
            </div>
          ) : (
            results.map((item, idx) => (
              <div
                key={item.id}
                onClick={() => {
                  router.push(item.href);
                  onClose();
                }}
                className={`p-3 rounded-xl cursor-pointer transition-colors flex items-center justify-between gap-3 ${
                  selectedIndex === idx
                    ? "bg-cyan-950/40 border border-cyan-800/40 text-cyan-300"
                    : "hover:bg-slate-900/60 text-slate-200"
                }`}
              >
                <div className="flex flex-col gap-1 min-w-0">
                  <div className="flex items-center gap-2">
                    <span className="text-xs font-semibold truncate text-white">
                      {item.title}
                    </span>
                    <span className="text-[10px] font-mono px-2 py-0.2 rounded bg-slate-900 text-slate-400 border border-slate-800 shrink-0">
                      {item.category}
                    </span>
                  </div>
                  <p className="text-xs text-slate-400 line-clamp-1">{item.excerpt}</p>
                </div>
                <CornerDownLeft className="w-4 h-4 text-slate-500 shrink-0" />
              </div>
            ))
          )}
        </div>

        {/* Footer shortcuts */}
        <div className="px-4 py-2 bg-slate-950 border-t border-slate-900 flex items-center justify-between text-[11px] text-slate-500 font-mono">
          <div className="flex items-center gap-3">
            <span>↑↓ Navigate</span>
            <span>↵ Select</span>
            <span>ESC Close</span>
          </div>
          <span>SP v1.0.0</span>
        </div>
      </div>
    </div>
  );
};
