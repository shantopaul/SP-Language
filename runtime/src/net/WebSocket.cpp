#include "sp/runtime/WebSocket.h"
#include "sp/runtime/Crypto.h"
#include <cstring>
#include <algorithm>

namespace sp::rt::ws {

std::string computeWebSocketAccept(const std::string& clientKey) {
    std::string combined = clientKey + WS_GUID;
    auto sha1Digest = sp::rt::crypto::sha1RawBytes(
        reinterpret_cast<const uint8_t*>(combined.data()), combined.size()
    );
    return sp::rt::crypto::base64EncodeBytes(sha1Digest.data(), sha1Digest.size());
}

std::vector<uint8_t> encodeFrame(const WsFrame& frame) {
    std::vector<uint8_t> out;
    size_t payloadLen = frame.payload.size();

    // Byte 0: FIN (bit 7) + Opcode (bits 0-3)
    uint8_t b0 = (frame.fin ? 0x80 : 0x00) | (static_cast<uint8_t>(frame.opcode) & 0x0F);
    out.push_back(b0);

    // Byte 1: Mask (bit 7) + Payload length indicator (bits 0-6)
    uint8_t maskBit = frame.masked ? 0x80 : 0x00;
    if (payloadLen < 126) {
        out.push_back(maskBit | static_cast<uint8_t>(payloadLen));
    } else if (payloadLen <= 0xFFFF) {
        out.push_back(maskBit | 126);
        out.push_back(static_cast<uint8_t>((payloadLen >> 8) & 0xFF));
        out.push_back(static_cast<uint8_t>(payloadLen & 0xFF));
    } else {
        out.push_back(maskBit | 127);
        for (int i = 7; i >= 0; --i) {
            out.push_back(static_cast<uint8_t>((payloadLen >> (i * 8)) & 0xFF));
        }
    }

    // Masking Key
    if (frame.masked) {
        for (int i = 0; i < 4; ++i) {
            out.push_back(frame.maskKey[i]);
        }
        // Mask payload
        for (size_t i = 0; i < payloadLen; ++i) {
            out.push_back(static_cast<uint8_t>(frame.payload[i]) ^ frame.maskKey[i % 4]);
        }
    } else {
        // Unmasked payload
        out.insert(out.end(), frame.payload.begin(), frame.payload.end());
    }

    return out;
}

std::vector<uint8_t> encodeTextFrame(const std::string& text, bool masked) {
    WsFrame frame;
    frame.fin = true;
    frame.opcode = WsOpCode::Text;
    frame.masked = masked;
    if (masked) {
        auto rnd = sp::rt::crypto::randomBytes(4);
        std::memcpy(frame.maskKey, rnd.data(), 4);
    }
    frame.payload = text;
    return encodeFrame(frame);
}

std::vector<uint8_t> encodeBinaryFrame(const uint8_t* data, size_t length, bool masked) {
    WsFrame frame;
    frame.fin = true;
    frame.opcode = WsOpCode::Binary;
    frame.masked = masked;
    if (masked) {
        auto rnd = sp::rt::crypto::randomBytes(4);
        std::memcpy(frame.maskKey, rnd.data(), 4);
    }
    if (data && length > 0) {
        frame.payload.assign(reinterpret_cast<const char*>(data), length);
    }
    return encodeFrame(frame);
}

std::vector<uint8_t> encodePingFrame(const std::string& data, bool masked) {
    WsFrame frame;
    frame.fin = true;
    frame.opcode = WsOpCode::Ping;
    frame.masked = masked;
    if (masked) {
        auto rnd = sp::rt::crypto::randomBytes(4);
        std::memcpy(frame.maskKey, rnd.data(), 4);
    }
    frame.payload = data;
    return encodeFrame(frame);
}

std::vector<uint8_t> encodePongFrame(const std::string& data, bool masked) {
    WsFrame frame;
    frame.fin = true;
    frame.opcode = WsOpCode::Pong;
    frame.masked = masked;
    if (masked) {
        auto rnd = sp::rt::crypto::randomBytes(4);
        std::memcpy(frame.maskKey, rnd.data(), 4);
    }
    frame.payload = data;
    return encodeFrame(frame);
}

std::vector<uint8_t> encodeCloseFrame(uint16_t code, const std::string& reason, bool masked) {
    WsFrame frame;
    frame.fin = true;
    frame.opcode = WsOpCode::Close;
    frame.masked = masked;
    if (masked) {
        auto rnd = sp::rt::crypto::randomBytes(4);
        std::memcpy(frame.maskKey, rnd.data(), 4);
    }
    std::string payload;
    payload.push_back(static_cast<char>((code >> 8) & 0xFF));
    payload.push_back(static_cast<char>(code & 0xFF));
    payload.append(reason);
    frame.payload = payload;
    return encodeFrame(frame);
}

bool decodeFrame(const uint8_t* data, size_t length, WsFrame& outFrame, size_t& bytesConsumed) {
    bytesConsumed = 0;
    if (length < 2) return false;

    uint8_t b0 = data[0];
    uint8_t b1 = data[1];

    outFrame.fin = (b0 & 0x80) != 0;
    outFrame.opcode = static_cast<WsOpCode>(b0 & 0x0F);
    outFrame.masked = (b1 & 0x80) != 0;

    uint8_t lenByte = b1 & 0x7F;
    size_t headerLen = 2;
    uint64_t payloadLen = 0;

    if (lenByte < 126) {
        payloadLen = lenByte;
    } else if (lenByte == 126) {
        if (length < 4) return false;
        payloadLen = (static_cast<uint64_t>(data[2]) << 8) | static_cast<uint64_t>(data[3]);
        headerLen = 4;
    } else { // 127
        if (length < 10) return false;
        payloadLen = 0;
        for (int i = 0; i < 8; ++i) {
            payloadLen = (payloadLen << 8) | static_cast<uint64_t>(data[2 + i]);
        }
        headerLen = 10;
    }

    if (outFrame.masked) {
        if (length < headerLen + 4) return false;
        std::memcpy(outFrame.maskKey, data + headerLen, 4);
        headerLen += 4;
    }

    if (length < headerLen + payloadLen) {
        return false; // Need more data
    }

    outFrame.payload.resize(static_cast<size_t>(payloadLen));
    const uint8_t* payloadSrc = data + headerLen;

    if (outFrame.masked) {
        for (size_t i = 0; i < payloadLen; ++i) {
            outFrame.payload[i] = static_cast<char>(payloadSrc[i] ^ outFrame.maskKey[i % 4]);
        }
    } else {
        std::memcpy(outFrame.payload.data(), payloadSrc, static_cast<size_t>(payloadLen));
    }

    // Parse close payload if close frame
    if (outFrame.opcode == WsOpCode::Close && payloadLen >= 2) {
        outFrame.closeCode = (static_cast<uint8_t>(outFrame.payload[0]) << 8) |
                              static_cast<uint8_t>(outFrame.payload[1]);
        outFrame.closeReason = outFrame.payload.substr(2);
    }

    bytesConsumed = headerLen + static_cast<size_t>(payloadLen);
    return true;
}

// ============================================================================
// WebSocket Connection Implementation
// ============================================================================

WebSocketConnection::WebSocketConnection(uint64_t id, SendCallback sender)
    : m_id(id), m_sender(std::move(sender)) {}

void WebSocketConnection::sendText(const std::string& text) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_sender) return;
    auto bytes = encodeTextFrame(text, false);
    m_sender(bytes);
}

