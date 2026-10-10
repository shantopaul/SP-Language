import React from "react";
import Link from "next/link";
import { ArrowLeft, Calendar, User, Sparkles, Terminal, CheckCircle2 } from "lucide-react";
import { Badge } from "@/components/ui/Badge";
import { CodeHighlighter } from "@/components/ui/CodeHighlighter";
import { Button } from "@/components/ui/Button";

export const metadata = {
  title: "Announcing SP 1.0.0: The Production-Ready Systems & Application Language",
  description:
    "Official launch of the SP Programming Language 1.0.0 GA with LLVM native compiler, standard library, package manager, and benchmarks.",
};

export default function ReleaseBlogPostPage() {
  const sampleCode = `import std.io;
import std.fs;

fn main() -> void {
    let message = "Hello from SP 1.0.0!";
    println(message);
    
    // Pattern matching
    let count = 42;
    let label = match (count) {
        0 => "Empty",
        42 => "The Answer",
        _ => "Other"
    };
    println("Status: \${label}");
}`;

  return (
    <div className="py-16 max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 relative">
      {/* Background ambient glow */}
      <div className="absolute top-10 left-1/2 -translate-x-1/2 w-[700px] h-[350px] bg-gradient-to-b from-[#0057FF]/15 to-transparent rounded-full blur-[100px] pointer-events-none -z-10" />

      {/* Back button */}
      <div className="mb-8">
        <Link
          href="/blog"
          className="inline-flex items-center gap-2 text-xs font-semibold text-slate-400 hover:text-[#00D1FF] transition-colors"
        >
          <ArrowLeft className="w-4 h-4" />
          <span>Back to All Articles</span>
        </Link>
      </div>

      {/* Header */}
      <header className="mb-12 border-b border-[#0A1F44] pb-8">
        <div className="flex items-center gap-3 mb-4">
          <Badge variant="secondary">Historical Release</Badge>
          <span className="flex items-center gap-1.5 text-xs text-slate-400">
            <Calendar className="w-3.5 h-3.5 text-[#00D1FF]" />
            October 6, 2026
          </span>
        </div>

        <h1 className="text-3xl sm:text-5xl font-black text-white tracking-tight leading-tight mb-6">
          Announcing SP 1.0.0: The Production-Ready Systems &amp; Application Language
        </h1>

        <div className="flex items-center gap-3 text-xs sm:text-sm text-slate-300">
          <div className="w-10 h-10 rounded-full bg-gradient-to-tr from-[#0057FF] to-[#00D1FF] p-0.5 shadow-[0_0_15px_rgba(0,209,255,0.4)]">
            <div className="w-full h-full bg-[#020D21] rounded-full flex items-center justify-center font-bold text-[#00D1FF]">
              SP
            </div>
          </div>
          <div>
            <div className="font-semibold text-white">Shanto Paul</div>
            <div className="text-slate-400 text-xs">Language Creator &amp; Lead Architect</div>
          </div>
        </div>
      </header>

      {/* Article Body */}
      <article className="space-y-6 text-slate-300 text-sm sm:text-base leading-relaxed">
        <p className="text-lg text-slate-200 font-medium">
          Today marks a landmark achievement in the development of the SP Programming Language. After extensive architecture design, compiler engineering, rigorous test suite development, and real-world benchmarking, we are thrilled to announce the general availability of <strong>SP 1.0.0</strong>.
        </p>

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          What is SP?
        </h2>
        <p>
          SP is a statically typed, compiled programming language engineered from the ground up to combine the raw execution speed and low resource footprint of languages like C and Rust with the ergonomics, clean syntax, and rapid developer velocity of modern languages like TypeScript and Go.
        </p>

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          Core Pillars of SP 1.0.0
        </h2>
        <ul className="space-y-3 list-none pl-0">
          <li className="flex items-start gap-2.5">
            <CheckCircle2 className="w-5 h-5 text-[#00D1FF] shrink-0 mt-0.5" />
            <div>
              <strong className="text-white">LLVM Native Compilation:</strong> Emits clean SP IR lowered to LLVM IR, running LLVM 18+ optimization passes (-O0, -O1, -O2, -O3) and linking native executables via Clang.
            </div>
          </li>
          <li className="flex items-start gap-2.5">
            <CheckCircle2 className="w-5 h-5 text-[#00D1FF] shrink-0 mt-0.5" />
            <div>
              <strong className="text-white">Zero-Cost Generics:</strong> Generic classes (<code>Box&lt;T&gt;</code>, <code>List&lt;T&gt;</code>, <code>Map&lt;K, V&gt;</code>) and functions are fully monomorphized at compile time, eliminating runtime type boxing or casting overhead.
            </div>
          </li>
          <li className="flex items-start gap-2.5">
            <CheckCircle2 className="w-5 h-5 text-[#00D1FF] shrink-0 mt-0.5" />
            <div>
              <strong className="text-white">Standard Library Out-of-the-Box:</strong> 8 comprehensive modules covering I/O, strings, mathematics, collections, filesystem, clock, OS environment, and child processes.
            </div>
          </li>
          <li className="flex items-start gap-2.5">
            <CheckCircle2 className="w-5 h-5 text-[#00D1FF] shrink-0 mt-0.5" />
            <div>
              <strong className="text-white">SP Package Manager (spm):</strong> Full dependency resolution, SemVer compatibility solver, cycle detection, and tamper-proof cryptographic <code>sp.lock</code> lockfiles.
            </div>
          </li>
          <li className="flex items-start gap-2.5">
            <CheckCircle2 className="w-5 h-5 text-[#00D1FF] shrink-0 mt-0.5" />
            <div>
              <strong className="text-white">First-Class Tooling:</strong> Built-in <code>spc fmt</code> code formatter, native <code>spc test</code> test runner, automatic <code>spc doc</code> generator, and official VS Code LSP/DAP extension.
            </div>
          </li>
        </ul>

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          Code in Action
        </h2>
        <p>Here is a taste of writing modern SP code:</p>

        <CodeHighlighter
          code={sampleCode}
          filename="main.sp"
          showLineNumbers={true}
        />

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          Performance Benchmarks
        </h2>
        <p>
          We benchmarked SP against 6 industry languages across 5 compute-heavy workloads. SP consistently achieved performance parity with optimized C and Rust, running up to <strong>62x faster than Python</strong> and <strong>3x faster than Java</strong> on recursive CPU tasks.
        </p>

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          Official Ecosystem Endpoints
        </h2>
        <p>The SP ecosystem is live today across three dedicated web portals:</p>
        <ul className="list-disc pl-5 space-y-1 text-sm">
          <li><strong>Official Website:</strong> <a href="https://splang.shantopaul.com" className="text-[#00D1FF] hover:underline">splang.shantopaul.com</a></li>
          <li><strong>Documentation:</strong> <a href="https://docs.splang.shantopaul.com" className="text-[#00D1FF] hover:underline">docs.splang.shantopaul.com</a></li>
          <li><strong>Package Registry:</strong> <a href="https://registry.splang.shantopaul.com" className="text-[#00D1FF] hover:underline">registry.splang.shantopaul.com</a></li>
        </ul>

        <div className="pt-8 border-t border-[#0A1F44] flex flex-col sm:flex-row items-center justify-between gap-4">
          <div>
            <p className="text-xs text-slate-400 mb-1">Ready to start building?</p>
            <p className="text-sm font-semibold text-white">Install SP and scaffold your first project.</p>
          </div>
          <Link href="/downloads">
            <Button variant="primary">Download SP 3.1.0</Button>
          </Link>
        </div>
      </article>
    </div>
  );
}
