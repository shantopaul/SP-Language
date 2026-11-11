import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { Callout } from "@/components/ui/Callout";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";
import { Radio, Wifi, Send, MessageSquare } from "lucide-react";

export const metadata = {
  title: "WebSockets & Server-Sent Events (std.net.ws & std.web.sse)",
  description: "RFC 6455 full-duplex WebSockets, WebSocketHub room broadcasting, and SSE in SP 4.0.",
};

export default function BackendWebSocketsPage() {
  const wsChatCode = `import std.io;
import std.web;
import std.net.ws;

fn main() {
    let app = new App();

    // 1. Initialize WebSocket Connection Hub
    let hub = new WebSocketHub();

    // 2. WebSocket Route Handler
    app.get("/ws/chat", (ptr req) => {
        let conn = WebSocketConnection.upgrade(req);
        if (conn == null) {
            return "HTTP/1.1 400 Bad Request\\r\\n\\r\\n";
        }

        // Register client in hub
        hub.register(conn);
        println("Client connected: " + conn.id());

        // Join room
        hub.joinRoom("general", conn);

        // Broadcast join notice
        hub.broadcastToRoom("general", "{\\"type\\":\\"join\\",\\"id\\":\\"" + conn.id() + "\\"}");

        // Message loop
        while (conn.isOpen()) {
            let msg = conn.receiveText();
            if (msg != "") {
                // Broadcast message to everyone in the room
                hub.broadcastToRoom("general", "{\\"user\\":\\"" + conn.id() + "\\",\\"msg\\":\\"" + msg + "\\"}");
            }
        }

        // Cleanup on disconnect
        hub.leaveRoom("general", conn);
        hub.unregister(conn);
        conn.close();
        return "";
    });

    app.listen(8080);
}`;

  const sseStreamCode = `// Server-Sent Events (SSE) Route Handler
app.get("/api/v1/events", (ptr req) => {
    let ctx = Context.create(req);

    // Returns stream chunk with text/event-stream headers
    // Heartbeats (: heartbeat\\n\\n) are emitted automatically to avoid proxy dropouts
    return ctx.sse("event: price_update\\ndata: {\\"symbol\\":\\"SP\\",\\"price\\":142.50}\\n\\n");
});`;

  return (
    <div className="space-y-8">
      <Breadcrumbs
        items={[
          { title: "SP 4.0 Backend Platform", href: "/backend/overview" },
          { title: "WebSockets & Real-Time" },
        ]}
      />

      <div>
        <div className="inline-flex items-center gap-1.5 px-3 py-1 rounded-full bg-cyan-950/60 border border-cyan-800/60 text-cyan-400 text-xs font-semibold mb-4">
          <Radio className="w-3.5 h-3.5" />
          <span>Real-Time Engine &bull; RFC 6455 &amp; SSE</span>
        </div>
        <h1 className="text-3xl sm:text-4xl font-extrabold text-white tracking-tight">
          WebSockets &amp; Server-Sent Events
        </h1>
        <p className="mt-3 text-base text-slate-300 leading-relaxed">
          Build interactive multiplayer experiences, live dashboards, and chat platforms with SP's RFC 6455 WebSocket engine and Server-Sent Events (SSE) streaming infrastructure.
        </p>
      </div>

      <Callout type="tip" title="RFC 6455 Protocol Conformance">
        The engine implements the complete RFC 6455 specification: SHA-1 handshake verification (RFC 3174), 4-byte client masking key unmasking, UTF-8 text frame validation, binary payloads, and automatic ping-pong keep-alive frames.
      </Callout>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Multi-Room WebSocket Chat Hub</h2>
        <p className="text-xs text-slate-400 mb-3">
          Manage hundreds of thousands of concurrent client connections with <code>WebSocketHub</code>:
        </p>
        <CodeBlock code={wsChatCode} filename="src/chat_server.sp" />
      </div>

      <div>
        <h2 className="text-xl font-bold text-white mb-2">Server-Sent Events (SSE)</h2>
        <p className="text-xs text-slate-400 mb-3">
          Unidirectional live event streaming directly to browser <code>EventSource</code> clients:
        </p>
        <CodeBlock code={sseStreamCode} filename="src/live_stream.sp" />
      </div>

      <div className="rounded-xl border border-slate-800 bg-slate-900/40 p-5">
        <h3 className="text-base font-bold text-white mb-3">WebSocket &amp; SSE API</h3>
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
                <td className="py-2 text-cyan-400">WebSocketConnection.upgrade(req)</td>
                <td className="py-2">Performs RFC 6455 handshake upgrade and returns connection</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.sendText(message)</td>
                <td className="py-2">Encodes and transmits an RFC 6455 text frame</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.receiveText()</td>
                <td className="py-2">Blocks until the next complete unmasked text frame is received</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">conn.close()</td>
                <td className="py-2">Transmits opcode 0x08 close control frame and closes socket</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">hub.broadcast(message)</td>
                <td className="py-2">Broadcasts frame concurrently to all active hub connections</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">hub.broadcastToRoom(room, msg)</td>
                <td className="py-2">Broadcasts frame exclusively to clients subscribed to named room</td>
              </tr>
              <tr>
                <td className="py-2 text-cyan-400">ctx.sse(data)</td>
                <td className="py-2">Formats and flushes SSE chunk with <code>text/event-stream</code> headers</td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>

      <PrevNextNav
        prev={{ title: "Security & Cryptography", href: "/backend/security" }}
        next={{ title: "Redis Caching & Pub/Sub", href: "/backend/redis" }}
      />
    </div>
  );
}