void WebSocketConnection::sendBinary(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_sender) return;
    auto bytes = encodeBinaryFrame(data, length, false);
    m_sender(bytes);
}

void WebSocketConnection::ping(const std::string& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_sender) return;
    auto bytes = encodePingFrame(data, false);
    m_sender(bytes);
}

void WebSocketConnection::pong(const std::string& data) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open || !m_sender) return;
    auto bytes = encodePongFrame(data, false);
    m_sender(bytes);
}

void WebSocketConnection::close(uint16_t code, const std::string& reason) {
    CloseHandler onCloseCopy;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_open) return;
        if (m_sender) {
            auto bytes = encodeCloseFrame(code, reason, false);
            m_sender(bytes);
        }
        m_open = false;
        onCloseCopy = m_onClose;
    }
    // Invoke the callback outside the lock: a handler may call back into
    // this connection (e.g. sendText), which would deadlock on m_mutex.
    if (onCloseCopy) {
        onCloseCopy(code, reason);
    }
}

void WebSocketConnection::handleIncomingBytes(const uint8_t* data, size_t length) {
    // Handlers are collected while locked and invoked after the lock is
    // released: a handler may call sendText()/close() on this connection,
    // which take the same mutex.
    std::vector<std::pair<TextHandler, std::string>> textCalls;
    std::vector<std::pair<BinaryHandler, std::vector<uint8_t>>> binaryCalls;
    std::vector<std::pair<CloseHandler, std::pair<uint16_t, std::string>>> closeCalls;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!data || length == 0) return;

        m_recvBuffer.insert(m_recvBuffer.end(), data, data + length);

        while (!m_recvBuffer.empty()) {
            WsFrame frame;
            size_t consumed = 0;
            if (!decodeFrame(m_recvBuffer.data(), m_recvBuffer.size(), frame, consumed)) {
                break; // Needs more bytes
            }

            m_recvBuffer.erase(m_recvBuffer.begin(), m_recvBuffer.begin() + consumed);

            if (frame.opcode == WsOpCode::Text) {
                m_receivedMessages.push_back(frame.payload);
                if (m_onText) textCalls.emplace_back(m_onText, frame.payload);
            } else if (frame.opcode == WsOpCode::Binary) {
                std::vector<uint8_t> bin(frame.payload.begin(), frame.payload.end());
                if (m_onBinary) binaryCalls.emplace_back(m_onBinary, std::move(bin));
            } else if (frame.opcode == WsOpCode::Ping) {
                // Automatic Pong echo
                if (m_sender) {
                    auto pongBytes = encodePongFrame(frame.payload, false);
                    m_sender(pongBytes);
                }
            } else if (frame.opcode == WsOpCode::Close) {
                m_open = false;
                if (m_onClose) closeCalls.emplace_back(m_onClose, std::make_pair(frame.closeCode, frame.closeReason));
            }
        }
    }

    for (auto& [handler, payload] : textCalls) handler(payload);
    for (auto& [handler, payload] : binaryCalls) handler(payload);
    for (auto& [handler, codeReason] : closeCalls) handler(codeReason.first, codeReason.second);
}

