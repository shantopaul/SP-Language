import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Documentation Generator (spc doc)",
  description: "Generating Markdown and modern HTML documentation from /// comments.",
};

export default function DocGeneratorPage() {
  const docCommentCode = `/// Calculates the Euclidean distance between two points.
///
/// Parameters:
///   x1: Starting X coordinate
///   y1: Starting Y coordinate
///   x2: Target X coordinate
///   y2: Target Y coordinate
///
/// Returns: The double-precision distance between points.
fn distance(double x1, double y1, double x2, double y2) -> double {
    let dx = x2 - x1;
    let dy = y2 - y1;
    return math.sqrt(dx * dx + dy * dy);
}`;

  const cliUsage = `# Generate Markdown documentation
spc doc src/ --out docs/ --format markdown

# Generate modern standalone HTML portal
spc doc src/ --out site/ --format html`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Developer Tools", href: "/tools" },
          { title: "Doc Generator (spc doc)" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Documentation Generator (`spc doc`)</h1>
      <p className="text-sm text-slate-300">
        <code>spc doc</code> parses docstrings prefixed with three slashes (<code>///</code>) directly above function, class, and module declarations and renders documentation.
      </p>

      <CodeBlock code={docCommentCode} filename="geometry.sp" />

      <h2 className="text-xl font-bold text-white pt-4">CLI Usage</h2>
      <CodeBlock code={cliUsage} language="bash" />

      <PrevNextNav
        prev={{ title: "Native Test Runner (spc test)", href: "/tools/test-runner" }}
        next={{ title: "Language Server (spls)", href: "/tools/lsp-server" }}
      />
    </div>
  );
}
