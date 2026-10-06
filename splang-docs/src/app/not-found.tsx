import React from "react";
import Link from "next/link";
import { ArrowLeft, BookOpen } from "lucide-react";

export default function DocsNotFound() {
  return (
    <div className="py-20 text-center">
      <div className="w-14 h-14 rounded-2xl bg-cyan-950/60 border border-cyan-800/80 flex items-center justify-center mx-auto mb-4 text-cyan-400">
        <BookOpen className="w-7 h-7" />
      </div>
      <span className="font-mono text-cyan-400 text-xs font-semibold">404 — Page Not Found</span>
      <h1 className="text-2xl font-bold text-white mt-1 mb-3">Documentation Topic Not Found</h1>
      <p className="text-xs text-slate-400 max-w-sm mx-auto mb-6">
        The documentation page you requested does not exist or has moved.
      </p>
      <Link
        href="/getting-started"
        className="inline-flex items-center gap-1.5 px-4 py-2 rounded-lg bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-semibold text-xs transition-colors"
      >
        <ArrowLeft className="w-3.5 h-3.5" />
        <span>Return to Docs Home</span>
      </Link>
    </div>
  );
}
