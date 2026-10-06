"use client";

import React, { useState } from "react";
import { Check, Copy, Terminal } from "lucide-react";
import { cn } from "@/lib/utils";

interface CodeHighlighterProps {
  code: string;
  language?: string;
  filename?: string;
  showLineNumbers?: boolean;
  className?: string;
}

export const CodeHighlighter: React.FC<CodeHighlighterProps> = ({
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
      // fallback
    }
  };

  const lines = code.trim().split("\n");

  // Syntax colorizer for SP Language code
  const highlightLine = (line: string): React.ReactNode => {
    // Comments
    if (line.trim().startsWith("//") || line.trim().startsWith("///")) {
      return <span className="text-emerald-400/80 italic">{line}</span>;
    }

    // Tokenization regex for SP keywords, types, strings, numbers, operators
    const tokens = line.split(
      /(\b(?:import|fn|class|interface|struct|enum|let|var|if|else|match|for|in|while|do|switch|case|default|return|break|continue|new|this|super|public|private|protected|override|virtual|abstract|true|false|null|void|int|double|string|bool|Array|List|Map|Set)\b|"[^"]*"|'[^']*'|\b\d+(?:\.\d+)?\b|[{}()[\];.,+\-*/=<>!&|?:]+)/g
    );

    return tokens.map((token, i) => {
      if (!token) return null;

      // Keywords
      if (
        /^(import|fn|class|interface|struct|enum|let|var|if|else|match|for|in|while|do|switch|case|default|return|break|continue|new|this|super|public|private|protected|override|virtual|abstract)$/.test(
          token
        )
      ) {
        return (
          <span key={i} className="text-cyan-400 font-semibold">
            {token}
          </span>
        );
      }

      // Types & Constants
      if (
        /^(void|int|double|string|bool|Array|List|Map|Set|true|false|null)$/.test(
          token
        )
      ) {
        return (
          <span key={i} className="text-emerald-400 font-medium">
            {token}
          </span>
        );
      }

      // Strings
      if (token.startsWith('"') || token.startsWith("'")) {
        // Handle ${expression} interpolation inside string
        if (token.includes("${")) {
          const parts = token.split(/(\$\{[^}]+\})/g);
          return (
            <span key={i} className="text-amber-300">
              {parts.map((p, idx) =>
                p.startsWith("${") ? (
                  <span key={idx} className="text-cyan-300 font-bold">
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

      // Numbers
      if (/^\d+(?:\.\d+)?$/.test(token)) {
        return (
          <span key={i} className="text-purple-400">
            {token}
          </span>
        );
      }

      // Operators and punctuation
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
        "rounded-xl border border-slate-800 bg-[#0c121e] overflow-hidden shadow-2xl text-xs sm:text-sm font-mono",
        className
      )}
    >
      {/* Code Header Bar */}
      <div className="flex items-center justify-between px-4 py-2.5 bg-slate-900/90 border-b border-slate-800/80">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5">
            <div className="w-3 h-3 rounded-full bg-rose-500/80 border border-rose-600" />
            <div className="w-3 h-3 rounded-full bg-amber-500/80 border border-amber-600" />
            <div className="w-3 h-3 rounded-full bg-emerald-500/80 border border-emerald-600" />
          </div>
          {filename ? (
            <span className="ml-2 text-xs font-sans text-slate-400 font-medium flex items-center gap-1.5">
              <Terminal className="w-3.5 h-3.5 text-slate-500" />
              {filename}
            </span>
          ) : (
            <span className="ml-2 text-[11px] font-sans uppercase tracking-wider text-slate-500 font-semibold">
              {language}
            </span>
          )}
        </div>
        <button
          onClick={handleCopy}
          className="flex items-center gap-1.5 px-2 py-1 rounded text-xs text-slate-400 hover:text-white hover:bg-slate-800 transition-colors"
          title="Copy to clipboard"
        >
          {copied ? (
            <>
              <Check className="w-3.5 h-3.5 text-emerald-400" />
              <span className="text-emerald-400">Copied</span>
            </>
          ) : (
            <>
              <Copy className="w-3.5 h-3.5" />
              <span>Copy</span>
            </>
          )}
        </button>
      </div>

      {/* Code Content */}
      <div className="p-4 overflow-x-auto">
        <pre className="flex flex-col space-y-1">
          {lines.map((line, index) => (
            <div key={index} className="table-row group">
              {showLineNumbers && (
                <span className="table-cell select-none pr-4 text-right text-slate-600 group-hover:text-slate-500 font-mono text-xs w-8">
                  {index + 1}
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
