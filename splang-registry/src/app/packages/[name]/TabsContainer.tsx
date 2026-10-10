"use client";

import React, { useState } from "react";
import { BookOpen, History, Layers, ShieldCheck } from "lucide-react";

interface TabsContainerProps {
  readmeContent: React.ReactNode;
  versionsContent: React.ReactNode;
  dependenciesContent: React.ReactNode;
  securityContent: React.ReactNode;
}

export const TabsContainer: React.FC<TabsContainerProps> = ({
  readmeContent,
  versionsContent,
  dependenciesContent,
  securityContent,
}) => {
  const [activeTab, setActiveTab] = useState<
    "readme" | "versions" | "dependencies" | "security"
  >("readme");

  return (
    <div className="space-y-6">
      {/* Tab Switcher */}
      <div className="flex items-center gap-1 border-b border-slate-200 dark:border-[#0A1F44] pb-px overflow-x-auto -mx-4 px-4 sm:mx-0 sm:px-0 scrollbar-thin scrollbar-thumb-slate-300 dark:scrollbar-thumb-[#0A1F44]">
        <button
          onClick={() => setActiveTab("readme")}
          className={`min-h-[44px] flex items-center gap-2 px-3.5 sm:px-4 py-2.5 text-xs sm:text-sm font-semibold transition-all border-b-2 font-mono whitespace-nowrap ${
            activeTab === "readme"
              ? "border-[#0057FF] dark:border-[#00D1FF] text-[#0057FF] dark:text-[#00D1FF] bg-blue-50 dark:bg-[#0057FF]/10 rounded-t-lg shadow-sm dark:shadow-[0_0_12px_rgba(0,209,255,0.15)]"
              : "border-transparent text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"
          }`}
        >
          <BookOpen className="w-4 h-4" />
          <span>Documentation</span>
        </button>

        <button
          onClick={() => setActiveTab("versions")}
          className={`min-h-[44px] flex items-center gap-2 px-3.5 sm:px-4 py-2.5 text-xs sm:text-sm font-semibold transition-all border-b-2 font-mono whitespace-nowrap ${
            activeTab === "versions"
              ? "border-[#0057FF] dark:border-[#00D1FF] text-[#0057FF] dark:text-[#00D1FF] bg-blue-50 dark:bg-[#0057FF]/10 rounded-t-lg shadow-sm dark:shadow-[0_0_12px_rgba(0,209,255,0.15)]"
              : "border-transparent text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"
          }`}
        >
          <History className="w-4 h-4" />
          <span>Versions</span>
        </button>

        <button
          onClick={() => setActiveTab("dependencies")}
          className={`min-h-[44px] flex items-center gap-2 px-3.5 sm:px-4 py-2.5 text-xs sm:text-sm font-semibold transition-all border-b-2 font-mono whitespace-nowrap ${
            activeTab === "dependencies"
              ? "border-[#0057FF] dark:border-[#00D1FF] text-[#0057FF] dark:text-[#00D1FF] bg-blue-50 dark:bg-[#0057FF]/10 rounded-t-lg shadow-sm dark:shadow-[0_0_12px_rgba(0,209,255,0.15)]"
              : "border-transparent text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"
          }`}
        >
          <Layers className="w-4 h-4" />
          <span>Dependencies</span>
        </button>

        <button
          onClick={() => setActiveTab("security")}
          className={`min-h-[44px] flex items-center gap-2 px-3.5 sm:px-4 py-2.5 text-xs sm:text-sm font-semibold transition-all border-b-2 font-mono whitespace-nowrap ${
            activeTab === "security"
              ? "border-[#0057FF] dark:border-[#00D1FF] text-[#0057FF] dark:text-[#00D1FF] bg-blue-50 dark:bg-[#0057FF]/10 rounded-t-lg shadow-sm dark:shadow-[0_0_12px_rgba(0,209,255,0.15)]"
              : "border-transparent text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white"
          }`}
        >
          <ShieldCheck className="w-4 h-4" />
          <span>Security &amp; Hashes</span>
        </button>
      </div>

      {/* Tab Content Panes */}
      <div className="min-h-[400px]">
        {activeTab === "readme" && (
          <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/40 p-4 sm:p-8 shadow-sm dark:shadow-xl">
            {readmeContent}
          </div>
        )}

        {activeTab === "versions" && (
          <div className="space-y-4">{versionsContent}</div>
        )}

        {activeTab === "dependencies" && (
          <div className="space-y-4">{dependenciesContent}</div>
        )}

        {activeTab === "security" && (
          <div className="space-y-4">{securityContent}</div>
        )}
      </div>
    </div>
  );
};
