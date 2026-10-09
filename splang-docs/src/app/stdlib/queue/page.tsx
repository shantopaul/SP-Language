import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.queue Module Reference",
  description: "In-process background workers, exponential backoff retries, and cron scheduling in SP.",
};

export default function StdQueuePage() {
  const exampleCode = `import std.queue;

fn main() {
    let queue = new JobQueue(2);

    queue.registerHandler("process_image", (payload) => {
        println("Processing image: " + payload);
    }, 3, 500);

    queue.enqueue("process_image", "avatar.png");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.queue" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.queue Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.queue</code> module provides in-process asynchronous task execution, worker threadpools, automatic retries with exponential backoff, dead-letter routing, and cron-style recurring job scheduling.
      </p>

      <CodeBlock code={exampleCode} filename="queue_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="JobQueue"
        signature="new JobQueue(int numWorkers = 4)"
        description="Creates a background task dispatcher with the designated number of concurrent worker threads."
        parameters={[{ name: "numWorkers", type: "int", description: "Number of background worker threads." }]}
        returns="JobQueue instance"
      />

      <ApiCard
        name="queue.enqueue"
        signature="fn enqueue(string taskType, string payload) -> bool"
        description="Pushes a task onto the work queue for asynchronous execution by an idle worker."
        parameters={[
          { name: "taskType", type: "string", description: "Registered task handler identifier." },
          { name: "payload", type: "string", description: "Task input data payload." },
        ]}
        returns="bool enqueue success"
      />

      <ApiCard
        name="Cron.schedule"
        signature="static fn schedule(string cronExpression, fn() -> void task) -> void"
        description="Registers a recurring periodic task using standard 5-field cron syntax."
        parameters={[
          { name: "cronExpression", type: "string", description: "5-field cron string (e.g. '*/5 * * * *')." },
          { name: "task", type: "fn() -> void", description: "Callback task to invoke on trigger." },
        ]}
        returns="void"
      />

      <PrevNextNav
        prev={{ title: "std.health Reference", href: "/stdlib/health" }}
        next={{ title: "Developer Tooling", href: "/tools" }}
      />
    </div>
  );
}
