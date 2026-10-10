"use client";

import React from "react";
import { AlignLeft } from "lucide-react";

export interface TocHeading {
  id: string;
  title: string;
  level: number;
}

interface TableOfContentsProps {
  headings?: TocHeading[];
}

export const TableOfContents: React.FC<TableOfContentsProps> = ({ headings = [] }) => {
  if (headings.length === 0) return null;

  return (
    <div className="hidden xl:block w-56 shrink-0 sticky top-24 self-start pl-6 text-xs border-l border-slate-200 dark:border-[#0A1F44] transition-colors">
      <div className="flex items-center gap-1.5 font-bold uppercase tracking-wider text-slate-500 dark:text-slate-400 mb-3 text-[11px]">
        <AlignLeft className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF]" />
        <span>On This Page</span>
      </div>
      <ul className="space-y-2">
        {headings.map((h) => (
          <li
            key={h.id}
            style={{ paddingLeft: `${(h.level - 2) * 12}px` }}
          >
            <a
              href={`#${h.id}`}
              className="text-slate-600 dark:text-slate-400 hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors block py-0.5"
            >
              {h.title}
            </a>
          </li>
        ))}
      </ul>
    </div>
  );
};
