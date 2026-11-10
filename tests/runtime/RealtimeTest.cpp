#include <gtest/gtest.h>
#include "sp/runtime/WebSocket.h"
#include "sp/runtime/Http.h"
#include "sp/runtime/Net.h"
#include "sp/runtime/Sse.h"
#include "sp/runtime/Redis.h"
#include "sp/runtime/Crypto.h"
#include "sp/runtime/Runtime.h"
#include <thread>
#include <chrono>
#include <vector>
#include <atomic>
#include <string>

using namespace sp::rt::ws;
using namespace sp::rt::sse;
using namespace sp::rt::redis;

// ============================================================================
// 1. WebSocket Handshake (RFC 6455 §1.3)
// ============================================================================

TEST(RealtimeTest, WebSocketHandshakeAcceptKeyRFC6455Vector) {
    // Official test vector from RFC 6455 §1.3:
    // Client Key: "dGhlIHNhbXBsZSBub25jZQ=="
    // Accept Key: "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
    std::string clientKey = "dGhlIHNhbXBsZSBub25jZQ==";
    std::string expectedAccept = "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=";

    std::string calculatedAccept = computeWebSocketAccept(clientKey);
    EXPECT_EQ(calculatedAccept, expectedAccept);

    // Verify C-ABI Bridge
    const char* cAbiAccept = sp_rt_ws_accept_key(clientKey.c_str());
    EXPECT_STREQ(cAbiAccept, expectedAccept.c_str());
}

// ============================================================================
// 2. WebSocket Frame Encoding & Decoding (RFC 6455 §5.2)
// ============================================================================

TEST(RealtimeTest, WebSocketFrameEncodeDecode) {
    // 1. Text Frame (Short payload < 126 bytes) - unmasked & masked
    std::string shortText = "Hello RFC 6455 WebSocket!";
    auto encodedUnmasked = encodeTextFrame(shortText, false);
    WsFrame frame1;
    size_t consumed1 = 0;
    ASSERT_TRUE(decodeFrame(encodedUnmasked.data(), encodedUnmasked.size(), frame1, consumed1));
    EXPECT_EQ(consumed1, encodedUnmasked.size());
    EXPECT_TRUE(frame1.fin);
    EXPECT_EQ(frame1.opcode, WsOpCode::Text);
    EXPECT_FALSE(frame1.masked);
    EXPECT_EQ(frame1.payload, shortText);

    auto encodedMasked = encodeTextFrame(shortText, true);
    WsFrame frame2;
    size_t consumed2 = 0;
    ASSERT_TRUE(decodeFrame(encodedMasked.data(), encodedMasked.size(), frame2, consumed2));
    EXPECT_EQ(consumed2, encodedMasked.size());
    EXPECT_TRUE(frame2.masked);
    EXPECT_EQ(frame2.payload, shortText);

    // 2. Medium payload (126 <= len <= 65535, 2-byte extended length)
    std::string mediumText(1000, 'X');
    auto encodedMedium = encodeTextFrame(mediumText, true);
    EXPECT_EQ(encodedMedium[1] & 0x7F, 126); // length indicator
    WsFrame frame3;
    size_t consumed3 = 0;
    ASSERT_TRUE(decodeFrame(encodedMedium.data(), encodedMedium.size(), frame3, consumed3));
    EXPECT_EQ(frame3.payload.size(), 1000);
    EXPECT_EQ(frame3.payload, mediumText);

    // 3. Large payload (> 65535, 8-byte extended length)
    std::string largeText(70000, 'Z');
    auto encodedLarge = encodeTextFrame(largeText, false);
    EXPECT_EQ(encodedLarge[1] & 0x7F, 127); // length indicator
    WsFrame frame4;
    size_t consumed4 = 0;
    ASSERT_TRUE(decodeFrame(encodedLarge.data(), encodedLarge.size(), frame4, consumed4));
    EXPECT_EQ(frame4.payload.size(), 70000);
    EXPECT_EQ(frame4.payload, largeText);

    // 4. Binary Frame
    std::vector<uint8_t> binData = {0x00, 0x01, 0x02, 0xFE, 0xFF};
    auto encodedBin = encodeBinaryFrame(binData.data(), binData.size(), false);
    WsFrame frame5;
    size_t consumed5 = 0;
    ASSERT_TRUE(decodeFrame(encodedBin.data(), encodedBin.size(), frame5, consumed5));
    EXPECT_EQ(frame5.opcode, WsOpCode::Binary);
    EXPECT_EQ(frame5.payload.size(), 5);

    // 5. Ping & Pong Frames
    auto pingBytes = encodePingFrame("heartbeat-ping", false);
    WsFrame pingFrame;
    size_t pingConsumed = 0;
    ASSERT_TRUE(decodeFrame(pingBytes.data(), pingBytes.size(), pingFrame, pingConsumed));
    EXPECT_EQ(pingFrame.opcode, WsOpCode::Ping);
    EXPECT_EQ(pingFrame.payload, "heartbeat-ping");

    // 6. Close Frame
    auto closeBytes = encodeCloseFrame(1000, "Normal Closure", false);
    WsFrame closeFrame;
    size_t closeConsumed = 0;
    ASSERT_TRUE(decodeFrame(closeBytes.data(), closeBytes.size(), closeFrame, closeConsumed));
    EXPECT_EQ(closeFrame.opcode, WsOpCode::Close);
    EXPECT_EQ(closeFrame.closeCode, 1000);
    EXPECT_EQ(closeFrame.closeReason, "Normal Closure");
}

