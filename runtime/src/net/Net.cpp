#include "sp/runtime/Net.h"
#include <mutex>
#include <cstring>
#include <iostream>

#if defined(_MSC_VER)
#pragma comment(lib, "ws2_32.lib")
#endif

#include <atomic>

namespace sp::rt::net {

static std::atomic<bool> g_wsaInitialized{false};

void initNetwork() {
#if defined(_WIN32)
    if (!g_wsaInitialized.exchange(true)) {
        WSADATA wsaData;
        int res = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (res != 0) {
            std::cerr << "[sp::net] WSAStartup failed: " << res << std::endl;
        }
    }
#endif
}

void shutdownNetwork() {
#if defined(_WIN32)
    WSACleanup();
#endif
}

std::string resolveHost(const std::string& host) {
    initNetwork();
    struct addrinfo hints{};
    hints.ai_family = AF_INET; // IPv4 preferred
    hints.ai_socktype = SOCK_STREAM;

    struct addrinfo* res = nullptr;
    int err = getaddrinfo(host.c_str(), nullptr, &hints, &res);
    if (err != 0 || !res) {
        return host; // Return as-is if resolution fails
    }

    char ipStr[INET_ADDRSTRLEN] = {0};
    auto* ipv4 = reinterpret_cast<struct sockaddr_in*>(res->ai_addr);
    inet_ntop(AF_INET, &(ipv4->sin_addr), ipStr, INET_ADDRSTRLEN);

    freeaddrinfo(res);
    return std::string(ipStr);
}

// ----------------------------------------------------------------------------
// TcpSocket Implementation
// ----------------------------------------------------------------------------

TcpSocket::TcpSocket() : m_handle(SP_INVALID_SOCKET) {
    initNetwork();
}

TcpSocket::TcpSocket(sp_socket_handle_t sock) : m_handle(sock) {
    initNetwork();
}

TcpSocket::~TcpSocket() {
    close();
}

TcpSocket::TcpSocket(TcpSocket&& other) noexcept : m_handle(other.m_handle) {
    other.m_handle = SP_INVALID_SOCKET;
}

TcpSocket& TcpSocket::operator=(TcpSocket&& other) noexcept {
    if (this != &other) {
        close();
        m_handle = other.m_handle;
        other.m_handle = SP_INVALID_SOCKET;
    }
    return *this;
}

void TcpSocket::close() {
    if (m_handle != SP_INVALID_SOCKET) {
#if defined(_WIN32)
        closesocket(m_handle);
#else
        ::close(m_handle);
#endif
        m_handle = SP_INVALID_SOCKET;
    }
}

bool TcpSocket::setNonBlocking(bool nonBlocking) {
    if (m_handle == SP_INVALID_SOCKET) return false;
#if defined(_WIN32)
    u_long mode = nonBlocking ? 1 : 0;
    return ioctlsocket(m_handle, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(m_handle, F_GETFL, 0);
    if (flags < 0) return false;
    flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(m_handle, F_SETFL, flags) == 0;
#endif
}

bool TcpSocket::setTimeouts(int recvTimeoutMs, int sendTimeoutMs) {
    if (m_handle == SP_INVALID_SOCKET) return false;
#if defined(_WIN32)
    DWORD rTimeout = static_cast<DWORD>(recvTimeoutMs);
    DWORD sTimeout = static_cast<DWORD>(sendTimeoutMs);
    setsockopt(m_handle, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&rTimeout), sizeof(rTimeout));
    setsockopt(m_handle, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&sTimeout), sizeof(sTimeout));
#else
    struct timeval tvR;
    tvR.tv_sec = recvTimeoutMs / 1000;
    tvR.tv_usec = (recvTimeoutMs % 1000) * 1000;
    setsockopt(m_handle, SOL_SOCKET, SO_RCVTIMEO, &tvR, sizeof(tvR));

    struct timeval tvS;
    tvS.tv_sec = sendTimeoutMs / 1000;
    tvS.tv_usec = (sendTimeoutMs % 1000) * 1000;
    setsockopt(m_handle, SOL_SOCKET, SO_SNDTIMEO, &tvS, sizeof(tvS));
#endif
    return true;
}

bool TcpSocket::connect(const std::string& host, int port, int timeoutMs) {
    initNetwork();
    close();

    std::string ip = resolveHost(host);

    m_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_handle == SP_INVALID_SOCKET) {
        return false;
    }

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        close();
        return false;
    }

    if (timeoutMs <= 0) {
        int res = ::connect(m_handle, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
        if (res == SP_SOCKET_ERROR) {
            close();
            return false;
        }
        return true;
    }

    // Connect with non-blocking timeout
    setNonBlocking(true);
    int res = ::connect(m_handle, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    if (res == 0) {
        setNonBlocking(false);
        return true;
    }

    fd_set writeSet;
    FD_ZERO(&writeSet);
    FD_SET(m_handle, &writeSet);

    struct timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int selRes = select(static_cast<int>(m_handle + 1), nullptr, &writeSet, nullptr, &tv);
    if (selRes > 0 && FD_ISSET(m_handle, &writeSet)) {
        int so_error = 0;
        socklen_t len = sizeof(so_error);
        getsockopt(m_handle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&so_error), &len);
        if (so_error == 0) {
            setNonBlocking(false);
            return true;
        }
    }

    close();
    return false;
}

