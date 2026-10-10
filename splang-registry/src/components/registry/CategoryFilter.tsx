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
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-xs font-medium whitespace-nowrap transition-all border ${
            !selectedCategory
              ? "bg-[#0057FF]/20 border-[#00D1FF]/50 text-[#00D1FF] font-semibold shadow-[0_0_12px_rgba(0,209,255,0.2)]"
              : "bg-[#081735]/80 border-[#0A1F44] text-slate-400 hover:text-white hover:border-[#0057FF]/40"
          }`}
        >
          <LayoutGrid className="w-3.5 h-3.5" />
          <span>All Categories</span>
        </Link>
      ) : (
        <button
          onClick={() => onSelectCategory?.(undefined)}
          className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-xs font-medium whitespace-nowrap transition-all border ${
            !selectedCategory
              ? "bg-[#0057FF]/20 border-[#00D1FF]/50 text-[#00D1FF] font-semibold shadow-[0_0_12px_rgba(0,209,255,0.2)]"
              : "bg-[#081735]/80 border-[#0A1F44] text-slate-400 hover:text-white hover:border-[#0057FF]/40"
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
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-xs font-medium whitespace-nowrap transition-all border ${
                isSelected
                  ? "bg-[#0057FF]/20 border-[#00D1FF]/50 text-[#00D1FF] font-semibold shadow-[0_0_12px_rgba(0,209,255,0.2)]"
                  : "bg-[#081735]/80 border-[#0A1F44] text-slate-400 hover:text-white hover:border-[#0057FF]/40"
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
            className={`flex items-center gap-1.5 px-3 py-1.5 rounded-xl text-xs font-medium whitespace-nowrap transition-all border ${
              isSelected
                ? "bg-[#0057FF]/20 border-[#00D1FF]/50 text-[#00D1FF] font-semibold shadow-[0_0_12px_rgba(0,209,255,0.2)]"
                : "bg-[#081735]/80 border-[#0A1F44] text-slate-400 hover:text-white hover:border-[#0057FF]/40"
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