// ============================================================================
// 3. WebSocket Hub & Rooms
// ============================================================================

TEST(RealtimeTest, WebSocketHubRoomsAndBroadcasting) {
    WebSocketHub hub;

    std::vector<std::string> client1Received;
    std::vector<std::string> client2Received;
    std::vector<std::string> client3Received;

    auto conn1 = std::make_shared<WebSocketConnection>(1, [&client1Received](const std::vector<uint8_t>& bytes) {
        WsFrame f; size_t c = 0;
        if (decodeFrame(bytes.data(), bytes.size(), f, c)) client1Received.push_back(f.payload);
    });
    auto conn2 = std::make_shared<WebSocketConnection>(2, [&client2Received](const std::vector<uint8_t>& bytes) {
        WsFrame f; size_t c = 0;
        if (decodeFrame(bytes.data(), bytes.size(), f, c)) client2Received.push_back(f.payload);
    });
    auto conn3 = std::make_shared<WebSocketConnection>(3, [&client3Received](const std::vector<uint8_t>& bytes) {
        WsFrame f; size_t c = 0;
        if (decodeFrame(bytes.data(), bytes.size(), f, c)) client3Received.push_back(f.payload);
    });

    hub.addConnection(conn1);
    hub.addConnection(conn2);
    hub.addConnection(conn3);
    EXPECT_EQ(hub.size(), 3);

    // Join rooms
    hub.joinRoom(1, "lobby");
    hub.joinRoom(2, "lobby");
    hub.joinRoom(3, "vip");

    EXPECT_EQ(hub.roomSize("lobby"), 2);
    EXPECT_EQ(hub.roomSize("vip"), 1);

    // Broadcast to room
    hub.broadcastTo("lobby", "Welcome to lobby!");
    EXPECT_EQ(client1Received.size(), 1);
    EXPECT_EQ(client1Received[0], "Welcome to lobby!");
    EXPECT_EQ(client2Received.size(), 1);
    EXPECT_EQ(client2Received[0], "Welcome to lobby!");
    EXPECT_EQ(client3Received.size(), 0); // Not in lobby!

    // Broadcast to all
    hub.broadcast("System maintenance in 10 minutes");
    EXPECT_EQ(client1Received.size(), 2);
    EXPECT_EQ(client2Received.size(), 2);
    EXPECT_EQ(client3Received.size(), 1);
    EXPECT_EQ(client3Received[0], "System maintenance in 10 minutes");

    // Leave room
    hub.leaveRoom(1, "lobby");
    EXPECT_EQ(hub.roomSize("lobby"), 1);

    // Remove connection
    hub.removeConnection(2);
    EXPECT_EQ(hub.size(), 2);
}

// ============================================================================
// 4. Server-Sent Events (SSE)
// ============================================================================

