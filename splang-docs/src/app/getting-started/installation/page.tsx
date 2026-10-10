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
  const winPs = `irm https://dl.splang.shantopaul.com/install.ps1 | iex`;
  const winWinget = `winget install SPLang.SP`;
  const unixCurl = `curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh`;
  const verifyCmd = `spc --version
spc doctor
spm --version`;
  const upgradeCli = `spc upgrade`;
  const upgradePs = `irm https://dl.splang.shantopaul.com/install.ps1 | iex`;
  const upgradeUnix = `curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh`;
  const uninstallCli = `spc uninstall`;
  const uninstallPs = `irm https://dl.splang.shantopaul.com/uninstall.ps1 | iex`;
  const uninstallUnix = `curl -fsSL https://dl.splang.shantopaul.com/uninstall.sh | sh`;

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
          The recommended 1-click method on Windows (automatically provisions compiler, precompiled runtimes, and LLVM toolchain):
        </p>
        <CodeBlock code={winPs} language="powershell" />
        <p className="text-xs text-slate-400">
          Or via <strong>winget</strong> (Windows Package Manager):
        </p>
        <CodeBlock code={winWinget} language="powershell" />
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
          Confirm that the compiler (<code>spc</code>), system doctor, and package manager (<code>spm</code>) are operational:
        </p>
        <CodeBlock code={verifyCmd} language="bash" />
        <Callout type="tip" title="System Diagnosis">
          Run <code>spc doctor</code> at any time to verify compiler integrity, standard library modules, and backend toolchain health.
        </Callout>
      </div>

      {/* Upgrading Section */}
      <div className="space-y-3 pt-4 border-t border-slate-800">
        <h2 className="text-xl font-bold text-white">Upgrading SP</h2>
        <p className="text-xs text-slate-400">
          Keep your SP compiler and standard libraries up to date. The upgrade process is in-place, zero-downtime, and automatically preserves your local projects and cached toolchains.
        </p>

        <h3 className="text-sm font-semibold text-slate-200 pt-2">Option 1: Native CLI Command</h3>
        <p className="text-xs text-slate-400">Run the built-in upgrade command from any terminal:</p>
        <CodeBlock code={upgradeCli} language="bash" />

        <h3 className="text-sm font-semibold text-slate-200 pt-2">Option 2: Universal One-Liner Installer</h3>
        <p className="text-xs text-slate-400">
          Running the official installer on a machine with SP already installed automatically detects the existing installation, checks versions, and upgrades binaries in 3–5 seconds without re-downloading toolchains:
        </p>
        <p className="text-xs text-slate-400 font-medium text-slate-300">On Windows (PowerShell):</p>
        <CodeBlock code={upgradePs} language="powershell" />
        <p className="text-xs text-slate-400 font-medium text-slate-300">On Linux &amp; macOS (Bash):</p>
        <CodeBlock code={upgradeUnix} language="bash" />
      </div>

      {/* Uninstallation Section */}
      <div className="space-y-3 pt-4 border-t border-slate-800">
        <h2 className="text-xl font-bold text-white">Uninstalling SP</h2>
        <p className="text-xs text-slate-400">
          SP provides a completely clean uninstaller that removes all binaries, runtimes, environment variables (<code>SP_HOME</code>), and user <code>PATH</code> entries with zero residue.
        </p>

        <h3 className="text-sm font-semibold text-slate-200 pt-2">Option 1: Native CLI Command</h3>
        <p className="text-xs text-slate-400">Run directly from any terminal window:</p>
        <CodeBlock code={uninstallCli} language="bash" />

        <h3 className="text-sm font-semibold text-slate-200 pt-2">Option 2: Online One-Liner Script</h3>
        <p className="text-xs text-slate-400">On Windows (PowerShell):</p>
        <CodeBlock code={uninstallPs} language="powershell" />
        <p className="text-xs text-slate-400">On Linux & macOS (Bash):</p>
        <CodeBlock code={uninstallUnix} language="bash" />

        <h3 className="text-sm font-semibold text-slate-200 pt-2">Option 3: Windows Installed Apps (GUI)</h3>
        <p className="text-xs text-slate-400">
          Navigate to <strong>Windows Settings &rarr; Apps &rarr; Installed apps &rarr; SP Programming Language</strong> and click <strong>Uninstall</strong>, or run <code>~/.sp/uninstall.exe</code>.
        </p>
      </div>

      <PrevNextNav
        prev={{ title: "Introduction", href: "/getting-started" }}
        next={{ title: "First Program", href: "/getting-started/first-program" }}
      />
    </div>
  );
}
