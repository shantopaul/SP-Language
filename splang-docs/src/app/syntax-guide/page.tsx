import React from "react";
import Link from "next/link";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { ArrowRight } from "lucide-react";

export const metadata = {
  title: "Syntax Guide Overview",
  description: "Complete overview of the SP Programming Language syntax and grammar constructs.",
};

export default function SyntaxGuideOverviewPage() {
  const summarySnippet = `import std.io;

// Variables with inference
let name = "SP";
var counter = 0;

// Type annotations & functions
fn add(int a, int b) -> int {
    return a + b;
}

// Pattern matching
enum Role { Admin, User }

fn check(Role r) -> string {
    return match (r) {
        Role.Admin => "Superuser",
        Role.User => "Standard"
    };
}`;

  const sections = [
    { title: "Variables & Constants", href: "/syntax-guide/variables", desc: "let and var declarations, mutability, and type inference." },
    { title: "Primitive & Complex Types", href: "/syntax-guide/data-types", desc: "int, double, string, bool, and collection structures." },
    { title: "Control Flow & Matching", href: "/syntax-guide/control-flow", desc: "if/else, for loops, while loops, and pattern matching." },
    { title: "Functions & Closures", href: "/syntax-guide/functions", desc: "Named functions, return types, and default parameters." },
    { title: "OOP, Classes & Structs", href: "/syntax-guide/oop", desc: "Constructors, inheritance, interfaces, and value structs." },
    { title: "Zero-Cost Generics", href: "/syntax-guide/generics", desc: "Generic types with compile-time monomorphization." },
    { title: "Lambdas & Functional Pipelines", href: "/syntax-guide/lambdas", desc: "Arrow lambdas, closures, map/filter/reduce." },
    { title: "Null Safety & Operators", href: "/syntax-guide/null-safety", desc: "Nullable types (T?), safe navigation (?.), and coalescing (??)." },
  ];

  return (
    <div className="space-y-6">
      <Breadcrumbs items={[{ title: "Language Tour", href: "/syntax-guide" }]} />

      <h1 className="text-3xl font-extrabold text-white">SP Language Syntax Tour</h1>
      <p className="text-sm text-slate-300 leading-relaxed">
        SP combines the best parts of C-family ergonomics with modern type safety. Explore the core syntax chapters below:
      </p>

      <CodeBlock code={summarySnippet} filename="tour_preview.sp" />

      {/* Complete 100 Examples Banner */}
      <Link
        href="/syntax-guide/all-examples"
        className="block p-5 sm:p-6 rounded-2xl border border-[#00D1FF]/40 bg-gradient-to-r from-[#0057FF]/20 via-[#081735]/80 to-[#020D21] hover:border-[#00D1FF] hover:shadow-[0_0_25px_rgba(0,209,255,0.2)] transition-all group shadow-xl"
      >
        <div className="flex items-center justify-between">
          <div>
            <span className="inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full text-[11px] font-semibold bg-[#0057FF]/30 text-[#00D1FF] border border-[#00D1FF]/40 mb-2">
              ⭐ New: All 100 Code Examples
            </span>
            <h3 className="text-base sm:text-lg font-bold text-white group-hover:text-[#00D1FF] transition-colors">
              Complete SP Syntax Guide (1–100 Examples)
            </h3>
            <p className="text-xs sm:text-sm text-slate-300 mt-1">
              Browse, search, and copy all 100 serial code examples from basic syntax to multi-threaded REST microservices.
            </p>
          </div>
          <ArrowRight className="w-5 h-5 text-[#00D1FF] group-hover:translate-x-1.5 transition-transform shrink-0 ml-4" />
        </div>
      </Link>

      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 pt-2">
        {sections.map((sec) => (
          <Link
            key={sec.href}
            href={sec.href}
            className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 hover:bg-[#081735] hover:border-[#0057FF]/50 transition-all group shadow-lg"
          >
            <h3 className="text-sm font-bold text-white group-hover:text-[#00D1FF] transition-colors flex items-center justify-between">
              <span>{sec.title}</span>
              <ArrowRight className="w-3.5 h-3.5 text-slate-500 group-hover:text-[#00D1FF] group-hover:translate-x-1 transition-transform" />
            </h3>
            <p className="text-xs text-slate-400 mt-1 leading-relaxed">{sec.desc}</p>
          </Link>
        ))}
      </div>

      <PrevNextNav
        prev={{ title: "Project Scaffolding", href: "/getting-started/project-setup" }}
        next={{ title: "Variables & Constants", href: "/syntax-guide/variables" }}
      />
    </div>
  );
}
