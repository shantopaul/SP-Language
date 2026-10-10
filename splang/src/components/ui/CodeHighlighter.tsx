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

  // Syntax colorizer for SP Language code matching official brand palette
  const highlightLine = (line: string): React.ReactNode => {
    // Comments
    if (line.trim().startsWith("//") || line.trim().startsWith("///")) {
      return <span className="text-slate-400/80 italic">{line}</span>;
    }

    // Tokenization regex for SP keywords, types, strings, numbers, operators
    const tokens = line.split(
      /(\b(?:import|fn|class|interface|struct|enum|let|var|if|else|match|for|in|while|do|switch|case|default|return|break|continue|new|this|super|public|private|protected|override|virtual|abstract|true|false|null|void|int|double|string|bool|Array|List|Map|Set|comptime|async|await)\b|"[^"]*"|'[^']*'|\b\d+(?:\.\d+)?\b|[{}()[\];.,+\-*/=<>!&|?:]+)/g
    );

    return tokens.map((token, i) => {
      if (!token) return null;

      // Core Keywords (Electric Cyan)
      if (
        /^(import|fn|class|interface|struct|enum|let|var|if|else|match|for|in|while|do|switch|case|default|return|break|continue|new|this|super|public|private|protected|override|virtual|abstract|comptime|async|await)$/.test(
          token
        )
      ) {
        return (
          <span key={i} className="text-[#00D1FF] font-semibold">
            {token}
          </span>
        );
      }

      // Types & Constants (Sky & Electric Blue)
      if (
        /^(void|int|double|string|bool|Array|List|Map|Set|true|false|null)$/.test(
          token
        )
      ) {
        return (
          <span key={i} className="text-blue-300 font-medium">
            {token}
          </span>
        );
      }

      // Strings (Warm Amber)
      if (token.startsWith('"') || token.startsWith("'")) {
        // Handle ${expression} interpolation inside string
        if (token.includes("${")) {
          const parts = token.split(/(\$\{[^}]+\})/g);
          return (
            <span key={i} className="text-amber-200">
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
          <span key={i} className="text-amber-200">
            {token}
          </span>
        );
      }

      // Numbers (Purple Accent)
      if (/^\d+(?:\.\d+)?$/.test(token)) {
        return (
          <span key={i} className="text-indigo-300">
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
        <span key={i} className="text-slate-100">
          {token}
        </span>
      );
    });
  };

  return (
    <div
      className={cn(
        "rounded-2xl border border-[#0A1F44] bg-[#020D21]/95 overflow-hidden shadow-2xl text-xs sm:text-sm font-mono backdrop-blur-xl",
        className
      )}
    >
      {/* Code Header Bar */}
      <div className="flex items-center justify-between px-4 py-3 bg-[#081735]/90 border-b border-[#0A1F44]">
        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5">
            <div className="w-3 h-3 rounded-full bg-rose-500/80 border border-rose-600/60" />
            <div className="w-3 h-3 rounded-full bg-amber-500/80 border border-amber-600/60" />
            <div className="w-3 h-3 rounded-full bg-emerald-500/80 border border-emerald-600/60" />
          </div>
          {filename ? (
            <span className="ml-2 text-xs font-mono text-cyan-300 font-medium flex items-center gap-1.5 bg-[#020D21]/80 px-2.5 py-1 rounded-md border border-[#0A1F44]">
              <Terminal className="w-3.5 h-3.5 text-cyan-400" />
              {filename}
            </span>
          ) : (
            <span className="ml-2 text-[11px] font-sans uppercase tracking-wider text-slate-400 font-semibold">
              {language}
            </span>
          )}
        </div>
        <button
          onClick={handleCopy}
          className="flex items-center gap-1.5 px-2.5 py-1 rounded-lg text-xs text-slate-300 hover:text-white hover:bg-[#0A1F44] transition-colors border border-transparent hover:border-[#0A1F44]"
          title="Copy code"
        >
          {copied ? (
            <>
              <Check className="w-3.5 h-3.5 text-cyan-400" />
              <span className="text-cyan-400 font-sans">Copied</span>
            </>
          ) : (
            <>
              <Copy className="w-3.5 h-3.5" />
              <span className="font-sans">Copy</span>
            </>
          )}
        </button>
      </div>

      {/* Code Content */}
      <div className="p-4 sm:p-5 overflow-x-auto">
        <pre className="flex flex-col space-y-1.5 leading-relaxed">
          {lines.map((line, index) => (
            <div key={index} className="table-row group">
              {showLineNumbers && (
                <span className="table-cell select-none pr-4 text-right text-slate-600 group-hover:text-cyan-400/60 font-mono text-xs w-8 transition-colors">
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
