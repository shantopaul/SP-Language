"use client";

import React, { useState, useEffect } from "react";
import Link from "next/link";
import { Search, Github, ExternalLink, Menu, X, BookOpen, Boxes, Cpu } from "lucide-react";
import { SearchModal } from "@/components/search/SearchModal";

interface HeaderProps {
  onToggleSidebar?: () => void;
}

export const Header: React.FC<HeaderProps> = ({ onToggleSidebar }) => {
  const [searchOpen, setSearchOpen] = useState(false);

  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.ctrlKey || e.metaKey) && e.key === "k") {
        e.preventDefault();
        setSearchOpen((prev) => !prev);
      }
    };
    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, []);

  return (
    <>
      <header className="sticky top-0 z-40 w-full border-b border-slate-800 bg-slate-950/80 backdrop-blur-xl">
        <div className="flex items-center justify-between h-16 px-4 sm:px-6 lg:px-8">
          {/* Left: Mobile sidebar toggle + Brand */}
          <div className="flex items-center gap-3">
            <button
              onClick={onToggleSidebar}
              className="lg:hidden p-2 rounded-lg text-slate-400 hover:text-white hover:bg-slate-900 focus:outline-none"
              aria-label="Toggle navigation"
            >
              <Menu className="w-5 h-5" />
            </button>

            <Link href="/" className="flex items-center gap-2.5">
              <div className="w-8 h-8 rounded-lg bg-gradient-to-tr from-emerald-500 via-cyan-500 to-blue-500 p-0.5 shadow-md shadow-cyan-500/20">
                <div className="w-full h-full bg-slate-950 rounded-[6px] flex items-center justify-center font-bold text-transparent bg-clip-text bg-gradient-to-tr from-emerald-400 to-cyan-400 text-sm">
                  SP
                </div>
              </div>
              <div className="flex flex-col">
                <span className="font-bold text-white text-sm tracking-tight">
                  SP Docs
                </span>
                <span className="text-[10px] font-mono text-cyan-400 -mt-1">
                  docs.splang.shantopaul.com
                </span>
              </div>
            </Link>

            <span className="hidden sm:inline-flex px-2 py-0.5 rounded-full text-[10px] font-mono bg-emerald-950 text-emerald-400 border border-emerald-800 ml-2">
              v1.0.0
            </span>
          </div>

          {/* Center: Search Trigger Bar */}
          <div className="flex-1 max-w-md mx-4">
            <button
              onClick={() => setSearchOpen(true)}
              className="w-full flex items-center justify-between px-3.5 py-1.5 rounded-xl border border-slate-800 bg-slate-900/60 hover:bg-slate-900 text-slate-400 hover:text-slate-200 transition-colors text-xs font-sans group"
            >
              <span className="flex items-center gap-2">
                <Search className="w-3.5 h-3.5 text-slate-500 group-hover:text-cyan-400 transition-colors" />
                <span>Search docs...</span>
              </span>
              <kbd className="hidden sm:inline-flex items-center px-1.5 py-0.5 rounded border border-slate-800 bg-slate-950 text-[10px] font-mono text-slate-500">
                Ctrl+K
              </kbd>
            </button>
          </div>

          {/* Right: Ecosystem portal links & GitHub */}
          <div className="flex items-center gap-2 sm:gap-4 text-xs font-medium">
            <a
              href="https://splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hidden md:flex items-center gap-1 text-slate-400 hover:text-white transition-colors"
            >
              <Cpu className="w-3.5 h-3.5 text-slate-500" />
              <span>Main Site</span>
              <ExternalLink className="w-3 h-3 text-slate-600" />
            </a>

            <a
              href="https://registry.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hidden md:flex items-center gap-1 text-slate-400 hover:text-white transition-colors"
            >
              <Boxes className="w-3.5 h-3.5 text-slate-500" />
              <span>Registry</span>
              <ExternalLink className="w-3 h-3 text-slate-600" />
            </a>

            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg border border-slate-800 bg-slate-900/80 text-slate-300 hover:text-white transition-colors"
            >
              <Github className="w-4 h-4" />
              <span className="hidden sm:inline">GitHub</span>
            </a>
          </div>
        </div>
      </header>

      {/* Global Search Modal */}
      <SearchModal isOpen={searchOpen} onClose={() => setSearchOpen(false)} />
    </>
  );
};
