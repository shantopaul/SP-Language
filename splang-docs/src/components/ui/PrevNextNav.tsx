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
    <div className="mt-16 pt-8 border-t border-slate-200 dark:border-[#0A1F44] grid grid-cols-1 sm:grid-cols-2 gap-4">
      {prev ? (
        <Link
          href={prev.href}
          className="group flex flex-col p-4 rounded-xl border border-slate-200 dark:border-[#0A1F44] hover:border-blue-300 dark:hover:border-[#0057FF]/60 bg-white dark:bg-[#081735]/60 hover:bg-slate-50 dark:hover:bg-[#081735] transition-all text-left shadow-sm dark:shadow-lg"
        >
          <span className="text-[11px] uppercase tracking-wider text-slate-500 flex items-center gap-1 mb-1">
            <ChevronLeft className="w-3.5 h-3.5 group-hover:-translate-x-0.5 transition-transform text-[#0057FF] dark:text-[#00D1FF]" />
            Previous
          </span>
          <span className="text-sm font-semibold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors">
            {prev.title}
          </span>
        </Link>
      ) : (
        <div />
      )}

      {next ? (
        <Link
          href={next.href}
          className="group flex flex-col p-4 rounded-xl border border-slate-200 dark:border-[#0A1F44] hover:border-blue-300 dark:hover:border-[#0057FF]/60 bg-white dark:bg-[#081735]/60 hover:bg-slate-50 dark:hover:bg-[#081735] transition-all text-right sm:items-end shadow-sm dark:shadow-lg"
        >
          <span className="text-[11px] uppercase tracking-wider text-slate-500 flex items-center gap-1 mb-1">
            Next
            <ChevronRight className="w-3.5 h-3.5 group-hover:translate-x-0.5 transition-transform text-[#0057FF] dark:text-[#00D1FF]" />
          </span>
          <span className="text-sm font-semibold text-slate-900 dark:text-white group-hover:text-[#0057FF] dark:group-hover:text-[#00D1FF] transition-colors">
            {next.title}
          </span>
        </Link>
      ) : (
        <div />
      )}
    </div>
  );
};