int64_t TcpSocket::write(const void* data, size_t length) {
    if (m_handle == SP_INVALID_SOCKET || !data || length == 0) return 0;
    int res = ::send(m_handle, reinterpret_cast<const char*>(data), static_cast<int>(length), 0);
    return res >= 0 ? static_cast<int64_t>(res) : -1;
}

int64_t TcpSocket::writeString(const std::string& str) {
    return write(str.data(), str.size());
}

int64_t TcpSocket::read(void* buffer, size_t maxLength) {
    if (m_handle == SP_INVALID_SOCKET || !buffer || maxLength == 0) return 0;
    int res = ::recv(m_handle, reinterpret_cast<char*>(buffer), static_cast<int>(maxLength), 0);
    return res >= 0 ? static_cast<int64_t>(res) : -1;
}

std::string TcpSocket::readString(size_t maxLength) {
    if (m_handle == SP_INVALID_SOCKET) return "";
    std::string buf;
    buf.resize(maxLength);
    int64_t bytes = read(&buf[0], maxLength);
    if (bytes <= 0) {
        return "";
    }
    buf.resize(static_cast<size_t>(bytes));
    return buf;
}

std::string TcpSocket::readAll(int timeoutMs) {
    if (m_handle == SP_INVALID_SOCKET) return "";
    setTimeouts(timeoutMs, timeoutMs);
    std::string result;
    char chunk[4096];
    while (true) {
        int64_t n = read(chunk, sizeof(chunk));
        if (n <= 0) break;
        result.append(chunk, static_cast<size_t>(n));
        if (n < static_cast<int64_t>(sizeof(chunk))) break;
    }
    return result;
}

// ----------------------------------------------------------------------------
// TcpListener Implementation
// ----------------------------------------------------------------------------

TcpListener::TcpListener() : m_handle(SP_INVALID_SOCKET) {
    initNetwork();
}

TcpListener::~TcpListener() {
    close();
}

TcpListener::TcpListener(TcpListener&& other) noexcept 
    : m_handle(other.m_handle), m_port(other.m_port) {
    other.m_handle = SP_INVALID_SOCKET;
    other.m_port = 0;
}

TcpListener& TcpListener::operator=(TcpListener&& other) noexcept {
    if (this != &other) {
        close();
        m_handle = other.m_handle;
        m_port = other.m_port;
        other.m_handle = SP_INVALID_SOCKET;
        other.m_port = 0;
    }
    return *this;
}

void TcpListener::close() {
    if (m_handle != SP_INVALID_SOCKET) {
#if defined(_WIN32)
        closesocket(m_handle);
#else
        ::close(m_handle);
#endif
        m_handle = SP_INVALID_SOCKET;
        m_port = 0;
    }
}

bool TcpListener::bindAndListen(const std::string& host, int port, int backlog) {
    initNetwork();
    close();

    m_handle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_handle == SP_INVALID_SOCKET) {
        return false;
    }

    int opt = 1;
#if defined(_WIN32)
    setsockopt(m_handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&opt), sizeof(opt));
#else
    setsockopt(m_handle, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

    std::string ip = host.empty() || host == "0.0.0.0" ? "0.0.0.0" : resolveHost(host);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        close();
        return false;
    }

    if (::bind(m_handle, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SP_SOCKET_ERROR) {
        close();
        return false;
    }

    if (::listen(m_handle, backlog) == SP_SOCKET_ERROR) {
        close();
        return false;
    }

    if (port == 0) {
        // Ephemeral port was chosen, resolve assigned port
        struct sockaddr_in sin{};
        socklen_t len = sizeof(sin);
        if (getsockname(m_handle, reinterpret_cast<struct sockaddr*>(&sin), &len) == 0) {
            m_port = ntohs(sin.sin_port);
        }
    } else {
        m_port = port;
    }

    return true;
}

std::unique_ptr<TcpSocket> TcpListener::accept() {
    if (m_handle == SP_INVALID_SOCKET) return nullptr;
    struct sockaddr_in clientAddr{};
    socklen_t clientLen = sizeof(clientAddr);
    sp_socket_handle_t clientSock = ::accept(m_handle, reinterpret_cast<struct sockaddr*>(&clientAddr), &clientLen);
    if (clientSock == SP_INVALID_SOCKET) {
        return nullptr;
    }
    return std::make_unique<TcpSocket>(clientSock);
}

