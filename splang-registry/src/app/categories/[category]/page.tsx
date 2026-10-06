import React from "react";
import { notFound } from "next/navigation";
import { Metadata } from "next";
import Link from "next/link";
import { PACKAGES } from "@/data/packages-db";
import { CATEGORIES } from "@/data/categories";
import { PackageCard } from "@/components/registry/PackageCard";
import { CategoryFilter } from "@/components/registry/CategoryFilter";
import {
  ChevronRight,
  Globe,
  Terminal,
  Wrench,
  Shield,
  Calculator,
  Database,
  CheckCircle,
  Package,
} from "lucide-react";

interface CategoryPageProps {
  params: {
    category: string;
  };
}

const iconMap: Record<string, React.FC<{ className?: string }>> = {
  Globe,
  Terminal,
  Wrench,
  Shield,
  Calculator,
  Database,
  CheckCircle,
};

export function generateStaticParams() {
  return CATEGORIES.map((cat) => ({
    category: cat.id,
  }));
}

export function generateMetadata({ params }: CategoryPageProps): Metadata {
  const category = CATEGORIES.find((c) => c.id === params.category);
  if (!category) {
    return {
      title: "Category Not Found",
    };
  }

  return {
    title: `${category.name} Packages — SP Package Registry`,
    description: category.description,
  };
}

export default function CategoryDetailPage({ params }: CategoryPageProps) {
  const category = CATEGORIES.find((c) => c.id === params.category);

  if (!category) {
    notFound();
  }

  const categoryPackages = PACKAGES.filter((p) => p.category === category.id);
  const Icon = iconMap[category.iconName] || Wrench;

  return (
    <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-10 space-y-8">
      {/* Breadcrumbs */}
      <nav className="flex items-center gap-1.5 text-xs text-slate-400 font-mono">
        <Link href="/" className="hover:text-cyan-400">
          Registry
        </Link>
        <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
        <Link href="/packages" className="hover:text-cyan-400">
          Categories
        </Link>
        <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
        <span className="text-white capitalize">{category.name}</span>
      </nav>

      {/* Category Banner */}
      <div className="rounded-2xl border border-slate-800 bg-slate-900/40 p-6 sm:p-8">
        <div className="flex items-start gap-4">
          <div className="w-12 h-12 rounded-xl bg-cyan-500/10 border border-cyan-500/30 text-cyan-400 flex items-center justify-center shrink-0">
            <Icon className="w-6 h-6" />
          </div>
          <div>
            <div className="flex items-center gap-3">
              <h1 className="text-2xl sm:text-3xl font-extrabold text-white tracking-tight">
                {category.name}
              </h1>
              <span className="text-xs font-mono text-cyan-400 bg-cyan-950/60 border border-cyan-800/60 px-2.5 py-0.5 rounded-full">
                {categoryPackages.length} {categoryPackages.length === 1 ? "package" : "packages"}
              </span>
            </div>
            <p className="mt-2 text-sm text-slate-300 max-w-2xl leading-relaxed">
              {category.description}
            </p>
          </div>
        </div>
      </div>

      {/* Category Filter Pills */}
      <div>
        <CategoryFilter selectedCategory={category.id} isLink={true} />
      </div>

      {/* Package Grid */}
      <div className="space-y-4">
        <div className="flex items-center justify-between pb-2 border-b border-slate-800/80 text-xs font-mono text-slate-400">
          <span>Packages in {category.name}</span>
          <Link href="/packages" className="text-cyan-400 hover:underline">
            Browse all categories &rarr;
          </Link>
        </div>

        {categoryPackages.length > 0 ? (
          <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
            {categoryPackages.map((pkg) => (
              <PackageCard key={pkg.name} pkg={pkg} />
            ))}
          </div>
        ) : (
          <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-12 text-center">
            <Package className="w-8 h-8 text-slate-500 mx-auto mb-2" />
            <h3 className="text-base font-semibold text-white font-mono mb-1">
              No packages published in this category yet
            </h3>
            <p className="text-xs text-slate-400 max-w-sm mx-auto mb-4">
              Be the first to publish an SP package for {category.name}!
            </p>
            <Link
              href="/publish"
              className="inline-flex items-center px-4 py-2 rounded-lg bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-semibold text-xs transition-colors"
            >
              Publish a Package
            </Link>
          </div>
        )}
      </div>
    </div>
  );
}
