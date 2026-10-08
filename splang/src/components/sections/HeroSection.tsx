"use client";

import React from "react";
import Link from "next/link";
import { ArrowRight, BookOpen, Flame, Github, Sparkles, CheckCircle2 } from "lucide-react";
import { Button } from "@/components/ui/Button";
import { Badge } from "@/components/ui/Badge";
import { QuickInstall } from "./QuickInstall";

export const HeroSection: React.FC = () => {
  return (
    <section className="relative pt-12 pb-20 md:pt-20 md:pb-32 overflow-hidden">
      {/* Background Glows */}
      <div className="absolute top-1/4 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[600px] h-[350px] bg-gradient-to-tr from-emerald-500/15 via-cyan-500/20 to-blue-600/15 blur-[120px] rounded-full pointer-events-none -z-10" />

      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 text-center">
        {/* Release Pill Banner */}
        <div className="inline-flex items-center gap-2 mb-6">
          <Link href="/blog/v1.0.0-release">
            <div className="inline-flex items-center gap-2 px-3.5 py-1.5 rounded-full border border-cyan-500/30 bg-cyan-950/40 text-xs sm:text-sm font-medium text-cyan-300 hover:border-cyan-400/50 hover:bg-cyan-950/60 transition-all shadow-[0_0_15px_rgba(6,182,212,0.15)] group cursor-pointer">
              <span className="flex h-2 w-2 rounded-full bg-cyan-400 animate-ping" />
              <span className="font-semibold text-white">SP 2.0.0 General Availability is officially released!</span>
              <span className="text-slate-400 group-hover:text-cyan-300 flex items-center transition-colors">
                Read Announcement <ArrowRight className="w-3.5 h-3.5 ml-1" />
              </span>
            </div>
          </Link>
        </div>

        {/* Main Headline */}
        <h1 className="text-4xl sm:text-6xl lg:text-7xl font-extrabold text-white tracking-tight leading-[1.1] max-w-5xl mx-auto">
          The Modern, High-Performance{" "}
          <span className="text-transparent bg-clip-text bg-gradient-to-r from-emerald-400 via-cyan-400 to-blue-500">
            Systems & WebAssembly
          </span>{" "}
          Language
        </h1>

        {/* Sub-Headline */}
        <p className="mt-6 text-lg sm:text-xl text-slate-300 max-w-3xl mx-auto leading-relaxed">
          SP 2.0 merges bare-metal LLVM & WebAssembly performance with compile-time function execution (comptime), language editions (2026/2028), structured async/await coroutines, and leak-free ARC memory management.
        </p>

        {/* Call to Actions */}
        <div className="mt-8 flex flex-wrap items-center justify-center gap-4">
          <a
            href="https://docs.splang.shantopaul.com/getting-started"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button size="lg" variant="primary">
              <Sparkles className="w-4 h-4" />
              <span>Get Started Now</span>
              <ArrowRight className="w-4 h-4 ml-1" />
            </Button>
          </a>

          <a
            href="https://docs.splang.shantopaul.com"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button size="lg" variant="secondary">
              <BookOpen className="w-4 h-4 text-cyan-400" />
              <span>Explore Docs</span>
            </Button>
          </a>

          <Link href="/benchmarks">
            <Button size="lg" variant="outline">
              <Flame className="w-4 h-4 text-amber-400" />
              <span>See Benchmarks</span>
            </Button>
          </Link>
        </div>

        {/* Terminal Quick Install Box */}
        <div className="mt-12 sm:mt-16">
          <QuickInstall />
        </div>

        {/* Feature metrics bar */}
        <div className="mt-14 pt-8 border-t border-slate-800/80 max-w-4xl mx-auto grid grid-cols-2 sm:grid-cols-4 gap-4 text-left">
          <div className="flex items-center gap-3 p-2">
            <CheckCircle2 className="w-5 h-5 text-emerald-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">LLVM Native</div>
              <div className="text-xs text-slate-400">Zero VM overhead (-O3)</div>
            </div>
          </div>
          <div className="flex items-center gap-3 p-2">
            <CheckCircle2 className="w-5 h-5 text-cyan-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">346 Tests (100%)</div>
              <div className="text-xs text-slate-400">Continuous matrix CI</div>
            </div>
          </div>
          <div className="flex items-center gap-3 p-2">
            <CheckCircle2 className="w-5 h-5 text-blue-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">All-in-One CLI</div>
              <div className="text-xs text-slate-400">spc, spm, spls, spd</div>
            </div>
          </div>
          <div className="flex items-center gap-3 p-2">
            <CheckCircle2 className="w-5 h-5 text-purple-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">Crypto Safe</div>
              <div className="text-xs text-slate-400">SHA-256 lockfiles</div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};
