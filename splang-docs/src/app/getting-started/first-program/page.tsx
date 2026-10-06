import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { CodeBlock } from "@/components/ui/CodeBlock";

export const metadata = {
  title: "First Program (Hello SP)",
  description: "Learn how to write, compile, and execute your first SP program natively.",
};

export default function FirstProgramPage() {
  const code = `import std.io;

fn main() -> void {
    let greeting = "Hello, SP 1.0.0!";
    println(greeting);
}`;

  const runCmd = `spc run hello.sp`;
  const buildCmd = `spc build hello.sp -o hello -O3`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Getting Started", href: "/getting-started" },
          { title: "First Program" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Your First SP Program</h1>
      <p className="text-sm text-slate-300">
        In this tutorial, you will write a complete SP program, run it instantly in JIT mode, and build an optimized native executable.
      </p>

      <h2 className="text-xl font-bold text-white pt-2">1. Write the Code</h2>
      <p className="text-xs text-slate-400">
        Create a new file named <code className="text-cyan-400 font-mono">hello.sp</code> in your preferred editor:
      </p>
      <CodeBlock code={code} filename="hello.sp" />

      <h2 className="text-xl font-bold text-white pt-4">2. Run Directly with `spc run`</h2>
      <p className="text-xs text-slate-400">
        The fastest way to test and execute your SP code during development is using <code className="text-cyan-400 font-mono">spc run</code>:
      </p>
      <CodeBlock code={runCmd} language="bash" />
      <p className="text-xs text-slate-400">Output:</p>
      <pre className="bg-black/60 p-3 rounded-lg border border-slate-800 font-mono text-xs text-emerald-400">
        Hello, SP 1.0.0!
      </pre>

      <h2 className="text-xl font-bold text-white pt-4">3. Compile a Standalone Native Executable</h2>
      <p className="text-xs text-slate-400">
        To compile an optimized, standalone binary ready for deployment:
      </p>
      <CodeBlock code={buildCmd} language="bash" />
      <Callout type="tip" title="Optimization Flag">
        The <code>-O3</code> flag instructs LLVM to enable full vectorization, loop unrolling, and inlining for maximum performance.
      </Callout>

      <PrevNextNav
        prev={{ title: "Installation", href: "/getting-started/installation" }}
        next={{ title: "Project Scaffolding (spm)", href: "/getting-started/project-setup" }}
      />
    </div>
  );
}
