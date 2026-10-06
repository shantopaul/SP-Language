import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.math Module Reference",
  description: "Trigonometric, logarithmic, and floating-point mathematics in SP.",
};

export default function StdMathPage() {
  const exampleCode = `import std.io;
import std.math;

fn main() -> void {
    let root = math.sqrt(144.0);
    let power = math.pow(2.0, 10.0);
    let angle = math.sin(math.PI / 2.0);
    
    println("sqrt(144) = \${root}");
    println("2^10 = \${power}");
    println("sin(PI/2) = \${angle}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.math" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.math Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.math</code> module provides mathematical functions, trigonometry, and scientific constants.
      </p>

      <CodeBlock code={exampleCode} filename="math_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported Functions & Constants</h2>

      <ApiCard
        name="math.PI"
        signature="const double math.PI = 3.14159265358979323846"
        description="The mathematical constant pi."
      />

      <ApiCard
        name="math.sqrt"
        signature="fn math.sqrt(double x) -> double"
        description="Returns the positive square root of a non-negative floating-point number."
        parameters={[{ name: "x", type: "double", description: "Input value." }]}
        returns="double"
      />

      <ApiCard
        name="math.pow"
        signature="fn math.pow(double base, double exp) -> double"
        description="Returns base raised to the power of exp."
        parameters={[
          { name: "base", type: "double", description: "Base number." },
          { name: "exp", type: "double", description: "Exponent." },
        ]}
        returns="double"
      />

      <ApiCard
        name="math.abs"
        signature="fn math.abs(double x) -> double"
        description="Returns the absolute magnitude of x."
        parameters={[{ name: "x", type: "double", description: "Target number." }]}
        returns="double"
      />

      <ApiCard
        name="math.min / math.max"
        signature="fn math.min(double a, double b) -> double&#10;fn math.max(double a, double b) -> double"
        description="Returns the smaller or larger of two numbers."
        returns="double"
      />

      <PrevNextNav
        prev={{ title: "std.string Reference", href: "/stdlib/string" }}
        next={{ title: "std.collections Reference", href: "/stdlib/collections" }}
      />
    </div>
  );
}
