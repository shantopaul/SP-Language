import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { CheckCircle2, Shield, ArrowRightLeft } from "lucide-react";

export const metadata = {
  title: "Language Editions (2026 / 2028)",
  description: "Learn how the SP Language Edition system delivers non-breaking evolution, backwards compatibility, and cross-edition linking.",
};

export default function Sp2EditionsPage() {
  const manifestExample = `[package]
name = "distributed_pipeline"
version = "2.0.0"
authors = ["Engineering Team <team@example.com>"]
edition = "2026" # Or "2028" for next-gen semantics

[dependencies]
std = "2.0.0"
fast_json = { version = "1.4.0", edition = "2026" }`;

  const cliExample = `# Compile source file under the 2026 stable edition
spc build main.sp --edition 2026 -o service_2026

# Compile with the 2028 next-gen edition
spc build main.sp --edition 2028 -o service_2028

# Automated migration to 2028 edition
spc fix --edition 2028`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 2.0 Evolution", href: "/sp2/overview" },
          { title: "Language Editions" },
        ]}
      />

      <div>
        <h1 className="text-3xl font-extrabold text-white">Language Editions</h1>
        <p className="mt-3 text-sm text-slate-300 leading-relaxed">
          The SP Language Edition system provides <strong>stability without stagnation</strong>. New language features, keyword reservations, and stricter compiler checks can be introduced in new editions without breaking existing SP 1.x or 2026-era codebases.
        </p>
      </div>

      <Callout type="tip" title="Zero ABI Breakage Guarantee">
        Editions operate strictly at the compiler frontend (parser, AST, and semantic analysis). Lowering to LLVM IR and the native ABI remain 100% interoperable across editions. A module written in edition <code>2026</code> can link directly with a module in edition <code>2028</code>.
      </Callout>

      <h2 className="text-xl font-bold text-white pt-2">Available Editions</h2>
      <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
        <div className="p-5 rounded-xl border border-slate-800 bg-slate-900/40">
          <div className="flex items-center gap-2 mb-2">
            <span className="px-2.5 py-0.5 rounded-full bg-emerald-950/80 border border-emerald-800 text-emerald-400 text-xs font-bold">
              Edition 2026 (Default)
            </span>
          </div>
          <p className="text-xs text-slate-300 leading-relaxed">
            The primary LTS edition. Guarantees 100% backward compatibility with all SP 1.0–1.7 syntax, including legacy function declaration forms, permissive visibility rules, and standard ARC memory ownership.
          </p>
          <ul className="mt-3 space-y-1.5 text-xs text-slate-400">
            <li className="flex items-center gap-1.5">
              <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400 shrink-0" />
              <span>Full SP 1.x backwards compatibility</span>
            </li>
            <li className="flex items-center gap-1.5">
              <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400 shrink-0" />
              <span>Comptime &amp; WebAssembly support enabled</span>
            </li>
            <li className="flex items-center gap-1.5">
              <CheckCircle2 className="w-3.5 h-3.5 text-emerald-400 shrink-0" />
              <span>Recommended for existing enterprise deployments</span>
            </li>
          </ul>
        </div>

        <div className="p-5 rounded-xl border border-slate-800 bg-slate-900/40">
          <div className="flex items-center gap-2 mb-2">
            <span className="px-2.5 py-0.5 rounded-full bg-cyan-950/80 border border-cyan-800 text-cyan-400 text-xs font-bold">
              Edition 2028 (Next-Gen)
            </span>
          </div>
          <p className="text-xs text-slate-300 leading-relaxed">
            The forward-looking edition designed for maximum performance, stricter safety checks, mandatory return-type signatures, and reserved keywords for advanced metaprogramming.
          </p>
          <ul className="mt-3 space-y-1.5 text-xs text-slate-400">
            <li className="flex items-center gap-1.5">
              <CheckCircle2 className="w-3.5 h-3.5 text-cyan-400 shrink-0" />
              <span>Mandatory explicit return signatures</span>
            </li>
            <li className="flex items-center gap-1.5">
              <CheckCircle2 className="w-3.5 h-3.5 text-cyan-400 shrink-0" />
              <span>Stricter non-null dereference validation</span>
            </li>
            <li className="flex items-center gap-1.5">
              <CheckCircle2 className="w-3.5 h-3.5 text-cyan-400 shrink-0" />
              <span>Direct migration path via <code>spc fix</code></span>
            </li>
          </ul>
        </div>
      </div>

      <h2 className="text-xl font-bold text-white pt-4">Configuring Editions</h2>
      <p className="text-sm text-slate-300">
        Specify your project edition in the <code>spc.toml</code> package manifest file:
      </p>
      <CodeBlock code={manifestExample} filename="spc.toml" language="toml" />

      <h2 className="text-xl font-bold text-white pt-4">CLI Flag Usage</h2>
      <p className="text-sm text-slate-300">
        Override or specify the target edition directly on the command line:
      </p>
      <CodeBlock code={cliExample} language="bash" />

      <PrevNextNav
        prev={{ title: "Release Overview", href: "/sp2/overview" }}
        next={{ title: "Compile-Time Execution (comptime)", href: "/sp2/comptime" }}
      />
    </div>
  );
}
