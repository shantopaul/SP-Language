module std.web.sse;

import std.io;

// ============================================================================
// Standard Library Server-Sent Events (SSE) (SP 2.8)
// Provides event stream formatting, automatic keep-alive heartbeats,
// and W3C event stream compatibility.
// ============================================================================

extern "C" {
    fn sp_rt_sse_format(string event, string data, string id, int retry) -> string;
    fn sp_rt_sse_heartbeat() -> string;
}

public class Sse {
    public static fn format(string event, string data, string id, int retry) -> string {
        return sp_rt_sse_format(event, data, id, retry);
    }

    public static fn formatEvent(string event, string data) -> string {
        return sp_rt_sse_format(event, data, "", 0);
    }

    public static fn formatData(string data) -> string {
        return sp_rt_sse_format("", data, "", 0);
    }

    public static fn heartbeat() -> string {
        return sp_rt_sse_heartbeat();
    }
}

public class SseStream {
    public int eventCount;

    public SseStream() {
        this.eventCount = 0;
    }

    public fn init() {
        this.eventCount = 0;
    }

    public static fn create() -> SseStream {
        return new SseStream();
    }

    public fn formatEvent(string event, string data) -> string {
        this.eventCount = this.eventCount + 1;
        return Sse.formatEvent(event, data);
    }

    public fn formatData(string data) -> string {
        this.eventCount = this.eventCount + 1;
        return Sse.formatData(data);
    }

    public fn formatHeartbeat() -> string {
        return Sse.heartbeat();
    }
}
