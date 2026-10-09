import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.redis Module Reference",
  description: "RESP2/3 Redis client, connection pooling, and distributed Pub/Sub in SP.",
};

export default function StdRedisPage() {
  const exampleCode = `import std.redis;

fn main() {
    let pool = new RedisPool("127.0.0.1", 6379, 8);
    let client = pool.acquire();

    client.setex("user:108:session", 3600, "active");
    let status = client.get("user:108:session");
    println("Session: " + status);

    pool.release(client);
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.redis" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.redis Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.redis</code> module provides high-speed Redis caching, connection pooling, and distributed Pub/Sub messaging using a zero-allocation RESP2/3 parser.
      </p>

      <CodeBlock code={exampleCode} filename="redis_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="RedisPool"
        signature="new RedisPool(string host, int port, int poolSize)"
        description="Creates a thread-safe connection pool connected to a Redis instance."
        parameters={[
          { name: "host", type: "string", description: "Redis host address." },
          { name: "port", type: "int", description: "Redis port (default: 6379)." },
          { name: "poolSize", type: "int", description: "Maximum connection count." },
        ]}
        returns="RedisPool instance"
      />

      <ApiCard
        name="client.get"
        signature="fn get(string key) -> string"
        description="Retrieves the string value associated with a Redis key."
        parameters={[{ name: "key", type: "string", description: "Redis key name." }]}
        returns="String value, or empty string on cache miss"
      />

      <ApiCard
        name="client.setex"
        signature="fn setex(string key, int seconds, string val) -> bool"
        description="Sets key value with an atomic TTL expiration."
        parameters={[
          { name: "key", type: "string", description: "Redis key." },
          { name: "seconds", type: "int", description: "Expiration time in seconds." },
          { name: "val", type: "string", description: "Value to store." },
        ]}
        returns="bool success status"
      />

      <ApiCard
        name="client.publish"
        signature="fn publish(string channel, string message) -> int"
        description="Publishes a message to a named Pub/Sub channel."
        parameters={[
          { name: "channel", type: "string", description: "Pub/Sub channel name." },
          { name: "message", type: "string", description: "Payload string." },
        ]}
        returns="Number of subscribed clients receiving the message"
      />

      <PrevNextNav
        prev={{ title: "std.db Reference", href: "/stdlib/db" }}
        next={{ title: "std.security Reference", href: "/stdlib/security" }}
      />
    </div>
  );
}
