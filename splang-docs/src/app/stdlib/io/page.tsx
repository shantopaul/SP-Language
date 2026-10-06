import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.io Module Reference",
  description: "Standard input, output, formatting, and console interaction functions in SP.",
};

export default function StdIoPage() {
  const exampleCode = `import std.io;

fn main() -> void {
    print("Enter your name: ");
    let name = readLine();
    println("Welcome to SP, \${name}!");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.io" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.io Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.io</code> module provides core primitives for printing to standard output and reading user input from standard input.
      </p>

      <CodeBlock code={exampleCode} filename="io_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported Functions</h2>

      <ApiCard
        name="println"
        signature="fn println(string message) -> void"
        description="Prints the specified string message to stdout followed by a trailing newline character."
        parameters={[{ name: "message", type: "string", description: "The text string to display." }]}
        returns="void"
      />

      <ApiCard
        name="print"
        signature="fn print(string message) -> void"
        description="Prints the specified message to stdout without appending a trailing newline."
        parameters={[{ name: "message", type: "string", description: "The text to display." }]}
        returns="void"
      />

      <ApiCard
        name="readLine"
        signature="fn readLine() -> string"
        description="Reads a line of text from standard input up to the next newline character."
        returns="string (the entered user input without trailing newline)"
      />

      <PrevNextNav
        prev={{ title: "Stdlib Overview", href: "/stdlib" }}
        next={{ title: "std.string Reference", href: "/stdlib/string" }}
      />
    </div>
  );
}
