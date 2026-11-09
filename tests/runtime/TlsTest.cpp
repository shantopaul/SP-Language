// TLS tests (Phase 2, slice A).
//
// Test certificates are generated at setup time with the `openssl` CLI:
// a local CA, a CA-signed server cert (SAN IP:127.0.0.1, DNS:localhost),
// a self-signed server cert, and a CA-signed cert for a different hostname.
// If the CLI is unavailable the tests SKIP explicitly instead of faking a pass.
// Nothing here trusts a cert it should not: verification is on by default
// and only the clearly-named insecureSkipVerify opt-in disables it.
//
// KNOWN ENVIRONMENT BLOCKER (pre-existing, not caused by this slice):
// in this environment every test that starts a real HttpServer aborts at
// teardown with `bit out of range 0 - FD_SETSIZE on fd_set` — HttpServer::
// stop() closes the listener while the accept thread is inside
// TcpListener::acceptTimeout's select(), and the fortified FD_ISSET on the
// now-closed handle aborts (same abort hits the pre-existing HttpTest /
// WebTest server tests). The TLS behaviour itself was verified end-to-end
// with a standalone driver (see docs/v2/PHASE-31-PRO-4.0.0.md); these gtest
// cases are the canonical suite and will pass once that Net.cpp race is
// fixed. The two listen()-failure tests below do not start a server and
// pass here today.

#include <gtest/gtest.h>
#include "sp/runtime/Http.h"
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>
#include <unistd.h>

using namespace sp::rt::http;

class TlsTest : public ::testing::Test {
protected:
    std::filesystem::path dir;

    void SetUp() override {
        if (std::system("openssl version >/dev/null 2>&1") != 0) {
            GTEST_SKIP() << "openssl CLI not available; TLS tests need it to generate test certificates";
        }
        const auto* info = ::testing::UnitTest::GetInstance()->current_test_info();
        dir = std::filesystem::temp_directory_path() /
              ("sp_tls_" + std::to_string(::getpid()) + "_" + info->name());
        std::filesystem::create_directories(dir);
        ASSERT_TRUE(genCa()) << "failed to generate test CA";
        ASSERT_TRUE(genServerCert()) << "failed to generate CA-signed server cert";
        ASSERT_TRUE(genSelfSigned()) << "failed to generate self-signed cert";
        ASSERT_TRUE(genOtherHostCert()) << "failed to generate other-host cert";
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
                   " -nodes -subj \"/CN=localhost\""
                   " -addext \"subjectAltName=IP:127.0.0.1,DNS:localhost\" 2>/dev/null") &&
               run("openssl x509 -req -in " + q(dir / "server.csr") +
                   " -CA " + q(dir / "ca.crt") + " -CAkey " + q(dir / "ca.key") +
                   " -CAcreateserial -out " + q(dir / "server.crt") +
                   " -days 2 -copy_extensions copy 2>/dev/null");
    }

    bool genSelfSigned() {
        return run("openssl req -x509 -newkey rsa:2048 -keyout " + q(dir / "self.key") +
                   " -out " + q(dir / "self.crt") +
                   " -days 2 -nodes -subj \"/CN=localhost\""
                   " -addext \"subjectAltName=IP:127.0.0.1,DNS:localhost\" 2>/dev/null");
    }

    // CA-signed cert for a different hostname (used for the mismatch test).
    bool genOtherHostCert() {
        return run("openssl req -newkey rsa:2048 -keyout " + q(dir / "other.key") +
                   " -out " + q(dir / "other.csr") +
                   " -nodes -subj \"/CN=other.local\""
                   " -addext \"subjectAltName=DNS:other.local\" 2>/dev/null") &&
               run("openssl x509 -req -in " + q(dir / "other.csr") +
                   " -CA " + q(dir / "ca.crt") + " -CAkey " + q(dir / "ca.key") +
                   " -CAcreateserial -out " + q(dir / "other.crt") +
                   " -days 2 -copy_extensions copy 2>/dev/null");
    }

    HttpServerConfig tlsConfig(const std::string& cert, const std::string& key) {
        HttpServerConfig cfg;
        cfg.tlsEnabled = true;
        cfg.tlsCertFile = (dir / cert).string();
        cfg.tlsKeyFile = (dir / key).string();
        return cfg;
    }

    // Start a server with one /hello route; returns its ephemeral port.
    int startHelloServer(HttpServer& server) {
        server.get("/hello", [](const HttpRequest&) {
            return HttpResponse::ok("tls-ok");
        });
        if (!server.listen("127.0.0.1", 0)) {
            return -1;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        return server.getPort();
    }

    std::string httpsUrl(int port) {
        return "https://127.0.0.1:" + std::to_string(port) + "/hello";
    }
};

