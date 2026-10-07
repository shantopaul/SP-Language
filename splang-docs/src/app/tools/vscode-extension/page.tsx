import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { Callout } from "@/components/ui/Callout";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "VS Code Extension",
  description: "Official VS Code extension for syntax highlighting, LSP, and debugging in SP.",
};

export default function VsCodeExtensionPage() {
  const winCmd = `irm https://dl.splang.shantopaul.com/sp-vscode.vsix -OutFile "$env:TEMP\\sp-vscode.vsix"; code --install-extension "$env:TEMP\\sp-vscode.vsix"`;
  const unixCmd = `curl -fsSL https://dl.splang.shantopaul.com/sp-vscode.vsix -o /tmp/sp-vscode.vsix && code --install-extension /tmp/sp-vscode.vsix`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Developer Tools", href: "/tools" },
          { title: "VS Code Extension" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Official VS Code Extension</h1>
      <p className="text-sm text-slate-300">
        The official SP Language Support extension delivers full syntax highlighting, TextMate grammar, language server integration, and snippets for Visual Studio Code.
      </p>

      <div className="space-y-3">
        <h2 className="text-sm font-semibold text-slate-200">Option 1: Install via Terminal (One-Liner)</h2>
        <div className="space-y-1">
          <p className="text-xs text-slate-400">Windows (PowerShell):</p>
          <CodeBlock code={winCmd} language="powershell" />
        </div>
        <div className="space-y-1">
          <p className="text-xs text-slate-400">Linux / macOS (Bash):</p>
          <CodeBlock code={unixCmd} language="bash" />
        </div>
      </div>

      <div className="space-y-2 pt-2">
        <h2 className="text-sm font-semibold text-slate-200">Option 2: Direct VSIX Download</h2>
        <p className="text-xs text-slate-400">
          Download the package directly:{" "}
          <a
            href="https://dl.splang.shantopaul.com/sp-vscode.vsix"
            className="text-cyan-400 underline hover:text-cyan-300 font-mono font-medium"
          >
            sp-vscode.vsix (11.8 KB)
          </a>
        </p>
        <p className="text-xs text-slate-400">
          In VS Code: Open <strong>Extensions</strong> (<code>Ctrl+Shift+X</code>) &rarr; Click <strong>...</strong> (top right of Extensions sidebar) &rarr; Select <strong>Install from VSIX...</strong>.
        </p>
      </div>

      <h2 className="text-xl font-bold text-white pt-4">Extension Features</h2>
      <ul className="list-disc list-inside space-y-2 text-xs sm:text-sm text-slate-300">
        <li><strong>TextMate Grammar:</strong> Complete syntax highlighting for keywords, types, decorators, strings, numbers, and doc comments.</li>
        <li><strong>Language Server Integration:</strong> Automatically connects to <code>spls.exe</code> for code completion, hover tooltips, and real-time red squiggly error diagnostics.</li>
        <li><strong>Debug Adapter Protocol:</strong> Configures <code>spd.exe</code> for one-click F5 debugging with breakpoints and variable viewing.</li>
        <li><strong>Snippets:</strong> Quick code snippets for <code>fn</code>, <code>class</code>, <code>struct</code>, <code>match</code>, and <code>test</code> blocks.</li>
      </ul>

      <Callout type="tip" title="Activating after install">
        Press <code>Ctrl+Shift+P</code> &rarr; run <code>Developer: Reload Window</code> to apply syntax highlighting immediately.
      </Callout>

      <PrevNextNav
        prev={{ title: "DAP Debugger (spd)", href: "/tools/debugger" }}
        next={{ title: "7-Language Benchmarks", href: "/benchmarks" }}
      />
    </div>
  );
}
