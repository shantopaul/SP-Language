"use client";

import React, { useState } from "react";
import { Check, Copy, Terminal, Shield } from "lucide-react";

interface InstallTab {
  id: string;
  label: string;
  os: string;
  command: string;
  description: string;
}

const INSTALL_TABS: InstallTab[] = [
  {
    id: "windows-winget",
    label: "Windows (winget)",
    os: "Windows 10/11",
    command: "winget install SPLang.SP",
    description: "Installs official SP compiler and packages to system PATH.",
  },
  {
    id: "windows-ps",
    label: "PowerShell",
    os: "Windows",
    command: "iwr -useb https://splang.shantopaul.com/install.ps1 | iex",
    description: "Automated PowerShell installer with environment verification.",
  },
  {
    id: "unix-curl",
    label: "Linux / macOS",
    os: "Ubuntu, Debian, macOS, Fedora, Arch",
    command: "curl -fsSL https://splang.shantopaul.com/install.sh | sh",
    description: "Direct shell installer detecting OS and CPU architecture.",
  },
  {
    id: "docker",
    label: "Docker Container",
    os: "Multi-arch (x86_64, arm64)",
    command: "docker pull shantopaul/splang:1.0.0",
    description: "Pre-configured isolated container with full LLVM toolchain.",
  },
];

export const QuickInstall: React.FC = () => {
  const [activeTab, setActiveTab] = useState(INSTALL_TABS[0].id);
  const [copied, setCopied] = useState(false);

  const currentTab = INSTALL_TABS.find((t) => t.id === activeTab) || INSTALL_TABS[0];

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(currentTab.command);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    } catch {
      // fallback
    }
  };

  return (
    <div className="w-full max-w-2xl mx-auto rounded-2xl border border-slate-800 bg-slate-900/90 shadow-2xl backdrop-blur-xl overflow-hidden">
      {/* OS Switcher Tabs */}
      <div className="flex border-b border-slate-800 bg-slate-950/60 overflow-x-auto">
        {INSTALL_TABS.map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`px-4 py-2.5 text-xs sm:text-sm font-medium whitespace-nowrap border-b-2 transition-all ${
              activeTab === tab.id
                ? "border-cyan-400 text-cyan-400 bg-slate-900/80"
                : "border-transparent text-slate-400 hover:text-slate-200 hover:bg-slate-900/40"
            }`}
          >
            {tab.label}
          </button>
        ))}
      </div>

      {/* Terminal Command Bar */}
      <div className="p-4 sm:p-5 flex flex-col gap-3">
        <div className="flex items-center justify-between gap-3 bg-black/60 rounded-xl px-4 py-3 border border-slate-800 font-mono text-xs sm:text-sm">
          <div className="flex items-center gap-2.5 overflow-x-auto text-slate-200">
            <Terminal className="w-4 h-4 text-cyan-400 shrink-0" />
            <span className="text-slate-500 select-none">$</span>
            <span className="font-semibold text-emerald-400 whitespace-nowrap">
              {currentTab.command}
            </span>
          </div>
          <button
            onClick={handleCopy}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-800 hover:bg-slate-700 text-slate-200 hover:text-white transition-colors text-xs font-sans shrink-0 font-medium"
            title="Copy command"
          >
            {copied ? (
              <>
                <Check className="w-3.5 h-3.5 text-emerald-400" />
                <span className="text-emerald-400">Copied</span>
              </>
            ) : (
              <>
                <Copy className="w-3.5 h-3.5" />
                <span>Copy</span>
              </>
            )}
          </button>
        </div>

        <div className="flex items-center justify-between text-xs text-slate-400 px-1">
          <span>{currentTab.description}</span>
          <span className="flex items-center gap-1 text-slate-500 font-mono text-[11px]">
            <Shield className="w-3 h-3 text-cyan-400" />
            SHA-256 Verified
          </span>
        </div>
      </div>
    </div>
  );
};
