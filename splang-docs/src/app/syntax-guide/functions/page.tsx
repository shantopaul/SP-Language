import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Functions & Signatures",
  description: "Declaring and invoking functions with return types and default parameters in SP.",
};

export default function FunctionsPage() {
  const code = `import std.io;

// Function with explicit parameters and return type
fn multiply(int a, int b) -> int {
    return a * b;
}

// Function with default parameter value
fn greet(string name, string greeting = "Welcome") -> void {
    println("\${greeting}, \${name}!");
}

// Function overloading supported by argument types
fn display(int x) -> void {
    println("Int: \${x}");
}

fn display(string s) -> void {
    println("String: \${s}");
}

fn main() -> void {
    let result = multiply(6, 7);
    println("6 * 7 = \${result}");
    greet("Alice");
    display(100);
    display("SP Language");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Functions & Closures" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Functions in SP</h1>
      <p className="text-sm text-slate-300">
        Functions are defined with the <code>fn</code> keyword, parameter types, and an explicit return type arrow <code>-&gt; Type</code>.
      </p>

      <CodeBlock code={code} filename="functions.sp" />

      <h2 className="text-xl font-bold text-white pt-2">Key Function Features</h2>
      <ul className="list-disc list-inside space-y-2 text-xs sm:text-sm text-slate-300">
        <li><strong>Static Return Path Analysis:</strong> All non-void functions are statically verified to return a value along every code execution branch.</li>
        <li><strong>Function Overloading:</strong> Multiple functions may share the same name provided their argument type signatures differ.</li>
        <li><strong>Default Arguments:</strong> Trailing parameters may specify default values.</li>
      </ul>

      <PrevNextNav
        prev={{ title: "Control Flow & Matching", href: "/syntax-guide/control-flow" }}
        next={{ title: "OOP & Structs", href: "/syntax-guide/oop" }}
      />
    </div>
  );
}
