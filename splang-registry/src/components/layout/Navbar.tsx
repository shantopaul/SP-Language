"use client";

import React from "react";
import Link from "next/link";
import Image from "next/image";
import { BookOpen, Cpu, Github, ExternalLink, Upload, Shield, PackageSearch } from "lucide-react";

export const Navbar: React.FC = () => {
  return (
    <header className="sticky top-0 z-40 w-full border-b border-[#0A1F44] bg-[#020D21]/80 backdrop-blur-xl">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          {/* Logo & Portal Name */}
          <div className="flex items-center gap-3">
            <Link href="/" className="flex items-center gap-3 group">
              <div className="relative w-9 h-9 rounded-xl overflow-hidden border border-[#0A1F44] bg-[#081735] flex items-center justify-center p-1 group-hover:border-[#00D1FF]/50 shadow-[0_0_15px_rgba(0,87,255,0.2)] transition-all">
                <Image
                  src="/brand/brand-emblem-navy.png"
                  alt="SP Language Emblem"
                  width={32}
                  height={32}
                  className="object-contain"
                  priority
                />
              </div>
              <div className="flex flex-col">
                <div className="flex items-center gap-2">
                  <span className="font-extrabold text-white text-sm tracking-tight group-hover:text-[#00D1FF] transition-colors">
                    SP Registry
                  </span>
                  <span className="hidden sm:inline-flex items-center gap-1.5 px-2 py-0.5 rounded-full border border-[#00D1FF]/40 bg-[#0057FF]/15 text-[#00D1FF] text-[10px] font-mono font-semibold shadow-[0_0_12px_rgba(0,209,255,0.2)]">
                    <span className="w-1.5 h-1.5 rounded-full bg-[#00D1FF] animate-pulse" />
                    <span>v3.1.0 GA</span>
                  </span>
                </div>
                <span className="text-[10px] font-mono text-[#00D1FF] -mt-0.5">
                  register.splang.shantopaul.com
                </span>
              </div>
            </Link>
          </div>

          {/* Navigation Links */}
          <nav className="hidden md:flex items-center gap-6 text-xs font-medium text-slate-300">
            <Link href="/packages" className="hover:text-[#00D1FF] transition-colors flex items-center gap-1.5">
              <PackageSearch className="w-3.5 h-3.5 text-slate-400" />
              <span>Explore Packages</span>
            </Link>
            <Link href="/publish" className="hover:text-[#00D1FF] transition-colors flex items-center gap-1.5">
              <Upload className="w-3.5 h-3.5 text-slate-400" />
              <span>Publish Guide</span>
            </Link>
            <Link href="/security" className="hover:text-[#00D1FF] transition-colors flex items-center gap-1.5">
              <Shield className="w-3.5 h-3.5 text-slate-400" />
              <span>Security &amp; Hashes</span>
            </Link>
            <a
              href="https://docs.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-[#00D1FF] transition-colors flex items-center gap-1.5"
            >
              <BookOpen className="w-3.5 h-3.5 text-slate-400" />
              <span>Docs</span>
              <ExternalLink className="w-3 h-3 text-slate-500" />
            </a>
            <a
              href="https://splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-[#00D1FF] transition-colors flex items-center gap-1.5"
            >
              <Cpu className="w-3.5 h-3.5 text-slate-400" />
              <span>Main Site</span>
              <ExternalLink className="w-3 h-3 text-slate-500" />
            </a>
          </nav>

          {/* Right Action */}
          <div className="flex items-center gap-3">
            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-[#0A1F44] bg-[#081735] text-slate-300 hover:text-white hover:border-[#0057FF]/60 hover:bg-[#0057FF]/10 transition-all text-xs font-medium"
            >
              <Github className="w-3.5 h-3.5" />
              <span>GitHub</span>
            </a>
          </div>
        </div>
      </div>
    </header>
  );
};
