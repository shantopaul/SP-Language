// WebSocket-over-TLS (wss://) end-to-end tests (Phase 4 slice D).
//
// Phase 2 built the TLS stack (HttpServer TLS + WsTransport abstraction)
// but never proved a WebSocket handshake + frame round-trip over TLS:
// there was no WebSocketClient in the runtime and no wss test. This file
// closes that gap with a minimal in-test WS client over raw OpenSSL
// (same pattern as TlsTest: certificates generated with the `openssl` CLI
// at setup, unique temp dir per test, explicit SKIP if the CLI is missing).
// The server side is the real stack: TLS HttpServer + the global
// WebSocketServer echo route; the client verifies the server cert against
// the test CA (no skip-verify anywhere in the positive test).
#if !defined(_WIN32)

#include <gtest/gtest.h>
#include "sp/runtime/Http.h"
#include "sp/runtime/WebSocket.h"

#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/x509v3.h>

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <chrono>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>

using namespace sp::rt::http;
namespace ws = sp::rt::ws;

class WssTest : public ::testing::Test {
protected:
    std::filesystem::path dir;

    void SetUp() override {
        if (std::system("openssl version >/dev/null 2>&1") != 0) {
            GTEST_SKIP() << "openssl CLI not available; wss tests need it to generate test certificates";
        }
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir = std::filesystem::temp_directory_path() /
              ("sp_wss_" + std::to_string(::getpid()) + "_" + info->name());
        std::filesystem::create_directories(dir);
        ASSERT_TRUE(genCa()) << "failed to generate test CA";
        ASSERT_TRUE(genServerCert()) << "failed to generate CA-signed server cert";
    }

    void TearDown() override {
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);
    }

    bool run(const std::string& cmd) { return std::system(cmd.c_str()) == 0; }
    std::string q(const std::filesystem::path& p) { return "\"" + p.string() + "\""; }

    bool genCa() {
        return run("openssl req -x509 -newkey rsa:2048 -keyout " + q(dir / "ca.key") +
                   " -out " + q(dir / "ca.crt") + " -days 2 -nodes -subj \"/CN=SP-Test-CA\" 2>/dev/null");
    }

    bool genServerCert() {
        return run("openssl req -newkey rsa:2048 -keyout " + q(dir / "server.key") +
                   " -out " + q(dir / "server.csr") +
                   " -nodes -subj \"/CN=localhost\"" +
                   " -addext \"subjectAltName=IP:127.0.0.1,DNS:localhost\" 2>/dev/null") &&
               run("openssl x509 -req -in " + q(dir / "server.csr") +
                   " -CA " + q(dir / "ca.crt") + " -CAkey " + q(dir / "ca.key") +
                   " -CAcreateserial -out " + q(dir / "server.crt") +
                   " -days 2 -copy_extensions copy 2>/dev/null");
    }

    HttpServerConfig tlsConfig() {
        HttpServerConfig cfg;
        cfg.tlsEnabled = true;
        cfg.tlsCertFile = (dir / "server.crt").string();
        cfg.tlsKeyFile = (dir / "server.key").string();
        return cfg;
    }

    // TLS server with a /wss-echo WebSocket route; returns its port.
    int startEchoServer(HttpServer& server) {
        ws::WebSocketServer::getGlobal().onText(
            "/wss-echo", [](uint64_t connId, const std::string& message) {
                ws::WebSocketServer::getGlobal().sendTo(connId, message);
            });
        if (!server.listen("127.0.0.1", 0)) {
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        return server.getPort();
    }

    // Minimal raw-socket + OpenSSL client. `trustCa` loads the test CA and
    // verifies the server cert (with the IP SAN checked); without it the
    // handshake must fail. Returns the connected SSL* (caller frees it and
    // closes the fd), or nullptr on failure.
    SSL* tlsConnect(int port, int fd, bool trustCa) {
        SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
        if (!ctx) return nullptr;
        if (trustCa) {
            if (SSL_CTX_load_verify_locations(ctx, (dir / "ca.crt").string().c_str(), nullptr) != 1) {
                SSL_CTX_free(ctx);
                return nullptr;
            }
        }
        SSL* ssl = SSL_new(ctx);
        SSL_CTX_free(ctx); // SSL holds its own reference.
        if (!ssl) return nullptr;
        // Verification is ON in both cases: without the test CA loaded the
        // handshake must fail (the negative test below depends on this --
        // a client default of SSL_VERIFY_NONE would accept anything).
        SSL_set_verify(ssl, SSL_VERIFY_PEER, nullptr);
        if (trustCa) {
            X509_VERIFY_PARAM* param = SSL_get0_param(ssl);
            X509_VERIFY_PARAM_set1_ip_asc(param, "127.0.0.1");
        }
        SSL_set_fd(ssl, fd);
        if (SSL_connect(ssl) != 1) {
            SSL_free(ssl);
            return nullptr;
        }
        return ssl;
    }

    int tcpConnect(int port) {
        int fd = ::socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) return -1;
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        ::setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        ::setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
        struct sockaddr_in addr {};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(static_cast<uint16_t>(port));
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        if (::connect(fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) != 0) {
            ::close(fd);
            return -1;
        }
        return fd;
    }

    // Read from SSL until `marker` appears; appends everything (including
    // any bytes past the marker) to `out`. False on timeout/error.
    bool sslReadUntil(SSL* ssl, const std::string& marker, std::string& out) {
        char buf[4096];
        for (int i = 0; i < 50; ++i) {
            if (out.find(marker) != std::string::npos) return true;
            int n = SSL_read(ssl, buf, sizeof(buf));
            if (n <= 0) return false;
            out.append(buf, static_cast<size_t>(n));
        }
        return out.find(marker) != std::string::npos;
    }

    // Read SSL bytes until decodeFrame yields a complete text frame.
    bool sslReadTextFrame(SSL* ssl, std::string& leftover, std::string& payloadOut) {
        std::vector<uint8_t> acc(leftover.begin(), leftover.end());
        for (int i = 0; i < 50; ++i) {
            ws::WsFrame frame;
            size_t consumed = 0;
            if (ws::decodeFrame(acc.data(), acc.size(), frame, consumed) &&
                frame.opcode == ws::WsOpCode::Text) {
                payloadOut = frame.payload;
                return true;
            }
            char buf[4096];
            int n = SSL_read(ssl, buf, sizeof(buf));
            if (n <= 0) return false;
            acc.insert(acc.end(), buf, buf + n);
        }
        return false;
    }
};

