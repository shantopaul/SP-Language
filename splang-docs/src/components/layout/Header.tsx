"use client";

import React, { useState, useEffect } from "react";
import Link from "next/link";
import Image from "next/image";
import { Search, Github, ExternalLink, Menu, Boxes, Cpu } from "lucide-react";
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
      <header className="sticky top-0 z-40 w-full border-b border-[#0A1F44] bg-[#020D21]/80 backdrop-blur-xl">
        <div className="flex items-center justify-between h-16 px-4 sm:px-6 lg:px-8">
          {/* Left: Mobile sidebar toggle + Brand */}
          <div className="flex items-center gap-3">
            <button
              onClick={onToggleSidebar}
              className="lg:hidden p-2 rounded-lg text-slate-400 hover:text-white hover:bg-[#081735] focus:outline-none transition-colors"
              aria-label="Toggle navigation"
            >
              <Menu className="w-5 h-5" />
            </button>

            <Link href="/" className="flex items-center gap-2.5 group">
              <div className="relative w-8 h-8 rounded-lg bg-[#081735] border border-[#0A1F44] p-1 flex items-center justify-center group-hover:border-[#00D1FF]/50 transition-all duration-300 shadow-[0_0_15px_rgba(0,87,255,0.15)]">
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
                  <span className="font-bold text-white text-sm tracking-tight group-hover:text-[#00D1FF] transition-colors">
                    SP Docs
                  </span>
                </div>
                <span className="text-[10px] font-mono text-slate-400 group-hover:text-cyan-400 -mt-1 transition-colors">
                  docs.splang.shantopaul.com
                </span>
              </div>
            </Link>

            {/* v3.1.0 GA Live Radar Badge */}
            <div className="hidden sm:inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full text-[10px] font-mono bg-[#0057FF]/15 text-[#00D1FF] border border-[#00D1FF]/30 ml-2 shadow-[0_0_10px_rgba(0,209,255,0.15)]">
              <span className="relative flex h-1.5 w-1.5">
                <span className="animate-ping absolute inline-flex h-full w-full rounded-full bg-[#00D1FF] opacity-75"></span>
                <span className="relative inline-flex rounded-full h-1.5 w-1.5 bg-[#00D1FF]"></span>
              </span>
              <span>v3.1.0 GA</span>
            </div>
          </div>

          {/* Center: Search Trigger Bar */}
          <div className="flex-1 max-w-md mx-4">
            <button
              onClick={() => setSearchOpen(true)}
              className="w-full flex items-center justify-between px-3.5 py-1.5 rounded-xl border border-[#0A1F44] bg-[#081735]/70 hover:bg-[#081735] hover:border-[#00D1FF]/40 text-slate-400 hover:text-slate-200 transition-all duration-200 text-xs font-sans group shadow-sm"
            >
              <span className="flex items-center gap-2">
                <Search className="w-3.5 h-3.5 text-slate-400 group-hover:text-[#00D1FF] transition-colors" />
                <span>Search docs, syntax, stdlib, tools...</span>
              </span>
              <kbd className="hidden sm:inline-flex items-center px-1.5 py-0.5 rounded border border-[#0A1F44] bg-[#020D21] text-[10px] font-mono text-slate-400 group-hover:text-[#00D1FF] group-hover:border-[#00D1FF]/30 transition-colors">
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
              className="hidden md:flex items-center gap-1 text-slate-300 hover:text-white transition-colors"
            >
              <Cpu className="w-3.5 h-3.5 text-[#00D1FF]" />
              <span>Main Site</span>
              <ExternalLink className="w-3 h-3 text-slate-500" />
            </a>

            <a
              href="https://registry.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hidden md:flex items-center gap-1 text-slate-300 hover:text-white transition-colors"
            >
              <Boxes className="w-3.5 h-3.5 text-[#00D1FF]" />
              <span>Registry</span>
              <ExternalLink className="w-3 h-3 text-slate-500" />
            </a>

            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-[#0A1F44] bg-[#081735] text-slate-200 hover:text-white hover:border-[#0057FF]/50 transition-all shadow-sm"
            >
              <Github className="w-4 h-4 text-white" />
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
