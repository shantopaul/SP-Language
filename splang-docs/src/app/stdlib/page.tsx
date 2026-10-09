import React from "react";
import Link from "next/link";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ArrowRight, Boxes, Terminal, Layers } from "lucide-react";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "Standard Library Overview",
  description: "Core native standard library modules in the SP Programming Language.",
};

const MODULES = [
  { name: "std.web", href: "/stdlib/web", desc: "Official web application framework, Radix routing, middlewares, and Context ergonomics." },
  { name: "std.db", href: "/stdlib/db", desc: "SQLite and PostgreSQL thread-safe connection pools, transactions, and SQL queries." },
  { name: "std.redis", href: "/stdlib/redis", desc: "RESP2/3 Redis in-memory caching, connection pool, and distributed Pub/Sub messaging." },
  { name: "std.security", href: "/stdlib/security", desc: "Argon2id password hashing, JWT HS256 tokens, and constant-time cryptography." },
  { name: "std.net.ws", href: "/stdlib/ws", desc: "RFC 6455 full-duplex WebSockets, WebSocketHub broadcasting, and Server-Sent Events." },
  { name: "std.log", href: "/stdlib/log", desc: "Structured JSON logging with severity levels and rotating file appenders." },
  { name: "std.metrics", href: "/stdlib/metrics", desc: "Prometheus Counter, Gauge, Histogram registry and /metrics exporter." },
  { name: "std.health", href: "/stdlib/health", desc: "Kubernetes /healthz (liveness) and /readyz (readiness) probe registry." },
  { name: "std.queue", href: "/stdlib/queue", desc: "In-process background workers, exponential backoff retries, and cron scheduling." },
  { name: "std.io", href: "/stdlib/io", desc: "Console printing, input reading, and formatted streaming." },
  { name: "std.string", href: "/stdlib/string", desc: "String manipulation, UTF-8 parsing, search, and transforms." },
  { name: "std.math", href: "/stdlib/math", desc: "Trigonometric functions, square roots, powers, min/max, constants." },
  { name: "std.collections", href: "/stdlib/collections", desc: "Dynamic arrays, HashMaps, Sets, and iterator utilities." },
  { name: "std.fs", href: "/stdlib/fs", desc: "Filesystem file reading, writing, directory walking, file exists." },
  { name: "std.time", href: "/stdlib/time", desc: "Monotonic timestamps, formatting, sleep delays, Duration." },
  { name: "std.os", href: "/stdlib/os", desc: "Environment variables, CLI argument arrays, and platform metadata." },
  { name: "std.process", href: "/stdlib/process", desc: "Subprocess spawning, command execution, and exit codes." },
];

export default function StdlibOverviewPage() {
  return (
    <div className="space-y-6">
      <Breadcrumbs items={[{ title: "Standard Library", href: "/stdlib" }]} />

      <h1 className="text-3xl font-extrabold text-white">SP Core Standard Library</h1>
      <p className="text-sm text-slate-300">
        SP ships with 17 self-contained, native standard library modules designed for day-to-day systems, backend web services, and high-concurrency microservices without external C dependencies.
      </p>

      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 pt-4">
        {MODULES.map((mod) => (
          <Link
            key={mod.name}
            href={mod.href}
            className="p-5 rounded-xl border border-slate-800 bg-slate-900/40 hover:bg-slate-900/80 hover:border-cyan-500/50 transition-all group"
          >
            <div className="flex items-center justify-between mb-2">
              <h3 className="font-mono text-base font-bold text-white group-hover:text-cyan-400 transition-colors">
                {mod.name}
              </h3>
              <ArrowRight className="w-4 h-4 text-slate-500 group-hover:text-cyan-400 group-hover:translate-x-1 transition-transform" />
            </div>
            <p className="text-xs text-slate-400 leading-relaxed">{mod.desc}</p>
          </Link>
        ))}
      </div>

      <PrevNextNav
        prev={{ title: "Null Safety", href: "/syntax-guide/null-safety" }}
        next={{ title: "std.web Reference", href: "/stdlib/web" }}
      />
    </div>
  );
}
