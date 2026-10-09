import React from "react";
import Link from "next/link";
import { Sparkles, ArrowRight, Download, BookOpen } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const ReleaseCallout: React.FC = () => {
  return (
    <section className="py-20 relative">
      <div className="max-w-5xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="relative rounded-3xl border border-cyan-500/30 bg-gradient-to-r from-cyan-950/40 via-slate-900/80 to-blue-950/40 p-8 sm:p-12 backdrop-blur-2xl overflow-hidden shadow-2xl text-center">
          {/* Subtle background glow */}
          <div className="absolute top-0 right-0 w-80 h-80 bg-cyan-500/10 rounded-full blur-3xl pointer-events-none" />
          <div className="absolute bottom-0 left-0 w-80 h-80 bg-blue-500/10 rounded-full blur-3xl pointer-events-none" />

          <div className="relative z-10 max-w-3xl mx-auto">
            <div className="inline-flex items-center gap-2 px-3 py-1 rounded-full bg-cyan-950/80 border border-cyan-700/60 text-cyan-300 text-xs font-semibold mb-6">
              <Sparkles className="w-3.5 h-3.5 text-cyan-400" />
              <span>Production Milestone</span>
            </div>

            <h2 className="text-3xl sm:text-5xl font-extrabold text-white tracking-tight leading-tight">
              Ready for Your Next Production Systems Project
            </h2>

            <p className="mt-4 text-base sm:text-lg text-slate-300 leading-relaxed">
              SP 3.0.0 is officially released and production-ready. Start building blazing-fast CLI utilities, backend microservices, WebAssembly modules, or computational kernels with confidence.
            </p>

            <div className="mt-8 flex flex-wrap items-center justify-center gap-4">
              <Link href="/downloads">
                <Button size="lg" variant="primary">
                  <Download className="w-4 h-4 mr-1" />
                  <span>Download SP 3.0.0</span>
                </Button>
              </Link>
              <a
                href="https://docs.splang.shantopaul.com/getting-started"
                target="_blank"
                rel="noopener noreferrer"
              >
                <Button size="lg" variant="secondary">
                  <BookOpen className="w-4 h-4 text-cyan-400 mr-1" />
                  <span>Read Quickstart Guide</span>
                  <ArrowRight className="w-4 h-4 ml-1" />
                </Button>
              </a>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};
