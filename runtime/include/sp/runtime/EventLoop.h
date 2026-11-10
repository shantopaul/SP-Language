#pragma once

#include "sp/runtime/Net.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

#if defined(__linux__)
#include <sys/epoll.h>
#endif

namespace sp::rt::net {

// Phase 40 slice A (pro-4.0.0): epoll event loop (ADR-003).
//
// The wait backend is epoll on Linux and poll()/WSAPoll elsewhere. The
// interface is identical on every platform, so the Scheduler, the SP
// EventLoop class and all tests are backend-agnostic. Level-triggered
// readiness is used on purpose: it has exactly the poll() semantics (a
// ready fd keeps reporting until drained), so no caller changes how it
// drains a socket. The win is per-wait cost: epoll_wait returns only the
// ready fds in O(ready), while poll() rebuilds and scans the whole fd set
// in O(registered) on every call — measured 0.417 ms/call vs 0.0005
// ms/call at 10,000 idle fds (see docs/v2/PHASE-40-PRO-4.0.0.md §0.2).
//
// If epoll_create1 fails (m_epollFd stays -1), the Linux build falls back
// to the poll() path transparently, so a restricted environment degrades
// to the old behaviour instead of failing to construct.
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

    // True when this loop waits with epoll (Linux with a live epoll
    // instance). False on the poll()/WSAPoll path — including on Linux
    // when epoll_create1 failed (e.g. the process is at its fd limit)
    // and the loop transparently fell back to poll().
    bool usesEpoll() const {
#if defined(__linux__)
        return m_epollFd >= 0;
#else
        return false;
#endif
    }

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

#if defined(__linux__)
    // epoll instance for this loop (-1 = creation failed: use the poll()
    // path instead). m_epollEvents is the reusable epoll_wait output
    // buffer, grown only when the watched set grows, so poll() allocates
    // nothing per call.
    int m_epollFd{-1};
    std::vector<struct epoll_event> m_epollEvents;
#endif
};

} // namespace sp::rt::net
