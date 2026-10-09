export interface NavItem {
  title: string;
  href: string;
  badge?: string;
}

export interface NavSection {
  title: string;
  items: NavItem[];
}

export const DOCS_NAVIGATION: NavSection[] = [
  {
    title: "SP 3.0 Backend Platform",
    items: [
      { title: "Platform Overview", href: "/backend/overview", badge: "v3.0 GA" },
      { title: "Web Framework (std.web)", href: "/backend/framework" },
      { title: "Context & Ergonomics (ctx)", href: "/backend/context" },
      { title: "Dependency Injection (di)", href: "/backend/di" },
      { title: "OpenAPI 3.0 & Swagger UI", href: "/backend/openapi" },
      { title: "In-Memory HTTP Testing", href: "/backend/testing" },
      { title: "Database & Pooling (std.db)", href: "/backend/database" },
      { title: "Security, JWT & Argon2id", href: "/backend/security" },
      { title: "WebSockets & Real-Time", href: "/backend/websockets" },
      { title: "Redis Caching & Pub/Sub", href: "/backend/redis" },
      { title: "Observability & Prometheus", href: "/backend/observability" },
      { title: "Job Queues & Schedulers", href: "/backend/queue" },
      { title: "Static Compilation & Docker", href: "/backend/docker" },
      { title: "TechEmpower Benchmarks", href: "/backend/benchmarks", badge: "185k req/s" },
    ],
  },
  {
    title: "SP 2.0 Evolution",
    items: [
      { title: "SP 2.0 Release Overview", href: "/sp2/overview" },
      { title: "Language Editions (2026/2028)", href: "/sp2/editions" },
      { title: "Compile-Time Execution (comptime)", href: "/sp2/comptime" },
      { title: "WebAssembly (wasm32-wasi)", href: "/sp2/wasm" },
      { title: "Structured Coroutines (async/await)", href: "/sp2/async-await" },
      { title: "Automated Migration (spc fix)", href: "/sp2/migration" },
    ],
  },
  {
    title: "Getting Started",
    items: [
      { title: "Introduction & Overview", href: "/getting-started" },
      { title: "Installation", href: "/getting-started/installation" },
      { title: "First Program (Hello SP)", href: "/getting-started/first-program" },
      { title: "Project Scaffolding (spm)", href: "/getting-started/project-setup" },
    ],
  },
  {
    title: "Language Tour & Syntax",
    items: [
      { title: "Syntax Overview", href: "/syntax-guide" },
      { title: "Complete Syntax Guide (1–100)", href: "/syntax-guide/all-examples", badge: "100 Examples" },
      { title: "Variables & Constants", href: "/syntax-guide/variables" },
      { title: "Primitive & Complex Types", href: "/syntax-guide/data-types" },
      { title: "Control Flow & Matching", href: "/syntax-guide/control-flow" },
      { title: "Functions & Closures", href: "/syntax-guide/functions" },
      { title: "OOP, Classes & Structs", href: "/syntax-guide/oop" },
      { title: "Zero-Cost Generics", href: "/syntax-guide/generics" },
      { title: "Lambdas & Higher-Order", href: "/syntax-guide/lambdas" },
      { title: "Null Safety & Operators", href: "/syntax-guide/null-safety" },
    ],
  },
  {
    title: "Standard Library (stdlib)",
    items: [
      { title: "Stdlib Overview", href: "/stdlib", badge: "17 Modules" },
      { title: "std.web (Web App Framework)", href: "/stdlib/web" },
      { title: "std.db (Database & Pooling)", href: "/stdlib/db" },
      { title: "std.redis (Redis & Pub/Sub)", href: "/stdlib/redis" },
      { title: "std.security (Crypto & JWT)", href: "/stdlib/security" },
      { title: "std.net.ws (WebSockets & SSE)", href: "/stdlib/ws" },
      { title: "std.log (Structured Logs)", href: "/stdlib/log" },
      { title: "std.metrics (Prometheus)", href: "/stdlib/metrics" },
      { title: "std.health (K8s Probes)", href: "/stdlib/health" },
      { title: "std.queue (Job Queues)", href: "/stdlib/queue" },
      { title: "std.io (Console I/O)", href: "/stdlib/io" },
      { title: "std.string (String Utils)", href: "/stdlib/string" },
      { title: "std.math (Math & Trig)", href: "/stdlib/math" },
      { title: "std.collections (List, Map, Set)", href: "/stdlib/collections" },
      { title: "std.fs (File System)", href: "/stdlib/fs" },
      { title: "std.time (Clock & Duration)", href: "/stdlib/time" },
      { title: "std.os (Environment & System)", href: "/stdlib/os" },
      { title: "std.process (Execution & IPC)", href: "/stdlib/process" },
    ],
  },
  {
    title: "Developer Tooling",
    items: [
      { title: "Toolchain Overview", href: "/tools" },
      { title: "Compiler CLI (spc)", href: "/tools/compiler-cli" },
      { title: "Package Manager (spm)", href: "/tools/package-manager" },
      { title: "Code Formatter (spc fmt)", href: "/tools/formatter" },
      { title: "Native Test Runner (spc test)", href: "/tools/test-runner" },
      { title: "Doc Generator (spc doc)", href: "/tools/doc-generator" },
      { title: "Language Server (spls)", href: "/tools/lsp-server" },
      { title: "DAP Debugger (spd)", href: "/tools/debugger" },
      { title: "VS Code Extension", href: "/tools/vscode-extension" },
    ],
  },
  {
    title: "Performance & Architecture",
    items: [
      { title: "7-Language Benchmarks", href: "/benchmarks" },
      { title: "TechEmpower Backend Benchmarks", href: "/backend/benchmarks", badge: "New" },
    ],
  },
];
