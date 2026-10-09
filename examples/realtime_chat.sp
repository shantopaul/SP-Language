module examples.realtime_chat;

import std.io;
import std.net.ws;
import std.web.sse;
import std.redis;
import std.web;

// ============================================================================
// SP 2.8 Real-Time Web & Distributed Caching Reference
//
// Features:
// - RFC 6455 WebSocket Handshake calculation & verification
// - WebSocketHub for multi-room broadcasting (lobby, dev, alerts)
// - Server-Sent Events (SSE) streaming with keep-alive heartbeats
// - Redis (RESP2 / RESP3) caching, atomic counters, and Pub/Sub broadcasting
// - High-performance backend integration with std.web
// ============================================================================

fn main() {
    println("=================================================================");
    println("   SP Lang 2.8 Real-Time Web & Distributed Caching Showcase      ");
    println("=================================================================");

    // 1. WebSocket RFC 6455 Handshake & Hub
    println("\n[1] RFC 6455 WebSocket Engine (std.net.ws):");
    let clientNonce = "dGhlIHNhbXBsZSBub25jZQ==";
    let acceptKey = WebSocket.computeAccept(clientNonce);
    println("    * Client Nonce:       " + clientNonce);
    println("    * Sec-WebSocket-Accept: " + acceptKey);
    println("    * Handshake Verified? " + (acceptKey == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=" ? "true" : "false"));

    let wsHub = new WebSocketHub();
    wsHub.init();
    wsHub.join(101, "general-chat");
    wsHub.join(102, "general-chat");
    wsHub.join(103, "dev-team");

    println("    * WebSocket Hub active rooms:");
    println("      - general-chat: " + wsHub.roomSize("general-chat") + " subscribers");
    println("      - dev-team:     " + wsHub.roomSize("dev-team") + " subscribers");

    wsHub.broadcastTo("general-chat", "{\"sender\":\"system\",\"msg\":\"Welcome to general-chat!\"}");
    println("    * Broadcasted payload to 'general-chat'");

    // 2. Server-Sent Events (SSE) Streaming
    println("\n[2] Server-Sent Events Engine (std.web.sse):");
    let sseStream = new SseStream();
    sseStream.init();

    let sampleEvent = sseStream.formatEvent("price-update", "{\"symbol\":\"SP\",\"price\":420.69}");
    println("    * Generated W3C SSE Event Chunk:");
    print(sampleEvent);

    let heartbeat = sseStream.formatHeartbeat();
    println("    * Generated SSE Heartbeat Comment:");
    print(heartbeat);

    // 3. Redis Distributed Caching & Pub/Sub
    println("\n[3] Redis Client & Connection Pool (std.redis):");
    let redis = RedisClient.connect("127.0.0.1", 6379);

    // Ping check
    let pong = redis.ping();
    println("    * Redis PING response: " + pong);

    // Key-Value Cache with TTL
    redis.setEx("session:user_42", 3600, "{\"userId\":42,\"role\":\"lead_engineer\"}");
    let cachedSession = redis.get("session:user_42");
    let sessionTtl = redis.ttl("session:user_42");
    println("    * Redis SETEX cached: " + cachedSession);
    println("    * Key TTL remaining:   " + sessionTtl + " seconds");

    // Atomic Counters
    redis.incr("analytics:websocket_messages");
    redis.incr("analytics:websocket_messages");
    let totalMessages = redis.get("analytics:websocket_messages");
    println("    * Atomic message counter: " + totalMessages);

    // Hashes & Lists
    redis.hset("user:profile:42", "handle", "antigravity");
    redis.hset("user:profile:42", "status", "coding");
    let handle = redis.hget("user:profile:42", "handle");
    println("    * User hash field 'handle': @" + handle);

    redis.rpush("chat:history:general", "Alice: Hello World");
    redis.rpush("chat:history:general", "Bob: Hey Alice!");
    let historyLen = redis.llen("chat:history:general");
    println("    * Chat history queue length: " + historyLen);

    // Redis Pub/Sub Broadcast
    let subscribersNotified = redis.publish("chat:events", "{\"event\":\"user_online\",\"user\":\"Alice\"}");
    println("    * Redis PUBLISH to channel 'chat:events' (recipients: " + subscribersNotified + ")");

    // 4. Redis Connection Pool
    let pool = new RedisPool("127.0.0.1", 6379, 2, 8);
    println("\n[4] Redis Connection Pool initialized (min: 2, max: 8):");
    println("    * Available connections: " + pool.available());
    println("    * Total pool size:       " + pool.total());

    let pooledClient = pool.acquire();
    pooledClient.set("pool:test", "active");
    println("    * Pooled client operation completed: " + pooledClient.get("pool:test"));
    pool.release(pooledClient);

    // Cleanup
    wsHub.destroy();
    redis.destroy();
    pool.destroy();

    println("\n[✓] Real-time messaging & distributed caching pipeline fully operational!");
}
