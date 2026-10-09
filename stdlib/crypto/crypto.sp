module std.crypto;

// ============================================================================
// Standard Library Cryptography & Secure Hashing (SP 1.6)
// ============================================================================

extern "C" {
    fn sp_rt_crypto_sha256(string data) -> string;
    fn sp_rt_crypto_hmac_sha256(string key, string data) -> string;
    fn sp_rt_crypto_base64_encode(string data) -> string;
    fn sp_rt_crypto_base64_decode(string data) -> string;
    fn sp_rt_crypto_random_hex(int bytes) -> string;
    fn sp_rt_crypto_uuid() -> string;
    fn sp_rt_crypto_base64url_encode(string data) -> string;
    fn sp_rt_crypto_base64url_decode(string data) -> string;
    fn sp_rt_crypto_timing_safe_equal(string a, string b) -> bool;
}

public fn sha256(string data) -> string {
    return sp_rt_crypto_sha256(data);
}

public fn hmacSha256(string key, string data) -> string {
    return sp_rt_crypto_hmac_sha256(key, data);
}

public fn base64Encode(string data) -> string {
    return sp_rt_crypto_base64_encode(data);
}

public fn base64Decode(string data) -> string {
    return sp_rt_crypto_base64_decode(data);
}

public fn base64UrlEncode(string data) -> string {
    return sp_rt_crypto_base64url_encode(data);
}

public fn base64UrlDecode(string data) -> string {
    return sp_rt_crypto_base64url_decode(data);
}

public fn timingSafeEqual(string a, string b) -> bool {
    return sp_rt_crypto_timing_safe_equal(a, b);
}

public fn randomHex(int bytes) -> string {
    return sp_rt_crypto_random_hex(bytes);
}

public fn randomUuid() -> string {
    return sp_rt_crypto_uuid();
}

public class Crypto {
    public static fn sha256(string data) -> string {
        return sp_rt_crypto_sha256(data);
    }

    public static fn hmacSha256(string key, string data) -> string {
        return sp_rt_crypto_hmac_sha256(key, data);
    }

    public static fn base64Encode(string data) -> string {
        return sp_rt_crypto_base64_encode(data);
    }

    public static fn base64Decode(string data) -> string {
        return sp_rt_crypto_base64_decode(data);
    }

    public static fn base64UrlEncode(string data) -> string {
        return sp_rt_crypto_base64url_encode(data);
    }

    public static fn base64UrlDecode(string data) -> string {
        return sp_rt_crypto_base64url_decode(data);
    }

    public static fn timingSafeEqual(string a, string b) -> bool {
        return sp_rt_crypto_timing_safe_equal(a, b);
    }

    public static fn randomHex(int bytes) -> string {
        return sp_rt_crypto_random_hex(bytes);
    }

    public static fn uuid() -> string {
        return sp_rt_crypto_uuid();
    }
}
