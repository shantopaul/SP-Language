"use client";

import React, { useState, useMemo, Suspense } from "react";
import { useSearchParams } from "next/navigation";
import { PACKAGES } from "@/data/packages-db";
import { CATEGORIES } from "@/data/categories";
import { PackageCard } from "@/components/registry/PackageCard";
import { CategoryFilter } from "@/components/registry/CategoryFilter";
import { Search, ArrowUpDown, X, PackageX } from "lucide-react";

function PackagesExplorerContent() {
  const searchParams = useSearchParams();
  const initialQuery = searchParams.get("q") || "";
  const initialCategory = searchParams.get("category") || undefined;

  const [search, setSearch] = useState(initialQuery);
  const [selectedCategory, setSelectedCategory] = useState<string | undefined>(
    initialCategory
  );
  const [sortBy, setSortBy] = useState<"downloads" | "newest" | "name">("downloads");

  const filteredPackages = useMemo(() => {
    let list = [...PACKAGES];

    if (selectedCategory) {
      list = list.filter((p) => p.category === selectedCategory);
    }

    if (search.trim()) {
      const q = search.toLowerCase().trim();
      list = list.filter(
        (p) =>
          p.name.toLowerCase().includes(q) ||
          p.description.toLowerCase().includes(q) ||
          p.keywords.some((k) => k.toLowerCase().includes(q)) ||
          p.author.toLowerCase().includes(q)
      );
    }

    // Sort
    if (sortBy === "downloads") {
      list.sort((a, b) => b.totalDownloads - a.totalDownloads);
    } else if (sortBy === "newest") {
      list.sort(
        (a, b) =>
          new Date(b.publishedAt).getTime() - new Date(a.publishedAt).getTime()
      );
    } else if (sortBy === "name") {
      list.sort((a, b) => a.name.localeCompare(b.name));
    }

    return list;
  }, [search, selectedCategory, sortBy]);

  return (
    <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10 space-y-8">
      {/* Header */}
      <div>
        <h1 className="text-3xl font-extrabold text-white tracking-tight">
          Explore SP Packages
        </h1>
        <p className="mt-1 text-sm text-slate-400">
          Search and browse {PACKAGES.length} packages available in the SP registry.
        </p>
      </div>

      {/* Controls Bar: Search & Sort */}
      <div className="flex flex-col sm:flex-row items-stretch sm:items-center justify-between gap-4">
        {/* Search Input */}
        <div className="relative flex-1 max-w-lg">
          <Search className="w-4 h-4 text-slate-400 absolute left-3.5 top-1/2 -translate-y-1/2 pointer-events-none" />
          <input
            type="text"
            value={search}
            onChange={(e) => setSearch(e.target.value)}
            placeholder="Filter by name, keyword, or description..."
            className="w-full pl-10 pr-9 py-2.5 rounded-xl border border-slate-800 bg-slate-900 text-slate-100 placeholder-slate-500 text-xs sm:text-sm focus:outline-none focus:ring-1 focus:ring-cyan-500 focus:border-cyan-500 transition-all"
          />
          {search && (
            <button
              onClick={() => setSearch("")}
              className="absolute right-3 top-1/2 -translate-y-1/2 text-slate-400 hover:text-white"
            >
              <X className="w-4 h-4" />
            </button>
          )}
        </div>

        {/* Sort Selector */}
        <div className="flex items-center gap-2 self-end sm:self-auto">
          <ArrowUpDown className="w-4 h-4 text-slate-400" />
          <label htmlFor="sort-select" className="text-xs text-slate-400 font-medium">
            Sort by:
          </label>
          <select
            id="sort-select"
            value={sortBy}
            onChange={(e) =>
              setSortBy(e.target.value as "downloads" | "newest" | "name")
            }
            className="px-3 py-2 rounded-lg border border-slate-800 bg-slate-900 text-slate-200 text-xs font-medium focus:outline-none focus:ring-1 focus:ring-cyan-500"
          >
            <option value="downloads">Most Downloaded</option>
            <option value="newest">Recently Published</option>
            <option value="name">Name (A-Z)</option>
          </select>
        </div>
      </div>

      {/* Categories Filter Pills */}
      <div>
        <CategoryFilter
          selectedCategory={selectedCategory}
          onSelectCategory={(cat) => setSelectedCategory(cat)}
        />
      </div>

      {/* Results Header */}
      <div className="flex items-center justify-between pb-2 border-b border-slate-800/80 text-xs font-mono text-slate-400">
        <div>
          Showing <span className="text-cyan-400 font-semibold">{filteredPackages.length}</span>{" "}
          packages
          {selectedCategory && (
            <span className="ml-1 text-slate-400">
              in category <span className="text-white capitalize">"{selectedCategory}"</span>
            </span>
          )}
        </div>
        {(search || selectedCategory) && (
          <button
            onClick={() => {
              setSearch("");
              setSelectedCategory(undefined);
            }}
            className="text-cyan-400 hover:underline"
          >
            Clear filters
          </button>
        )}
      </div>

      {/* Package Grid */}
      {filteredPackages.length > 0 ? (
        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          {filteredPackages.map((pkg) => (
            <PackageCard key={pkg.name} pkg={pkg} />
          ))}
        </div>
      ) : (
        <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-12 text-center">
          <div className="w-12 h-12 rounded-full bg-slate-800 border border-slate-700 text-slate-400 flex items-center justify-center mx-auto mb-3">
            <PackageX className="w-6 h-6" />
          </div>
          <h3 className="text-base font-semibold text-white font-mono mb-1">
            No packages matched your query
          </h3>
          <p className="text-xs text-slate-400 max-w-sm mx-auto mb-4">
            Try searching for a different keyword, category, or clear your search filters.
          </p>
          <button
            onClick={() => {
              setSearch("");
              setSelectedCategory(undefined);
            }}
            className="px-4 py-2 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium transition-colors"
          >
            Reset Filters
          </button>
        </div>
      )}
    </div>
  );
}

export default function PackagesPage() {
  return (
    <Suspense fallback={<div className="max-w-7xl mx-auto px-4 py-12 text-slate-400">Loading package catalog...</div>}>
      <PackagesExplorerContent />
    </Suspense>
  );
}
