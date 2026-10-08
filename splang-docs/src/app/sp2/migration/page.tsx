import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Wrench, CheckCircle, ShieldAlert, ArrowRight } from "lucide-react";

export const metadata = {
  title: "Automated Migration (spc fix)",
  description: "Seamlessly upgrade your SP 1.x projects to SP 2.0 and modern editions using the built-in automated refactoring tool.",
};

export default function Sp2MigrationPage() {
  const dryRunCmd = `# 1. Preview changes across the entire project without modifying files
spc fix --dry-run

# 2. Automatically apply syntax modernizations and rewrite AST
spc fix

# 3. Upgrade project directly to Edition 2028
spc fix --edition 2028

# 4. Verify test suite passes 100%
spc test`;

  const beforeAfterDiff = `// BEFORE: SP 1.x legacy permissive syntax
fn calculateTax(amount, rate) {
    if (amount <= 0) return 0;
    return amount * rate;
}

// AFTER: Modern SP 2.0 / Edition 2028 syntax (auto-applied by spc fix)
fn calculateTax(double amount, double rate) -> double {
    if (amount <= 0.0) return 0.0;
    return amount * rate;
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 2.0 Evolution", href: "/sp2/overview" },
          { title: "Automated Migration" },
        ]}
      />

      <div>
        <h1 className="text-3xl font-extrabold text-white">Automated Migration (`spc fix`)</h1>
        <p className="mt-3 text-sm text-slate-300 leading-relaxed">
          Migrating a codebase between major versions shouldn&apos;t require manual tedious rewrites. The SP 2.0 toolchain includes <code>spc fix</code>, an AST-aware refactoring engine that automatically inspects, modernizes, and upgrades your code to modern SP standards.
        </p>
      </div>

      <Callout type="tip" title="Safe &amp; Reversible">
        Always run <code>spc fix --dry-run</code> first to view a unified diff of all prospective modifications before committing them to disk.
      </Callout>

      <h2 className="text-xl font-bold text-white pt-2">Migration Commands</h2>
      <p className="text-sm text-slate-300">
        Run the migration workflow in four straightforward steps:
      </p>
      <CodeBlock code={dryRunCmd} language="bash" />

      <h2 className="text-xl font-bold text-white pt-4">AST Refactoring Transformations</h2>
      <p className="text-sm text-slate-300">
        <code>spc fix</code> performs intelligent semantic AST transformations rather than naive text string replacements:
      </p>
      <div className="space-y-3 pt-2 text-xs sm:text-sm">
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <strong className="text-white block mb-1">Explicit Type Annotations</strong>
          <span className="text-slate-400">Infers and inserts explicit return type signatures (<code>-&gt; double</code>) required by modern edition standards.</span>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <strong className="text-white block mb-1">Modern Keyword Alignment</strong>
          <span className="text-slate-400">Renames legacy or deprecated library identifiers and wraps constants in <code>comptime</code> blocks where optimal.</span>
        </div>
        <div className="p-3.5 rounded-xl border border-slate-800 bg-slate-900/40">
          <strong className="text-white block mb-1">Manifest Updates</strong>
          <span className="text-slate-400">Updates <code>edition = &quot;2026&quot;</code> or <code>edition = &quot;2028&quot;</code> in your <code>spc.toml</code> configuration file.</span>
        </div>
      </div>

      <h2 className="text-xl font-bold text-white pt-4">Example Transformation</h2>
      <CodeBlock code={beforeAfterDiff} filename="tax_service.sp" />

      <PrevNextNav
        prev={{ title: "Structured Coroutines (async/await)", href: "/sp2/async-await" }}
        next={{ title: "Getting Started Overview", href: "/getting-started" }}
      />
    </div>
  );
}
