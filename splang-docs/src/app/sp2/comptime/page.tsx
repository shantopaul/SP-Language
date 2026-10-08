import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Zap, Cpu, Sparkles } from "lucide-react";

export const metadata = {
  title: "Compile-Time Function Execution (comptime)",
  description: "Execute functions during compilation in SP 2.0. Constant folding, compile-time assertions, and zero runtime cost.",
};

export default function Sp2ComptimePage() {
  const basicExample = `import std.io;

// Function designated to execute strictly within the compiler
comptime fn factorial(int n) -> int {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

fn main() -> void {
    // Evaluated at build time: lowered directly to the LLVM IR constant '3628800'
    let val = comptime factorial(10);
    println("Calculated at compile time: \${val}");
}`;

  const tableExample = `import std.io;

// Generate compile-time precomputed trigonometry or hash tables
comptime fn generateCRC32Table() -> int[] {
    let table = [];
    for (int i = 0; i < 256; i = i + 1) {
        int rem = i;
        for (int j = 0; j < 8; j = j + 1) {
            if ((rem & 1) != 0) {
                rem = (rem >> 1) ^ 0xEDB88320;
            } else {
                rem = rem >> 1;
            }
        }
        table.push(rem);
    }
    return table;
}

fn main() -> void {
    // Zero runtime calculation - instant lookup table embedded in binary .rodata
    let crcTable = comptime generateCRC32Table();
    println("CRC table size: \${crcTable.length()} entries ready at startup");
}`;

  const llvmIrInspection = `; LLVM IR emitted by spc -O2 for: let val = comptime factorial(10)
; Notice there is NO function call, NO loop, and NO stack allocation!
store i64 3628800, i64* %val, align 8`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 2.0 Evolution", href: "/sp2/overview" },
          { title: "Compile-Time Execution" },
        ]}
      />

      <div>
        <h1 className="text-3xl font-extrabold text-white">Compile-Time Function Execution (`comptime`)</h1>
        <p className="mt-3 text-sm text-slate-300 leading-relaxed">
          SP 2.0 introduces first-class <code>comptime</code> execution. Functions marked with <code>comptime</code> or expressions invoked in a <code>comptime</code> block are evaluated by the SP AST interpreter during compilation, burning results directly into the resulting binary as static data.
        </p>
      </div>

      <Callout type="tip" title="True Zero Runtime Overhead">
        Computations executed with <code>comptime</code> consume exactly <strong>0 CPU cycles and 0 nanoseconds of memory allocation</strong> at runtime. The compiler folds results into LLVM IR immediate constants or <code>.rodata</code> binary sections.
      </Callout>

      <h2 className="text-xl font-bold text-white pt-2">Basic Syntax &amp; Usage</h2>
      <p className="text-sm text-slate-300">
        Mark functions with <code>comptime fn</code> or wrap expressions with <code>comptime &lt;expr&gt;</code>:
      </p>
      <CodeBlock code={basicExample} filename="comptime_basic.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Precomputing Tables &amp; Static Data</h2>
      <p className="text-sm text-slate-300">
        Generate lookup tables, hash dictionaries, or validation schemas ahead-of-time rather than during application cold-start:
      </p>
      <CodeBlock code={tableExample} filename="lookup_table.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Inspecting LLVM IR Output</h2>
      <p className="text-sm text-slate-300">
        When compiling with <code>spc build main.sp --emit-llvm</code>, observe how the compiler eliminates all runtime branching and function calls:
      </p>
      <CodeBlock code={llvmIrInspection} language="llvm" />

      <h2 className="text-xl font-bold text-white pt-4">Rules &amp; Capabilities</h2>
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 text-xs sm:text-sm">
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h4 className="font-bold text-white mb-1.5 flex items-center gap-1.5">
            <Zap className="w-4 h-4 text-amber-400" />
            <span>Deterministic Execution</span>
          </h4>
          <p className="text-slate-400 leading-relaxed">
            Comptime code runs in an isolated deterministic interpreter. Non-deterministic operations like dynamic network sockets or random seed generators are rejected at compile time.
          </p>
        </div>

        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h4 className="font-bold text-white mb-1.5 flex items-center gap-1.5">
            <Cpu className="w-4 h-4 text-cyan-400" />
            <span>Compile-Time Assertions</span>
          </h4>
          <p className="text-slate-400 leading-relaxed">
            Use <code>comptime assert(condition, message)</code> to catch configuration bugs, invalid buffer lengths, or unsupported target configurations before linking.
          </p>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Language Editions", href: "/sp2/editions" }}
        next={{ title: "WebAssembly (wasm32-wasi)", href: "/sp2/wasm" }}
      />
    </div>
  );
}
