"use client";

import React, { useState } from "react";
import { Check, Copy, Terminal } from "lucide-react";
import { cn } from "@/lib/utils";

interface CodeBlockProps {
  code: string;
  language?: string;
  filename?: string;
  showLineNumbers?: boolean;
  className?: string;
}

export const CodeBlock: React.FC<CodeBlockProps> = ({
  code,
  language = "sp",
  filename,
  showLineNumbers = true,
  className,
}) => {
  const [copied, setCopied] = useState(false);

  const handleCopy = async () => {
    try {
      await navigator.clipboard.writeText(code);
      setCopied(true);
      setTimeout(() => setCopied(false), 2000);
    } catch {
      // ignore
    }
  };

  const lines = code.trim().split("\n");

  const highlightLine = (line: string): React.ReactNode => {
    if (line.trim().startsWith("//") || line.trim().startsWith("///")) {
      return <span className="text-slate-500 italic">{line}</span>;
    }

    const tokens = line.split(
      /(\b(?:import|fn|class|interface|struct|enum|let|var|if|else|match|for|in|while|do|switch|case|default|return|break|continue|new|this|super|public|private|protected|override|virtual|abstract|async|await|comptime|static|const|true|false|null|void|int|double|float|string|bool|byte|char|Array|List|Map|Set|Context|Response|Request|Result|Option)\b|"[^"]*"|'[^']*'|\b\d+(?:\.\d+)?\b|[{}()[\];.,+\-*/=<>!&|?:]+)/g
    );

    return tokens.map((token, i) => {
      if (!token) return null;

      if (
        /^(import|fn|class|interface|struct|enum|let|var|if|else|match|for|in|while|do|switch|case|default|return|break|continue|new|this|super|public|private|protected|override|virtual|abstract|async|await|comptime|static|const)$/.test(
          token
        )
      ) {
        return (
          <span key={i} className="text-[#00D1FF] font-semibold">
            {token}
          </span>
        );
      }

      if (/^(void|int|double|float|string|bool|byte|char|Array|List|Map|Set|Context|Response|Request|Result|Option|true|false|null)$/.test(token)) {
        return (
          <span key={i} className="text-[#60a5fa] font-medium">
            {token}
          </span>
        );
      }

      if (token.startsWith('"') || token.startsWith("'")) {
        if (token.includes("${")) {
          const parts = token.split(/(\$\{[^}]+\})/g);
          return (
            <span key={i} className="text-amber-300">
              {parts.map((p, idx) =>
                p.startsWith("${") ? (
                  <span key={idx} className="text-[#00D1FF] font-bold">
                    {p}
                  </span>
                ) : (
                  p
                )
              )}
            </span>
          );
        }
        return (
          <span key={i} className="text-amber-300">
            {token}
          </span>
        );
      }

      if (/^\d+(?:\.\d+)?$/.test(token)) {
        return (
          <span key={i} className="text-purple-400">
            {token}
          </span>
        );
      }

      if (/^[{}()[\];.,+\-*/=<>!&|?:]+$/.test(token)) {
        return (
          <span key={i} className="text-slate-400">
            {token}
          </span>
        );
      }

      return (
        <span key={i} className="text-slate-200">
          {token}
        </span>
      );
    });
  };

  return (
    <div
      className={cn(
        "rounded-xl border border-[#0A1F44] bg-[#020D21] overflow-hidden shadow-2xl text-xs sm:text-sm font-mono my-5",
        className
      )}
    >
      <div className="flex items-center justify-between px-4 py-2.5 bg-[#081735]/90 border-b border-[#0A1F44]">
        <div className="flex items-center gap-3">
          {/* macOS window control dots */}
          <div className="flex items-center gap-1.5">
            <span className="w-2.5 h-2.5 rounded-full bg-[#ff5f56]/80" />
            <span className="w-2.5 h-2.5 rounded-full bg-[#ffbd2e]/80" />
            <span className="w-2.5 h-2.5 rounded-full bg-[#27c93f]/80" />
          </div>

          {filename ? (
            <span className="text-xs font-sans text-slate-300 font-medium flex items-center gap-1.5 pl-1">
              <Terminal className="w-3.5 h-3.5 text-[#00D1FF]" />
              <span>{filename}</span>
            </span>
          ) : (
            <span className="text-[11px] font-sans uppercase tracking-wider text-slate-400 font-semibold pl-1">
              {language}
            </span>
          )}
        </div>

        <button
          onClick={handleCopy}
          className="flex items-center gap-1.5 px-2.5 py-1.5 rounded-lg text-xs text-slate-400 hover:text-white hover:bg-[#0A1F44] transition-colors min-h-[36px]"
          title="Copy code"
          aria-label="Copy code to clipboard"
        >
          {copied ? (
            <>
              <Check className="w-3.5 h-3.5 text-[#00D1FF]" />
              <span className="text-[#00D1FF] font-medium">Copied</span>
            </>
          ) : (
            <>
              <Copy className="w-3.5 h-3.5" />
              <span>Copy</span>
            </>
          )}
        </button>
      </div>

      <div className="p-3 sm:p-4 overflow-x-auto scrollbar-thin text-xs sm:text-sm">
        <pre className="flex flex-col space-y-1">
          {lines.map((line, idx) => (
            <div key={idx} className="table-row group">
              {showLineNumbers && (
                <span className="table-cell select-none pr-2.5 sm:pr-4 text-right text-slate-600 group-hover:text-slate-500 font-mono text-[11px] sm:text-xs w-6 sm:w-8">
                  {idx + 1}
                </span>
              )}
              <span className="table-cell whitespace-pre">{highlightLine(line)}</span>
            </div>
          ))}
        </pre>
      </div>
    </div>
  );
};
