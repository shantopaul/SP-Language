import React from "react";
import { BENCHMARKS_DATA } from "@/data/benchmarks-data";
import { Flame, Cpu, Gauge, Shield, Terminal, ArrowRight } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const metadata = {
  title: "Performance Benchmarks",
  description:
    "Official benchmarks comparing SP Programming Language with C, C++, Rust, Java, Node.js, and Python across 5 standard computing workloads.",
};

export default function BenchmarksPage() {
  return (
    <div className="py-16 max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 relative">
      {/* Background ambient glow */}
      <div className="absolute top-10 left-1/2 -translate-x-1/2 w-[700px] h-[350px] bg-gradient-to-b from-[#0057FF]/15 to-transparent rounded-full blur-[100px] pointer-events-none -z-10" />

      {/* Page Header */}
      <div className="text-center max-w-3xl mx-auto mb-16">
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-[#0057FF]/15 border border-[#00D1FF]/30 text-[#00D1FF] text-xs font-semibold mb-3 shadow-[0_0_15px_rgba(0,209,255,0.15)]">
          <Flame className="w-4 h-4 text-[#00D1FF]" />
          <span>7-Language Performance Suite</span>
        </div>
        <h1 className="text-4xl sm:text-5xl font-black text-white tracking-tight">
          Performance <span className="text-transparent bg-clip-text bg-gradient-to-r from-white via-slate-100 to-[#00D1FF]">Benchmarks</span>
        </h1>
        <p className="mt-4 text-base sm:text-lg text-slate-300">
          SP is engineered from the ground up for high-performance computing. We rigorously test SP against leading systems and application languages to ensure exceptional raw runtime execution and memory efficiency.
        </p>
      </div>

      {/* Test Environment Card */}
      <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/70 backdrop-blur-md p-6 sm:p-8 mb-16 shadow-2xl relative overflow-hidden">
        <div className="absolute top-0 right-0 w-64 h-64 bg-[#0057FF]/10 rounded-full blur-3xl pointer-events-none" />
        <h2 className="text-lg font-bold text-white mb-4 flex items-center gap-2">
          <Cpu className="w-5 h-5 text-[#00D1FF]" />
          Hardware & Test Environment
        </h2>
        <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4 text-xs font-mono">
          <div className="bg-[#020D21]/80 p-4 rounded-xl border border-[#0A1F44]/80">
            <span className="text-slate-500 block uppercase font-sans text-[11px] font-semibold tracking-wider">Processor</span>
            <span className="text-slate-200 font-semibold text-sm">AMD Ryzen 9 7950X</span>
            <span className="text-slate-400 block mt-1">16 Cores / 32 Threads @ 4.5GHz</span>
          </div>
          <div className="bg-[#020D21]/80 p-4 rounded-xl border border-[#0A1F44]/80">
            <span className="text-slate-500 block uppercase font-sans text-[11px] font-semibold tracking-wider">Memory</span>
            <span className="text-slate-200 font-semibold text-sm">32 GB DDR5-6000</span>
            <span className="text-slate-400 block mt-1">Dual-channel, Low latency</span>
          </div>
          <div className="bg-[#020D21]/80 p-4 rounded-xl border border-[#0A1F44]/80">
            <span className="text-slate-500 block uppercase font-sans text-[11px] font-semibold tracking-wider">Operating System</span>
            <span className="text-slate-200 font-semibold text-sm">Ubuntu 24.04 LTS / Win 11</span>
            <span className="text-slate-400 block mt-1">Linux Kernel 6.8 / MSVC CRT</span>
          </div>
          <div className="bg-[#020D21]/80 p-4 rounded-xl border border-[#0A1F44]/80">
            <span className="text-slate-500 block uppercase font-sans text-[11px] font-semibold tracking-wider">Compiler Backend</span>
            <span className="text-slate-200 font-semibold text-sm">LLVM 18.1.8 Target</span>
            <span className="text-slate-400 block mt-1">Full LTO + Vectorization</span>
          </div>
        </div>
      </div>

      {/* Workload Deep Dives */}
      <div className="space-y-16 mb-16">
        {BENCHMARKS_DATA.map((workload, idx) => (
          <div
            key={workload.id}
            className="rounded-2xl border border-[#0A1F44] bg-[#081735]/60 backdrop-blur-md p-6 sm:p-8 shadow-2xl hover:border-[#0057FF]/40 transition-all duration-300"
          >
            <div className="flex flex-col md:flex-row md:items-center justify-between pb-6 mb-6 border-b border-[#0A1F44] gap-4">
              <div>
                <div className="text-xs font-mono text-[#00D1FF] uppercase tracking-wider mb-1 font-semibold">
                  Workload #{idx + 1}
                </div>
                <h3 className="text-2xl font-bold text-white">{workload.name}</h3>
                <p className="text-sm text-slate-300 mt-1 max-w-2xl">{workload.description}</p>
              </div>
              <div className="flex items-center gap-2">
                <span className="px-3 py-1 rounded-full text-xs font-mono bg-[#0057FF]/20 text-[#00D1FF] border border-[#00D1FF]/40 shadow-[0_0_10px_rgba(0,209,255,0.1)]">
                  Median of 10 Warm Iterations
                </span>
              </div>
            </div>

            {/* Table of Results */}
            <div className="overflow-x-auto">
              <table className="w-full text-left text-xs sm:text-sm">
                <thead>
                  <tr className="border-b border-[#0A1F44] text-slate-400 uppercase text-[11px] tracking-wider">
                    <th className="pb-3 font-semibold">Language</th>
                    <th className="pb-3 font-semibold">Compiler / Runtime</th>
                    <th className="pb-3 font-semibold">Execution Time (ms)</th>
                    <th className="pb-3 font-semibold">Peak Memory (MB)</th>
                    <th className="pb-3 font-semibold">Speedup vs Python</th>
                    <th className="pb-3 text-right font-semibold">SP Relative Speed</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-[#0A1F44]/60 font-mono">
                  {workload.results.map((res) => {
                    const spResult = workload.results.find((r) => r.isSP)!;
                    const relativeRatio = (res.runtimeMs / spResult.runtimeMs).toFixed(2);
                    const isFasterThanSP = res.runtimeMs < spResult.runtimeMs;

                    return (
                      <tr
                        key={res.language}
                        className={`transition-colors ${
                          res.isSP
                            ? "bg-[#0057FF]/15 border-l-2 border-[#00D1FF] font-bold"
                            : "hover:bg-[#081735]/60"
                        }`}
                      >
                        <td className="py-3.5 px-3 font-sans flex items-center gap-2">
                          <span
                            className={res.isSP ? "text-[#00D1FF] font-bold text-base drop-shadow-[0_0_8px_rgba(0,209,255,0.5)]" : "text-slate-200"}
                          >
                            {res.language}
                          </span>
                          {res.isSP && (
                            <span className="px-2 py-0.5 rounded-full bg-[#0057FF]/30 text-[#00D1FF] text-[10px] font-bold border border-[#00D1FF]/50">
                              SP Target
                            </span>
                          )}
                        </td>
                        <td className="py-3.5 px-3 text-slate-400 font-sans">{res.compiler}</td>
                        <td className="py-3.5 px-3 text-white font-semibold">
                          {res.runtimeMs} ms
                        </td>
                        <td className="py-3.5 px-3 text-slate-300">{res.memoryMb} MB</td>
                        <td className="py-3.5 px-3 text-[#00D1FF] font-semibold">
                          {res.speedupVsPython}
                        </td>
                        <td className="py-3.5 px-3 text-right font-sans">
                          {res.isSP ? (
                            <span className="text-[#00D1FF] font-semibold font-mono">1.00x (Baseline)</span>
                          ) : isFasterThanSP ? (
                            <span className="text-amber-400 font-mono font-medium">
                              {relativeRatio}x faster
                            </span>
                          ) : (
                            <span className="text-slate-400 font-mono">
                              {relativeRatio}x slower
                            </span>
                          )}
                        </td>
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>
          </div>
        ))}
      </div>

      {/* Replication Guide */}
      <div className="rounded-2xl border border-[#0A1F44] bg-[#081735]/70 backdrop-blur-md p-6 sm:p-8 shadow-2xl relative overflow-hidden">
        <h3 className="text-xl font-bold text-white mb-2 flex items-center gap-2">
          <Terminal className="w-5 h-5 text-[#00D1FF]" />
          Reproducing Benchmarks Locally
        </h3>
        <p className="text-sm text-slate-300 mb-4">
          All benchmark source files in SP, C, C++, Rust, Java, Node.js, and Python are open-source and included in the repository under <code className="text-[#00D1FF] font-mono">benchmarks/</code>. You can run the entire automated benchmarking suite with:
        </p>
        <pre className="bg-[#020D21] p-4 rounded-xl border border-[#0A1F44] font-mono text-xs text-[#00D1FF] overflow-x-auto mb-4">
          git clone https://github.com/shantopaul/SP-Language.git&#10;cd SP-Language&#10;python benchmarks/run_benchmarks.py --all --iterations 10
        </pre>
        <p className="text-xs text-slate-400">
          The runner will automatically compile all source variants with highest optimization flags (-O3 / LTO), warm up memory caches, and output structured latency percentiles (P50, P90, P99).
        </p>
      </div>
    </div>
  );
}
