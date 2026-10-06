import React from "react";
import { Metadata } from "next";
import Link from "next/link";
import { CopyButton } from "@/components/ui/CopyButton";
import {
  ShieldCheck,
  Lock,
  FileCheck2,
  AlertOctagon,
  Terminal,
  CheckCircle2,
  Mail,
  Fingerprint,
} from "lucide-react";

export const metadata: Metadata = {
  title: "Security & Cryptographic Integrity",
  description:
    "Learn how the SP Package Registry protects the software supply chain through SHA-256 digests, lockfile enforcement, and immutable tarballs.",
};

export default function SecurityPage() {
  const sampleLockfile = `[[package]]
name = "crypto"
version = "1.0.4"
source = "registry+https://registry.splang.shantopaul.com"
checksum = "c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01234"
dependencies = []

[[package]]
name = "http"
version = "1.1.0"
source = "registry+https://registry.splang.shantopaul.com"
checksum = "b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01"
dependencies = [
    "crypto 1.0.4",
    "json 1.2.0"
]`;

  return (
    <div className="max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 py-12 space-y-12">
      {/* Header */}
      <div>
        <div className="inline-flex items-center gap-2 px-3 py-1 rounded-full border border-emerald-500/30 bg-emerald-950/40 text-emerald-400 text-xs font-mono mb-4">
          <ShieldCheck className="w-3.5 h-3.5" />
          <span>Supply Chain Security Protocol</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Security & Cryptographic Integrity
        </h1>
        <p className="mt-3 text-base text-slate-300 max-w-3xl leading-relaxed">
          The SP Package Registry is engineered with strict cryptographic verification to defend against supply chain tampering, unauthorized substitutions, and dependency confusion attacks.
        </p>
      </div>

      {/* Core Principles */}
      <section className="grid grid-cols-1 md:grid-cols-3 gap-6">
        <div className="p-6 rounded-2xl border border-slate-800 bg-slate-900/40 space-y-3">
          <div className="w-10 h-10 rounded-xl bg-cyan-500/10 border border-cyan-500/30 text-cyan-400 flex items-center justify-center">
            <Fingerprint className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white">SHA-256 Digest Verification</h3>
          <p className="text-xs text-slate-400 leading-relaxed">
            Every published archive is indexed with an authoritative SHA-256 hash. SPM recomputes the hash on download and halts immediately if there is any mismatch.
          </p>
        </div>

        <div className="p-6 rounded-2xl border border-slate-800 bg-slate-900/40 space-y-3">
          <div className="w-10 h-10 rounded-xl bg-emerald-500/10 border border-emerald-500/30 text-emerald-400 flex items-center justify-center">
            <FileCheck2 className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white">Deterministic Lockfiles</h3>
          <p className="text-xs text-slate-400 leading-relaxed">
            Every project generates an immutable <code className="text-cyan-400 font-mono">sp.lock</code> containing exact version pins and cryptographic checksums for CI/CD consistency.
          </p>
        </div>

        <div className="p-6 rounded-2xl border border-slate-800 bg-slate-900/40 space-y-3">
          <div className="w-10 h-10 rounded-xl bg-indigo-500/10 border border-indigo-500/30 text-indigo-400 flex items-center justify-center">
            <Lock className="w-5 h-5" />
          </div>
          <h3 className="text-base font-bold text-white">Zero Script Execution</h3>
          <p className="text-xs text-slate-400 leading-relaxed">
            Unlike other ecosystems, SPM does NOT permit arbitrary pre-install or post-install shell script execution during dependency downloads.
          </p>
        </div>
      </section>

      {/* Example sp.lock */}
      <section className="space-y-4">
        <h2 className="text-xl font-bold text-white tracking-tight flex items-center gap-2">
          <span>Deterministic Lockfile Specification (</span>
          <code className="text-cyan-400 font-mono text-lg">sp.lock</code>
          <span>)</span>
        </h2>
        <p className="text-sm text-slate-300 leading-relaxed">
          The SP lockfile records the authoritative source and SHA-256 checksum for every direct and indirect dependency:
        </p>

        <div className="rounded-xl border border-slate-800 bg-slate-950 overflow-hidden shadow-lg">
          <div className="flex items-center justify-between px-4 py-2 border-b border-slate-800 bg-slate-900/60 font-mono text-xs text-slate-400">
            <span>sp.lock</span>
            <CopyButton text={sampleLockfile} label="Copy Lockfile" className="h-6 text-[10px] px-2" />
          </div>
          <pre className="p-4 font-mono text-xs text-slate-200 overflow-x-auto leading-relaxed">
            <code>{sampleLockfile}</code>
          </pre>
        </div>
      </section>

      {/* Integrity Verification Commands */}
      <section className="space-y-4">
        <h2 className="text-xl font-bold text-white tracking-tight flex items-center gap-2">
          <Terminal className="w-5 h-5 text-cyan-400" />
          <span>Verifying Package Integrity Locally</span>
        </h2>
        <p className="text-sm text-slate-300 leading-relaxed">
          You can inspect and verify local cache integrity at any time using native SPM commands:
        </p>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
          <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/50 space-y-2">
            <span className="text-xs font-semibold uppercase tracking-wider text-slate-300 font-mono flex items-center gap-1.5">
              <CheckCircle2 className="w-4 h-4 text-emerald-400" />
              <span>Verify Specific Package</span>
            </span>
            <p className="text-xs text-slate-400">
              Validates downloaded tarball against the registry API checksum.
            </p>
            <div className="p-2.5 rounded bg-slate-950 font-mono text-xs text-cyan-300 flex items-center justify-between">
              <code>spm verify crypto 1.0.4</code>
              <CopyButton text="spm verify crypto 1.0.4" className="h-6 text-[10px] px-2" />
            </div>
          </div>

          <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/50 space-y-2">
            <span className="text-xs font-semibold uppercase tracking-wider text-slate-300 font-mono flex items-center gap-1.5">
              <CheckCircle2 className="w-4 h-4 text-emerald-400" />
              <span>Audit Dependency Tree</span>
            </span>
            <p className="text-xs text-slate-400">
              Scans all active project dependencies for known security advisories.
            </p>
            <div className="p-2.5 rounded bg-slate-950 font-mono text-xs text-cyan-300 flex items-center justify-between">
              <code>spm audit</code>
              <CopyButton text="spm audit" className="h-6 text-[10px] px-2" />
            </div>
          </div>
        </div>
      </section>

      {/* Reporting Vulnerabilities */}
      <section className="rounded-2xl border border-slate-800 bg-gradient-to-br from-slate-900/60 to-slate-950 p-6 sm:p-8 space-y-4">
        <div className="flex items-center gap-2.5 text-white font-bold text-lg">
          <Mail className="w-5 h-5 text-cyan-400" />
          <span>Responsible Vulnerability Disclosure</span>
        </div>
        <p className="text-xs sm:text-sm text-slate-300 leading-relaxed max-w-3xl">
          If you discover a security issue or vulnerability in any package published to the SP Registry, please contact our Security Working Group before public disclosure. We coordinate with package authors to issue patched releases swiftly.
        </p>
        <div className="pt-2 flex items-center gap-3">
          <a
            href="mailto:security@splang.shantopaul.com"
            className="px-4 py-2 rounded-lg bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-semibold text-xs font-mono transition-colors"
          >
            security@splang.shantopaul.com
          </a>
          <span className="text-xs text-slate-400">PGP Key ID: <code className="text-slate-300 font-mono">0x4F8E29C1</code></span>
        </div>
      </section>
    </div>
  );
}
