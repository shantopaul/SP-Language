"use client";

import React from "react";
import { Package } from "@/types/package";
import { CopyButton } from "@/components/ui/CopyButton";
import {
  ShieldCheck,
  Lock,
  FileCheck2,
  AlertCircle,
  Terminal,
  ExternalLink,
} from "lucide-react";

interface SecurityBadgeProps {
  pkg: Package;
}

export const SecurityBadge: React.FC<SecurityBadgeProps> = ({ pkg }) => {
  const latestVersion = pkg.versions[0];
  const checksum = latestVersion ? latestVersion.checksum : "";

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between pb-2 border-b border-slate-800">
        <h2 className="text-base font-semibold text-white font-mono flex items-center gap-2">
          <ShieldCheck className="w-5 h-5 text-emerald-400" />
          <span>Security & Integrity Analysis</span>
        </h2>
        <span className="text-xs font-mono text-emerald-400 bg-emerald-950/60 border border-emerald-800/80 px-2.5 py-0.5 rounded-full">
          Audit: PASSED
        </span>
      </div>

      {/* SHA-256 Card */}
      <div className="rounded-xl border border-slate-800 bg-slate-900/60 p-5 space-y-3">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Lock className="w-4 h-4 text-cyan-400" />
            <span className="text-xs font-semibold uppercase tracking-wider text-slate-200">
              Cryptographic Checksum (SHA-256)
            </span>
          </div>
          <span className="text-xs font-mono text-slate-400">
            v{latestVersion?.version}
          </span>
        </div>

        <div className="flex items-center justify-between gap-3 p-3 bg-slate-950 rounded-lg border border-slate-800">
          <code className="text-xs font-mono text-cyan-300 break-all select-all">
            {checksum}
          </code>
          <CopyButton text={checksum} label="Copy" className="shrink-0 h-7 text-xs px-2" />
        </div>

        <p className="text-xs text-slate-400 leading-relaxed">
          The SP Package Manager (<code className="text-cyan-400">spm</code>) automatically verifies this digest against <code className="text-cyan-400">sp.lock</code> before uncompressing any archive. If the digest differs by even a single bit, installation is immediately aborted.
        </p>
      </div>

      {/* Verification Instructions */}
      <div className="rounded-xl border border-slate-800 bg-slate-900/60 p-5 space-y-3">
        <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-200 flex items-center gap-2">
          <Terminal className="w-4 h-4 text-cyan-400" />
          <span>Verify Integrity via CLI</span>
        </h3>
        <div className="p-3 bg-slate-950 rounded-lg border border-slate-800 font-mono text-xs text-slate-200 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <span className="text-cyan-400">$</span>
            <span>spm verify {pkg.name}</span>
          </div>
          <CopyButton text={`spm verify ${pkg.name}`} label="Copy" className="h-7 text-xs px-2" />
        </div>
      </div>

      {/* Security Features List */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-950/60">
          <FileCheck2 className="w-5 h-5 text-emerald-400 mb-2" />
          <h4 className="text-xs font-bold text-white mb-1">Immutable Tarballs</h4>
          <p className="text-[11px] text-slate-400 leading-relaxed">
            Published versions can never be rewritten or modified. Once released, the SHA-256 hash is permanent.
          </p>
        </div>

        <div className="p-4 rounded-xl border border-slate-800 bg-slate-950/60">
          <ShieldCheck className="w-5 h-5 text-cyan-400 mb-2" />
          <h4 className="text-xs font-bold text-white mb-1">Sandboxed Builds</h4>
          <p className="text-[11px] text-slate-400 leading-relaxed">
            SP packages have no pre/post-install arbitrary shell hooks, eliminating common npm-style supply chain attacks.
          </p>
        </div>

        <div className="p-4 rounded-xl border border-slate-800 bg-slate-950/60">
          <Lock className="w-5 h-5 text-indigo-400 mb-2" />
          <h4 className="text-xs font-bold text-white mb-1">Strict Lockfile</h4>
          <p className="text-[11px] text-slate-400 leading-relaxed">
            All transient sub-dependencies are deterministically locked with exact hashes in <code className="text-cyan-400">sp.lock</code>.
          </p>
        </div>
      </div>
    </div>
  );
};