std::unique_ptr<TcpSocket> TcpListener::acceptTimeout(int timeoutMs) {
    if (m_handle == SP_INVALID_SOCKET) return nullptr;

    // Snapshot the handle: another thread may close the listener (e.g.
    // HttpServer::stop()) while select() blocks. Reading m_handle again
    // after select() could see SP_INVALID_SOCKET (-1), and the fortified
    // FD_ISSET(-1) aborts the whole process via __fdelt_chk. The snapshot
    // keeps every fd_set operation on the fd select() actually waited on;
    // if the listener was closed meanwhile, accept() fails and we return
    // nullptr. (Phase 31: this race aborted every HttpServer test that
    // called stop() while the accept thread was mid-select.)
    sp_socket_handle_t fd = m_handle;
    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(fd, &readSet);

    struct timeval tv{};
    tv.tv_sec = timeoutMs / 1000;
    tv.tv_usec = (timeoutMs % 1000) * 1000;

    int selRes = select(static_cast<int>(fd + 1), &readSet, nullptr, nullptr, &tv);
    if (selRes > 0 && FD_ISSET(fd, &readSet)) {
        return accept();
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// UdpSocket Implementation
// ----------------------------------------------------------------------------

UdpSocket::UdpSocket() : m_handle(SP_INVALID_SOCKET) {
    initNetwork();
}

UdpSocket::~UdpSocket() {
    close();
}

UdpSocket::UdpSocket(UdpSocket&& other) noexcept : m_handle(other.m_handle), m_port(other.m_port) {
    other.m_handle = SP_INVALID_SOCKET;
    other.m_port = 0;
}

UdpSocket& UdpSocket::operator=(UdpSocket&& other) noexcept {
    if (this != &other) {
        close();
        m_handle = other.m_handle;
        m_port = other.m_port;
        other.m_handle = SP_INVALID_SOCKET;
        other.m_port = 0;
    }
    return *this;
}

void UdpSocket::close() {
    if (m_handle != SP_INVALID_SOCKET) {
#if defined(_WIN32)
        closesocket(m_handle);
#else
        ::close(m_handle);
#endif
        m_handle = SP_INVALID_SOCKET;
        m_port = 0;
    }
}

bool UdpSocket::setNonBlocking(bool nonBlocking) {
    if (m_handle == SP_INVALID_SOCKET) return false;
#if defined(_WIN32)
    u_long mode = nonBlocking ? 1 : 0;
    return ioctlsocket(m_handle, FIONBIO, &mode) == 0;
#else
    int flags = fcntl(m_handle, F_GETFL, 0);
    if (flags < 0) return false;
    flags = nonBlocking ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK);
    return fcntl(m_handle, F_SETFL, flags) == 0;
#endif
}

bool UdpSocket::bind(const std::string& host, int port) {
    initNetwork();
    close();

    m_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_handle == SP_INVALID_SOCKET) return false;

    std::string ip = host.empty() || host == "0.0.0.0" ? "0.0.0.0" : resolveHost(host);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        close();
        return false;
    }

    if (::bind(m_handle, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == SP_SOCKET_ERROR) {
        close();
        return false;
    }

    if (port == 0) {
        struct sockaddr_in sin{};
        socklen_t len = sizeof(sin);
        if (getsockname(m_handle, reinterpret_cast<struct sockaddr*>(&sin), &len) == 0) {
            m_port = ntohs(sin.sin_port);
        }
    } else {
        m_port = port;
    }
    return true;
}

int64_t UdpSocket::sendTo(const void* data, size_t len, const std::string& host, int port) {
    if (m_handle == SP_INVALID_SOCKET || !data || len == 0) return 0;
    std::string ip = resolveHost(host);

    struct sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        return -1;
    }

    int res = ::sendto(m_handle, reinterpret_cast<const char*>(data), static_cast<int>(len), 0,
                       reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr));
    return res >= 0 ? static_cast<int64_t>(res) : -1;
}

int64_t UdpSocket::sendString(const std::string& str, const std::string& host, int port) {
    return sendTo(str.data(), str.size(), host, port);
}

int64_t UdpSocket::recvFrom(void* buffer, size_t maxLen, std::string& outSenderHost, int& outSenderPort) {
    if (m_handle == SP_INVALID_SOCKET || !buffer || maxLen == 0) return 0;

    struct sockaddr_in senderAddr{};
    socklen_t senderLen = sizeof(senderAddr);

    int res = ::recvfrom(m_handle, reinterpret_cast<char*>(buffer), static_cast<int>(maxLen), 0,
                         reinterpret_cast<struct sockaddr*>(&senderAddr), &senderLen);
    if (res < 0) return -1;

    char ipStr[INET_ADDRSTRLEN] = {0};
    inet_ntop(AF_INET, &(senderAddr.sin_addr), ipStr, INET_ADDRSTRLEN);
    outSenderHost = ipStr;
    outSenderPort = ntohs(senderAddr.sin_port);

    return static_cast<int64_t>(res);
}

std::string UdpSocket::recvString(size_t maxLen, std::string& outSenderHost, int& outSenderPort) {
    if (m_handle == SP_INVALID_SOCKET) return "";
    std::string buf;
    buf.resize(maxLen);
    int64_t bytes = recvFrom(&buf[0], maxLen, outSenderHost, outSenderPort);
    if (bytes <= 0) return "";
    buf.resize(static_cast<size_t>(bytes));
    return buf;
}

} // namespace sp::rt::net
