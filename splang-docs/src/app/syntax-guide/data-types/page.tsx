import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Data Types",
  description: "Primitive and collection data types in the SP Programming Language.",
};

export default function DataTypesPage() {
  const code = `import std.io;

fn main() -> void {
    // Scalar Primitives
    int a = 42;
    double b = 3.14159;
    bool isFast = true;
    string text = "SP Systems";
    
    // Arrays and Literals
    Array<int> scores = [95, 88, 100];
    
    println("Int: \${a}, Double: \${b}, Bool: \${isFast}");
    println("Array size: \${scores.length()}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Data Types" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Primitive & Compound Types</h1>
      <p className="text-sm text-slate-300">
        SP features a static, nominal type system with built-in primitive scalars and high-performance collection abstractions.
      </p>

      <CodeBlock code={code} filename="types.sp" />

      <h2 className="text-xl font-bold text-white pt-2">Primitive Types Table</h2>
      <div className="overflow-x-auto">
        <table className="w-full text-left text-xs sm:text-sm border border-slate-800 rounded-xl overflow-hidden">
          <thead className="bg-slate-900 text-slate-300 font-semibold border-b border-slate-800">
            <tr>
              <th className="p-3">Type</th>
              <th className="p-3">Size & Representation</th>
              <th className="p-3">Example</th>
            </tr>
          </thead>
          <tbody className="divide-y divide-slate-800/60 font-mono text-xs">
            <tr>
              <td className="p-3 text-cyan-400 font-bold">int</td>
              <td className="p-3 text-slate-400">64-bit signed integer (i64)</td>
              <td className="p-3 text-emerald-400">let x: int = 100;</td>
            </tr>
            <tr>
              <td className="p-3 text-cyan-400 font-bold">double</td>
              <td className="p-3 text-slate-400">64-bit IEEE-754 float (f64)</td>
              <td className="p-3 text-emerald-400">let d: double = 3.14;</td>
            </tr>
            <tr>
              <td className="p-3 text-cyan-400 font-bold">bool</td>
              <td className="p-3 text-slate-400">1-bit boolean (i1)</td>
              <td className="p-3 text-emerald-400">let flag: bool = true;</td>
            </tr>
            <tr>
              <td className="p-3 text-cyan-400 font-bold">string</td>
              <td className="p-3 text-slate-400">UTF-8 heap slice pointer</td>
              <td className="p-3 text-emerald-400">let s: string = &quot;SP&quot;;</td>
            </tr>
            <tr>
              <td className="p-3 text-cyan-400 font-bold">void</td>
              <td className="p-3 text-slate-400">Unit type representing no value</td>
              <td className="p-3 text-emerald-400">fn exit() -&gt; void</td>
            </tr>
          </tbody>
        </table>
      </div>

      <PrevNextNav
        prev={{ title: "Variables & Constants", href: "/syntax-guide/variables" }}
        next={{ title: "Control Flow & Matching", href: "/syntax-guide/control-flow" }}
      />
    </div>
  );
}
