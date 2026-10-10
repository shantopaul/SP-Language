"use client";

import React, { useState } from "react";
import Link from "next/link";
import { BENCHMARKS_DATA } from "@/data/benchmarks-data";
import { Flame, ArrowUpRight, BarChart3, Database } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const BenchmarkVisualizer: React.FC = () => {
  const [activeWorkloadId, setActiveWorkloadId] = useState(BENCHMARKS_DATA[0].id);
  const [metric, setMetric] = useState<"runtime" | "memory">("runtime");

  const currentWorkload =
    BENCHMARKS_DATA.find((w) => w.id === activeWorkloadId) || BENCHMARKS_DATA[0];

  // Calculate highest value for proportional bars
  const maxRuntime = Math.max(...currentWorkload.results.map((r) => r.runtimeMs));
  const maxMemory = Math.max(...currentWorkload.results.map((r) => r.memoryMb));

  return (
    <section className="py-24 bg-[#020D21] border-t border-[#0A1F44] relative">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="flex flex-col md:flex-row md:items-end justify-between mb-12 gap-6">
          <div>
            <div className="inline-flex items-center gap-1.5 px-3.5 py-1 rounded-full bg-[#081735] border border-amber-500/40 text-amber-400 text-xs font-semibold mb-3 shadow-sm">
              <Flame className="w-3.5 h-3.5" />
              <span>Verifiable Real-World Benchmarks</span>
            </div>
            <h2 className="text-3xl sm:text-5xl font-extrabold text-white tracking-tight">
              Speed on Par with C &amp; Rust
            </h2>
            <p className="mt-3 text-sm sm:text-base text-slate-300 max-w-2xl leading-relaxed">
              Evaluated across standard computing workloads on identical hardware (AMD Ryzen 9 / 32GB RAM).
            </p>
          </div>

          {/* Metric Switcher */}
          <div className="flex items-center gap-2 bg-[#081735] p-1.5 rounded-xl border border-[#0A1F44] self-start md:self-auto">
            <button
              onClick={() => setMetric("runtime")}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium transition-all ${
                metric === "runtime"
                  ? "bg-[#0057FF]/30 text-cyan-300 border border-cyan-500/50 shadow-glow-sm"
                  : "text-slate-400 hover:text-white"
              }`}
            >
              <BarChart3 className="w-3.5 h-3.5 text-cyan-400" />
              <span>Execution Time (ms)</span>
            </button>
            <button
              onClick={() => setMetric("memory")}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium transition-all ${
                metric === "memory"
                  ? "bg-[#0057FF]/30 text-cyan-300 border border-cyan-500/50 shadow-glow-sm"
                  : "text-slate-400 hover:text-white"
              }`}
            >
              <Database className="w-3.5 h-3.5 text-cyan-400" />
              <span>Peak Memory (MB)</span>
            </button>
          </div>
        </div>

        {/* Workload Selector Tabs */}
        <div className="flex items-center gap-2 overflow-x-auto pb-2 mb-8 scrollbar-none">
          {BENCHMARKS_DATA.map((workload) => (
            <button
              key={workload.id}
              onClick={() => setActiveWorkloadId(workload.id)}
              className={`px-4 py-2 rounded-xl text-xs sm:text-sm font-medium whitespace-nowrap transition-all ${
                activeWorkloadId === workload.id
                  ? "bg-[#081735] text-cyan-300 border border-cyan-400/50 shadow-glow-sm font-semibold"
                  : "bg-[#081735]/40 text-slate-400 border border-[#0A1F44] hover:text-slate-200 hover:border-slate-700"
              }`}
            >
              {workload.name.split("(")[0].trim()}
            </button>
          ))}
        </div>

        {/* Workload Description Banner */}
        <div className="mb-6 px-5 py-3.5 rounded-xl bg-[#081735]/60 border border-[#0A1F44] text-xs sm:text-sm text-slate-300 flex items-center justify-between">
          <span>
            <strong className="text-white">Benchmark:</strong> {currentWorkload.name} — {currentWorkload.description}
          </span>
          <span className="hidden sm:inline text-xs text-slate-400 font-mono">
            Lower is better
          </span>
        </div>

        {/* Visual Bar Chart */}
        <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/70 backdrop-blur-xl p-6 sm:p-8 mb-8 space-y-4 shadow-xl">
          {currentWorkload.results.map((res) => {
            const val = metric === "runtime" ? res.runtimeMs : res.memoryMb;
            const maxVal = metric === "runtime" ? maxRuntime : maxMemory;
            const percentage = Math.max(4, Math.round((val / maxVal) * 100));

            return (
              <div key={res.language} className="space-y-1.5">
                <div className="flex items-center justify-between text-xs sm:text-sm">
                  <div className="flex items-center gap-2">
                    <span
                      className={`font-semibold ${
                        res.isSP ? "text-cyan-300 font-bold" : "text-slate-200"
                      }`}
                    >
                      {res.language}
                    </span>
                    <span className="text-[11px] font-mono text-slate-400">
                      ({res.compiler})
                    </span>
                    {res.isSP && (
                      <span className="px-2 py-0.5 rounded-full bg-cyan-950 text-cyan-300 text-[10px] font-mono border border-cyan-700 font-semibold shadow-glow-sm">
                        Target
                      </span>
                    )}
                  </div>
                  <div className="flex items-center gap-3">
                    <span className="font-mono text-xs text-cyan-400 font-semibold">
                      {res.speedupVsPython}
                    </span>
                    <span
                      className={`font-mono text-xs sm:text-sm font-semibold ${
                        res.isSP ? "text-cyan-300" : "text-slate-300"
                      }`}
                    >
                      {metric === "runtime" ? `${val} ms` : `${val} MB`}
                    </span>
                  </div>
                </div>

                {/* Progress bar */}
                <div className="w-full h-3.5 bg-[#020D21] rounded-full overflow-hidden border border-[#0A1F44] p-0.5">
                  <div
                    className={`h-full rounded-full transition-all duration-500 ${
                      res.isSP
                        ? "bg-gradient-to-r from-[#0057FF] via-[#00D1FF] to-[#38e1ff] shadow-[0_0_15px_rgba(0,209,255,0.45)]"
                        : res.language === "C" || res.language === "C++" || res.language === "Rust"
                        ? "bg-slate-400"
                        : "bg-slate-600"
                    }`}
                    style={{ width: `${percentage}%` }}
                  />
                </div>
              </div>
            );
          })}
        </div>

        {/* Footer Link to Full Benchmarks */}
        <div className="flex flex-col sm:flex-row items-center justify-between gap-4">
          <p className="text-xs text-slate-400">
            Certified TechEmpower methodology &amp; source code: <code className="font-mono text-slate-300">docs/BENCHMARKS.md</code>.
          </p>
          <Link href="/benchmarks">
            <Button variant="outline" size="sm">
              <span>View Full 7-Language Benchmark Matrix</span>
              <ArrowUpRight className="w-4 h-4 ml-1" />
            </Button>
          </Link>
        </div>
      </div>
    </section>
  );
};
