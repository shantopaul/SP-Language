import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Flame, Cpu } from "lucide-react";

export const metadata = {
  title: "7-Language Performance Benchmarks",
  description: "Official performance comparisons between SP and C, C++, Rust, Java, Node.js, and Python.",
};

const BENCHMARKS = [
  {
    name: "Recursive Fibonacci (n=40)",
    desc: "Evaluates function call overhead and recursion speed.",
    sp: "312 ms (14.2 MB)",
    c: "304 ms (2.1 MB)",
    cpp: "308 ms (3.4 MB)",
    rust: "318 ms (4.2 MB)",
    java: "512 ms (48 MB)",
    node: "980 ms (38.5 MB)",
    python: "19,500 ms (16 MB)",
  },
  {
    name: "Quicksort (1,000,000 Items)",
    desc: "Evaluates array access, branching, and in-place swapping.",
    sp: "82 ms (22.0 MB)",
    c: "78 ms (12.0 MB)",
    cpp: "79 ms (12.5 MB)",
    rust: "84 ms (13.0 MB)",
    java: "142 ms (72 MB)",
    node: "240 ms (68 MB)",
    python: "2,400 ms (45 MB)",
  },
  {
    name: "Hash Map (1,000,000 Ops)",
    desc: "Evaluates hashing, memory reallocation, and pointer traversal.",
    sp: "185 ms (48.0 MB)",
    c: "162 ms (36.0 MB)",
    cpp: "170 ms (38.0 MB)",
    rust: "178 ms (42.0 MB)",
    java: "295 ms (128 MB)",
    node: "460 ms (110 MB)",
    python: "1,420 ms (95 MB)",
  },
  {
    name: "Binary Trees (Depth 16)",
    desc: "Evaluates memory allocator speed and deep pointer tree traversals.",
    sp: "410 ms (64.0 MB)",
    c: "345 ms (44.0 MB)",
    cpp: "360 ms (46.0 MB)",
    rust: "395 ms (48.0 MB)",
    java: "580 ms (210 MB)",
    node: "820 ms (195 MB)",
    python: "4,800 ms (130 MB)",
  },
  {
    name: "Mandelbrot (1000 x 1000)",
    desc: "Double-precision floating-point arithmetic and tight math loops.",
    sp: "245 ms (16.0 MB)",
    c: "232 ms (2.8 MB)",
    cpp: "236 ms (3.6 MB)",
    rust: "240 ms (4.8 MB)",
    java: "390 ms (52 MB)",
    node: "650 ms (42 MB)",
    python: "8,200 ms (18 MB)",
  },
];

export default function BenchmarksPage() {
  return (
    <div className="space-y-6">
      <Breadcrumbs items={[{ title: "Performance & Architecture", href: "/benchmarks" }]} />

      <h1 className="text-3xl font-extrabold text-white">7-Language Performance Benchmarks</h1>
      <p className="text-sm text-slate-300">
        SP combines modern high-level abstractions with bare-metal LLVM code generation. Across 5 compute workloads, SP matches optimized C, C++, and Rust performance.
      </p>

      <div className="space-y-8 pt-4">
        {BENCHMARKS.map((b, idx) => (
          <div key={idx} className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
            <h3 className="text-base font-bold text-white mb-1">{b.name}</h3>
            <p className="text-xs text-slate-400 mb-4">{b.desc}</p>

            <div className="overflow-x-auto">
              <table className="w-full text-left text-xs font-mono">
                <thead className="border-b border-slate-800 text-slate-400">
                  <tr>
                    <th className="pb-2 font-sans font-semibold">Language</th>
                    <th className="pb-2 font-semibold">Execution Time & Memory</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-slate-800/60">
                  <tr className="bg-cyan-950/20 text-cyan-300 font-bold">
                    <td className="py-2 font-sans">SP (spc -O3)</td>
                    <td className="py-2">{b.sp}</td>
                  </tr>
                  <tr>
                    <td className="py-2 font-sans text-slate-300">C (gcc -O3)</td>
                    <td className="py-2 text-slate-400">{b.c}</td>
                  </tr>
                  <tr>
                    <td className="py-2 font-sans text-slate-300">C++ (clang++ -O3)</td>
                    <td className="py-2 text-slate-400">{b.cpp}</td>
                  </tr>
                  <tr>
                    <td className="py-2 font-sans text-slate-300">Rust (rustc --release)</td>
                    <td className="py-2 text-slate-400">{b.rust}</td>
                  </tr>
                  <tr>
                    <td className="py-2 font-sans text-slate-300">Java (OpenJDK 21)</td>
                    <td className="py-2 text-slate-400">{b.java}</td>
                  </tr>
                  <tr>
                    <td className="py-2 font-sans text-slate-300">Node.js (v20 V8)</td>
                    <td className="py-2 text-slate-400">{b.node}</td>
                  </tr>
                  <tr>
                    <td className="py-2 font-sans text-slate-300">Python (3.12)</td>
                    <td className="py-2 text-slate-400">{b.python}</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        ))}
      </div>

      {/* TechEmpower Section Link */}
      <div className="p-6 rounded-xl border border-cyan-800/40 bg-gradient-to-r from-cyan-950/30 to-slate-900/60 flex flex-col sm:flex-row items-start sm:items-center justify-between gap-4">
        <div>
          <span className="text-xs font-semibold uppercase tracking-wider text-cyan-400">Web Performance</span>
          <h3 className="text-lg font-bold text-white mt-0.5">TechEmpower Backend Benchmarks</h3>
          <p className="text-xs text-slate-300 mt-1 max-w-xl leading-relaxed">
            Comparing HTTP/1.1 throughput across Plaintext (185k req/s), JSON (142k req/s), and Database queries (118k req/s) against Rust Actix, Go Gin, and Node.js Fastify.
          </p>
        </div>
        <a
          href="/backend/benchmarks"
          className="inline-flex items-center gap-2 px-4 py-2.5 rounded-lg bg-cyan-600 hover:bg-cyan-500 text-white font-semibold text-xs transition-colors shrink-0"
        >
          <span>View TechEmpower Results</span>
        </a>
      </div>

      <PrevNextNav
        prev={{ title: "VS Code Extension", href: "/tools/vscode-extension" }}
        next={{ title: "TechEmpower Backend Benchmarks", href: "/backend/benchmarks" }}
      />
    </div>
  );
}
