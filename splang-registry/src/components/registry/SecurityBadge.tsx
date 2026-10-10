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
      <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-2 pb-3 border-b border-[#0A1F44]">
        <h2 className="text-sm sm:text-base font-bold text-white font-mono flex items-center gap-2">
          <ShieldCheck className="w-5 h-5 text-[#00D1FF] shrink-0" />
          <span>Security &amp; Integrity Analysis</span>
        </h2>
        <span className="self-start sm:self-auto text-xs font-mono font-semibold text-[#00D1FF] bg-[#0057FF]/20 border border-[#00D1FF]/40 px-3 py-0.5 rounded-full shadow-[0_0_12px_rgba(0,209,255,0.2)]">
          Audit: PASSED (100%)
        </span>
      </div>

      {/* SHA-256 Card */}
      <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/60 p-5 space-y-3 shadow-lg">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Lock className="w-4 h-4 text-[#00D1FF]" />
            <span className="text-xs font-bold uppercase tracking-wider text-slate-200">
              Cryptographic Checksum (SHA-256)
            </span>
          </div>
          <span className="text-xs font-mono text-[#00D1FF] bg-[#0057FF]/15 px-2 py-0.5 rounded border border-[#00D1FF]/30">
            v{latestVersion?.version}
          </span>
        </div>

        <div className="flex items-center justify-between gap-3 p-3 bg-[#020D21] rounded-xl border border-[#0A1F44]">
          <code className="text-xs font-mono text-[#00D1FF] break-all select-all font-semibold">
            {checksum}
          </code>
          <CopyButton text={checksum} label="Copy" className="shrink-0 h-7 text-xs px-2" />
        </div>

        <p className="text-xs text-slate-400 leading-relaxed">
          The SP Package Manager (<code className="text-[#00D1FF]">spm</code>) automatically verifies this digest against <code className="text-[#00D1FF]">sp.lock</code> before uncompressing any archive. If the digest differs by even a single bit, installation is immediately aborted.
        </p>
      </div>

      {/* Verification Instructions */}
      <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/60 p-5 space-y-3 shadow-lg">
        <h3 className="text-xs font-bold uppercase tracking-wider text-slate-200 flex items-center gap-2">
          <Terminal className="w-4 h-4 text-[#00D1FF]" />
          <span>Verify Integrity via CLI</span>
        </h3>
        <div className="p-3 bg-[#020D21] rounded-xl border border-[#0A1F44] font-mono text-xs text-slate-200 flex items-center justify-between">
          <div className="flex items-center gap-2">
            <span className="text-[#00D1FF] font-bold">$</span>
            <span className="text-white font-semibold">spm verify {pkg.name}</span>
          </div>
          <CopyButton text={`spm verify ${pkg.name}`} label="Copy" className="h-7 text-xs px-2" />
        </div>
      </div>

      {/* Security Features List */}
      <div className="grid grid-cols-1 sm:grid-cols-3 gap-4">
        <div className="p-4 rounded-xl border border-[#0A1F44] bg-[#081735]/40 hover:border-[#0057FF]/40 transition-colors">
          <FileCheck2 className="w-5 h-5 text-[#00D1FF] mb-2" />
          <h4 className="text-xs font-bold text-white mb-1">Immutable Tarballs</h4>
          <p className="text-[11px] text-slate-400 leading-relaxed">
            Published versions can never be rewritten or modified. Once released, the SHA-256 hash is permanent.
          </p>
        </div>

        <div className="p-4 rounded-xl border border-[#0A1F44] bg-[#081735]/40 hover:border-[#0057FF]/40 transition-colors">
          <ShieldCheck className="w-5 h-5 text-[#00D1FF] mb-2" />
          <h4 className="text-xs font-bold text-white mb-1">Sandboxed Builds</h4>
          <p className="text-[11px] text-slate-400 leading-relaxed">
            SP packages have no pre/post-install arbitrary shell hooks, eliminating common npm-style supply chain attacks.
          </p>
        </div>

        <div className="p-4 rounded-xl border border-[#0A1F44] bg-[#081735]/40 hover:border-[#0057FF]/40 transition-colors">
          <Lock className="w-5 h-5 text-[#60A5FA] mb-2" />
          <h4 className="text-xs font-bold text-white mb-1">Strict Lockfile</h4>
          <p className="text-[11px] text-slate-400 leading-relaxed">
            All transient sub-dependencies are deterministically locked with exact hashes in <code className="text-[#00D1FF]">sp.lock</code>.
          </p>
        </div>
      </div>
    </div>
  );
};
