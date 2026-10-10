"use client";

import React from "react";
import Link from "next/link";
import { Package } from "@/types/package";
import { Badge } from "@/components/ui/Badge";
import { formatDownloads, formatDate } from "@/lib/utils";
import { Download, Calendar, ShieldCheck, Tag, ExternalLink } from "lucide-react";
import { CopyButton } from "@/components/ui/CopyButton";

interface PackageCardProps {
  pkg: Package;
}

export const PackageCard: React.FC<PackageCardProps> = ({ pkg }) => {
  return (
    <div className="group relative rounded-2xl border border-[#0A1F44] bg-[#081735]/70 hover:bg-[#081735] hover:border-[#0057FF]/60 hover:shadow-[0_0_25px_rgba(0,87,255,0.2)] transition-all duration-300 p-5 flex flex-col justify-between shadow-lg">
      <div>
        {/* Top Header */}
        <div className="flex items-start justify-between gap-3 mb-2">
          <div className="flex items-center gap-2 flex-wrap">
            <Link
              href={`/packages/${pkg.name}`}
              className="text-base font-bold text-white group-hover:text-[#00D1FF] transition-colors font-mono tracking-tight"
            >
              {pkg.name}
            </Link>
            <span className="text-xs font-mono text-[#00D1FF] bg-[#0057FF]/15 px-2 py-0.5 rounded-md border border-[#00D1FF]/30">
              v{pkg.version}
            </span>
            {pkg.official && (
              <Badge variant="cyan" className="text-[10px] py-0 font-semibold">
                Official
              </Badge>
            )}
          </div>

          <div className="flex items-center gap-1.5 opacity-90 group-hover:opacity-100 transition-opacity">
            <CopyButton
              text={`spm add ${pkg.name}`}
              label="spm add"
              className="text-[11px] py-1 px-2 h-6 font-mono bg-[#020D21]/80 hover:bg-[#0057FF]/20 border border-[#0A1F44] hover:border-[#00D1FF]/40"
            />
          </div>
        </div>

        {/* Description */}
        <p className="text-xs text-slate-300 line-clamp-2 leading-relaxed mb-4">
          {pkg.description}
        </p>

        {/* Tags */}
        <div className="flex items-center gap-1.5 flex-wrap mb-4">
          <span className="text-[11px] font-mono text-[#00D1FF] bg-[#0057FF]/15 border border-[#00D1FF]/30 px-2 py-0.5 rounded-full capitalize">
            {pkg.category}
          </span>
          {pkg.keywords.slice(0, 3).map((kw) => (
            <span
              key={kw}
              className="text-[10px] font-mono text-slate-400 bg-[#020D21]/60 border border-[#0A1F44] px-2 py-0.5 rounded-full"
            >
              #{kw}
            </span>
          ))}
        </div>
      </div>

      {/* Meta Footer */}
      <div className="pt-3 border-t border-[#0A1F44] flex items-center justify-between text-[11px] text-slate-400 font-mono">
        <div className="flex items-center gap-3">
          <span className="flex items-center gap-1 text-slate-300" title="Total Downloads">
            <Download className="w-3.5 h-3.5 text-[#00D1FF]" />
            <span>{formatDownloads(pkg.totalDownloads)}</span>
          </span>
          <span className="flex items-center gap-1" title="License">
            <span>{pkg.license}</span>
          </span>
        </div>
        <div className="flex items-center gap-1 text-slate-400">
          <Calendar className="w-3 h-3 text-slate-400" />
          <span>{formatDate(pkg.publishedAt)}</span>
        </div>
      </div>
    </div>
  );
};
