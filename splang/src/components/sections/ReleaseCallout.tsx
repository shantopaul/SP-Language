import React from "react";
import Link from "next/link";
import { Sparkles, ArrowRight, Download, BookOpen } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const ReleaseCallout: React.FC = () => {
  return (
    <section className="py-24 relative bg-slate-50 dark:bg-[#020D21] transition-colors">
      <div className="max-w-5xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="relative rounded-3xl border border-blue-200 dark:border-cyan-500/40 bg-gradient-to-r from-blue-50/90 via-sky-50/80 to-blue-50/90 dark:from-[#081735]/90 dark:via-[#0A1F44]/80 dark:to-[#081735]/90 p-8 sm:p-14 backdrop-blur-2xl overflow-hidden shadow-xl dark:shadow-2xl text-center">
          {/* Subtle background glow */}
          <div className="absolute top-0 right-0 w-96 h-96 bg-[#00D1FF]/10 rounded-full blur-3xl pointer-events-none" />
          <div className="absolute bottom-0 left-0 w-96 h-96 bg-[#0057FF]/15 rounded-full blur-3xl pointer-events-none" />

          <div className="relative z-10 max-w-3xl mx-auto">
            <div className="inline-flex items-center gap-2 px-4 py-1.5 rounded-full bg-white dark:bg-[#020D21]/80 border border-blue-200 dark:border-cyan-500/50 text-[#0057FF] dark:text-cyan-300 text-xs font-semibold mb-6 shadow-sm dark:shadow-glow-sm">
              <Sparkles className="w-3.5 h-3.5 text-[#0057FF] dark:text-cyan-400" />
              <span>Production Milestone</span>
            </div>

            <h2 className="text-3xl sm:text-5xl font-extrabold text-slate-900 dark:text-white tracking-tight leading-tight">
              Ready for Your Next Production Systems Project
            </h2>

            <p className="mt-5 text-base sm:text-lg text-slate-600 dark:text-slate-300 leading-relaxed max-w-2xl mx-auto">
              SP 3.1.0 GA is officially released and battle-tested. Build blazing-fast CLI utilities, high-throughput web microservices, WebAssembly modules, or computational kernels with deterministic memory safety.
            </p>

            <div className="mt-9 flex flex-wrap items-center justify-center gap-4">
              <Link href="/downloads">
                <Button size="lg" variant="primary">
                  <Download className="w-4 h-4 mr-1.5" />
                  <span>Download SP v3.1.0 GA</span>
                  <ArrowRight className="w-4 h-4 ml-1" />
                </Button>
              </Link>
              <a
                href="https://docs.splang.shantopaul.com/getting-started"
                target="_blank"
                rel="noopener noreferrer"
              >
                <Button size="lg" variant="secondary">
                  <BookOpen className="w-4 h-4 text-[#0057FF] dark:text-cyan-400 mr-1.5" />
                  <span>Read Quickstart Guide</span>
                </Button>
              </a>
            </div>
          </div>
        </div>
      </div>
    </section>
  );
};
