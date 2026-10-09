#include <gtest/gtest.h>
#include "sp/runtime/Crypto.h"
#include "sp/runtime/Password.h"
#include "sp/runtime/Jwt.h"
#include "sp/runtime/Session.h"
#include "sp/runtime/Runtime.h"
#include <chrono>
#include <thread>

using namespace sp::rt::crypto;

// ============================================================================
// 1. Base64Url & TimingSafeEqual Tests
// ============================================================================

TEST(SecurityTest, Base64UrlCodecRFC4648) {
    // Normal base64 might have '+' or '/' and '=' padding
    std::string testStr = "\xFB\xFF\xFE\x00\x01\x02";
    std::string standardB64 = base64Encode(testStr);
    EXPECT_TRUE(standardB64.find('+') != std::string::npos || standardB64.find('/') != std::string::npos || standardB64.find('=') != std::string::npos);

    std::string urlSafe = base64UrlEncode(testStr);
    EXPECT_EQ(urlSafe.find('+'), std::string::npos);
    EXPECT_EQ(urlSafe.find('/'), std::string::npos);
    EXPECT_EQ(urlSafe.find('='), std::string::npos);

    // Roundtrip decoding
    std::string roundtrip = base64UrlDecode(urlSafe);
    EXPECT_EQ(roundtrip, testStr);

    // Standard string test
    std::string text = "SP-Language-RFC7519-JWT-Safe";
    std::string encodedText = base64UrlEncode(text);
    EXPECT_EQ(base64UrlDecode(encodedText), text);
}

TEST(SecurityTest, TimingSafeEqualPreventsSideChannels) {
    std::string a = "super_secret_token_1234567890";
    std::string b = "super_secret_token_1234567890";
    std::string c = "super_secret_token_1234567891"; // differs at end
    std::string d = "xuper_secret_token_1234567890"; // differs at start
    std::string e = "short";

    EXPECT_TRUE(timingSafeEqual(a, b));
    EXPECT_FALSE(timingSafeEqual(a, c));
    EXPECT_FALSE(timingSafeEqual(a, d));
    EXPECT_FALSE(timingSafeEqual(a, e));
    EXPECT_TRUE(sp_rt_crypto_timing_safe_equal(a.c_str(), b.c_str()));
    EXPECT_FALSE(sp_rt_crypto_timing_safe_equal(a.c_str(), c.c_str()));
}

// ============================================================================
// 2. Argon2id Password Hashing Tests (RFC 9106)
// ============================================================================

TEST(SecurityTest, Argon2idPasswordHashAndVerify) {
    std::string password = "CorrectHorseBatteryStaple!";

    PasswordOptions opts;
    opts.algorithm = PasswordAlgorithm::Argon2id;
    opts.memoryKiB = 1024; // 1 MB for fast test execution
    opts.iterations = 2;
    opts.parallelism = 1;
    opts.hashLength = 32;

    std::string hash = Password::hash(password, opts);

    // Verify PHC string structure
    EXPECT_TRUE(hash.rfind("$argon2id$v=19$m=1024,t=2,p=1$", 0) == 0);

    // Positive verification
    EXPECT_TRUE(Password::verify(password, hash));

    // Negative verification
    EXPECT_FALSE(Password::verify("WrongPassword123", hash));
    EXPECT_FALSE(Password::verify("CorrectHorseBatteryStaple", hash)); // missing '!'

    // Rehash check
    PasswordOptions targetHigherOpts;
    targetHigherOpts.algorithm = PasswordAlgorithm::Argon2id;
    targetHigherOpts.memoryKiB = 2048;
    EXPECT_TRUE(Password::needsRehash(hash, targetHigherOpts));
}

// ============================================================================
// 3. Bcrypt Password Hashing Tests
// ============================================================================

TEST(SecurityTest, BcryptPasswordHashAndVerify) {
    std::string password = "EnterpriseSecurePassword#2026";

    PasswordOptions opts;
    opts.algorithm = PasswordAlgorithm::Bcrypt;
    opts.bcryptCost = 4; // minimum valid cost for ultra-fast test execution

    std::string hash = Password::hash(password, opts);

    // Verify MCF string structure ($2a$04$...)
    EXPECT_TRUE(hash.rfind("$2a$04$", 0) == 0);
    EXPECT_EQ(hash.size(), 60u);

    // Positive verification
    EXPECT_TRUE(Password::verify(password, hash));

    // Negative verification
    EXPECT_FALSE(Password::verify("EnterpriseWrongPassword", hash));

    // Polymorphic verification: verify automatically distinguishes between Argon2id and Bcrypt
    EXPECT_TRUE(Password::verify(password, hash));
}

// ============================================================================
// 4. JSON Web Tokens (RFC 7519) Tests
// ============================================================================

