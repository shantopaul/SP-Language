import React from "react";
import Link from "next/link";
import { Boxes, ExternalLink, ShieldCheck, Terminal, BookOpen, Cpu, Heart } from "lucide-react";

export const Footer: React.FC = () => {
  return (
    <footer className="border-t border-slate-800 bg-slate-950 text-slate-400 py-12 mt-20">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="grid grid-cols-1 md:grid-cols-4 gap-8 mb-12">
          {/* Brand Col */}
          <div className="md:col-span-1">
            <Link href="/" className="flex items-center gap-2.5 mb-4">
              <div className="w-8 h-8 rounded-lg bg-gradient-to-tr from-cyan-500 via-blue-500 to-indigo-500 p-0.5 shadow-md shadow-cyan-500/20">
                <div className="w-full h-full bg-slate-950 rounded-[6px] flex items-center justify-center font-bold text-transparent bg-clip-text bg-gradient-to-tr from-cyan-400 to-blue-400 text-sm">
                  SP
                </div>
              </div>
              <span className="font-bold text-white text-base tracking-tight">
                SP Package Registry
              </span>
            </Link>
            <p className="text-xs text-slate-400 leading-relaxed mb-4">
              The official centralized package repository for the SP Programming Language. Distributing cryptographically verified, high-performance packages for SP 1.0.
            </p>
            <div className="flex items-center gap-2 text-xs font-mono text-cyan-400">
              <ShieldCheck className="w-4 h-4 text-emerald-400" />
              <span>SHA-256 Verified Registry</span>
            </div>
          </div>

          {/* Registry Navigation */}
          <div>
            <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-200 mb-4">
              Registry
            </h3>
            <ul className="space-y-2.5 text-xs">
              <li>
                <Link href="/packages" className="hover:text-cyan-400 transition-colors">
                  Explore All Packages
                </Link>
              </li>
              <li>
                <Link href="/publish" className="hover:text-cyan-400 transition-colors">
                  Publishing Guide
                </Link>
              </li>
              <li>
                <Link href="/security" className="hover:text-cyan-400 transition-colors">
                  Security & Integrity
                </Link>
              </li>
              <li>
                <Link href="/categories/utilities" className="hover:text-cyan-400 transition-colors">
                  Utilities & Helpers
                </Link>
              </li>
              <li>
                <Link href="/categories/web" className="hover:text-cyan-400 transition-colors">
                  Web & Networking
                </Link>
              </li>
            </ul>
          </div>

          {/* Developer CLI Tools */}
          <div>
            <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-200 mb-4">
              SPM CLI & Protocol
            </h3>
            <ul className="space-y-2.5 text-xs">
              <li>
                <span className="font-mono text-slate-300">spm search &lt;query&gt;</span>
              </li>
              <li>
                <span className="font-mono text-slate-300">spm add &lt;pkg&gt;</span>
              </li>
              <li>
                <span className="font-mono text-slate-300">spm publish</span>
              </li>
              <li>
                <span className="font-mono text-slate-300">spm verify</span>
              </li>
              <li className="pt-2">
                <a
                  href="/api/v1/packages"
                  className="text-cyan-400 hover:underline flex items-center gap-1 font-mono text-[11px]"
                >
                  <Terminal className="w-3 h-3" />
                  <span>REST API v1 Endpoints</span>
                </a>
              </li>
            </ul>
          </div>

          {/* SP Ecosystem Network */}
          <div>
            <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-200 mb-4">
              Ecosystem
            </h3>
            <ul className="space-y-2.5 text-xs">
              <li>
                <a
                  href="https://splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Cpu className="w-3.5 h-3.5 text-slate-400" />
                  <span>Official Website</span>
                  <ExternalLink className="w-3 h-3 text-slate-500" />
                </a>
              </li>
              <li>
                <a
                  href="https://docs.splang.shantopaul.com"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <BookOpen className="w-3.5 h-3.5 text-slate-400" />
                  <span>Documentation Portal</span>
                  <ExternalLink className="w-3 h-3 text-slate-500" />
                </a>
              </li>
              <li>
                <a
                  href="https://github.com/shantopaul/SP-Language"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="hover:text-cyan-400 transition-colors flex items-center gap-1.5"
                >
                  <Boxes className="w-3.5 h-3.5 text-slate-400" />
                  <span>GitHub Repository</span>
                  <ExternalLink className="w-3 h-3 text-slate-500" />
                </a>
              </li>
            </ul>
          </div>
        </div>

        {/* Bottom Bar */}
        <div className="pt-8 border-t border-slate-800/80 flex flex-col sm:flex-row items-center justify-between text-xs text-slate-400 gap-4">
          <p>© 2026 SP Language Initiative by Shanto Paul. Released under Apache-2.0 / MIT licenses.</p>
          <div className="flex items-center gap-1 text-slate-400">
            <span>Crafted with</span>
            <Heart className="w-3.5 h-3.5 text-red-500 inline fill-red-500" />
            <span>for high-performance software engineering</span>
          </div>
        </div>
      </div>
    </footer>
  );
};
