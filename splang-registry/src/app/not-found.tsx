import React from "react";
import Link from "next/link";
import { PackageX, ArrowLeft, Search } from "lucide-react";

export default function NotFound() {
  return (
    <div className="max-w-2xl mx-auto px-4 py-24 text-center space-y-6">
      <div className="w-16 h-16 rounded-2xl bg-cyan-500/10 border border-cyan-500/20 text-cyan-400 flex items-center justify-center mx-auto">
        <PackageX className="w-8 h-8" />
      </div>

      <div className="space-y-2">
        <span className="text-xs font-mono uppercase tracking-widest text-cyan-400">
          404 — Package or Resource Not Found
        </span>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Page Does Not Exist
        </h1>
        <p className="text-sm text-slate-400 max-w-md mx-auto leading-relaxed">
          The package or page you are looking for has not been published or may have been relocated.
        </p>
      </div>

      <div className="flex items-center justify-center gap-3 pt-4">
        <Link
          href="/"
          className="flex items-center gap-1.5 px-4 py-2.5 rounded-lg border border-slate-800 bg-slate-900 hover:bg-slate-800 text-slate-200 text-xs font-medium transition-colors"
        >
          <ArrowLeft className="w-3.5 h-3.5" />
          <span>Return Home</span>
        </Link>
        <Link
          href="/packages"
          className="flex items-center gap-1.5 px-4 py-2.5 rounded-lg bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-semibold text-xs transition-colors shadow-md shadow-cyan-500/20"
        >
          <Search className="w-3.5 h-3.5" />
          <span>Browse All Packages</span>
        </Link>
      </div>
    </div>
  );
}
