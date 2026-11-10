#include "sp/runtime/Postgres.h"
#include "sp/runtime/Crypto.h"
#include <iostream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <algorithm>
#include <openssl/ssl.h>
#include <openssl/err.h>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;
#define INVALID_SOCK INVALID_SOCKET
#define CLOSE_SOCK closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
typedef int socket_t;
#define INVALID_SOCK (-1)
#define CLOSE_SOCK ::close
#endif

namespace sp::runtime {

// --- MD5 Implementation for PostgreSQL Authentication (RFC 1321) ---

namespace {

struct MD5Context {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
};

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define FF(a, b, c, d, x, s, ac) { \
    (a) += F((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define GG(a, b, c, d, x, s, ac) { \
    (a) += G((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define HH(a, b, c, d, x, s, ac) { \
    (a) += H((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}
#define II(a, b, c, d, x, s, ac) { \
    (a) += I((b), (c), (d)) + (x) + (uint32_t)(ac); \
    (a) = ROTATE_LEFT((a), (s)); \
    (a) += (b); \
}

void md5Transform(uint32_t state[4], const uint8_t block[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    for (int i = 0, j = 0; i < 16; ++i, j += 4) {
        x[i] = ((uint32_t)block[j]) | (((uint32_t)block[j + 1]) << 8) |
               (((uint32_t)block[j + 2]) << 16) | (((uint32_t)block[j + 3]) << 24);
    }

    // Round 1
    FF(a, b, c, d, x[ 0], 7, 0xd76aa478); FF(d, a, b, c, x[ 1], 12, 0xe8c7b756);
    FF(c, d, a, b, x[ 2], 17, 0x242070db); FF(b, c, d, a, x[ 3], 22, 0xc1bdceee);
    FF(a, b, c, d, x[ 4], 7, 0xf57c0faf); FF(d, a, b, c, x[ 5], 12, 0x4787c62a);
    FF(c, d, a, b, x[ 6], 17, 0xa8304613); FF(b, c, d, a, x[ 7], 22, 0xfd469501);
    FF(a, b, c, d, x[ 8], 7, 0x698098d8); FF(d, a, b, c, x[ 9], 12, 0x8b44f7af);
    FF(c, d, a, b, x[10], 17, 0xffff5bb1); FF(b, c, d, a, x[11], 22, 0x895cd7be);
    FF(a, b, c, d, x[12], 7, 0x6b901122); FF(d, a, b, c, x[13], 12, 0xfd987193);
    FF(c, d, a, b, x[14], 17, 0xa679438e); FF(b, c, d, a, x[15], 22, 0x49b40821);

    // Round 2
    GG(a, b, c, d, x[ 1], 5, 0xf61e2562); GG(d, a, b, c, x[ 6], 9, 0xc040b340);
    GG(c, d, a, b, x[11], 14, 0x265e5a51); GG(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    GG(a, b, c, d, x[ 5], 5, 0xd62f105d); GG(d, a, b, c, x[10], 9, 0x02441453);
    GG(c, d, a, b, x[15], 14, 0xd8a1e681); GG(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    GG(a, b, c, d, x[ 9], 5, 0x21e1cde6); GG(d, a, b, c, x[14], 9, 0xc33707d6);
    GG(c, d, a, b, x[ 3], 14, 0xf4d50d87); GG(b, c, d, a, x[ 8], 20, 0x455a14ed);
    GG(a, b, c, d, x[13], 5, 0xa9e3e905); GG(d, a, b, c, x[ 2], 9, 0xfcefa3f8);
    GG(c, d, a, b, x[ 7], 14, 0x676f02d9); GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    // Round 3
    HH(a, b, c, d, x[ 5], 4, 0xfffa3942); HH(d, a, b, c, x[ 8], 11, 0x8771f681);
    HH(c, d, a, b, x[11], 16, 0x6d9d6122); HH(b, c, d, a, x[14], 23, 0xfde5380c);
    HH(a, b, c, d, x[ 1], 4, 0xa4beea44); HH(d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    HH(c, d, a, b, x[ 7], 16, 0xf6bb4b60); HH(b, c, d, a, x[10], 23, 0xbebfbc70);
    HH(a, b, c, d, x[13], 4, 0x289b7ec6); HH(d, a, b, c, x[ 0], 11, 0xeaa127fa);
    HH(c, d, a, b, x[ 3], 16, 0xd4ef3085); HH(b, c, d, a, x[ 6], 23, 0x04881d05);
    HH(a, b, c, d, x[ 9], 4, 0xd9d4d039); HH(d, a, b, c, x[12], 11, 0xe6db99e5);
    HH(c, d, a, b, x[15], 16, 0x1fa27cf8); HH(b, c, d, a, x[ 2], 23, 0xc4ac5665);

    // Round 4
    II(a, b, c, d, x[ 0], 6, 0xf4292244); II(d, a, b, c, x[ 7], 10, 0x432aff97);
    II(c, d, a, b, x[14], 15, 0xab9423a7); II(b, c, d, a, x[ 5], 21, 0xfc93a039);
    II(a, b, c, d, x[12], 6, 0x655b59c3); II(d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    II(c, d, a, b, x[10], 15, 0xffeff47d); II(b, c, d, a, x[ 1], 21, 0x85845dd1);
    II(a, b, c, d, x[ 8], 6, 0x6fa87e4f); II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    II(c, d, a, b, x[ 6], 15, 0xa3014314); II(b, c, d, a, x[13], 21, 0x4e0811a1);
    II(a, b, c, d, x[ 4], 6, 0xf7537e82); II(d, a, b, c, x[11], 10, 0xbd3af235);
    II(c, d, a, b, x[ 2], 15, 0x2ad7d2bb); II(b, c, d, a, x[ 9], 21, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void md5Init(MD5Context* ctx) {
    ctx->count[0] = ctx->count[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

void md5Update(MD5Context* ctx, const uint8_t* input, size_t inputLen) {
    uint32_t i = 0, index = (ctx->count[0] >> 3) & 0x3F;
    if ((ctx->count[0] += ((uint32_t)inputLen << 3)) < ((uint32_t)inputLen << 3))
        ctx->count[1]++;
    ctx->count[1] += ((uint32_t)inputLen >> 29);

    uint32_t partLen = 64 - index;
    if (inputLen >= partLen) {
        std::memcpy(&ctx->buffer[index], input, partLen);
        md5Transform(ctx->state, ctx->buffer);
        for (i = partLen; i + 63 < inputLen; i += 64) {
            md5Transform(ctx->state, &input[i]);
        }
        index = 0;
    }
    std::memcpy(&ctx->buffer[index], &input[i], inputLen - i);
}

void md5Final(uint8_t digest[16], MD5Context* ctx) {
    static const uint8_t PADDING[64] = { 0x80 };
    uint8_t bits[8];
    for (int i = 0; i < 4; ++i) {
        bits[i] = (uint8_t)((ctx->count[0] >> (i * 8)) & 0xFF);
        bits[i + 4] = (uint8_t)((ctx->count[1] >> (i * 8)) & 0xFF);
    }
    uint32_t index = (ctx->count[0] >> 3) & 0x3F;
    uint32_t padLen = (index < 56) ? (56 - index) : (120 - index);
    md5Update(ctx, PADDING, padLen);
    md5Update(ctx, bits, 8);

    for (int i = 0; i < 4; ++i) {
        digest[i     ] = (uint8_t)((ctx->state[0] >> (i * 8)) & 0xFF);
        digest[i +  4] = (uint8_t)((ctx->state[1] >> (i * 8)) & 0xFF);
        digest[i +  8] = (uint8_t)((ctx->state[2] >> (i * 8)) & 0xFF);
        digest[i + 12] = (uint8_t)((ctx->state[3] >> (i * 8)) & 0xFF);
    }
}

std::string md5Hex(const std::string& input) {
    MD5Context ctx;
    md5Init(&ctx);
    md5Update(&ctx, reinterpret_cast<const uint8_t*>(input.data()), input.size());
    uint8_t digest[16];
    md5Final(digest, &ctx);

    static const char hexChars[] = "0123456789abcdef";
    std::string result;
    result.reserve(32);
    for (int i = 0; i < 16; ++i) {
        result.push_back(hexChars[(digest[i] >> 4) & 0xF]);
        result.push_back(hexChars[digest[i] & 0xF]);
    }
    return result;
}

std::string md5HexBytes(const uint8_t* data, size_t len) {
    MD5Context ctx;
    md5Init(&ctx);
    md5Update(&ctx, data, len);
    uint8_t digest[16];
    md5Final(digest, &ctx);

    static const char hexChars[] = "0123456789abcdef";
    std::string result;
    result.reserve(32);
    for (int i = 0; i < 16; ++i) {
        result.push_back(hexChars[(digest[i] >> 4) & 0xF]);
        result.push_back(hexChars[digest[i] & 0xF]);
    }
    return result;
}

void writeInt32BE(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
}

void writeInt16BE(std::vector<uint8_t>& buf, uint16_t val) {
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
}

void writeStringNull(std::vector<uint8_t>& buf, const std::string& str) {
    buf.insert(buf.end(), str.begin(), str.end());
    buf.push_back(0);
}

uint32_t readInt32BE(const uint8_t* buf) {
    return (static_cast<uint32_t>(buf[0]) << 24) |
           (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) |
           static_cast<uint32_t>(buf[3]);
}

uint16_t readInt16BE(const uint8_t* buf) {
    return (static_cast<uint16_t>(buf[0]) << 8) |
           static_cast<uint16_t>(buf[1]);
}

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        bytes.push_back(static_cast<uint8_t>(std::stoi(hex.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

std::vector<uint8_t> pbkdf2HmacSha256(const std::string& password, const std::vector<uint8_t>& salt, int iterations) {
    std::vector<uint8_t> saltWithInt = salt;
    saltWithInt.push_back(0); saltWithInt.push_back(0); saltWithInt.push_back(0); saltWithInt.push_back(1);

    std::string uHex = sp::rt::crypto::hmacSha256Bytes(
        reinterpret_cast<const uint8_t*>(password.data()), password.size(),
        saltWithInt.data(), saltWithInt.size());
    std::vector<uint8_t> u = hexToBytes(uHex);
    std::vector<uint8_t> result = u;

    for (int iter = 1; iter < iterations; ++iter) {
        uHex = sp::rt::crypto::hmacSha256Bytes(
            reinterpret_cast<const uint8_t*>(password.data()), password.size(),
            u.data(), u.size());
        u = hexToBytes(uHex);
        for (size_t i = 0; i < 32 && i < u.size(); ++i) {
            result[i] ^= u[i];
        }
    }
    return result;
}

} // namespace

// Extract the human-readable message ('M' field) from an ErrorResponse
// payload; falls back to a generic string when no 'M' field is present.
static std::string pgErrorMessage(const std::vector<uint8_t>& payload) {
    std::string err;
    size_t idx = 0;
    while (idx < payload.size() && payload[idx] != 0) {
        char code = static_cast<char>(payload[idx++]);
        std::string val;
        while (idx < payload.size() && payload[idx] != 0) {
            val.push_back(static_cast<char>(payload[idx++]));
        }
        if (idx < payload.size()) idx++; // Skip null terminator
        if (code == 'M') err = val;
    }
    return err.empty() ? "PostgreSQL error" : err;
}

// --- PgConfig ---

PgConfig PgConfig::parseConnectionString(const std::string& connStr) {
    PgConfig cfg;
    if (connStr.empty()) return cfg;

    // URI format: postgres://[user[:password]@]host[:port][/database][?params]
    // or postgresql://...
    std::string s = connStr;
    const std::string prefix1 = "postgres://";
    const std::string prefix2 = "postgresql://";
    if (s.rfind(prefix1, 0) == 0) {
        s = s.substr(prefix1.size());
    } else if (s.rfind(prefix2, 0) == 0) {
        s = s.substr(prefix2.size());
    } else {
        // Key-value format: host=localhost port=5432 user=...
        std::istringstream iss(connStr);
        std::string token;
        while (iss >> token) {
            auto eq = token.find('=');
            if (eq != std::string::npos) {
                std::string k = token.substr(0, eq);
                std::string v = token.substr(eq + 1);
                if (k == "host") cfg.host = v;
                else if (k == "port") cfg.port = std::stoi(v);
                else if (k == "user") cfg.user = v;
                else if (k == "password") cfg.password = v;
                else if (k == "dbname" || k == "database") cfg.database = v;
                else if (k == "sslmode") cfg.sslmode = v;
                else if (k == "sslrootcert") cfg.sslrootcert = v;
            }
        }
        return cfg;
    }

    // Split query params if any
    auto qPos = s.find('?');
    if (qPos != std::string::npos) {
        std::string q = s.substr(qPos + 1);
        s = s.substr(0, qPos);
        std::istringstream qss(q);
        std::string param;
        while (std::getline(qss, param, '&')) {
            auto eq = param.find('=');
            if (eq != std::string::npos) {
                std::string k = param.substr(0, eq);
                std::string v = param.substr(eq + 1);
                if (k == "sslmode") cfg.sslmode = v;
                else if (k == "sslrootcert") cfg.sslrootcert = v;
            }
        }
    }

    // User/Password
    auto atPos = s.find('@');
    if (atPos != std::string::npos) {
        std::string userPass = s.substr(0, atPos);
        s = s.substr(atPos + 1);
        auto colonPos = userPass.find(':');
        if (colonPos != std::string::npos) {
            cfg.user = userPass.substr(0, colonPos);
            cfg.password = userPass.substr(colonPos + 1);
        } else {
            cfg.user = userPass;
        }
    }

    // Database name
    auto slashPos = s.find('/');
    if (slashPos != std::string::npos) {
        cfg.database = s.substr(slashPos + 1);
        s = s.substr(0, slashPos);
    }

    // Host & Port
    auto colonPos = s.find(':');
    if (colonPos != std::string::npos) {
        cfg.host = s.substr(0, colonPos);
        cfg.port = std::stoi(s.substr(colonPos + 1));
    } else if (!s.empty()) {
        cfg.host = s;
    }

    return cfg;
}

// --- PgResult ---

void PgResult::setCommandTag(const std::string& tag) {
    m_commandTag = tag;
    // Common tags: "INSERT 0 1", "UPDATE 3", "DELETE 2", "SELECT 4"
    auto lastSpace = tag.rfind(' ');
    if (lastSpace != std::string::npos) {
        try {
            m_affectedRows = std::stoll(tag.substr(lastSpace + 1));
        } catch (...) {
            m_affectedRows = 0;
        }
    }
}

const std::string& PgResult::columnName(int col) const {
    static const std::string empty;
    if (col >= 0 && col < static_cast<int>(m_columns.size())) {
        return m_columns[col].name;
    }
    return empty;
}

uint32_t PgResult::columnTypeOid(int col) const {
    if (col >= 0 && col < static_cast<int>(m_columns.size())) {
        return m_columns[col].typeOid;
    }
    return 0;
}

const std::string& PgResult::getValue(int row, int col) const {
    static const std::string empty;
    if (row >= 0 && row < static_cast<int>(m_rows.size())) {
        const auto& r = m_rows[row];
        if (col >= 0 && col < static_cast<int>(r.values.size())) {
            return r.values[col];
        }
    }
    return empty;
}

bool PgResult::isNull(int row, int col) const {
    if (row >= 0 && row < static_cast<int>(m_rows.size())) {
        const auto& r = m_rows[row];
        if (col >= 0 && col < static_cast<int>(r.isNull.size())) {
            return r.isNull[col];
        }
    }
    return true;
}

// --- PgConnection ---

PgConnection::~PgConnection() {
    close();
}

PgConnection::PgConnection(PgConnection&& other) noexcept
    : m_socket(other.m_socket),
      m_sslCtx(other.m_sslCtx),
      m_ssl(other.m_ssl),
      m_config(std::move(other.m_config)),
      m_lastError(std::move(other.m_lastError)),
      m_connected(other.m_connected),
      m_prepared(std::move(other.m_prepared)),
      m_transactionStatus(other.m_transactionStatus),
      m_backendPid(other.m_backendPid),
      m_backendKey(other.m_backendKey) {
    other.m_socket = 0;
    other.m_sslCtx = nullptr;
    other.m_ssl = nullptr;
    other.m_connected = false;
}

PgConnection& PgConnection::operator=(PgConnection&& other) noexcept {
    if (this != &other) {
        close();
        m_socket = other.m_socket;
        m_sslCtx = other.m_sslCtx;
        m_ssl = other.m_ssl;
        m_config = std::move(other.m_config);
        m_lastError = std::move(other.m_lastError);
        m_connected = other.m_connected;
        m_prepared = std::move(other.m_prepared);
        m_transactionStatus = other.m_transactionStatus;
        m_backendPid = other.m_backendPid;
        m_backendKey = other.m_backendKey;
        other.m_socket = 0;
        other.m_sslCtx = nullptr;
        other.m_ssl = nullptr;
        other.m_connected = false;
    }
    return *this;
}

bool PgConnection::isConnected() const {
    return m_connected && m_socket != 0 && m_socket != static_cast<uintptr_t>(INVALID_SOCK);
}

void PgConnection::close() {
    // Always release whatever resources exist, even if the connection never
    // reached the connected state (e.g. a failed TLS handshake or auth):
    // the old code only freed the socket/SSL when isConnected() was true,
    // leaking the fd and SSL objects on every failed connect().
    if (m_connected && m_socket != 0 && m_socket != static_cast<uintptr_t>(INVALID_SOCK)) {
        sendMessage('X', {}); // Terminate; best effort, ignore failure
    }
    if (m_ssl) {
        SSL_shutdown(static_cast<SSL*>(m_ssl));
        SSL_free(static_cast<SSL*>(m_ssl));
        m_ssl = nullptr;
    }
    if (m_sslCtx) {
        SSL_CTX_free(static_cast<SSL_CTX*>(m_sslCtx));
        m_sslCtx = nullptr;
    }
    if (m_socket != 0 && m_socket != static_cast<uintptr_t>(INVALID_SOCK)) {
        CLOSE_SOCK(static_cast<socket_t>(m_socket));
    }
    m_socket = 0;
    m_connected = false;
    m_prepared.clear();
}

bool PgConnection::connect(const std::string& connStr) {
    return connect(PgConfig::parseConnectionString(connStr));
}

bool PgConnection::connect(const PgConfig& config) {
    close();
    m_config = config;

#if defined(_WIN32)
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* result = nullptr;
    std::string portStr = std::to_string(m_config.port);
    int rc = getaddrinfo(m_config.host.c_str(), portStr.c_str(), &hints, &result);
    if (rc != 0 || !result) {
        m_lastError = "Failed to resolve host: " + m_config.host;
        return false;
    }

    socket_t sock = INVALID_SOCK;
    for (struct addrinfo* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        sock = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (sock == INVALID_SOCK) continue;

        if (::connect(sock, ptr->ai_addr, static_cast<int>(ptr->ai_addrlen)) == 0) {
            break;
        }

        CLOSE_SOCK(sock);
        sock = INVALID_SOCK;
    }
    freeaddrinfo(result);

    if (sock == INVALID_SOCK) {
        m_lastError = "Could not connect to PostgreSQL server at " + m_config.host + ":" + portStr;
        return false;
    }

    m_socket = static_cast<uintptr_t>(sock);

    // Negotiate TLS/SSL if requested
    if (m_config.sslmode != "disable") {
        std::vector<uint8_t> sslReq;
        writeInt32BE(sslReq, 8);
        writeInt32BE(sslReq, 80877103); // SSLRequest code
        if (::send(sock, reinterpret_cast<const char*>(sslReq.data()), static_cast<int>(sslReq.size()), 0) != 8) {
            m_lastError = "Failed to send SSLRequest to " + m_config.host;
            close();
            return false;
        }

        char sslReply = 0;
        if (::recv(sock, &sslReply, 1, 0) != 1) {
            m_lastError = "Failed to receive SSL response from " + m_config.host;
            close();
            return false;
        }

        if (sslReply == 'S') {
            OPENSSL_init_ssl(0, NULL);
            const SSL_METHOD* method = TLS_client_method();
            SSL_CTX* ctx = SSL_CTX_new(method);
            if (!ctx) {
                m_lastError = "Failed to create OpenSSL context";
                close();
                return false;
            }
            // Certificate verification (verify-ca / verify-full). This must
            // be configured BEFORE SSL_connect: the default context does
            // not verify the peer at all, which is exactly what sslmode=
            // require keeps doing below (encrypted but unauthenticated --
            // documented, not verified).
            const bool verifyPeer = m_config.sslRequiresVerification();
            if (verifyPeer) {
                SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
                if (SSL_CTX_set_default_verify_paths(ctx) != 1) {
                    m_lastError = "Failed to load default TLS trust roots";
                    SSL_CTX_free(ctx);
                    close();
                    return false;
                }
                if (!m_config.sslrootcert.empty()) {
                    if (SSL_CTX_load_verify_locations(ctx, m_config.sslrootcert.c_str(), nullptr) != 1) {
                        m_lastError = "Failed to load sslrootcert: " + m_config.sslrootcert;
                        SSL_CTX_free(ctx);
                        close();
                        return false;
                    }
                }
            }
            SSL* ssl = SSL_new(ctx);
            if (!ssl) {
                SSL_CTX_free(ctx);
                m_lastError = "Failed to create OpenSSL structure";
                close();
                return false;
            }
            SSL_set_fd(ssl, static_cast<int>(sock));
            // SNI: Server Name Indication (Required by cloud tenants including Neon)
            SSL_set_tlsext_host_name(ssl, m_config.host.c_str());
            // verify-full: the certificate must match the host we connected
            // to (SAN DNS/IP, via X509_check_host semantics). Set before
            // the handshake so verification happens during SSL_connect.
            if (m_config.sslRequiresHostnameVerification()) {
                if (SSL_set1_host(ssl, m_config.host.c_str()) != 1) {
                    m_lastError = "Failed to configure TLS hostname verification for host: " + m_config.host;
                    SSL_free(ssl);
                    SSL_CTX_free(ctx);
                    close();
                    return false;
                }
                SSL_set_hostflags(ssl, 0);
            }

            if (SSL_connect(ssl) <= 0) {
                char errBuf[256];
                ERR_error_string_n(ERR_get_error(), errBuf, sizeof(errBuf));
                m_lastError = std::string("TLS handshake failed: ") + errBuf;
                SSL_free(ssl);
                SSL_CTX_free(ctx);
                close();
                return false;
            }

            // The handshake alone does not prove the chain verified: with
            // SSL_VERIFY_PEER OpenSSL aborts the handshake on a bad chain,
            // but check the result explicitly so a verification failure can
            // never slip through as a connected session.
            if (verifyPeer) {
                long verifyResult = SSL_get_verify_result(ssl);
                if (verifyResult != X509_V_OK) {
                    m_lastError = std::string("TLS certificate verification failed: ") +
                        X509_verify_cert_error_string(verifyResult);
                    SSL_free(ssl);
                    SSL_CTX_free(ctx);
                    close();
                    return false;
                }
            }

            m_sslCtx = ctx;
            m_ssl = ssl;
        } else if (m_config.sslmode == "require" || m_config.sslRequiresVerification()) {
            // Server answered 'N' (no TLS). require and both verify modes
            // must fail loudly -- never silently fall back to plaintext.
            // sslmode=prefer (and other non-verifying modes) keep the
            // previous fall-through to an unencrypted connection.
            m_lastError = "Server does not support SSL but sslmode=" + m_config.sslmode + " was specified";
            close();
            return false;
        }
    }

    if (!sendStartupMessage()) {
        close();
        return false;
    }

    if (!processAuthMessages()) {
        close();
        return false;
    }

    m_connected = true;
    return true;
}

bool PgConnection::writeBytes(const uint8_t* buf, size_t len) {
    if (!m_socket) return false;
    size_t total = 0;
    while (total < len) {
        int sent = 0;
        if (m_ssl) {
            sent = SSL_write(static_cast<SSL*>(m_ssl), buf + total, static_cast<int>(len - total));
        } else {
#if defined(MSG_NOSIGNAL)
            // A server that already closed the connection must surface as a
            // failed write, not kill the process with SIGPIPE.
            sent = send(static_cast<socket_t>(m_socket), reinterpret_cast<const char*>(buf + total), static_cast<int>(len - total), MSG_NOSIGNAL);
#else
            sent = send(static_cast<socket_t>(m_socket), reinterpret_cast<const char*>(buf + total), static_cast<int>(len - total), 0);
#endif
        }
        if (sent <= 0) return false;
        total += sent;
    }
    return true;
}

bool PgConnection::readBytes(uint8_t* buf, size_t len) {
    if (!m_socket) return false;
    size_t total = 0;
    while (total < len) {
        int n = 0;
        if (m_ssl) {
            n = SSL_read(static_cast<SSL*>(m_ssl), buf + total, static_cast<int>(len - total));
        } else {
            n = recv(static_cast<socket_t>(m_socket), reinterpret_cast<char*>(buf + total), static_cast<int>(len - total), 0);
        }
        if (n <= 0) return false;
        total += n;
    }
    return true;
}

bool PgConnection::sendMessage(char msgType, const std::vector<uint8_t>& payload) {
    if (!m_socket) return false;
    std::vector<uint8_t> buffer;
    buffer.reserve(payload.size() + 5);
    if (msgType != 0) {
        buffer.push_back(static_cast<uint8_t>(msgType));
    }
    uint32_t len = static_cast<uint32_t>(payload.size() + 4);
    writeInt32BE(buffer, len);
    buffer.insert(buffer.end(), payload.begin(), payload.end());
    return writeBytes(buffer.data(), buffer.size());
}

bool PgConnection::readMessage(char& msgType, std::vector<uint8_t>& payload) {
    if (!m_socket) return false;
    uint8_t typeBuf[1];
    if (!readBytes(typeBuf, 1)) return false;
    msgType = static_cast<char>(typeBuf[0]);

    uint8_t lenBuf[4];
    if (!readBytes(lenBuf, 4)) return false;
    uint32_t totalLen = readInt32BE(lenBuf);
    if (totalLen < 4) return false;
    uint32_t payloadLen = totalLen - 4;

    payload.resize(payloadLen);
    if (payloadLen > 0) {
        if (!readBytes(payload.data(), payloadLen)) return false;
    }
    return true;
}

bool PgConnection::sendStartupMessage() {
    if (!m_socket) return false;
    std::vector<uint8_t> body;
    // Protocol version 3.0 = 196608 (0x00030000)
    writeInt32BE(body, 196608);

    writeStringNull(body, "user");
    writeStringNull(body, m_config.user);

    writeStringNull(body, "database");
    writeStringNull(body, m_config.database);

    writeStringNull(body, "application_name");
    writeStringNull(body, "sp-lang");

    writeStringNull(body, "client_encoding");
    writeStringNull(body, "UTF8");

    body.push_back(0); // Final terminating zero byte

    std::vector<uint8_t> packet;
    uint32_t totalLen = static_cast<uint32_t>(body.size() + 4);
    writeInt32BE(packet, totalLen);
    packet.insert(packet.end(), body.begin(), body.end());

    return writeBytes(packet.data(), packet.size());
}

bool PgConnection::processAuthMessages() {
    while (true) {
        char msgType = 0;
        std::vector<uint8_t> payload;
        if (!readMessage(msgType, payload)) {
            m_lastError = "Connection closed during authentication handshake";
            return false;
        }

        if (msgType == 'R') { // Authentication request
            if (payload.size() < 4) return false;
            uint32_t authType = readInt32BE(payload.data());
            if (authType == 0) {
                // AuthenticationOk
                continue;
            } else if (authType == 3) {
                // CleartextPassword
                if (!sendPasswordMessage(m_config.password)) return false;
            } else if (authType == 5) {
                // MD5Password
                if (payload.size() < 8) return false;
                std::vector<uint8_t> salt(payload.begin() + 4, payload.begin() + 8);
                if (!sendMd5PasswordMessage(salt)) return false;
            } else if (authType == 10) {
                // SASL SCRAM-SHA-256
                std::vector<uint8_t> saslList(payload.begin() + 4, payload.end());
                if (!handleScramAuth(saslList)) return false;
            } else {
                m_lastError = "Unsupported PostgreSQL authentication type: " + std::to_string(authType);
                return false;
            }
        } else if (msgType == 'K') { // BackendKeyData
            if (payload.size() >= 8) {
                m_backendPid = static_cast<int32_t>(readInt32BE(payload.data()));
                m_backendKey = static_cast<int32_t>(readInt32BE(payload.data() + 4));
            }
        } else if (msgType == 'S') { // ParameterStatus
            // Ignore for now
        } else if (msgType == 'Z') { // ReadyForQuery
            if (!payload.empty()) {
                m_transactionStatus = static_cast<char>(payload[0]);
            }
            return true;
        } else if (msgType == 'E') { // ErrorResponse
            std::string errStr;
            size_t idx = 0;
            while (idx < payload.size() && payload[idx] != 0) {
                char code = static_cast<char>(payload[idx++]);
                std::string val;
                while (idx < payload.size() && payload[idx] != 0) {
                    val.push_back(static_cast<char>(payload[idx++]));
                }
                if (idx < payload.size()) idx++; // Skip null terminator
                if (code == 'M') {
                    errStr = val;
                }
            }
            m_lastError = errStr.empty() ? "PostgreSQL authentication error" : errStr;
            return false;
        }
    }
}

bool PgConnection::sendPasswordMessage(const std::string& password) {
    std::vector<uint8_t> payload;
    writeStringNull(payload, password);
    return sendMessage('p', payload);
}

bool PgConnection::sendMd5PasswordMessage(const std::vector<uint8_t>& salt) {
    // hash1 = md5(password + user)
    std::string h1 = md5Hex(m_config.password + m_config.user);

    // hash2 = "md5" + md5(h1 + salt)
    std::vector<uint8_t> h1Salt;
    h1Salt.insert(h1Salt.end(), h1.begin(), h1.end());
    h1Salt.insert(h1Salt.end(), salt.begin(), salt.end());
    std::string h2 = "md5" + md5HexBytes(h1Salt.data(), h1Salt.size());

    return sendPasswordMessage(h2);
}

bool PgConnection::handleScramAuth(const std::vector<uint8_t>& /*saslList*/) {
    // 1. Generate client nonce
    std::string clientNonce = sp::rt::crypto::randomHex(16);
    std::string clientFirstBare = "n=" + m_config.user + ",r=" + clientNonce;
    std::string clientFirstMsg = "n,," + clientFirstBare;

    // Send SASLInitialResponse: 'p'
    // Format: "SCRAM-SHA-256\0" + Int32(clientFirstMsg.length()) + clientFirstMsg
    std::vector<uint8_t> initPayload;
    writeStringNull(initPayload, "SCRAM-SHA-256");
    writeInt32BE(initPayload, static_cast<uint32_t>(clientFirstMsg.size()));
    initPayload.insert(initPayload.end(), clientFirstMsg.begin(), clientFirstMsg.end());
    if (!sendMessage('p', initPayload)) return false;

    // 2. Read AuthenticationSASLContinue (11)
    char msgType = 0;
    std::vector<uint8_t> contPayload;
    if (!readMessage(msgType, contPayload)) return false;
    if (msgType != 'R' || contPayload.size() < 4) return false;
    uint32_t authSub = readInt32BE(contPayload.data());
    if (authSub != 11) {
        m_lastError = "Expected SASLContinue (11), got: " + std::to_string(authSub);
        return false;
    }

    std::string serverFirstMsg(reinterpret_cast<const char*>(contPayload.data() + 4), contPayload.size() - 4);
    // Parse r=, s=, i=
    std::string serverNonce, saltB64;
    int iterations = 4096;
    std::istringstream ss(serverFirstMsg);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (token.rfind("r=", 0) == 0) serverNonce = token.substr(2);
        else if (token.rfind("s=", 0) == 0) saltB64 = token.substr(2);
        else if (token.rfind("i=", 0) == 0) iterations = std::stoi(token.substr(2));
    }

    if (serverNonce.empty() || saltB64.empty()) {
        m_lastError = "Invalid SASL server first message";
        return false;
    }

    // Decode salt
    std::string saltStr = sp::rt::crypto::base64Decode(saltB64);
    std::vector<uint8_t> salt(saltStr.begin(), saltStr.end());

    // 3. Compute SaltedPassword, ClientKey, StoredKey
    std::vector<uint8_t> saltedPassword = pbkdf2HmacSha256(m_config.password, salt, iterations);

    // ClientKey = HMAC(SaltedPassword, "Client Key")
    static const char clientKeyConst[] = "Client Key";
    std::string ckHex = sp::rt::crypto::hmacSha256Bytes(
        saltedPassword.data(), saltedPassword.size(),
        reinterpret_cast<const uint8_t*>(clientKeyConst), sizeof(clientKeyConst) - 1);
    std::vector<uint8_t> clientKey = hexToBytes(ckHex);

    // StoredKey = SHA256(ClientKey)
    std::string skHex = sp::rt::crypto::sha256Bytes(clientKey.data(), clientKey.size());
    std::vector<uint8_t> storedKey = hexToBytes(skHex);

    // AuthMessage = client_first_bare + "," + server_first + "," + client_final_without_proof
    std::string clientFinalWithoutProof = "c=biws,r=" + serverNonce;
    std::string authMessage = clientFirstBare + "," + serverFirstMsg + "," + clientFinalWithoutProof;

    // ClientSignature = HMAC(StoredKey, AuthMessage)
    std::string csHex = sp::rt::crypto::hmacSha256Bytes(
        storedKey.data(), storedKey.size(),
        reinterpret_cast<const uint8_t*>(authMessage.data()), authMessage.size());
    std::vector<uint8_t> clientSig = hexToBytes(csHex);

    // ClientProof = ClientKey XOR ClientSignature
    std::vector<uint8_t> clientProof(32);
    for (size_t i = 0; i < 32; ++i) {
        clientProof[i] = clientKey[i] ^ clientSig[i];
    }
    std::string clientProofB64 = sp::rt::crypto::base64EncodeBytes(clientProof.data(), clientProof.size());

    // 4. Send SASLResponse
    std::string clientFinalMsg = clientFinalWithoutProof + ",p=" + clientProofB64;
    std::vector<uint8_t> finalPayload(clientFinalMsg.begin(), clientFinalMsg.end());
    if (!sendMessage('p', finalPayload)) return false;

    // 5. Read AuthenticationSASLFinal (12) and verify the server's
    // signature (RFC 7677 §3). A server that cannot prove knowledge of
    // the password -- or that sends no signature at all -- must fail
    // authentication; previously the v= value was never checked, so any
    // server (including an impostor) was accepted.
    std::vector<uint8_t> saslFinalPayload;
    if (!readMessage(msgType, saslFinalPayload)) return false;
    if (msgType != 'R' || saslFinalPayload.size() < 4) return false;
    uint32_t finalSub = readInt32BE(saslFinalPayload.data());
    if (finalSub != 12) {
        m_lastError = "Expected SASLFinal (12), got: " + std::to_string(finalSub);
        return false;
    }

    std::string serverFinalMsg(reinterpret_cast<const char*>(saslFinalPayload.data() + 4),
                               saslFinalPayload.size() - 4);
    std::string serverSigB64;
    {
        std::istringstream fss(serverFinalMsg);
        std::string tok;
        while (std::getline(fss, tok, ',')) {
            if (tok.rfind("v=", 0) == 0) serverSigB64 = tok.substr(2);
        }
    }
    if (serverSigB64.empty()) {
        m_lastError = "SCRAM server signature verification failed: server sent no ServerSignature (v=)";
        return false;
    }

    // ServerKey = HMAC(SaltedPassword, "Server Key")
    static const char serverKeyConst[] = "Server Key";
    std::string serverKeyHex = sp::rt::crypto::hmacSha256Bytes(
        saltedPassword.data(), saltedPassword.size(),
        reinterpret_cast<const uint8_t*>(serverKeyConst), sizeof(serverKeyConst) - 1);
    std::vector<uint8_t> serverKey = hexToBytes(serverKeyHex);

    // ServerSignature = HMAC(ServerKey, AuthMessage)
    std::string serverSigHex = sp::rt::crypto::hmacSha256Bytes(
        serverKey.data(), serverKey.size(),
        reinterpret_cast<const uint8_t*>(authMessage.data()), authMessage.size());
    std::vector<uint8_t> expectedSig = hexToBytes(serverSigHex);

    // Compare the decoded bytes in constant time (base64 spellings of the
    // same bytes can differ in padding, so compare bytes, not strings).
    std::string receivedSigStr = sp::rt::crypto::base64Decode(serverSigB64);
    std::vector<uint8_t> receivedSig(receivedSigStr.begin(), receivedSigStr.end());
    if (!sp::rt::crypto::timingSafeEqualBytes(expectedSig.data(), expectedSig.size(),
                                              receivedSig.data(), receivedSig.size())) {
        m_lastError = "SCRAM server signature verification failed";
        return false;
    }

    return true;
}

bool PgConnection::sendParse(const std::string& name, const std::string& sql, int paramTypeCount) {
    std::vector<uint8_t> parsePayload;
    writeStringNull(parsePayload, name);
    writeStringNull(parsePayload, sql);
    writeInt16BE(parsePayload, static_cast<uint16_t>(paramTypeCount));
    for (int i = 0; i < paramTypeCount; ++i) {
        writeInt32BE(parsePayload, 0); // unspecified type OID, infer
    }
    return sendMessage('P', parsePayload);
}

bool PgConnection::sendBindExecute(const std::string& stmtName, const std::vector<std::string>& params) {
    // Bind ('B'): fresh unnamed portal, the named statement, all params
    // in text format, all result columns in text format.
    std::vector<uint8_t> bindPayload;
    writeStringNull(bindPayload, ""); // unnamed portal
    writeStringNull(bindPayload, stmtName);
    writeInt16BE(bindPayload, 0); // all params text format
    writeInt16BE(bindPayload, static_cast<uint16_t>(params.size()));
    for (const auto& p : params) {
        writeInt32BE(bindPayload, static_cast<uint32_t>(p.size()));
        bindPayload.insert(bindPayload.end(), p.begin(), p.end());
    }
    writeInt16BE(bindPayload, 0); // all result columns text format
    if (!sendMessage('B', bindPayload)) return false;

    // Describe portal ('D'): 'P' + unnamed portal name
    std::vector<uint8_t> descPayload;
    descPayload.push_back('P');
    writeStringNull(descPayload, "");
    if (!sendMessage('D', descPayload)) return false;

    // Execute ('E'): unnamed portal, 0 = unlimited rows
    std::vector<uint8_t> execPayload;
    writeStringNull(execPayload, "");
    writeInt32BE(execPayload, 0);
    if (!sendMessage('E', execPayload)) return false;

    // Sync ('S')
    return sendMessage('S', {});
}

bool PgConnection::readResultLoop(const std::shared_ptr<PgResult>& res) {
    while (true) {
        char msgType = 0;
        std::vector<uint8_t> msgData;
        if (!readMessage(msgType, msgData)) {
            res->setOk(false);
            res->setErrorMessage("Connection dropped during query");
            return false;
        }

        if (msgType == '1') { // ParseComplete
            // OK
        } else if (msgType == '2') { // BindComplete
            // OK
        } else if (msgType == 'T') { // RowDescription
            if (msgData.size() >= 2) {
                uint16_t numFields = readInt16BE(msgData.data());
                size_t offset = 2;
                for (uint16_t i = 0; i < numFields; ++i) {
                    PgColumn col;
                    while (offset < msgData.size() && msgData[offset] != 0) {
                        col.name.push_back(static_cast<char>(msgData[offset++]));
                    }
                    if (offset < msgData.size()) offset++; // skip 0
                    if (offset + 18 <= msgData.size()) {
                        col.typeOid = readInt32BE(&msgData[offset + 6]);
                        col.format = static_cast<int16_t>(readInt16BE(&msgData[offset + 16]));
                        offset += 18;
                    }
                    res->addColumn(std::move(col));
                }
            }
        } else if (msgType == 'D') { // DataRow
            if (msgData.size() >= 2) {
                uint16_t numCols = readInt16BE(msgData.data());
                size_t offset = 2;
                PgRow row;
                row.values.reserve(numCols);
                row.isNull.reserve(numCols);

                for (uint16_t i = 0; i < numCols; ++i) {
                    if (offset + 4 <= msgData.size()) {
                        int32_t len = static_cast<int32_t>(readInt32BE(&msgData[offset]));
                        offset += 4;
                        if (len == -1) {
                            row.values.push_back("");
                            row.isNull.push_back(true);
                        } else if (offset + static_cast<size_t>(len) <= msgData.size()) {
                            std::string val(reinterpret_cast<const char*>(&msgData[offset]), len);
                            row.values.push_back(std::move(val));
                            row.isNull.push_back(false);
                            offset += len;
                        }
                    }
                }
                res->addRow(std::move(row));
            }
        } else if (msgType == 'C') { // CommandComplete
            std::string tag(reinterpret_cast<const char*>(msgData.data()), msgData.size());
            if (!tag.empty() && tag.back() == 0) tag.pop_back();
            res->setCommandTag(tag);
        } else if (msgType == 'E') { // ErrorResponse
            // Record the error but keep reading until ReadyForQuery: the
            // server still sends it after Sync, and stopping early would
            // desynchronise the connection.
            res->setOk(false);
            res->setErrorMessage(pgErrorMessage(msgData));
        } else if (msgType == 'Z') { // ReadyForQuery
            if (!msgData.empty()) {
                m_transactionStatus = static_cast<char>(msgData[0]);
            }
            return true;
        }
        // Any other message type (NoticeResponse 'N', ParameterStatus 'S',
        // etc.) is ignored; the loop continues until ReadyForQuery.
    }
}

std::shared_ptr<PgResult> PgConnection::query(const std::string& sql) {
    auto res = std::make_shared<PgResult>();
    if (!isConnected()) {
        res->setOk(false);
        res->setErrorMessage("Not connected to PostgreSQL");
        return res;
    }

    std::vector<uint8_t> payload;
    writeStringNull(payload, sql);
    if (!sendMessage('Q', payload)) {
        res->setOk(false);
        res->setErrorMessage("Failed to send query");
        return res;
    }

    readResultLoop(res);
    return res;
}

std::shared_ptr<PgResult> PgConnection::queryParams(const std::string& sql, const std::vector<std::string>& params) {
    // If no params, fallback to simple query
    if (params.empty()) {
        return query(sql);
    }

    auto res = std::make_shared<PgResult>();
    if (!isConnected()) {
        res->setOk(false);
        res->setErrorMessage("Not connected to PostgreSQL");
        return res;
    }

    // Parse the unnamed statement, then Bind/Describe/Execute/Sync.
    if (!sendParse("", sql, static_cast<int>(params.size()))) {
        res->setOk(false);
        res->setErrorMessage("Failed to send Parse message");
        return res;
    }
    if (!sendBindExecute("", params)) {
        res->setOk(false);
        res->setErrorMessage("Failed to send Bind message");
        return res;
    }

    readResultLoop(res);
    return res;
}

bool PgConnection::prepare(const std::string& name, const std::string& sql) {
    if (!isConnected()) {
        m_lastError = "Not connected to PostgreSQL";
        return false;
    }
    if (name.empty()) {
        m_lastError = "Prepared statement name must not be empty";
        return false;
    }
    if (sql.empty()) {
        m_lastError = "Prepared statement SQL must not be empty";
        return false;
    }

    // Parse with the named statement (0 declared param types: PostgreSQL
    // infers them from the $n placeholders), then Sync so any Parse error
    // is reported immediately instead of at the next Execute.
    if (!sendParse(name, sql, 0)) {
        m_lastError = "Failed to send Parse message";
        return false;
    }
    if (!sendMessage('S', {})) {
        m_lastError = "Failed to send Sync message";
        return false;
    }

    auto res = std::make_shared<PgResult>();
    if (!readResultLoop(res)) {
        m_lastError = res->errorMessage();
        return false;
    }
    if (!res->isOk()) {
        // Duplicate name (42P05) and any other Parse error surface here;
        // the name is NOT registered, so a later retry can succeed.
        m_lastError = res->errorMessage();
        return false;
    }

    m_prepared.insert(name);
    return true;
}

std::shared_ptr<PgResult> PgConnection::execPrepared(const std::string& name, const std::vector<std::string>& params) {
    auto res = std::make_shared<PgResult>();
    if (!isConnected()) {
        res->setOk(false);
        res->setErrorMessage("Not connected to PostgreSQL");
        m_lastError = "Not connected to PostgreSQL";
        return res;
    }
    if (!hasPrepared(name)) {
        std::string err = "Prepared statement not found: " + name;
        res->setOk(false);
        res->setErrorMessage(err);
        m_lastError = err;
        return res;
    }

    if (!sendBindExecute(name, params)) {
        res->setOk(false);
        res->setErrorMessage("Failed to send Bind message");
        m_lastError = res->errorMessage();
        return res;
    }

    readResultLoop(res);
    if (!res->isOk()) {
        m_lastError = res->errorMessage();
    }
    return res;
}

bool PgConnection::ping() {
    auto res = query("SELECT 1;");
    return res && res->isOk();
}

bool PgConnection::beginTransaction() {
    auto res = query("BEGIN;");
    return res && res->isOk();
}

bool PgConnection::commit() {
    auto res = query("COMMIT;");
    return res && res->isOk();
}

bool PgConnection::rollback() {
    auto res = query("ROLLBACK;");
    return res && res->isOk();
}

} // namespace sp::runtime