TEST(RealtimeTest, SseFormattingAndHeartbeat) {
    // 1. Simple Data
    std::string s1 = formatData("simple message");
    EXPECT_EQ(s1, "data: simple message\n\n");

    // 2. Full Event with Event, Data, ID, and Retry
    std::string s2 = formatEvent("order-status", "{\"orderId\":99,\"status\":\"shipped\"}", "evt_101", 3000);
    EXPECT_TRUE(s2.find("event: order-status\n") != std::string::npos);
    EXPECT_TRUE(s2.find("data: {\"orderId\":99,\"status\":\"shipped\"}\n") != std::string::npos);
    EXPECT_TRUE(s2.find("id: evt_101\n") != std::string::npos);
    EXPECT_TRUE(s2.find("retry: 3000\n") != std::string::npos);
    EXPECT_TRUE(s2.ends_with("\n\n"));

    // 3. Multiline data handling (W3C standard)
    std::string multiline = "line1\nline2\nline3";
    std::string s3 = formatData(multiline);
    EXPECT_TRUE(s3.find("data: line1\ndata: line2\ndata: line3\n\n") != std::string::npos);

    // 4. Heartbeat keep-alive comment
    std::string hb = formatHeartbeat();
    EXPECT_EQ(hb, ": heartbeat\n\n");

    // 5. SseStream helper
    std::vector<std::string> streamedChunks;
    SseStream stream([&streamedChunks](const std::string& chunk) {
        streamedChunks.push_back(chunk);
    });

    stream.sendEvent("user-login", "user_42");
    stream.sendHeartbeat();
    EXPECT_EQ(stream.getEventsCount(), 1);
    EXPECT_EQ(streamedChunks.size(), 2);
    EXPECT_TRUE(streamedChunks[0].find("event: user-login") != std::string::npos);
    EXPECT_EQ(streamedChunks[1], ": heartbeat\n\n");

    // 6. C-ABI bindings
    const char* cSse = sp_rt_sse_format("tick", "{\"price\":105.5}", "1", 1000);
    EXPECT_TRUE(std::string(cSse).find("event: tick\n") != std::string::npos);
    const char* cHb = sp_rt_sse_heartbeat();
    EXPECT_STREQ(cHb, ": heartbeat\n\n");
}

// ============================================================================
// 5. RESP2 / RESP3 Protocol Parser
// ============================================================================

TEST(RealtimeTest, RespParserCompliance) {
    // 1. Simple String
    std::string rawSimple = "+OK\r\n";
    RespValue val1; size_t cons1 = 0;
    ASSERT_TRUE(parseResp(rawSimple.data(), rawSimple.size(), val1, cons1));
    EXPECT_EQ(val1.type, RespType::SimpleString);
    EXPECT_EQ(val1.stringVal, "OK");
    EXPECT_EQ(cons1, rawSimple.size());

    // 2. Error String
    std::string rawErr = "-ERR unknown command\r\n";
    RespValue val2; size_t cons2 = 0;
    ASSERT_TRUE(parseResp(rawErr.data(), rawErr.size(), val2, cons2));
    EXPECT_EQ(val2.type, RespType::Error);
    EXPECT_EQ(val2.stringVal, "ERR unknown command");

    // 3. Integer
    std::string rawInt = ":1024\r\n";
    RespValue val3; size_t cons3 = 0;
    ASSERT_TRUE(parseResp(rawInt.data(), rawInt.size(), val3, cons3));
    EXPECT_EQ(val3.type, RespType::Integer);
    EXPECT_EQ(val3.intVal, 1024);

    // 4. Bulk String
    std::string rawBulk = "$6\r\nfoobar\r\n";
    RespValue val4; size_t cons4 = 0;
    ASSERT_TRUE(parseResp(rawBulk.data(), rawBulk.size(), val4, cons4));
    EXPECT_EQ(val4.type, RespType::BulkString);
    EXPECT_EQ(val4.stringVal, "foobar");

    // 5. Null Bulk String
    std::string rawNullBulk = "$-1\r\n";
    RespValue val5; size_t cons5 = 0;
    ASSERT_TRUE(parseResp(rawNullBulk.data(), rawNullBulk.size(), val5, cons5));
    EXPECT_EQ(val5.type, RespType::Null);

    // 6. Array
    std::string rawArray = "*2\r\n$3\r\nfoo\r\n$3\r\nbar\r\n";
    RespValue val6; size_t cons6 = 0;
    ASSERT_TRUE(parseResp(rawArray.data(), rawArray.size(), val6, cons6));
    EXPECT_EQ(val6.type, RespType::Array);
    ASSERT_EQ(val6.arrayVal.size(), 2);
    EXPECT_EQ(val6.arrayVal[0].stringVal, "foo");
    EXPECT_EQ(val6.arrayVal[1].stringVal, "bar");

    // 7. Command Formatter
    std::string wire = formatCommand({"SET", "user:1", "Alice"});
    EXPECT_EQ(wire, "*3\r\n$3\r\nSET\r\n$6\r\nuser:1\r\n$5\r\nAlice\r\n");
}

