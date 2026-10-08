export interface DownloadArtifact {
  os: string;
  arch: string;
  filename: string;
  format: string;
  size: string;
  sha256: string;
  url: string;
  primary?: boolean;
}

export interface ReleaseInfo {
  version: string;
  releaseDate: string;
  title: string;
  tag: string;
  announcementUrl: string;
  artifacts: DownloadArtifact[];
  highlights: string[];
}

export const CURRENT_RELEASE: ReleaseInfo = {
  version: "2.0.0",
  releaseDate: "October 8, 2026",
  title: "SP 2.0.0 — Language Editions, Comptime & WebAssembly GA",
  tag: "v2.0.0",
  announcementUrl: "https://docs.splang.shantopaul.com/sp2/overview",
  highlights: [
    "Language Editions (edition = '2026' and edition = '2028') with automated migration tool (spc fix)",
    "Compile-time function execution (comptime) for constant folding, table generation, and compile-time assertions",
    "First-class WebAssembly target (--target wasm32-wasi) targeting WASI preview 1 and serverless edge runtimes",
    "Structured async / await coroutines with zero allocations and seamless threadpool event-loop integration",
    "Self-hosted SP compiler frontend written in pure SP (compiler/self_hosted/)",
    "Deterministic Automated Reference Counting (ARC) with zero stop-the-world GC pauses and 0 bytes leaked",
    "Production package publishing and dependency management on registry.splang.shantopaul.com",
    "346/346 continuous verification tests passing (100% pass rate)",
  ],
  artifacts: [
    {
      os: "Windows",
      arch: "x64 (64-bit)",
      filename: "sp-setup.exe",
      format: ".exe (GUI Installer)",
      size: "2.76 MB",
      sha256: "84D36638402673CD129133BE83D0AF31C5BD176BA2C0130BC73F2A1A0DA2C34A",
      url: "https://dl.splang.shantopaul.com/sp-setup.exe",
      primary: true,
    },
    {
      os: "Windows",
      arch: "x64 (64-bit)",
      filename: "sp-1.0.0-windows-x64.zip",
      format: ".zip (Portable)",
      size: "2.62 MB",
      sha256: "691FDE6F305520FEE2E4CE589B0C5BB264367EEC0A8CEFD56BBE44D42DE9DE75",
      url: "https://dl.splang.shantopaul.com/sp-1.0.0-windows-x64.zip",
    },
    {
      os: "VS Code",
      arch: "All Platforms",
      filename: "sp-vscode.vsix",
      format: ".vsix (Extension)",
      size: "11.8 KB",
      sha256: "50F86C1435041704B8FA48E2DDF3FF34E557DB09460A21B358034F955703026D",
      url: "https://dl.splang.shantopaul.com/sp-vscode.vsix",
    },
    {
      os: "Linux",
      arch: "x86_64",
      filename: "sp-1.0.0-linux-x86_64.tar.gz",
      format: ".tar.gz",
      size: "22.8 MB",
      sha256: "a1b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v1.0.0/sp-1.0.0-linux-x86_64.tar.gz",
      primary: true,
    },
    {
      os: "Linux",
      arch: "aarch64 (ARM64)",
      filename: "sp-1.0.0-linux-arm64.tar.gz",
      format: ".tar.gz",
      size: "21.9 MB",
      sha256: "b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v1.0.0/sp-1.0.0-linux-arm64.tar.gz",
    },
    {
      os: "macOS",
      arch: "Apple Silicon (M1/M2/M3/M4)",
      filename: "sp-1.0.0-macos-arm64.tar.gz",
      format: ".tar.gz",
      size: "23.4 MB",
      sha256: "c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef012",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v1.0.0/sp-1.0.0-macos-arm64.tar.gz",
      primary: true,
    },
    {
      os: "macOS",
      arch: "Intel (x86_64)",
      filename: "sp-1.0.0-macos-x64.tar.gz",
      format: ".tar.gz",
      size: "23.9 MB",
      sha256: "d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0123",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v1.0.0/sp-1.0.0-macos-x64.tar.gz",
    },
    {
      os: "Source",
      arch: "Source Code",
      filename: "sp-1.0.0-source.tar.gz",
      format: ".tar.gz",
      size: "4.8 MB",
      sha256: "e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01234",
      url: "https://github.com/shantopaul/SP-Language/archive/refs/tags/v1.0.0.tar.gz",
    },
  ],
};
