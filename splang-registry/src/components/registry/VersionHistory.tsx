"use client";

import React from "react";
import { PackageVersion } from "@/types/package";
import { formatDate } from "@/lib/utils";
import { CopyButton } from "@/components/ui/CopyButton";
import { ShieldCheck, HardDrive, Calendar, Terminal } from "lucide-react";

interface VersionHistoryProps {
  packageName: string;
  versions: PackageVersion[];
}

export const VersionHistory: React.FC<VersionHistoryProps> = ({
  packageName,
  versions,
}) => {
  return (
    <div className="space-y-4">
      <div className="flex items-center justify-between pb-3 border-b border-[#0A1F44]">
        <h2 className="text-base font-bold text-white font-mono">
          Release History ({versions.length} versions)
        </h2>
        <span className="text-xs text-slate-400 font-mono">
          All releases are cryptographically signed &amp; immutable
        </span>
      </div>

      <div className="divide-y divide-[#0A1F44] border border-[#0A1F44] rounded-2xl bg-[#081735]/40 overflow-hidden shadow-xl">
        {versions.map((ver) => (
          <div key={ver.version} className="p-4 sm:p-5 hover:bg-[#081735]/80 transition-colors">
            <div className="flex flex-col sm:flex-row sm:items-center justify-between gap-3 mb-2">
              <div className="flex items-center gap-2.5">
                <span className="text-base font-bold text-white font-mono">
                  v{ver.version}
                </span>
                <span className="text-[11px] font-mono text-[#00D1FF] bg-[#0057FF]/15 border border-[#00D1FF]/30 px-2.5 py-0.5 rounded-full">
                  spm &gt;= {ver.spmMinVersion}
                </span>
              </div>

              <div className="flex items-center gap-4 text-xs text-slate-400 font-mono">
                <span className="flex items-center gap-1">
                  <HardDrive className="w-3.5 h-3.5 text-slate-400" />
                  <span>{ver.size}</span>
                </span>
                <span className="flex items-center gap-1">
                  <Calendar className="w-3.5 h-3.5 text-slate-400" />
                  <span>{formatDate(ver.publishedAt)}</span>
                </span>
              </div>
            </div>

            {ver.changelog && (
              <p className="text-xs text-slate-300 my-2 leading-relaxed pl-1">
                {ver.changelog}
              </p>
            )}

            {/* Checksum row */}
            <div className="mt-3 pt-3 border-t border-[#0A1F44]/50 flex flex-col sm:flex-row sm:items-center justify-between gap-2 text-[11px] font-mono text-slate-400">
              <div className="flex items-center gap-1.5 overflow-hidden">
                <ShieldCheck className="w-3.5 h-3.5 text-[#00D1FF] shrink-0" />
                <span className="text-slate-400">SHA-256:</span>
                <span className="text-slate-300 truncate max-w-xs sm:max-w-md">
                  {ver.checksum}
                </span>
              </div>

              <div className="flex items-center gap-2 self-end sm:self-auto">
                <CopyButton
                  text={ver.checksum}
                  label="Copy Hash"
                  className="h-6 text-[10px] px-2"
                />
                <CopyButton
                  text={`spm add ${packageName}@${ver.version}`}
                  label="Copy spm add"
                  className="h-6 text-[10px] px-2 bg-[#0057FF]/15 text-[#00D1FF] hover:bg-[#0057FF]/30 border border-[#00D1FF]/40"
                />
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
};