// ============================================================================
// 6. Redis Core Commands Execution
// ============================================================================

TEST(RealtimeTest, RedisClientCommands) {
    RedisClient client;
    client.setMockMode(true);
    ASSERT_TRUE(client.connect("127.0.0.1", 6379));

    // PING & AUTH
    EXPECT_EQ(client.ping(), "PONG");
    EXPECT_EQ(client.ping("hello"), "hello");
    EXPECT_TRUE(client.auth("secretpass"));

    // Strings & Keys
    EXPECT_TRUE(client.set("session_key", "sess_9999"));
    EXPECT_TRUE(client.exists("session_key"));
    auto val = client.get("session_key");
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, "sess_9999");

    // SETEX & TTL
    EXPECT_TRUE(client.setEx("temp_token", 3600, "token_abc"));
    EXPECT_EQ(client.ttl("temp_token"), 3600);

    // INCR & DECR
    EXPECT_EQ(client.incr("page_views"), 1);
    EXPECT_EQ(client.incr("page_views"), 2);
    EXPECT_EQ(client.decr("page_views"), 1);

    // Hashes
    EXPECT_TRUE(client.hset("user:100", "name", "Bob"));
    EXPECT_TRUE(client.hset("user:100", "role", "admin"));
    auto name = client.hget("user:100", "name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(*name, "Bob");
    auto userMap = client.hgetAll("user:100");
    EXPECT_EQ(userMap["name"], "Bob");
    EXPECT_EQ(userMap["role"], "admin");
    EXPECT_TRUE(client.hdel("user:100", "role"));
    EXPECT_FALSE(client.hget("user:100", "role").has_value());

    // Lists
    EXPECT_EQ(client.rpush("queue:tasks", "task1"), 1);
    EXPECT_EQ(client.rpush("queue:tasks", "task2"), 2);
    EXPECT_EQ(client.lpush("queue:tasks", "task0"), 3);
    EXPECT_EQ(client.llen("queue:tasks"), 3);
    auto popped = client.lpop("queue:tasks");
    ASSERT_TRUE(popped.has_value());
    EXPECT_EQ(*popped, "task0");
    auto range = client.lrange("queue:tasks", 0, -1);
    ASSERT_EQ(range.size(), 2);
    EXPECT_EQ(range[0], "task1");
    EXPECT_EQ(range[1], "task2");

    // DEL
    EXPECT_TRUE(client.del("session_key"));
    EXPECT_FALSE(client.exists("session_key"));
}

// ============================================================================
// 7. Redis Pub/Sub
// ============================================================================

TEST(RealtimeTest, RedisPubSubMessaging) {
    RedisClient redis;
    redis.setMockMode(true);
    ASSERT_TRUE(redis.connect());

    std::vector<std::string> chatMessages;
    std::vector<std::string> alertMessages;

    redis.subscribe({"chat:general"}, [&chatMessages](const std::string&, const std::string& msg) {
        chatMessages.push_back(msg);
    });

    redis.subscribe({"alerts"}, [&alertMessages](const std::string&, const std::string& msg) {
        alertMessages.push_back(msg);
    });

    int64_t notified1 = redis.publish("chat:general", "Alice joined the room");
    EXPECT_EQ(notified1, 1);
    ASSERT_EQ(chatMessages.size(), 1);
    EXPECT_EQ(chatMessages[0], "Alice joined the room");

    int64_t notified2 = redis.publish("alerts", "CPU usage at 95%");
    EXPECT_EQ(notified2, 1);
    ASSERT_EQ(alertMessages.size(), 1);
    EXPECT_EQ(alertMessages[0], "CPU usage at 95%");

    // No subscribers on unused channel
    int64_t notified0 = redis.publish("unused_channel", "anyone there?");
    EXPECT_EQ(notified0, 0);
}

// ============================================================================
// 8. Redis Connection Pool Concurrency
// ============================================================================