// ============================================================================
// WebSocket Hub Implementation
// ============================================================================

void WebSocketHub::addConnection(std::shared_ptr<WebSocketConnection> conn) {
    if (!conn) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections[conn->getId()] = conn;
    if (!conn->getRoom().empty()) {
        m_rooms[conn->getRoom()].insert(conn->getId());
    }
}

void WebSocketHub::removeConnection(uint64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_connections.find(id);
    if (it != m_connections.end()) {
        std::string room = it->second->getRoom();
        if (!room.empty()) {
            m_rooms[room].erase(id);
            if (m_rooms[room].empty()) {
                m_rooms.erase(room);
            }
        }
        m_connections.erase(it);
    }
}

std::shared_ptr<WebSocketConnection> WebSocketHub::getConnection(uint64_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_connections.find(id);
    return (it != m_connections.end()) ? it->second : nullptr;
}

void WebSocketHub::broadcast(const std::string& message) {
    std::vector<std::shared_ptr<WebSocketConnection>> targets;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        targets.reserve(m_connections.size());
        for (const auto& [_, conn] : m_connections) {
            if (conn->isOpen()) {
                targets.push_back(conn);
            }
        }
    }
    for (auto& conn : targets) {
        conn->sendText(message);
    }
}

void WebSocketHub::broadcastTo(const std::string& room, const std::string& message) {
    std::vector<std::shared_ptr<WebSocketConnection>> targets;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_rooms.find(room);
        if (it != m_rooms.end()) {
            for (uint64_t id : it->second) {
                auto cIt = m_connections.find(id);
                if (cIt != m_connections.end() && cIt->second->isOpen()) {
                    targets.push_back(cIt->second);
                }
            }
        }
    }
    for (auto& conn : targets) {
        conn->sendText(message);
    }
}

