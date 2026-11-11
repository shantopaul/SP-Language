import React from "react";
import { CURRENT_RELEASE } from "@/data/releases";
import { Download, ShieldCheck, Terminal, Check, ExternalLink, HardDrive, RefreshCw, Trash2, Cpu } from "lucide-react";
import { Button } from "@/components/ui/Button";

export const metadata = {
  title: "Downloads & Releases",
  description: "Official pre-compiled binaries and release packages for SP Programming Language on Windows, Linux, and macOS.",
};

export default function DownloadsPage() {
  return (
    <div className="py-12 sm:py-16 max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 transition-colors">
      {/* Header */}
      <div className="text-center max-w-3xl mx-auto mb-16">
        <div className="inline-flex items-center gap-1.5 px-3.5 py-1 rounded-full bg-blue-50 dark:bg-[#081735] border border-blue-200 dark:border-cyan-500/40 text-[#0057FF] dark:text-cyan-300 text-xs font-semibold mb-3 shadow-sm dark:shadow-glow-sm">
          <ShieldCheck className="w-4 h-4 text-[#0057FF] dark:text-cyan-400" />
          <span>Production Release v{CURRENT_RELEASE.version} GA</span>
        </div>
        <h1 className="text-4xl sm:text-5xl font-extrabold text-slate-900 dark:text-white tracking-tight">
          Download SP Language
        </h1>
        <p className="mt-4 text-base sm:text-lg text-slate-600 dark:text-slate-300 leading-relaxed">
          Official pre-compiled native toolchains for Windows, Linux, and macOS. Includes compiler (<code>spc</code>), package manager (<code>spm</code>), language server (<code>spls</code>), and debugger (<code>spd</code>).
        </p>
      </div>

      {/* Primary OS Cards Grid */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-8 mb-16">
        {/* Windows Card */}
        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/75 p-6 sm:p-7 flex flex-col justify-between shadow-sm dark:shadow-xl hover:border-blue-300 dark:hover:border-cyan-400/50 hover:shadow-xl transition-all backdrop-blur-xl">
          <div>
            <div className="flex items-center justify-between mb-4">
              <span className="text-xl font-bold text-slate-900 dark:text-white">Windows</span>
              <span className="px-2.5 py-0.5 rounded-full text-xs font-mono bg-blue-50 dark:bg-cyan-950/80 text-[#0057FF] dark:text-cyan-300 border border-blue-200 dark:border-cyan-700 font-semibold shadow-sm">
                x64 (64-bit)
              </span>
            </div>
            <p className="text-xs text-slate-600 dark:text-slate-300 mb-6 leading-relaxed">
              Compatible with Windows 10 &amp; 11 (64-bit). Instant 1-click PowerShell installer, GUI setup &amp; portable zip.
            </p>
            <div className="bg-[#0B1528] p-3 rounded-xl border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 mb-6 overflow-x-auto whitespace-nowrap shadow-inner">
              irm https://dl.splang.shantopaul.com/install.ps1 | iex
            </div>
          </div>
          <div className="flex flex-col gap-2.5">
            <a
              href="https://dl.splang.shantopaul.com/sp-setup.exe"
              target="_blank"
              rel="noopener noreferrer"
            >
              <Button className="w-full justify-center" variant="primary">
                <Download className="w-4 h-4 mr-1.5" />
                Download Installer (.exe) (2.76 MB)
              </Button>
            </a>
            <a
              href="https://dl.splang.shantopaul.com/sp-4.0.0-windows-x64.zip"
              target="_blank"
              rel="noopener noreferrer"
            >
              <Button className="w-full justify-center" variant="secondary">
                <Download className="w-4 h-4 mr-1.5" />
                Download Portable (.zip) (13.13 MB)
              </Button>
            </a>
          </div>
        </div>

        {/* Linux Card */}
        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/75 p-6 sm:p-7 flex flex-col justify-between shadow-sm dark:shadow-xl hover:border-blue-300 dark:hover:border-cyan-400/50 hover:shadow-xl transition-all backdrop-blur-xl">
          <div>
            <div className="flex items-center justify-between mb-4">
              <span className="text-xl font-bold text-slate-900 dark:text-white">Linux</span>
              <span className="px-2.5 py-0.5 rounded-full text-xs font-mono bg-blue-50 dark:bg-blue-950/80 text-[#0057FF] dark:text-blue-300 border border-blue-200 dark:border-blue-700 font-semibold shadow-sm">
                x86_64 / arm64
              </span>
            </div>
            <p className="text-xs text-slate-600 dark:text-slate-300 mb-6 leading-relaxed">
              Compatible with Ubuntu, Debian, Fedora, Arch, and RHEL. Includes glibc native runtime linkage.
            </p>
            <div className="bg-[#0B1528] p-3 rounded-xl border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 mb-6 overflow-x-auto whitespace-nowrap shadow-inner">
              curl -fsSL https://dl.splang.shantopaul.com/install.sh | sh
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
        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/75 p-6 sm:p-7 flex flex-col justify-between shadow-sm dark:shadow-xl hover:border-blue-300 dark:hover:border-cyan-400/50 hover:shadow-xl transition-all backdrop-blur-xl">
          <div>
            <div className="flex items-center justify-between mb-4">
              <span className="text-xl font-bold text-slate-900 dark:text-white">macOS</span>
              <span className="px-2.5 py-0.5 rounded-full text-xs font-mono bg-indigo-50 dark:bg-indigo-950/80 text-indigo-700 dark:text-indigo-300 border border-indigo-200 dark:border-indigo-700 font-semibold shadow-sm">
                Apple Silicon / Intel
              </span>
            </div>
            <p className="text-xs text-slate-600 dark:text-slate-300 mb-6 leading-relaxed">
              Universal binary support for macOS 12 Monterey through macOS 15 Sequoia (arm64 &amp; x86_64).
            </p>
            <div className="bg-[#0B1528] p-3 rounded-xl border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 mb-6 overflow-x-auto whitespace-nowrap shadow-inner">
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
      <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 p-6 sm:p-8 mb-16 shadow-sm dark:shadow-2xl backdrop-blur-xl">
        <div className="flex flex-col sm:flex-row sm:items-center justify-between pb-6 mb-6 border-b border-slate-200 dark:border-[#0A1F44] gap-4">
          <div>
            <h2 className="text-xl font-bold text-slate-900 dark:text-white">All Release Artifacts &amp; Checksums</h2>
            <p className="text-xs text-slate-500 dark:text-slate-400 mt-1">
              Verify package authenticity against official SHA-256 integrity digests.
            </p>
          </div>
          <a
            href="https://github.com/shantopaul/SP-Language/releases"
            target="_blank"
            rel="noopener noreferrer"
            className="flex items-center gap-1.5 text-xs text-[#0057FF] dark:text-cyan-300 hover:text-blue-700 dark:hover:text-white font-medium self-start sm:self-auto transition-colors"
          >
            <span>GitHub Releases Archive</span>
            <ExternalLink className="w-3.5 h-3.5" />
          </a>
        </div>

        <div className="overflow-x-auto -mx-4 px-4 sm:mx-0 sm:px-0 scrollbar-thin">
          <table className="w-full text-left text-xs sm:text-sm">
            <thead>
              <tr className="border-b border-slate-200 dark:border-[#0A1F44] text-slate-500 dark:text-slate-400 uppercase text-[11px] tracking-wider">
                <th className="pb-3 font-semibold">OS / Platform</th>
                <th className="pb-3 font-semibold">Architecture</th>
                <th className="pb-3 font-semibold">Package File</th>
                <th className="pb-3 font-semibold">Size</th>
                <th className="pb-3 font-semibold">SHA-256 Checksum</th>
                <th className="pb-3 text-right font-semibold">Download</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-100 dark:divide-[#0A1F44]/60 font-mono">
              {CURRENT_RELEASE.artifacts.map((art, idx) => (
                <tr key={idx} className="hover:bg-slate-50 dark:hover:bg-[#0A1F44]/40 transition-colors">
                  <td className="py-3 font-sans font-medium text-slate-900 dark:text-slate-200">{art.os}</td>
                  <td className="py-3 text-slate-600 dark:text-slate-400 font-sans">{art.arch}</td>
                  <td className="py-3 text-[#0057FF] dark:text-cyan-300 font-semibold">{art.filename}</td>
                  <td className="py-3 text-slate-700 dark:text-slate-300">{art.size}</td>
                  <td className="py-3 text-[11px] text-slate-500 max-w-[180px] truncate" title={art.sha256}>
                    {art.sha256}
                  </td>
                  <td className="py-3 text-right">
                    <a
                      href={art.url}
                      target="_blank"
                      rel="noopener noreferrer"
                      className="inline-flex items-center gap-1 px-3 py-1 rounded-lg bg-slate-100 dark:bg-[#081735] hover:bg-[#0057FF] hover:text-white text-slate-700 dark:text-slate-200 transition-all text-xs font-sans font-medium border border-slate-200 dark:border-[#0A1F44] hover:border-[#0057FF]"
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
      <div className="grid grid-cols-1 md:grid-cols-2 gap-8 mb-16">
        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 p-6 backdrop-blur-xl shadow-sm">
          <h3 className="text-base font-semibold text-slate-900 dark:text-white mb-2 flex items-center gap-2">
            <Terminal className="w-4 h-4 text-[#0057FF] dark:text-cyan-400" />
            Verify SHA-256 on Windows
          </h3>
          <p className="text-xs text-slate-600 dark:text-slate-400 mb-3">
            Open PowerShell in your downloads folder and execute:
          </p>
          <pre className="bg-[#0B1528] p-3.5 rounded-xl border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 overflow-x-auto shadow-inner">
            Get-FileHash sp-4.0.0-windows-x64.zip -Algorithm SHA256
          </pre>
        </div>

        <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/60 p-6 backdrop-blur-xl shadow-sm">
          <h3 className="text-base font-semibold text-slate-900 dark:text-white mb-2 flex items-center gap-2">
            <Terminal className="w-4 h-4 text-[#0057FF] dark:text-cyan-400" />
            Verify SHA-256 on Linux / macOS
          </h3>
          <p className="text-xs text-slate-600 dark:text-slate-400 mb-3">
            Run sha256sum in your terminal to verify integrity:
          </p>
          <pre className="bg-[#0B1528] p-3.5 rounded-xl border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 overflow-x-auto shadow-inner">
            sha256sum sp-4.0.0-linux-x86_64.tar.gz
          </pre>
        </div>
      </div>

      {/* Instant Auto-Upgrade Guide */}
      <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 p-6 sm:p-8 mb-16 backdrop-blur-xl shadow-sm">
        <div className="flex items-center gap-2 mb-3">
          <RefreshCw className="w-5 h-5 text-[#0057FF] dark:text-cyan-400" />
          <h2 className="text-xl font-bold text-slate-900 dark:text-white">Instant Auto-Upgrade</h2>
        </div>
        <p className="text-sm text-slate-600 dark:text-slate-300 mb-6 leading-relaxed">
          Upgrades are fast (3–5 seconds), in-place, and preserve existing projects, settings, and cached LLVM toolchains with zero data loss.
        </p>

        <div className="grid grid-cols-1 md:grid-cols-2 gap-6 text-xs sm:text-sm">
          <div className="p-5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-slate-50 dark:bg-[#020D21]">
            <span className="font-semibold text-slate-900 dark:text-white block mb-1">1. Built-in CLI Upgrade</span>
            <p className="text-slate-500 dark:text-slate-400 text-xs mb-3">Run directly from any terminal prompt:</p>
            <pre className="bg-[#0B1528] p-3 rounded-lg border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300">spc upgrade</pre>
          </div>

          <div className="p-5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-slate-50 dark:bg-[#020D21]">
            <span className="font-semibold text-slate-900 dark:text-white block mb-1">2. Universal One-Liner (PowerShell)</span>
            <p className="text-slate-500 dark:text-slate-400 text-xs mb-3">Re-run installer anytime for auto-upgrade:</p>
            <pre className="bg-[#0B1528] p-3 rounded-lg border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-cyan-300 overflow-x-auto">irm https://dl.splang.shantopaul.com/install.ps1 | iex</pre>
          </div>
        </div>
      </div>

      {/* Complete Uninstallation Guide */}
      <div className="rounded-2xl border border-slate-200 dark:border-[#0A1F44] bg-white dark:bg-[#081735]/70 p-6 sm:p-8 backdrop-blur-xl shadow-sm">
        <div className="flex items-center gap-2 mb-3">
          <Trash2 className="w-5 h-5 text-rose-500 dark:text-rose-400" />
          <h2 className="text-xl font-bold text-slate-900 dark:text-white">Clean Uninstallation</h2>
        </div>
        <p className="text-sm text-slate-600 dark:text-slate-300 mb-6 leading-relaxed">
          SP includes a 100% clean uninstaller that removes all compiler binaries, runtimes, environment variables (<code>SP_HOME</code>), and user <code>PATH</code> entries with zero residue.
        </p>

        <div className="grid grid-cols-1 md:grid-cols-3 gap-6 text-xs sm:text-sm">
          <div className="p-5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-slate-50 dark:bg-[#020D21]">
            <span className="font-semibold text-slate-900 dark:text-white block mb-1">1. Native CLI Command</span>
            <p className="text-slate-500 dark:text-slate-400 text-xs mb-3">Run directly from any terminal:</p>
            <pre className="bg-[#0B1528] p-3 rounded-lg border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-rose-300">spc uninstall</pre>
          </div>

          <div className="p-5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-slate-50 dark:bg-[#020D21]">
            <span className="font-semibold text-slate-900 dark:text-white block mb-1">2. Online One-Liner (PowerShell)</span>
            <p className="text-slate-500 dark:text-slate-400 text-xs mb-3">Instant removal without local files:</p>
            <pre className="bg-[#0B1528] p-3 rounded-lg border border-slate-800 dark:border-[#0A1F44] font-mono text-xs text-rose-300 overflow-x-auto">irm https://dl.splang.shantopaul.com/uninstall.ps1 | iex</pre>
          </div>

          <div className="p-5 rounded-xl border border-slate-200 dark:border-[#0A1F44] bg-slate-50 dark:bg-[#020D21]">
            <span className="font-semibold text-slate-900 dark:text-white block mb-1">3. Windows Installed Apps</span>
            <p className="text-slate-500 dark:text-slate-400 text-xs mb-3">Via Windows Settings or standalone .exe:</p>
            <p className="text-slate-700 dark:text-slate-300 text-xs font-mono bg-slate-100 dark:bg-[#081735] p-3 rounded-lg border border-slate-200 dark:border-[#0A1F44]">Settings &rarr; Apps &rarr; Installed apps &rarr; SP Language &rarr; Uninstall</p>
          </div>
        </div>
      </div>
    </div>
  );
}
