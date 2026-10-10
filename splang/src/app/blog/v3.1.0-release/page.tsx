import React from "react";
import Link from "next/link";
import { ArrowLeft, Calendar, User, CheckCircle2, Zap, Terminal } from "lucide-react";
import { Badge } from "@/components/ui/Badge";
import { CodeHighlighter } from "@/components/ui/CodeHighlighter";
import { Button } from "@/components/ui/Button";

export const metadata = {
  title: "Announcing SP 3.1.0 GA: Enterprise Standard Library & Production Web Engine",
  description:
    "Official launch of SP 3.1.0 GA featuring 23 stdlib modules, std.web (185k req/s), native OpenSSL static TLS, async coroutines, and spc upgrade.",
};

export default function Release310BlogPostPage() {
  const sampleCode = `import std.web;
import std.json;
import std.db;

class UserDto {
    id: int,
    username: string,
    role: string,
}

fn handle_users(ctx: &web.Context) -> web.Response {
    let pool = db.get_pool();
    let rows = pool.query("SELECT id, username, role FROM users LIMIT 50");
    return ctx.json(rows);
}

fn main() -> void {
    let app = web.new_server();
    app.get("/api/v1/users", handle_users);
    
    println("[SP Web] Server listening on http://0.0.0.0:8080");
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
      <header className="mb-12 border-b border-[#0A1F44] pb-8">
        <div className="flex items-center gap-3 mb-4">
          <Badge variant="primary">Official v3.1.0 GA</Badge>
          <span className="flex items-center gap-1.5 text-xs text-slate-400">
            <Calendar className="w-3.5 h-3.5 text-[#00D1FF]" />
            October 10, 2026
          </span>
        </div>

        <h1 className="text-3xl sm:text-5xl font-black text-white tracking-tight leading-tight mb-6">
          Announcing SP 3.1.0: Enterprise Standard Library, Production Web Engine & Universal Toolchain
        </h1>

        <div className="flex items-center gap-3 text-xs sm:text-sm text-slate-300">
          <div className="w-10 h-10 rounded-full bg-gradient-to-tr from-[#0057FF] to-[#00D1FF] p-0.5 shadow-[0_0_15px_rgba(0,209,255,0.4)]">
            <div className="w-full h-full bg-[#020D21] rounded-full flex items-center justify-center font-bold text-[#00D1FF]">
              SP
            </div>
          </div>
          <div>
            <div className="font-semibold text-white">Shanto Paul</div>
            <div className="text-slate-400 text-xs">Language Creator & Lead Architect</div>
          </div>
        </div>
      </header>

      {/* Article Body */}
      <article className="space-y-6 text-slate-300 text-sm sm:text-base leading-relaxed">
        <p className="text-lg text-slate-200 font-medium">
          We are thrilled to announce the general availability of <strong>SP 3.1.0 GA</strong>. This release transforms SP from a lightning-fast native compiler into an end-to-end production ecosystem ready for microservices, low-latency databases, distributed queues, and native systems engineering.
        </p>

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          What&apos;s New in SP 3.1.0
        </h2>

        <div className="space-y-4">
          <div className="rounded-xl border border-[#0A1F44] bg-[#081735]/60 p-5">
            <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#00D1FF]" />
              23 Production Standard Library Modules
            </h3>
            <p className="text-xs sm:text-sm text-slate-400">
              SP 3.1.0 ships with 23 battle-tested modules out of the box: <code>collections</code>, <code>crypto</code>, <code>db</code>, <code>fs</code>, <code>health</code>, <code>http</code>, <code>io</code>, <code>json</code>, <code>log</code>, <code>math</code>, <code>mem</code>, <code>metrics</code>, <code>net</code>, <code>os</code>, <code>process</code>, <code>queue</code>, <code>redis</code>, <code>security</code>, <code>string</code>, <code>sync</code>, <code>test</code>, <code>time</code>, and <code>web</code>.
            </p>
          </div>

          <div className="rounded-xl border border-[#0A1F44] bg-[#081735]/60 p-5">
            <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#00D1FF]" />
              Production std.web Framework (185,000 req/s)
            </h3>
            <p className="text-xs sm:text-sm text-slate-400">
              Built on a non-blocking work-stealing thread pool and zero-alloc radix routing, <code>std.web</code> achieves 185k req/s at sub-millisecond latencies, matching raw C++ micro-frameworks.
            </p>
          </div>

          <div className="rounded-xl border border-[#0A1F44] bg-[#081735]/60 p-5">
            <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#00D1FF]" />
              Static OpenSSL Cryptography & Database TLS
            </h3>
            <p className="text-xs sm:text-sm text-slate-400">
              The SP runtime links <code>libssl.a</code> and <code>libcrypto.a</code> statically. Your compiled executables require zero external DLLs on Windows or dynamic shared objects on Linux, ensuring true single-binary distribution.
            </p>
          </div>

          <div className="rounded-xl border border-[#0A1F44] bg-[#081735]/60 p-5">
            <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
              <Zap className="w-4 h-4 text-[#00D1FF]" />
              Universal Auto-Upgrade &amp; System Doctor
            </h3>
            <p className="text-xs sm:text-sm text-slate-400">
              Simply run <code>spc upgrade</code> from any terminal to check the latest releases on <code>dl.splang.shantopaul.com</code> and update your toolchain in-place. Run <code>spc doctor</code> to perform full diagnostic smoke tests on your LLVM target, stdlib archives, and environment variables.
            </p>
          </div>
        </div>

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          Code in Action: Modern REST Service
        </h2>
        <p>Building high-throughput microservices in SP 3.1.0 takes fewer lines than Go and runs as fast as C++:</p>

        <CodeHighlighter
          code={sampleCode}
          filename="server.sp"
          showLineNumbers={true}
        />

        <h2 className="text-2xl font-bold text-white pt-6 border-t border-[#0A1F44]">
          Instant Installation
        </h2>
        <p>To upgrade to or install SP 3.1.0 on Windows:</p>

        <pre className="bg-[#020D21] p-4 rounded-xl border border-[#0A1F44] font-mono text-xs text-[#00D1FF] overflow-x-auto">
          irm https://dl.splang.shantopaul.com/install.ps1 | iex
        </pre>

        <p>Or on Linux / macOS:</p>

        <pre className="bg-[#020D21] p-4 rounded-xl border border-[#0A1F44] font-mono text-xs text-[#00D1FF] overflow-x-auto">
          curl -fsSL https://dl.splang.shantopaul.com/install.sh | bash
        </pre>

        <div className="pt-8 border-t border-[#0A1F44] flex flex-col sm:flex-row items-center justify-between gap-4">
          <div>
            <p className="text-xs text-slate-400 mb-1">Explore all downloads and packages</p>
            <p className="text-sm font-semibold text-white">SP 3.1.0 GA is available for Windows, Linux, and macOS.</p>
          </div>
          <Link href="/downloads">
            <Button variant="primary">Download SP 3.1.0</Button>
          </Link>
        </div>
      </article>
    </div>
  );
}
