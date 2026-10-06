import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Debug Adapter Protocol (spd)",
  description: "Native DAP debugger daemon for breakpoints, stepping, and inspection in SP.",
};

export default function DebuggerPage() {
  const launchConfig = `{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "Debug SP Program",
      "type": "sp",
      "request": "launch",
      "program": "\${workspaceFolder}/src/main.sp",
      "stopOnEntry": false
    }
  ]
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Developer Tools", href: "/tools" },
          { title: "DAP Debugger (spd)" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">Debug Adapter Protocol (`spd`)</h1>
      <p className="text-sm text-slate-300">
        <code>spd</code> is the official Debug Adapter Protocol daemon. It translates VS Code and editor debug commands into native LLVM runtime execution breakpoints.
      </p>

      <h2 className="text-xl font-bold text-white pt-4">VS Code `launch.json` Setup</h2>
      <CodeBlock code={launchConfig} filename=".vscode/launch.json" language="json" />

      <h2 className="text-xl font-bold text-white pt-4">Features Supported</h2>
      <ul className="list-disc list-inside space-y-2 text-xs sm:text-sm text-slate-300">
        <li><strong>Breakpoints:</strong> Line breakpoints and conditional pause execution.</li>
        <li><strong>Stepping:</strong> Step Over, Step Into, and Step Out.</li>
        <li><strong>Scopes & Variables:</strong> Local stack frame variable inspection with type labels.</li>
        <li><strong>Call Stack:</strong> Multi-frame call stack traceback with source mapping.</li>
      </ul>

      <PrevNextNav
        prev={{ title: "Language Server (spls)", href: "/tools/lsp-server" }}
        next={{ title: "VS Code Extension", href: "/tools/vscode-extension" }}
      />
    </div>
  );
}
