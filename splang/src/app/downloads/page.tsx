import React from "react";
import { CURRENT_RELEASE } from "@/data/releases";
import { Download, ShieldCheck, Terminal, Check, ExternalLink, HardDrive } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const metadata = {
  title: "Downloads & Releases",
  description: "Official pre-compiled binaries and release packages for SP Programming Language on Windows, Linux, and macOS.",
};

export default function DownloadsPage() {
  return (
    <div className="py-16 max-w-7xl mx-auto px-4 sm:px-6 lg:px-8">
      {/* Header */}
      <div className="text-center max-w-3xl mx-auto mb-16">
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-emerald-950/60 border border-emerald-800/60 text-emerald-400 text-xs font-semibold mb-3">
          <ShieldCheck className="w-4 h-4" />
          <span>Production Release v{CURRENT_RELEASE.version}</span>
        </div>
        <h1 className="text-4xl sm:text-5xl font-extrabold text-white tracking-tight">
          Download SP Language
        </h1>
        <p className="mt-4 text-base sm:text-lg text-slate-300">
          Official pre-compiled native toolchain packages for Windows, Linux, and macOS. Includes compiler (<code>spc</code>), package manager (<code>spm</code>), language server (<code>spls</code>), and debugger (<code>spd</code>).
        </p>
      </div>

      {/* Primary OS Cards Grid */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-8 mb-16">
        {/* Windows Card */}
        <div className="rounded-2xl border border-slate-800 bg-slate-900/60 p-6 flex flex-col justify-between shadow-xl hover:border-cyan-500/40 transition-all">
          <div>
            <div className="flex items-center justify-between mb-4">
              <span className="text-xl font-bold text-white">Windows</span>
              <span className="px-2 py-0.5 rounded text-xs font-mono bg-cyan-950 text-cyan-400 border border-cyan-800">
                x64
              </span>
            </div>
            <p className="text-xs text-slate-400 mb-6">
              Compatible with Windows 10 & 11 (64-bit). Includes installer & zip distribution.
            </p>
            <div className="bg-black/50 p-3 rounded-xl border border-slate-800 font-mono text-xs text-emerald-400 mb-6">
              winget install SPLang.SP
            </div>
          </div>
          <a
            href={CURRENT_RELEASE.artifacts.find((a) => a.os === "Windows")?.url}
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button className="w-full justify-center" variant="primary">
              <Download className="w-4 h-4 mr-1.5" />
              Download .zip ({CURRENT_RELEASE.artifacts.find((a) => a.os === "Windows")?.size || "3.85 MB"})
            </Button>
          </a>
        </div>

        {/* Linux Card */}
        <div className="rounded-2xl border border-slate-800 bg-slate-900/60 p-6 flex flex-col justify-between shadow-xl hover:border-emerald-500/40 transition-all">
          <div>
            <div className="flex items-center justify-between mb-4">
              <span className="text-xl font-bold text-white">Linux</span>
              <span className="px-2 py-0.5 rounded text-xs font-mono bg-emerald-950 text-emerald-400 border border-emerald-800">
                x86_64 / arm64
              </span>
            </div>
            <p className="text-xs text-slate-400 mb-6">
              Compatible with Ubuntu, Debian, Fedora, Arch, and RHEL. Includes glibc runtime linkage.
            </p>
            <div className="bg-black/50 p-3 rounded-xl border border-slate-800 font-mono text-xs text-emerald-400 mb-6 overflow-x-auto whitespace-nowrap">
              curl -fsSL https://splang.shantopaul.com/install.sh | sh
            </div>
          </div>
          <a
            href={CURRENT_RELEASE.artifacts.find((a) => a.os === "Linux" && a.arch.includes("x86_64"))?.url}
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button className="w-full justify-center" variant="primary">
              <Download className="w-4 h-4 mr-1.5" />
              Download .tar.gz (22.8 MB)
            </Button>
          </a>
        </div>

        {/* macOS Card */}
        <div className="rounded-2xl border border-slate-800 bg-slate-900/60 p-6 flex flex-col justify-between shadow-xl hover:border-blue-500/40 transition-all">
          <div>
            <div className="flex items-center justify-between mb-4">
              <span className="text-xl font-bold text-white">macOS</span>
              <span className="px-2 py-0.5 rounded text-xs font-mono bg-blue-950 text-blue-400 border border-blue-800">
                Apple Silicon / Intel
              </span>
            </div>
            <p className="text-xs text-slate-400 mb-6">
              Universal binary support for macOS 12 Monterey through macOS 15 Sequoia.
            </p>
            <div className="bg-black/50 p-3 rounded-xl border border-slate-800 font-mono text-xs text-emerald-400 mb-6 overflow-x-auto whitespace-nowrap">
              brew install shantopaul/tap/sp
            </div>
          </div>
          <a
            href={CURRENT_RELEASE.artifacts.find((a) => a.os === "macOS" && a.arch.includes("Apple Silicon"))?.url}
            target="_blank"
            rel="noopener noreferrer"
          >
            <Button className="w-full justify-center" variant="primary">
              <Download className="w-4 h-4 mr-1.5" />
              Download Apple Silicon (23.4 MB)
            </Button>
          </a>
        </div>
      </div>

      {/* Complete Release Artifacts Table with SHA-256 */}
      <div className="rounded-2xl border border-slate-800 bg-slate-900/40 p-6 sm:p-8 mb-16 shadow-2xl">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between pb-6 mb-6 border-b border-slate-800 gap-4">
          <div>
            <h2 className="text-xl font-bold text-white">All Release Artifacts & Checksums</h2>
            <p className="text-xs text-slate-400 mt-1">
              Verify package authenticity against official SHA-256 integrity digests.
            </p>
          </div>
          <a
            href="https://github.com/shantopaul/SP-Language/releases"
            target="_blank"
            rel="noopener noreferrer"
            className="flex items-center gap-1.5 text-xs text-cyan-400 hover:text-cyan-300 font-medium self-start sm:self-auto"
          >
            <span>GitHub Releases Archive</span>
            <ExternalLink className="w-3.5 h-3.5" />
          </a>
        </div>

        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs sm:text-sm">
            <thead>
              <tr className="border-b border-slate-800 text-slate-400 uppercase text-[11px] tracking-wider">
                <th className="pb-3 font-semibold">OS / Platform</th>
                <th className="pb-3 font-semibold">Architecture</th>
                <th className="pb-3 font-semibold">Package File</th>
                <th className="pb-3 font-semibold">Size</th>
                <th className="pb-3 font-semibold">SHA-256 Checksum</th>
                <th className="pb-3 text-right font-semibold">Download</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 font-mono">
              {CURRENT_RELEASE.artifacts.map((art, idx) => (
                <tr key={idx} className="hover:bg-slate-800/30 transition-colors">
                  <td className="py-3 font-sans font-medium text-slate-200">{art.os}</td>
                  <td className="py-3 text-slate-400 font-sans">{art.arch}</td>
                  <td className="py-3 text-cyan-300">{art.filename}</td>
                  <td className="py-3 text-slate-400">{art.size}</td>
                  <td className="py-3 text-[11px] text-slate-500 max-w-[180px] truncate" title={art.sha256}>
                    {art.sha256}
                  </td>
                  <td className="py-3 text-right">
                    <a
                      href={art.url}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="inline-flex items-center gap-1 px-3 py-1 rounded bg-slate-800 hover:bg-cyan-600 hover:text-white text-slate-200 transition-colors text-xs font-sans font-medium"
                    >
                      <Download className="w-3.5 h-3.5" />
                      Get
                    </a>
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      </div>

      {/* Verification Instructions */}
      <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
        <div className="rounded-xl border border-slate-800 bg-slate-900/30 p-6">
          <h3 className="text-base font-semibold text-white mb-2 flex items-center gap-2">
            <Terminal className="w-4 h-4 text-cyan-400" />
            Verify SHA-256 on Windows
          </h3>
          <p className="text-xs text-slate-400 mb-3">
            Open PowerShell in your downloads folder and execute:
          </p>
          <pre className="bg-black/60 p-3 rounded-lg border border-slate-800 font-mono text-xs text-emerald-400 overflow-x-auto">
            Get-FileHash sp-1.0.0-windows-x64.zip -Algorithm SHA256
          </pre>
        </div>

        <div className="rounded-xl border border-slate-800 bg-slate-900/30 p-6">
          <h3 className="text-base font-semibold text-white mb-2 flex items-center gap-2">
            <Terminal className="w-4 h-4 text-emerald-400" />
            Verify SHA-256 on Linux / macOS
          </h3>
          <p className="text-xs text-slate-400 mb-3">
            Run sha256sum in your terminal to verify integrity:
          </p>
          <pre className="bg-black/60 p-3 rounded-lg border border-slate-800 font-mono text-xs text-emerald-400 overflow-x-auto">
            sha256sum sp-1.0.0-linux-x86_64.tar.gz
          </pre>
        </div>
      </div>
    </div>
  );
}
