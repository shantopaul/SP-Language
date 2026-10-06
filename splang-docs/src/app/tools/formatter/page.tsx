import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Code Formatter (spc fmt)",
  description: "Deterministic, idempotent SP code formatting tool.",
};

export default function FormatterPage() {
  const usage = `# Check formatting without modifying files (returns non-zero if diff exists)
spc fmt src/ --check

# Format files in-place
spc fmt src/ --write

# Custom 4-space indent
spc fmt src/ --write --indent 4`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Developer Tools", href: "/tools" },
          { title: "Code Formatter (spc fmt)" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Code Formatter (`spc fmt`)</h1>
      <p className="text-sm text-slate-300">
        SP features an opinionated, built-in code formatter that enforces uniform style across codebases.
      </p>

      <CodeBlock code={usage} language="bash" />

      <h2 className="text-xl font-bold text-white pt-4">Formatting Guarantees</h2>
      <ul className="list-disc list-inside space-y-2 text-xs sm:text-sm text-slate-300">
        <li><strong>Strict Idempotency:</strong> Applying the formatter repeatedly produces the identical AST and text layout: <code>fmt(fmt(x)) == fmt(x)</code>.</li>
        <li><strong>Preserves Comments:</strong> All single-line <code>//</code> comments, multi-line blocks, and <code>///</code> docstrings are preserved.</li>
        <li><strong>CI Ready:</strong> The <code>--check</code> flag is suited for GitHub Actions and pre-commit hooks.</li>
      </ul>

      <PrevNextNav
        prev={{ title: "Package Manager (spm)", href: "/tools/package-manager" }}
        next={{ title: "Native Test Runner (spc test)", href: "/tools/test-runner" }}
      />
    </div>
  );
}
