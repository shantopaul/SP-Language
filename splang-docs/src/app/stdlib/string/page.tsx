import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.string Module Reference",
  description: "String manipulation, UTF-8 processing, and search utilities in SP.",
};

export default function StdStringPage() {
  const exampleCode = `import std.io;
import std.string;

fn main() -> void {
    let raw = "   Systems Engineering   ";
    let trimmed = string.trim(raw);
    let upper = string.upper(trimmed);
    
    println("Trimmed: '\${trimmed}'");
    println("Upper: '\${upper}'");
    println("Length: \${string.length(trimmed)}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.string" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.string Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.string</code> module provides high-performance native UTF-8 string algorithms.
      </p>

      <CodeBlock code={exampleCode} filename="string_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported Functions</h2>

      <ApiCard
        name="string.length"
        signature="fn string.length(string s) -> int"
        description="Returns the total character count (in UTF-8 codepoints) of the string."
        parameters={[{ name: "s", type: "string", description: "Target string." }]}
        returns="int"
      />

      <ApiCard
        name="string.upper"
        signature="fn string.upper(string s) -> string"
        description="Returns a copy of the string transformed to uppercase."
        parameters={[{ name: "s", type: "string", description: "Target string." }]}
        returns="string"
      />

      <ApiCard
        name="string.lower"
        signature="fn string.lower(string s) -> string"
        description="Returns a copy of the string transformed to lowercase."
        parameters={[{ name: "s", type: "string", description: "Target string." }]}
        returns="string"
      />

      <ApiCard
        name="string.trim"
        signature="fn string.trim(string s) -> string"
        description="Returns a copy of the string with leading and trailing whitespace stripped."
        parameters={[{ name: "s", type: "string", description: "Target string." }]}
        returns="string"
      />

      <ApiCard
        name="string.contains"
        signature="fn string.contains(string s, string sub) -> bool"
        description="Returns true if substring sub is found within target string s."
        parameters={[
          { name: "s", type: "string", description: "Target string." },
          { name: "sub", type: "string", description: "Substring to search for." },
        ]}
        returns="bool"
      />

      <PrevNextNav
        prev={{ title: "std.io Reference", href: "/stdlib/io" }}
        next={{ title: "std.math Reference", href: "/stdlib/math" }}
      />
    </div>
  );
}
