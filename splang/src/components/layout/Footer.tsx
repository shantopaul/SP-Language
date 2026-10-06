import React from "react";
import Link from "next/link";
import { Github, Heart, Terminal, BookOpen, Boxes, Cpu, ShieldCheck } from "lucide-react";

export const Footer: React.FC = () => {
  return (
    <footer className="border-t border-slate-800/80 bg-slate-950 text-slate-400 text-sm">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-12 lg:py-16">
        <div className="grid grid-cols-1 md:grid-cols-4 gap-8 lg:gap-12">
          {/* Brand Col */}
          <div className="space-y-4 md:col-span-1">
            <div className="flex items-center gap-2.5">
              <div className="w-8 h-8 rounded-lg bg-gradient-to-tr from-emerald-500 to-cyan-500 p-0.5 shadow-md shadow-cyan-500/20">
                <div className="w-full h-full bg-slate-950 rounded-[6px] flex items-center justify-center font-bold text-transparent bg-clip-text bg-gradient-to-tr from-emerald-400 to-cyan-400 text-sm">
                  SP
                </div>
              </div>
              <span className="font-bold text-white text-base">SP Language</span>
            </div>
            <p className="text-slate-400 text-xs leading-relaxed">
              The modern, high-performance systems and application programming language with LLVM native compilation, safe memory ergonomics, and built-in developer tooling.
            </p>
            <div className="pt-2 text-xs text-slate-500">
              Released under Apache-2.0 / MIT License.
            </div>
          </div>

          {/* Ecosystem Column */}
          <div className="space-y-3">
            <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-200">
              Ecosystem & Domains
            </h4>
            <ul className="space-y-2 text-xs">
              <li>
                <a
                  href="https://splang.shantopaul.com"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Cpu className="w-3.5 h-3.5 text-slate-500" />
                  <span>Main Portal (splang.shantopaul.com)</span>
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <BookOpen className="w-3.5 h-3.5 text-slate-500" />
                  <span>Documentation (docs.splang.shantopaul.com)</span>
                </a>
              </li>
              <li>
                <a
                  href="https://registry.splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Boxes className="w-3.5 h-3.5 text-slate-500" />
                  <span>Package Registry (registry.splang.shantopaul.com)</span>
                </a>
              </li>
              <li>
                <Link href="/downloads" className="hover:text-cyan-400 transition-colors">
                  Official Binaries & Toolchains
                </Link>
              </li>
            </ul>
          </div>

          {/* Tools & Resources */}
          <div className="space-y-3">
            <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-200">
              Developer Tools
            </h4>
            <ul className="space-y-2 text-xs">
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/compiler-cli"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Terminal className="w-3.5 h-3.5 text-slate-500" />
                  <span>Compiler (spc build, run, fmt, test)</span>
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/package-manager"
                  className="hover:text-cyan-400 transition-colors"
                >
                  Package Manager (spm init, add, publish)
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/lsp-server"
                  className="hover:text-cyan-400 transition-colors"
                >
                  Language Server Protocol (spls)
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com/tools/debugger"
                  className="hover:text-cyan-400 transition-colors"
                >
                  Debug Adapter Protocol (spd)
                </a>
              </li>
              <li>
                <Link href="/benchmarks" className="hover:text-cyan-400 transition-colors">
                  Multi-Language Benchmarks
                </Link>
              </li>
            </ul>
          </div>

          {/* Project & Community */}
          <div className="space-y-3">
            <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-200">
              Community & Author
            </h4>
            <ul className="space-y-2 text-xs">
              <li>
                <a
                  href="https://github.com/shantopaul/SP-Language"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Github className="w-3.5 h-3.5 text-slate-500" />
                  <span>GitHub Repository</span>
                </a>
              </li>
              <li>
                <Link href="/blog" className="hover:text-cyan-400 transition-colors">
                  Release Notes & Announcements
                </Link>
              </li>
              <li>
                <Link href="/community" className="hover:text-cyan-400 transition-colors">
                  Contributing Guidelines
                </Link>
              </li>
              <li>
                <a
                  href="https://shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-cyan-400 transition-colors"
                >
                  Designed & Created by Shanto Paul
                </a>
              </li>
            </ul>
          </div>
        </div>

        {/* Bottom Bar */}
        <div className="mt-12 pt-8 border-t border-slate-900 flex flex-col sm:flex-row items-center justify-between gap-4 text-xs text-slate-500">
          <p>© {new Date().getFullYear()} SP Programming Language. Developed with passion by Shanto Paul.</p>
          <div className="flex items-center gap-4">
            <span className="flex items-center gap-1 text-slate-400">
              <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
              Production Ready v1.0.0
            </span>
            <span className="text-slate-700">•</span>
            <a
              href="https://github.com/shantopaul/SP-Language"
              className="hover:text-slate-300 transition-colors"
            >
              GitHub
            </a>
          </div>
        </div>
      </div>
    </footer>
  );
};
