"use client";

import React from "react";
import Link from "next/link";
import Image from "next/image";
import { ArrowRight, BookOpen, Flame, Sparkles, CheckCircle2, Download, Zap } from "lucide-react";
import { Button } from "@/components/ui/Button";
import { QuickInstall } from "./QuickInstall";

export const HeroSection: React.FC = () => {
  return (
    <section className="relative pt-10 pb-20 md:pt-16 md:pb-32 overflow-hidden bg-ambient-radial">
      {/* Background Radial Glow Spotlights */}
      <div className="absolute top-1/4 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[700px] h-[400px] bg-gradient-to-tr from-blue-600/20 via-cyan-500/20 to-transparent blur-[140px] rounded-full pointer-events-none -z-10" />

      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 text-center">
        {/* Floating 3D Geometric Brand Emblem Centerpiece */}
        <div className="relative mx-auto mb-6 w-24 h-24 sm:w-28 sm:h-28 group">
          <div className="absolute -inset-2 bg-gradient-to-tr from-[#0057FF]/40 via-[#00D1FF]/30 to-transparent rounded-3xl blur-2xl group-hover:blur-3xl transition-all" />
          <div className="relative w-full h-full rounded-2xl bg-[#081735]/90 border border-[#0A1F44] p-3 shadow-2xl flex items-center justify-center animate-float">
            <Image
              src="/brand/brand-emblem-navy.png"
              alt="SP Language 3D Emblem"
              width={88}
              height={88}
              className="object-contain drop-shadow-[0_10px_20px_rgba(0,87,255,0.4)]"
              priority
            />
          </div>
        </div>

        {/* Release Pill Banner */}
        <div className="inline-flex items-center gap-2 mb-6">
          <Link href="/downloads">
            <div className="inline-flex items-center gap-2 px-4 py-1.5 rounded-full border border-cyan-500/40 bg-[#081735]/80 text-xs sm:text-sm font-medium text-cyan-300 hover:border-cyan-400 hover:bg-[#0A1F44] transition-all shadow-[0_0_20px_rgba(0,209,255,0.15)] group cursor-pointer backdrop-blur-md">
              <span className="flex h-2 w-2 rounded-full bg-[#00D1FF] animate-ping" />
              <span className="font-semibold text-white">SP 3.1.0 Universal Experience GA is officially released!</span>
              <span className="text-slate-400 group-hover:text-cyan-300 flex items-center transition-colors">
                Get v3.1.0 <ArrowRight className="w-3.5 h-3.5 ml-1" />
              </span>
            </div>
          </Link>
        </div>

        {/* Main Headline */}
        <h1 className="text-4xl sm:text-6xl lg:text-7xl font-extrabold text-white tracking-tight leading-[1.1] max-w-5xl mx-auto">
          The Modern, High-Performance{" "}
          <span className="text-transparent bg-clip-text bg-gradient-to-r from-white via-cyan-100 to-[#00D1FF]">
            Systems &amp; Backend Platform
          </span>{" "}
          Language
        </h1>

        {/* Sub-Headline */}
        <p className="mt-6 text-base sm:text-xl text-slate-300 max-w-3xl mx-auto leading-relaxed">
          SP merges bare-metal LLVM speed with an official enterprise Web Framework (<code className="text-cyan-300 bg-[#081735] px-1.5 py-0.5 rounded border border-[#0A1F44] text-sm">std.web</code>), sub-microsecond Radix routing, connection pooling (SQLite &amp; PostgreSQL), deterministic ARC (zero GC pauses), and 185,000 req/s throughput.
        </p>

        {/* Call to Actions */}
        <div className="mt-8 flex flex-wrap items-center justify-center gap-4">
          <Link href="/downloads">
            <Button size="lg" variant="primary">
              <Download className="w-4 h-4 mr-1" />
              <span>Download SP v3.1.0</span>
              <ArrowRight className="w-4 h-4 ml-1" />
            </Button>
          </Link>

          <a
            href="https://docs.splang.shantopaul.com"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button size="lg" variant="secondary">
              <BookOpen className="w-4 h-4 text-cyan-400 mr-1" />
              <span>Explore Docs</span>
            </Button>
          </a>

          <Link href="/benchmarks">
            <Button size="lg" variant="outline">
              <Flame className="w-4 h-4 text-amber-400 mr-1" />
              <span>See Benchmarks</span>
            </Button>
          </Link>
        </div>

        {/* Terminal Quick Install Box */}
        <div className="mt-12 sm:mt-16">
          <QuickInstall />
        </div>

        {/* Feature metrics bar */}
        <div className="mt-14 pt-8 border-t border-[#0A1F44] max-w-4xl mx-auto grid grid-cols-2 sm:grid-cols-4 gap-4 text-left">
          <div className="flex items-center gap-3 p-2 bg-[#081735]/40 rounded-xl border border-[#0A1F44]/50">
            <CheckCircle2 className="w-5 h-5 text-cyan-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">LLVM Native (-O3)</div>
              <div className="text-xs text-slate-400">Zero VM overhead</div>
            </div>
          </div>
          <div className="flex items-center gap-3 p-2 bg-[#081735]/40 rounded-xl border border-[#0A1F44]/50">
            <CheckCircle2 className="w-5 h-5 text-cyan-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">474 Tests (100%)</div>
              <div className="text-xs text-slate-400">Matrix verification CI</div>
            </div>
          </div>
          <div className="flex items-center gap-3 p-2 bg-[#081735]/40 rounded-xl border border-[#0A1F44]/50">
            <CheckCircle2 className="w-5 h-5 text-blue-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">23 Stdlib Modules</div>
              <div className="text-xs text-slate-400">Enterprise battery ready</div>
            </div>
          </div>
          <div className="flex items-center gap-3 p-2 bg-[#081735]/40 rounded-xl border border-[#0A1F44]/50">
            <CheckCircle2 className="w-5 h-5 text-purple-400 shrink-0" />
            <div>
              <div className="text-sm font-semibold text-white">Deterministic ARC</div>
              <div className="text-xs text-slate-400">Zero GC stop pauses</div>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};
