"use client";

import React from "react";
import { CopyButton } from "@/components/ui/CopyButton";

interface ReadmeViewerProps {
  content: string;
}

export const ReadmeViewer: React.FC<ReadmeViewerProps> = ({ content }) => {
  // Simple, robust Markdown parser tailored for READMEs
  const lines = content.split("\n");
  const elements: React.ReactNode[] = [];

  let inCodeBlock = false;
  let codeBuffer: string[] = [];
  let codeLanguage = "";

  lines.forEach((line, index) => {
    if (line.startsWith("```")) {
      if (!inCodeBlock) {
        inCodeBlock = true;
        codeLanguage = line.slice(3).trim();
        codeBuffer = [];
      } else {
        inCodeBlock = false;
        const codeText = codeBuffer.join("\n");
        elements.push(
          <div
            key={`code-${index}`}
            className="my-4 rounded-xl border border-slate-800 bg-slate-950 overflow-hidden shadow-lg"
          >
            <div className="flex items-center justify-between px-3.5 py-1.5 border-b border-slate-800/80 bg-slate-900/60 text-[11px] font-mono text-slate-400">
              <span>{codeLanguage || "text"}</span>
              <CopyButton text={codeText} label="Copy" className="h-6 text-[10px] px-2" />
            </div>
            <pre className="p-4 font-mono text-xs text-slate-200 overflow-x-auto leading-relaxed">
              <code>{codeText}</code>
            </pre>
          </div>
        );
      }
      return;
    }

    if (inCodeBlock) {
      codeBuffer.push(line);
      return;
    }

    const trimmed = line.trim();
    if (!trimmed) {
      return;
    }

    // Headings
    if (trimmed.startsWith("# ")) {
      elements.push(
        <h1
          key={`h1-${index}`}
          className="text-2xl font-bold text-white mt-6 mb-3 font-mono border-b border-slate-800 pb-2"
        >
          {trimmed.slice(2)}
        </h1>
      );
      return;
    }
    if (trimmed.startsWith("## ")) {
      elements.push(
        <h2
          key={`h2-${index}`}
          className="text-xl font-semibold text-slate-100 mt-5 mb-2 font-mono flex items-center gap-2"
        >
          <span className="text-cyan-400">#</span> {trimmed.slice(3)}
        </h2>
      );
      return;
    }
    if (trimmed.startsWith("### ")) {
      elements.push(
        <h3
          key={`h3-${index}`}
          className="text-base font-medium text-slate-200 mt-4 mb-2 font-mono"
        >
          {trimmed.slice(4)}
        </h3>
      );
      return;
    }

    // Lists
    if (trimmed.startsWith("- ")) {
      elements.push(
        <li key={`li-${index}`} className="ml-5 list-disc text-sm text-slate-300 my-1 leading-relaxed">
          {trimmed.slice(2)}
        </li>
      );
      return;
    }

    // Normal Paragraph
    elements.push(
      <p key={`p-${index}`} className="text-sm text-slate-300 my-2 leading-relaxed">
        {trimmed}
      </p>
    );
  });

  return (
    <article className="prose prose-invert max-w-none text-slate-300">
      {elements}
    </article>
  );
};
