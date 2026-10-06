"use client";

import React from "react";
import Link from "next/link";
import { CATEGORIES } from "@/data/categories";
import {
  Globe,
  Terminal,
  Wrench,
  Shield,
  Calculator,
  Database,
  CheckCircle,
  LayoutGrid,
} from "lucide-react";

interface CategoryFilterProps {
  selectedCategory?: string;
  onSelectCategory?: (category: string | undefined) => void;
  isLink?: boolean;
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

export const CategoryFilter: React.FC<CategoryFilterProps> = ({
  selectedCategory,
  onSelectCategory,
  isLink = false,
}) => {
  return (
    <div className="flex items-center gap-2 overflow-x-auto pb-2 scrollbar-none">
      {isLink ? (
        <Link
          href="/packages"
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium whitespace-nowrap transition-colors border ${
            !selectedCategory
              ? "bg-cyan-500/10 border-cyan-500/40 text-cyan-400"
              : "bg-slate-900 border-slate-800 text-slate-400 hover:text-slate-200 hover:border-slate-700"
          }`}
        >
          <LayoutGrid className="w-3.5 h-3.5" />
          <span>All Categories</span>
        </Link>
      ) : (
        <button
          onClick={() => onSelectCategory?.(undefined)}
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium whitespace-nowrap transition-colors border ${
            !selectedCategory
              ? "bg-cyan-500/10 border-cyan-500/40 text-cyan-400"
              : "bg-slate-900 border-slate-800 text-slate-400 hover:text-slate-200 hover:border-slate-700"
          }`}
        >
          <LayoutGrid className="w-3.5 h-3.5" />
          <span>All Categories</span>
        </button>
      )}

      {CATEGORIES.map((cat) => {
        const Icon = iconMap[cat.iconName] || Wrench;
        const isSelected = selectedCategory === cat.id;

        if (isLink) {
          return (
            <Link
              key={cat.id}
              href={`/categories/${cat.id}`}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium whitespace-nowrap transition-colors border ${
                isSelected
                  ? "bg-cyan-500/10 border-cyan-500/40 text-cyan-400"
                  : "bg-slate-900 border-slate-800 text-slate-400 hover:text-slate-200 hover:border-slate-700"
              }`}
            >
              <Icon className="w-3.5 h-3.5" />
              <span>{cat.name}</span>
            </Link>
          );
        }

        return (
          <button
            key={cat.id}
            onClick={() => onSelectCategory?.(cat.id)}
            className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium whitespace-nowrap transition-colors border ${
              isSelected
                ? "bg-cyan-500/10 border-cyan-500/40 text-cyan-400"
                : "bg-slate-900 border-slate-800 text-slate-400 hover:text-slate-200 hover:border-slate-700"
            }`}
          >
            <Icon className="w-3.5 h-3.5" />
            <span>{cat.name}</span>
          </button>
        );
      })}
    </div>
  );
};
