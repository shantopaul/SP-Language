import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Database, Zap, RefreshCw, Send } from "lucide-react";

export const metadata = {
  title: "Redis Caching & Pub/Sub (std.redis)",
  description: "RESP2/3 Redis client, connection pooling, and distributed Pub/Sub in SP 4.0.",
};

export default function BackendRedisPage() {
  const redisCachingCode = `import std.io;
import std.web;
import std.redis;

fn main() {
    let app = new App();

    // 1. Initialize Thread-Safe Redis Connection Pool
    let redisPool = new RedisPool("127.0.0.1", 6379, 16);

    app.get("/api/v1/products/:id", (ptr req) => {
        let ctx = Context.create(req);
        let id = ctx.param("id");
        let cacheKey = "product:" + id;

        let client = redisPool.acquire();

        // Check Redis cache first
        let cached = client.get(cacheKey);
        if (cached != "") {
            redisPool.release(client);
            return ctx.json("{\\"data\\":" + cached + ",\\"cached\\":true}");
        }

        // Cache miss: fetch from primary database (simulated)
        let productJson = "{\\"id\\":\\"" + id + "\\",\\"name\\":\\"Gaming Laptop\\",\\"price\\":1299}";

        // Store in Redis with 60 second TTL (SETEX)
        client.setex(cacheKey, 60, productJson);
        redisPool.release(client);

        return ctx.json("{\\"data\\":" + productJson + ",\\"cached\\":false}");
    });

    app.listen(8080);
}`;

  const redisPubSubCode = `// Distributed Pub/Sub Messaging across SP instances
let pubClient = redisPool.acquire();

// 1. Publish invalidation event to all cluster nodes
pubClient.publish("cache:invalidate", "product:42");
redisPool.release(pubClient);

// 2. Subscriber worker thread
spawn async () => {
    let subClient = new RedisClient("127.0.0.1", 6379);
    subClient.subscribe("cache:invalidate", (channel, message) => {
        println("Received cache invalidation for: " + message);
    });
};`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 4.0 Backend Platform", href: "/backend/overview" },
          { title: "Redis Caching & Pub/Sub" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Zap className="w-3.5 h-3.5" />
          <span>Distributed In-Memory Store &bull; std.redis</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          Redis Caching &amp; Distributed Pub/Sub
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          The <code>std.redis</code> module features a zero-allocation RESP2/RESP3 protocol parser, thread-safe connection pool, and distributed Pub/Sub broadcasting for caching, session stores, and real-time clustering.
        </p>
      </div>

      <Callout type="tip" title="RESP2 / RESP3 Protocol Parser">
        The parser handles simple strings, errors, integers, bulk strings, arrays, and map structures directly into native SP types with zero intermediary string copies.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">High-Throughput Caching Route</h2>
        <p className="text-xs text-slate-400 mb-3">
          Check Redis cache before falling back to persistent databases:
        </p>
        <CodeBlock code={redisCachingCode} filename="src/cache_service.sp" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Distributed Pub/Sub Synchronization</h2>
        <p className="text-xs text-slate-400 mb-3">
          Broadcast state updates across multiple running SP server instances:
        </p>
        <CodeBlock code={redisPubSubCode} filename="src/pubsub_worker.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">Core Supported Commands</h3>
        <div className="overflow-x-auto">
          <table className="w-full text-left text-xs font-mono">
            <thead className="border-b border-slate-800 text-slate-400">
              <tr>
                <th className="pb-2 font-sans font-semibold">Category</th>
                <th className="pb-2 font-sans font-semibold">Methods</th>
                <th className="pb-2 font-sans font-semibold">Description</th>
              </tr>
            </thead>
            <tbody className="divide-y divide-slate-800/60 text-slate-300">
              <tr>
                <td className="py-2 text-cyan-400 font-sans">Strings &amp; Keys</td>
                <td className="py-2"><code>get, set, setex, del, exists, expire, ttl</code></td>
                <td className="py-2">Basic key-value storage with TTL expiration</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400 font-sans">Counters</td>
                <td className="py-2"><code>incr, decr, incrby</code></td>
                <td className="py-2">Atomic integer operations for rate limiters &amp; counters</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400 font-sans">Hashes</td>
                <td className="py-2"><code>hget, hset, hdel, hgetall</code></td>
                <td className="py-2">Field-value mappings for user sessions and profiles</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400 font-sans">Lists</td>
                <td className="py-2"><code>lpush, rpush, lpop, rpop, llen, lrange</code></td>
                <td className="py-2">Ordered queues and event logs</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400 font-sans">Pub/Sub</td>
                <td className="py-2"><code>publish, subscribe</code></td>
                <td className="py-2">Distributed event messaging across cluster nodes</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "WebSockets & Real-Time", href: "/backend/websockets" }}
        next={{ title: "Observability & Telemetry", href: "/backend/observability" }}
      />
    </div>
  );
}
