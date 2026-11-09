#include "sp/runtime/Password.h"
#include "sp/runtime/Jwt.h"
#include "sp/runtime/Session.h"
#include "sp/runtime/PersistentSession.h"
#include "sp/runtime/Crypto.h"
#include "sp/runtime/Runtime.h"
#include <cstdlib>
#include <cstring>
#include <string>
#include <memory>

namespace {

const char* alloc_str(const std::string& s) {
    char* res = static_cast<char*>(std::malloc(s.size() + 1));
    if (res) {
        std::memcpy(res, s.c_str(), s.size() + 1);
    }
    return res;
}

struct JwtResultWrapper {
    sp::rt::crypto::JwtValidationResult result;
};

} // anonymous namespace

extern "C" {

// ============================================================================
// Base64Url & Timing Safe Equal C-ABI
// ============================================================================

const char* sp_rt_crypto_base64url_encode(const char* data) {
    if (!data) return alloc_str("");
    return alloc_str(sp::rt::crypto::base64UrlEncode(data));
}

const char* sp_rt_crypto_base64url_decode(const char* data) {
    if (!data) return alloc_str("");
    return alloc_str(sp::rt::crypto::base64UrlDecode(data));
}

bool sp_rt_crypto_timing_safe_equal(const char* a, const char* b) {
    if (!a || !b) return (a == b);
    return sp::rt::crypto::timingSafeEqual(a, b);
}

// ============================================================================
// Password Hashing C-ABI (Argon2id & Bcrypt)
// ============================================================================

const char* sp_rt_password_hash(const char* password, const char* algorithm, int64_t cost_or_memory, int64_t iterations, int64_t parallelism) {
    if (!password) return alloc_str("");
    std::string pass(password);
    std::string alg = algorithm ? algorithm : "argon2id";

    sp::rt::crypto::PasswordOptions opts;
    if (alg == "bcrypt") {
        opts.algorithm = sp::rt::crypto::PasswordAlgorithm::Bcrypt;
        if (cost_or_memory >= 4 && cost_or_memory <= 31) {
            opts.bcryptCost = static_cast<uint32_t>(cost_or_memory);
        } else {
            opts.bcryptCost = 10;
        }
    } else {
        opts.algorithm = sp::rt::crypto::PasswordAlgorithm::Argon2id;
        opts.memoryKiB = (cost_or_memory > 0) ? static_cast<uint32_t>(cost_or_memory) : 65536;
        opts.iterations = (iterations > 0) ? static_cast<uint32_t>(iterations) : 3;
        opts.parallelism = (parallelism > 0) ? static_cast<uint32_t>(parallelism) : 1;
        opts.hashLength = 32;
    }

    try {
        std::string hashed = sp::rt::crypto::Password::hash(pass, opts);
        return alloc_str(hashed);
    } catch (...) {
        return alloc_str("");
    }
}

bool sp_rt_password_verify(const char* password, const char* hash) {
    if (!password || !hash) return false;
    try {
        return sp::rt::crypto::Password::verify(password, hash);
    } catch (...) {
        return false;
    }
}

// ============================================================================
// JWT C-ABI (RFC 7519)
// ============================================================================

const char* sp_rt_jwt_sign(const char* payload_json, const char* secret, const char* algorithm, int64_t expires_in_seconds, const char* issuer, const char* audience) {
    (void)algorithm;
    if (!payload_json || !secret) return alloc_str("");

    std::string rawPayload = payload_json;
    std::string err;
    auto jsonVal = sp::rt::json::parse(rawPayload, &err);
    if (!jsonVal || !jsonVal->isObject()) {
        jsonVal = sp::rt::json::JsonValue::makeObject();
    }

    // Set claims if specified
    if (expires_in_seconds > 0) {
        int64_t now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();
        jsonVal->set("exp", sp::rt::json::JsonValue::makeInt(now + expires_in_seconds));
        if (!jsonVal->has("iat")) {
            jsonVal->set("iat", sp::rt::json::JsonValue::makeInt(now));
        }
    }

    if (issuer && std::strlen(issuer) > 0) {
        jsonVal->set("iss", sp::rt::json::JsonValue::makeString(issuer));
    }
    if (audience && std::strlen(audience) > 0) {
        jsonVal->set("aud", sp::rt::json::JsonValue::makeString(audience));
    }

    std::string token = sp::rt::crypto::Jwt::signJson(jsonVal->stringify(0), secret);
    return alloc_str(token);
}

void* sp_rt_jwt_verify(const char* token, const char* secret, const char* expected_issuer, const char* expected_audience) {
    if (!token || !secret) return nullptr;

    sp::rt::crypto::JwtValidationOptions opts;
    if (expected_issuer && std::strlen(expected_issuer) > 0) {
        opts.expectedIssuer = expected_issuer;
    }
    if (expected_audience && std::strlen(expected_audience) > 0) {
        opts.expectedAudience = expected_audience;
    }

    auto res = sp::rt::crypto::Jwt::verify(token, secret, opts);
    auto* wrapper = new JwtResultWrapper();
    wrapper->result = std::move(res);
    return wrapper;
}

bool sp_rt_jwt_res_is_valid(void* res) {
    if (!res) return false;
    auto* wrapper = static_cast<JwtResultWrapper*>(res);
    return wrapper->result.valid;
}

const char* sp_rt_jwt_res_error(void* res) {
    if (!res) return alloc_str("Null JWT verification result");
    auto* wrapper = static_cast<JwtResultWrapper*>(res);
    return alloc_str(wrapper->result.error);
}

const char* sp_rt_jwt_res_payload(void* res) {
    if (!res) return alloc_str("{}");
    auto* wrapper = static_cast<JwtResultWrapper*>(res);
    return alloc_str(wrapper->result.rawPayload);
}

const char* sp_rt_jwt_res_claim(void* res, const char* claim_name) {
    if (!res || !claim_name) return alloc_str("");
    auto* wrapper = static_cast<JwtResultWrapper*>(res);
    return alloc_str(wrapper->result.claims.getString(claim_name));
}

void sp_rt_jwt_res_free(void* res) {
    if (res) {
        auto* wrapper = static_cast<JwtResultWrapper*>(res);
        delete wrapper;
    }
}

// ============================================================================
// Signed Cookie & Session Management C-ABI
// ============================================================================

const char* sp_rt_cookie_sign(const char* val, const char* secret) {
    if (!val || !secret) return alloc_str("");
    return alloc_str(sp::rt::crypto::Cookie::signValue(val, secret));
}

const char* sp_rt_cookie_unsign(const char* signed_val, const char* secret) {
    if (!signed_val || !secret) return alloc_str("");
    return alloc_str(sp::rt::crypto::Cookie::unsignValue(signed_val, secret));
}

void* sp_rt_session_store_create(int64_t default_ttl_secs) {
    return new sp::rt::crypto::SessionStore(default_ttl_secs > 0 ? default_ttl_secs : 3600);
}

const char* sp_rt_session_create(void* store, const char* data_json, int64_t ttl_secs) {
    if (!store) return alloc_str("");
    auto* s = static_cast<sp::rt::crypto::SessionStore*>(store);
    std::string sid = s->create(data_json ? data_json : "{}", ttl_secs);
    return alloc_str(sid);
}

const char* sp_rt_session_get(void* store, const char* session_id) {
    if (!store || !session_id) return alloc_str("");
    auto* s = static_cast<sp::rt::crypto::SessionStore*>(store);
    return alloc_str(s->get(session_id));
}

bool sp_rt_session_set(void* store, const char* session_id, const char* data_json, int64_t ttl_secs) {
    if (!store || !session_id) return false;
    auto* s = static_cast<sp::rt::crypto::SessionStore*>(store);
    return s->set(session_id, data_json ? data_json : "{}", ttl_secs);
}

bool sp_rt_session_destroy(void* store, const char* session_id) {
    if (!store || !session_id) return false;
    auto* s = static_cast<sp::rt::crypto::SessionStore*>(store);
    return s->destroy(session_id);
}

const char* sp_rt_session_regenerate(void* store, const char* old_session_id, int64_t ttl_secs) {
    if (!store || !old_session_id) return alloc_str("");
    auto* s = static_cast<sp::rt::crypto::SessionStore*>(store);
    return alloc_str(s->regenerate(old_session_id, ttl_secs));
}

void sp_rt_session_store_destroy(void* store) {
    if (store) {
        auto* s = static_cast<sp::rt::crypto::SessionStore*>(store);
        delete s;
    }
}

// ============================================================================
// Persistent Session C-ABI (Phase 31)
// ============================================================================

void* sp_rt_psession_open(const char* path, int64_t default_ttl_secs) {
    if (!path || !*path) return nullptr;
    auto* s = new sp::rt::crypto::PersistentSessionStore(default_ttl_secs > 0 ? default_ttl_secs : 3600);
    if (!s->open(path)) {
        delete s;
        return nullptr;
    }
    return s;
}

void sp_rt_psession_close(void* store) {
    if (store) {
        delete static_cast<sp::rt::crypto::PersistentSessionStore*>(store);
    }
}

const char* sp_rt_psession_last_error(void* store) {
    if (!store) return alloc_str("persistent session store is not open");
    return alloc_str(static_cast<sp::rt::crypto::PersistentSessionStore*>(store)->lastError());
}

const char* sp_rt_psession_create(void* store, const char* data_json, int64_t ttl_secs) {
    if (!store) return alloc_str("");
    auto* s = static_cast<sp::rt::crypto::PersistentSessionStore*>(store);
    return alloc_str(s->create(data_json ? data_json : "{}", ttl_secs));
}

const char* sp_rt_psession_get(void* store, const char* session_id) {
    if (!store || !session_id) return alloc_str("");
    auto* s = static_cast<sp::rt::crypto::PersistentSessionStore*>(store);
    return alloc_str(s->get(session_id));
}

bool sp_rt_psession_set(void* store, const char* session_id, const char* data_json, int64_t ttl_secs) {
    if (!store || !session_id) return false;
    auto* s = static_cast<sp::rt::crypto::PersistentSessionStore*>(store);
    return s->set(session_id, data_json ? data_json : "{}", ttl_secs);
}

bool sp_rt_psession_destroy(void* store, const char* session_id) {
    if (!store || !session_id) return false;
    auto* s = static_cast<sp::rt::crypto::PersistentSessionStore*>(store);
    return s->destroy(session_id);
}

const char* sp_rt_psession_regenerate(void* store, const char* old_session_id, int64_t ttl_secs) {
    if (!store || !old_session_id) return alloc_str("");
    auto* s = static_cast<sp::rt::crypto::PersistentSessionStore*>(store);
    return alloc_str(s->regenerate(old_session_id, ttl_secs));
}

int64_t sp_rt_psession_count(void* store) {
    if (!store) return 0;
    auto* s = static_cast<sp::rt::crypto::PersistentSessionStore*>(store);
    return static_cast<int64_t>(s->count());
}

} // extern "C"
