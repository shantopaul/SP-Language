import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { CodeBlock } from "@/components/ui/CodeBlock";

export const metadata = {
  title: "Project Scaffolding (spm)",
  description: "Scaffolding multi-file applications and libraries using the SP Package Manager (spm).",
};

export default function ProjectSetupPage() {
  const initCmd = `spm init my_app`;
  const tomlExample = `[package]
name = "my_app"
version = "0.1.0"
authors = ["Your Name <you@example.com>"]
license = "MIT"

[dependencies]
json = "^1.2.0"
http = "^1.1.0"`;

  const buildRunCmd = `cd my_app
spm build
spm run`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Getting Started", href: "/getting-started" },
          { title: "Project Scaffolding" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Project Scaffolding with SPM</h1>
      <p className="text-sm text-slate-300">
        SP includes a modern package and project manager named <strong>spm</strong>. It creates standardized project directories, manages dependencies with Semantic Versioning, and produces deterministic lockfiles.
      </p>

      <h2 className="text-xl font-bold text-white pt-2">1. Initialize a New Project</h2>
      <CodeBlock code={initCmd} language="bash" />
      <p className="text-xs text-slate-400">
        This generates the following directory layout:
      </p>
      <pre className="bg-slate-900/60 p-4 rounded-xl border border-slate-800 font-mono text-xs text-slate-300">
{`my_app/
├── sp.toml            # Project configuration and dependency manifest
├── src/
│   └── main.sp        # Application entrypoint
└── tests/
    └── app_test.sp    # Embedded test blocks`}
      </pre>

      <h2 className="text-xl font-bold text-white pt-4">2. The `sp.toml` Manifest</h2>
      <p className="text-xs text-slate-400">
        Dependencies are declared in standard TOML syntax:
      </p>
      <CodeBlock code={tomlExample} filename="sp.toml" />

      <h2 className="text-xl font-bold text-white pt-4">3. Building and Running</h2>
      <CodeBlock code={buildRunCmd} language="bash" />

      <Callout type="note" title="Lockfile Generation">
        When you run <code>spm build</code> or <code>spm install</code>, spm automatically generates <code>sp.lock</code> containing cryptographic SHA-256 digests of all resolved packages for reproducible team builds.
      </Callout>

      <PrevNextNav
        prev={{ title: "First Program", href: "/getting-started/first-program" }}
        next={{ title: "Syntax Overview", href: "/syntax-guide" }}
      />
    </div>
  );
}
