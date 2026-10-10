"use client";

import React from "react";
import Link from "next/link";
import { usePathname } from "next/navigation";
import { DOCS_NAVIGATION } from "@/data/navigation";
import { cn } from "@/lib/utils";

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
          className="fixed inset-0 z-40 bg-black/80 backdrop-blur-sm lg:hidden animate-in fade-in duration-200"
          onClick={onClose}
        />
      )}

      {/* Sidebar container */}
      <aside
        className={cn(
          "fixed top-16 bottom-0 left-0 z-40 w-64 border-r border-[#0A1F44] bg-[#020D21]/95 lg:bg-transparent overflow-y-auto px-4 py-6 transition-transform duration-200 lg:translate-x-0 lg:static lg:w-64 lg:shrink-0 scrollbar-thin",
          isOpen ? "translate-x-0" : "-translate-x-full"
        )}
      >
        <nav className="space-y-7">
          {DOCS_NAVIGATION.map((section, idx) => (
            <div key={idx} className="space-y-1.5">
              <h4 className="text-[11px] font-bold uppercase tracking-wider text-slate-400 px-2.5">
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
                          "flex items-center justify-between px-2.5 py-1.5 rounded-lg text-xs transition-all duration-150",
                          isActive
                            ? "bg-[#0057FF]/15 text-[#00D1FF] border border-[#00D1FF]/40 font-semibold shadow-[0_0_12px_rgba(0,209,255,0.15)]"
                            : "text-slate-400 hover:text-white hover:bg-[#081735]/70"
                        )}
                      >
                        <span className="truncate">{item.title}</span>
                        {item.badge && (
                          <span className="text-[10px] px-1.5 py-0.2 rounded-full bg-[#0057FF]/20 text-[#00D1FF] border border-[#00D1FF]/30 font-medium">
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
