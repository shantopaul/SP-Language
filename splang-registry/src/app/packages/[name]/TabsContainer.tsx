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
      <div className="flex items-center gap-1 border-b border-slate-800 pb-px overflow-x-auto">
        <button
          onClick={() => setActiveTab("readme")}
          className={`flex items-center gap-2 px-4 py-2.5 text-xs sm:text-sm font-semibold transition-colors border-b-2 font-mono whitespace-nowrap ${
            activeTab === "readme"
              ? "border-cyan-400 text-cyan-400"
              : "border-transparent text-slate-400 hover:text-slate-200"
          }`}
        >
          <BookOpen className="w-4 h-4" />
          <span>Documentation</span>
        </button>

        <button
          onClick={() => setActiveTab("versions")}
          className={`flex items-center gap-2 px-4 py-2.5 text-xs sm:text-sm font-semibold transition-colors border-b-2 font-mono whitespace-nowrap ${
            activeTab === "versions"
              ? "border-cyan-400 text-cyan-400"
              : "border-transparent text-slate-400 hover:text-slate-200"
          }`}
        >
          <History className="w-4 h-4" />
          <span>Versions</span>
        </button>

        <button
          onClick={() => setActiveTab("dependencies")}
          className={`flex items-center gap-2 px-4 py-2.5 text-xs sm:text-sm font-semibold transition-colors border-b-2 font-mono whitespace-nowrap ${
            activeTab === "dependencies"
              ? "border-cyan-400 text-cyan-400"
              : "border-transparent text-slate-400 hover:text-slate-200"
          }`}
        >
          <Layers className="w-4 h-4" />
          <span>Dependencies</span>
        </button>

        <button
          onClick={() => setActiveTab("security")}
          className={`flex items-center gap-2 px-4 py-2.5 text-xs sm:text-sm font-semibold transition-colors border-b-2 font-mono whitespace-nowrap ${
            activeTab === "security"
              ? "border-cyan-400 text-cyan-400"
              : "border-transparent text-slate-400 hover:text-slate-200"
          }`}
        >
          <ShieldCheck className="w-4 h-4" />
          <span>Security & Hashes</span>
        </button>
      </div>

      {/* Tab Content Panes */}
      <div className="min-h-[400px]">
        {activeTab === "readme" && (
          <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-6 sm:p-8">
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
