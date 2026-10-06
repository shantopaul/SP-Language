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
      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-8 text-center">
        <div className="w-12 h-12 rounded-full bg-emerald-950/60 border border-emerald-800/80 text-emerald-400 flex items-center justify-center mx-auto mb-3">
          <CheckCircle2 className="w-6 h-6" />
        </div>
        <h3 className="text-base font-semibold text-white font-mono mb-1">
          Zero External Dependencies
        </h3>
        <p className="text-xs text-slate-400 max-w-md mx-auto">
          This package is self-contained and does not rely on any third-party dependencies, minimizing build times and supply chain attack vectors.
        </p>
      </div>
    );
  }

  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between pb-2 border-b border-slate-800">
        <h2 className="text-base font-semibold text-white font-mono flex items-center gap-2">
          <Layers className="w-4 h-4 text-cyan-400" />
          <span>Dependencies ({dependencies.length})</span>
        </h2>
        <span className="text-xs text-slate-400">
          Resolved automatically by <code className="text-cyan-400">spm</code>
        </span>
      </div>

      <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
        {dependencies.map((dep) => (
          <div
            key={dep.name}
            className="flex items-center justify-between p-3.5 rounded-xl border border-slate-800 bg-slate-900/60 hover:border-slate-700 transition-colors"
          >
            <div className="flex items-center gap-3">
              <div className="w-8 h-8 rounded-lg bg-slate-800 border border-slate-700 flex items-center justify-center text-slate-300">
                <Box className="w-4 h-4 text-cyan-400" />
              </div>
              <div>
                <Link
                  href={`/packages/${dep.name}`}
                  className="text-sm font-semibold text-white hover:text-cyan-400 font-mono transition-colors flex items-center gap-1"
                >
                  <span>{dep.name}</span>
                  <ArrowUpRight className="w-3 h-3 text-slate-400" />
                </Link>
                <span className="text-xs font-mono text-slate-400">
                  Requirement: <span className="text-cyan-400">{dep.req}</span>
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
