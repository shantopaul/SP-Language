"use client";

import React from "react";
import Link from "next/link";
import { PackageDependency } from "@/types/package";
import { Box, Layers, ArrowUpRight, CheckCircle2 } from "lucide-react";

interface DependencyTreeProps {
  dependencies: PackageDependency[];
}

export const DependencyTree: React.FC<DependencyTreeProps> = ({ dependencies }) => {
  if (!dependencies || dependencies.length === 0) {
    return (
      <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/40 p-8 text-center shadow-lg">
        <div className="w-12 h-12 rounded-full bg-[#0057FF]/15 border border-[#00D1FF]/40 text-[#00D1FF] flex items-center justify-center mx-auto mb-3 shadow-[0_0_15px_rgba(0,209,255,0.2)]">
          <CheckCircle2 className="w-6 h-6" />
        </div>
        <h3 className="text-base font-bold text-white font-mono mb-1">
          Zero External Dependencies
        </h3>
        <p className="text-xs text-slate-400 max-w-md mx-auto leading-relaxed">
          This package is self-contained and does not rely on any third-party dependencies, minimizing build times and supply chain attack vectors.
        </p>
      </div>
    );
  }

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between pb-3 border-b border-[#0A1F44]">
        <h2 className="text-base font-bold text-white font-mono flex items-center gap-2">
          <Layers className="w-4 h-4 text-[#00D1FF]" />
          <span>Dependencies ({dependencies.length})</span>
        </h2>
        <span className="text-xs text-slate-400 font-mono">
          Resolved automatically by <code className="text-[#00D1FF]">spm</code>
        </span>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
        {dependencies.map((dep) => (
          <div
            key={dep.name}
            className="flex items-center justify-between p-3.5 rounded-xl border border-[#0A1F44] bg-[#081735]/60 hover:border-[#0057FF]/50 transition-all shadow-md"
          >
            <div className="flex items-center gap-3">
              <div className="w-8 h-8 rounded-lg bg-[#020D21] border border-[#0A1F44] flex items-center justify-center text-slate-300">
                <Box className="w-4 h-4 text-[#00D1FF]" />
              </div>
              <div>
                <Link
                  href={`/packages/${dep.name}`}
                  className="text-sm font-bold text-white hover:text-[#00D1FF] font-mono transition-colors flex items-center gap-1"
                >
                  <span>{dep.name}</span>
                  <ArrowUpRight className="w-3 h-3 text-slate-400" />
                </Link>
                <span className="text-xs font-mono text-slate-400">
                  Requirement: <span className="text-[#00D1FF]">{dep.req}</span>
                </span>
              </div>
            </div>

            {dep.optional && (
              <span className="text-[10px] font-mono text-amber-400 bg-amber-950/60 border border-amber-800/60 px-2 py-0.5 rounded">
                Optional
              </span>
            )}
          </div>
        ))}
      </div>
    </div>
  );
};
