import React from "react";
import Link from "next/link";
import { ArrowLeft, Calendar, Zap } from "lucide-react";
import { Badge } from "@/components/ui/Badge";
import { CodeHighlighter } from "@/components/ui/CodeHighlighter";
import { Button } from "@/components/ui/Button";

export const metadata = {
  title: "Announcing SP 4.0.0 GA: ARC Memory Safety, Async Epoll Loop & FFI Bindgen",
  description:
    "Official launch of SP 4.0.0 GA featuring deterministic ARC with weak cycle breaking, Result/Option with ?, real async/await over epoll, spc bindgen, and full LSP/DAP depth.",
};

export default function Release400BlogPostPage() {
  const sampleCode = `import std.io;
import std.web;

async fn fetch_status() -> Result<string, string> {
    return Ok("SP 4.0.0 GA ready");
}

fn main() -> void {
    let app = web.new_server();
    app.get("/health", (ctx: &web.Context) => {
        return ctx.json("{\\"status\\":\\"ok\\",\\"version\\":\\"4.0.0\\"}");
    });

    println("[SP 4.0.0] Server listening on http://0.0.0.0:8080");
    app.listen(8080);
}`;

  return (
    <div className="py-16 max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 relative">
      {/* Background ambient glow */}
      <div className="absolute top-10 left-1/2 -translate-x-1/2 w-[700px] h-[350px] bg-gradient-to-b from-[#0057FF]/15 to-transparent rounded-full blur-[100px] pointer-events-none -z-10" />

      {/* Back button */}
      <div className="mb-8">
        <Link
          href="/blog"
          className="inline-flex items-center gap-2 text-xs font-semibold text-slate-400 hover:text-[#00D1FF] transition-colors"
        >
          <ArrowLeft className="w-4 h-4" />
          <span>Back to All Articles</span>
        </Link>
      </div>

      {/* Header */}
      <header className="mb-12 border-b border-slate-200 dark:border-[#0A1F44] pb-8">
        <div className="flex items-center gap-3 mb-4">
          <Badge variant="primary">Official v4.0.0 GA</Badge>
          <span className="flex items-center gap-1.5 text-xs text-slate-500 dark:text-slate-400">
            <Calendar className="w-3.5 h-3.5 text-[#0057FF] dark:text-[#00D1FF]" />
            November 11, 2026
          </span>
        </div>

        <h1 className="text-3xl sm:text-5xl font-black text-slate-900 dark:text-white tracking-tight leading-tight mb-6">
          Announcing SP 4.0.0: ARC Memory Safety, Real Async Epoll Loop, FFI Bindgen &amp; IDE Depth
        </h1>

        <div className="flex items-center gap-3 text-xs sm:text-sm text-slate-600 dark:text-slate-300">
          <div className="w-10 h-10 rounded-full bg-gradient-to-tr from-[#0057FF] to-[#00D1FF] p-0.5 shadow-md dark:shadow-[0_0_15px_rgba(0,209,255,0.4)]">
            <div className="w-full h-full bg-white dark:bg-[#020D21] rounded-full flex items-center justify-center font-bold text-[#0057FF] dark:text-[#00D1FF]">
              SP
            </div>
          </div>
          <div>
            <div className="font-semibold text-slate-900 dark:text-white">Shanto Paul</div>
            <div className="text-slate-500 dark:text-slate-400 text-xs">Language Creator &amp; Lead Architect</div>
          </div>
        </div>
      </header>

      {/* Article Body */}
      <article className="space-y-6 text-slate-600 dark:text-slate-300 text-sm sm:text-base leading-relaxed">
        <p className="text-lg text-slate-800 dark:text-slate-200 font-medium">
          We are thrilled to announce the general availability of <strong>SP 4.0.0 GA</strong>. Following Phases 30–40, SP 4.0.0 completes the core language safety, asynchronous concurrency, native FFI, and developer tooling pillars on top of the production backend ecosystem.
        </p>

        <h2 className="text-2xl font-bold text-slate-900 dark:text-white pt-6 border-t border-slate-200 dark:border-[#0A1F44]">
          What&apos;s New in SP 4.0.0
        </h2>

        <div className="space-y-4">
          <div className="rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 p-5 shadow-sm">
            <h3 className="text-base font-bold text-slate-900 dark:text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
              Deterministic ARC &amp; Escape Analysis
            </h3>
            <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400">
              Complete Automatic Reference Counting with weak references, cycle-breaking semantics, arena allocation lowering, and compiler-verified escape analysis with zero GC pauses.
            </p>
          </div>

          <div className="rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 p-5 shadow-sm">
            <h3 className="text-base font-bold text-slate-900 dark:text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
              Result / Option &amp; Try-Catch Error Handling
            </h3>
            <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400">
              First-class <code>Result&lt;T, E&gt;</code> and <code>Option&lt;T&gt;</code> error propagation with the <code>?</code> operator alongside full <code>try</code> / <code>catch</code> / <code>finally</code> lowering.
            </p>
          </div>

          <div className="rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 p-5 shadow-sm">
            <h3 className="text-base font-bold text-slate-900 dark:text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
              Real Async / Await &amp; Epoll Event Loop
            </h3>
            <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400">
              Heap-frame coroutine lowering with cooperative scheduler, timer heaps, task cancellation, and native <code>epoll</code> I/O event loop scalability beyond <code>FD_SETSIZE</code>.
            </p>
          </div>

          <div className="rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 p-5 shadow-sm">
            <h3 className="text-base font-bold text-slate-900 dark:text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#0057FF] dark:text-[#00D1FF]" />
              C Header Bindgen, LSP Depth &amp; DAP Debugger
            </h3>
            <p className="text-xs sm:text-sm text-slate-600 dark:text-slate-400">
              Automated C FFI binding generation via <code>spc bindgen</code> with struct-by-value ABI lowering, semantic tokens and workspace symbols in <code>spls</code>, and interactive debugging in <code>spd</code>.
            </p>
          </div>
        </div>

        <h2 className="text-2xl font-bold text-slate-900 dark:text-white pt-6 border-t border-slate-200 dark:border-[#0A1F44]">
          Code in Action: SP 4.0.0 Service
        </h2>
        <p>Write safe, expressive, bare-metal services in SP 4.0.0:</p>

        <CodeHighlighter
          code={sampleCode}
          filename="server.sp"
          showLineNumbers={true}
        />

        <h2 className="text-2xl font-bold text-slate-900 dark:text-white pt-6 border-t border-slate-200 dark:border-[#0A1F44]">
          Instant Installation
        </h2>
        <p>To upgrade to or install SP 4.0.0 on Windows:</p>

        <pre className="bg-[#0B1528] p-4 rounded-xl border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 overflow-x-auto shadow-inner">
          irm https://dl.splang.shantopaul.com/install.ps1 | iex
        </pre>

        <p>Or on Linux / macOS:</p>

        <pre className="bg-[#0B1528] p-4 rounded-xl border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 overflow-x-auto shadow-inner">
          curl -fsSL https://dl.splang.shantopaul.com/install.sh | bash
        </pre>

        <div className="pt-8 border-t border-slate-200 dark:border-[#0A1F44] flex flex-col sm:flex-row items-center justify-between gap-4">
          <div>
            <p className="text-xs text-slate-500 dark:text-slate-400 mb-1">Explore all downloads and packages</p>
            <p className="text-sm font-semibold text-slate-900 dark:text-white">SP 4.0.0 GA is available for Windows, Linux, and macOS.</p>
          </div>
          <Link href="/downloads">
            <Button variant="primary">Download SP 4.0.0</Button>
          </Link>
        </div>
      </article>
    </div>
  );
}