TEST(RealtimeTest, RedisConnectionPoolConcurrency) {
    RedisPoolConfig cfg;
    cfg.minConnections = 2;
    cfg.maxConnections = 6;
    cfg.mockMode = true;

    RedisPool pool(cfg);
    EXPECT_EQ(pool.total(), 2);

    constexpr int numThreads = 6;
    constexpr int opsPerThread = 25;
    std::vector<std::thread> workers;
    std::atomic<int> completedOps{0};

    for (int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&pool, t, &completedOps]() {
            for (int op = 0; op < opsPerThread; ++op) {
                auto client = pool.acquire();
                if (client) {
                    std::string key = "thread_counter_" + std::to_string(t);
                    client->incr(key);
                    pool.release(client);
                    completedOps++;
                }
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    EXPECT_EQ(completedOps.load(), numThreads * opsPerThread);
    EXPECT_GE(pool.total(), 2);
    EXPECT_LE(pool.total(), 6);
    pool.close();
}

// ============================================================================
// 8b. Redis Honesty: connect failure is explicit, mock is opt-in only
// ============================================================================

TEST(RealtimeTest, RedisConnectFailureIsExplicitNotMock) {
    // Default mode is live, NOT mock: a refused connection must fail
    // loudly instead of silently serving in-memory mock data.
    RedisClient client;
    EXPECT_FALSE(client.isMockMode());
    EXPECT_FALSE(client.connect("127.0.0.1", 63999, 500));
    EXPECT_FALSE(client.isConnected());
    EXPECT_FALSE(client.lastError().empty());

    // A live-but-disconnected client fails visibly on use.
    auto res = client.execute({"GET", "any_key"});
    EXPECT_TRUE(res.isError());
    EXPECT_NE(client.ping().find("ERR"), std::string::npos);

    // Mock mode is opt-in only, and it works when explicitly enabled.
    RedisClient mock;
    mock.setMockMode(true);
    ASSERT_TRUE(mock.connect("127.0.0.1", 63999, 500));
    EXPECT_TRUE(mock.isMockMode());
    EXPECT_TRUE(mock.isConnected());
    ASSERT_TRUE(mock.set("k", "v"));
    auto v = mock.get("k");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "v");
}

TEST(RealtimeTest, RedisPoolAcquireFailsWithoutServer) {
    RedisPoolConfig cfg;
    cfg.minConnections = 0;
    cfg.maxConnections = 1;
    cfg.acquireTimeoutMs = 200;
    // mockMode stays at its default (false): no silent mock pool.
    EXPECT_FALSE(cfg.mockMode);

    RedisPool pool(cfg);
    EXPECT_EQ(pool.total(), 0);
    EXPECT_EQ(pool.acquire(), nullptr);
    pool.close();
}

// ============================================================================
// 9. Realtime WebSocket & Redis High-Throughput Stress Test
// ============================================================================

TEST(RealtimeTest, WebSocketAndRedisStress) {
    WebSocketHub wsHub;
    RedisClient redis;
    redis.setMockMode(true);
    ASSERT_TRUE(redis.connect());

    std::atomic<int> wsBroadcastsReceived{0};

    // Spin up 10 simulated client connections
    constexpr int clientCount = 10;
    std::vector<std::shared_ptr<WebSocketConnection>> conns;

    for (uint64_t i = 1; i <= clientCount; ++i) {
        auto conn = std::make_shared<WebSocketConnection>(i, [&wsBroadcastsReceived](const std::vector<uint8_t>& bytes) {
            WsFrame f; size_t c = 0;
            if (decodeFrame(bytes.data(), bytes.size(), f, c)) {
                wsBroadcastsReceived++;
            }
        });
        wsHub.addConnection(conn);
        wsHub.joinRoom(i, "live-feed");
        conns.push_back(conn);
    }

    EXPECT_EQ(wsHub.roomSize("live-feed"), clientCount);

    // Simulate 200 high-frequency price updates streamed into Redis and broadcast over WebSockets
    constexpr int eventCount = 200;
    for (int e = 0; e < eventCount; ++e) {
        std::string payload = "{\"tick\":" + std::to_string(e) + ",\"price\":" + std::to_string(100.0 + e) + "}";
        redis.set("last_price", payload);
        wsHub.broadcastTo("live-feed", payload);
    }

    EXPECT_EQ(wsBroadcastsReceived.load(), clientCount * eventCount);
    auto lastPrice = redis.get("last_price");
    ASSERT_TRUE(lastPrice.has_value());
    EXPECT_TRUE(lastPrice->find("\"tick\":199") != std::string::npos);
}

// ============================================================================
// 10. Phase 31: WebSocket transport over the HTTP server (RFC 6455)
//     Real handshake + masked text frame + echo, with a raw socket client.
//     No fake: the bytes on the wire are real WS frames.
// ============================================================================

namespace {

void phase31WsEchoHandler(int64_t connId, const char* msg) {
    // Echo every text frame back to the same connection.
    sp_rt_ws_send(connId, msg ? msg : "");
}

} // anonymous namespace

TEST(RealtimeTest, Phase31WebSocketUpgradeHandshakeAndEchoRoundTrip) {
    // Register the echo route the way an SP program does (C ABI).
    sp_rt_ws_route("/echo", &phase31WsEchoHandler);

    sp::rt::http::HttpServer server;
    server.get("/ping", [](const sp::rt::http::HttpRequest&) {
        return sp::rt::http::HttpResponse::ok("pong");
    });
    ASSERT_TRUE(server.listen("127.0.0.1", 0));
    int port = server.getPort();
    ASSERT_GT(port, 0);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Raw socket client: RFC 6455 handshake, no WebSocket library.
    sp::rt::net::TcpSocket client;
    ASSERT_TRUE(client.connect("127.0.0.1", port, 3000));
    client.setTimeouts(3000, 3000);
    const std::string key = "dGhlIHNhbXBsZSBub25jZQ=="; // RFC 6455 §1.3 example
    std::string upgradeReq =
        "GET /echo HTTP/1.1\r\n"
        "Host: 127.0.0.1\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Key: " + key + "\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n";
    ASSERT_GT(client.writeString(upgradeReq), 0);

    std::string resp;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (resp.find("\r\n\r\n") == std::string::npos &&
           std::chrono::steady_clock::now() < deadline) {
        std::string chunk = client.readString(4096);
        if (chunk.empty()) break;
        resp += chunk;
    }
    EXPECT_NE(resp.find("101 Switching Protocols"), std::string::npos);
    // RFC 6455 §1.3: accept for "dGhlIHNhbXBsZSBub25jZQ==" is fixed.
    EXPECT_NE(resp.find("s3pPLMBiTxaQ9kYGzzhZRbK+xOo="), std::string::npos);
    EXPECT_GE(sp_rt_ws_connection_count(), 1);

    // Plain HTTP still works on the same server while the WS conn is open.
    auto ping = sp::rt::http::HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/ping");
    EXPECT_EQ(ping.statusCode, 200);
    EXPECT_EQ(ping.body, "pong");

    // Client frames MUST be masked (RFC 6455 §5.1). Fixed mask key so the
    // test is deterministic.
    WsFrame frame;
    frame.fin = true;
    frame.opcode = WsOpCode::Text;
    frame.masked = true;
    frame.maskKey[0] = 0x37;
    frame.maskKey[1] = 0xfa;
    frame.maskKey[2] = 0x21;
    frame.maskKey[3] = 0x3d;
    frame.payload = "hello ws";
    auto bytes = encodeFrame(frame);
    ASSERT_GT(client.write(bytes.data(), bytes.size()), 0);

    // Read the echoed (unmasked, server-side) frame.
    std::string incoming;
    WsFrame echo;
    size_t consumed = 0;
    bool gotEcho = false;
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        std::string chunk = client.readString(4096);
        if (chunk.empty()) break;
        incoming += chunk;
        if (decodeFrame(reinterpret_cast<const uint8_t*>(incoming.data()),
                        incoming.size(), echo, consumed)) {
            gotEcho = true;
            break;
        }
    }
    ASSERT_TRUE(gotEcho);
    EXPECT_EQ(echo.opcode, WsOpCode::Text);
    EXPECT_EQ(echo.payload, "hello ws");

    // Clean close, then the server must drop the connection.
    auto closeBytes = encodeCloseFrame(1000, "done", true);
    client.write(closeBytes.data(), closeBytes.size());
    client.close();
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_EQ(sp_rt_ws_connection_count(), 0);

    server.stop();
}
