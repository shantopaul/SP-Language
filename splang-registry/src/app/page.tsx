"use client";

import React, { useState } from "react";
import Link from "next/link";
import Image from "next/image";
import { useRouter } from "next/navigation";
import { PACKAGES } from "@/data/packages-db";
import { CATEGORIES } from "@/data/categories";
import { PackageCard } from "@/components/registry/PackageCard";
import { CategoryFilter } from "@/components/registry/CategoryFilter";
import { CopyButton } from "@/components/ui/CopyButton";
import {
  Search,
  ArrowRight,
  ShieldCheck,
  Download,
  Boxes,
  Terminal,
  Zap,
  Sparkles,
  Layers,
  ChevronRight,
} from "lucide-react";

export default function RegistryHomePage() {
  const router = useRouter();
  const [searchQuery, setSearchQuery] = useState("");

  const handleSearchSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    if (searchQuery.trim()) {
      router.push(`/packages?q=${encodeURIComponent(searchQuery.trim())}`);
    } else {
      router.push("/packages");
    }
  };

  // Sort popular packages by downloads
  const popularPackages = [...PACKAGES].sort(
    (a, b) => b.totalDownloads - a.totalDownloads
  ).slice(0, 4);

  // Recently updated packages
  const recentPackages = [...PACKAGES].sort(
    (a, b) => new Date(b.publishedAt).getTime() - new Date(a.publishedAt).getTime()
  ).slice(0, 4);

  const totalDownloads = PACKAGES.reduce((sum, p) => sum + p.totalDownloads, 0);

  const quickKeywords = ["web", "db", "crypto", "json", "redis", "ws"];

  return (
    <div className="space-y-16 pb-16">
      {/* Hero Section */}
      <section className="relative pt-16 pb-16 overflow-hidden border-b border-[#0A1F44] bg-gradient-to-b from-[#081735]/40 via-[#020D21] to-[#020D21]">
        <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_top,_var(--tw-gradient-stops))] from-[#0057FF]/20 via-transparent to-transparent pointer-events-none" />

        <div className="max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 text-center relative z-10">
          {/* Release Radar Pill */}
          <div className="inline-flex items-center gap-2 px-3.5 py-1.5 rounded-full border border-[#00D1FF]/40 bg-[#0057FF]/15 text-[#00D1FF] text-xs font-mono mb-6 shadow-[0_0_15px_rgba(0,209,255,0.2)]">
            <span className="w-2 h-2 rounded-full bg-[#00D1FF] animate-pulse" />
            <span>Official Package Registry for SP v3.1.0 GA</span>
          </div>

          <h1 className="text-4xl sm:text-5xl lg:text-6xl font-black text-white tracking-tight leading-tight">
            Discover &amp; Distribute <br />
            <span className="text-transparent bg-clip-text bg-gradient-to-r from-[#00D1FF] via-[#0057FF] to-[#2563EB]">
              SP Packages
            </span>
          </h1>

          <p className="mt-4 text-base sm:text-lg text-slate-300 max-w-2xl mx-auto leading-relaxed">
            The central distribution network for high-performance libraries, tools, and frameworks built with the SP Programming Language.
          </p>

          {/* Large Search Bar */}
          <form
            onSubmit={handleSearchSubmit}
            className="mt-8 max-w-2xl mx-auto relative group"
          >
            <div className="relative flex items-center shadow-2xl">
              <Search className="w-5 h-5 text-slate-400 absolute left-4 pointer-events-none group-focus-within:text-[#00D1FF] transition-colors" />
              <input
                type="text"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                placeholder="Search packages, keywords, authors... (e.g. web, db, crypto)"
                className="w-full pl-12 pr-28 py-4 rounded-2xl border border-[#0A1F44] bg-[#081735]/90 text-white placeholder-slate-400 text-sm focus:outline-none focus:ring-2 focus:ring-[#00D1FF]/50 focus:border-[#00D1FF]/60 shadow-xl transition-all"
              />
              <button
                type="submit"
                className="absolute right-2.5 px-5 py-2.5 bg-gradient-to-r from-[#0057FF] to-[#00D1FF] hover:opacity-90 text-white font-bold text-xs rounded-xl transition-all shadow-md shadow-[#0057FF]/30 flex items-center gap-1.5"
              >
                <span>Search</span>
                <ArrowRight className="w-3.5 h-3.5" />
              </button>
            </div>
            {/* Quick keyword chips */}
            <div className="flex items-center justify-center gap-2 mt-3 flex-wrap text-xs text-slate-400">
              <span className="font-mono">Quick tags:</span>
              {quickKeywords.map((tag) => (
                <button
                  key={tag}
                  type="button"
                  onClick={() => router.push(`/packages?q=${tag}`)}
                  className="px-2.5 py-0.5 rounded-full border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#0057FF]/20 hover:border-[#00D1FF]/40 text-slate-300 hover:text-[#00D1FF] transition-all font-mono text-[11px]"
                >
                  #{tag}
                </button>
              ))}
            </div>
          </form>

          {/* Quick ecosystem metrics */}
          <div className="mt-12 grid grid-cols-2 sm:grid-cols-4 gap-4 max-w-4xl mx-auto text-left">
            <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 backdrop-blur-md shadow-lg hover:border-[#0057FF]/40 transition-colors">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <Boxes className="w-4 h-4 text-[#00D1FF]" />
                <span>Verified Packages</span>
              </div>
              <div className="text-2xl font-black text-white font-mono">{PACKAGES.length}</div>
            </div>

            <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 backdrop-blur-md shadow-lg hover:border-[#0057FF]/40 transition-colors">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <Download className="w-4 h-4 text-[#60A5FA]" />
                <span>Total Downloads</span>
              </div>
              <div className="text-2xl font-black text-white font-mono">
                {totalDownloads.toLocaleString()}
              </div>
            </div>

            <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 backdrop-blur-md shadow-lg hover:border-[#0057FF]/40 transition-colors">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <ShieldCheck className="w-4 h-4 text-[#00D1FF]" />
                <span>Integrity Guard</span>
              </div>
              <div className="text-2xl font-black text-[#00D1FF] font-mono">100% SHA-256</div>
            </div>

            <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 backdrop-blur-md shadow-lg hover:border-[#0057FF]/40 transition-colors">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <Zap className="w-4 h-4 text-[#00D1FF]" />
                <span>Standard Library</span>
              </div>
              <div className="text-2xl font-black text-white font-mono">23 Modules</div>
            </div>
          </div>
        </div>
      </section>

      {/* Main Content Area */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 space-y-16">
        {/* Categories Section */}
        <section className="space-y-4">
          <div className="flex items-center justify-between">
            <h2 className="text-lg font-bold text-white tracking-tight flex items-center gap-2">
              <Layers className="w-4 h-4 text-[#00D1FF]" />
              <span>Browse by Category</span>
            </h2>
            <Link
              href="/packages"
              className="text-xs font-semibold text-[#00D1FF] hover:text-white flex items-center gap-1 transition-colors"
            >
              <span>View all categories</span>
              <ChevronRight className="w-3.5 h-3.5" />
            </Link>
          </div>
          <CategoryFilter isLink={true} />
        </section>

        {/* Popular Packages Grid */}
        <section className="space-y-6">
          <div className="flex items-center justify-between pb-3 border-b border-[#0A1F44]">
            <div>
              <h2 className="text-xl font-bold text-white tracking-tight">
                Most Popular Packages
              </h2>
              <p className="text-xs text-slate-400 mt-0.5">
                Widely adopted ecosystem and official standard packages with high download volume
              </p>
            </div>
            <Link
              href="/packages"
              className="text-xs font-semibold text-[#00D1FF] hover:text-white flex items-center gap-1 transition-colors"
            >
              <span>Explore all</span>
              <ArrowRight className="w-3.5 h-3.5" />
            </Link>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {popularPackages.map((pkg) => (
              <PackageCard key={pkg.name} pkg={pkg} />
            ))}
          </div>
        </section>

        {/* Recently Updated Packages Grid */}
        <section className="space-y-6">
          <div className="flex items-center justify-between pb-3 border-b border-[#0A1F44]">
            <div>
              <h2 className="text-xl font-bold text-white tracking-tight">
                Recently Published &amp; Updated
              </h2>
              <p className="text-xs text-slate-400 mt-0.5">
                The latest production versions released to the official registry
              </p>
            </div>
            <Link
              href="/packages"
              className="text-xs font-semibold text-[#00D1FF] hover:text-white flex items-center gap-1 transition-colors"
            >
              <span>Explore all</span>
              <ArrowRight className="w-3.5 h-3.5" />
            </Link>
          </div>

          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {recentPackages.map((pkg) => (
              <PackageCard key={pkg.name} pkg={pkg} />
            ))}
          </div>
        </section>

        {/* Quick Start with SPM CLI */}
        <section className="rounded-3xl border border-[#0A1F44] bg-gradient-to-br from-[#081735]/90 via-[#081735]/50 to-[#020D21] p-6 sm:p-10 shadow-2xl">
          <div className="flex flex-col lg:flex-row items-start lg:items-center justify-between gap-8">
            <div className="max-w-xl">
              <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-[#0057FF]/20 border border-[#00D1FF]/40 text-[#00D1FF] text-xs font-mono font-semibold mb-4 shadow-[0_0_12px_rgba(0,209,255,0.2)]">
                <Terminal className="w-3.5 h-3.5" />
                <span>SPM Package Manager</span>
              </div>
              <h3 className="text-2xl sm:text-3xl font-black text-white tracking-tight">
                Manage Dependencies Seamlessly
              </h3>
              <p className="mt-3 text-sm text-slate-300 leading-relaxed">
                The SP Language toolchain includes <code className="text-[#00D1FF] font-mono font-semibold">spm</code>, a zero-overhead package manager that enforces semantic versioning, reproducible builds, and cryptographic verification on every package.
              </p>
              <div className="mt-6 flex items-center gap-3">
                <Link
                  href="/publish"
                  className="px-5 py-2.5 rounded-xl bg-gradient-to-r from-[#0057FF] to-[#00D1FF] hover:opacity-90 text-white font-bold text-xs transition-all shadow-md shadow-[#0057FF]/30"
                >
                  Publish a Package
                </Link>
                <Link
                  href="/security"
                  className="px-5 py-2.5 rounded-xl border border-[#0A1F44] bg-[#020D21] hover:bg-[#0057FF]/15 hover:border-[#00D1FF]/40 text-slate-200 text-xs font-medium transition-all"
                >
                  Security Architecture
                </Link>
              </div>
            </div>

            {/* Terminal snippet box */}
            <div className="w-full lg:max-w-md rounded-2xl border border-[#0A1F44] bg-[#020D21] p-5 font-mono text-xs shadow-2xl">
              <div className="flex items-center justify-between pb-3 mb-3 border-b border-[#0A1F44] text-slate-500">
                <div className="flex items-center gap-1.5">
                  <div className="w-2.5 h-2.5 rounded-full bg-[#ef4444]" />
                  <div className="w-2.5 h-2.5 rounded-full bg-[#f59e0b]" />
                  <div className="w-2.5 h-2.5 rounded-full bg-[#10b981]" />
                </div>
                <span className="text-[11px] text-[#00D1FF] font-semibold">spm terminal</span>
              </div>
              <div className="space-y-2 text-slate-200">
                <div className="flex items-center gap-2">
                  <span className="text-[#00D1FF] select-none font-bold">$</span>
                  <span className="text-white font-semibold">spm init my_service</span>
                </div>
                <div className="flex items-center gap-2">
                  <span className="text-[#00D1FF] select-none font-bold">$</span>
                  <span className="text-white font-semibold">spm add web db</span>
                </div>
                <div className="text-[#00D1FF] pl-4 text-[11px]">
                  ✓ Resolved dependencies in 14ms
                </div>
                <div className="text-[#00D1FF] pl-4 text-[11px]">
                  ✓ Checksums verified (SHA-256)
                </div>
                <div className="flex items-center gap-2">
                  <span className="text-[#00D1FF] select-none font-bold">$</span>
                  <span className="text-white font-semibold">spc build --release</span>
                </div>
              </div>
            </div>
          </div>
        </section>
      </div>
    </div>
  );
}
