import React from "react";
import { Breadcrumbs } from "@/components/ui/Breadcrumbs";
import { ApiCard } from "@/components/docs/ApiCard";
import { CodeBlock } from "@/components/ui/CodeBlock";
import { PrevNextNav } from "@/components/ui/PrevNextNav";

export const metadata = {
  title: "std.net.ws Module Reference",
  description: "RFC 6455 full-duplex WebSockets and connection hub in SP.",
};

export default function StdWsPage() {
  const exampleCode = `import std.web;
import std.net.ws;

let app = new App();
let hub = new WebSocketHub();

app.get("/ws", (ptr req) => {
    let conn = WebSocketConnection.upgrade(req);
    hub.register(conn);

    while (conn.isOpen()) {
        let msg = conn.receiveText();
        hub.broadcast(msg);
    }
    return "";
});`;

  return (
    <div className="space-y-6">
      <Breadcrumbs
        items={[
          { title: "Standard Library", href: "/stdlib" },
          { title: "std.net.ws" },
        ]}
      />

      <h1 className="text-3xl font-extrabold text-white">std.net.ws Module Reference</h1>
      <p className="text-sm text-slate-300">
        The <code>std.net.ws</code> module provides an RFC 6455 conforming WebSocket server engine, frame parser, serializer, and multi-client broadcast hub.
      </p>

      <CodeBlock code={exampleCode} filename="ws_example.sp" />

      <h2 className="text-xl font-bold text-white pt-4">Exported APIs</h2>

      <ApiCard
        name="WebSocketConnection.upgrade"
        signature="static fn upgrade(ptr req) -> WebSocketConnection"
        description="Performs RFC 6455 HTTP upgrade handshake and returns connected socket wrapper."
        parameters={[{ name: "req", type: "ptr", description: "Native request handle pointer." }]}
        returns="WebSocketConnection instance"
      />

      <ApiCard
        name="conn.sendText"
        signature="fn sendText(string msg) -> bool"
        description="Encodes and transmits an unmasked UTF-8 text frame (opcode 0x01)."
        parameters={[{ name: "msg", type: "string", description: "Message string to transmit." }]}
        returns="bool delivery status"
      />

      <ApiCard
        name="hub.broadcast"
        signature="fn broadcast(string msg) -> void"
        description="Broadcasts text frame concurrently to all registered connections in the hub."
        parameters={[{ name: "msg", type: "string", description: "Message payload." }]}
        returns="void"
      />

      <ApiCard
        name="hub.broadcastToRoom"
        signature="fn broadcastToRoom(string room, string msg) -> void"
        description="Broadcasts frame exclusively to sockets joined to the specified room identifier."
        parameters={[
          { name: "room", type: "string", description: "Target room name." },
          { name: "msg", type: "string", description: "Message payload." },
        ]}
        returns="void"
      />

      <PrevNextNav
        prev={{ title: "std.security Reference", href: "/stdlib/security" }}
        next={{ title: "std.log Reference", href: "/stdlib/log" }}
      />
    </div>
  );
}
