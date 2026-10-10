"use client";

import React, { useState } from "react";
import Link from "next/link";
import Image from "next/image";
import { BookOpen, Cpu, Github, ExternalLink, Upload, Shield, PackageSearch, Menu, X } from "lucide-react";
import { ThemeToggle } from "@/components/theme/ThemeToggle";

export const Navbar: React.FC = () => {
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);

  const navLinks = [
    { name: "Explore Packages", href: "/packages", icon: PackageSearch, external: false },
    { name: "Publish Guide", href: "/publish", icon: Upload, external: false },
    { name: "Security & Integrity", href: "/security", icon: Shield, external: false },
    { name: "Documentation", href: "https://docs.splang.shantopaul.com", icon: BookOpen, external: true },
    { name: "Main Website", href: "https://splang.shantopaul.com", icon: Cpu, external: true },
  ];

  return (
    <header className="sticky top-0 z-50 w-full border-b border-slate-200 dark:border-[#0A1F44] bg-white/85 dark:bg-[#020D21]/80 backdrop-blur-xl transition-colors">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          {/* Logo & Portal Name */}
          <div className="flex items-center gap-3">
            <Link href="/" className="flex items-center gap-2.5 sm:gap-3 group">
              <div className="relative w-8 h-8 sm:w-9 sm:h-9 rounded-xl overflow-hidden border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735] flex items-center justify-center p-1 group-hover:border-[#00D1FF]/50 shadow-sm dark:shadow-[0_0_15px_rgba(0,87,255,0.2)] transition-all flex-shrink-0">
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
                <div className="flex items-center gap-1.5 sm:gap-2">
                  <span className="font-extrabold text-slate-900 dark:text-white text-sm sm:text-base tracking-tight group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors">
                    SP Registry
                  </span>
                  <span className="hidden sm:inline-flex items-center gap-1.5 px-2 py-0.5 rounded-full border border-[#00D1FF]/40 bg-[#0057FF]/15 text-[#0057FF] dark:text-[#00D1FF] text-[10px] font-mono font-semibold shadow-[0_0_12px_rgba(0,209,255,0.2)]">
                    <span className="w-1.5 h-1.5 rounded-full bg-[#00D1FF] animate-pulse" />
                    <span>v3.1.0 GA</span>
                  </span>
                </div>
                <span className="text-[10px] font-mono text-slate-500 dark:text-[#00D1FF] -mt-0.5 truncate max-w-[150px] sm:max-w-none">
                  register.splang.shantopaul.com
                </span>
              </div>
            </Link>
          </div>

          {/* Desktop Navigation Links */}
          <nav className="hidden md:flex items-center gap-6 text-xs font-medium text-slate-600 dark:text-slate-300">
            <Link href="/packages" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5">
              <PackageSearch className="w-3.5 h-3.5 text-slate-400" />
              <span>Explore Packages</span>
            </Link>
            <Link href="/publish" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5">
              <Upload className="w-3.5 h-3.5 text-slate-400" />
              <span>Publish Guide</span>
            </Link>
            <Link href="/security" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5">
              <Shield className="w-3.5 h-3.5 text-slate-400" />
              <span>Security &amp; Hashes</span>
            </Link>
            <a
              href="https://docs.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5"
            >
              <BookOpen className="w-3.5 h-3.5 text-slate-400" />
              <span>Docs</span>
              <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
            </a>
            <a
              href="https://splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5"
            >
              <Cpu className="w-3.5 h-3.5 text-slate-400" />
              <span>Main Site</span>
              <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
            </a>
          </nav>

          {/* Right Action */}
          <div className="flex items-center gap-1.5 sm:gap-3">
            <ThemeToggle variant="segmented" className="hidden sm:inline-flex" />
            <ThemeToggle variant="button" className="sm:hidden" />

            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="hidden sm:flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735] text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:border-[#0057FF]/60 hover:bg-slate-50 dark:hover:bg-[#0057FF]/10 transition-all text-xs font-medium shadow-sm"
            >
              <Github className="w-3.5 h-3.5" />
              <span>GitHub</span>
            </a>

            {/* Mobile Hamburger Menu Toggle */}
            <button
              onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
              className="md:hidden p-2 rounded-xl text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] border border-transparent hover:border-slate-300 dark:hover:border-[#0A1F44] focus:outline-none transition-colors"
              aria-label="Toggle menu"
            >
              {mobileMenuOpen ? <X className="w-5 h-5" /> : <Menu className="w-5 h-5" />}
            </button>
          </div>
        </div>
      </div>

      {/* Mobile Drawer */}
      {mobileMenuOpen && (
        <div className="md:hidden border-b border-slate-200 dark:border-[#0A1F44] bg-white/95 dark:bg-[#020D21]/95 backdrop-blur-2xl px-4 pt-3 pb-6 space-y-2 shadow-xl animate-in slide-in-from-top-2 duration-200">
          <div className="pb-2 mb-2 border-b border-slate-100 dark:border-[#0A1F44]/50 flex items-center justify-between">
            <span className="text-[10px] font-mono uppercase tracking-wider text-slate-500">Navigation</span>
            <span className="inline-flex items-center gap-1 px-2 py-0.5 rounded-full text-[10px] font-mono bg-[#0057FF]/15 text-[#0057FF] dark:text-[#00D1FF] border border-[#00D1FF]/30">
              v3.1.0 GA
            </span>
          </div>

          {navLinks.map((link) => {
            const Icon = link.icon;
            if (link.external) {
              return (
                <a
                  key={link.name}
                  href={link.href}
                  target="_blank"
                  rel="noopener noreferrer"
                  onClick={() => setMobileMenuOpen(false)}
                  className="flex items-center justify-between px-3.5 py-3 rounded-xl text-sm font-medium text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] border border-transparent hover:border-slate-200 dark:hover:border-[#0A1F44] transition-all min-h-[44px]"
                >
                  <span className="flex items-center gap-3">
                    <Icon className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
                    <span>{link.name}</span>
                  </span>
                  <ExternalLink className="w-3.5 h-3.5 text-slate-400" />
                </a>
              );
            }

            return (
              <Link
                key={link.name}
                href={link.href}
                onClick={() => setMobileMenuOpen(false)}
                className="flex items-center justify-between px-3.5 py-3 rounded-xl text-sm font-medium text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] border border-transparent hover:border-slate-200 dark:hover:border-[#0A1F44] transition-all min-h-[44px]"
              >
                <span className="flex items-center gap-3">
                  <Icon className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
                  <span>{link.name}</span>
                </span>
              </Link>
            );
          })}

          <div className="pt-3 pb-1 border-t border-slate-100 dark:border-[#0A1F44]/50">
            <div className="text-[10px] font-mono uppercase tracking-wider text-slate-500 mb-2">Display Theme</div>
            <ThemeToggle variant="segmented" className="w-full justify-around" />
          </div>

          <div className="pt-2">
            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center justify-center gap-2 w-full py-2.5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735] text-slate-800 dark:text-slate-200 text-sm font-medium hover:border-[#0057FF]/40 shadow-sm min-h-[44px]"
            >
              <Github className="w-4 h-4" />
              <span>SP-Language on GitHub</span>
            </a>
          </div>
        </div>
      )}
    </header>
  );
};
