#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <functional>
#include <mutex>

namespace sp::rt::sse {

// ============================================================================
// Server-Sent Events (W3C / WHATWG HTML5 SSE)
// ============================================================================

struct SseMessage {
    std::string event;
    std::string data;
    std::string id;
    int64_t retry{0}; // milliseconds

    std::string serialize() const;
};

std::string formatEvent(const std::string& event, const std::string& data, 
                        const std::string& id = "", int64_t retry = 0);
std::string formatData(const std::string& data);
std::string formatComment(const std::string& comment);
std::string formatHeartbeat();

class SseStream {
public:
    using ChunkSink = std::function<void(const std::string&)>;

    explicit SseStream(ChunkSink sink = nullptr);
    ~SseStream() = default;

    void sendEvent(const std::string& event, const std::string& data, 
                   const std::string& id = "", int64_t retry = 0);
    void sendData(const std::string& data);
    void sendComment(const std::string& comment);
    void sendHeartbeat();
    void close();

    bool isOpen() const { return m_open; }
    size_t getEventsCount() const { return m_eventsCount; }
    const std::vector<std::string>& getEmittedChunks() const { return m_emittedChunks; }

private:
    ChunkSink m_sink;
    bool m_open{true};
    size_t m_eventsCount{0};
    std::vector<std::string> m_emittedChunks;
    mutable std::mutex m_mutex;
};

} // namespace sp::rt::sse
