import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.metrics Module Reference",
  description: "Prometheus Counter, Gauge, Histogram metrics registry and /metrics exporter in SP.",
};

export default function StdMetricsPage() {
  const exampleCode = `import std.metrics;

fn main() {
    let registry = new MetricsRegistry();
    let requests = registry.counter("http_requests_total", "Total request count");
    let activeConns = registry.gauge("active_connections", "Current active socket count");

    requests.inc();
    activeConns.set(42.0);

    let scrapeText = registry.scrape();
    println(scrapeText);
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.metrics" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.metrics Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.metrics</code> module provides an in-process telemetry registry matching the official Prometheus text format. Supports Counters, Gauges, and Histograms for latency tracking.
      </p>

      <CodeBlock code={exampleCode} filename="metrics_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="registry.counter"
        signature="fn counter(string name, string help) -> Counter"
        description="Registers a monotonically increasing counter metric."
        parameters={[
          { name: "name", type: "string", description: "Prometheus metric identifier." },
          { name: "help", type: "string", description: "Metric description string." },
        ]}
        returns="Counter instance"
      />

      <ApiCard
        name="registry.gauge"
        signature="fn gauge(string name, string help) -> Gauge"
        description="Registers a gauge metric representing a value that can arbitrarily go up or down."
        parameters={[
          { name: "name", type: "string", description: "Prometheus metric identifier." },
          { name: "help", type: "string", description: "Metric description string." },
        ]}
        returns="Gauge instance"
      />

      <ApiCard
        name="registry.histogram"
        signature="fn histogram(string name, string help) -> Histogram"
        description="Registers a latency histogram with configurable percentile buckets."
        parameters={[
          { name: "name", type: "string", description: "Prometheus metric identifier." },
          { name: "help", type: "string", description: "Metric description string." },
        ]}
        returns="Histogram instance"
      />

      <ApiCard
        name="registry.scrape"
        signature="fn scrape() -> string"
        description="Generates standard OpenMetrics / Prometheus plain text representation of all registered metrics."
        returns="Prometheus scrape body string"
      />

      <PrevNextNav
        prev={{ title: "std.log Reference", href: "/stdlib/log" }}
        next={{ title: "std.health Reference", href: "/stdlib/health" }}
      />
    </div>
  );
}
