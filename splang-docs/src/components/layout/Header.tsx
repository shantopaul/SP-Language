"use client";

import React, { useState, useEffect } from "react";
import Link from "next/link";
import Image from "next/image";
import { Search, Github, ExternalLink, Menu, Boxes, Cpu } from "lucide-react";
import { SearchModal } from "@/components/search/SearchModal";
import { ThemeToggle } from "@/components/theme/ThemeToggle";

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
      <header className="sticky top-0 z-40 w-full border-b border-slate-200 dark:border-[#0A1F44] bg-white/85 dark:bg-[#020D21]/80 backdrop-blur-xl transition-colors">
        <div className="flex items-center justify-between h-16 px-3 sm:px-6 lg:px-8 gap-2">
          {/* Left: Mobile sidebar toggle + Brand */}
          <div className="flex items-center gap-2 sm:gap-3 flex-shrink-0">
            <button
              onClick={onToggleSidebar}
              className="lg:hidden p-2 rounded-xl text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white bg-slate-100 dark:bg-[#081735] border border-slate-200 dark:border-[#0A1F44] focus:outline-none transition-colors min-h-[40px] min-w-[40px] flex items-center justify-center"
              aria-label="Toggle navigation menu"
            >
              <Menu className="w-5 h-5" />
            </button>

            <Link href="/" className="flex items-center gap-2 group flex-shrink-0">
              <div className="relative w-8 h-8 rounded-lg bg-white dark:bg-[#081735] border border-slate-200 dark:border-[#0A1F44] p-1 flex items-center justify-center group-hover:border-[#00D1FF]/50 transition-all duration-300 shadow-sm dark:shadow-[0_0_15px_rgba(0,87,255,0.15)]">
                <Image
                  src="/brand/brand-emblem.png"
                  alt="SP Language Emblem"
                  width={28}
                  height={28}
                  className="object-contain"
                  priority
                />
              </div>
              <div className="flex flex-col">
                <div className="flex items-center gap-1.5">
                  <span className="font-bold text-slate-900 dark:text-white text-sm sm:text-base tracking-tight group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors">
                    SP Docs
                  </span>
                </div>
                <span className="text-[10px] font-mono text-slate-500 dark:text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-cyan-400 -mt-1 transition-colors hidden sm:block">
                  docs.splang.shantopaul.com
                </span>
              </div>
            </Link>

            {/* v3.1.0 GA Live Radar Badge */}
            <div className="hidden md:inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full text-[10px] font-mono bg-[#0057FF]/15 text-[#0057FF] dark:text-[#00D1FF] border border-[#00D1FF]/30 shadow-[0_0_10px_rgba(0,209,255,0.15)]">
              <span className="relative flex h-1.5 w-1.5">
                <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-[#00D1FF] opacity-75"></span>
                <span className="relative inline-flex rounded-full h-1.5 w-1.5 bg-[#00D1FF]"></span>
              </span>
              <span>v3.1.0 GA</span>
            </div>
          </div>

          {/* Center: Search Trigger Bar */}
          <div className="flex-1 max-w-[150px] sm:max-w-xs md:max-w-md mx-1 sm:mx-4">
            <button
              onClick={() => setSearchOpen(true)}
              className="w-full flex items-center justify-between px-2.5 sm:px-3.5 py-2 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 hover:bg-slate-50 dark:hover:bg-[#081735] hover:border-[#0057FF]/40 dark:hover:border-[#00D1FF]/40 text-slate-500 dark:text-slate-400 hover:text-slate-800 dark:hover:text-slate-200 transition-all duration-200 text-xs font-sans group shadow-sm min-h-[40px]"
              aria-label="Search documentation"
            >
              <span className="flex items-center gap-1.5 sm:gap-2 truncate">
                <Search className="w-3.5 h-3.5 text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors flex-shrink-0" />
                <span className="truncate sm:hidden">Search...</span>
                <span className="truncate hidden sm:inline">Search docs, syntax, stdlib...</span>
              </span>
              <kbd className="hidden sm:inline-flex items-center px-1.5 py-0.5 rounded border border-slate-200 dark:border-[#0A1F44] bg-slate-100 dark:bg-[#020D21] text-[10px] font-mono text-slate-500 dark:text-slate-400 group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] group-hover:border-[#00D1FF]/30 transition-colors flex-shrink-0">
                Ctrl+K
              </kbd>
            </button>
          </div>

          {/* Right: Ecosystem portal links, Theme Toggle & GitHub */}
          <div className="flex items-center gap-2 sm:gap-3 text-xs font-medium">
            <a
              href="https://splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hidden lg:flex items-center gap-1 text-slate-600 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white transition-colors"
            >
              <Cpu className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF]" />
              <span>Main Site</span>
              <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
            </a>

            <a
              href="https://registry.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hidden lg:flex items-center gap-1 text-slate-600 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white transition-colors"
            >
              <Boxes className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF]" />
              <span>Registry</span>
              <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
            </a>

            {/* Theme Mode Toggle */}
            <ThemeToggle variant="segmented" className="hidden sm:inline-flex" />
            <ThemeToggle variant="button" className="sm:hidden" />

            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735] text-slate-700 dark:text-slate-200 hover:text-slate-900 dark:hover:text-white hover:border-[#0057FF]/50 transition-all shadow-sm"
            >
              <Github className="w-4 h-4 text-slate-700 dark:text-white" />
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
