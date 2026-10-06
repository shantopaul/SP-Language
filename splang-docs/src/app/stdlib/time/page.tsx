import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.time Module Reference",
  description: "High-resolution monotonic timestamps and duration tracking in SP.",
};

export default function StdTimePage() {
  const exampleCode = `import std.io;
import std.time;

fn main() -> void {
    let start = time.now();
    
    // Simulate short delay
    time.sleep(100); // 100 milliseconds
    
    let elapsed = time.now() - start;
    println("Elapsed time: \${elapsed} ms");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.time" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.time Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.time</code> module provides nanosecond-precision timestamps, wall clock access, and thread sleep execution.
      </p>

      <CodeBlock code={exampleCode} filename="time_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported Functions</h2>

      <ApiCard
        name="time.now"
        signature="fn time.now() -> int"
        description="Returns the current monotonic system timestamp in milliseconds."
        returns="int"
      />

      <ApiCard
        name="time.sleep"
        signature="fn time.sleep(int milliseconds) -> void"
        description="Suspends execution of the current thread for the specified duration."
        parameters={[{ name: "milliseconds", type: "int", description: "Delay in milliseconds." }]}
        returns="void"
      />

      <PrevNextNav
        prev={{ title: "std.fs Reference", href: "/stdlib/fs" }}
        next={{ title: "std.os Reference", href: "/stdlib/os" }}
      />
    </div>
  );
}
