import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Variables & Constants",
  description: "Declaring variables in SP with let, var, and static type annotations.",
};

export default function VariablesPage() {
  const code = `import std.io;

fn main() -> void {
    // 1. Immutable bindings with 'let' (recommended)
    let language = "SP";
    let version = 1.0;
    
    // 2. Mutable bindings with 'var'
    var counter = 0;
    counter = counter + 1;
    
    // 3. Explicit type annotations
    int explicitAge = 25;
    string explicitCity = "Stockholm";
    
    println("\${language} v\${version} - Count: \${counter}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Variables & Constants" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Variables & Constants</h1>
      <p className="text-sm text-slate-300">
        In SP, variables can be declared using type inference (<code>let</code> and <code>var</code>) or explicit type annotations.
      </p>

      <CodeBlock code={code} filename="variables.sp" />

      <h2 className="text-xl font-bold text-white pt-2">Immutability by Default with `let`</h2>
      <p className="text-xs text-slate-400">
        Variables declared with <code>let</code> are immutable once initialized. Reassigning a <code>let</code> variable triggers a compile-time error:
      </p>

      <Callout type="warning" title="Compiler Error E1003">
        <code>let x = 10; x = 20; // Error E1003: Cannot reassign immutable variable &apos;x&apos;</code>
      </Callout>

      <h2 className="text-xl font-bold text-white pt-2">Mutability with `var`</h2>
      <p className="text-xs text-slate-400">
        When a value needs to be reassigned, declare it with <code>var</code>. The type of the variable is inferred from the initial assignment and cannot change to an incompatible type.
      </p>

      <PrevNextNav
        prev={{ title: "Syntax Overview", href: "/syntax-guide" }}
        next={{ title: "Data Types", href: "/syntax-guide/data-types" }}
      />
    </div>
  );
}
