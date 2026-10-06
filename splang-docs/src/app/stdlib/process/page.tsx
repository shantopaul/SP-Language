import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.process Module Reference",
  description: "Subprocess execution, external commands, and IPC in SP.",
};

export default function StdProcessPage() {
  const exampleCode = `import std.io;
import std.process;

fn main() -> void {
    let exitCode = process.exec("git --version");
    println("Git check exit code: \${exitCode}");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.process" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.process Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.process</code> module provides primitives for spawning child processes and executing system commands.
      </p>

      <CodeBlock code={exampleCode} filename="process_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported Functions</h2>

      <ApiCard
        name="process.exec"
        signature="fn process.exec(string command) -> int"
        description="Spawns a shell command synchronously and returns the exit status code."
        parameters={[{ name: "command", type: "string", description: "Command string to execute." }]}
        returns="int (0 if process succeeded)"
      />

      <PrevNextNav
        prev={{ title: "std.os Reference", href: "/stdlib/os" }}
        next={{ title: "Developer Tools", href: "/tools" }}
      />
    </div>
  );
}