// (a) wss:// end-to-end: TLS handshake with a CA-trusted cert, RFC 6455
// upgrade (101 + correct Sec-WebSocket-Accept), masked client text frame,
// server echo over the same TLS connection.
TEST_F(WssTest, WssEchoRoundTrip) {
    HttpServer server(tlsConfig());
    int port = startEchoServer(server);
    ASSERT_GT(port, 0) << "TLS server failed to start: " << server.getLastError();

    int fd = tcpConnect(port);
    ASSERT_GE(fd, 0);
    SSL* ssl = tlsConnect(port, fd, /*trustCa=*/true);
    ASSERT_NE(ssl, nullptr) << "TLS handshake with trusted CA must succeed";

    const std::string key = "dGhlIHNhbXBsZSBub25jZQ==";
    std::string req = "GET /wss-echo HTTP/1.1\r\n"
                      "Host: 127.0.0.1:" + std::to_string(port) + "\r\n"
                      "Upgrade: websocket\r\n"
                      "Connection: Upgrade\r\n"
                      "Sec-WebSocket-Key: " + key + "\r\n"
                      "Sec-WebSocket-Version: 13\r\n\r\n";
    ASSERT_GT(SSL_write(ssl, req.data(), static_cast<int>(req.size())), 0);

    std::string raw;
    ASSERT_TRUE(sslReadUntil(ssl, "\r\n\r\n", raw)) << "no HTTP upgrade response";
    size_t hdrEnd = raw.find("\r\n\r\n");
    ASSERT_NE(hdrEnd, std::string::npos);
    std::string headers = raw.substr(0, hdrEnd);
    EXPECT_NE(headers.find("101"), std::string::npos) << "expected 101 Switching Protocols: " << headers;
    std::string expectedAccept = ws::computeWebSocketAccept(key);
    EXPECT_NE(headers.find(expectedAccept), std::string::npos)
        << "Sec-WebSocket-Accept mismatch: " << headers;
    std::string leftover = raw.substr(hdrEnd + 4);

    // Clients MUST mask: encodeTextFrame(masked=true).
    std::vector<uint8_t> frame = ws::encodeTextFrame("hello-wss", true);
    ASSERT_GT(SSL_write(ssl, frame.data(), static_cast<int>(frame.size())), 0);

    std::string echo;
    ASSERT_TRUE(sslReadTextFrame(ssl, leftover, echo)) << "no echo frame received";
    EXPECT_EQ(echo, "hello-wss");

    std::vector<uint8_t> closeFrame = ws::encodeCloseFrame(1000, "", true);
    SSL_write(ssl, closeFrame.data(), static_cast<int>(closeFrame.size()));
    SSL_shutdown(ssl);
    SSL_free(ssl);
    ::close(fd);
    server.stop();
}

// (b) The same handshake FAILS when the client does not trust the test CA:
// verification is on, not silently skipped.
TEST_F(WssTest, WssRejectsUntrustedCert) {
    HttpServer server(tlsConfig());
    int port = startEchoServer(server);
    ASSERT_GT(port, 0) << "TLS server failed to start: " << server.getLastError();

    int fd = tcpConnect(port);
    ASSERT_GE(fd, 0);
    SSL* ssl = tlsConnect(port, fd, /*trustCa=*/false);
    EXPECT_EQ(ssl, nullptr) << "handshake with an untrusted CA must fail";
    if (ssl) {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }
    ::close(fd);
    server.stop();
}

#endif // !defined(_WIN32)
