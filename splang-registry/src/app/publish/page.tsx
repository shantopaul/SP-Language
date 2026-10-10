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
        <div className="inline-flex items-center gap-2 px-3.5 py-1.5 rounded-full border border-[#00D1FF]/40 bg-[#0057FF]/15 text-[#00D1FF] text-xs font-mono mb-4 shadow-[0_0_12px_rgba(0,209,255,0.2)]">
          <Upload className="w-3.5 h-3.5" />
          <span>Official SPM Publishing Guide</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-black text-white tracking-tight">
          Publishing to the SP Registry
        </h1>
        <p className="mt-3 text-base text-slate-300 max-w-3xl leading-relaxed">
          The SP Package Registry connects your libraries and tools to developers worldwide. Follow this guide to prepare, test, sign, and publish your SP package.
        </p>
      </div>

      {/* Step 1: sp.toml Manifest */}
      <section className="space-y-4">
        <div className="flex items-center gap-3">
          <div className="w-9 h-9 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/40 text-[#00D1FF] font-mono font-bold flex items-center justify-center text-sm shadow-[0_0_12px_rgba(0,209,255,0.2)]">
            1
          </div>
          <h2 className="text-xl font-bold text-white tracking-tight">
            Configure Package Manifest (<code className="text-[#00D1FF] font-mono text-base">sp.toml</code>)
          </h2>
        </div>
        <p className="text-sm text-slate-300 leading-relaxed">
          Every SP package requires a valid <code className="text-[#00D1FF] font-mono">sp.toml</code> in the project root containing metadata, dependencies, license, and repository links.
        </p>

        <div className="rounded-2xl border border-[#0A1F44] bg-[#020D21] overflow-hidden shadow-xl">
          <div className="flex items-center justify-between px-4 py-2.5 border-b border-[#0A1F44] bg-[#081735]/70 font-mono text-xs">
            <div className="flex items-center gap-2">
              <div className="flex items-center gap-1.5">
                <div className="w-2.5 h-2.5 rounded-full bg-[#ef4444]" />
                <div className="w-2.5 h-2.5 rounded-full bg-[#f59e0b]" />
                <div className="w-2.5 h-2.5 rounded-full bg-[#10b981]" />
              </div>
              <span className="text-[#00D1FF] font-semibold ml-2">sp.toml</span>
            </div>
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
          <div className="w-9 h-9 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/40 text-[#00D1FF] font-mono font-bold flex items-center justify-center text-sm shadow-[0_0_12px_rgba(0,209,255,0.2)]">
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
          <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 space-y-2.5 shadow-lg">
            <span className="text-xs font-bold uppercase tracking-wider text-slate-200 font-mono flex items-center gap-2">
              <CheckCircle2 className="w-4 h-4 text-[#00D1FF]" />
              <span>1. Run Test Suite</span>
            </span>
            <p className="text-xs text-slate-400">
              Verify 100% of your test cases pass with the native test runner.
            </p>
            <div className="p-3 rounded-xl bg-[#020D21] border border-[#0A1F44] font-mono text-xs text-[#00D1FF] flex items-center justify-between">
              <code>spc test</code>
              <CopyButton text="spc test" className="h-6 text-[10px] px-2" />
            </div>
          </div>

          <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 space-y-2.5 shadow-lg">
            <span className="text-xs font-bold uppercase tracking-wider text-slate-200 font-mono flex items-center gap-2">
              <CheckCircle2 className="w-4 h-4 text-[#00D1FF]" />
              <span>2. Code Formatting</span>
            </span>
            <p className="text-xs text-slate-400">
              Check formatting idempotency against the standard style guide.
            </p>
            <div className="p-3 rounded-xl bg-[#020D21] border border-[#0A1F44] font-mono text-xs text-[#00D1FF] flex items-center justify-between">
              <code>spc fmt --check</code>
              <CopyButton text="spc fmt --check" className="h-6 text-[10px] px-2" />
            </div>
          </div>
        </div>
      </section>

      {/* Step 3: Authentication & Token */}
      <section className="space-y-4">
        <div className="flex items-center gap-3">
          <div className="w-9 h-9 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/40 text-[#00D1FF] font-mono font-bold flex items-center justify-center text-sm shadow-[0_0_12px_rgba(0,209,255,0.2)]">
            3
          </div>
          <h2 className="text-xl font-bold text-white tracking-tight">
            Authenticate SPM CLI
          </h2>
        </div>
        <p className="text-sm text-slate-300 leading-relaxed">
          To publish under your developer namespace, authenticate using an API publish token issued by the SP Registry.
        </p>

        <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 space-y-3 shadow-lg">
          <div className="p-3 rounded-xl bg-[#020D21] border border-[#0A1F44] font-mono text-xs text-slate-200 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <span className="text-[#00D1FF] select-none font-bold">$</span>
              <span>spm login --token &lt;YOUR_API_TOKEN&gt;</span>
            </div>
            <CopyButton text="spm login --token <YOUR_API_TOKEN>" className="h-6 text-[10px] px-2" />
          </div>
          <p className="text-xs text-slate-400">
            Tokens are securely stored in your local user profile (<code className="text-[#00D1FF] font-mono">~/.spm/credentials</code>) with restricted file permissions.
          </p>
        </div>
      </section>

      {/* Step 4: Run spm publish */}
      <section className="space-y-4">
        <div className="flex items-center gap-3">
          <div className="w-9 h-9 rounded-xl bg-[#0057FF]/20 border border-[#00D1FF]/40 text-[#00D1FF] font-mono font-bold flex items-center justify-center text-sm shadow-[0_0_12px_rgba(0,209,255,0.2)]">
            4
          </div>
          <h2 className="text-xl font-bold text-white tracking-tight">
            Publish Your Package
          </h2>
        </div>
        <p className="text-sm text-slate-300 leading-relaxed">
          Execute the publish command from within the directory containing your <code className="text-[#00D1FF] font-mono">sp.toml</code>:
        </p>

        <div className="p-5 rounded-2xl border border-[#0A1F44] bg-[#081735]/60 space-y-3 shadow-lg">
          <div className="p-3 rounded-xl bg-[#020D21] border border-[#0A1F44] font-mono text-xs text-slate-200 flex items-center justify-between">
            <div className="flex items-center gap-2">
              <span className="text-[#00D1FF] select-none font-bold">$</span>
              <span>spm publish</span>
            </div>
            <CopyButton text="spm publish" className="h-6 text-[10px] px-2" />
          </div>

          <div className="rounded-xl bg-[#020D21] border border-[#0A1F44] p-4 text-xs font-mono text-slate-300 space-y-1.5">
            <div className="text-slate-400">Packaging source tarball...</div>
            <div className="text-[#00D1FF]">Computing SHA-256 digest: e3b0c44298fc1c149afbf4c8...</div>
            <div className="text-slate-400">Uploading package to register.splang.shantopaul.com...</div>
            <div className="text-[#00D1FF] font-bold">
              ✓ Successfully published my-awesome-lib@1.0.0
            </div>
          </div>
        </div>
      </section>

      {/* Immutability & Yanking Policy */}
      <section className="rounded-2xl border border-amber-900/40 bg-amber-950/20 p-6 space-y-3 shadow-lg">
        <div className="flex items-center gap-2 text-amber-400 font-bold text-sm">
          <AlertTriangle className="w-5 h-5" />
          <span>Package Immutability &amp; Yank Policy</span>
        </div>
        <p className="text-xs text-slate-300 leading-relaxed">
          To ensure reproducible builds across the globe, packages published to the SP Registry are permanent and immutable. Once version <code className="text-white font-mono">1.0.0</code> is published, its tarball and SHA-256 hash can never be modified or replaced.
        </p>
        <p className="text-xs text-slate-400 leading-relaxed">
          If a critical bug or vulnerability is discovered, you can use <code className="text-amber-300 font-mono">spm yank &lt;version&gt;</code> to prevent new installations while allowing existing <code className="text-white font-mono">sp.lock</code> builds to function uninterrupted.
        </p>
      </section>

      {/* Next Step Links */}
      <div className="pt-6 border-t border-[#0A1F44] flex flex-col sm:flex-row items-center justify-between gap-4">
        <Link
          href="/packages"
          className="text-xs text-slate-400 hover:text-white transition-colors flex items-center gap-1 font-mono"
        >
          <span>&larr; Return to package catalog</span>
        </Link>
        <Link
          href="/security"
          className="text-xs text-[#00D1FF] hover:underline transition-colors flex items-center gap-1 font-mono font-semibold"
        >
          <span>Learn about Registry Security Architecture &rarr;</span>
        </Link>
      </div>
    </div>
  );
}
