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
    <div className="hidden xl:block w-56 shrink-0 sticky top-24 self-start pl-6 text-xs border-l border-slate-800/80">
      <div className="flex items-center gap-1.5 font-semibold uppercase tracking-wider text-slate-400 mb-3 text-[11px]">
        <AlignLeft className="w-3.5 h-3.5 text-cyan-400" />
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
              className="text-slate-400 hover:text-cyan-400 transition-colors block py-0.5"
            >
              {h.title}
            </a>
          </li>
        ))}
      </ul>
    </div>
  );
};
