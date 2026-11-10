#include <gtest/gtest.h>
#include "sp/runtime/Net.h"
#include "sp/runtime/EventLoop.h"
#include <thread>
#include <chrono>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/socket.h>
#endif

using namespace sp::rt::net;

TEST(NetTest, DNSResolution) {
    std::string ip = resolveHost("127.0.0.1");
    EXPECT_EQ(ip, "127.0.0.1");

    std::string localIp = resolveHost("localhost");
    EXPECT_FALSE(localIp.empty());
}

TEST(NetTest, TcpSocketListenConnectEcho) {
    TcpListener listener;
    ASSERT_TRUE(listener.bindAndListen("127.0.0.1", 0));
    int port = listener.getPort();
    ASSERT_GT(port, 0);

    std::thread serverThread([&listener]() {
        auto client = listener.acceptTimeout(2000);
        ASSERT_NE(client, nullptr);
        ASSERT_TRUE(client->isValid());

        std::string req = client->readString(1024);
        EXPECT_EQ(req, "Ping SP TCP");

        int64_t written = client->writeString("Pong SP TCP");
        EXPECT_EQ(written, 11);
        client->close();
    });

    TcpSocket clientSock;
    ASSERT_TRUE(clientSock.connect("127.0.0.1", port, 2000));
    ASSERT_TRUE(clientSock.isValid());

    int64_t w = clientSock.writeString("Ping SP TCP");
    EXPECT_EQ(w, 11);

    std::string resp = clientSock.readString(1024);
    EXPECT_EQ(resp, "Pong SP TCP");

    clientSock.close();
    serverThread.join();
    listener.close();
}

TEST(NetTest, UdpSocketSendRecv) {
    UdpSocket server;
    ASSERT_TRUE(server.bind("127.0.0.1", 0));
    
    // Find assigned port
    struct sockaddr_in sin{};
    socklen_t len = sizeof(sin);
    getsockname(server.getHandle(), reinterpret_cast<struct sockaddr*>(&sin), &len);
    int port = ntohs(sin.sin_port);
    ASSERT_GT(port, 0);

    UdpSocket client;
    ASSERT_TRUE(client.bind("127.0.0.1", 0));

    // Phase 4 slice D: ASSERT (not EXPECT) on the send. If sendto fails
    // (e.g. this VM's sandbox returns EPERM for UDP sendto), no datagram
    // will ever arrive and the recv below would block forever; failing fast
    // here keeps the failure visible instead of a hang. The receive is
    // additionally bounded to 5 s via setTimeouts so a lost datagram fails
    // the test instead of hanging the suite.
    int64_t sent = client.sendString("UDP SP Message", "127.0.0.1", port);
    ASSERT_EQ(sent, 14);

    ASSERT_TRUE(server.setTimeouts(5000, 5000));
    std::string senderHost;
    int senderPort = 0;
    std::string received = server.recvString(512, senderHost, senderPort);
    EXPECT_EQ(received, "UDP SP Message");
    EXPECT_FALSE(senderHost.empty());
    EXPECT_GT(senderPort, 0);

    client.close();
    server.close();
}

TEST(NetTest, EventLoopPolling) {
    TcpListener listener;
    ASSERT_TRUE(listener.bindAndListen("127.0.0.1", 0));
    int port = listener.getPort();

    TcpSocket client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));

    auto serverConn = listener.accept();
    ASSERT_NE(serverConn, nullptr);

    EventLoop loop;
    EXPECT_TRUE(loop.addSocket(serverConn->getHandle(), SP_EVENT_READ));

    // Initially nothing to read (0 ready events with 50ms timeout)
    int64_t ready = loop.poll(50);
    EXPECT_EQ(ready, 0);

    // Client writes data
    client.writeString("Data for EventLoop");

    // Poll should now report readable event
    ready = loop.poll(1000);
    EXPECT_GE(ready, 1);
    EXPECT_GE(loop.getReadyCount(), 1u);
    EXPECT_TRUE(loop.getReadyEvent(0).events & SP_EVENT_READ);

    std::string readData = serverConn->readString(256);
    EXPECT_EQ(readData, "Data for EventLoop");

    loop.removeSocket(serverConn->getHandle());
    client.close();
    serverConn->close();
    listener.close();
}

// Phase 40 slice C (pro-4.0.0): the epoll backend must be live on Linux
// (a fresh loop in a test process has plenty of fds, so epoll_create1
// succeeds), and the poll() fallback elsewhere.
TEST(NetTest, EventLoopUsesEpollOnLinux) {
    EventLoop loop;
#if defined(__linux__)
    EXPECT_TRUE(loop.usesEpoll());
#else
    EXPECT_FALSE(loop.usesEpoll());
#endif
}

// Phase 40 slice C: an empty loop never reports anything.
TEST(NetTest, EventLoopEmptyPoll) {
    EventLoop loop;
    EXPECT_EQ(loop.poll(0), 0);
    EXPECT_EQ(loop.poll(10), 0);
    EXPECT_EQ(loop.getReadyCount(), 0u);
    EXPECT_FALSE(loop.addSocket(SP_INVALID_SOCKET, SP_EVENT_READ));
}

