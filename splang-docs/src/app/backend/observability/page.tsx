import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Activity, BarChart2, HeartHandshake, Eye } from "lucide-react";

export const metadata = {
  title: "Observability & Telemetry (std.log, std.metrics, std.health)",
  description: "Prometheus scraping, structured JSON logging, and Kubernetes probes in SP 3.0.",
};

export default function BackendObservabilityPage() {
  const obsSetupCode = `import std.io;
import std.web;
import std.log;
import std.metrics;
import std.health;

fn main() {
    let app = new App();

    // 1. Structured JSON Logger
    let logger = new Logger();
    logger.setLevel(LogLevel.INFO);
    logger.info("Initializing enterprise backend service...", "env", "production");

    // 2. Prometheus Metrics Registry
    let registry = new MetricsRegistry();
    let httpRequests = registry.counter("http_requests_total", "Total incoming HTTP requests");
    let activeSessions = registry.gauge("active_sessions", "Current live user sessions");
    let latencyHist = registry.histogram("http_request_duration_seconds", "HTTP request latency");

    // Expose standard /metrics Prometheus scrape endpoint
    app.get("/metrics", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.text(registry.scrape());
    });

    // 3. Kubernetes Liveness & Readiness Probes
    let health = new HealthRegistry();
    health.addCheck("database", () => checkDbConnectivity());
    health.addCheck("redis", () => checkRedisConnectivity());

    app.get("/healthz", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\\"status\\":\\"alive\\"}");
    });

    app.get("/readyz", (ptr req) => {
        let ctx = Context.create(req);
        if (health.isReady()) {
            return ctx.json("{\\"status\\":\\"ready\\"}");
        } else {
            return ctx.json("{\\"status\\":\\"not_ready\\",\\"errors\\":\\"db offline\\"}", 503);
        }
    });

    app.listen(8080);
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 3.0 Backend Platform", href: "/backend/overview" },
          { title: "Observability & Telemetry" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Activity className="w-3.5 h-3.5" />
          <span>Telemetry &bull; std.log &amp; std.metrics</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Observability, Prometheus &amp; Health Probes
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          Production backends require comprehensive visibility. SP provides native Prometheus metrics registries, high-speed structured JSON logging, and Kubernetes readiness/liveness check endpoints.
        </p>
      </div>

      <Callout type="tip" title="Official Prometheus Text Format">
        The <code>/metrics</code> output matches the official OpenMetrics and Prometheus text specification, allowing immediate integration with Grafana Agent, Datadog, and VictoriaMetrics without sidecars.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Metrics, Probes &amp; Logging Pipeline</h2>
        <p className="text-xs text-slate-400 mb-3">
          Configure Prometheus scrape endpoints, Kubernetes probes, and structured logs:
        </p>
        <CodeBlock code={obsSetupCode} filename="src/telemetry.sp" />
      </div>

      <div className="space-y-6">
        <h2 className="text-xl font-bold text-white">Observability Modules</h2>

        <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
          <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
            <BarChart2 className="w-4 h-4 text-cyan-400" />
            <span>std.metrics &bull; Prometheus Registry</span>
          </h3>
          <p className="text-xs text-slate-400 mb-3">
            Track counts, gauges, and request latency percentiles (p50, p90, p99):
          </p>
          <ul className="list-disc list-inside text-xs text-slate-300 space-y-1 font-mono">
            <li><code>registry.counter(name, help)</code>: Monotonically increasing counter</li>
            <li><code>registry.gauge(name, help)</code>: Values that can increase or decrease (active connections)</li>
            <li><code>registry.histogram(name, help)</code>: Quantile distribution buckets (latency)</li>
            <li><code>registry.scrape()</code>: Formats all registered metrics into Prometheus text format</li>
          </ul>
        </div>

        <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
          <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
            <Eye className="w-4 h-4 text-emerald-400" />
            <span>std.log &bull; Structured JSON Logging</span>
          </h3>
          <p className="text-xs text-slate-400 mb-3">
            Emit machine-readable JSON logs directly ingested by FluentBit, vector, and Loki:
          </p>
          <pre className="p-3 bg-slate-950 rounded-lg text-xs font-mono text-emerald-400 overflow-x-auto border border-slate-800">
            {`{"timestamp":"2026-10-09T12:00:00Z","level":"INFO","msg":"User signed in","user_id":"108","ip":"192.168.1.10"}`}
          </pre>
        </div>

        <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
          <h3 className="text-base font-bold text-white mb-2 flex items-center gap-2">
            <HeartHandshake className="w-4 h-4 text-rose-400" />
            <span>std.health &bull; Kubernetes Probes</span>
          </h3>
          <p className="text-xs text-slate-400">
            Standard <code>/healthz</code> (liveness) checks if the process is responsive, while <code>/readyz</code> (readiness) validates database connections and downstream caches before routing traffic.
          </p>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Redis Caching & Pub/Sub", href: "/backend/redis" }}
        next={{ title: "Background Job Queues", href: "/backend/queue" }}
      />
    </div>
  );
}
