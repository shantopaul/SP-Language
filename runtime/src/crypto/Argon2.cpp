#include "sp/runtime/Password.h"
#include "sp/runtime/Crypto.h"
#include <cstring>
#include <vector>
#include <array>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

namespace sp::rt::crypto {

namespace {

// ============================================================================
// BLAKE2b Implementation (RFC 7693 & RFC 9106)
// ============================================================================

static const uint64_t BLAKE2B_IV[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

static const uint8_t BLAKE2B_SIGMA[12][16] = {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 },
    { 11,  8, 12,  0,  5,  2, 15, 13, 10, 14,  3,  6,  7,  1,  9,  4 },
    {  7,  9,  3,  1, 13, 12, 11, 14,  2,  6,  5, 10,  4,  0, 15,  8 },
    {  9,  0,  5,  7,  2,  4, 10, 15, 14,  1, 11, 12,  6,  8,  3, 13 },
    {  2, 12,  6, 10,  0, 11,  8,  3,  4, 13,  7,  5, 15, 14,  1,  9 },
    { 12,  5,  1, 15, 14, 13,  4, 10,  0,  7,  6,  3,  9,  2,  8, 11 },
    { 13, 11,  7, 14, 12,  1,  3,  9,  5,  0, 15,  4,  8,  6,  2, 10 },
    {  6, 15, 14,  9, 11,  3,  0,  8, 12,  2, 13,  7,  1,  4, 10,  5 },
    { 10,  2,  8,  4,  7,  6,  1,  5, 15, 11,  9, 14,  3, 12, 13,  0 },
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15 },
    { 14, 10,  4,  8,  9, 15, 13,  6,  1, 12,  0,  2, 11,  7,  5,  3 }
};

inline uint64_t rotr64(uint64_t w, unsigned c) {
    return (w >> c) | (w << (64 - c));
}

inline void blake2bG(uint64_t v[16], int a, int b, int c, int d, uint64_t x, uint64_t y) {
    v[a] = v[a] + v[b] + x;
    v[d] = rotr64(v[d] ^ v[a], 32);
    v[c] = v[c] + v[d];
    v[b] = rotr64(v[b] ^ v[c], 24);
    v[a] = v[a] + v[b] + y;
    v[d] = rotr64(v[d] ^ v[a], 16);
    v[c] = v[c] + v[d];
    v[b] = rotr64(v[b] ^ v[c], 63);
}

void blake2bCompress(uint64_t h[8], const uint8_t block[128], uint64_t t0, uint64_t t1, bool last) {
    uint64_t v[16];
    uint64_t m[16];

    for (int i = 0; i < 8; ++i) {
        v[i] = h[i];
        v[i + 8] = BLAKE2B_IV[i];
    }

    v[12] ^= t0;
    v[13] ^= t1;
    if (last) {
        v[14] = ~v[14];
    }

    for (int i = 0; i < 16; ++i) {
        m[i] = (static_cast<uint64_t>(block[i * 8 + 0])) |
               (static_cast<uint64_t>(block[i * 8 + 1]) << 8) |
               (static_cast<uint64_t>(block[i * 8 + 2]) << 16) |
               (static_cast<uint64_t>(block[i * 8 + 3]) << 24) |
               (static_cast<uint64_t>(block[i * 8 + 4]) << 32) |
               (static_cast<uint64_t>(block[i * 8 + 5]) << 40) |
               (static_cast<uint64_t>(block[i * 8 + 6]) << 48) |
               (static_cast<uint64_t>(block[i * 8 + 7]) << 56);
    }

    for (int r = 0; r < 12; ++r) {
        blake2bG(v, 0, 4,  8, 12, m[BLAKE2B_SIGMA[r][0]], m[BLAKE2B_SIGMA[r][1]]);
        blake2bG(v, 1, 5,  9, 13, m[BLAKE2B_SIGMA[r][2]], m[BLAKE2B_SIGMA[r][3]]);
        blake2bG(v, 2, 6, 10, 14, m[BLAKE2B_SIGMA[r][4]], m[BLAKE2B_SIGMA[r][5]]);
        blake2bG(v, 3, 7, 11, 15, m[BLAKE2B_SIGMA[r][6]], m[BLAKE2B_SIGMA[r][7]]);
        blake2bG(v, 0, 5, 10, 15, m[BLAKE2B_SIGMA[r][8]], m[BLAKE2B_SIGMA[r][9]]);
        blake2bG(v, 1, 6, 11, 12, m[BLAKE2B_SIGMA[r][10]], m[BLAKE2B_SIGMA[r][11]]);
        blake2bG(v, 2, 7,  8, 13, m[BLAKE2B_SIGMA[r][12]], m[BLAKE2B_SIGMA[r][13]]);
        blake2bG(v, 3, 4,  9, 14, m[BLAKE2B_SIGMA[r][14]], m[BLAKE2B_SIGMA[r][15]]);
    }

    for (int i = 0; i < 8; ++i) {
        h[i] ^= v[i] ^ v[i + 8];
    }
}

std::vector<uint8_t> blake2b(size_t outLen, const uint8_t* in, size_t inLen, const uint8_t* key = nullptr, size_t keyLen = 0) {
    if (outLen == 0 || outLen > 64 || keyLen > 64) {
        throw std::invalid_argument("Invalid blake2b parameters");
    }

    uint64_t h[8];
    for (int i = 0; i < 8; ++i) {
        h[i] = BLAKE2B_IV[i];
    }
    h[0] ^= 0x01010000 ^ (static_cast<uint64_t>(keyLen) << 8) ^ outLen;

    uint8_t block[128] = {0};
    size_t blockOffset = 0;
    uint64_t totalBytes = 0;

    if (keyLen > 0) {
        std::memcpy(block, key, keyLen);
        blockOffset = 128;
        totalBytes = 128;
    }

    size_t inOffset = 0;
    while (inOffset < inLen) {
        if (blockOffset == 128) {
            blake2bCompress(h, block, totalBytes, 0, false);
            std::memset(block, 0, 128);
            blockOffset = 0;
        }
        size_t take = std::min<size_t>(128 - blockOffset, inLen - inOffset);
        std::memcpy(block + blockOffset, in + inOffset, take);
        blockOffset += take;
        inOffset += take;
        totalBytes += take;
    }

    blake2bCompress(h, block, totalBytes, 0, true);

    std::vector<uint8_t> out(outLen);
    for (size_t i = 0; i < outLen; ++i) {
        out[i] = static_cast<uint8_t>((h[i / 8] >> ((i % 8) * 8)) & 0xFF);
    }
    return out;
}

// Variable length hash H' (RFC 9106 §3.2.2)
std::vector<uint8_t> argon2HPrime(size_t outLen, const uint8_t* in, size_t inLen) {
    std::vector<uint8_t> prefix(4 + inLen);
    prefix[0] = static_cast<uint8_t>(outLen & 0xFF);
    prefix[1] = static_cast<uint8_t>((outLen >> 8) & 0xFF);
    prefix[2] = static_cast<uint8_t>((outLen >> 16) & 0xFF);
    prefix[3] = static_cast<uint8_t>((outLen >> 24) & 0xFF);
    std::memcpy(prefix.data() + 4, in, inLen);

    if (outLen <= 64) {
        return blake2b(outLen, prefix.data(), prefix.size());
    }

    // outLen > 64
    size_t r = (outLen + 31) / 32 - 2;
    std::vector<uint8_t> out(outLen);

    auto vPrev = blake2b(64, prefix.data(), prefix.size());
    std::memcpy(out.data(), vPrev.data(), 32);

    for (size_t i = 1; i < r; ++i) {
        vPrev = blake2b(64, vPrev.data(), vPrev.size());
        std::memcpy(out.data() + i * 32, vPrev.data(), 32);
    }

    size_t rem = outLen - 32 * r;
    auto vLast = blake2b(rem, vPrev.data(), vPrev.size());
    std::memcpy(out.data() + r * 32, vLast.data(), rem);

    return out;
}

// ============================================================================
// Argon2 Core Compression Function G (RFC 9106 §3.5)
// ============================================================================

struct Argon2Block {
    uint64_t v[128]; // 1024 bytes
};

inline void store64Le(uint8_t* p, uint64_t val) {
    for (int i = 0; i < 8; ++i) {
        p[i] = static_cast<uint8_t>((val >> (i * 8)) & 0xFF);
    }
}

inline uint64_t load64Le(const uint8_t* p) {
    uint64_t val = 0;
    for (int i = 0; i < 8; ++i) {
        val |= (static_cast<uint64_t>(p[i]) << (i * 8));
    }
    return val;
}

inline void argon2G(uint64_t& a, uint64_t& b, uint64_t& c, uint64_t& d) {
    uint64_t prod1 = 2ULL * (a & 0xFFFFFFFFULL) * (b & 0xFFFFFFFFULL);
    a = a + b + prod1;
    d = rotr64(d ^ a, 32);

    uint64_t prod2 = 2ULL * (c & 0xFFFFFFFFULL) * (d & 0xFFFFFFFFULL);
    c = c + d + prod2;
    b = rotr64(b ^ c, 24);

    uint64_t prod3 = 2ULL * (a & 0xFFFFFFFFULL) * (b & 0xFFFFFFFFULL);
    a = a + b + prod3;
    d = rotr64(d ^ a, 16);

    uint64_t prod4 = 2ULL * (c & 0xFFFFFFFFULL) * (d & 0xFFFFFFFFULL);
    c = c + d + prod4;
    b = rotr64(b ^ c, 63);
}

void applyP(uint64_t r[16]) {
    // 8 rows of 2 64-bit words -> 16 uint64_t
    // G on columns
    argon2G(r[0], r[4],  r[8],  r[12]);
    argon2G(r[1], r[5],  r[9],  r[13]);
    argon2G(r[2], r[6],  r[10], r[14]);
    argon2G(r[3], r[7],  r[11], r[15]);

    // G on diagonals
    argon2G(r[0], r[5],  r[10], r[15]);
    argon2G(r[1], r[6],  r[11], r[12]);
    argon2G(r[2], r[7],  r[8],  r[13]);
    argon2G(r[3], r[4],  r[9],  r[14]);
}

void argon2BlockCompress(const Argon2Block& X, const Argon2Block& Y, Argon2Block& Z, bool xorWithExisting) {
    Argon2Block R;
    for (int i = 0; i < 128; ++i) {
        R.v[i] = X.v[i] ^ Y.v[i];
    }

    Argon2Block Q = R;

    // Transform 8 columns
    for (int col = 0; col < 8; ++col) {
        uint64_t r[16];
        for (int row = 0; row < 8; ++row) {
            r[row * 2 + 0] = Q.v[row * 16 + col * 2 + 0];
            r[row * 2 + 1] = Q.v[row * 16 + col * 2 + 1];
        }
        applyP(r);
        for (int row = 0; row < 8; ++row) {
            Q.v[row * 16 + col * 2 + 0] = r[row * 2 + 0];
            Q.v[row * 16 + col * 2 + 1] = r[row * 2 + 1];
        }
    }

    // Transform 8 rows
    for (int row = 0; row < 8; ++row) {
        uint64_t r[16];
        for (int col = 0; col < 16; ++col) {
            r[col] = Q.v[row * 16 + col];
        }
        applyP(r);
        for (int col = 0; col < 16; ++col) {
            Q.v[row * 16 + col] = r[col];
        }
    }

    // Z = Q ^ R
    if (xorWithExisting) {
        for (int i = 0; i < 128; ++i) {
            Z.v[i] ^= Q.v[i] ^ R.v[i];
        }
    } else {
        for (int i = 0; i < 128; ++i) {
            Z.v[i] = Q.v[i] ^ R.v[i];
        }
    }
}

inline void pushLe32(std::vector<uint8_t>& buf, uint32_t val) {
    buf.push_back(static_cast<uint8_t>(val & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 8) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 16) & 0xFF));
    buf.push_back(static_cast<uint8_t>((val >> 24) & 0xFF));
}

} // anonymous namespace

