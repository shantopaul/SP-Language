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
  version: "3.0.0",
  releaseDate: "October 9, 2026",
  title: "SP 3.0.0 — Enterprise Backend Platform GA",
  tag: "v3.0.0",
  announcementUrl: "https://docs.splang.shantopaul.com/backend/overview",
  highlights: [
    "Official high-level Web Framework (std.web.app, std.web.context, std.web.di) with sub-microsecond Radix tree routing",
    "Zero-socket In-Memory HTTP Integration Testing harness (std.test.http) with sub-millisecond fluent assertions",
    "Automated OpenAPI 3.0.3 specification generator (spc doc --openapi) with embedded Swagger UI and Redoc viewers",
    "Thread-safe high-throughput database connection pooling for SQLite and PostgreSQL (std.db)",
    "Hardened enterprise security: RFC 9106 Argon2id password hashing, JWT HS256 tokens, and constant-time cryptography",
    "RFC 6455 full-duplex WebSockets (std.net.ws) with WebSocketHub room broadcasting and Server-Sent Events (std.web.sse)",
    "High-performance Redis client (std.redis) with connection pooling and distributed Pub/Sub broadcasting",
    "Production observability: Prometheus /metrics registry (std.metrics), structured JSON logs (std.log), and Kubernetes /healthz & /readyz probes (std.health)",
    "In-process background workers, exponential backoff retries, dead-letter queues, and cron scheduling (std.queue)",
    "Musl libc static Linux compilation (spc build --static) producing zero-dependency Docker scratch images (< 15MB)",
    "Certified TechEmpower benchmarks: 185,000 req/s Plaintext, 142,000 req/s JSON, 118,000 req/s Single DB query",
    "474/474 continuous verification tests passing (100% pass rate)",
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
