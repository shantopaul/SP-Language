#include "sp/runtime/Crypto.h"
#include <array>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>

#if defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#else
#include <fstream>
#endif

namespace sp::rt::crypto {

namespace {

// ============================================================================
// SHA-256 Implementation (FIPS 180-4)
// ============================================================================

inline uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline uint32_t sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

inline uint32_t sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

inline uint32_t gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

inline uint32_t gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static const uint32_t K256[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
    0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
    0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
    0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
    0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
    0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
    0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
    0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

std::array<uint8_t, 32> computeRawSha256(const uint8_t* data, size_t length) {
    uint32_t h[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    uint64_t bitLen = static_cast<uint64_t>(length) * 8;
    std::vector<uint8_t> msg(data, data + length);
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
        }
        for (int i = 16; i < 64; ++i) {
            w[i] = gamma1(w[i - 2]) + w[i - 7] + gamma0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = h[0], b = h[1], c = h[2], d = h[3];
        uint32_t e = h[4], f = h[5], g = h[6], hVal = h[7];

        for (int i = 0; i < 64; ++i) {
            uint32_t t1 = hVal + sigma1(e) + ch(e, f, g) + K256[i] + w[i];
            uint32_t t2 = sigma0(a) + maj(a, b, c);
            hVal = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        h[0] += a; h[1] += b; h[2] += c; h[3] += d;
        h[4] += e; h[5] += f; h[6] += g; h[7] += hVal;
    }

    std::array<uint8_t, 32> digest{};
    for (int i = 0; i < 8; ++i) {
        digest[i * 4 + 0] = static_cast<uint8_t>((h[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xFF);
    }
    return digest;
}

inline uint32_t rol(uint32_t value, size_t bits) {
    return (value << bits) | (value >> (32 - bits));
}

std::array<uint8_t, 20> computeRawSha1(const uint8_t* data, size_t length) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    uint64_t bitLen = static_cast<uint64_t>(length) * 8;
    std::vector<uint8_t> msg(data, data + length);
    msg.push_back(0x80);
    while ((msg.size() % 64) != 56) {
        msg.push_back(0x00);
    }
    for (int i = 7; i >= 0; --i) {
        msg.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                   (static_cast<uint32_t>(msg[chunk + i * 4 + 3]));
        }
        for (int i = 16; i < 80; ++i) {
            w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f = 0;
            uint32_t k = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }

            uint32_t temp = rol(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rol(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::array<uint8_t, 20> digest{};
    uint32_t h[5] = {h0, h1, h2, h3, h4};
    for (int i = 0; i < 5; ++i) {
        digest[i * 4 + 0] = static_cast<uint8_t>((h[i] >> 24) & 0xFF);
        digest[i * 4 + 1] = static_cast<uint8_t>((h[i] >> 16) & 0xFF);
        digest[i * 4 + 2] = static_cast<uint8_t>((h[i] >> 8) & 0xFF);
        digest[i * 4 + 3] = static_cast<uint8_t>(h[i] & 0xFF);
    }
    return digest;
}

std::string bytesToHex(const uint8_t* data, size_t length) {
    static const char hexChars[] = "0123456789abcdef";
    std::string hex;
    hex.reserve(length * 2);
    for (size_t i = 0; i < length; ++i) {
        hex.push_back(hexChars[(data[i] >> 4) & 0x0F]);
        hex.push_back(hexChars[data[i] & 0x0F]);
    }
    return hex;
}

} // anonymous namespace

std::string sha256Bytes(const uint8_t* data, size_t length) {
    auto digest = computeRawSha256(data, length);
    return bytesToHex(digest.data(), digest.size());
}

std::string sha256(const std::string& input) {
    return sha256Bytes(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

std::string sha1(const std::string& input) {
    auto digest = computeRawSha1(reinterpret_cast<const uint8_t*>(input.data()), input.size());
    return bytesToHex(digest.data(), digest.size());
}

std::vector<uint8_t> sha1RawBytes(const uint8_t* data, size_t length) {
    auto digest = computeRawSha1(data, length);
    return std::vector<uint8_t>(digest.begin(), digest.end());
}

// ============================================================================
// HMAC-SHA256 (RFC 2104)
// ============================================================================

std::string hmacSha256Bytes(const uint8_t* key, size_t keyLen, const uint8_t* msg, size_t msgLen) {
    constexpr size_t blockSize = 64;
    std::array<uint8_t, blockSize> kPad{};

    if (keyLen > blockSize) {
        auto keyHash = computeRawSha256(key, keyLen);
        std::memcpy(kPad.data(), keyHash.data(), keyHash.size());
    } else {
        std::memcpy(kPad.data(), key, keyLen);
    }

    std::vector<uint8_t> inner(blockSize + msgLen);
    for (size_t i = 0; i < blockSize; ++i) {
        inner[i] = kPad[i] ^ 0x36;
    }
    std::memcpy(inner.data() + blockSize, msg, msgLen);

    auto innerHash = computeRawSha256(inner.data(), inner.size());

    std::vector<uint8_t> outer(blockSize + 32);
    for (size_t i = 0; i < blockSize; ++i) {
        outer[i] = kPad[i] ^ 0x5c;
    }
    std::memcpy(outer.data() + blockSize, innerHash.data(), 32);

    auto finalDigest = computeRawSha256(outer.data(), outer.size());
    return bytesToHex(finalDigest.data(), finalDigest.size());
}

std::string hmacSha256(const std::string& key, const std::string& message) {
    return hmacSha256Bytes(
        reinterpret_cast<const uint8_t*>(key.data()), key.size(),
        reinterpret_cast<const uint8_t*>(message.data()), message.size()
    );
}

// ============================================================================
// Base64 Implementation
// ============================================================================

static const char b64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64EncodeBytes(const uint8_t* data, size_t length) {
    std::string ret;
    ret.reserve(((length + 2) / 3) * 4);

    size_t i = 0;
    while (i < length) {
        size_t remaining = length - i;
        uint32_t octet_a = data[i++];
        uint32_t octet_b = (remaining > 1) ? data[i++] : 0;
        uint32_t octet_c = (remaining > 2) ? data[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        ret.push_back(b64Chars[(triple >> 18) & 0x3F]);
        ret.push_back(b64Chars[(triple >> 12) & 0x3F]);
        ret.push_back(remaining > 1 ? b64Chars[(triple >> 6) & 0x3F] : '=');
        ret.push_back(remaining > 2 ? b64Chars[triple & 0x3F] : '=');
    }
    return ret;
}

std::string base64Encode(const std::string& input) {
    return base64EncodeBytes(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

std::string base64Decode(const std::string& input) {
    std::string out;
    std::vector<int> table(256, -1);
    for (int i = 0; i < 64; ++i) {
        table[static_cast<uint8_t>(b64Chars[i])] = i;
    }

    uint32_t val = 0;
    int valb = -8;
    for (uint8_t c : input) {
        if (c == '=') break;
        if (table[c] == -1) continue;
        val = (val << 6) | table[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// ============================================================================
// Base64Url Implementation (RFC 4648 §5, URL-safe and unpadded for JWT)
// ============================================================================

std::string base64UrlEncodeBytes(const uint8_t* data, size_t length) {
    std::string b64 = base64EncodeBytes(data, length);
    std::string urlSafe;
    urlSafe.reserve(b64.size());
    for (char c : b64) {
        if (c == '+') {
            urlSafe.push_back('-');
        } else if (c == '/') {
            urlSafe.push_back('_');
        } else if (c != '=') {
            urlSafe.push_back(c);
        }
    }
    return urlSafe;
}

std::string base64UrlEncode(const std::string& input) {
    return base64UrlEncodeBytes(reinterpret_cast<const uint8_t*>(input.data()), input.size());
}

std::string base64UrlDecode(const std::string& input) {
    std::string b64;
    b64.reserve(input.size() + 4);
    for (char c : input) {
        if (c == '-') {
            b64.push_back('+');
        } else if (c == '_') {
            b64.push_back('/');
        } else {
            b64.push_back(c);
        }
    }
    size_t rem = b64.size() % 4;
    if (rem == 2) {
        b64.append("==");
    } else if (rem == 3) {
        b64.append("=");
    }
    return base64Decode(b64);
}

// ============================================================================
// Constant-Time Equality (Protects against timing attacks)
// ============================================================================

bool timingSafeEqualBytes(const uint8_t* a, size_t aLen, const uint8_t* b, size_t bLen) {
    if (aLen != bLen) {
        return false;
    }
    uint8_t result = 0;
    for (size_t i = 0; i < aLen; ++i) {
        result |= (a[i] ^ b[i]);
    }
    return result == 0;
}

bool timingSafeEqual(const std::string& a, const std::string& b) {
    return timingSafeEqualBytes(
        reinterpret_cast<const uint8_t*>(a.data()), a.size(),
        reinterpret_cast<const uint8_t*>(b.data()), b.size()
    );
}

// ============================================================================
// Secure Random Bytes & UUID v4
// ============================================================================

std::vector<uint8_t> randomBytes(size_t count) {
    std::vector<uint8_t> buf(count);
    if (count == 0) return buf;

    bool filled = false;
#if defined(_WIN32)
    HCRYPTPROV hCryptProv = 0;
    if (CryptAcquireContext(&hCryptProv, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptGenRandom(hCryptProv, static_cast<DWORD>(count), buf.data())) {
            filled = true;
        }
        CryptReleaseContext(hCryptProv, 0);
    }
#else
    std::ifstream urandom("/dev/urandom", std::ios::in | std::ios::binary);
    if (urandom) {
        urandom.read(reinterpret_cast<char*>(buf.data()), count);
        if (urandom.gcount() == static_cast<std::streamsize>(count)) {
            filled = true;
        }
    }
#endif

    if (!filled) {
        // Fallback to std::random_device
        std::random_device rd;
        for (size_t i = 0; i < count; ++i) {
            buf[i] = static_cast<uint8_t>(rd() & 0xFF);
        }
    }

    return buf;
}

std::string randomHex(size_t byteCount) {
    auto bytes = randomBytes(byteCount);
    return bytesToHex(bytes.data(), bytes.size());
}

std::string randomUuid() {
    auto bytes = randomBytes(16);
    // Version 4
    bytes[6] = (bytes[6] & 0x0F) | 0x40;
    // Variant 1
    bytes[8] = (bytes[8] & 0x3F) | 0x80;

    std::string hex = bytesToHex(bytes.data(), 16);
    // Format: 8-4-4-4-12
    return hex.substr(0, 8) + "-" +
           hex.substr(8, 4) + "-" +
           hex.substr(12, 4) + "-" +
           hex.substr(16, 4) + "-" +
           hex.substr(20, 12);
}

} // namespace sp::rt::crypto
