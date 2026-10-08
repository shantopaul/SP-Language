#pragma once

#include "sp/runtime/Net.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace sp::rt::net {

constexpr int64_t SP_EVENT_READ  = 1;
constexpr int64_t SP_EVENT_WRITE = 2;
constexpr int64_t SP_EVENT_ERROR = 4;

struct ReadyEvent {
    sp_socket_handle_t socketFd{SP_INVALID_SOCKET};
    int64_t events{0};
    void* userData{nullptr};
};

class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    EventLoop(const EventLoop&) = delete;
    EventLoop& operator=(const EventLoop&) = delete;

    bool addSocket(sp_socket_handle_t sock, int64_t events, void* userData = nullptr);
    bool modifySocket(sp_socket_handle_t sock, int64_t events);
    bool removeSocket(sp_socket_handle_t sock);

    int64_t poll(int timeoutMs);

    size_t getReadyCount() const { return m_readyEvents.size(); }
    const ReadyEvent& getReadyEvent(size_t index) const { return m_readyEvents[index]; }
    const std::vector<ReadyEvent>& getReadyEvents() const { return m_readyEvents; }

private:
    struct WatchedSocket {
        sp_socket_handle_t sock;
        int64_t events;
        void* userData;
    };

    std::unordered_map<sp_socket_handle_t, WatchedSocket> m_watched;
    std::vector<ReadyEvent> m_readyEvents;
};

} // namespace sp::rt::net
