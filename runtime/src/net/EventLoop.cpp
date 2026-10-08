#include "sp/runtime/EventLoop.h"
#include <algorithm>

namespace sp::rt::net {

EventLoop::EventLoop() {
    initNetwork();
}

EventLoop::~EventLoop() {
    m_watched.clear();
    m_readyEvents.clear();
}

bool EventLoop::addSocket(sp_socket_handle_t sock, int64_t events, void* userData) {
    if (sock == SP_INVALID_SOCKET) return false;
    m_watched[sock] = WatchedSocket{sock, events, userData};
    return true;
}

bool EventLoop::modifySocket(sp_socket_handle_t sock, int64_t events) {
    auto it = m_watched.find(sock);
    if (it == m_watched.end()) return false;
    it->second.events = events;
    return true;
}

bool EventLoop::removeSocket(sp_socket_handle_t sock) {
    return m_watched.erase(sock) > 0;
}

int64_t EventLoop::poll(int timeoutMs) {
    m_readyEvents.clear();
    if (m_watched.empty()) return 0;

    std::vector<struct pollfd> pollfds;
    pollfds.reserve(m_watched.size());

    for (const auto& [sock, info] : m_watched) {
        struct pollfd pfd{};
        pfd.fd = sock;
        pfd.events = 0;
        if (info.events & SP_EVENT_READ) {
            pfd.events |= POLLIN;
        }
        if (info.events & SP_EVENT_WRITE) {
            pfd.events |= POLLOUT;
        }
        pfd.revents = 0;
        pollfds.push_back(pfd);
    }

#if defined(_WIN32)
    int ret = WSAPoll(pollfds.data(), static_cast<ULONG>(pollfds.size()), timeoutMs);
#else
    int ret = ::poll(pollfds.data(), static_cast<nfds_t>(pollfds.size()), timeoutMs);
#endif

    if (ret <= 0) {
        return ret; // 0 for timeout, <0 for error
    }

    for (const auto& pfd : pollfds) {
        if (pfd.revents != 0) {
            auto it = m_watched.find(pfd.fd);
            if (it != m_watched.end()) {
                int64_t revents = 0;
                if (pfd.revents & (POLLIN | POLLPRI)) {
                    revents |= SP_EVENT_READ;
                }
                if (pfd.revents & POLLOUT) {
                    revents |= SP_EVENT_WRITE;
                }
                if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
                    revents |= SP_EVENT_ERROR;
                }

                m_readyEvents.push_back(ReadyEvent{
                    it->first,
                    revents,
                    it->second.userData
                });
            }
        }
    }

    return static_cast<int64_t>(m_readyEvents.size());
}

} // namespace sp::rt::net
