import React from "react";
import Link from "next/link";
import { ArrowLeft, BookOpen } from "lucide-react";

export default function DocsNotFound() {
  return (
    <div className="py-20 text-center relative">
      <div className="w-16 h-16 rounded-2xl bg-[#0057FF]/15 border border-[#00D1FF]/40 flex items-center justify-center mx-auto mb-4 text-[#00D1FF] shadow-[0_0_20px_rgba(0,209,255,0.25)]">
        <BookOpen className="w-8 h-8" />
      </div>
      <span className="font-mono text-[#00D1FF] text-xs font-bold uppercase tracking-wider px-3 py-1 rounded-full bg-[#0057FF]/20 border border-[#00D1FF]/30 inline-block mb-3">
        404 — Topic Not Found
      </span>
      <h1 className="text-3xl font-black text-white mt-1 mb-3">Documentation Page Not Found</h1>
      <p className="text-xs sm:text-sm text-slate-400 max-w-sm mx-auto mb-8 leading-relaxed">
        The documentation page you requested does not exist or has been restructured in the SP 3.1.0 update.
      </p>
      <Link
        href="/getting-started"
        className="inline-flex items-center gap-2 px-5 py-2.5 rounded-xl bg-gradient-to-r from-[#0057FF] to-[#00D1FF] text-white font-semibold text-xs shadow-[0_0_20px_rgba(0,87,255,0.3)] hover:opacity-95 transition-all"
      >
        <ArrowLeft className="w-4 h-4" />
        <span>Return to Docs Home</span>
      </Link>
    </div>
  );
}
