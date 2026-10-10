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
        <nav className="flex items-center gap-2 text-xs text-slate-400 font-mono">
          <Link href="/" className="hover:text-[#00D1FF] transition-colors">
            Registry
          </Link>
          <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
          <Link href="/packages" className="hover:text-[#00D1FF] transition-colors">
            Packages
          </Link>
          <ChevronRight className="w-3.5 h-3.5 text-slate-600" />
          <span className="text-[#00D1FF] font-semibold">{pkg.name}</span>
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
              <h3 className="text-xs font-bold uppercase tracking-wider text-slate-300 font-mono">
                Install Package
              </h3>
              <InstallCommand packageName={pkg.name} version={pkg.version} />
            </div>

            {/* Metadata Card */}
            <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 p-6 space-y-4 shadow-sm dark:shadow-xl">
              <h3 className="text-xs font-bold uppercase tracking-wider text-slate-900 dark:text-slate-200 font-mono pb-3 border-b border-slate-100 dark:border-[#0A1F44]">
                Package Details
              </h3>

              <div className="space-y-3 text-xs font-mono">
                <div className="flex items-center justify-between text-slate-500 dark:text-slate-400">
                  <span>Version:</span>
                  <span className="text-slate-900 dark:text-white font-semibold">v{pkg.version}</span>
                </div>

                <div className="flex items-center justify-between text-slate-500 dark:text-slate-400">
                  <span>License:</span>
                  <span className="text-slate-900 dark:text-white">{pkg.license}</span>
                </div>

                <div className="flex items-center justify-between text-slate-500 dark:text-slate-400">
                  <span>Total Downloads:</span>
                  <span className="text-[#0057FF] dark:text-[#00D1FF] font-bold">
                    {formatDownloads(pkg.totalDownloads)}
                  </span>
                </div>

                <div className="flex items-center justify-between text-slate-500 dark:text-slate-400">
                  <span>Published:</span>
                  <span className="text-slate-700 dark:text-slate-200">{formatDate(pkg.publishedAt)}</span>
                </div>

                <div className="flex items-center justify-between text-slate-500 dark:text-slate-400">
                  <span>SPM Min Version:</span>
                  <span className="text-slate-700 dark:text-slate-200">
                    &gt;= {pkg.versions[0]?.spmMinVersion || "1.0.0"}
                  </span>
                </div>

                <div className="flex items-center justify-between text-slate-500 dark:text-slate-400">
                  <span>Checksum:</span>
                  <span className="text-[#0057FF] dark:text-[#00D1FF] flex items-center gap-1 font-semibold">
                    <ShieldCheck className="w-3.5 h-3.5" />
                    <span>SHA-256</span>
                  </span>
                </div>
              </div>

              {/* Author & Repository */}
              <div className="pt-3 border-t border-slate-100 dark:border-[#0A1F44] space-y-2 text-xs">
                <div>
                  <span className="text-slate-500 dark:text-slate-400 font-mono block mb-1">Author:</span>
                  <span className="text-slate-800 dark:text-slate-200 break-words font-medium">{pkg.author}</span>
                </div>

                {pkg.repositoryUrl && (
                  <div className="pt-1">
                    <a
                      href={pkg.repositoryUrl}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="text-[#0057FF] dark:text-[#00D1FF] hover:underline flex items-center gap-1 font-mono text-xs"
                    >
                      <GitBranch className="w-3.5 h-3.5" />
                      <span className="truncate">{pkg.repositoryUrl}</span>
                      <ExternalLink className="w-3 h-3 shrink-0" />
                    </a>
                  </div>
                )}
              </div>

              {/* Keywords */}
              <div className="pt-3 border-t border-slate-100 dark:border-[#0A1F44] space-y-2">
                <span className="text-xs text-slate-500 dark:text-slate-400 font-mono block">Keywords:</span>
                <div className="flex flex-wrap gap-1.5">
                  {pkg.keywords.map((kw) => (
                    <Link
                      key={kw}
                      href={`/packages?q=${encodeURIComponent(kw)}`}
                      className="text-[11px] font-mono px-2 py-0.5 rounded-full bg-slate-100 dark:bg-[#020D21] border border-slate-200 dark:border-[#0A1F44] text-slate-600 dark:text-slate-300 hover:text-[#0057FF] dark:hover:text-[#00D1FF] hover:border-blue-300 dark:hover:border-[#00D1FF]/40 transition-colors"
                    >
                      #{kw}
                    </Link>
                  ))}
                </div>
              </div>
            </div>

            {/* Quick Command Reference */}
            <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/40 p-5 text-xs font-mono text-slate-500 dark:text-slate-400 space-y-2.5 shadow-sm dark:shadow-lg">
              <div className="flex items-center gap-2 text-slate-900 dark:text-white font-bold">
                <Terminal className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
                <span>Quick SPM command</span>
              </div>
              <p className="text-[11px] text-slate-500 dark:text-slate-400 leading-relaxed">
                Run inside any SP project directory:
              </p>
              <div className="p-2.5 bg-slate-900 dark:bg-[#020D21] rounded-xl border border-slate-800 dark:border-[#0A1F44] text-[#00D1FF] font-semibold select-all">
                spm add {pkg.name}
              </div>
            </div>
          </div>
        </div>
      </div>
    </div>
  );
}
