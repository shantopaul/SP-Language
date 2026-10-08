import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Cpu, Zap, Activity } from "lucide-react";

export const metadata = {
  title: "Structured Coroutines (async / await)",
  description: "Cooperative, non-blocking asynchronous coroutines and lock-free channels in SP 2.0.",
};

export default function Sp2AsyncAwaitPage() {
  const asyncExample = `import std.io;
import std.sync;
import std.time;

// Asynchronous coroutine definition
async fn fetchClusterMetrics(string clusterId, int timeoutMs) -> string {
    println("Dispatching query to \${clusterId}...");
    // Simulated non-blocking asynchronous yield point
    time.sleepMs(timeoutMs);
    return "Status: ONLINE | Node: \${clusterId} | P99: 0.8ms";
}

fn main() -> void {
    println("Initializing SP 2.0 Coroutine Runtime...");

    // Spawn concurrent asynchronous tasks
    let taskA = spawn fetchClusterMetrics("cluster-us-west", 50);
    let taskB = spawn fetchClusterMetrics("cluster-eu-central", 30);

    // Await completions cooperatively
    let resB = await taskB;
    let resA = await taskA;

    println("Received [B]: \${resB}");
    println("Received [A]: \${resA}");
}`;

  const channelExample = `import std.io;
import std.sync;

fn main() -> void {
    // Unbounded lock-free MPMC message channel
    let ch = new sync.Channel<int>(10);

    // Producer coroutine
    spawn async () => {
        for (int i = 1; i <= 5; i = i + 1) {
            ch.send(i * 100);
        }
        ch.close();
    };

    // Consumer main loop
    while (!ch.isClosed()) {
        let msg = ch.receive();
        if (msg != null) {
            println("Processed channel message: \${msg}");
        }
    }
}`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 2.0 Evolution", href: "/sp2/overview" },
          { title: "Structured Coroutines" },
        ]}
      />

      <div>
        <h1 className="text-3xl font-extrabold text-white">Structured Coroutines (`async` / `await`)</h1>
        <p className="mt-3 text-sm text-slate-300 leading-relaxed">
          SP 2.0 delivers lightweight, cooperative asynchronous coroutines. Unlike heavyweight OS threads that require 1–2MB stack allocations per thread, SP coroutines suspend and resume on cooperative yield points with a stack frame size under a few hundred bytes.
        </p>
      </div>

      <Callout type="tip" title="High Concurrency, Minimal Footprint">
        A single machine can comfortably execute <strong>100,000+ simultaneous SP coroutines</strong> across a small fixed pool of worker threads, eliminating context-switch thrashing and memory exhaustion.
      </Callout>

      <h2 className="text-xl font-bold text-white pt-2">Async Function Declaration &amp; Spawning</h2>
      <p className="text-sm text-slate-300">
        Mark non-blocking routines with <code>async fn</code> and initiate execution with the <code>spawn</code> keyword:
      </p>
      <CodeBlock code={asyncExample} filename="coroutine_pipeline.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Lock-Free Channels (`std.sync.Channel`)</h2>
      <p className="text-sm text-slate-300">
        Pass typed data safely between coroutines without manual mutex locking:
      </p>
      <CodeBlock code={channelExample} filename="channels.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Core Principles</h2>
      <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 text-xs sm:text-sm">
        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h4 className="font-bold text-white mb-1.5 flex items-center gap-1.5">
            <Cpu className="w-4 h-4 text-cyan-400" />
            <span>Cooperative Scheduling</span>
          </h4>
          <p className="text-slate-400 leading-relaxed">
            Coroutines yield execution when awaiting pending I/O or channel operations, allowing CPU cores to immediately schedule next ready tasks.
          </p>
        </div>

        <div className="p-4 rounded-xl border border-slate-800 bg-slate-900/40">
          <h4 className="font-bold text-white mb-1.5 flex items-center gap-1.5">
            <Activity className="w-4 h-4 text-emerald-400" />
            <span>Deterministic ARC Clean-up</span>
          </h4>
          <p className="text-slate-400 leading-relaxed">
            When a coroutine completes or is cancelled, all scoped variables and closures are automatically and deterministically freed without GC pause spikes.
          </p>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "WebAssembly (wasm32-wasi)", href: "/sp2/wasm" }}
        next={{ title: "Automated Migration (spc fix)", href: "/sp2/migration" }}
      />
    </div>
  );
}
