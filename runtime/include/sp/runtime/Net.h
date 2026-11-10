#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET sp_socket_handle_t;
#define SP_INVALID_SOCKET INVALID_SOCKET
#define SP_SOCKET_ERROR SOCKET_ERROR
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
typedef int sp_socket_handle_t;
#define SP_INVALID_SOCKET (-1)
#define SP_SOCKET_ERROR (-1)
#endif

namespace sp::rt::net {

// Socket initialization & cleanup
void initNetwork();
void shutdownNetwork();

// DNS resolution
std::string resolveHost(const std::string& host);

// TCP Socket wrapper
class TcpSocket {
public:
    TcpSocket();
    explicit TcpSocket(sp_socket_handle_t sock);
    ~TcpSocket();

    TcpSocket(const TcpSocket&) = delete;
    TcpSocket& operator=(const TcpSocket&) = delete;
    TcpSocket(TcpSocket&& other) noexcept;
    TcpSocket& operator=(TcpSocket&& other) noexcept;

    bool isValid() const { return m_handle != SP_INVALID_SOCKET; }
    sp_socket_handle_t getHandle() const { return m_handle; }

    bool connect(const std::string& host, int port, int timeoutMs = 5000);
    int64_t write(const void* data, size_t length);
    int64_t writeString(const std::string& str);
    int64_t read(void* buffer, size_t maxLength);
    std::string readString(size_t maxLength = 65536);
    std::string readAll(int timeoutMs = 2000);

    bool setNonBlocking(bool nonBlocking);
    bool setTimeouts(int recvTimeoutMs, int sendTimeoutMs);
    void close();

private:
    sp_socket_handle_t m_handle{SP_INVALID_SOCKET};
};

// TCP Listener wrapper
class TcpListener {
public:
    TcpListener();
    ~TcpListener();

    TcpListener(const TcpListener&) = delete;
    TcpListener& operator=(const TcpListener&) = delete;
    TcpListener(TcpListener&& other) noexcept;
    TcpListener& operator=(TcpListener&& other) noexcept;

    bool isValid() const { return m_handle != SP_INVALID_SOCKET; }
    sp_socket_handle_t getHandle() const { return m_handle; }

    bool bindAndListen(const std::string& host, int port, int backlog = 128);
    std::unique_ptr<TcpSocket> accept();
    std::unique_ptr<TcpSocket> acceptTimeout(int timeoutMs);
    void close();

    int getPort() const { return m_port; }

private:
    sp_socket_handle_t m_handle{SP_INVALID_SOCKET};
    int m_port{0};
};

// UDP Socket wrapper
class UdpSocket {
public:
    UdpSocket();
    ~UdpSocket();

    UdpSocket(const UdpSocket&) = delete;
    UdpSocket& operator=(const UdpSocket&) = delete;
    UdpSocket(UdpSocket&& other) noexcept;
    UdpSocket& operator=(UdpSocket&& other) noexcept;

    bool isValid() const { return m_handle != SP_INVALID_SOCKET; }
    sp_socket_handle_t getHandle() const { return m_handle; }

    bool bind(const std::string& host, int port);
    int64_t sendTo(const void* data, size_t len, const std::string& host, int port);
    int64_t sendString(const std::string& str, const std::string& host, int port);
    int64_t recvFrom(void* buffer, size_t maxLen, std::string& outSenderHost, int& outSenderPort);
    std::string recvString(size_t maxLen, std::string& outSenderHost, int& outSenderPort);

    bool setNonBlocking(bool nonBlocking);
    // Phase 4 slice D: mirror of TcpSocket::setTimeouts. UdpSocket::recvFrom
    // used to block forever with no timeout: when a datagram is lost (or the
    // send fails), the caller hung until the test harness killed it.
    // Bound the receive with this before any recv that must not hang.
    bool setTimeouts(int recvTimeoutMs, int sendTimeoutMs);
    void close();

private:
    sp_socket_handle_t m_handle{SP_INVALID_SOCKET};
    int m_port{0};
};

} // namespace sp::rt::net
