"use client";

import React from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { X, BookOpen } from "lucide-react";
import { DOCS_NAVIGATION } from "@/data/navigation";
import { cn } from "@/lib/utils";
import { ThemeToggle } from "@/components/theme/ThemeToggle";

interface SidebarProps {
  isOpen?: boolean;
  onClose?: () => void;
}

export const Sidebar: React.FC<SidebarProps> = ({ isOpen, onClose }) => {
  const pathname = usePathname();

  return (
    <>
      {/* Mobile backdrop overlay */}
      {isOpen && (
        <div
          className="fixed inset-0 z-40 bg-black/60 backdrop-blur-sm lg:hidden animate-in fade-in duration-200"
          onClick={onClose}
          aria-hidden="true"
        />
      )}

      {/* Sidebar container */}
      <aside
        className={cn(
          "fixed top-0 bottom-0 left-0 z-50 w-72 sm:w-80 border-r border-slate-200 dark:border-[#0A1F44] bg-white/98 dark:bg-[#020D21]/98 lg:bg-transparent lg:top-16 lg:z-30 lg:w-64 lg:shrink-0 lg:border-r overflow-y-auto px-4 py-6 transition-transform duration-200 lg:translate-x-0 lg:static scrollbar-thin shadow-2xl lg:shadow-none",
          isOpen ? "translate-x-0" : "-translate-x-full"
        )}
      >
        {/* Mobile Header with close button */}
        <div className="flex lg:hidden items-center justify-between pb-4 mb-4 border-b border-slate-200 dark:border-[#0A1F44]">
          <div className="flex items-center gap-2">
            <BookOpen className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
            <span className="font-bold text-sm text-slate-900 dark:text-white">Documentation</span>
          </div>
          <button
            onClick={onClose}
            className="p-1.5 rounded-lg text-slate-500 hover:text-slate-900 dark:text-slate-400 dark:hover:text-white bg-slate-100 dark:bg-[#081735] border border-slate-200 dark:border-[#0A1F44]"
            aria-label="Close sidebar"
          >
            <X className="w-5 h-5" />
          </button>
        </div>

        <nav className="space-y-6">
          {DOCS_NAVIGATION.map((section, idx) => (
            <div key={idx} className="space-y-1">
              <h4 className="text-[11px] font-bold uppercase tracking-wider text-slate-500 dark:text-slate-400 px-2.5 mb-1.5">
                {section.title}
              </h4>
              <ul className="space-y-0.5">
                {section.items.map((item) => {
                  const isActive = pathname === item.href;
                  return (
                    <li key={item.href}>
                      <Link
                        href={item.href}
                        onClick={onClose}
                        className={cn(
                          "flex items-center justify-between px-3 py-2 rounded-xl text-xs transition-all duration-150 min-h-[38px]",
                          isActive
                            ? "bg-blue-50 dark:bg-[#0057FF]/15 text-[#0057FF] dark:text-[#00D1FF] border border-blue-200 dark:border-[#00D1FF]/40 font-semibold shadow-sm dark:shadow-[0_0_12px_rgba(0,209,255,0.15)]"
                            : "text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-slate-100 dark:hover:bg-[#081735]/70"
                        )}
                      >
                        <span className="truncate">{item.title}</span>
                        {item.badge && (
                          <span className="text-[10px] px-1.5 py-0.5 rounded-full bg-blue-100 dark:bg-[#0057FF]/20 text-[#0057FF] dark:text-[#00D1FF] border border-blue-200 dark:border-[#00D1FF]/30 font-medium">
                            {item.badge}
                          </span>
                        )}
                      </Link>
                    </li>
                  );
                })}
              </ul>
            </div>
          ))}
        </nav>

        {/* Mobile footer within drawer */}
        <div className="pt-6 mt-6 border-t border-slate-200 dark:border-[#0A1F44] lg:hidden space-y-3">
          <div className="text-[10px] font-mono uppercase text-slate-500">Theme Mode</div>
          <ThemeToggle variant="segmented" className="w-full justify-around" />
        </div>
      </aside>
    </>
  );
};
