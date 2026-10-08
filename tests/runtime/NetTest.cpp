#include <gtest/gtest.h>
#include "sp/runtime/Net.h"
#include "sp/runtime/EventLoop.h"
#include <thread>
#include <chrono>

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

    int64_t sent = client.sendString("UDP SP Message", "127.0.0.1", port);
    EXPECT_EQ(sent, 14);

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
