"use client";

import React from "react";
import Link from "next/link";
import { ChevronRight, Home } from "lucide-react";

interface BreadcrumbItem {
  title: string;
  href?: string;
}

interface BreadcrumbsProps {
  items: BreadcrumbItem[];
}

export const Breadcrumbs: React.FC<BreadcrumbsProps> = ({ items }) => {
  return (
    <nav className="flex items-center gap-1.5 text-xs text-slate-400 mb-6 flex-wrap">
      <Link
        href="/getting-started"
        className="hover:text-cyan-400 transition-colors flex items-center gap-1"
      >
        <Home className="w-3.5 h-3.5" />
        <span>Docs</span>
      </Link>
      {items.map((item, idx) => {
        const isLast = idx === items.length - 1;
        return (
          <React.Fragment key={idx}>
            <ChevronRight className="w-3 h-3 text-slate-600" />
            {item.href && !isLast ? (
              <Link href={item.href} className="hover:text-cyan-400 transition-colors">
                {item.title}
              </Link>
            ) : (
              <span className="text-slate-200 font-medium">{item.title}</span>
            )}
          </React.Fragment>
        );
      })}
    </nav>
  );
};
