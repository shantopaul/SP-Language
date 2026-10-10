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
      <nav className="flex items-center gap-2 text-xs text-slate-400 font-mono">
        <Link href="/" className="hover:text-[#00D1FF] transition-colors">
          Registry
        </Link>
        <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
        <Link href="/packages" className="hover:text-[#00D1FF] transition-colors">
          Categories
        </Link>
        <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
        <span className="text-[#00D1FF] capitalize font-semibold">{category.name}</span>
      </nav>

      {/* Category Banner */}
      <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/60 p-6 sm:p-8 shadow-xl">
        <div className="flex items-start gap-4">
          <div className="w-12 h-12 rounded-xl bg-[#0057FF]/15 border border-[#00D1FF]/40 text-[#00D1FF] flex items-center justify-center shrink-0 shadow-[0_0_15px_rgba(0,209,255,0.2)]">
            <Icon className="w-6 h-6" />
          </div>
          <div>
            <div className="flex items-center gap-3">
              <h1 className="text-2xl sm:text-3xl font-black text-white tracking-tight">
                {category.name}
              </h1>
              <span className="text-xs font-mono font-semibold text-[#00D1FF] bg-[#0057FF]/20 border border-[#00D1FF]/40 px-3 py-0.5 rounded-full shadow-[0_0_10px_rgba(0,209,255,0.15)]">
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
        <div className="flex items-center justify-between pb-3 border-b border-[#0A1F44] text-xs font-mono text-slate-400">
          <span>Packages in {category.name}</span>
          <Link href="/packages" className="text-[#00D1FF] hover:underline font-semibold">
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
          <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/40 p-12 text-center shadow-xl">
            <Package className="w-8 h-8 text-slate-500 mx-auto mb-2" />
            <h3 className="text-base font-bold text-white font-mono mb-1">
              No packages published in this category yet
            </h3>
            <p className="text-xs text-slate-400 max-w-sm mx-auto mb-4 leading-relaxed">
              Be the first to publish an SP package for {category.name}!
            </p>
            <Link
              href="/publish"
              className="inline-flex items-center px-4 py-2 rounded-xl bg-gradient-to-r from-[#0057FF] to-[#00D1FF] hover:opacity-90 text-white font-bold text-xs transition-all shadow-md"
            >
              Publish a Package
            </Link>
          </div>
        )}
      </div>
    </div>
  );
}
