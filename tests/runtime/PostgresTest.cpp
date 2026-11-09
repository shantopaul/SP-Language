// Postgres depth tests (Phase 2, slice B1) -- no live server required.
//
// A minimal fake PostgreSQL wire-protocol stub (loopback TCP listener)
// speaks startup / cleartext + SCRAM-SHA-256 auth / Parse / Bind /
// Describe / Execute / Sync and optional TLS, so the driver can be proven
// against the wire without a real server. Nothing here talks to a real
// PostgreSQL instance.
#if !defined(_WIN32)

#include <gtest/gtest.h>
#include "sp/runtime/Postgres.h"
#include "sp/runtime/Runtime.h"
#include "sp/runtime/Crypto.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

using namespace sp::runtime;
namespace fs = std::filesystem;
namespace crypto = sp::rt::crypto;

// ---------------------------------------------------------------------------
// Small wire helpers (shared by stub and test-side crypto)
// ---------------------------------------------------------------------------

static void putU32(std::vector<uint8_t>& buf, uint32_t v) {
    buf.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}
static void putU16(std::vector<uint8_t>& buf, uint16_t v) {
    buf.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(v & 0xFF));
}
static void putCStr(std::vector<uint8_t>& buf, const std::string& s) {
    buf.insert(buf.end(), s.begin(), s.end());
    buf.push_back(0);
}
static uint32_t getU32(const uint8_t* p) {
    return (static_cast<uint32_t>(p[0]) << 24) | (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | static_cast<uint32_t>(p[3]);
}
static uint16_t getU16(const uint8_t* p) {
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}

static std::vector<uint8_t> hexToBytesTest(const std::string& hex) {
    std::vector<uint8_t> out;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        out.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return out;
}

// PBKDF2-HMAC-SHA-256 (RFC 2898), implemented here from the public crypto
// helpers so the stub can compute the SCRAM values independently of the
// driver's internal (anonymous-namespace) copy.
static std::vector<uint8_t> pbkdf2Test(const std::string& password,
                                       const std::vector<uint8_t>& salt, int iterations) {
    std::vector<uint8_t> block = salt;
    block.push_back(0); block.push_back(0); block.push_back(0); block.push_back(1);
    std::vector<uint8_t> u = hexToBytesTest(crypto::hmacSha256Bytes(
        reinterpret_cast<const uint8_t*>(password.data()), password.size(),
        block.data(), block.size()));
    std::vector<uint8_t> result = u;
    for (int i = 1; i < iterations; ++i) {
        u = hexToBytesTest(crypto::hmacSha256Bytes(
            reinterpret_cast<const uint8_t*>(password.data()), password.size(),
            u.data(), u.size()));
        for (size_t j = 0; j < result.size() && j < u.size(); ++j) result[j] ^= u[j];
    }
    return result;
}

static std::string b64Test(const std::vector<uint8_t>& v) {
    return crypto::base64EncodeBytes(v.data(), v.size());
}
static std::vector<uint8_t> hmacTest(const std::vector<uint8_t>& key, const std::string& msg) {
    return hexToBytesTest(crypto::hmacSha256Bytes(
        key.data(), key.size(),
        reinterpret_cast<const uint8_t*>(msg.data()), msg.size()));
}

// ---------------------------------------------------------------------------
// Fake PostgreSQL server
// ---------------------------------------------------------------------------

struct FdTransport {
    int fd = -1;
    SSL* ssl = nullptr;

    bool readExact(uint8_t* buf, size_t len) {
        size_t got = 0;
        while (got < len) {
            int n = ssl ? SSL_read(ssl, buf + got, static_cast<int>(len - got))
                        : ::recv(fd, reinterpret_cast<char*>(buf + got), len - got, 0);
            if (n <= 0) return false;
            got += static_cast<size_t>(n);
        }
        return true;
    }
    bool writeAll(const uint8_t* buf, size_t len) {
        size_t sent = 0;
        while (sent < len) {
            int n = ssl ? SSL_write(ssl, buf + sent, static_cast<int>(len - sent))
                        : ::send(fd, reinterpret_cast<const char*>(buf + sent), len - sent, 0);
            if (n <= 0) return false;
            sent += static_cast<size_t>(n);
        }
        return true;
    }
};

static bool sendPgMsg(FdTransport& t, char type, const std::vector<uint8_t>& payload) {
    std::vector<uint8_t> buf;
    buf.push_back(static_cast<uint8_t>(type));
    putU32(buf, static_cast<uint32_t>(payload.size() + 4));
    buf.insert(buf.end(), payload.begin(), payload.end());
    return t.writeAll(buf.data(), buf.size());
}

// Read one backend/frontend message: 1 type byte + 4 length bytes + body.
static bool readPgMsg(FdTransport& t, char& type, std::vector<uint8_t>& payload) {
    uint8_t typeBuf[1];
    if (!t.readExact(typeBuf, 1)) return false;
    type = static_cast<char>(typeBuf[0]);
    uint8_t lenBuf[4];
    if (!t.readExact(lenBuf, 4)) return false;
    uint32_t total = getU32(lenBuf);
    if (total < 4) return false;
    payload.resize(total - 4);
    if (total > 4 && !t.readExact(payload.data(), total - 4)) return false;
    return true;
}

static std::vector<uint8_t> authOkMsg() {
    std::vector<uint8_t> p;
    putU32(p, 0);
    return p;
}
static std::vector<uint8_t> readyMsg() {
    return std::vector<uint8_t>{static_cast<uint8_t>('I')};
}
static std::vector<uint8_t> errorMsg(const std::string& severity, const std::string& code,
                                     const std::string& message) {
    std::vector<uint8_t> p;
    p.push_back('S'); putCStr(p, severity);
    p.push_back('C'); putCStr(p, code);
    p.push_back('M'); putCStr(p, message);
    p.push_back(0);
    return p;
}
static std::vector<uint8_t> rowDescMsg() {
    // One text column named "value", type OID 25.
    std::vector<uint8_t> p;
    putU16(p, 1);
    putCStr(p, "value");
    putU32(p, 0);    // table OID
    putU16(p, 0);    // column attribute number
    putU32(p, 25);   // type OID (text)
    putU16(p, 0xFFFF); // type length (-1)
    putU32(p, 0xFFFFFFFF); // type modifier (-1)
    putU16(p, 0);    // text format
    return p;
}
static std::vector<uint8_t> dataRowMsg(const std::string& value) {
    std::vector<uint8_t> p;
    putU16(p, 1);
    putU32(p, static_cast<uint32_t>(value.size()));
    p.insert(p.end(), value.begin(), value.end());
    return p;
}
static std::vector<uint8_t> commandCompleteMsg(const std::string& tag) {
    std::vector<uint8_t> p(tag.begin(), tag.end());
    p.push_back(0);
    return p;
}

// Parse a SCRAM comma-separated attribute string into key -> value.
static std::map<std::string, std::string> scramAttrs(const std::string& s) {
    std::map<std::string, std::string> out;
    size_t pos = 0;
    while (pos < s.size()) {
        size_t comma = s.find(',', pos);
        std::string tok = s.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
        auto eq = tok.find('=');
        if (eq != std::string::npos) out[tok.substr(0, eq)] = tok.substr(eq + 1);
        if (comma == std::string::npos) break;
        pos = comma + 1;
    }
    return out;
}

class FakePgServer {
public:
    enum class AuthMode { Trust, ScramCorrect, ScramWrong, ScramNoSignature };

    AuthMode auth = AuthMode::Trust;
    bool useTls = false;
    bool tlsSendNo = false; // answer 'N' to SSLRequest instead of 'S'
    std::string certFile;
    std::string keyFile;
    std::string password = "secretpw";

    std::atomic<int> parseCount{0};
    std::atomic<int> bindCount{0};
    std::atomic<bool> scramClientProofValid{false};
    std::atomic<int> connections{0};

    FakePgServer() = default;
    ~FakePgServer() { stop(); }
    FakePgServer(const FakePgServer&) = delete;
    FakePgServer& operator=(const FakePgServer&) = delete;

    void start() {
        // The stub writes to sockets the client may already have closed
        // (e.g. right after a failed SCRAM check); a SIGPIPE must surface
        // as a failed write, not kill the whole test process.
        std::signal(SIGPIPE, SIG_IGN);
        listenFd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        int one = 1;
        ::setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port = 0;
        if (::bind(listenFd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
            FAIL() << "fake server bind failed";
        }
        if (::listen(listenFd_, 8) != 0) FAIL() << "fake server listen failed";
        sockaddr_in bound{};
        socklen_t len = sizeof(bound);
        ::getsockname(listenFd_, reinterpret_cast<sockaddr*>(&bound), &len);
        port_.store(ntohs(bound.sin_port));
        running_.store(true);
        thread_ = std::thread([this] { acceptLoop(); });
    }

    int port() const { return port_.load(); }

    void waitReady() {
        for (int i = 0; i < 500 && port() == 0; ++i) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        ASSERT_GT(port(), 0);
    }

    int parseCountFor(const std::string& name) {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = parseByName_.find(name);
        return it == parseByName_.end() ? 0 : it->second;
    }
    std::string lastBindStmt() {
        std::lock_guard<std::mutex> lk(mu_);
        return lastBindStmt_;
    }
    std::vector<std::string> lastBindParams() {
        std::lock_guard<std::mutex> lk(mu_);
        return lastBindParams_;
    }

    void stop() {
        if (!running_.exchange(false)) return;
        if (listenFd_ >= 0) {
            ::shutdown(listenFd_, SHUT_RDWR);
            ::close(listenFd_);
            listenFd_ = -1;
        }
        if (thread_.joinable()) thread_.join();
    }

private:
    void acceptLoop() {
        while (running_.load()) {
            int c = ::accept(listenFd_, nullptr, nullptr);
            if (c < 0) {
                if (!running_.load()) break;
                continue;
            }
            connections.fetch_add(1);
            serve(c);
            ::close(c);
        }
    }

    void serve(int fd) {
        FdTransport t;
        t.fd = fd;
        SSL_CTX* ctx = nullptr;
        SSL* ssl = nullptr;

        if (useTls) {
            // Read the 8-byte SSLRequest first.
            uint8_t req[8];
            if (!t.readExact(req, 8)) return;
            if (tlsSendNo) {
                uint8_t n = 'N';
                t.writeAll(&n, 1);
                // Fall through to a plaintext startup read; the client is
                // expected to fail the connection for verify modes.
            } else {
                uint8_t s = 'S';
                if (!t.writeAll(&s, 1)) return;
                SSL_library_init();
                ctx = SSL_CTX_new(TLS_server_method());
                if (!ctx) return;
                if (SSL_CTX_use_certificate_file(ctx, certFile.c_str(), SSL_FILETYPE_PEM) != 1 ||
                    SSL_CTX_use_PrivateKey_file(ctx, keyFile.c_str(), SSL_FILETYPE_PEM) != 1) {
                    SSL_CTX_free(ctx);
                    return;
                }
                ssl = SSL_new(ctx);
                SSL_set_fd(ssl, fd);
                if (SSL_accept(ssl) <= 0) {
                    SSL_free(ssl);
                    SSL_CTX_free(ctx);
                    return;
                }
                t.ssl = ssl;
            }
        }

        // Startup message: 4-byte length + body.
        {
            uint8_t lenBuf[4];
            if (!t.readExact(lenBuf, 4)) { cleanup(ctx, ssl); return; }
            uint32_t total = getU32(lenBuf);
            if (total < 4 || total > 65536) { cleanup(ctx, ssl); return; }
            std::vector<uint8_t> body(total - 4);
            if (total > 4 && !t.readExact(body.data(), total - 4)) { cleanup(ctx, ssl); return; }
        }

        if (auth != AuthMode::Trust) {
            if (!doScramAuth(t)) { cleanup(ctx, ssl); return; }
        }
        // AuthenticationOk + ReadyForQuery (Trust mode sends them directly;
        // SCRAM mode sends them after the SASL exchange in doScramAuth).
        if (auth == AuthMode::Trust) {
            if (!sendPgMsg(t, 'R', authOkMsg())) { cleanup(ctx, ssl); return; }
            if (!sendPgMsg(t, 'Z', readyMsg())) { cleanup(ctx, ssl); return; }
        }

        commandLoop(t);
        cleanup(ctx, ssl);
    }

    void cleanup(SSL_CTX* ctx, SSL* ssl) {
        if (ssl) { SSL_shutdown(ssl); SSL_free(ssl); }
        if (ctx) SSL_CTX_free(ctx);
    }

    bool doScramAuth(FdTransport& t) {
        // R(10): SASL, mechanisms list "SCRAM-SHA-256\0"
        std::vector<uint8_t> mech;
        putU32(mech, 10);
        putCStr(mech, "SCRAM-SHA-256");
        if (!sendPgMsg(t, 'R', mech)) return false;

        // Read SASLInitialResponse ('p').
        char type = 0;
        std::vector<uint8_t> init;
        if (!readPgMsg(t, type, init) || type != 'p') return false;
        // init = "SCRAM-SHA-256\0" + int32 len + client-first-message
        size_t off = 0;
        while (off < init.size() && init[off] != 0) ++off;
        if (off >= init.size() || off + 5 > init.size()) return false;
        off++; // skip NUL
        uint32_t firstLen = getU32(init.data() + off);
        off += 4;
        if (off + firstLen > init.size()) return false;
        std::string clientFirst(reinterpret_cast<const char*>(init.data() + off), firstLen);
        // "n,,n=<user>,r=<clientNonce>"
        if (clientFirst.rfind("n,,", 0) != 0) return false;
        std::string clientFirstBare = clientFirst.substr(3);
        auto attrs = scramAttrs(clientFirstBare);
        std::string clientNonce = attrs["r"];
        if (clientNonce.empty()) return false;

        // Server-first: fixed salt + 4096 iterations, server nonce extends
        // the client nonce (RFC 7677 requirement).
        std::string saltRaw = "PgTestSalt123456"; // 16 bytes
        std::vector<uint8_t> salt(saltRaw.begin(), saltRaw.end());
        std::string serverNonce = clientNonce + "-srv-nonce";
        std::string serverFirst = "r=" + serverNonce + ",s=" + b64Test(salt) + ",i=4096";
        std::vector<uint8_t> cont;
        putU32(cont, 11);
        cont.insert(cont.end(), serverFirst.begin(), serverFirst.end());
        if (!sendPgMsg(t, 'R', cont)) return false;

        // Read SASLResponse ('p'): client-final-message.
        std::vector<uint8_t> fin;
        if (!readPgMsg(t, type, fin) || type != 'p') return false;
        std::string clientFinal(reinterpret_cast<const char*>(fin.data()), fin.size());
        auto finAttrs = scramAttrs(clientFinal);
        std::string clientFinalWithoutProof = "c=biws,r=" + serverNonce;
        std::string authMessage = clientFirstBare + "," + serverFirst + "," + clientFinalWithoutProof;

        // Verify the client's proof (recorded for the test to assert).
        std::vector<uint8_t> salted = pbkdf2Test(password, salt, 4096);
        std::vector<uint8_t> clientKey = hmacTest(salted, "Client Key");
        std::vector<uint8_t> storedKey = hexToBytesTest(
            crypto::sha256Bytes(clientKey.data(), clientKey.size()));
        std::vector<uint8_t> clientSig = hmacTest(storedKey, authMessage);
        std::vector<uint8_t> expectedProof(32);
        for (size_t i = 0; i < 32; ++i) expectedProof[i] = clientKey[i] ^ clientSig[i];
        std::string proofB64 = finAttrs["p"];
        std::string proofRaw = crypto::base64Decode(proofB64);
        std::vector<uint8_t> proof(proofRaw.begin(), proofRaw.end());
        scramClientProofValid.store(
            crypto::timingSafeEqualBytes(expectedProof.data(), expectedProof.size(),
                                         proof.data(), proof.size()));

        // Server signature (correct unless the test asked for a wrong one).
        std::vector<uint8_t> serverSig;
        if (auth == AuthMode::ScramCorrect) {
            std::vector<uint8_t> serverKey = hmacTest(salted, "Server Key");
            serverSig = hmacTest(serverKey, authMessage);
        } else if (auth == AuthMode::ScramWrong) {
            serverSig.assign(32, 0xAB); // wrong on purpose
        }
        std::vector<uint8_t> finalPayload;
        putU32(finalPayload, 12);
        if (auth != AuthMode::ScramNoSignature) {
            std::string v = "v=" + b64Test(serverSig);
            finalPayload.insert(finalPayload.end(), v.begin(), v.end());
        }
        if (!sendPgMsg(t, 'R', finalPayload)) return false;

        if (!sendPgMsg(t, 'R', authOkMsg())) return false;
        if (!sendPgMsg(t, 'Z', readyMsg())) return false;
        return true;
    }

    void commandLoop(FdTransport& t) {
        std::set<std::string> prepared;
        while (true) {
            char type = 0;
            std::vector<uint8_t> payload;
            if (!readPgMsg(t, type, payload)) return;
            if (type == 'X') return;
            if (type == 'Q') {
                // Simple query: one "value" column, one row "1".
                if (!sendPgMsg(t, 'T', rowDescMsg())) return;
                if (!sendPgMsg(t, 'D', dataRowMsg("1"))) return;
                if (!sendPgMsg(t, 'C', commandCompleteMsg("SELECT 1"))) return;
                if (!sendPgMsg(t, 'Z', readyMsg())) return;
            } else if (type == 'P') {
                parseCount.fetch_add(1);
                size_t off = 0;
                std::string name, sql;
                while (off < payload.size() && payload[off] != 0)
                    name.push_back(static_cast<char>(payload[off++]));
                ++off;
                while (off < payload.size() && payload[off] != 0)
                    sql.push_back(static_cast<char>(payload[off++]));
                {
                    std::lock_guard<std::mutex> lk(mu_);
                    parseByName_[name]++;
                }
                if (!name.empty() && prepared.count(name) != 0) {
                    // Duplicate named statement: real PostgreSQL answers
                    // 42P05. No ParseComplete; ReadyForQuery follows Sync.
                    if (!sendPgMsg(t, 'E', errorMsg("ERROR", "42P05",
                        "prepared statement \"" + name + "\" already exists"))) return;
                } else {
                    if (!name.empty()) prepared.insert(name);
                    if (!sendPgMsg(t, '1', {})) return;
                }
            } else if (type == 'B') {
                bindCount.fetch_add(1);
                size_t off = 0;
                std::string portal, stmt;
                while (off < payload.size() && payload[off] != 0)
                    portal.push_back(static_cast<char>(payload[off++]));
                ++off;
                while (off < payload.size() && payload[off] != 0)
                    stmt.push_back(static_cast<char>(payload[off++]));
                ++off;
                if (off + 2 > payload.size()) return;
                uint16_t nFmt = getU16(payload.data() + off); off += 2 + 2 * nFmt;
                if (off + 2 > payload.size()) return;
                uint16_t nParams = getU16(payload.data() + off); off += 2;
                std::vector<std::string> params;
                for (uint16_t i = 0; i < nParams; ++i) {
                    if (off + 4 > payload.size()) return;
                    int32_t plen = static_cast<int32_t>(getU32(payload.data() + off)); off += 4;
                    if (plen < 0) { params.emplace_back(""); continue; }
                    if (off + static_cast<size_t>(plen) > payload.size()) return;
                    params.emplace_back(reinterpret_cast<const char*>(payload.data() + off), plen);
                    off += plen;
                }
                {
                    std::lock_guard<std::mutex> lk(mu_);
                    lastBindStmt_ = stmt;
                    lastBindParams_ = params;
                }
                if (!sendPgMsg(t, '2', {})) return;
            } else if (type == 'D') {
                if (!sendPgMsg(t, 'T', rowDescMsg())) return;
            } else if (type == 'E') {
                std::string value = "1";
                {
                    std::lock_guard<std::mutex> lk(mu_);
                    if (!lastBindParams_.empty()) value = lastBindParams_[0];
                }
                if (!sendPgMsg(t, 'D', dataRowMsg(value))) return;
                if (!sendPgMsg(t, 'C', commandCompleteMsg("SELECT 1"))) return;
            } else if (type == 'S') {
                if (!sendPgMsg(t, 'Z', readyMsg())) return;
            }
            // Unknown message types are ignored.
        }
    }

    int listenFd_ = -1;
    std::atomic<int> port_{0};
    std::atomic<bool> running_{false};
    std::thread thread_;
    std::mutex mu_;
    std::map<std::string, int> parseByName_;
    std::string lastBindStmt_;
    std::vector<std::string> lastBindParams_;
};

static PgConfig cfgFor(FakePgServer& srv, const std::string& sslmode = "disable") {
    PgConfig cfg;
    cfg.host = "127.0.0.1";
    cfg.port = srv.port();
    cfg.user = "testuser";
    cfg.password = "secretpw";
    cfg.database = "testdb";
    cfg.sslmode = sslmode;
    return cfg;
}

// ---------------------------------------------------------------------------
// TLS test certificates (generated with the openssl CLI, if present)
// ---------------------------------------------------------------------------

static bool opensslAvailable() {
    return std::system("openssl version >/dev/null 2>&1") == 0;
}

static std::string uniqueTmpDir(const std::string& tag) {
    static std::atomic<int> counter{0};
    fs::path dir = fs::temp_directory_path() /
        ("sp_pg_b1_" + tag + "_" + std::to_string(::getpid()) + "_" +
         std::to_string(counter.fetch_add(1)));
    fs::create_directories(dir);
    return dir.string();
}

// Self-signed cert for the given SAN (e.g. "IP:127.0.0.1" or
// "DNS:other.example.com"). Returns false when openssl is unavailable or
// generation fails; callers must fail/skip explicitly on false.
static bool generateSelfSignedCert(const std::string& dir, const std::string& cn,
                                   const std::string& san,
                                   std::string& certOut, std::string& keyOut) {
    certOut = dir + "/cert.pem";
    keyOut = dir + "/key.pem";
    std::string cmd = "openssl req -x509 -newkey rsa:2048 -keyout '" + keyOut +
        "' -out '" + certOut + "' -days 2 -nodes -subj '/CN=" + cn +
        "' -addext 'subjectAltName=" + san + "' >/dev/null 2>&1";
    if (std::system(cmd.c_str()) != 0) return false;
    return fs::exists(certOut) && fs::exists(keyOut);
}

// ---------------------------------------------------------------------------
// PgConfig: new sslmode values
// ---------------------------------------------------------------------------

TEST(PostgresB1, ConfigParsesVerifyModesAndSslRootCert) {
    auto cfg = PgConfig::parseConnectionString(
        "postgres://u:p@127.0.0.1:5432/db?sslmode=verify-full&sslrootcert=/tmp/ca.pem");
    EXPECT_EQ(cfg.sslmode, "verify-full");
    EXPECT_EQ(cfg.sslrootcert, "/tmp/ca.pem");
    EXPECT_TRUE(cfg.sslRequiresVerification());
    EXPECT_TRUE(cfg.sslRequiresHostnameVerification());

    auto cfgCa = PgConfig::parseConnectionString(
        "host=127.0.0.1 port=5432 user=u password=p dbname=db sslmode=verify-ca sslrootcert=/tmp/ca2.pem");
    EXPECT_EQ(cfgCa.sslmode, "verify-ca");
    EXPECT_EQ(cfgCa.sslrootcert, "/tmp/ca2.pem");
    EXPECT_TRUE(cfgCa.sslRequiresVerification());
    EXPECT_FALSE(cfgCa.sslRequiresHostnameVerification());

    auto cfgReq = PgConfig::parseConnectionString("host=h sslmode=require");
    EXPECT_FALSE(cfgReq.sslRequiresVerification());
    EXPECT_FALSE(cfgReq.sslRequiresHostnameVerification());
    EXPECT_TRUE(cfgReq.sslrootcert.empty());

    auto cfgDef = PgConfig::parseConnectionString("host=h");
    EXPECT_EQ(cfgDef.sslmode, "disable");
    EXPECT_FALSE(cfgDef.sslRequiresVerification());
    EXPECT_TRUE(cfgDef.sslrootcert.empty());
}

// ---------------------------------------------------------------------------
// Prepared statements against the fake server
// ---------------------------------------------------------------------------

TEST(PostgresB1, PreparedStatementPrepareExecAndReuse) {
    FakePgServer srv;
    srv.start();
    srv.waitReady();

    PgConnection conn;
    ASSERT_TRUE(conn.connect(cfgFor(srv)));

    ASSERT_TRUE(conn.prepare("get_val", "SELECT $1::text"));
    EXPECT_TRUE(conn.hasPrepared("get_val"));

    auto res1 = conn.execPrepared("get_val", {"alice"});
    ASSERT_NE(res1, nullptr);
    ASSERT_TRUE(res1->isOk()) << res1->errorMessage();
    ASSERT_EQ(res1->rowCount(), 1);
    EXPECT_EQ(res1->columnName(0), "value");
    EXPECT_EQ(res1->getValue(0, 0), "alice");

    // Second exec reuses the prepared statement: Parse must NOT be sent again.
    auto res2 = conn.execPrepared("get_val", {"bob"});
    ASSERT_NE(res2, nullptr);
    ASSERT_TRUE(res2->isOk());
    EXPECT_EQ(res2->getValue(0, 0), "bob");

    EXPECT_EQ(srv.parseCountFor("get_val"), 1);
    EXPECT_EQ(srv.bindCount.load(), 2);
    EXPECT_EQ(srv.lastBindStmt(), "get_val");

    // Duplicate prepare fails loudly (server 42P05), old statement kept.
    EXPECT_FALSE(conn.prepare("get_val", "SELECT $1::text"));
    EXPECT_NE(conn.lastError().find("already exists"), std::string::npos);
    EXPECT_TRUE(conn.hasPrepared("get_val"));
    auto res3 = conn.execPrepared("get_val", {"carol"});
    ASSERT_TRUE(res3->isOk());
    EXPECT_EQ(res3->getValue(0, 0), "carol");

    // Unknown statement: real error, never a fake success.
    auto resBad = conn.execPrepared("no_such_stmt", {"x"});
    ASSERT_NE(resBad, nullptr);
    EXPECT_FALSE(resBad->isOk());
    EXPECT_NE(resBad->errorMessage().find("not found"), std::string::npos);

    // queryParams (unnamed statement) still works after the refactor.
    auto resQ = conn.queryParams("SELECT $1::text", {"dave"});
    ASSERT_NE(resQ, nullptr);
    ASSERT_TRUE(resQ->isOk()) << resQ->errorMessage();
    EXPECT_EQ(resQ->getValue(0, 0), "dave");

    conn.close();
    EXPECT_FALSE(conn.hasPrepared("get_val")); // registry cleared on close
    srv.stop();
}

TEST(PostgresB1, PrepareFailsWhenNotConnected) {
    PgConnection conn;
    EXPECT_FALSE(conn.prepare("x", "SELECT 1"));
    EXPECT_FALSE(conn.lastError().empty());
    auto res = conn.execPrepared("x", {});
    ASSERT_NE(res, nullptr);
    EXPECT_FALSE(res->isOk());
}

TEST(PostgresB1, PgCAbiPrepareAndExecPrepared) {
    FakePgServer srv;
    srv.start();
    srv.waitReady();

    std::string cs = "host=127.0.0.1 port=" + std::to_string(srv.port()) +
        " user=testuser password=secretpw dbname=testdb sslmode=disable";
    void* conn = sp_rt_pg_connect(cs.c_str());
    ASSERT_NE(conn, nullptr);

    EXPECT_TRUE(sp_rt_pg_prepare(conn, "q1", "SELECT $1::text"));
    EXPECT_FALSE(sp_rt_pg_prepare(conn, "q1", "SELECT $1::text")); // duplicate
    EXPECT_NE(std::string(sp_rt_pg_last_error(conn)).find("already exists"),
              std::string::npos);

    const char* params[] = {"hello"};
    void* res = sp_rt_pg_exec_prepared(conn, "q1", params, 1);
    ASSERT_NE(res, nullptr);
    EXPECT_TRUE(sp_rt_pg_res_is_ok(res));
    EXPECT_EQ(sp_rt_pg_res_row_count(res), 1);
    EXPECT_STREQ(sp_rt_pg_res_get_value(res, 0, 0), "hello");
    sp_rt_pg_res_free(res);

    // Failed exec returns nullptr, never a fake-success handle.
    void* bad = sp_rt_pg_exec_prepared(conn, "missing", params, 1);
    EXPECT_EQ(bad, nullptr);
    EXPECT_NE(std::string(sp_rt_pg_last_error(conn)).find("not found"),
              std::string::npos);

    // Parse reached the server twice: once for the successful prepare and
    // once for the duplicate, which the server rejected (42P05). The
    // duplicate is detected by the server, not swallowed locally.
    EXPECT_EQ(srv.parseCountFor("q1"), 2);
    sp_rt_pg_close(conn);
    srv.stop();
}

// ---------------------------------------------------------------------------
// SCRAM server-signature verification
// ---------------------------------------------------------------------------

TEST(PostgresB1, ScramWrongServerSignatureFailsConnect) {
    FakePgServer srv;
    srv.auth = FakePgServer::AuthMode::ScramWrong;
    srv.start();
    srv.waitReady();

    PgConnection conn;
    EXPECT_FALSE(conn.connect(cfgFor(srv)));
    EXPECT_NE(conn.lastError().find("SCRAM server signature verification failed"),
              std::string::npos);
    srv.stop();
}

TEST(PostgresB1, ScramMissingServerSignatureFailsConnect) {
    FakePgServer srv;
    srv.auth = FakePgServer::AuthMode::ScramNoSignature;
    srv.start();
    srv.waitReady();

    PgConnection conn;
    EXPECT_FALSE(conn.connect(cfgFor(srv)));
    EXPECT_NE(conn.lastError().find("SCRAM server signature verification failed"),
              std::string::npos);
    srv.stop();
}

TEST(PostgresB1, ScramCorrectServerSignatureSucceeds) {
    FakePgServer srv;
    srv.auth = FakePgServer::AuthMode::ScramCorrect;
    srv.start();
    srv.waitReady();

    PgConnection conn;
    ASSERT_TRUE(conn.connect(cfgFor(srv))) << conn.lastError();
    EXPECT_TRUE(srv.scramClientProofValid.load()); // stub verified our proof too
    EXPECT_TRUE(conn.ping()); // simple query works after SCRAM auth
    conn.close();
    srv.stop();
}

// ---------------------------------------------------------------------------
// TLS verification
// ---------------------------------------------------------------------------

TEST(PostgresB1, TlsVerifyCaRejectsUntrustedCert) {
    if (!opensslAvailable()) {
        GTEST_SKIP() << "openssl CLI not available; cannot generate a test certificate";
    }
    std::string dir = uniqueTmpDir("ca");
    std::string cert, key;
    ASSERT_TRUE(generateSelfSignedCert(dir, "wrong.example.com", "DNS:wrong.example.com", cert, key));

    FakePgServer srv;
    srv.useTls = true;
    srv.certFile = cert;
    srv.keyFile = key;
    srv.start();
    srv.waitReady();

    PgConnection conn;
    EXPECT_FALSE(conn.connect(cfgFor(srv, "verify-ca")));
    // Either the handshake aborts ("certificate verify failed") or the
    // explicit post-handshake check fires; both must mention the certificate.
    EXPECT_NE(conn.lastError().find("certificate"), std::string::npos)
        << conn.lastError();
    conn.close();
    srv.stop();
    fs::remove_all(dir);
}

TEST(PostgresB1, TlsVerifyFullRejectsHostnameMismatch) {
    if (!opensslAvailable()) {
        GTEST_SKIP() << "openssl CLI not available; cannot generate a test certificate";
    }
    std::string dir = uniqueTmpDir("full-mismatch");
    std::string cert, key;
    ASSERT_TRUE(generateSelfSignedCert(dir, "other.example.com", "DNS:other.example.com", cert, key));

    FakePgServer srv;
    srv.useTls = true;
    srv.certFile = cert;
    srv.keyFile = key;
    srv.start();
    srv.waitReady();

    // The cert itself is trusted via sslrootcert, but it is for a different
    // host than 127.0.0.1, so verify-full must refuse it.
    PgConfig cfg = cfgFor(srv, "verify-full");
    cfg.sslrootcert = cert;
    PgConnection conn;
    EXPECT_FALSE(conn.connect(cfg));
    EXPECT_NE(conn.lastError().find("certificate"), std::string::npos)
        << conn.lastError();
    conn.close();
    srv.stop();
    fs::remove_all(dir);
}

TEST(PostgresB1, TlsVerifyFullAcceptsMatchingCert) {
    if (!opensslAvailable()) {
        GTEST_SKIP() << "openssl CLI not available; cannot generate a test certificate";
    }
    std::string dir = uniqueTmpDir("full-ok");
    std::string cert, key;
    ASSERT_TRUE(generateSelfSignedCert(dir, "127.0.0.1", "IP:127.0.0.1", cert, key));

    FakePgServer srv;
    srv.useTls = true;
    srv.certFile = cert;
    srv.keyFile = key;
    srv.start();
    srv.waitReady();

    PgConfig cfg = cfgFor(srv, "verify-full");
    cfg.sslrootcert = cert;
    PgConnection conn;
    ASSERT_TRUE(conn.connect(cfg)) << conn.lastError();
    EXPECT_TRUE(conn.ping());
    conn.close();
    srv.stop();
    fs::remove_all(dir);
}

TEST(PostgresB1, TlsVerifyModeRefusesPlaintextServer) {
    FakePgServer srv;
    srv.useTls = true;
    srv.tlsSendNo = true; // server answers 'N' to SSLRequest
    srv.start();
    srv.waitReady();

    PgConnection conn;
    EXPECT_FALSE(conn.connect(cfgFor(srv, "verify-ca")));
    EXPECT_NE(conn.lastError().find("does not support SSL"), std::string::npos)
        << conn.lastError();
    conn.close();
    srv.stop();
}

TEST(PostgresB1, TlsRequireDoesNotVerifyCert) {
    // Characterisation test, not an endorsement: sslmode=require encrypts
    // but does NOT verify the peer (documented behaviour, unchanged by
    // this slice). A self-signed cert must therefore still connect.
    if (!opensslAvailable()) {
        GTEST_SKIP() << "openssl CLI not available; cannot generate a test certificate";
    }
    std::string dir = uniqueTmpDir("require");
    std::string cert, key;
    ASSERT_TRUE(generateSelfSignedCert(dir, "wrong.example.com", "DNS:wrong.example.com", cert, key));

    FakePgServer srv;
    srv.useTls = true;
    srv.certFile = cert;
    srv.keyFile = key;
    srv.start();
    srv.waitReady();

    PgConnection conn;
    ASSERT_TRUE(conn.connect(cfgFor(srv, "require"))) << conn.lastError();
    EXPECT_TRUE(conn.ping());
    conn.close();
    srv.stop();
    fs::remove_all(dir);
}

#endif // !defined(_WIN32)
