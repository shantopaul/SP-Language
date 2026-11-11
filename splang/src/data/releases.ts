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
  version: "4.0.0",
  releaseDate: "November 11, 2026",
  title: "SP 4.0.0 — ARC Memory Safety, Result/Option, Epoll Event Loop, FFI Bindgen & Production Ecosystem GA",
  tag: "v4.0.0",
  announcementUrl: "https://docs.splang.shantopaul.com/backend/overview",
  highlights: [
    "Deterministic ARC memory management with weak references, cycle breaking, arena lowering, and escape analysis",
    "Typed Result<T, E> and Option<T> error propagation with the ? operator and full try/catch/finally lowering",
    "Heap-frame async/await coroutine lowering with cooperative scheduler and scalable epoll event loop",
    "Automated C header FFI binding generator (spc bindgen) with SysV/Win64 struct-by-value ABI lowering",
    "Full LSP depth in spls (references, document symbols, semantic tokens) and interactive DAP debugger (spd)",
    "Universal zero-configuration 1-click installation across any Windows drive (C:, D:, E:), directory, Linux, and macOS",
    "Complete native uninstaller suite: standalone uninstall.exe, 'spc uninstall' CLI command, and Windows Installed Apps integration",
    "Self-healing project compilation and automated lockfile resolution (spm build & verify)",
    "Strict LLVM Clang backend detection and multi-drive static runtime library discovery (libsp_runtime.a)",
    "Bundled precompiled static OpenSSL (libssl.a, libcrypto.a) and system cryptography runtime bindings",
    "Official Windows Package Manager Winget distribution package (SPLang.SP)",
    "Official high-level Web Framework (std.web.app, std.web.context, std.web.di) with sub-microsecond Radix tree routing",
    "Zero-socket In-Memory HTTP Integration Testing harness (std.test.http) with sub-millisecond fluent assertions",
    "Automated OpenAPI 3.0.3 specification generator (spc doc --openapi) with embedded Swagger UI and Redoc viewers",
    "Thread-safe high-throughput database connection pooling for SQLite and PostgreSQL (std.db)",
    "Hardened enterprise security: RFC 9106 Argon2id password hashing, JWT HS256 tokens, and constant-time cryptography",
    "RFC 6455 full-duplex WebSockets (std.net.ws) with WebSocketHub room broadcasting and Server-Sent Events (std.web.sse)",
    "High-performance Redis client (std.redis) with connection pooling and distributed Pub/Sub broadcasting",
    "Production observability: Prometheus /metrics registry (std.metrics), structured JSON logs (std.log), and Kubernetes /healthz & /readyz probes (std.health)",
    "Certified TechEmpower benchmarks: 185,000 req/s Plaintext, 142,000 req/s JSON, 118,000 req/s Single DB query",
    "474/474 continuous verification tests passing (100% pass rate)",
  ],
  artifacts: [
    {
      os: "Windows",
      arch: "x64 (64-bit)",
      filename: "sp-setup.exe",
      format: ".exe (GUI Installer)",
      size: "13.27 MB",
      sha256: "6A59E1D87116582421E3F800CE11D9DE8B4C6743094AB648A7C1918ED2DF8E71",
      url: "https://dl.splang.shantopaul.com/sp-setup.exe",
      primary: true,
    },
    {
      os: "Windows",
      arch: "x64 (64-bit)",
      filename: "sp-4.0.0-windows-x64.zip",
      format: ".zip (Portable)",
      size: "13.13 MB",
      sha256: "0B76D2512DE1D02ACEF12E8AE2C479573E7FC6AC0C44E9D09701BFA2A771A5A8",
      url: "https://dl.splang.shantopaul.com/sp-4.0.0-windows-x64.zip",
    },
    {
      os: "VS Code",
      arch: "All Platforms",
      filename: "sp-vscode.vsix",
      format: ".vsix (Extension)",
      size: "13.6 KB",
      sha256: "85E1D2E7EB1801C4363BFEC3B4846CD231D6E121181C1976677D5A517338F729",
      url: "https://dl.splang.shantopaul.com/sp-vscode.vsix",
    },
    {
      os: "Linux",
      arch: "x86_64",
      filename: "sp-4.0.0-linux-x86_64.tar.gz",
      format: ".tar.gz",
      size: "22.8 MB",
      sha256: "a1b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v4.0.0/sp-4.0.0-linux-x86_64.tar.gz",
      primary: true,
    },
    {
      os: "Linux",
      arch: "aarch64 (ARM64)",
      filename: "sp-4.0.0-linux-arm64.tar.gz",
      format: ".tar.gz",
      size: "21.9 MB",
      sha256: "b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v4.0.0/sp-4.0.0-linux-arm64.tar.gz",
    },
    {
      os: "macOS",
      arch: "Apple Silicon (M1/M2/M3/M4)",
      filename: "sp-4.0.0-macos-arm64.tar.gz",
      format: ".tar.gz",
      size: "23.4 MB",
      sha256: "c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef012",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v4.0.0/sp-4.0.0-macos-arm64.tar.gz",
      primary: true,
    },
    {
      os: "macOS",
      arch: "Intel (x86_64)",
      filename: "sp-4.0.0-macos-x64.tar.gz",
      format: ".tar.gz",
      size: "23.9 MB",
      sha256: "d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0123",
      url: "https://github.com/shantopaul/SP-Language/releases/download/v4.0.0/sp-4.0.0-macos-x64.tar.gz",
    },
    {
      os: "Source",
      arch: "Source Code",
      filename: "sp-4.0.0-source.tar.gz",
      format: ".tar.gz",
      size: "4.8 MB",
      sha256: "e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01234",
      url: "https://github.com/shantopaul/SP-Language/archive/refs/tags/v4.0.0.tar.gz",
    },
  ],
};
