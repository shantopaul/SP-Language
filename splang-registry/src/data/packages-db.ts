import { Package } from "@/types/package";

export const PACKAGES: Package[] = [
  {
    name: "json",
    version: "1.2.0",
    description: "High-performance streaming JSON parser and serializer with native SP type mapping.",
    author: "SP Core Team <core@splang.shantopaul.com>",
    license: "Apache-2.0",
    category: "utilities",
    keywords: ["json", "serialization", "parser", "streaming"],
    totalDownloads: 142500,
    publishedAt: "2026-10-06",
    official: true,
    repositoryUrl: "https://github.com/shantopaul/sp-json",
    dependencies: [],
    versions: [
      {
        version: "1.2.0",
        publishedAt: "2026-10-06",
        checksum: "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855",
        size: "34.2 KB",
        spmMinVersion: "1.0.0",
        changelog: "Add SIMD-accelerated string escaping and zero-copy byte slices.",
      },
      {
        version: "1.1.0",
        publishedAt: "2026-09-15",
        checksum: "a1b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0",
        size: "31.8 KB",
        spmMinVersion: "0.9.0",
        changelog: "Initial stable release supporting streaming token reader.",
      },
    ],
    readme: `# json for SP Language

High-performance streaming JSON parser and serializer with native SP type mapping.

## Installation
Add to your \`sp.toml\`:
\`\`\`toml
[dependencies]
json = "^1.2.0"
\`\`\`
Or install via CLI:
\`\`\`bash
spm add json
\`\`\`

## Quick Example
\`\`\`sp
import std.io;
import json;

fn main() -> void {
    let payload = "{\\"name\\": \\"SP\\", \\"version\\": 1.0}";
    let parsed = json.parse(payload);
    
    println("Package name: \${parsed.get('name')}");
}
\`\`\`
`,
  },
  {
    name: "http",
    version: "1.1.0",
    description: "Asynchronous HTTP/1.1 and HTTP/2 client and server framework with middleware routing.",
    author: "SP Core Team <core@splang.shantopaul.com>",
    license: "MIT",
    category: "web",
    keywords: ["http", "client", "server", "rest", "networking"],
    totalDownloads: 98200,
    publishedAt: "2026-10-06",
    official: true,
    repositoryUrl: "https://github.com/shantopaul/sp-http",
    dependencies: [
      { name: "json", req: "^1.2.0" },
      { name: "crypto", req: "^1.0.4" },
    ],
    versions: [
      {
        version: "1.1.0",
        publishedAt: "2026-10-06",
        checksum: "b2c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01",
        size: "68.5 KB",
        spmMinVersion: "1.0.0",
        changelog: "Add connection pooling and TLS 1.3 certificate validation.",
      },
    ],
    readme: `# http for SP Language

Fast, lightweight HTTP client and server library with middleware routing.

## Usage
\`\`\`sp
import std.io;
import http;

fn main() -> void {
    let server = new http.Server(8080);
    
    server.get("/", (http.Request req, http.Response res) => {
        res.send("Hello from SP microservice!");
    });
    
    server.listen();
}
\`\`\`
`,
  },
  {
    name: "crypto",
    version: "1.0.4",
    description: "Cryptographic primitives including SHA-256, HMAC, AES-256-GCM, and secure random byte generation.",
    author: "Security Working Group",
    license: "Apache-2.0",
    category: "crypto",
    keywords: ["crypto", "sha256", "aes", "hmac", "security"],
    totalDownloads: 74100,
    publishedAt: "2026-10-06",
    official: true,
    repositoryUrl: "https://github.com/shantopaul/sp-crypto",
    dependencies: [],
    versions: [
      {
        version: "1.0.4",
        publishedAt: "2026-10-06",
        checksum: "c3d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef012",
        size: "42.0 KB",
        spmMinVersion: "1.0.0",
        changelog: "Constant-time comparison algorithms and AES-GCM hardware instructions.",
      },
    ],
    readme: `# crypto for SP Language

Cryptographic hashing and encryption primitives.

## Example
\`\`\`sp
import std.io;
import crypto;

fn main() -> void {
    let hash = crypto.sha256("Secret Message");
    println("SHA-256 digest: \${hash}");
}
\`\`\`
`,
  },
  {
    name: "cli",
    version: "1.3.1",
    description: "Robust command-line argument parser with subcommands, automatic help generation, and interactive prompts.",
    author: "Developer Tools WG",
    license: "MIT",
    category: "cli",
    keywords: ["cli", "argparse", "terminal", "subcommands", "flags"],
    totalDownloads: 112800,
    publishedAt: "2026-10-06",
    official: true,
    repositoryUrl: "https://github.com/shantopaul/sp-cli",
    dependencies: [],
    versions: [
      {
        version: "1.3.1",
        publishedAt: "2026-10-06",
        checksum: "d4e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0123",
        size: "28.4 KB",
        spmMinVersion: "1.0.0",
        changelog: "Add interactive multi-choice terminal prompts and colored help format.",
      },
    ],
    readme: `# cli for SP Language

Create intuitive, modern command-line tools.

## Example
\`\`\`sp
import std.io;
import cli;

fn main() -> void {
    let app = new cli.App("mytool", "1.0.0");
    app.flag("--verbose", "-v", "Enable verbose logging");
    app.parse(os.args());
}
\`\`\`
`,
  },
  {
    name: "log",
    version: "1.0.2",
    description: "Fast, structured logging library supporting JSON formatting, level filtering, and colored terminal outputs.",
    author: "SP Core Team",
    license: "Apache-2.0",
    category: "utilities",
    keywords: ["log", "logger", "structured", "json", "debug"],
    totalDownloads: 89600,
    publishedAt: "2026-10-06",
    official: true,
    repositoryUrl: "https://github.com/shantopaul/sp-log",
    dependencies: [],
    versions: [
      {
        version: "1.0.2",
        publishedAt: "2026-10-06",
        checksum: "e5f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef01234",
        size: "19.5 KB",
        spmMinVersion: "1.0.0",
        changelog: "Add asynchronous worker thread log sink with zero caller latency.",
      },
    ],
    readme: `# log for SP Language

Structured, high-throughput logging.

## Example
\`\`\`sp
import log;

fn main() -> void {
    log.info("Server started on port 8080");
    log.warn("Database connection took 120ms");
}
\`\`\`
`,
  },
  {
    name: "math-matrix",
    version: "0.9.5",
    description: "Linear algebra and matrix mathematics library optimized for scientific computing.",
    author: "Scientific SP Team",
    license: "MIT",
    category: "math",
    keywords: ["matrix", "linear-algebra", "tensors", "math", "scientific"],
    totalDownloads: 31400,
    publishedAt: "2026-10-06",
    official: false,
    repositoryUrl: "https://github.com/shantopaul/sp-math-matrix",
    dependencies: [],
    versions: [
      {
        version: "0.9.5",
        publishedAt: "2026-10-06",
        checksum: "f60718293a4b5c6d7e8f90123456789abcdef0123456789abcdef012345",
        size: "52.0 KB",
        spmMinVersion: "1.0.0",
        changelog: "AVX2 SIMD vectorization for matrix multiplication.",
      },
    ],
    readme: `# math-matrix for SP Language

Fast 2D and 3D matrix mathematics.
`,
  },
  {
    name: "db-sqlite",
    version: "1.0.1",
    description: "Embedded SQLite database driver with prepared statements and transaction safety.",
    author: "Database WG",
    license: "Apache-2.0",
    category: "database",
    keywords: ["sqlite", "database", "sql", "storage"],
    totalDownloads: 56200,
    publishedAt: "2026-10-06",
    official: true,
    repositoryUrl: "https://github.com/shantopaul/sp-db-sqlite",
    dependencies: [],
    versions: [
      {
        version: "1.0.1",
        publishedAt: "2026-10-06",
        checksum: "0718293a4b5c6d7e8f90123456789abcdef0123456789abcdef0123456",
        size: "88.2 KB",
        spmMinVersion: "1.0.0",
        changelog: "Upgrade SQLite C amalgamation to 3.46.1.",
      },
    ],
    readme: `# db-sqlite for SP Language

Native SQLite binding for SP.
`,
  },
  {
    name: "test-bench",
    version: "1.0.0",
    description: "Micro-benchmarking and test assertions library with statistical runtime variance reporting.",
    author: "Developer Tools WG",
    license: "MIT",
    category: "testing",
    keywords: ["benchmark", "testing", "assertions", "metrics"],
    totalDownloads: 45900,
    publishedAt: "2026-10-06",
    official: true,
    repositoryUrl: "https://github.com/shantopaul/sp-test-bench",
    dependencies: [],
    versions: [
      {
        version: "1.0.0",
        publishedAt: "2026-10-06",
        checksum: "18293a4b5c6d7e8f90123456789abcdef0123456789abcdef01234567",
        size: "24.0 KB",
        spmMinVersion: "1.0.0",
        changelog: "Initial 1.0.0 production GA release.",
      },
    ],
    readme: `# test-bench for SP Language

Statistical micro-benchmarking toolkit for SP.
`,
  },
];

export function getAllPackages(): Package[] {
  return PACKAGES;
}

export function getPackageByName(name: string): Package | undefined {
  return PACKAGES.find((p) => p.name.toLowerCase() === name.toLowerCase());
}

export function searchPackages(query: string, category?: string): Package[] {
  let list = PACKAGES;
  if (category) {
    list = list.filter((p) => p.category === category);
  }
  if (!query) return list;

  const q = query.toLowerCase();
  return list.filter(
    (p) =>
      p.name.toLowerCase().includes(q) ||
      p.description.toLowerCase().includes(q) ||
      p.keywords.some((k) => k.toLowerCase().includes(q))
  );
}