void WebSocketHub::joinRoom(uint64_t id, const std::string& room) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_connections.find(id);
    if (it != m_connections.end()) {
        std::string oldRoom = it->second->getRoom();
        if (!oldRoom.empty()) {
            m_rooms[oldRoom].erase(id);
        }
        it->second->setRoom(room);
        if (!room.empty()) {
            m_rooms[room].insert(id);
        }
    }
}

void WebSocketHub::leaveRoom(uint64_t id, const std::string& room) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_rooms.find(room);
    if (it != m_rooms.end()) {
        it->second.erase(id);
    }
    auto cIt = m_connections.find(id);
    if (cIt != m_connections.end() && cIt->second->getRoom() == room) {
        cIt->second->setRoom("");
    }
}

size_t WebSocketHub::size() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connections.size();
}

size_t WebSocketHub::roomSize(const std::string& room) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_rooms.find(room);
    return (it != m_rooms.end()) ? it->second.size() : 0;
}

void WebSocketHub::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_connections.clear();
    m_rooms.clear();
}

// ============================================================================
// WebSocket Server Transport Implementation
// ============================================================================

WebSocketServer& WebSocketServer::getGlobal() {
    static WebSocketServer instance;
    return instance;
}

void WebSocketServer::onText(const std::string& path, WsTextHandler handler) {
    if (path.empty() || !handler) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    m_textHandlers[path] = std::move(handler);
}

bool WebSocketServer::hasRoute(const std::string& path) {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_textHandlers.find(path) != m_textHandlers.end();
}

bool WebSocketServer::serveUpgraded(WsTransport& transport, const std::string& path,
                                    const std::string& clientKey, const std::string& leftover) {
    WsTextHandler handler;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_textHandlers.find(path);
        if (it == m_textHandlers.end()) return false;
        handler = it->second;
    }
    if (clientKey.empty() || !transport.isValid()) return false;

    uint64_t connId = m_nextConnId.fetch_add(1);
    auto conn = std::make_shared<WebSocketConnection>(
        connId, [&transport](const std::vector<uint8_t>& bytes) {
            if (transport.isValid() && !bytes.empty()) {
                transport.writeString(std::string(bytes.begin(), bytes.end()));
            }
        });
    conn->onText([handler, connId](const std::string& message) {
        handler(connId, message);
    });
    // Register before the handshake bytes go out, so the connection is
    // visible to sendTo()/connectionCount() as soon as the peer sees 101.
    m_hub.addConnection(conn);

    // RFC 6455 §4.2.2: 101 Switching Protocols with the accept key.
    std::string acceptKey = computeWebSocketAccept(clientKey);
    std::string handshake =
        "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: " + acceptKey + "\r\n"
        "\r\n";
    if (transport.writeString(handshake) <= 0) {
        m_hub.removeConnection(connId);
        return false;
    }

    // Bytes that arrived in the same TCP segment as the HTTP request.
    if (!leftover.empty()) {
        conn->handleIncomingBytes(reinterpret_cast<const uint8_t*>(leftover.data()), leftover.size());
    }

    // Frame loop on this (worker) thread until the peer closes or errors.
    while (transport.isValid() && conn->isOpen()) {
        std::string chunk = transport.readSome(16384);
        if (chunk.empty()) break; // Peer closed or read error.
        conn->handleIncomingBytes(reinterpret_cast<const uint8_t*>(chunk.data()), chunk.size());
    }

    m_hub.removeConnection(connId);
    return true;
}

bool WebSocketServer::sendTo(uint64_t connId, const std::string& message) {
    auto conn = m_hub.getConnection(connId);
    if (!conn || !conn->isOpen()) return false;
    conn->sendText(message);
    return true;
}

void WebSocketServer::broadcast(const std::string& message) {
    m_hub.broadcast(message);
}

void WebSocketServer::closeConnection(uint64_t connId, uint16_t code, const std::string& reason) {
    auto conn = m_hub.getConnection(connId);
    if (conn) conn->close(code, reason);
}

size_t WebSocketServer::connectionCount() const {
    return m_hub.size();
}

} // namespace sp::rt::ws
