import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "SP Language Server (spls)",
  description: "Official JSON-RPC 2.0 LSP daemon providing IDE features.",
};

export default function LspServerPage() {
  const cliUsage = `# Launch LSP daemon over standard I/O (for IDE integration)
spls --stdio

# Run with verbose diagnostic logging
spls --stdio --log-level debug`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Developer Tools", href: "/tools" },
          { title: "Language Server (spls)" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">SP Language Server (`spls`)</h1>
      <p className="text-sm text-slate-300">
        <code>spls</code> implements the Language Server Protocol (LSP 3.17) over JSON-RPC 2.0. It delivers IDE intelligence to editors like VS Code, Neovim, Emacs, and Helix.
      </p>

      <CodeBlock code={cliUsage} language="bash" />

      <h2 className="text-xl font-bold text-white pt-4">Supported Capabilities</h2>
      <ul className="list-disc list-inside space-y-2 text-xs sm:text-sm text-slate-300">
        <li><strong>Real-Time Diagnostics:</strong> Identifies syntax and type errors on keystroke without saving.</li>
        <li><strong>Autocompletion:</strong> Suggests keywords, function signatures, class members, and stdlib modules.</li>
        <li><strong>Hover Documentation:</strong> Renders docstrings and type signatures when hovering over symbols.</li>
        <li><strong>Go to Definition:</strong> Jumps to symbol declaration across project files.</li>
        <li><strong>Formatting on Save:</strong> Invokes the native formatter engine.</li>
      </ul>

      <PrevNextNav
        prev={{ title: "Doc Generator (spc doc)", href: "/tools/doc-generator" }}
        next={{ title: "DAP Debugger (spd)", href: "/tools/debugger" }}
      />
    </div>
  );
}
