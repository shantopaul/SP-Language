#include "sp/runtime/Jwt.h"
#include "sp/runtime/Crypto.h"
#include <chrono>
#include <sstream>
#include <stdexcept>
#include <iomanip>

namespace sp::rt::crypto {

namespace {

std::vector<uint8_t> hexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;
    bytes.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        uint8_t b = static_cast<uint8_t>(std::stoul(hex.substr(i, 2), nullptr, 16));
        bytes.push_back(b);
    }
    return bytes;
}

std::vector<std::string> split(const std::string& s, char delim) {
    std::vector<std::string> elems;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

int64_t currentEpochSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

} // anonymous namespace

// ============================================================================
// JwtClaims Implementation
// ============================================================================

JwtClaims::JwtClaims() {
    m_json = json::JsonValue::makeObject();
}

JwtClaims::JwtClaims(json::JsonValuePtr json, std::shared_ptr<std::string> rawJson) {
    if (json && json->isObject()) {
        m_json = json;
    } else {
        m_json = json::JsonValue::makeObject();
    }
    m_rawJson = rawJson;
}

void JwtClaims::setIssuer(const std::string& iss) {
    setString("iss", iss);
}

std::string JwtClaims::getIssuer() const {
    return getString("iss");
}

void JwtClaims::setSubject(const std::string& sub) {
    setString("sub", sub);
}

std::string JwtClaims::getSubject() const {
    return getString("sub");
}

void JwtClaims::setAudience(const std::string& aud) {
    setString("aud", aud);
}

std::string JwtClaims::getAudience() const {
    return getString("aud");
}

void JwtClaims::setExpiration(int64_t exp) {
    setInt("exp", exp);
}

int64_t JwtClaims::getExpiration() const {
    return getInt("exp");
}

void JwtClaims::setNotBefore(int64_t nbf) {
    setInt("nbf", nbf);
}

int64_t JwtClaims::getNotBefore() const {
    return getInt("nbf");
}

void JwtClaims::setIssuedAt(int64_t iat) {
    setInt("iat", iat);
}

int64_t JwtClaims::getIssuedAt() const {
    return getInt("iat");
}

void JwtClaims::setJwtId(const std::string& jti) {
    setString("jti", jti);
}

std::string JwtClaims::getJwtId() const {
    return getString("jti");
}

void JwtClaims::setString(const std::string& key, const std::string& val) {
    m_json->set(key, json::JsonValue::makeString(val));
}

std::string JwtClaims::getString(const std::string& key, const std::string& defaultVal) const {
    if (m_json && m_json->has(key)) {
        auto val = m_json->get(key);
        if (val) {
            return std::string(val->asString());
        }
    }
    return defaultVal;
}

void JwtClaims::setInt(const std::string& key, int64_t val) {
    m_json->set(key, json::JsonValue::makeInt(val));
}

int64_t JwtClaims::getInt(const std::string& key, int64_t defaultVal) const {
    if (m_json && m_json->has(key)) {
        auto val = m_json->get(key);
        if (val) {
            return val->asInt();
        }
    }
    return defaultVal;
}

void JwtClaims::setBool(const std::string& key, bool val) {
    m_json->set(key, json::JsonValue::makeBool(val));
}

bool JwtClaims::getBool(const std::string& key, bool defaultVal) const {
    if (m_json && m_json->has(key)) {
        auto val = m_json->get(key);
        if (val) {
            return val->asBool();
        }
    }
    return defaultVal;
}

bool JwtClaims::has(const std::string& key) const {
    return m_json && m_json->has(key);
}

std::string JwtClaims::toJsonString() const {
    if (!m_json) return "{}";
    return m_json->stringify(0);
}

// ============================================================================
// Jwt Signing & Verification
// ============================================================================

std::string Jwt::signJson(const std::string& payloadJson, const std::string& secret, JwtAlgorithm alg) {
    (void)alg;
    std::string headerJson = "{\"alg\":\"HS256\",\"typ\":\"JWT\"}";
    std::string headerB64 = base64UrlEncode(headerJson);
    std::string payloadB64 = base64UrlEncode(payloadJson);
    std::string signingInput = headerB64 + "." + payloadB64;

    std::string hexSig = hmacSha256(secret, signingInput);
    auto rawSigBytes = hexToBytes(hexSig);
    std::string sigB64 = base64UrlEncodeBytes(rawSigBytes.data(), rawSigBytes.size());

    return signingInput + "." + sigB64;
}

