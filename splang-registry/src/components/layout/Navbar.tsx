"use client";

import React from "react";
import Link from "next/link";
import { Boxes, BookOpen, Cpu, Github, ExternalLink, Upload, Shield } from "lucide-react";

export const Navbar: React.FC = () => {
  return (
    <header className="sticky top-0 z-40 w-full border-b border-slate-800 bg-slate-950/80 backdrop-blur-xl">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex items-center justify-between h-16">
          {/* Logo & Portal Name */}
          <div className="flex items-center gap-3">
            <Link href="/" className="flex items-center gap-2.5">
              <div className="w-8 h-8 rounded-lg bg-gradient-to-tr from-cyan-500 via-blue-500 to-indigo-500 p-0.5 shadow-md shadow-cyan-500/20">
                <div className="w-full h-full bg-slate-950 rounded-[6px] flex items-center justify-center font-bold text-transparent bg-clip-text bg-gradient-to-tr from-cyan-400 to-blue-400 text-sm">
                  SP
                </div>
              </div>
              <div className="flex flex-col">
                <span className="font-bold text-white text-sm tracking-tight">
                  Package Registry
                </span>
                <span className="text-[10px] font-mono text-cyan-400 -mt-1">
                  registry.splang.shantopaul.com
                </span>
              </div>
            </Link>
          </div>

          {/* Navigation Links */}
          <nav className="hidden md:flex items-center gap-6 text-xs font-medium text-slate-300">
            <Link href="/packages" className="hover:text-cyan-400 transition-colors">
              Explore Packages
            </Link>
            <Link href="/publish" className="hover:text-cyan-400 transition-colors flex items-center gap-1">
              <Upload className="w-3.5 h-3.5 text-slate-400" />
              <span>Publish Guide</span>
            </Link>
            <Link href="/security" className="hover:text-cyan-400 transition-colors flex items-center gap-1">
              <Shield className="w-3.5 h-3.5 text-slate-400" />
              <span>Security & Hashes</span>
            </Link>
            <a
              href="https://docs.splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-cyan-400 transition-colors flex items-center gap-1"
            >
              <BookOpen className="w-3.5 h-3.5 text-slate-400" />
              <span>Documentation</span>
              <ExternalLink className="w-3 h-3 text-slate-500" />
            </a>
            <a
              href="https://splang.shantopaul.com"
              target="_blank"
              rel="noopener noreferrer"
              className="hover:text-cyan-400 transition-colors flex items-center gap-1"
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
              className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg border border-slate-800 bg-slate-900 text-slate-300 hover:text-white transition-all text-xs font-medium"
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