// Phase 40 slice C: many idle sockets, none ready, then exactly one
// becomes ready. This is the functional half of the epoll scale proof
// (the timing half is the probe in ~/workspace/p11-probes/).
TEST(NetTest, EventLoopManyIdleSockets) {
    constexpr int kPairs = 500;
    TcpListener listener;
    ASSERT_TRUE(listener.bindAndListen("127.0.0.1", 0));
    int port = listener.getPort();

    std::vector<TcpSocket> clients;
    std::vector<std::unique_ptr<TcpSocket>> servers;
    for (int i = 0; i < kPairs; ++i) {
        TcpSocket client;
        ASSERT_TRUE(client.connect("127.0.0.1", port));
        auto serverConn = listener.accept();
        ASSERT_NE(serverConn, nullptr);
        clients.push_back(std::move(client));
        servers.push_back(std::move(serverConn));
    }

    EventLoop loop;
    for (auto& serverConn : servers) {
        ASSERT_TRUE(loop.addSocket(serverConn->getHandle(), SP_EVENT_READ));
    }

    // Nothing to read anywhere.
    EXPECT_EQ(loop.poll(0), 0);
    EXPECT_EQ(loop.getReadyCount(), 0u);

    // Wake exactly one connection.
    clients[123].writeString("wake");
    int64_t ready = loop.poll(1000);
    EXPECT_EQ(ready, 1);
    EXPECT_EQ(loop.getReadyCount(), 1u);
    EXPECT_EQ(loop.getReadyEvent(0).socketFd, servers[123]->getHandle());
    EXPECT_TRUE(loop.getReadyEvent(0).events & SP_EVENT_READ);

    for (auto& serverConn : servers) {
        loop.removeSocket(serverConn->getHandle());
        serverConn->close();
    }
    for (auto& client : clients) client.close();
    listener.close();
}

// Phase 40 slice C: modify changes the watched interest (a re-add
// overwrites it the same way), remove stops reporting, and both report
// false for a socket that was never added.
TEST(NetTest, EventLoopModifyAndRemove) {
    TcpListener listener;
    ASSERT_TRUE(listener.bindAndListen("127.0.0.1", 0));
    int port = listener.getPort();

    TcpSocket client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    auto serverConn = listener.accept();
    ASSERT_NE(serverConn, nullptr);

    EventLoop loop;
    // Unknown fd: both report false.
    EXPECT_FALSE(loop.modifySocket(serverConn->getHandle(), SP_EVENT_READ));
    EXPECT_FALSE(loop.removeSocket(serverConn->getHandle()));

    ASSERT_TRUE(loop.addSocket(serverConn->getHandle(), SP_EVENT_READ));
    EXPECT_EQ(loop.poll(0), 0); // nothing to read

    // Re-add overwrites the interest: WRITE is ready at once (the send
    // buffer is empty), which proves the MOD path took effect.
    ASSERT_TRUE(loop.addSocket(serverConn->getHandle(), SP_EVENT_WRITE));
    EXPECT_EQ(loop.poll(0), 1);
    EXPECT_TRUE(loop.getReadyEvent(0).events & SP_EVENT_WRITE);

    // Back to READ: nothing to read again.
    ASSERT_TRUE(loop.modifySocket(serverConn->getHandle(), SP_EVENT_READ));
    EXPECT_EQ(loop.poll(0), 0);

    // Data arrives, then remove: the ready data is no longer reported.
    client.writeString("hello");
    EXPECT_EQ(loop.poll(1000), 1);
    EXPECT_TRUE(loop.removeSocket(serverConn->getHandle()));
    EXPECT_EQ(loop.poll(0), 0);
    EXPECT_FALSE(loop.removeSocket(serverConn->getHandle()));

    client.close();
    serverConn->close();
    listener.close();
}

// Phase 40 slice C: a peer that resets the connection reports ERROR on
// the watched socket (READ arrives with it, since the reset data is still
// readable). A graceful FIN close does NOT set ERROR — the kernel reports
// it as READ (EOF) only, on the poll() backend too — so the test forces
// an RST with SO_LINGER 0 instead of a plain close.
#if defined(__unix__) || defined(__APPLE__)
TEST(NetTest, EventLoopErrorEvent) {
    TcpListener listener;
    ASSERT_TRUE(listener.bindAndListen("127.0.0.1", 0));
    int port = listener.getPort();

    TcpSocket client;
    ASSERT_TRUE(client.connect("127.0.0.1", port));
    auto serverConn = listener.accept();
    ASSERT_NE(serverConn, nullptr);

    EventLoop loop;
    ASSERT_TRUE(loop.addSocket(serverConn->getHandle(), SP_EVENT_READ));
    EXPECT_EQ(loop.poll(0), 0);

    // Hard close: RST instead of FIN.
    struct linger lg{};
    lg.l_onoff = 1;
    lg.l_linger = 0;
    ASSERT_EQ(::setsockopt(client.getHandle(), SOL_SOCKET, SO_LINGER, &lg,
                           sizeof(lg)),
              0);
    client.close();

    int64_t ready = loop.poll(1000);
    EXPECT_GE(ready, 1);
    EXPECT_GE(loop.getReadyCount(), 1u);
    EXPECT_TRUE(loop.getReadyEvent(0).events & SP_EVENT_ERROR);

    loop.removeSocket(serverConn->getHandle());
    serverConn->close();
    listener.close();
}
#endif
