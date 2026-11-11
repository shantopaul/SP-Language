"use client";

import React, { useState } from "react";
import Link from "next/link";
import Image from "next/image";
import { usePathname } from "next/navigation";
import {
  BookOpen,
  Boxes,
  Code2,
  Download,
  Github,
  Menu,
  Sparkles,
  X,
  ExternalLink,
  Flame,
} from "lucide-react";
import { Badge } from "@/components/ui/Badge";
import { Button } from "@/components/ui/Button";
import { ThemeToggle } from "@/components/theme/ThemeToggle";

export const Navbar: React.FC = () => {
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);
  const pathname = usePathname();

  const navLinks = [
    { name: "Documentation", href: "https://docs.splang.shantopaul.com", external: true, icon: BookOpen },
    { name: "Registry", href: "https://registry.splang.shantopaul.com", external: true, icon: Boxes },
    { name: "Benchmarks", href: "/benchmarks", icon: Flame },
    { name: "Downloads", href: "/downloads", icon: Download },
    { name: "Community", href: "/community", icon: Code2 },
  ];

  return (
    <header className="sticky top-0 z-50 w-full border-b border-slate-200 dark:border-[#0A1F44] bg-white/85 dark:bg-[#020D21]/85 backdrop-blur-xl transition-colors">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          {/* Official Brand Logo & Version */}
          <div className="flex items-center gap-3">
            <Link href="/" className="flex items-center gap-3 group">
              <div className="relative w-9 h-9 rounded-xl p-0.5 shadow-md shadow-blue-500/20 group-hover:shadow-cyan-400/35 transition-all">
                <Image
                  src="/brand/brand-emblem-navy.png"
                  alt="SP Language Emblem"
                  width={36}
                  height={36}
                  className="rounded-lg object-contain"
                  priority
                />
              </div>
              <div className="flex flex-col">
                <div className="font-extrabold text-slate-900 dark:text-white text-base tracking-tight leading-none group-hover:text-cyan-500 dark:group-hover:text-cyan-300 transition-colors flex items-center gap-1">
                  <span>SP</span>
                  <span className="text-[#0057FF] dark:text-[#00D1FF]">Lang</span>
                </div>
                <span className="text-[10px] font-mono text-slate-500 dark:text-slate-400 mt-0.5">
                  splang.shantopaul.com
                </span>
              </div>
            </Link>
            <Link href="/downloads">
              <Badge variant="success" className="hidden sm:inline-flex ml-2 cursor-pointer hover:border-cyan-400 transition-all">
                <span className="w-1.5 h-1.5 rounded-full bg-[#00D1FF] animate-ping" />
                v4.0.0 GA
              </Badge>
            </Link>
          </div>

          {/* Desktop Navigation Links */}
          <nav className="hidden md:flex items-center gap-1">
            {navLinks.map((link) => {
              const Icon = link.icon;
              const isActive = pathname === link.href;

              if (link.external) {
                return (
                  <a
                    key={link.name}
                    href={link.href}
                    target="_blank"
                    rel="noopener noreferrer"
                    className="flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-sm font-medium text-slate-600 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] hover:border-slate-300 dark:hover:border-[#0A1F44] border border-transparent transition-all group"
                  >
                    <Icon className="w-4 h-4 text-slate-400 group-hover:text-cyan-500 dark:group-hover:text-cyan-400 transition-colors" />
                    <span>{link.name}</span>
                    <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500 group-hover:text-slate-600 dark:group-hover:text-slate-300" />
                  </a>
                );
              }

              return (
                <Link
                  key={link.name}
                  href={link.href}
                  className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-sm font-medium transition-all ${
                    isActive
                      ? "text-[#0057FF] dark:text-cyan-300 bg-blue-50/80 dark:bg-[#081735] border border-blue-200 dark:border-cyan-500/40 shadow-sm dark:shadow-glow-sm"
                      : "text-slate-600 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] hover:border-slate-300 dark:hover:border-[#0A1F44] border border-transparent"
                  }`}
                >
                  <Icon className={`w-4 h-4 ${isActive ? "text-[#0057FF] dark:text-cyan-400" : "text-slate-400"}`} />
                  <span>{link.name}</span>
                </Link>
              );
            })}
          </nav>

          {/* Right Action Buttons */}
          <div className="hidden sm:flex items-center gap-2.5">
            {/* Theme Mode Toggle */}
            <ThemeToggle variant="segmented" />

            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="flex items-center gap-2 px-3 py-1.5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/80 text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:border-[#0057FF]/40 dark:hover:border-cyan-500/40 hover:bg-slate-50 dark:hover:bg-[#0A1F44] transition-all text-sm font-medium shadow-sm"
            >
              <Github className="w-4 h-4 text-slate-500 dark:text-slate-400" />
              <span>GitHub</span>
            </a>
            <a
              href="https://docs.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
            >
              <Button size="sm" variant="primary">
                <Sparkles className="w-3.5 h-3.5" />
                <span>Get Started</span>
              </Button>
            </a>
          </div>

          {/* Mobile controls */}
          <div className="sm:hidden flex items-center gap-1.5">
            <ThemeToggle variant="button" />
            <button
              onClick={() => setMobileMenuOpen(!mobileMenuOpen)}
              className="min-h-[44px] min-w-[44px] flex items-center justify-center p-2 rounded-xl text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] border border-transparent hover:border-slate-300 dark:hover:border-[#0A1F44] focus:outline-none"
              aria-label="Toggle menu"
            >
              {mobileMenuOpen ? <X className="w-6 h-6" /> : <Menu className="w-6 h-6" />}
            </button>
          </div>
        </div>
      </div>

      {/* Mobile Drawer */}
      {mobileMenuOpen && (
        <div className="md:hidden border-b border-slate-200 dark:border-[#0A1F44] bg-white/95 dark:bg-[#020D21]/95 backdrop-blur-2xl px-4 pt-2 pb-6 space-y-2">
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
                  className="min-h-[44px] flex items-center justify-between px-3.5 py-2.5 rounded-xl text-base font-medium text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] border border-transparent hover:border-slate-200 dark:hover:border-[#0A1F44]"
                >
                  <span className="flex items-center gap-2.5">
                    <Icon className="w-5 h-5 text-[#0057FF] dark:text-cyan-400" />
                    {link.name}
                  </span>
                  <ExternalLink className="w-4 h-4 text-slate-400" />
                </a>
              );
            }
            return (
              <Link
                key={link.name}
                href={link.href}
                onClick={() => setMobileMenuOpen(false)}
                className="min-h-[44px] flex items-center justify-between px-3.5 py-2.5 rounded-xl text-base font-medium text-slate-700 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735] border border-transparent hover:border-slate-200 dark:hover:border-[#0A1F44]"
              >
                <span className="flex items-center gap-2.5">
                  <Icon className="w-5 h-5 text-[#0057FF] dark:text-cyan-400" />
                  {link.name}
                </span>
              </Link>
            );
          })}
          <div className="pt-3 pb-1">
            <div className="text-xs font-mono uppercase text-slate-500 mb-2">Theme Mode</div>
            <ThemeToggle variant="segmented" className="w-full justify-around" />
          </div>
          <div className="pt-2 flex flex-col gap-2.5">
            <a
              href="https://github.com/shantopaul/SP-Language"
              target="_blank"
              rel="noopener noreferrer"
              className="min-h-[44px] flex items-center justify-center gap-2 w-full py-2.5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735] text-slate-800 dark:text-slate-200 text-sm font-medium hover:border-[#0057FF]/40 shadow-sm"
            >
              <Github className="w-4 h-4" />
              <span>View on GitHub</span>
            </a>
            <a
              href="https://docs.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="w-full"
            >
              <Button className="w-full min-h-[44px] justify-center">Get Started</Button>
            </a>
          </div>
        </div>
      )}
    </header>
  );
};