// ============================================================================
// Password::argon2idRaw Implementation (RFC 9106)
// ============================================================================

std::vector<uint8_t> Password::argon2idRaw(
    const std::string& password,
    const std::vector<uint8_t>& salt,
    uint32_t iterations,
    uint32_t memoryKiB,
    uint32_t parallelism,
    uint32_t tagLength
) {
    if (parallelism < 1 || iterations < 1 || tagLength < 4) {
        throw std::invalid_argument("Invalid Argon2 parameters");
    }

    // Memory size m must be at least 8*p KiB
    uint32_t minMemory = 8 * parallelism;
    if (memoryKiB < minMemory) {
        memoryKiB = minMemory;
    }
    // Number of 1024-byte blocks m' = 4 * p * floor(m / (4 * p))
    uint32_t blocksCount = 4 * parallelism * (memoryKiB / (4 * parallelism));
    uint32_t q = blocksCount / parallelism; // columns per lane

    // Initial hash H0 (RFC 9106 §3.2.1)
    std::vector<uint8_t> h0Buf;
    pushLe32(h0Buf, parallelism);
    pushLe32(h0Buf, tagLength);
    pushLe32(h0Buf, memoryKiB);
    pushLe32(h0Buf, iterations);
    pushLe32(h0Buf, 0x13); // version 1.3
    pushLe32(h0Buf, 2);    // Argon2id type = 2

    pushLe32(h0Buf, static_cast<uint32_t>(password.size()));
    h0Buf.insert(h0Buf.end(), password.begin(), password.end());

    pushLe32(h0Buf, static_cast<uint32_t>(salt.size()));
    h0Buf.insert(h0Buf.end(), salt.begin(), salt.end());

    pushLe32(h0Buf, 0); // key length
    pushLe32(h0Buf, 0); // associated data length

    auto h0 = blake2b(64, h0Buf.data(), h0Buf.size());

    // Allocate memory matrix: p lanes, q columns
    std::vector<std::vector<Argon2Block>> B(parallelism, std::vector<Argon2Block>(q));

    // Initialize first two columns B[i][0] and B[i][1]
    for (uint32_t i = 0; i < parallelism; ++i) {
        for (uint32_t col = 0; col < 2; ++col) {
            std::vector<uint8_t> initInput = h0;
            pushLe32(initInput, col);
            pushLe32(initInput, i);
            auto blockBytes = argon2HPrime(1024, initInput.data(), initInput.size());
            for (int k = 0; k < 128; ++k) {
                B[i][col].v[k] = load64Le(blockBytes.data() + k * 8);
            }
        }
    }

    // Helper for generating Argon2i pseudo-random addresses
    auto computeAddressBlock = [&](uint32_t pass, uint32_t lane, uint32_t slice, uint32_t step, Argon2Block& outBlock) {
        Argon2Block inBlock{};
        inBlock.v[0] = pass;
        inBlock.v[1] = lane;
        inBlock.v[2] = slice;
        inBlock.v[3] = blocksCount;
        inBlock.v[4] = iterations;
        inBlock.v[5] = 2; // Argon2id
        inBlock.v[6] = step;

        Argon2Block zeroBlock{};
        Argon2Block tmp{};
        argon2BlockCompress(zeroBlock, inBlock, tmp, false);
        argon2BlockCompress(zeroBlock, tmp, outBlock, false);
    };

    // Iterate through passes
    Argon2Block addressBlock{};
    uint32_t addressIndex = 128; // force calculation on first request
    uint32_t addressStep = 0;

    for (uint32_t r = 0; r < iterations; ++r) {
        for (uint32_t s = 0; s < 4; ++s) {
            uint32_t sliceLength = q / 4;
            for (uint32_t colIndex = 0; colIndex < sliceLength; ++colIndex) {
                uint32_t j = s * sliceLength + colIndex;

                for (uint32_t i = 0; i < parallelism; ++i) {
                    if (r == 0 && j < 2) {
                        continue; // columns 0 and 1 are already initialized
                    }

                    // Reference block determination
                    const Argon2Block& prevBlock = (j == 0) ? B[i][q - 1] : B[i][j - 1];

                    uint64_t J1 = 0, J2 = 0;
                    // Argon2id: First pass, slices 0 and 1 use Argon2i; others use Argon2d
                    if (r == 0 && s < 2) {
                        // Argon2i mode
                        if (addressIndex >= 128) {
                            addressStep++;
                            computeAddressBlock(r, i, s, addressStep, addressBlock);
                            addressIndex = 0;
                        }
                        uint64_t addr = addressBlock.v[addressIndex++];
                        J1 = addr & 0xFFFFFFFFULL;
                        J2 = addr >> 32;
                    } else {
                        // Argon2d mode
                        J1 = prevBlock.v[0] & 0xFFFFFFFFULL;
                        J2 = prevBlock.v[0] >> 32;
                    }

                    // Determine reference lane l
                    uint32_t l = static_cast<uint32_t>(J2 % parallelism);
                    if (r == 0 && s == 0) {
                        l = i;
                    }

                    // Determine reference column z
                    // Set of reference blocks R
                    uint32_t refAreaSize = 0;
                    uint32_t startCol = 0;

                    if (r == 0) {
                        if (l == i) {
                            refAreaSize = j - 1;
                        } else {
                            refAreaSize = s * sliceLength - ((colIndex == 0) ? 1 : 0);
                        }
                    } else {
                        if (l == i) {
                            refAreaSize = q - sliceLength + colIndex - 1;
                        } else {
                            refAreaSize = q - sliceLength + ((colIndex == 0) ? 0 : colIndex - 1);
                        }
                        startCol = ((s + 1) % 4) * sliceLength;
                    }

                    if (refAreaSize == 0) {
                        refAreaSize = 1;
                    }

                    uint64_t x = (J1 * J1) >> 32;
                    uint64_t y = (static_cast<uint64_t>(refAreaSize) * x) >> 32;
                    uint64_t zOffset = static_cast<uint64_t>(refAreaSize) - 1 - y;

                    uint32_t z = (startCol + static_cast<uint32_t>(zOffset)) % q;

                    // Compute B[i][j] = G(prevBlock, B[l][z])
                    argon2BlockCompress(prevBlock, B[l][z], B[i][j], (r > 0));
                }
            }
        }
    }

    // XOR blocks of the last column
    Argon2Block finalBlock = B[0][q - 1];
    for (uint32_t i = 1; i < parallelism; ++i) {
        for (int k = 0; k < 128; ++k) {
            finalBlock.v[k] ^= B[i][q - 1].v[k];
        }
    }

    // Convert finalBlock to bytes
    std::vector<uint8_t> finalBytes(1024);
    for (int k = 0; k < 128; ++k) {
        store64Le(finalBytes.data() + k * 8, finalBlock.v[k]);
    }

    return argon2HPrime(tagLength, finalBytes.data(), finalBytes.size());
}

} // namespace sp::rt::crypto
