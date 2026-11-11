import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Clock, RefreshCw, Cpu, Layers } from "lucide-react";

export const metadata = {
  title: "Background Job Queues & Schedulers (std.queue)",
  description: "In-process background workers, exponential backoff, dead-letter queues, and cron scheduling in SP 4.0.",
};

export default function BackendQueuePage() {
  const queueExampleCode = `import std.io;
import std.web;
import std.queue;

fn main() {
    let app = new App();

    // 1. Initialize In-Process Job Queue (4 Concurrent Worker Threads)
    let queue = new JobQueue(4);

    // 2. Register Job Handler with Automatic Retries & Exponential Backoff
    queue.registerHandler("email.welcome", (payload) => {
        println("Sending welcome email to: " + payload);
        // If an exception is thrown, the worker retries up to 3 times
        // with exponential backoff (1s, 2s, 4s) before routing to Dead-Letter Queue (DLQ)
    }, 3, 1000);

    // 3. Periodic Cron Job Scheduler
    // Runs every 5 minutes
    Cron.schedule("*/5 * * * *", () => {
        println("Executing periodic cache sweep...");
    });

    // 4. HTTP Route enqueuing asynchronous background work
    app.post("/api/v1/users", (ptr req) => {
        let ctx = Context.create(req);
        let userEmail = "dev@splang.io";

        // Non-blocking asynchronous dispatch
        queue.enqueue("email.welcome", userEmail);

        return ctx.json("{\\"status\\":\\"User created, email queued\\"}", 201);
    });

    app.listen(8080);
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 4.0 Backend Platform", href: "/backend/overview" },
          { title: "Job Queues & Schedulers" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Clock className="w-3.5 h-3.5" />
          <span>Asynchronous Tasks &bull; std.queue</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Background Workers &amp; Cron Scheduling
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          Offload compute-intensive tasks, emails, report generation, and periodic maintenance using the built-in <code>std.queue</code> module. Features thread-safe task pools, exponential backoff retries, and dead-letter queues.
        </p>
      </div>

      <Callout type="tip" title="Zero External Broker Dependency">
        For many microservices, dedicated RabbitMQ or Celery deployments are unnecessary. <code>std.queue</code> runs directly inside your compiled SP binary with negligible memory overhead.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Job Queue &amp; Cron Integration</h2>
        <p className="text-xs text-slate-400 mb-3">
          Enqueue asynchronous jobs from HTTP routes and schedule recurring cron executions:
        </p>
        <CodeBlock code={queueExampleCode} filename="src/worker_app.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Queue API Methods</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="border-b border-slate-800 text-slate-400">
              <tr>
                <th className="pb-2 font-sans font-semibold">Method</th>
                <th className="pb-2 font-sans font-semibold">Description</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 text-slate-300">
              <tr>
                <td className="py-2 text-cyan-400">new JobQueue(workers)</td>
                <td className="py-2">Spawns a thread pool of worker goroutines dedicated to job processing</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">queue.registerHandler(type, fn, retries, backoffMs)</td>
                <td className="py-2">Registers task processor callback with retry policy and backoff multiplier</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">queue.enqueue(type, payload)</td>
                <td className="py-2">Pushes task onto work channel and notifies idle worker</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">Cron.schedule(cronExpression, callback)</td>
                <td className="py-2">Registers 5-field cron pattern (min, hour, dom, mon, dow)</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Observability & Telemetry", href: "/backend/observability" }}
        next={{ title: "Static Compilation & Docker", href: "/backend/docker" }}
      />
    </div>
  );
}
