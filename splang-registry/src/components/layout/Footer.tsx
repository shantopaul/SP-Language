import React from "react";
import Link from "next/link";
import Image from "next/image";
import { Boxes, ExternalLink, ShieldCheck, Terminal, BookOpen, Cpu, Heart } from "lucide-react";
import { ThemeToggle } from "@/components/theme/ThemeToggle";

export const Footer: React.FC = () => {
  return (
    <footer className="border-t border-slate-200 dark:border-[#0A1F44] bg-slate-100 dark:bg-[#020D21] text-slate-600 dark:text-slate-400 py-12 mt-20 transition-colors">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="grid grid-cols-1 md:grid-cols-4 gap-8 mb-12">
          {/* Brand Col */}
          <div className="md:col-span-1">
            <Link href="/" className="flex items-center gap-3 mb-4 group">
              <div className="relative w-8 h-8 rounded-lg overflow-hidden border border-[#0A1F44] bg-[#081735] flex items-center justify-center p-1 group-hover:border-[#00D1FF]/50 transition-colors">
                <Image
                  src="/brand/brand-emblem-navy.png"
                  alt="SP Language Emblem"
                  width={28}
                  height={28}
                  className="object-contain"
                />
              </div>
              <span className="font-extrabold text-slate-900 dark:text-white text-base tracking-tight group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors">
                SP Registry
              </span>
            </Link>
            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed mb-4">
              The official centralized package repository for the SP Programming Language. Distributing cryptographically verified, high-performance packages for SP 3.1.0 GA.
            </p>
            <div className="inline-flex items-center gap-2 px-2.5 py-1 rounded-full border border-blue-200 dark:border-[#00D1FF]/30 bg-blue-50 dark:bg-[#0057FF]/15 text-[#0057FF] dark:text-[#00D1FF] text-xs font-mono">
              <ShieldCheck className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF]" />
              <span>SHA-256 Verified Registry</span>
            </div>
          </div>

          {/* Registry Navigation */}
          <div>
            <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-900 dark:text-slate-200 mb-4">
              Registry
            </h3>
            <ul className="space-y-2.5 text-xs">
              <li>
                <Link href="/packages" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors">
                  Explore All Packages
                </Link>
              </li>
              <li>
                <Link href="/publish" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors">
                  Publishing Guide
                </Link>
              </li>
              <li>
                <Link href="/security" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors">
                  Security &amp; Integrity
                </Link>
              </li>
              <li>
                <Link href="/categories/utilities" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors">
                  Utilities &amp; Helpers
                </Link>
              </li>
              <li>
                <Link href="/categories/web" className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors">
                  Web &amp; Networking
                </Link>
              </li>
            </ul>
          </div>

          {/* Developer CLI Tools */}
          <div>
            <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-900 dark:text-slate-200 mb-4">
              SPM CLI &amp; Protocol
            </h3>
            <ul className="space-y-2.5 text-xs font-mono">
              <li>
                <span className="text-slate-700 dark:text-slate-300">spm search &lt;query&gt;</span>
              </li>
              <li>
                <span className="text-slate-700 dark:text-slate-300">spm add &lt;pkg&gt;</span>
              </li>
              <li>
                <span className="text-slate-700 dark:text-slate-300">spm publish</span>
              </li>
              <li>
                <span className="text-slate-700 dark:text-slate-300">spm verify</span>
              </li>
              <li className="pt-2">
                <a
                  href="/api/v1/packages"
                  className="text-[#0057FF] dark:text-[#00D1FF] hover:underline flex items-center gap-1 font-mono text-[11px]"
                >
                  <Terminal className="w-3 h-3" />
                  <span>REST API v1 Endpoints</span>
                </a>
              </li>
            </ul>
          </div>

          {/* SP Ecosystem Network */}
          <div>
            <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-900 dark:text-slate-200 mb-4">
              Ecosystem
            </h3>
            <ul className="space-y-2.5 text-xs">
              <li>
                <a
                  href="https://splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5"
                >
                  <Cpu className="w-3.5 h-3.5 text-slate-400" />
                  <span>Official Website</span>
                  <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5"
                >
                  <BookOpen className="w-3.5 h-3.5 text-slate-400" />
                  <span>Documentation Portal</span>
                  <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
                </a>
              </li>
              <li>
                <a
                  href="https://github.com/shantopaul/SP-Language"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors flex items-center gap-1.5"
                >
                  <Boxes className="w-3.5 h-3.5 text-slate-400" />
                  <span>GitHub Repository</span>
                  <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-500" />
                </a>
              </li>
            </ul>
          </div>
        </div>

        {/* Bottom Bar */}
        <div className="pt-8 border-t border-slate-200 dark:border-[#0A1F44] flex flex-col sm:flex-row items-center justify-between text-xs text-slate-500 gap-4">
          <p>© 2026 SP Language Initiative by Shanto Paul. Released under Apache-2.0 / MIT licenses.</p>
          <div className="flex items-center gap-4">
            <ThemeToggle variant="segmented" />
            <span className="hidden md:inline text-slate-500 dark:text-slate-400">Crafted for high-performance systems engineering</span>
          </div>
        </div>
      </div>
    </footer>
  );
};
