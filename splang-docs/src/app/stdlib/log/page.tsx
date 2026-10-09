import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.log Module Reference",
  description: "Structured JSON logging, rotating file appenders, and log level filtering in SP.",
};

export default function StdLogPage() {
  const exampleCode = `import std.log;

fn main() {
    let logger = new Logger();
    logger.setLevel(LogLevel.INFO);

    logger.info("Server started", "port", "8080", "env", "prod");
    logger.warn("High memory utilization", "usage_pct", "88");
}`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.log" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.log Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.log</code> module provides high-performance structured JSON logging with severity level filtering (TRACE, DEBUG, INFO, WARN, ERROR, FATAL), rotating file appenders, and contextual fields.
      </p>

      <CodeBlock code={exampleCode} filename="log_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="Logger.setLevel"
        signature="fn setLevel(LogLevel level) -> void"
        description="Configures minimum severity threshold for log output."
        parameters={[{ name: "level", type: "LogLevel", description: "LogLevel enum value." }]}
        returns="void"
      />

      <ApiCard
        name="logger.info"
        signature="fn info(string msg, ...fields) -> void"
        description="Emits an INFO-level log entry formatted as JSON with timestamp and contextual key-value pairs."
        parameters={[
          { name: "msg", type: "string", description: "Primary human-readable log message." },
          { name: "fields", type: "string...", description: "Optional alternating key-value string metadata pairs." },
        ]}
        returns="void"
      />

      <ApiCard
        name="logger.error"
        signature="fn error(string msg, ...fields) -> void"
        description="Emits an ERROR-level log entry to stderr or configured appender."
        parameters={[
          { name: "msg", type: "string", description: "Error explanation." },
          { name: "fields", type: "string...", description: "Contextual fields (e.g. user_id, trace_id)." },
        ]}
        returns="void"
      />

      <PrevNextNav
        prev={{ title: "std.net.ws Reference", href: "/stdlib/ws" }}
        next={{ title: "std.metrics Reference", href: "/stdlib/metrics" }}
      />
    </div>
  );
}
