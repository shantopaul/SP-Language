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
    <div className="border-b border-slate-800 bg-slate-900/40 pb-8 pt-6">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex flex-col md:flex-row md:items-center justify-between gap-4">
          <div>
            <div className="flex items-center gap-3 flex-wrap">
              <h1 className="text-3xl font-extrabold text-white font-mono tracking-tight">
                {pkg.name}
              </h1>
              <span className="text-sm font-mono text-cyan-400 bg-cyan-950/60 border border-cyan-800/60 px-2.5 py-0.5 rounded-full">
                v{pkg.version}
              </span>
              {pkg.official && (
                <Badge variant="cyan" className="text-xs">
                  Official Ecosystem Package
                </Badge>
              )}
            </div>
            <p className="mt-3 text-base text-slate-300 max-w-3xl leading-relaxed">
              {pkg.description}
            </p>
          </div>

          <div className="flex items-center gap-3">
            {pkg.repositoryUrl && (
              <a
                href={pkg.repositoryUrl}
                target="_blank"
                rel="noopener noreferrer"
                className="flex items-center gap-1.5 px-3.5 py-2 rounded-lg border border-slate-700 bg-slate-800 hover:bg-slate-700 text-slate-200 text-xs font-medium transition-colors"
              >
                <GitBranch className="w-3.5 h-3.5 text-cyan-400" />
                <span>Repository</span>
                <ExternalLink className="w-3 h-3 text-slate-400" />
              </a>
            )}
          </div>
        </div>

        {/* Quick Stats Bar */}
        <div className="mt-6 flex flex-wrap items-center gap-6 text-xs text-slate-400 pt-4 border-t border-slate-800/60 font-mono">
          <div className="flex items-center gap-1.5">
            <Download className="w-4 h-4 text-cyan-400" />
            <span className="text-slate-200 font-semibold">
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
            <span className="text-cyan-400 capitalize">{pkg.category}</span>
          </div>
        </div>
      </div>
    </div>
  );
};