TEST(SecurityTest, JwtSignAndVerifyClaims) {
    std::string secret = "top-secret-signing-key-for-sp-jwt-engine";

    JwtClaims claims;
    claims.setSubject("user_100293");
    claims.setIssuer("https://auth.splang.org");
    claims.setAudience("https://api.splang.org");
    claims.setString("role", "admin");
    claims.setInt("authLevel", 3);
    claims.setBool("mfa", true);

    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    claims.setIssuedAt(now);
    claims.setExpiration(now + 3600); // 1 hour

    std::string token = Jwt::sign(claims, secret);

    // Verify 3 dot-separated segments
    size_t dot1 = token.find('.');
    size_t dot2 = token.find('.', dot1 + 1);
    EXPECT_NE(dot1, std::string::npos);
    EXPECT_NE(dot2, std::string::npos);
    EXPECT_EQ(token.find('.', dot2 + 1), std::string::npos);

    // Valid verification
    JwtValidationOptions opts;
    opts.expectedIssuer = "https://auth.splang.org";
    opts.expectedAudience = "https://api.splang.org";
    auto res = Jwt::verify(token, secret, opts);

    EXPECT_TRUE(res.valid);
    EXPECT_EQ(res.error, "");
    EXPECT_EQ(res.claims.getSubject(), "user_100293");
    EXPECT_EQ(res.claims.getIssuer(), "https://auth.splang.org");
    EXPECT_EQ(res.claims.getAudience(), "https://api.splang.org");
    EXPECT_EQ(res.claims.getString("role"), "admin");
    EXPECT_EQ(res.claims.getInt("authLevel"), 3);
    EXPECT_TRUE(res.claims.getBool("mfa"));

    // Tampered signature rejected
    std::string tamperedSigToken = token.substr(0, token.size() - 2) + "aa";
    auto tamperedSigRes = Jwt::verify(tamperedSigToken, secret, opts);
    EXPECT_FALSE(tamperedSigRes.valid);
    EXPECT_EQ(tamperedSigRes.error, "Invalid JWT signature");

    // Wrong secret rejected
    auto wrongSecretRes = Jwt::verify(token, "wrong-secret-key", opts);
    EXPECT_FALSE(wrongSecretRes.valid);
    EXPECT_EQ(wrongSecretRes.error, "Invalid JWT signature");

    // Issuer mismatch rejected
    JwtValidationOptions badIssuerOpts = opts;
    badIssuerOpts.expectedIssuer = "https://wrong.issuer.com";
    auto badIssuerRes = Jwt::verify(token, secret, badIssuerOpts);
    EXPECT_FALSE(badIssuerRes.valid);
    EXPECT_EQ(badIssuerRes.error, "Issuer mismatch");

    // Audience mismatch rejected
    JwtValidationOptions badAudOpts = opts;
    badAudOpts.expectedAudience = "https://wrong.audience.com";
    auto badAudRes = Jwt::verify(token, secret, badAudOpts);
    EXPECT_FALSE(badAudRes.valid);
    EXPECT_EQ(badAudRes.error, "Audience mismatch");
}

TEST(SecurityTest, JwtExpirationAndNotBefore) {
    std::string secret = "jwt-expiry-test-secret";
    int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();

    // Expired token
    JwtClaims expiredClaims;
    expiredClaims.setSubject("user_expired");
    expiredClaims.setExpiration(now - 10); // expired 10 seconds ago
    std::string expiredToken = Jwt::sign(expiredClaims, secret);

    auto expiredRes = Jwt::verify(expiredToken, secret);
    EXPECT_FALSE(expiredRes.valid);
    EXPECT_EQ(expiredRes.error, "Token expired");

    // Token not yet active (nbf)
    JwtClaims futureClaims;
    futureClaims.setSubject("user_future");
    futureClaims.setNotBefore(now + 100); // valid in 100 seconds
    std::string futureToken = Jwt::sign(futureClaims, secret);

    auto futureRes = Jwt::verify(futureToken, secret);
    EXPECT_FALSE(futureRes.valid);
    EXPECT_EQ(futureRes.error, "Token not yet active (nbf)");

    // Unverified inspection
    auto unverified = Jwt::decodeUnverified(futureToken);
    EXPECT_EQ(unverified.getSubject(), "user_future");
}

// ============================================================================
// 5. Signed Cookies & Serialization Tests
// ============================================================================

TEST(SecurityTest, SignedCookiesAndVerification) {
    std::string secret = "cookie-signing-secret-key-32bytes!";
    std::string rawSession = "session_xyz_777888";

    std::string signedCookie = Cookie::signValue(rawSession, secret);
    EXPECT_TRUE(signedCookie.rfind("s:session_xyz_777888.", 0) == 0);

    // Positive unsign
    std::string unsignedVal = Cookie::unsignValue(signedCookie, secret);
    EXPECT_EQ(unsignedVal, rawSession);

    // Wrong secret yields empty string
    EXPECT_EQ(Cookie::unsignValue(signedCookie, "wrong-secret"), "");

    // Tampered payload yields empty string
    std::string tamperedCookie = "s:session_xyz_999999." + signedCookie.substr(signedCookie.rfind('.') + 1);
    EXPECT_EQ(Cookie::unsignValue(tamperedCookie, secret), "");
}

