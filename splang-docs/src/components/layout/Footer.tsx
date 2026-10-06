import React from "react";
import { Github, Edit3 } from "lucide-react";

export const DocsFooter: React.FC = () => {
  return (
    <footer className="mt-20 pt-8 border-t border-slate-800 text-xs text-slate-500 flex flex-col sm:flex-row items-center justify-between gap-4">
      <p>© {new Date().getFullYear()} SP Programming Language. Developed by Shanto Paul.</p>
      <div className="flex items-center gap-4">
        <a
          href="https://github.com/shantopaul/SP-Language/edit/main/docs"
          target="_blank"
          rel="noopener noreferrer"
          className="flex items-center gap-1.5 text-slate-400 hover:text-cyan-400 transition-colors"
        >
          <Edit3 className="w-3.5 h-3.5" />
          <span>Edit this page on GitHub</span>
        </a>
        <span>•</span>
        <a
          href="https://splang.shantopaul.com"
          target="_blank"
          rel="noopener noreferrer"
          className="text-slate-400 hover:text-white transition-colors"
        >
          Official Website
        </a>
      </div>
    </footer>
  );
};
