"use client";

import React, { useState } from "react";
import { Check, Copy, Terminal, Shield, HardDrive } from "lucide-react";

interface InstallTab {
  id: string;
  label: string;
  os: string;
  command: string;
  description: string;
}

const INSTALL_TABS: InstallTab[] = [
  {
    id: "windows-ps",
    label: "PowerShell (Windows)",
    os: "Windows 10/11 (x64)",
    command: "irm https://dl.splang.shantopaul.com/install.ps1 | iex",
    description: "Instant 1-click Windows installer with universal drive & LLVM toolchain auto-setup.",
  },
  {
    id: "unix-curl",
    label: "Linux / macOS",
    os: "Ubuntu, Debian, macOS, Fedora, Arch",
    command: "curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh",
    description: "Native POSIX shell installer with CPU architecture detection and glibc runtime linkage.",
  },
  {
    id: "upgrade",
    label: "Auto-Upgrade",
    os: "All Platforms",
    command: "spc upgrade",
    description: "In-place zero-downtime auto-upgrade to the latest SP compiler release (v3.1.0 GA).",
  },
  {
    id: "uninstall",
    label: "Clean Uninstall",
    os: "Windows / Linux / macOS",
    command: "spc uninstall",
    description: "Full clean removal: spc uninstall (or irm https://dl.splang.shantopaul.com/uninstall.ps1 | iex).",
  },
  {
    id: "windows-winget",
    label: "Winget",
    os: "Windows 10/11",
    command: "winget install SPLang.SP",
    description: "Installs official SP compiler package via Windows Package Manager.",
  },
  {
    id: "docker",
    label: "Docker",
    os: "Multi-arch (x86_64, arm64)",
    command: "docker pull shantopaul/splang:3.1.0",
    description: "Isolated container environment bundled with LLVM Clang 18 and precompiled runtime.",
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
    <div className="w-full max-w-3xl mx-auto rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/85 shadow-xl dark:shadow-2xl backdrop-blur-2xl overflow-hidden transition-colors">
      {/* OS Switcher Tabs */}
      <div className="flex border-b border-slate-200 dark:border-[#0A1F44] bg-slate-100 dark:bg-[#020D21]/80 overflow-x-auto scrollbar-none px-1">
        {INSTALL_TABS.map((tab) => (
          <button
            key={tab.id}
            onClick={() => setActiveTab(tab.id)}
            className={`px-3.5 sm:px-5 py-3 text-xs sm:text-sm font-medium whitespace-nowrap border-b-2 transition-all flex-shrink-0 min-h-[44px] ${
              activeTab === tab.id
                ? "border-[#0057FF] dark:border-[#00D1FF] text-[#0057FF] dark:text-[#00D1FF] bg-white dark:bg-[#081735]/90 font-semibold shadow-sm dark:shadow-[inset_0_-2px_10px_rgba(0,209,255,0.1)]"
                : "border-transparent text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200 hover:bg-slate-200/50 dark:hover:bg-[#081735]/40"
            }`}
          >
            {tab.label}
          </button>
        ))}
      </div>

      {/* Terminal Command Bar */}
      <div className="p-3.5 sm:p-6 flex flex-col gap-3">
        <div className="flex items-center justify-between gap-2 sm:gap-3 bg-slate-900 dark:bg-[#020D21] rounded-xl px-3 sm:px-4 py-3 sm:py-3.5 border border-slate-800 dark:border-[#0A1F44] font-mono text-xs sm:text-sm group hover:border-[#0057FF]/40 dark:hover:border-cyan-500/40 transition-colors">
          <div className="flex items-center gap-2 sm:gap-3 overflow-x-auto scrollbar-none text-slate-200 py-0.5 min-w-0 flex-1">
            <Terminal className="w-4 h-4 text-[#00D1FF] shrink-0" />
            <span className="text-slate-500 select-none shrink-0">$</span>
            <span className="font-semibold text-cyan-300 whitespace-nowrap selection:bg-cyan-500/30">
              {currentTab.command}
            </span>
          </div>
          <button
            onClick={handleCopy}
            className="flex items-center gap-1.5 px-3 py-1.5 rounded-lg bg-slate-800 dark:bg-[#081735] hover:bg-slate-700 dark:hover:bg-[#0A1F44] text-slate-200 hover:text-white transition-all text-xs font-sans shrink-0 font-medium border border-slate-700 dark:border-[#0A1F44] hover:border-cyan-500/40 shadow-sm min-h-[36px]"
            title="Copy command"
            aria-label="Copy installation command"
          >
            {copied ? (
              <>
                <Check className="w-3.5 h-3.5 text-cyan-400" />
                <span className="text-cyan-400 font-bold">Copied</span>
              </>
            ) : (
              <>
                <Copy className="w-3.5 h-3.5 text-slate-400" />
                <span>Copy</span>
              </>
            )}
          </button>
        </div>

        <div className="flex flex-col sm:flex-row sm:items-center justify-between text-xs text-slate-600 dark:text-slate-400 px-1 gap-2">
          <span className="leading-relaxed">{currentTab.description}</span>
          <div className="flex items-center gap-3 font-mono text-[11px] text-slate-500 dark:text-slate-400 shrink-0">
            <span className="flex items-center gap-1">
              <HardDrive className="w-3 h-3 text-[#0057FF] dark:text-blue-400" />
              Multi-Drive
            </span>
            <span className="flex items-center gap-1">
              <Shield className="w-3 h-3 text-[#00D1FF] dark:text-cyan-400" />
              SHA-256
            </span>
          </div>
        </div>
      </div>
    </div>
  );
};
