import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Lambdas & Closures",
  description: "First-class anonymous functions, lexical closures, and functional methods in SP.",
};

export default function LambdasPage() {
  const code = `import std.io;
import std.collections;

fn main() -> void {
    let numbers = [1, 2, 3, 4, 5, 6, 7, 8];
    
    // 1. Arrow lambda syntax
    let evens = numbers.filter((int n) => n % 2 == 0);
    
    // 2. Chained transformation
    let doubled = evens.map((int n) => n * 2);
    
    // 3. Closure capturing lexical scope
    int multiplier = 5;
    let customScaled = numbers.map((int n) => n * multiplier);
    
    println("Original length: \${numbers.length()}");
    println("Evens length: \${evens.length()}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Lambdas & Closures" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Lambdas & Functional Pipelines</h1>
      <p className="text-sm text-slate-300">
        SP treats functions as first-class values. You can define anonymous arrow functions <code>(params) =&gt; expr</code> and pass them into higher-order functions like <code>map</code>, <code>filter</code>, and <code>reduce</code>.
      </p>

      <CodeBlock code={code} filename="lambdas.sp" />

      <h2 className="text-xl font-bold text-white pt-2">Lexical Closure Capture</h2>
      <p className="text-xs text-slate-400">
        Lambdas can capture variables from their enclosing lexical scope. Captured variables are packed into an automatic closure context struct and lowered into LLVM IR safely.
      </p>

      <PrevNextNav
        prev={{ title: "Zero-Cost Generics", href: "/syntax-guide/generics" }}
        next={{ title: "Null Safety", href: "/syntax-guide/null-safety" }}
      />
    </div>
  );
}
