#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace sp::rt::crypto {

// SHA-256
std::string sha256(const std::string& input);
std::string sha256Bytes(const uint8_t* data, size_t length);

// HMAC-SHA256
std::string hmacSha256(const std::string& key, const std::string& message);
std::string hmacSha256Bytes(const uint8_t* key, size_t keyLen, const uint8_t* msg, size_t msgLen);

// Base64
std::string base64Encode(const std::string& input);
std::string base64EncodeBytes(const uint8_t* data, size_t length);
std::string base64Decode(const std::string& input);

// Cryptographically secure random numbers & tokens
std::vector<uint8_t> randomBytes(size_t count);
std::string randomHex(size_t byteCount);
std::string randomUuid();

} // namespace sp::rt::crypto
