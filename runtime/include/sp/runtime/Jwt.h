#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "sp/runtime/Json.h"

namespace sp::rt::crypto {

enum class JwtAlgorithm {
    HS256
};

class JwtClaims {
public:
    JwtClaims();
    explicit JwtClaims(json::JsonValuePtr json, std::shared_ptr<std::string> rawJson = nullptr);

    void setIssuer(const std::string& iss);
    std::string getIssuer() const;

    void setSubject(const std::string& sub);
    std::string getSubject() const;

    void setAudience(const std::string& aud);
    std::string getAudience() const;

    void setExpiration(int64_t exp);
    int64_t getExpiration() const;

    void setNotBefore(int64_t nbf);
    int64_t getNotBefore() const;

    void setIssuedAt(int64_t iat);
    int64_t getIssuedAt() const;

    void setJwtId(const std::string& jti);
    std::string getJwtId() const;

    // Generic claim setters and getters
    void setString(const std::string& key, const std::string& val);
    std::string getString(const std::string& key, const std::string& defaultVal = "") const;

    void setInt(const std::string& key, int64_t val);
    int64_t getInt(const std::string& key, int64_t defaultVal = 0) const;

    void setBool(const std::string& key, bool val);
    bool getBool(const std::string& key, bool defaultVal = false) const;

    bool has(const std::string& key) const;

    json::JsonValuePtr getJson() const { return m_json; }
    std::string toJsonString() const;

private:
    json::JsonValuePtr m_json;
    std::shared_ptr<std::string> m_rawJson;
};

struct JwtValidationOptions {
    std::string expectedIssuer = "";
    std::string expectedAudience = "";
    int64_t clockToleranceSeconds = 0;
    bool validateExpiration = true;
    bool validateNotBefore = true;
};

struct JwtValidationResult {
    bool valid = false;
    std::string error = "";
    std::string algorithm = "";
    JwtClaims claims;
    std::string rawPayload = "";
};

class Jwt {
public:
    /// Sign a JWT with claims and secret (default HS256)
    static std::string sign(const JwtClaims& claims, const std::string& secret, JwtAlgorithm alg = JwtAlgorithm::HS256);

    /// Sign a JWT directly from raw payload JSON
    static std::string signJson(const std::string& payloadJson, const std::string& secret, JwtAlgorithm alg = JwtAlgorithm::HS256);

    /// Verify signature and validate claims (exp, nbf, iss, aud)
    static JwtValidationResult verify(const std::string& token, const std::string& secret, const JwtValidationOptions& options = {});

    /// Decode payload without verifying signature (for token inspection)
    static JwtClaims decodeUnverified(const std::string& token);
};

} // namespace sp::rt::crypto
