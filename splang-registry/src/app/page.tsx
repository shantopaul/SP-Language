"use client";

import React, { useState } from "react";
import Link from "next/link";
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

  return (
    <div className="space-y-16 pb-12">
      {/* Hero Section */}
      <section className="relative pt-16 pb-12 overflow-hidden border-b border-slate-800/80 bg-gradient-to-b from-slate-900/50 via-slate-950 to-slate-950">
        <div className="absolute inset-0 bg-[radial-gradient(ellipse_at_top,_var(--tw-gradient-stops))] from-cyan-900/20 via-transparent to-transparent pointer-events-none" />

        <div className="max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 text-center relative z-10">
          <div className="inline-flex items-center gap-2 px-3 py-1.5 rounded-full border border-cyan-500/30 bg-cyan-950/40 text-cyan-400 text-xs font-mono mb-6">
            <Sparkles className="w-3.5 h-3.5 text-cyan-400" />
            <span>Official Package Registry for SP 3.0</span>
          </div>

          <h1 className="text-4xl sm:text-5xl lg:text-6xl font-black text-white tracking-tight leading-tight">
            Discover & Distribute <br />
            <span className="text-transparent bg-clip-text bg-gradient-to-r from-cyan-400 via-blue-400 to-indigo-400">
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
            <div className="relative flex items-center">
              <Search className="w-5 h-5 text-slate-400 absolute left-4 pointer-events-none group-focus-within:text-cyan-400 transition-colors" />
              <input
                type="text"
                value={searchQuery}
                onChange={(e) => setSearchQuery(e.target.value)}
                placeholder="Search packages, keywords, authors... (e.g. json, http, crypto)"
                className="w-full pl-12 pr-28 py-3.5 rounded-xl border border-slate-800 bg-slate-900/90 text-slate-100 placeholder-slate-500 text-sm focus:outline-none focus:ring-2 focus:ring-cyan-500/50 focus:border-cyan-500/50 shadow-xl transition-all"
              />
              <button
                type="submit"
                className="absolute right-2 px-4 py-2 bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-semibold text-xs rounded-lg transition-all shadow-md shadow-cyan-500/20 flex items-center gap-1.5"
              >
                <span>Search</span>
                <ArrowRight className="w-3.5 h-3.5" />
              </button>
            </div>
          </form>

          {/* Quick ecosystem metrics */}
          <div className="mt-12 grid grid-cols-2 sm:grid-cols-4 gap-4 max-w-3xl mx-auto text-left">
            <div className="p-4 rounded-xl border border-slate-800/80 bg-slate-900/50 backdrop-blur-sm">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <Boxes className="w-3.5 h-3.5 text-cyan-400" />
                <span>Verified Packages</span>
              </div>
              <div className="text-xl font-bold text-white font-mono">{PACKAGES.length}</div>
            </div>

            <div className="p-4 rounded-xl border border-slate-800/80 bg-slate-900/50 backdrop-blur-sm">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <Download className="w-3.5 h-3.5 text-blue-400" />
                <span>Total Downloads</span>
              </div>
              <div className="text-xl font-bold text-white font-mono">
                {totalDownloads.toLocaleString()}
              </div>
            </div>

            <div className="p-4 rounded-xl border border-slate-800/80 bg-slate-900/50 backdrop-blur-sm">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
                <span>Integrity Guard</span>
              </div>
              <div className="text-xl font-bold text-emerald-400 font-mono">100% SHA-256</div>
            </div>

            <div className="p-4 rounded-xl border border-slate-800/80 bg-slate-900/50 backdrop-blur-sm">
              <div className="flex items-center gap-2 text-slate-400 text-xs font-mono mb-1">
                <Zap className="w-3.5 h-3.5 text-amber-400" />
                <span>Compatibility</span>
              </div>
              <div className="text-xl font-bold text-white font-mono">SP 1.0 Ready</div>
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
              <Layers className="w-4 h-4 text-cyan-400" />
              <span>Browse by Category</span>
            </h2>
            <Link
              href="/packages"
              className="text-xs font-medium text-cyan-400 hover:text-cyan-300 flex items-center gap-1"
            >
              <span>View all categories</span>
              <ChevronRight className="w-3 h-3" />
            </Link>
          </div>
          <CategoryFilter isLink={true} />
        </section>

        {/* Popular Packages Grid */}
        <section className="space-y-6">
          <div className="flex items-center justify-between pb-2 border-b border-slate-800/80">
            <div>
              <h2 className="text-xl font-bold text-white tracking-tight">
                Most Popular Packages
              </h2>
              <p className="text-xs text-slate-400 mt-0.5">
                Widely adopted community and official libraries with the highest download volume
              </p>
            </div>
            <Link
              href="/packages"
              className="text-xs font-medium text-cyan-400 hover:text-cyan-300 flex items-center gap-1"
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
          <div className="flex items-center justify-between pb-2 border-b border-slate-800/80">
            <div>
              <h2 className="text-xl font-bold text-white tracking-tight">
                Recently Published & Updated
              </h2>
              <p className="text-xs text-slate-400 mt-0.5">
                The latest production versions released to the official registry
              </p>
            </div>
            <Link
              href="/packages"
              className="text-xs font-medium text-cyan-400 hover:text-cyan-300 flex items-center gap-1"
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
        <section className="rounded-2xl border border-slate-800 bg-gradient-to-br from-slate-900/90 via-slate-900/50 to-slate-950 p-6 sm:p-8">
          <div className="flex flex-col lg:flex-row items-start lg:items-center justify-between gap-6">
            <div className="max-w-xl">
              <div className="inline-flex items-center gap-1.5 px-2.5 py-1 rounded-md bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 text-xs font-mono mb-3">
                <Terminal className="w-3.5 h-3.5" />
                <span>SPM Package Manager</span>
              </div>
              <h3 className="text-2xl font-bold text-white tracking-tight">
                Manage Dependencies Seamlessly
              </h3>
              <p className="mt-2 text-sm text-slate-300 leading-relaxed">
                The SP Language toolchain includes <code className="text-cyan-400 font-mono">spm</code>, a zero-overhead package manager that enforces semantic versioning, reproducible builds, and cryptographic verification on every package.
              </p>
              <div className="mt-4 flex items-center gap-4">
                <Link
                  href="/publish"
                  className="px-4 py-2 rounded-lg bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-semibold text-xs transition-colors shadow-md shadow-cyan-500/20"
                >
                  Publish a Package
                </Link>
                <Link
                  href="/security"
                  className="px-4 py-2 rounded-lg border border-slate-700 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium transition-colors"
                >
                  Security Architecture
                </Link>
              </div>
            </div>

            {/* Terminal snippet box */}
            <div className="w-full lg:max-w-md rounded-xl border border-slate-800 bg-slate-950 p-4 font-mono text-xs shadow-2xl">
              <div className="flex items-center justify-between pb-3 mb-3 border-b border-slate-800/80 text-slate-500">
                <div className="flex items-center gap-1.5">
                  <div className="w-2.5 h-2.5 rounded-full bg-red-500/60" />
                  <div className="w-2.5 h-2.5 rounded-full bg-amber-500/60" />
                  <div className="w-2.5 h-2.5 rounded-full bg-emerald-500/60" />
                </div>
                <span className="text-[11px]">spm terminal</span>
              </div>
              <div className="space-y-2 text-slate-300">
                <div className="flex items-center gap-2">
                  <span className="text-cyan-400 select-none">$</span>
                  <span>spm init my_service</span>
                </div>
                <div className="flex items-center gap-2">
                  <span className="text-cyan-400 select-none">$</span>
                  <span>spm add json http</span>
                </div>
                <div className="text-emerald-400/90 pl-4 text-[11px]">
                  ✓ Resolved dependencies in 14ms
                </div>
                <div className="text-emerald-400/90 pl-4 text-[11px]">
                  ✓ Checksums verified (SHA-256)
                </div>
                <div className="flex items-center gap-2">
                  <span className="text-cyan-400 select-none">$</span>
                  <span>spc build --release</span>
                </div>
              </div>
            </div>
          </div>
        </section>
      </div>
    </div>
  );
}
