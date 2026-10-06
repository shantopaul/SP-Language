import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Native Test Runner (spc test)",
  description: "Executing embedded test blocks with assertion assertions and filtering.",
};

export default function TestRunnerPage() {
  const exampleTest = `import std.math;

test "Verify Square Root Calculation" {
    let result = math.sqrt(100.0);
    assert(result == 10.0, "Expected sqrt(100) to equal 10");
}

test "Verify Power Function" {
    let p = math.pow(2.0, 3.0);
    assert(p == 8.0, "Expected 2^3 == 8");
}`;

  const cliUsage = `# Run all tests in the current directory and subdirectories
spc test

# Run tests matching a regex filter
spc test --filter "Square Root"

# Verbose execution with assertion logs
spc test -v`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Developer Tools", href: "/tools" },
          { title: "Native Test Runner (spc test)" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Native Test Runner (`spc test`)</h1>
      <p className="text-sm text-slate-300">
        SP enables unit and integration testing without external dependencies through embedded <code>test &quot;name&quot; &#123; ... &#125;</code> blocks.
      </p>

      <CodeBlock code={exampleTest} filename="math_test.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Running Tests</h2>
      <CodeBlock code={cliUsage} language="bash" />

      <PrevNextNav
        prev={{ title: "Code Formatter (spc fmt)", href: "/tools/formatter" }}
        next={{ title: "Doc Generator (spc doc)", href: "/tools/doc-generator" }}
      />
    </div>
  );
}
