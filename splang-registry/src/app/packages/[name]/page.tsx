import React from "react";
import { notFound } from "next/navigation";
import { Metadata } from "next";
import Link from "next/link";
import { PACKAGES, getPackageByName } from "@/data/packages-db";
import { PackageHeader } from "@/components/registry/PackageHeader";
import { InstallCommand } from "@/components/registry/InstallCommand";
import { ReadmeViewer } from "@/components/registry/ReadmeViewer";
import { VersionHistory } from "@/components/registry/VersionHistory";
import { DependencyTree } from "@/components/registry/DependencyTree";
import { SecurityBadge } from "@/components/registry/SecurityBadge";
import { TabsContainer } from "./TabsContainer";
import {
  Download,
  Calendar,
  Scale,
  User,
  ShieldCheck,
  Tag,
  GitBranch,
  ExternalLink,
  ChevronRight,
  Terminal,
} from "lucide-react";
import { formatDownloads, formatDate } from "@/lib/utils";

interface PageProps {
  params: {
    name: string;
  };
}

export function generateStaticParams() {
  return PACKAGES.map((pkg) => ({
    name: pkg.name,
  }));
}

export function generateMetadata({ params }: PageProps): Metadata {
  const pkg = getPackageByName(params.name);
  if (!pkg) {
    return {
      title: "Package Not Found",
    };
  }

  return {
    title: `${pkg.name} v${pkg.version} — SP Package Registry`,
    description: pkg.description,
    openGraph: {
      title: `${pkg.name} v${pkg.version}`,
      description: pkg.description,
    },
  };
}

export default function PackageDetailPage({ params }: PageProps) {
  const pkg = getPackageByName(params.name);

  if (!pkg) {
    notFound();
  }

  return (
    <div className="space-y-6 pb-16">
      {/* Breadcrumb Bar */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 pt-4">
        <nav className="flex items-center gap-1.5 text-xs text-slate-400 font-mono">
          <Link href="/" className="hover:text-cyan-400">
            Registry
          </Link>
          <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
          <Link href="/packages" className="hover:text-cyan-400">
            Packages
          </Link>
          <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
          <span className="text-white">{pkg.name}</span>
        </nav>
      </div>

      {/* Package Header */}
      <PackageHeader pkg={pkg} />

      {/* Main Grid: Content + Sidebar */}
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="grid grid-cols-1 lg:grid-cols-3 gap-8">
          {/* Main Tabbed Content (2 cols) */}
          <div className="lg:col-span-2 space-y-6">
            <TabsContainer
              readmeContent={<ReadmeViewer content={pkg.readme} />}
              versionsContent={
                <VersionHistory packageName={pkg.name} versions={pkg.versions} />
              }
              dependenciesContent={
                <DependencyTree dependencies={pkg.dependencies} />
              }
              securityContent={<SecurityBadge pkg={pkg} />}
            />
          </div>

          {/* Sidebar (1 col) */}
          <div className="space-y-6">
            {/* Install Command Box */}
            <div className="space-y-2">
              <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-300 font-mono">
                Install Package
              </h3>
              <InstallCommand packageName={pkg.name} version={pkg.version} />
            </div>

            {/* Metadata Card */}
            <div className="rounded-xl border border-slate-800 bg-slate-900/60 p-5 space-y-4">
              <h3 className="text-xs font-semibold uppercase tracking-wider text-slate-300 font-mono pb-2 border-b border-slate-800/80">
                Package Details
              </h3>

              <div className="space-y-3 text-xs font-mono">
                <div className="flex items-center justify-between text-slate-400">
                  <span>Version:</span>
                  <span className="text-white font-semibold">v{pkg.version}</span>
                </div>

                <div className="flex items-center justify-between text-slate-400">
                  <span>License:</span>
                  <span className="text-white">{pkg.license}</span>
                </div>

                <div className="flex items-center justify-between text-slate-400">
                  <span>Total Downloads:</span>
                  <span className="text-cyan-400 font-semibold">
                    {formatDownloads(pkg.totalDownloads)}
                  </span>
                </div>

                <div className="flex items-center justify-between text-slate-400">
                  <span>Published:</span>
                  <span className="text-slate-200">{formatDate(pkg.publishedAt)}</span>
                </div>

                <div className="flex items-center justify-between text-slate-400">
                  <span>SPM Min Version:</span>
                  <span className="text-slate-200">
                    &gt;= {pkg.versions[0]?.spmMinVersion || "1.0.0"}
                  </span>
                </div>

                <div className="flex items-center justify-between text-slate-400">
                  <span>Checksum:</span>
                  <span className="text-emerald-400 flex items-center gap-1">
                    <ShieldCheck className="w-3.5 h-3.5" />
                    <span>SHA-256</span>
                  </span>
                </div>
              </div>

              {/* Author & Repository */}
              <div className="pt-3 border-t border-slate-800/80 space-y-2 text-xs">
                <div>
                  <span className="text-slate-400 font-mono block mb-1">Author:</span>
                  <span className="text-slate-200 break-words">{pkg.author}</span>
                </div>

                {pkg.repositoryUrl && (
                  <div className="pt-1">
                    <a
                      href={pkg.repositoryUrl}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="text-cyan-400 hover:text-cyan-300 flex items-center gap-1 font-mono text-xs"
                    >
                      <GitBranch className="w-3.5 h-3.5" />
                      <span className="truncate">{pkg.repositoryUrl}</span>
                      <ExternalLink className="w-3 h-3 shrink-0" />
                    </a>
                  </div>
                )}
              </div>

              {/* Keywords */}
              <div className="pt-3 border-t border-slate-800/80 space-y-2">
                <span className="text-xs text-slate-400 font-mono block">Keywords:</span>
                <div className="flex flex-wrap gap-1.5">
                  {pkg.keywords.map((kw) => (
                    <Link
                      key={kw}
                      href={`/packages?q=${encodeURIComponent(kw)}`}
                      className="text-[11px] font-mono px-2 py-0.5 rounded bg-slate-800/80 border border-slate-700/60 text-slate-300 hover:text-cyan-400 hover:border-cyan-500/40 transition-colors"
                    >
                      #{kw}
                    </Link>
                  ))}
                </div>
              </div>
            </div>

            {/* Quick Command Reference */}
            <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-4 text-xs font-mono text-slate-400 space-y-2">
              <div className="flex items-center gap-1.5 text-slate-300 font-semibold">
                <Terminal className="w-3.5 h-3.5 text-cyan-400" />
                <span>Quick SPM command</span>
              </div>
              <p className="text-[11px] text-slate-400 leading-relaxed">
                Run inside any SP project directory:
              </p>
              <div className="p-2 bg-slate-950 rounded border border-slate-800/80 text-cyan-300 select-all">
                spm add {pkg.name}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
