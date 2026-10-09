#include "sp/runtime/Sse.h"
#include <sstream>

namespace sp::rt::sse {

std::string SseMessage::serialize() const {
    std::string out;
    if (!id.empty()) {
        out += "id: " + id + "\n";
    }
    if (!event.empty()) {
        out += "event: " + event + "\n";
    }
    if (retry > 0) {
        out += "retry: " + std::to_string(retry) + "\n";
    }

    // Handle multiline data per SSE specification
    if (data.empty()) {
        out += "data: \n";
    } else {
        std::istringstream stream(data);
        std::string line;
        while (std::getline(stream, line)) {
            // Strip trailing \r if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            out += "data: " + line + "\n";
        }
    }

    out += "\n"; // End of event
    return out;
}

std::string formatEvent(const std::string& event, const std::string& data, 
                        const std::string& id, int64_t retry) {
    SseMessage msg;
    msg.event = event;
    msg.data = data;
    msg.id = id;
    msg.retry = retry;
    return msg.serialize();
}

std::string formatData(const std::string& data) {
    return formatEvent("", data);
}

std::string formatComment(const std::string& comment) {
    return ": " + comment + "\n\n";
}

std::string formatHeartbeat() {
    return ": heartbeat\n\n";
}

SseStream::SseStream(ChunkSink sink)
    : m_sink(std::move(sink)) {}

void SseStream::sendEvent(const std::string& event, const std::string& data, 
                          const std::string& id, int64_t retry) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return;
    std::string chunk = formatEvent(event, data, id, retry);
    m_eventsCount++;
    m_emittedChunks.push_back(chunk);
    if (m_sink) {
        m_sink(chunk);
    }
}

void SseStream::sendData(const std::string& data) {
    sendEvent("", data);
}

void SseStream::sendComment(const std::string& comment) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_open) return;
    std::string chunk = formatComment(comment);
    m_emittedChunks.push_back(chunk);
    if (m_sink) {
        m_sink(chunk);
    }
}

void SseStream::sendHeartbeat() {
    sendComment("heartbeat");
}

void SseStream::close() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_open = false;
}

} // namespace sp::rt::sse
