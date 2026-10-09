import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { CodeBlock } from "@/components/ui/CodeBlock";

export const metadata = {
  title: "Installation Guide",
  description: "How to install the SP language toolchain on Windows, Linux, and macOS.",
};

export default function InstallationPage() {
  const winWinget = `winget install SPLang.SP`;
  const winPs = `irm https://dl.splang.shantopaul.com/install.ps1 | iex`;
  const unixCurl = `curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh`;
  const verifyCmd = `spc --version
spm --version`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Getting Started", href: "/getting-started" },
          { title: "Installation" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Installing SP</h1>
      <p className="text-sm text-slate-300">
        Choose your operating system below to install the official SP toolchain.
      </p>

      {/* Windows Section */}
      <div className="space-y-3 pt-2">
        <h2 className="text-xl font-bold text-white">Windows 10 / 11</h2>
        <p className="text-xs text-slate-400">
          The recommended method on Windows is via <strong>winget</strong> (Windows Package Manager):
        </p>
        <CodeBlock code={winWinget} language="powershell" />
        <p className="text-xs text-slate-400">
          Or run the automated PowerShell script:
        </p>
        <CodeBlock code={winPs} language="powershell" />
      </div>

      {/* Linux & macOS Section */}
      <div className="space-y-3 pt-4 border-t border-slate-800">
        <h2 className="text-xl font-bold text-white">Linux & macOS</h2>
        <p className="text-xs text-slate-400">
          Open your terminal and run the one-line shell installer:
        </p>
        <CodeBlock code={unixCurl} language="bash" />
      </div>

      {/* Verification */}
      <div className="space-y-3 pt-4 border-t border-slate-800">
        <h2 className="text-xl font-bold text-white">Verifying Installation</h2>
        <p className="text-xs text-slate-400">
          Confirm that the compiler (<code>spc</code>) and package manager (<code>spm</code>) are available in your PATH:
        </p>
        <CodeBlock code={verifyCmd} language="bash" />
        <Callout type="tip" title="Output Expectation">
          Both commands should return version <code>3.x</code> (e.g. <code>spc version 2.0.0 / 3.1.0</code>).
        </Callout>
      </div>

      <PrevNextNav
        prev={{ title: "Introduction", href: "/getting-started" }}
        next={{ title: "First Program", href: "/getting-started/first-program" }}
      />
    </div>
  );
}
