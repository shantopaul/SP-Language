module std.net.ws;

import std.io;

// ============================================================================
// Standard Library WebSocket (RFC 6455) Engine (SP 2.8)
// Provides handshake verification, frame encoding/decoding, and connection hub.
// ============================================================================

extern "C" {
    fn sp_rt_ws_accept_key(string client_key) -> string;
    fn sp_rt_ws_hub_create() -> ptr;
    fn sp_rt_ws_hub_destroy(ptr hub);
    fn sp_rt_ws_hub_broadcast(ptr hub, string msg);
    fn sp_rt_ws_hub_broadcast_room(ptr hub, string room, string msg);
    fn sp_rt_ws_hub_join(ptr hub, int conn_id, string room);
    fn sp_rt_ws_hub_leave(ptr hub, int conn_id, string room);
    fn sp_rt_ws_hub_size(ptr hub) -> int;
    fn sp_rt_ws_hub_room_size(ptr hub, string room) -> int;
    // Phase 3 slice A: route registration + send. The handler is a bare
    // SP fn passed as `ptr` (its real address is lowered by the
    // compiler); the runtime bridge calls it with the C ABI below.
    //   handler: fn onMessage(int64 connId, string msg)
    // `int64` matches the C `int64_t conn_id` exactly.
    fn sp_rt_ws_route(string path, ptr handlerFnPtr);
    fn sp_rt_ws_send(int64 conn_id, string msg) -> bool;
    fn sp_rt_ws_broadcast(string msg);
}

// Phase 3 slice A: route text messages on `path` to an SP handler fn:
//   fn onChat(int64 connId, string msg) { wsSend(connId, "echo: " + msg); }
//   wsRoute("/chat", onChat);
// The handler runs on the connection's worker thread and may call
// wsSend. The route registry is process-wide.
public fn wsRoute(string path, ptr handlerFnPtr) {
    sp_rt_ws_route(path, handlerFnPtr);
}

public fn wsSend(int64 connId, string msg) -> bool {
    return sp_rt_ws_send(connId, msg);
}

public fn wsBroadcast(string msg) {
    sp_rt_ws_broadcast(msg);
}

public class WebSocket {
    public static fn computeAccept(string clientKey) -> string {
        return sp_rt_ws_accept_key(clientKey);
    }

    public static fn isUpgrade(string upgradeHeader, string connHeader) -> bool {
        return upgradeHeader == "websocket" || upgradeHeader == "WebSocket";
    }
}

public class WebSocketHub {
    public ptr handle;

    public WebSocketHub() {
        this.handle = sp_rt_ws_hub_create();
    }

    public fn init() {
        if (this.handle == null) {
            this.handle = sp_rt_ws_hub_create();
        }
    }

    public static fn create() -> WebSocketHub {
        return new WebSocketHub();
    }

    public fn broadcast(string message) {
        if (this.handle != null) {
            sp_rt_ws_hub_broadcast(this.handle, message);
        }
    }

    public fn broadcastTo(string room, string message) {
        if (this.handle != null) {
            sp_rt_ws_hub_broadcast_room(this.handle, room, message);
        }
    }

    public fn join(int connId, string room) {
        if (this.handle != null) {
            sp_rt_ws_hub_join(this.handle, connId, room);
        }
    }

    public fn leave(int connId, string room) {
        if (this.handle != null) {
            sp_rt_ws_hub_leave(this.handle, connId, room);
        }
    }

    public fn size() -> int {
        if (this.handle != null) {
            return sp_rt_ws_hub_size(this.handle);
        }
        return 0;
    }

    public fn roomSize(string room) -> int {
        if (this.handle != null) {
            return sp_rt_ws_hub_room_size(this.handle, room);
        }
        return 0;
    }

    public fn destroy() {
        if (this.handle != null) {
            sp_rt_ws_hub_destroy(this.handle);
            this.handle = null;
        }
    }
}
