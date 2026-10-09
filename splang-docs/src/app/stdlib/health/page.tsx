import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.health Module Reference",
  description: "Kubernetes liveness (/healthz) and readiness (/readyz) health check registry in SP.",
};

export default function StdHealthPage() {
  const exampleCode = `import std.health;

fn main() {
    let registry = new HealthRegistry();

    registry.addCheck("database", () => {
        // Return true if DB connection succeeds, false otherwise
        return true;
    });

    println("Service ready: " + (registry.isReady() ? "YES" : "NO"));
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.health" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.health Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.health</code> module provides health checking infrastructure specifically designed for container orchestrators like Kubernetes, Docker Swarm, and AWS ECS.
      </p>

      <CodeBlock code={exampleCode} filename="health_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="registry.addCheck"
        signature="fn addCheck(string component, fn() -> bool checker) -> void"
        description="Registers a health check callback validating a subsystem (e.g. database pool, Redis, disk)."
        parameters={[
          { name: "component", type: "string", description: "Subsystem name." },
          { name: "checker", type: "fn() -> bool", description: "Function returning true if healthy." },
        ]}
        returns="void"
      />

      <ApiCard
        name="registry.isReady"
        signature="fn isReady() -> bool"
        description="Executes all registered health checks. Returns true if and only if all checks pass."
        returns="bool readiness status"
      />

      <PrevNextNav
        prev={{ title: "std.metrics Reference", href: "/stdlib/metrics" }}
        next={{ title: "std.queue Reference", href: "/stdlib/queue" }}
      />
    </div>
  );
}
