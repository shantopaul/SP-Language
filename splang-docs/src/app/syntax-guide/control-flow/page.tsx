import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Control Flow & Matching",
  description: "Conditionals, loops, and exhaustive pattern matching in SP.",
};

export default function ControlFlowPage() {
  const codeIf = `if (score >= 90) {
    println("Grade: A");
} else if (score >= 75) {
    println("Grade: B");
} else {
    println("Grade: C");
}`;

  const codeMatch = `let statusCode = 200;

let message = match (statusCode) {
    200 => "OK - Request successful",
    404 => "Not Found",
    500 => "Internal Server Error",
    _   => "Unknown status"
};

println(message);`;

  const codeLoops = `// 1. Classic for loop
for (int i = 0; i < 5; i = i + 1) {
    println("Index: \${i}");
}

// 2. Collection for-in loop
let fruits = ["Apple", "Orange", "Banana"];
for (string f in fruits) {
    println("Fruit: \${f}");
}

// 3. While loop
var n = 3;
while (n > 0) {
    n = n - 1;
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Language Tour", href: "/syntax-guide" },
          { title: "Control Flow & Matching" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Control Flow & Matching</h1>
      <p className="text-sm text-slate-300">
        SP supports standard branching constructs, loops, and modern pattern matching.
      </p>

      <h2 className="text-xl font-bold text-white pt-2">Conditional If-Else</h2>
      <CodeBlock code={codeIf} />

      <h2 className="text-xl font-bold text-white pt-4">Pattern Matching (`match`)</h2>
      <p className="text-xs text-slate-400">
        The <code>match</code> construct evaluates an expression against multiple branch patterns. It is an expression that yields a value:
      </p>
      <CodeBlock code={codeMatch} />

      <h2 className="text-xl font-bold text-white pt-4">Loops (`for`, `for-in`, `while`)</h2>
      <CodeBlock code={codeLoops} />

      <PrevNextNav
        prev={{ title: "Data Types", href: "/syntax-guide/data-types" }}
        next={{ title: "Functions & Closures", href: "/syntax-guide/functions" }}
      />
    </div>
  );
}