// (a) TLS round-trip: CA-signed server cert, client trusts that CA.
TEST_F(TlsTest, TlsRoundTripWithTrustedCa) {
    HttpServer server(tlsConfig("server.crt", "server.key"));
    int port = startHelloServer(server);
    ASSERT_GT(port, 0) << "TLS server failed to start: " << server.getLastError();

    HttpClientOptions opts;
    opts.caFile = (dir / "ca.crt").string();
    HttpResponse res = HttpClient::get(httpsUrl(port), opts);
    EXPECT_EQ(res.statusCode, 200) << "body: " << res.body;
    EXPECT_EQ(res.body, "tls-ok");
    server.stop();
}

// (b) Client REJECTS a self-signed cert when verification is on (the default).
TEST_F(TlsTest, TlsRejectsSelfSignedCert) {
    HttpServer server(tlsConfig("self.crt", "self.key"));
    int port = startHelloServer(server);
    ASSERT_GT(port, 0) << "TLS server failed to start: " << server.getLastError();

    HttpResponse res = HttpClient::get(httpsUrl(port));  // default: verify on
    EXPECT_EQ(res.statusCode, 500) << "self-signed cert must not be accepted";
    EXPECT_NE(res.body.find("TLS"), std::string::npos) << "error must name TLS: " << res.body;
    EXPECT_EQ(res.body.find("tls-ok"), std::string::npos);
    server.stop();
}

// Hostname verification: cert is CA-signed but for a different host.
TEST_F(TlsTest, TlsRejectsHostnameMismatch) {
    HttpServer server(tlsConfig("other.crt", "other.key"));
    int port = startHelloServer(server);
    ASSERT_GT(port, 0) << "TLS server failed to start: " << server.getLastError();

    HttpClientOptions opts;
    opts.caFile = (dir / "ca.crt").string();
    HttpResponse res = HttpClient::get(httpsUrl(port), opts);
    EXPECT_EQ(res.statusCode, 500) << "hostname mismatch must not be accepted";
    EXPECT_NE(res.body.find("TLS"), std::string::npos) << "error must name TLS: " << res.body;
    server.stop();
}

// The insecure opt-in exists and works — but only when explicitly requested.
TEST_F(TlsTest, TlsInsecureSkipVerifyConnectsToSelfSigned) {
    HttpServer server(tlsConfig("self.crt", "self.key"));
    int port = startHelloServer(server);
    ASSERT_GT(port, 0) << "TLS server failed to start: " << server.getLastError();

    HttpClientOptions opts;
    opts.insecureSkipVerify = true;
    HttpResponse res = HttpClient::get(httpsUrl(port), opts);
    EXPECT_EQ(res.statusCode, 200) << "body: " << res.body;
    EXPECT_EQ(res.body, "tls-ok");
    server.stop();
}

// A TLS server does not silently serve plaintext: a plain HTTP client gets
// no response (empty response -> 500), never a 200 over plaintext.
TEST_F(TlsTest, TlsServerRejectsPlaintextClient) {
    HttpServer server(tlsConfig("server.crt", "server.key"));
    int port = startHelloServer(server);
    ASSERT_GT(port, 0) << "TLS server failed to start: " << server.getLastError();

    HttpResponse res = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/hello");
    EXPECT_EQ(res.statusCode, 500);
    EXPECT_EQ(res.body.find("tls-ok"), std::string::npos);
    server.stop();
}

// (c) Plaintext HTTP still works unchanged.
TEST_F(TlsTest, PlaintextHttpStillWorks) {
    HttpServer server;
    int port = startHelloServer(server);
    ASSERT_GT(port, 0);

    HttpResponse res = HttpClient::get("http://127.0.0.1:" + std::to_string(port) + "/hello");
    EXPECT_EQ(res.statusCode, 200) << "body: " << res.body;
    EXPECT_EQ(res.body, "tls-ok");
    server.stop();
}

// Fail loudly at startup: TLS enabled but the cert file does not exist.
TEST_F(TlsTest, TlsServerRejectsMissingCertFile) {
    HttpServerConfig cfg;
    cfg.tlsEnabled = true;
    cfg.tlsCertFile = (dir / "does-not-exist.crt").string();
    cfg.tlsKeyFile = (dir / "server.key").string();
    HttpServer server(cfg);
    EXPECT_FALSE(server.listen("127.0.0.1", 0));
    EXPECT_FALSE(server.getLastError().empty()) << "listen() must report why TLS failed";
}

// Fail loudly at startup: key does not match the certificate.
TEST_F(TlsTest, TlsServerRejectsMismatchedKey) {
    HttpServerConfig cfg;
    cfg.tlsEnabled = true;
    cfg.tlsCertFile = (dir / "server.crt").string();
    cfg.tlsKeyFile = (dir / "self.key").string();  // belongs to self.crt
    HttpServer server(cfg);
    EXPECT_FALSE(server.listen("127.0.0.1", 0));
    EXPECT_FALSE(server.getLastError().empty()) << "listen() must report why TLS failed";
}
