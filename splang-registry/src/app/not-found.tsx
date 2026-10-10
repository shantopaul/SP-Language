import React from "react";
import Link from "next/link";
import { PackageX, ArrowLeft, Search } from "lucide-react";

export default function NotFound() {
  return (
    <div className="max-w-2xl mx-auto px-4 py-24 text-center">
      <div className="relative rounded-3xl border border-[#0A1F44] bg-[#081735]/70 backdrop-blur-xl p-8 sm:p-12 shadow-2xl space-y-6">
        <div className="w-16 h-16 rounded-2xl bg-gradient-to-br from-[#0057FF]/20 to-[#00D1FF]/10 border border-[#00D1FF]/30 text-[#00D1FF] flex items-center justify-center mx-auto shadow-[0_0_24px_rgba(0,209,255,0.2)]">
          <PackageX className="w-8 h-8" />
        </div>

        <div className="space-y-2">
          <div className="inline-flex items-center gap-2 px-3 py-1 rounded-full border border-[#00D1FF]/40 bg-[#0057FF]/15 text-[#00D1FF] text-xs font-mono uppercase tracking-widest shadow-[0_0_12px_rgba(0,209,255,0.15)]">
            404 — Package or Resource Not Found
          </div>
          <h1 className="text-3xl sm:text-4xl font-black text-white tracking-tight">
            Page Does Not Exist
          </h1>
          <p className="text-sm text-slate-300 max-w-md mx-auto leading-relaxed">
            The requested package, route, or resource is not registered in the SP Package Registry or may have been relocated.
          </p>
        </div>

        <div className="flex flex-wrap items-center justify-center gap-3 pt-4">
          <Link
            href="/"
            className="inline-flex items-center gap-2 px-5 py-2.5 rounded-xl border border-[#0A1F44] bg-[#020D21] hover:bg-[#081735] hover:border-[#00D1FF]/40 text-slate-200 text-xs font-bold transition-all"
          >
            <ArrowLeft className="w-3.5 h-3.5 text-[#00D1FF]" />
            <span>Return Home</span>
          </Link>
          <Link
            href="/packages"
            className="inline-flex items-center gap-2 px-5 py-2.5 rounded-xl bg-gradient-to-r from-[#0057FF] to-[#00D1FF] hover:opacity-90 text-white font-bold text-xs transition-all shadow-md shadow-[#0057FF]/30"
          >
            <Search className="w-3.5 h-3.5" />
            <span>Browse All Packages</span>
          </Link>
        </div>
      </div>
    </div>
  );
}