TEST(SecurityTest, CookieHeaderSerialization) {
    Cookie c;
    c.name = "token";
    c.value = "abc123xyz";
    c.path = "/api/v1";
    c.domain = "example.com";
    c.maxAge = 7200;
    c.httpOnly = true;
    c.secure = true;
    c.sameSite = "Strict";

    std::string serialized = c.serialize();
    EXPECT_EQ(serialized, "token=abc123xyz; Path=/api/v1; Domain=example.com; Max-Age=7200; HttpOnly; Secure; SameSite=Strict");
}

// ============================================================================
// 6. SessionStore Tests & Session Fixation Protection
// ============================================================================

TEST(SecurityTest, SessionStoreCrudAndRegeneration) {
    SessionStore store(3600);

    std::string initData = "{\"userId\":42,\"role\":\"editor\"}";
    std::string sid = store.create(initData);
    EXPECT_FALSE(sid.empty());

    // Retrieve
    EXPECT_EQ(store.get(sid), initData);

    // Update
    std::string updatedData = "{\"userId\":42,\"role\":\"admin\"}";
    EXPECT_TRUE(store.set(sid, updatedData));
    EXPECT_EQ(store.get(sid), updatedData);

    // Regenerate (prevents session fixation attacks)
    std::string newSid = store.regenerate(sid);
    EXPECT_NE(newSid, sid);
    EXPECT_FALSE(newSid.empty());

    // Old session must be invalid
    EXPECT_EQ(store.get(sid), "");

    // New session retains data
    EXPECT_EQ(store.get(newSid), updatedData);

    // Destroy
    EXPECT_TRUE(store.destroy(newSid));
    EXPECT_EQ(store.get(newSid), "");
}

// ============================================================================
// 7. C-ABI Security Runtime Bridges
// ============================================================================

TEST(SecurityTest, CAbiSecurityBridges) {
    // Password Bridge
    const char* pHash = sp_rt_password_hash("BridgePassword123", "argon2id", 1024, 2, 1);
    ASSERT_NE(pHash, nullptr);
    EXPECT_TRUE(sp_rt_password_verify("BridgePassword123", pHash));
    EXPECT_FALSE(sp_rt_password_verify("BridgePasswordWrong", pHash));

    // Bcrypt Bridge
    const char* bHash = sp_rt_password_hash("BridgeBcryptPass", "bcrypt", 4, 0, 0);
    ASSERT_NE(bHash, nullptr);
    EXPECT_TRUE(sp_rt_password_verify("BridgeBcryptPass", bHash));
    EXPECT_FALSE(sp_rt_password_verify("BridgeBcryptWrong", bHash));

    // JWT Bridge
    const char* jwtToken = sp_rt_jwt_sign("{\"sub\":\"bridge_user\",\"role\":\"developer\"}", "bridgeSecret", "HS256", 3600, "splang", "api");
    ASSERT_NE(jwtToken, nullptr);

    void* verifyHandle = sp_rt_jwt_verify(jwtToken, "bridgeSecret", "splang", "api");
    ASSERT_NE(verifyHandle, nullptr);
    EXPECT_TRUE(sp_rt_jwt_res_is_valid(verifyHandle));
    EXPECT_STREQ(sp_rt_jwt_res_claim(verifyHandle, "sub"), "bridge_user");
    EXPECT_STREQ(sp_rt_jwt_res_claim(verifyHandle, "role"), "developer");
    sp_rt_jwt_res_free(verifyHandle);

    // Cookie Bridge
    const char* signedCookie = sp_rt_cookie_sign("test_cookie_val", "cookieSecret");
    ASSERT_NE(signedCookie, nullptr);
    const char* unsignedCookie = sp_rt_cookie_unsign(signedCookie, "cookieSecret");
    EXPECT_STREQ(unsignedCookie, "test_cookie_val");
    const char* badCookie = sp_rt_cookie_unsign(signedCookie, "wrongSecret");
    EXPECT_STREQ(badCookie, "");

    // Session Bridge
    void* sStore = sp_rt_session_store_create(1800);
    ASSERT_NE(sStore, nullptr);
    const char* sid = sp_rt_session_create(sStore, "{\"auth\":true}", 1800);
    EXPECT_STREQ(sp_rt_session_get(sStore, sid), "{\"auth\":true}");
    const char* newSid = sp_rt_session_regenerate(sStore, sid, 1800);
    EXPECT_STREQ(sp_rt_session_get(sStore, sid), "");
    EXPECT_STREQ(sp_rt_session_get(sStore, newSid), "{\"auth\":true}");
    sp_rt_session_destroy(sStore, newSid);
    sp_rt_session_store_destroy(sStore);
}
