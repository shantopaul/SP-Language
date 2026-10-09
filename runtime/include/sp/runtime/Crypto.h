#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace sp::rt::crypto {

// SHA-256
std::string sha256(const std::string& input);
std::string sha256Bytes(const uint8_t* data, size_t length);

// SHA-1 (RFC 3174, required for RFC 6455 WebSocket Handshake)
std::string sha1(const std::string& input);
std::vector<uint8_t> sha1RawBytes(const uint8_t* data, size_t length);

// HMAC-SHA256
std::string hmacSha256(const std::string& key, const std::string& message);
std::string hmacSha256Bytes(const uint8_t* key, size_t keyLen, const uint8_t* msg, size_t msgLen);

// Base64
std::string base64Encode(const std::string& input);
std::string base64EncodeBytes(const uint8_t* data, size_t length);
std::string base64Decode(const std::string& input);

// Base64Url (RFC 4648 §5, URL-safe and unpadded for JWTs)
std::string base64UrlEncode(const std::string& input);
std::string base64UrlEncodeBytes(const uint8_t* data, size_t length);
std::string base64UrlDecode(const std::string& input);

// Constant-Time Equality (Protects against timing attacks)
bool timingSafeEqual(const std::string& a, const std::string& b);
bool timingSafeEqualBytes(const uint8_t* a, size_t aLen, const uint8_t* b, size_t bLen);

// Cryptographically secure random numbers & tokens
std::vector<uint8_t> randomBytes(size_t count);
std::string randomHex(size_t byteCount);
std::string randomUuid();

} // namespace sp::rt::crypto
