"use client";

import React from "react";
import { Package } from "@/types/package";
import { Badge } from "@/components/ui/Badge";
import { formatDownloads, formatDate } from "@/lib/utils";
import {
  Download,
  Calendar,
  ShieldCheck,
  User,
  Scale,
  GitBranch,
  ExternalLink,
  Tag,
} from "lucide-react";

interface PackageHeaderProps {
  pkg: Package;
}

export const PackageHeader: React.FC<PackageHeaderProps> = ({ pkg }) => {
  return (
    <div className="border-b border-[#0A1F44] bg-[#020D21]/90 pb-8 pt-6">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-3 flex-wrap">
              <h1 className="text-2xl sm:text-4xl font-black text-white font-mono tracking-tight break-all sm:break-normal">
                {pkg.name}
              </h1>
              <span className="text-sm font-mono text-[#00D1FF] bg-[#0057FF]/15 border border-[#00D1FF]/40 px-3 py-0.5 rounded-full shadow-[0_0_12px_rgba(0,209,255,0.2)] font-semibold">
                v{pkg.version}
              </span>
              {pkg.official && (
                <Badge variant="cyan" className="text-xs font-semibold">
                  Official Ecosystem Package
                </Badge>
              )}
            </div>
            <p className="mt-3 text-sm sm:text-base text-slate-300 max-w-3xl leading-relaxed">
              {pkg.description}
            </p>
          </div>

          <div className="flex items-center gap-3 w-full sm:w-auto">
            {pkg.repositoryUrl && (
              <a
                href={pkg.repositoryUrl}
                target="_blank"
                rel="noopener noreferrer"
                className="w-full sm:w-auto justify-center min-h-[40px] flex items-center gap-2 px-4 py-2 rounded-xl border border-[#0A1F44] bg-[#081735] hover:bg-[#0057FF]/15 hover:border-[#00D1FF]/40 text-slate-200 hover:text-white text-xs font-semibold transition-all shadow-md"
              >
                <GitBranch className="w-3.5 h-3.5 text-[#00D1FF]" />
                <span>Repository</span>
                <ExternalLink className="w-3 h-3 text-slate-400" />
              </a>
            )}
          </div>
        </div>

        {/* Quick Stats Bar */}
        <div className="mt-6 flex flex-wrap items-center gap-x-6 gap-y-2.5 text-xs text-slate-400 pt-4 border-t border-[#0A1F44] font-mono">
          <div className="flex items-center gap-1.5">
            <Download className="w-4 h-4 text-[#00D1FF]" />
            <span className="text-white font-bold">
              {formatDownloads(pkg.totalDownloads)}
            </span>
            <span>total downloads</span>
          </div>

          <div className="flex items-center gap-1.5">
            <Scale className="w-4 h-4 text-slate-400" />
            <span>License:</span>
            <span className="text-slate-200">{pkg.license}</span>
          </div>

          <div className="flex items-center gap-1.5">
            <User className="w-4 h-4 text-slate-400" />
            <span>Author:</span>
            <span className="text-slate-200">{pkg.author}</span>
          </div>

          <div className="flex items-center gap-1.5">
            <Calendar className="w-4 h-4 text-slate-400" />
            <span>Published:</span>
            <span className="text-slate-200">{formatDate(pkg.publishedAt)}</span>
          </div>

          <div className="flex items-center gap-1.5">
            <Tag className="w-4 h-4 text-slate-400" />
            <span>Category:</span>
            <span className="text-[#00D1FF] capitalize font-semibold">{pkg.category}</span>
          </div>
        </div>
      </div>
    </div>
  );
};
