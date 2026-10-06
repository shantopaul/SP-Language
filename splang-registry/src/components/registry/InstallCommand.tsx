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
    <div className="rounded-xl border border-slate-800 bg-slate-900/90 overflow-hidden shadow-lg">
      <div className="flex items-center justify-between px-3 py-2 border-b border-slate-800/80 bg-slate-950/60">
        <div className="flex items-center gap-1">
          <button
            onClick={() => setTab("cli")}
            className={`flex items-center gap-1.5 px-2.5 py-1 rounded-md text-xs font-medium transition-colors ${
              tab === "cli"
                ? "bg-cyan-500/10 text-cyan-400 border border-cyan-500/30"
                : "text-slate-400 hover:text-slate-200"
            }`}
          >
            <Terminal className="w-3.5 h-3.5" />
            <span>CLI Command</span>
          </button>
          <button
            onClick={() => setTab("manifest")}
            className={`flex items-center gap-1.5 px-2.5 py-1 rounded-md text-xs font-medium transition-colors ${
              tab === "manifest"
                ? "bg-cyan-500/10 text-cyan-400 border border-cyan-500/30"
                : "text-slate-400 hover:text-slate-200"
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

      <div className="p-3 bg-slate-950/80 font-mono text-xs text-slate-200 overflow-x-auto select-all">
        {tab === "cli" ? (
          <div className="flex items-center gap-2">
            <span className="text-cyan-400 select-none">$</span>
            <span className="text-slate-100">{cliCommand}</span>
          </div>
        ) : (
          <pre className="text-slate-300 font-mono leading-relaxed whitespace-pre">
            {manifestSnippet}
          </pre>
        )}
      </div>
    </div>
  );
};
