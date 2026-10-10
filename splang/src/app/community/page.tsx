import React from "react";
import { Github, Heart, MessageSquare, Shield, GitPullRequest, Bug, ExternalLink } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const metadata = {
  title: "Community & Contributing",
  description: "Join the SP Programming Language open-source community, contribute to compiler development, and report issues.",
};

export default function CommunityPage() {
  return (
    <div className="py-16 max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 relative">
      {/* Background ambient glow */}
      <div className="absolute top-10 left-1/2 -translate-x-1/2 w-[600px] h-[300px] bg-gradient-to-b from-[#0057FF]/15 to-transparent rounded-full blur-[100px] pointer-events-none -z-10" />

      {/* Header */}
      <div className="text-center max-w-3xl mx-auto mb-16">
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-blue-50 dark:bg-[#0057FF]/15 border border-blue-200 dark:border-[#00D1FF]/30 text-[#0057FF] dark:text-[#00D1FF] text-xs font-semibold mb-3 shadow-sm dark:shadow-[0_0_15px_rgba(0,209,255,0.15)]">
          <Heart className="w-4 h-4 text-rose-500 fill-rose-500/20" />
          <span>Open Source Community</span>
        </div>
        <h1 className="text-4xl sm:text-5xl font-black text-slate-900 dark:text-white tracking-tight">
          Join the SP <span className="text-transparent bg-clip-text bg-gradient-to-r from-blue-600 via-sky-600 to-[#0057FF] dark:from-white dark:via-slate-100 dark:to-[#00D1FF]">Community</span>
        </h1>
        <p className="mt-4 text-base sm:text-lg text-slate-600 dark:text-slate-300">
          SP is an open-source project created and maintained by Shanto Paul alongside passionate developers worldwide. We welcome contributions to the compiler, standard library, developer tools, and documentation.
        </p>
      </div>

      {/* Engagement Channels Grid */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-6 mb-16">
        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 backdrop-blur-md p-6 flex flex-col justify-between shadow-sm dark:shadow-xl hover:border-blue-300 dark:hover:border-[#0057FF]/50 transition-all group">
          <div>
            <div className="w-12 h-12 rounded-xl bg-blue-50 dark:bg-[#0057FF]/15 border border-blue-200 dark:border-[#00D1FF]/30 flex items-center justify-center mb-4 text-[#0057FF] dark:text-[#00D1FF] group-hover:scale-105 group-hover:shadow-md dark:group-hover:shadow-[0_0_15px_rgba(0,209,255,0.25)] transition-all">
              <Github className="w-6 h-6" />
            </div>
            <h3 className="text-lg font-bold text-slate-900 dark:text-white mb-2">GitHub Repository</h3>
            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed mb-6">
              Browse source code, star the repository, track pull requests, and contribute to the C++ / LLVM compiler backend.
            </p>
          </div>
          <a
            href="https://github.com/shantopaul/SP-Language"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button variant="secondary" className="w-full justify-center text-xs">
              <span>View Repository</span>
              <ExternalLink className="w-3.5 h-3.5 ml-1 text-[#0057FF] dark:text-[#00D1FF]" />
            </Button>
          </a>
        </div>

        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 backdrop-blur-md p-6 flex flex-col justify-between shadow-sm dark:shadow-xl hover:border-blue-300 dark:hover:border-[#0057FF]/50 transition-all group">
          <div>
            <div className="w-12 h-12 rounded-xl bg-amber-50 dark:bg-[#0057FF]/15 border border-amber-200 dark:border-[#00D1FF]/30 flex items-center justify-center mb-4 text-amber-600 dark:text-amber-400 group-hover:scale-105 group-hover:shadow-md dark:group-hover:shadow-[0_0_15px_rgba(0,209,255,0.25)] transition-all">
              <Bug className="w-6 h-6" />
            </div>
            <h3 className="text-lg font-bold text-slate-900 dark:text-white mb-2">Issue Tracker</h3>
            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed mb-6">
              Found a bug or have a syntax suggestion? Open an issue on GitHub with reproduction steps and expected output.
            </p>
          </div>
          <a
            href="https://github.com/shantopaul/SP-Language/issues"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button variant="secondary" className="w-full justify-center text-xs">
              <span>Report Issue</span>
              <ExternalLink className="w-3.5 h-3.5 ml-1 text-[#0057FF] dark:text-[#00D1FF]" />
            </Button>
          </a>
        </div>

        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 backdrop-blur-md p-6 flex flex-col justify-between shadow-sm dark:shadow-xl hover:border-blue-300 dark:hover:border-[#0057FF]/50 transition-all group">
          <div>
            <div className="w-12 h-12 rounded-xl bg-indigo-50 dark:bg-[#0057FF]/15 border border-indigo-200 dark:border-[#00D1FF]/30 flex items-center justify-center mb-4 text-indigo-600 dark:text-indigo-400 group-hover:scale-105 group-hover:shadow-md dark:group-hover:shadow-[0_0_15px_rgba(0,209,255,0.25)] transition-all">
              <GitPullRequest className="w-6 h-6" />
            </div>
            <h3 className="text-lg font-bold text-slate-900 dark:text-white mb-2">Pull Requests</h3>
            <p className="text-xs text-slate-600 dark:text-slate-400 leading-relaxed mb-6">
              Read our CONTRIBUTING guide, verify tests in CMake / CTest, and submit PRs with granular commits.
            </p>
          </div>
          <a
            href="https://github.com/shantopaul/SP-Language/pulls"
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button variant="secondary" className="w-full justify-center text-xs">
              <span>Open Pull Request</span>
              <ExternalLink className="w-3.5 h-3.5 ml-1 text-[#0057FF] dark:text-[#00D1FF]" />
            </Button>
          </a>
        </div>
      </div>

      {/* Contributing Guidelines Box */}
      <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 backdrop-blur-md p-6 sm:p-8 mb-16 shadow-sm dark:shadow-2xl relative overflow-hidden">
        <h2 className="text-2xl font-bold text-slate-900 dark:text-white mb-4">How to Contribute</h2>
        <div className="space-y-4 text-sm text-slate-600 dark:text-slate-300">
          <p>
            Contributing to the SP Programming Language is straightforward. Follow these steps to build and verify the compiler locally:
          </p>
          <ol className="list-decimal list-inside space-y-2 text-xs sm:text-sm text-slate-600 dark:text-slate-400 font-mono">
            <li>
              Clone repository: <code className="text-[#0057FF] dark:text-[#00D1FF] bg-slate-100 dark:bg-[#020D21] px-2 py-0.5 rounded border border-slate-200 dark:border-[#0A1F44]">git clone https://github.com/shantopaul/SP-Language.git</code>
            </li>
            <li>
              Configure build: <code className="text-[#0057FF] dark:text-[#00D1FF] bg-slate-100 dark:bg-[#020D21] px-2 py-0.5 rounded border border-slate-200 dark:border-[#0A1F44]">cmake -B build -S . -DCMAKE_BUILD_TYPE=Release</code>
            </li>
            <li>
              Build compiler: <code className="text-[#0057FF] dark:text-[#00D1FF] bg-slate-100 dark:bg-[#020D21] px-2 py-0.5 rounded border border-slate-200 dark:border-[#0A1F44]">cmake --build build --config Release -j 8</code>
            </li>
            <li>
              Execute test suite: <code className="text-[#0057FF] dark:text-[#00D1FF] bg-slate-100 dark:bg-[#020D21] px-2 py-0.5 rounded border border-slate-200 dark:border-[#0A1F44]">ctest --test-dir build --output-on-failure</code>
            </li>
          </ol>
          <p className="text-xs text-slate-500 pt-2">
            Please make sure all 218 test suites pass before submitting your pull request.
          </p>
        </div>
      </div>
    </div>
  );
}
