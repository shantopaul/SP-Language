"use client";

import React, { useState } from "react";
import { Terminal, FileCode, Check, Copy } from "lucide-react";
import { CopyButton } from "@/components/ui/CopyButton";

interface InstallCommandProps {
  packageName: string;
  version: string;
}

export const InstallCommand: React.FC<InstallCommandProps> = ({
  packageName,
  version,
}) => {
  const [tab, setTab] = useState<"cli" | "manifest">("cli");

  const cliCommand = `spm add ${packageName}`;
  const manifestSnippet = `[dependencies]\n${packageName} = "^${version}"`;

  return (
    <div className="rounded-2xl border border-slate-300 dark:border-[#0A1F44] bg-[#081735] overflow-hidden shadow-sm dark:shadow-xl">
      <div className="flex items-center justify-between px-3 py-2 border-b border-slate-700 dark:border-[#0A1F44] bg-[#020D21]/90">
        <div className="flex items-center gap-1">
          <button
            onClick={() => setTab("cli")}
            className={`flex items-center gap-1.5 px-2.5 py-1 rounded-lg text-xs font-medium transition-all ${
              tab === "cli"
                ? "bg-[#0057FF]/30 text-[#60A5FA] dark:text-[#00D1FF] border border-blue-400/40 dark:border-[#00D1FF]/40 shadow-none dark:shadow-[0_0_10px_rgba(0,209,255,0.15)]"
                : "text-slate-400 hover:text-white"
            }`}
          >
            <Terminal className="w-3.5 h-3.5" />
            <span>CLI Command</span>
          </button>
          <button
            onClick={() => setTab("manifest")}
            className={`flex items-center gap-1.5 px-2.5 py-1 rounded-lg text-xs font-medium transition-all ${
              tab === "manifest"
                ? "bg-[#0057FF]/30 text-[#60A5FA] dark:text-[#00D1FF] border border-blue-400/40 dark:border-[#00D1FF]/40 shadow-none dark:shadow-[0_0_10px_rgba(0,209,255,0.15)]"
                : "text-slate-400 hover:text-white"
            }`}
          >
            <FileCode className="w-3.5 h-3.5" />
            <span>sp.toml</span>
          </button>
        </div>
        <CopyButton
          text={tab === "cli" ? cliCommand : manifestSnippet}
          label="Copy"
          className="h-7 text-xs px-2"
        />
      </div>

      <div className="p-3.5 bg-[#020D21] font-mono text-xs text-slate-200 overflow-x-auto select-all">
        {tab === "cli" ? (
          <div className="flex items-center gap-2">
            <span className="text-[#00D1FF] select-none font-bold">$</span>
            <span className="text-white font-semibold">{cliCommand}</span>
          </div>
        ) : (
          <pre className="text-slate-200 font-mono leading-relaxed whitespace-pre">
            {manifestSnippet}
          </pre>
        )}
      </div>
    </div>
  );
};
