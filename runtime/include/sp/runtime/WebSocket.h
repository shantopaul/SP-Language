#pragma once

#include "sp/runtime/Net.h"
#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>

namespace sp::rt::ws {

// ============================================================================
// RFC 6455 Constants & Types
// ============================================================================

constexpr const char* WS_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

enum class WsOpCode : uint8_t {
    Continuation = 0x0,
    Text         = 0x1,
    Binary       = 0x2,
    Close        = 0x8,
    Ping         = 0x9,
    Pong         = 0xA
};

struct WsFrame {
    bool fin{true};
    WsOpCode opcode{WsOpCode::Text};
    bool masked{false};
    uint8_t maskKey[4]{0, 0, 0, 0};
    std::string payload;
    uint16_t closeCode{1000};
    std::string closeReason;
};

// ============================================================================
// RFC 6455 Handshake & Frame Utilities
// ============================================================================

std::string computeWebSocketAccept(const std::string& clientKey);

std::vector<uint8_t> encodeFrame(const WsFrame& frame);
std::vector<uint8_t> encodeTextFrame(const std::string& text, bool masked = false);
std::vector<uint8_t> encodeBinaryFrame(const uint8_t* data, size_t length, bool masked = false);
std::vector<uint8_t> encodePingFrame(const std::string& data = "", bool masked = false);
std::vector<uint8_t> encodePongFrame(const std::string& data = "", bool masked = false);
std::vector<uint8_t> encodeCloseFrame(uint16_t code = 1000, const std::string& reason = "", bool masked = false);

bool decodeFrame(const uint8_t* data, size_t length, WsFrame& outFrame, size_t& bytesConsumed);

// ============================================================================
// WebSocket Connection
// ============================================================================

class WebSocketConnection : public std::enable_shared_from_this<WebSocketConnection> {
public:
    using TextHandler = std::function<void(const std::string&)>;
    using BinaryHandler = std::function<void(const std::vector<uint8_t>&)>;
    using CloseHandler = std::function<void(uint16_t, const std::string&)>;
    using ErrorHandler = std::function<void(const std::string&)>;
    using SendCallback = std::function<void(const std::vector<uint8_t>&)>;

    explicit WebSocketConnection(uint64_t id, SendCallback sender = nullptr);
    ~WebSocketConnection() = default;

    uint64_t getId() const { return m_id; }
    const std::string& getRoom() const { return m_room; }
    void setRoom(const std::string& room) { m_room = room; }

    bool isOpen() const { return m_open; }

    void sendText(const std::string& text);
    void sendBinary(const uint8_t* data, size_t length);
    void ping(const std::string& data = "");
    void pong(const std::string& data = "");
    void close(uint16_t code = 1000, const std::string& reason = "");

    void onText(TextHandler handler) { m_onText = std::move(handler); }
    void onBinary(BinaryHandler handler) { m_onBinary = std::move(handler); }
    void onClose(CloseHandler handler) { m_onClose = std::move(handler); }
    void onError(ErrorHandler handler) { m_onError = std::move(handler); }

    // Incoming frame processing
    void handleIncomingBytes(const uint8_t* data, size_t length);

    // Inspection
    const std::vector<std::string>& getReceivedMessages() const { return m_receivedMessages; }

private:
    uint64_t m_id{0};
    std::string m_room;
    bool m_open{true};
    SendCallback m_sender;

    TextHandler m_onText;
    BinaryHandler m_onBinary;
    CloseHandler m_onClose;
    ErrorHandler m_onError;

    std::vector<uint8_t> m_recvBuffer;
    std::vector<std::string> m_receivedMessages;
    mutable std::mutex m_mutex;
};

// ============================================================================
// WebSocket Hub (Rooms, Broadcasting & Multi-Client Registry)
// ============================================================================

class WebSocketHub {
public:
    WebSocketHub() = default;
    ~WebSocketHub() = default;

    WebSocketHub(const WebSocketHub&) = delete;
    WebSocketHub& operator=(const WebSocketHub&) = delete;

    void addConnection(std::shared_ptr<WebSocketConnection> conn);
    void removeConnection(uint64_t id);
    std::shared_ptr<WebSocketConnection> getConnection(uint64_t id);

    void broadcast(const std::string& message);
    void broadcastTo(const std::string& room, const std::string& message);

    void joinRoom(uint64_t id, const std::string& room);
    void leaveRoom(uint64_t id, const std::string& room);

    size_t size() const;
    size_t roomSize(const std::string& room) const;

    void clear();

private:
    mutable std::mutex m_mutex;
    std::unordered_map<uint64_t, std::shared_ptr<WebSocketConnection>> m_connections;
    std::unordered_map<std::string, std::unordered_set<uint64_t>> m_rooms;
};

// ============================================================================
// WebSocket Server Transport (RFC 6455 over the HTTP server)
// ============================================================================
//
// The HTTP server hands an upgraded connection to WebSocketServer, which
// performs the 101 handshake and then runs the frame loop on the calling
// (worker) thread until the connection closes. Text handlers are registered
// per path; a handler is a plain function invoked for every text frame
// received on that path (an SP `fn` passed as `ptr` through the C ABI is a
// native function pointer, so it is invoked directly, not stubbed).
//
// One process-wide registry: an SP program normally runs a single
// HttpServer, so routes registered here apply to every HttpServer in the
// process. This is documented, not hidden.

using WsTextHandler = std::function<void(uint64_t connId, const std::string& message)>;

// Minimal byte transport for an upgraded connection. The HTTP server adapts
// its connection to this interface, so the frame loop works identically
// over plaintext TCP and over TLS (wss://) without WebSocket.cpp knowing
// about either. readSome() returns "" on close/error.
class WsTransport {
public:
    virtual ~WsTransport() = default;
    virtual std::string readSome(size_t maxLength) = 0;
    virtual int64_t writeString(const std::string& str) = 0;
    virtual bool isValid() const = 0;
};

class WebSocketServer {
public:
    static WebSocketServer& getGlobal();

    WebSocketServer() = default;
    ~WebSocketServer() = default;

    WebSocketServer(const WebSocketServer&) = delete;
    WebSocketServer& operator=(const WebSocketServer&) = delete;

    // Register the text handler for one upgrade path (e.g. "/echo").
    void onText(const std::string& path, WsTextHandler handler);
    [[nodiscard]] bool hasRoute(const std::string& path);

    // Perform the 101 handshake and run the frame loop on the calling
    // thread until the peer closes or the connection errors. `leftover`
    // holds bytes already read past the HTTP request headers. Returns
    // false when no route is registered for the path (caller then falls
    // back to a normal HTTP response); true once the connection is done.
    bool serveUpgraded(WsTransport& transport, const std::string& path,
                       const std::string& clientKey, const std::string& leftover);

    // Send from any thread (writes are serialized per connection).
    bool sendTo(uint64_t connId, const std::string& message);
    void broadcast(const std::string& message);
    void closeConnection(uint64_t connId, uint16_t code = 1000, const std::string& reason = "");
    [[nodiscard]] size_t connectionCount() const;

private:
    std::atomic<uint64_t> m_nextConnId{1};
    WebSocketHub m_hub;
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, WsTextHandler> m_textHandlers;
};

} // namespace sp::rt::ws
