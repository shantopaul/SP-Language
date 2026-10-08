import React from "react";
import Link from "next/link";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { CodeBlock } from "@/components/ui/CodeBlock";

export const metadata = {
  title: "Getting Started Overview",
  description: "Introduction to SP programming language philosophy, design, and architecture.",
};

export default function GettingStartedOverviewPage() {
  return (
    <div className="space-y-6">
      <Breadcrumbs items={[{ title: "Getting Started", href: "/getting-started" }]} />

      <h1 className="text-3xl font-extrabold text-white">Introduction to SP</h1>

      <p className="text-sm text-slate-300 leading-relaxed">
        SP is a statically typed, natively compiled language designed for developers who demand both the bare-metal performance and deterministic memory model of C and Rust, and the syntactic clarity and ergonomics of modern application languages.
      </p>

      <Callout type="note" title="Core Design Philosophy">
        <p>
          SP achieves zero runtime abstraction penalty through static analysis and compile-time monomorphization. Every program is lowered to SP Intermediate Representation (SP IR) and compiled directly to machine instructions via LLVM 18+.
        </p>
      </Callout>

      <h2 className="text-xl font-bold text-white pt-4">Why SP?</h2>
      <ul className="list-disc list-inside space-y-2 text-sm text-slate-300">
        <li><strong>LLVM-Optimized Performance:</strong> Direct machine code execution with full support for <code>-O0</code>, <code>-O1</code>, <code>-O2</code>, and <code>-O3</code> optimizations.</li>
        <li><strong>Modern Syntax:</strong> Clean expression structures, safe null handling (<code>?.</code>, <code>??</code>), and exhaustive pattern matching.</li>
        <li><strong>Zero-Cost Generics:</strong> Generic collections and methods compile into specialized native symbols with no dynamic boxing.</li>
        <li><strong>All-In-One Toolchain:</strong> Shipped with a built-in package manager (<code>spm</code>), code formatter (<code>spc fmt</code>), test runner (<code>spc test</code>), and Language Server (<code>spls</code>).</li>
      </ul>

      <PrevNextNav
        prev={{
          title: "SP 2.0 Release Overview",
          href: "/sp2/overview",
        }}
        next={{
          title: "Installation Guide",
          href: "/getting-started/installation",
        }}
      />
    </div>
  );
}
