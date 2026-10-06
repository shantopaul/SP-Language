"use client";

import React from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { DOCS_NAVIGATION } from "@/data/navigation";
import { cn } from "@/lib/utils";
import { ChevronRight } from "lucide-react";

interface SidebarProps {
  isOpen?: boolean;
  onClose?: () => void;
}

export const Sidebar: React.FC<SidebarProps> = ({ isOpen, onClose }) => {
  const pathname = usePathname();

  return (
    <>
      {/* Mobile backdrop */}
      {isOpen && (
        <div
          className="fixed inset-0 z-40 bg-black/70 backdrop-blur-sm lg:hidden"
          onClick={onClose}
        />
      )}

      {/* Sidebar container */}
      <aside
        className={cn(
          "fixed top-16 bottom-0 left-0 z-40 w-64 border-r border-slate-800 bg-slate-950/95 overflow-y-auto px-4 py-6 transition-transform duration-200 lg:translate-x-0 lg:static lg:w-64 lg:shrink-0",
          isOpen ? "translate-x-0" : "-translate-x-full"
        )}
      >
        <nav className="space-y-8">
          {DOCS_NAVIGATION.map((section, idx) => (
            <div key={idx} className="space-y-2">
              <h4 className="text-xs font-semibold uppercase tracking-wider text-slate-400 px-2">
                {section.title}
              </h4>
              <ul className="space-y-1">
                {section.items.map((item) => {
                  const isActive = pathname === item.href;
                  return (
                    <li key={item.href}>
                      <Link
                        href={item.href}
                        onClick={onClose}
                        className={cn(
                          "flex items-center justify-between px-2.5 py-1.5 rounded-lg text-xs font-medium transition-colors",
                          isActive
                            ? "bg-cyan-950/60 text-cyan-300 border border-cyan-800/60 font-semibold"
                            : "text-slate-400 hover:text-slate-200 hover:bg-slate-900/60"
                        )}
                      >
                        <span className="truncate">{item.title}</span>
                        {item.badge && (
                          <span className="text-[10px] px-1.5 py-0.2 rounded bg-cyan-950 text-cyan-400 border border-cyan-800">
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
      </aside>
    </>
  );
};
