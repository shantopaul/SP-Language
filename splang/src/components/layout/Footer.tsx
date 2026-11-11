import React from "react";
import Link from "next/link";
import Image from "next/image";
import { Github, Heart, Terminal, BookOpen, Boxes, Cpu, ShieldCheck, Download } from "lucide-react";
import { ThemeToggle } from "@/components/theme/ThemeToggle";

export const Footer: React.FC = () => {
  return (
    <footer className="border-t border-slate-200 dark:border-[#0A1F44] bg-slate-100 dark:bg-[#020D21] text-slate-600 dark:text-slate-400 text-sm transition-colors">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12 lg:py-16">
        <div className="grid grid-cols-1 md:grid-cols-4 gap-8 lg:gap-12">
          {/* Brand Col */}
          <div className="space-y-4 md:col-span-1">
            <div className="flex items-center gap-3">
              <div className="relative w-8 h-8 rounded-xl p-0.5 shadow-md shadow-blue-500/20">
                <Image
                  src="/brand/brand-emblem-navy.png"
                  alt="SP Language Emblem"
                  width={32}
                  height={32}
                  className="rounded-lg object-contain"
                />
              </div>
              <div className="font-extrabold text-slate-900 dark:text-white text-base tracking-tight flex items-center gap-1">
                <span>SP</span>
                <span className="text-[#0057FF] dark:text-[#00D1FF]">Lang</span>
              </div>
            </div>
            <p className="text-slate-600 dark:text-slate-400 text-xs leading-relaxed">
              The modern, high-performance systems and backend platform language with LLVM native compilation, deterministic ARC memory safety, and 23 standard library modules.
            </p>
            <div className="pt-2 text-xs text-slate-500">
              Released under Apache-2.0 / MIT License.
            </div>
          </div>

          {/* Ecosystem Column */}
          <div className="space-y-3">
            <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-900 dark:text-slate-200">
              Ecosystem &amp; Domains
            </h4>
            <ul className="space-y-2 text-xs">
              <li>
                <a
                  href="https://splang.shantopaul.com"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Cpu className="w-3.5 h-3.5 text-slate-400" />
                  <span>Main Portal (splang.shantopaul.com)</span>
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <BookOpen className="w-3.5 h-3.5 text-slate-400" />
                  <span>Documentation (docs.splang.shantopaul.com)</span>
                </a>
              </li>
              <li>
                <a
                  href="https://registry.splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Boxes className="w-3.5 h-3.5 text-slate-400" />
                  <span>Package Registry (registry.splang.shantopaul.com)</span>
                </a>
              </li>
              <li>
                <Link href="/downloads" className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors flex items-center gap-1.5">
                  <Download className="w-3.5 h-3.5 text-slate-400" />
                  <span>Official Binaries &amp; Toolchains</span>
                </Link>
              </li>
            </ul>
          </div>

          {/* Tools & Resources */}
          <div className="space-y-3">
            <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-900 dark:text-slate-200">
              Developer Tools
            </h4>
            <ul className="space-y-2 text-xs">
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/compiler-cli"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Terminal className="w-3.5 h-3.5 text-slate-400" />
                  <span>Compiler (spc build, run, fmt, test)</span>
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/package-manager"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors"
                >
                  Package Manager (spm init, add, publish)
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/lsp-server"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors"
                >
                  Language Server Protocol (spls)
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/debugger"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors"
                >
                  Debug Adapter Protocol (spd)
                </a>
              </li>
              <li>
                <Link href="/benchmarks" className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors">
                  Multi-Language Benchmarks
                </Link>
              </li>
            </ul>
          </div>

          {/* Project & Community */}
          <div className="space-y-3">
            <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-900 dark:text-slate-200">
              Community &amp; Author
            </h4>
            <ul className="space-y-2 text-xs">
              <li>
                <a
                  href="https://github.com/shantopaul/SP-Language"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Github className="w-3.5 h-3.5 text-slate-400" />
                  <span>GitHub Repository</span>
                </a>
              </li>
              <li>
                <Link href="/blog" className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors">
                  Release Notes &amp; Announcements
                </Link>
              </li>
              <li>
                <a
                  href="https://github.com/shantopaul/SP-Language/issues"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors"
                >
                  Issue Tracker &amp; Bug Reports
                </a>
              </li>
              <li>
                <a
                  href="https://shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-cyan-400 transition-colors"
                >
                  Author Website (shantopaul.com)
                </a>
              </li>
            </ul>
          </div>
        </div>

        {/* Bottom Bar */}
        <div className="mt-12 pt-8 border-t border-slate-200 dark:border-[#0A1F44] flex flex-col sm:flex-row items-center justify-between text-xs text-slate-500 gap-4">
          <div className="flex items-center gap-2">
            <span>&copy; {new Date().getFullYear()} Shanto Paul. All rights reserved.</span>
            <span>•</span>
            <span className="text-[#0057FF] dark:text-cyan-400 font-mono">v4.0.0 GA</span>
          </div>

          <div className="flex items-center gap-4">
            <ThemeToggle variant="segmented" />
            <span className="hidden md:flex items-center gap-1">
              Crafted with <Heart className="w-3.5 h-3.5 text-rose-500 fill-rose-500" /> for systems programmers.
            </span>
          </div>
        </div>
      </div>
    </footer>
  );
};
