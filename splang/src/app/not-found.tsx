import React from "react";
import Link from "next/link";
import { ArrowLeft, Compass } from "lucide-react";
import { Button } from "@/components/ui/Button";

export default function NotFound() {
  return (
    <div className="min-h-[70vh] flex items-center justify-center px-4 relative">
      {/* Background ambient glow */}
      <div className="absolute top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-[500px] h-[300px] bg-gradient-to-b from-[#0057FF]/15 to-transparent rounded-full blur-[100px] pointer-events-none -z-10" />

      <div className="text-center max-w-md">
        <div className="w-16 h-16 rounded-2xl bg-[#0057FF]/15 border border-[#00D1FF]/40 flex items-center justify-center mx-auto mb-6 text-[#00D1FF] shadow-[0_0_20px_rgba(0,209,255,0.25)]">
          <Compass className="w-8 h-8" />
        </div>
        <span className="font-mono text-[#00D1FF] text-xs font-bold uppercase tracking-wider px-3 py-1 rounded-full bg-[#0057FF]/20 border border-[#00D1FF]/30 inline-block mb-3">
          Error 404
        </span>
        <h1 className="text-3xl sm:text-4xl font-black text-white mb-3">Page Not Found</h1>
        <p className="text-sm text-slate-400 mb-8 leading-relaxed">
          The requested page could not be found. Please check the URL or navigate back to the main portal or documentation.
        </p>
        <div className="flex items-center justify-center gap-3">
          <Link href="/">
            <Button variant="primary">
              <ArrowLeft className="w-4 h-4 mr-1.5" />
              <span>Back to Home</span>
            </Button>
          </Link>
          <a
            href="https://docs.splang.shantopaul.com"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button variant="secondary">Go to Docs</Button>
          </a>
        </div>
      </div>
    </div>
  );
}
