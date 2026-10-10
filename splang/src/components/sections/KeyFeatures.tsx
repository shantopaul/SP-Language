import React from "react";
import {
  Cpu,
  ShieldCheck,
  Zap,
  Boxes,
  Terminal,
  Layers,
  Sparkles,
  Server,
  Lock,
} from "lucide-react";
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/Card";

export const KeyFeatures: React.FC = () => {
  const features = [
    {
      icon: Server,
      title: "Enterprise Web Platform (std.web)",
      description:
        "Official high-throughput backend framework with sub-microsecond Radix routing, connection pooling (SQLite & Postgres), automated OpenAPI 3.0 generation, and zero-socket in-memory testing delivering 185,000 req/s.",
      iconColor: "text-cyan-400",
      glowBg: "bg-cyan-500/10 border-cyan-500/30",
    },
    {
      icon: Cpu,
      title: "Native LLVM & WebAssembly Backend",
      description:
        "Compiles directly to optimized machine code (.exe, ELF, Mach-O) and WebAssembly (wasm32-wasi) via LLVM intermediate representation with -O3 optimization and full LTO vectorization.",
      iconColor: "text-blue-400",
      glowBg: "bg-blue-500/10 border-blue-500/30",
    },
    {
      icon: ShieldCheck,
      title: "Deterministic ARC Memory Model",
      description:
        "Hybrid value semantics and compiler-automated reference counting with RAII scope-exit drops. Zero stop-the-world GC pauses, predictable microsecond latency, and 0 memory leaks.",
      iconColor: "text-cyan-300",
      glowBg: "bg-cyan-500/10 border-cyan-500/30",
    },
    {
      icon: Zap,
      title: "Compile-Time Execution (comptime)",
      description:
        "Execute pure SP functions, compile-time assertions, and precomputed constant lookup tables directly during compiler build passes with zero runtime overhead.",
      iconColor: "text-amber-400",
      glowBg: "bg-amber-500/10 border-amber-500/30",
    },
    {
      icon: Layers,
      title: "23 Production Stdlib Modules",
      description:
        "Comprehensive battery-included standard library covering crypto (Argon2id, AES, SHA), database pooling, Redis client, WebSockets, HTTP client/server, metrics, and processes.",
      iconColor: "text-indigo-400",
      glowBg: "bg-indigo-500/10 border-indigo-500/30",
    },
    {
      icon: Terminal,
      title: "Unified Production Toolchain",
      description:
        "Shipped out-of-the-box with native compiler (spc), package manager (spm), code formatter (spc fmt), test runner (spc test), and Language Server Protocol (spls) for VS Code.",
      iconColor: "text-blue-300",
      glowBg: "bg-blue-500/10 border-blue-500/30",
    },
  ];

  return (
    <section className="py-24 relative bg-slate-50 dark:bg-[#020D21] border-t border-slate-200 dark:border-transparent">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-16">
          <div className="inline-flex items-center gap-1.5 px-3.5 py-1 rounded-full bg-blue-50 dark:bg-[#081735] border border-blue-200 dark:border-cyan-500/40 text-[#0057FF] dark:text-cyan-300 text-xs font-semibold mb-3 shadow-sm dark:shadow-glow-sm">
            <Sparkles className="w-3.5 h-3.5 text-[#0057FF] dark:text-cyan-400" />
            <span>Architecture &amp; Capabilities</span>
          </div>
          <h2 className="text-3xl sm:text-5xl font-extrabold text-slate-900 dark:text-white tracking-tight">
            Engineered for Systems &amp; Scale
          </h2>
          <p className="mt-4 text-base text-slate-600 dark:text-slate-300 leading-relaxed">
            Every feature in SP is deliberately designed to provide modern developer ergonomics without compromising on raw hardware execution speed.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {features.map((feature, idx) => {
            const Icon = feature.icon;
            return (
              <Card
                key={idx}
                className="transition-all duration-300 hover:-translate-y-1.5 hover:border-blue-400 dark:hover:border-cyan-400/40 hover:shadow-lg dark:hover:shadow-[0_12px_35px_rgba(0,87,255,0.15)] flex flex-col justify-between"
              >
                <div>
                  <CardHeader>
                    <div className={`w-12 h-12 rounded-xl border flex items-center justify-center mb-4 ${feature.glowBg}`}>
                      <Icon className={`w-6 h-6 ${feature.iconColor}`} />
                    </div>
                    <CardTitle className="text-xl text-slate-900 dark:text-white font-bold">{feature.title}</CardTitle>
                  </CardHeader>
                  <CardContent>
                    <CardDescription className="text-slate-600 dark:text-slate-300 leading-relaxed text-sm">
                      {feature.description}
                    </CardDescription>
                  </CardContent>
                </div>
              </Card>
            );
          })}
        </div>
      </div>
    </section>
  );
};
