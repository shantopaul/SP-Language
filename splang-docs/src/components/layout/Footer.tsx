import React from "react";
import { Edit3, ExternalLink } from "lucide-react";
import { ThemeToggle } from "@/components/theme/ThemeToggle";

export const DocsFooter: React.FC = () => {
  return (
    <footer className="mt-20 pt-8 border-t border-slate-200 dark:border-[#0A1F44] text-xs text-slate-500 flex flex-col sm:flex-row items-center justify-between gap-4 transition-colors">
      <p>
        © {new Date().getFullYear()} SP Programming Language (v3.1.0 GA). Created &amp; engineered by{" "}
        <span className="text-slate-800 dark:text-slate-300 font-medium">Shanto Paul</span>.
      </p>
      <div className="flex items-center gap-4">
        <ThemeToggle variant="segmented" />
        <a
          href="https://github.com/shantopaul/SP-Language/tree/main/splang-docs"
          target="_blank"
          rel="noopener noreferrer"
          className="flex items-center gap-1.5 text-slate-500 dark:text-slate-400 hover:text-[#0057FF] dark:hover:text-[#00D1FF] transition-colors"
        >
          <Edit3 className="w-3.5 h-3.5" />
          <span>Edit on GitHub</span>
        </a>
        <span className="text-slate-300 dark:text-[#0A1F44]">•</span>
        <a
          href="https://splang.shantopaul.com"
          target="_blank"
          rel="noopener noreferrer"
          className="flex items-center gap-1 text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white transition-colors"
        >
          <span>Official Website</span>
          <ExternalLink className="w-3 h-3 text-slate-400 dark:text-slate-600" />
        </a>
      </div>
    </footer>
  );
};
