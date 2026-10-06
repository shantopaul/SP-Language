import React from "react";
import {
  Cpu,
  ShieldCheck,
  Zap,
  Boxes,
  Code2,
  Terminal,
  Layers,
  Sparkles,
} from "lucide-react";
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from "@/components/ui/Card";

export const KeyFeatures: React.FC = () => {
  const features = [
    {
      icon: Cpu,
      title: "Native LLVM 18+ Backend",
      description:
        "Compiles directly to optimized machine code (.exe, ELF, Mach-O) via LLVM intermediate representation with -O0 through -O3 optimization passes.",
      color: "text-emerald-400",
      border: "hover:border-emerald-500/50",
    },
    {
      icon: ShieldCheck,
      title: "Expressive Type & Null Safety",
      description:
        "Static type checking with bidirectional inference, nullable types (T?), safe navigation (?.), and null-coalescing (??) catching errors before code generation.",
      color: "text-cyan-400",
      border: "hover:border-cyan-500/50",
    },
    {
      icon: Zap,
      title: "Zero-Cost Generics",
      description:
        "Specializes generic classes (Box<T>, List<T>, Map<K,V>) and functions via monomorphization, eliminating runtime boxing and dynamic casting overhead.",
      color: "text-blue-400",
      border: "hover:border-blue-500/50",
    },
    {
      icon: Layers,
      title: "Clean Object-Oriented Architecture",
      description:
        "Full support for classes, constructors, single inheritance, vtable virtual dispatch, interface contracts, struct value types, and type-safe enums.",
      color: "text-indigo-400",
      border: "hover:border-indigo-500/50",
    },
    {
      icon: Boxes,
      title: "Built-In Package Manager (spm)",
      description:
        "Complete dependency resolution, cycle detection, semantic versioning, and deterministic sp.lock generation backed by cryptographic SHA-256 validation.",
      color: "text-purple-400",
      border: "hover:border-purple-500/50",
    },
    {
      icon: Terminal,
      title: "Unified Developer Tooling",
      description:
        "Shipped with an idempotent formatter (spc fmt), native test runner (spc test), documentation generator (spc doc), and official VS Code LSP/DAP support.",
      color: "text-amber-400",
      border: "hover:border-amber-500/50",
    },
  ];

  return (
    <section className="py-24 relative">
      <div className="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
        <div className="text-center max-w-3xl mx-auto mb-16">
          <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-emerald-950/60 border border-emerald-800/60 text-emerald-400 text-xs font-medium mb-3">
            <Sparkles className="w-3.5 h-3.5" />
            <span>Architecture & Capabilities</span>
          </div>
          <h2 className="text-3xl sm:text-4xl font-bold text-white tracking-tight">
            Engineered for Systems & Applications
          </h2>
          <p className="mt-3 text-base text-slate-400 leading-relaxed">
            Every feature in SP is deliberately crafted to provide modern developer joy without compromising on raw hardware execution speed.
          </p>
        </div>

        <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6">
          {features.map((feature, idx) => {
            const Icon = feature.icon;
            return (
              <Card
                key={idx}
                className={`transition-all duration-300 hover:-translate-y-1 ${feature.border}`}
              >
                <CardHeader>
                  <div className="w-12 h-12 rounded-xl bg-slate-800/80 border border-slate-700/80 flex items-center justify-center mb-3">
                    <Icon className={`w-6 h-6 ${feature.color}`} />
                  </div>
                  <CardTitle>{feature.title}</CardTitle>
                </CardHeader>
                <CardContent>
                  <CardDescription>{feature.description}</CardDescription>
                </CardContent>
              </Card>
            );
          })}
        </div>
      </div>
    </section>
  );
};
