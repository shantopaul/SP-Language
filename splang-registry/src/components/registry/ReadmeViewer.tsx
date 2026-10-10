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
            className="my-5 rounded-2xl border border-slate-300 dark:border-[#0A1F44] bg-[#020D21] overflow-hidden shadow-sm dark:shadow-xl"
          >
            <div className="flex items-center justify-between px-4 py-2 border-b border-slate-700 dark:border-[#0A1F44] bg-[#081735]/90 text-[11px] font-mono">
              <div className="flex items-center gap-2">
                <div className="flex items-center gap-1.5">
                  <div className="w-2.5 h-2.5 rounded-full bg-[#ef4444]" />
                  <div className="w-2.5 h-2.5 rounded-full bg-[#f59e0b]" />
                  <div className="w-2.5 h-2.5 rounded-full bg-[#10b981]" />
                </div>
                <span className="text-[#60A5FA] dark:text-[#00D1FF] font-semibold ml-2">{codeLanguage || "sp"}</span>
              </div>
              <CopyButton text={codeText} label="Copy" className="h-6 text-[10px] px-2" />
            </div>
            <pre className="p-4 font-mono text-xs text-slate-100 overflow-x-auto leading-relaxed">
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
          className="text-2xl font-black text-slate-900 dark:text-white mt-6 mb-3 font-mono border-b border-slate-200 dark:border-[#0A1F44] pb-2 tracking-tight"
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
          className="text-xl font-bold text-slate-900 dark:text-white mt-5 mb-2 font-mono flex items-center gap-2"
        >
          <span className="text-[#0057FF] dark:text-[#00D1FF]">#</span> {trimmed.slice(3)}
        </h2>
      );
      return;
    }
    if (trimmed.startsWith("### ")) {
      elements.push(
        <h3
          key={`h3-${index}`}
          className="text-base font-semibold text-slate-800 dark:text-slate-200 mt-4 mb-2 font-mono"
        >
          {trimmed.slice(4)}
        </h3>
      );
      return;
    }

    // Lists
    if (trimmed.startsWith("- ")) {
      elements.push(
        <li key={`li-${index}`} className="ml-5 list-disc text-sm text-slate-700 dark:text-slate-300 my-1 leading-relaxed">
          {trimmed.slice(2)}
        </li>
      );
      return;
    }

    // Normal Paragraph
    elements.push(
      <p key={`p-${index}`} className="text-sm text-slate-700 dark:text-slate-300 my-2 leading-relaxed">
        {trimmed}
      </p>
    );
  });

  return (
    <article className="prose dark:prose-invert max-w-none text-slate-700 dark:text-slate-300">
      {elements}
    </article>
  );
};
