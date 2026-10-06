import React from "react";
import Link from "next/link";
import { ChevronLeft, ChevronRight } from "lucide-react";

interface NavLink {
  title: string;
  href: string;
}

interface PrevNextNavProps {
  prev?: NavLink;
  next?: NavLink;
}

export const PrevNextNav: React.FC<PrevNextNavProps> = ({ prev, next }) => {
  return (
    <div className="mt-16 pt-8 border-t border-slate-800/80 grid grid-cols-1 sm:grid-cols-2 gap-4">
      {prev ? (
        <Link
          href={prev.href}
          className="group flex flex-col p-4 rounded-xl border border-slate-800 hover:border-cyan-500/50 bg-slate-900/30 hover:bg-slate-900/60 transition-all text-left"
        >
          <span className="text-[11px] uppercase tracking-wider text-slate-500 flex items-center gap-1 mb-1">
            <ChevronLeft className="w-3.5 h-3.5 group-hover:-translate-x-0.5 transition-transform" />
            Previous
          </span>
          <span className="text-sm font-semibold text-white group-hover:text-cyan-400 transition-colors">
            {prev.title}
          </span>
        </Link>
      ) : (
        <div />
      )}

      {next ? (
        <Link
          href={next.href}
          className="group flex flex-col p-4 rounded-xl border border-slate-800 hover:border-cyan-500/50 bg-slate-900/30 hover:bg-slate-900/60 transition-all text-right sm:items-end"
        >
          <span className="text-[11px] uppercase tracking-wider text-slate-500 flex items-center gap-1 mb-1">
            Next
            <ChevronRight className="w-3.5 h-3.5 group-hover:translate-x-0.5 transition-transform" />
          </span>
          <span className="text-sm font-semibold text-white group-hover:text-cyan-400 transition-colors">
            {next.title}
          </span>
        </Link>
      ) : (
        <div />
      )}
    </div>
  );
};
