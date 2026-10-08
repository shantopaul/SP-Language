#include <gtest/gtest.h>
#include "sp/runtime/Crypto.h"
#include <regex>

using namespace sp::rt::crypto;

TEST(CryptoTest, Sha256StandardVectors) {
    // NIST Test Vector 1: Empty string
    EXPECT_EQ(sha256(""), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");

    // NIST Test Vector 2: "abc"
    EXPECT_EQ(sha256("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");

    // NIST Test Vector 3: 56-byte message
    std::string msg = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    EXPECT_EQ(sha256(msg), "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

TEST(CryptoTest, HmacSha256Vector) {
    std::string key = "key";
    std::string data = "The quick brown fox jumps over the lazy dog";
    std::string expected = "f7bc83f430538424b13298e6aa6fb143ef4d59a14946175997479dbc2d1a3cd8";

    EXPECT_EQ(hmacSha256(key, data), expected);
}

TEST(CryptoTest, Base64EncodeAndDecodeRoundtrip) {
    std::string original = "Hello, SP Cloud & Native Applications!";
    std::string encoded = base64Encode(original);
    std::string decoded = base64Decode(encoded);

    EXPECT_EQ(decoded, original);
    EXPECT_EQ(base64Encode("Hello, World!"), "SGVsbG8sIFdvcmxkIQ==");
    EXPECT_EQ(base64Decode("SGVsbG8sIFdvcmxkIQ=="), "Hello, World!");
}

TEST(CryptoTest, SecureRandomHexAndUuid) {
    std::string hex16 = randomHex(16);
    EXPECT_EQ(hex16.size(), 32u);

    std::string hex16_b = randomHex(16);
    EXPECT_EQ(hex16_b.size(), 32u);
    EXPECT_NE(hex16, hex16_b); // Entropy assertion

    std::string uuid = randomUuid();
    EXPECT_EQ(uuid.size(), 36u);
    EXPECT_EQ(uuid[14], '4'); // Version 4
    char variant = uuid[19];
    EXPECT_TRUE(variant == '8' || variant == '9' || variant == 'a' || variant == 'b');

    std::regex uuidRegex("^[0-9a-f]{8}-[0-9a-f]{4}-4[0-9a-f]{3}-[89ab][0-9a-f]{3}-[0-9a-f]{12}$");
    EXPECT_TRUE(std::regex_match(uuid, uuidRegex));
}