std::string Jwt::sign(const JwtClaims& claims, const std::string& secret, JwtAlgorithm alg) {
    return signJson(claims.toJsonString(), secret, alg);
}

JwtValidationResult Jwt::verify(const std::string& token, const std::string& secret, const JwtValidationOptions& options) {
    JwtValidationResult res;

    auto parts = split(token, '.');
    if (parts.size() != 3) {
        res.valid = false;
        res.error = "Malformed JWT: expected 3 segments";
        return res;
    }

    const std::string& headerB64 = parts[0];
    const std::string& payloadB64 = parts[1];
    const std::string& sigB64 = parts[2];

    // Verify signature
    std::string signingInput = headerB64 + "." + payloadB64;
    std::string hexSig = hmacSha256(secret, signingInput);
    auto rawSigBytes = hexToBytes(hexSig);
    std::string expectedSigB64 = base64UrlEncodeBytes(rawSigBytes.data(), rawSigBytes.size());

    if (!timingSafeEqual(sigB64, expectedSigB64)) {
        res.valid = false;
        res.error = "Invalid JWT signature";
        return res;
    }

    // Decode header
    std::string headerJsonStr = base64UrlDecode(headerB64);
    std::string err;
    auto headerJson = json::parse(headerJsonStr, &err);
    if (!headerJson || !headerJson->isObject()) {
        res.valid = false;
        res.error = "Malformed JWT header";
        return res;
    }

    if (headerJson->has("alg")) {
        res.algorithm = std::string(headerJson->get("alg")->asStringView());
    }
    if (res.algorithm != "HS256") {
        res.valid = false;
        res.error = "Unsupported JWT algorithm: " + res.algorithm;
        return res;
    }

    // Decode payload
    auto payloadStrPtr = std::make_shared<std::string>(base64UrlDecode(payloadB64));
    auto payloadJson = json::parse(*payloadStrPtr, &err);
    if (!payloadJson || !payloadJson->isObject()) {
        res.valid = false;
        res.error = "Malformed JWT payload";
        return res;
    }

    res.rawPayload = *payloadStrPtr;
    res.claims = JwtClaims(payloadJson, payloadStrPtr);

    int64_t now = currentEpochSeconds();

    // Validate exp
    if (options.validateExpiration && payloadJson->has("exp")) {
        int64_t exp = payloadJson->get("exp")->asInt();
        if (now > exp + options.clockToleranceSeconds) {
            res.valid = false;
            res.error = "Token expired";
            return res;
        }
    }

    // Validate nbf
    if (options.validateNotBefore && payloadJson->has("nbf")) {
        int64_t nbf = payloadJson->get("nbf")->asInt();
        if (now < nbf - options.clockToleranceSeconds) {
            res.valid = false;
            res.error = "Token not yet active (nbf)";
            return res;
        }
    }

    // Validate issuer
    if (!options.expectedIssuer.empty()) {
        if (!payloadJson->has("iss") || payloadJson->get("iss")->asStringView() != options.expectedIssuer) {
            res.valid = false;
            res.error = "Issuer mismatch";
            return res;
        }
    }

    // Validate audience
    if (!options.expectedAudience.empty()) {
        if (!payloadJson->has("aud") || payloadJson->get("aud")->asStringView() != options.expectedAudience) {
            res.valid = false;
            res.error = "Audience mismatch";
            return res;
        }
    }

    res.valid = true;
    return res;
}

JwtClaims Jwt::decodeUnverified(const std::string& token) {
    auto parts = split(token, '.');
    if (parts.size() < 2) {
        return JwtClaims();
    }
    auto payloadStrPtr = std::make_shared<std::string>(base64UrlDecode(parts[1]));
    auto payloadJson = json::parse(*payloadStrPtr);
    return JwtClaims(payloadJson, payloadStrPtr);
}

} // namespace sp::rt::crypto
