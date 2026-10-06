import React from "react";
import { Metadata } from "next";
import Link from "next/link";
import { CopyButton } from "@/components/ui/CopyButton";
import {
  Upload,
  FileCode,
  ShieldCheck,
  CheckCircle2,
  Terminal,
  AlertTriangle,
  Key,
  BookOpen,
  ArrowRight,
} from "lucide-react";

export const metadata: Metadata = {
  title: "Package Publishing Guide",
  description:
    "Learn how to create, test, and publish packages to the official SP Package Registry using spm.",
};

export default function PublishPage() {
  const spTomlExample = `[package]
name = "my-awesome-lib"
version = "1.0.0"
authors = ["Your Name <you@example.com>"]
description = "A high-performance algorithm library for SP"
license = "MIT"
readme = "README.md"
repository = "https://github.com/username/my-awesome-lib"
keywords = ["algorithms", "performance", "math"]
category = "utilities"

[dependencies]
json = "^1.2.0"

[dev-dependencies]
test-bench = "^1.0.0"`;

  return (
    <div className="max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 py-12 space-y-12">
      {/* Header */}
      <div>
        <div className="inline-flex items-center gap-2 px-3 py-1 rounded-full border border-cyan-500/30 bg-cyan-950/40 text-cyan-400 text-xs font-mono mb-4">
          <Upload className="w-3.5 h-3.5" />
          <span>Official SPM Publishing Guide</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Publishing to the SP Registry
        </h1>
        <p className="mt-3 text-base text-slate-300 max-w-3xl leading-relaxed">
          The SP Package Registry connects your libraries and tools to developers worldwide. Follow this guide to prepare, test, sign, and publish your SP package.
        </p>
      </div>

      {/* Step 1: sp.toml Manifest */}
      <section className="space-y-4">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-cyan-500/10 border border-cyan-500/30 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm">
            1
          </div>
          <h2 className="text-xl font-bold text-white tracking-tight">
            Configure Package Manifest (<code className="text-cyan-400 font-mono text-base">sp.toml</code>)
          </h2>
        </div>
        <p className="text-sm text-slate-300 leading-relaxed">
          Every SP package requires a valid <code className="text-cyan-400 font-mono">sp.toml</code> in the project root containing metadata, dependencies, license, and repository links.
        </p>

        <div className="rounded-xl border border-slate-800 bg-slate-950 overflow-hidden shadow-lg">
          <div className="flex items-center justify-between px-4 py-2 border-b border-slate-800 bg-slate-900/60 font-mono text-xs text-slate-400">
            <span>sp.toml</span>
            <CopyButton text={spTomlExample} label="Copy Manifest" className="h-6 text-[10px] px-2" />
          </div>
          <pre className="p-4 font-mono text-xs text-slate-200 overflow-x-auto leading-relaxed">
            <code>{spTomlExample}</code>
          </pre>
        </div>
      </section>

      {/* Step 2: Quality & Pre-Publish Checks */}
      <section className="space-y-4">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-cyan-500/10 border border-cyan-500/30 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm">
            2
          </div>
          <h2 className="text-xl font-bold text-white tracking-tight">
            Run Pre-Publication Verification
          </h2>
        </div>
        <p className="text-sm text-slate-300 leading-relaxed">
          Ensure all unit tests pass, code adheres to official formatting, and there are no compiler warnings before publishing.
        </p>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/50 space-y-2">
            <span className="text-xs font-semibold uppercase tracking-wider text-slate-300 font-mono flex items-center gap-1.5">
              <CheckCircle2 className="w-4 h-4 text-emerald-400" />
              <span>1. Run Test Suite</span>
            </span>
            <p className="text-xs text-slate-400">
              Verify 100% of your test cases pass with the native test runner.
            </p>
            <div className="p-2.5 rounded bg-slate-950 font-mono text-xs text-cyan-300 flex items-center justify-between">
              <code>spc test</code>
              <CopyButton text="spc test" className="h-6 text-[10px] px-2" />
            </div>
          </div>

          <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/50 space-y-2">
            <span className="text-xs font-semibold uppercase tracking-wider text-slate-300 font-mono flex items-center gap-1.5">
              <CheckCircle2 className="w-4 h-4 text-emerald-400" />
              <span>2. Code Formatting</span>
            </span>
            <p className="text-xs text-slate-400">
              Check formatting idempotency against the standard style guide.
            </p>
            <div className="p-2.5 rounded bg-slate-950 font-mono text-xs text-cyan-300 flex items-center justify-between">
              <code>spc fmt --check</code>
              <CopyButton text="spc fmt --check" className="h-6 text-[10px] px-2" />
            </div>
          </div>
        </div>
      </section>

      {/* Step 3: Authentication & Token */}
      <section className="space-y-4">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-cyan-500/10 border border-cyan-500/30 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm">
            3
          </div>
          <h2 className="text-xl font-bold text-white tracking-tight">
            Authenticate SPM CLI
          </h2>
        </div>
        <p className="text-sm text-slate-300 leading-relaxed">
          To publish under your developer namespace, authenticate using an API publish token issued by the SP Registry.
        </p>

        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/50 space-y-3">
          <div className="p-2.5 rounded bg-slate-950 font-mono text-xs text-slate-200 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <span className="text-cyan-400 select-none">$</span>
              <span>spm login --token &lt;YOUR_API_TOKEN&gt;</span>
            </div>
            <CopyButton text="spm login --token <YOUR_API_TOKEN>" className="h-6 text-[10px] px-2" />
          </div>
          <p className="text-xs text-slate-400">
            Tokens are securely stored in your local user profile (<code className="text-cyan-400 font-mono">~/.spm/credentials</code>) with restricted file permissions.
          </p>
        </div>
      </section>

      {/* Step 4: Run spm publish */}
      <section className="space-y-4">
        <div className="flex items-center gap-3">
          <div className="w-8 h-8 rounded-lg bg-cyan-500/10 border border-cyan-500/30 text-cyan-400 font-mono font-bold flex items-center justify-center text-sm">
            4
          </div>
          <h2 className="text-xl font-bold text-white tracking-tight">
            Publish Your Package
          </h2>
        </div>
        <p className="text-sm text-slate-300 leading-relaxed">
          Execute the publish command from within the directory containing your <code className="text-cyan-400 font-mono">sp.toml</code>:
        </p>

        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/50 space-y-3">
          <div className="p-2.5 rounded bg-slate-950 font-mono text-xs text-slate-200 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <span className="text-cyan-400 select-none">$</span>
              <span>spm publish</span>
            </div>
            <CopyButton text="spm publish" className="h-6 text-[10px] px-2" />
          </div>

          <div className="rounded-lg bg-slate-950/80 p-3 text-xs font-mono text-slate-300 space-y-1">
            <div className="text-slate-400">Packaging source tarball...</div>
            <div className="text-cyan-400">Computing SHA-256 digest: e3b0c44298fc1c149afbf4c8...</div>
            <div className="text-slate-400">Uploading package to registry.splang.shantopaul.com...</div>
            <div className="text-emerald-400 font-semibold">
              ✓ Successfully published my-awesome-lib@1.0.0
            </div>
          </div>
        </div>
      </section>

      {/* Immutability & Yanking Policy */}
      <section className="rounded-2xl border border-amber-900/40 bg-amber-950/20 p-6 space-y-3">
        <div className="flex items-center gap-2 text-amber-400 font-semibold text-sm">
          <AlertTriangle className="w-5 h-5" />
          <span>Package Immutability & Yank Policy</span>
        </div>
        <p className="text-xs text-slate-300 leading-relaxed">
          To ensure reproducible builds across the globe, packages published to the SP Registry are permanent and immutable. Once version <code className="text-white font-mono">1.0.0</code> is published, its tarball and SHA-256 hash can never be modified or replaced.
        </p>
        <p className="text-xs text-slate-400 leading-relaxed">
          If a critical bug or vulnerability is discovered, you can use <code className="text-amber-300 font-mono">spm yank &lt;version&gt;</code> to prevent new installations while allowing existing <code className="text-white font-mono">sp.lock</code> builds to function uninterrupted.
        </p>
      </section>

      {/* Next Step Links */}
      <div className="pt-6 border-t border-slate-800/80 flex flex-col sm:flex-row items-center justify-between gap-4">
        <Link
          href="/packages"
          className="text-xs text-slate-400 hover:text-white transition-colors flex items-center gap-1 font-mono"
        >
          <span>&larr; Return to package catalog</span>
        </Link>
        <Link
          href="/security"
          className="text-xs text-cyan-400 hover:text-cyan-300 transition-colors flex items-center gap-1 font-mono"
        >
          <span>Learn about Registry Security Architecture &rarr;</span>
        </Link>
      </div>
    </div>
  );
}
