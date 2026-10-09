module std.security;

// ============================================================================
// Standard Library Security, Authentication & Cryptography Facade (SP 2.6)
// ============================================================================

import std.security.jwt;
import std.security.session;
import std.security.middleware;
import std.crypto.password;

public class Security {
    public static fn hashPassword(string plainText) -> string {
        return Password.hash(plainText);
    }

    public static fn verifyPassword(string plainText, string hash) -> bool {
        return Password.verify(plainText, hash);
    }

    public static fn signJwt(string payloadJson, string secret) -> string {
        return Jwt.sign(payloadJson, secret);
    }

    public static fn verifyJwt(string token, string secret) -> JwtValidationResult {
        return Jwt.verify(token, secret);
    }

    public static fn signCookie(string value, string secret) -> string {
        return Cookie.sign(value, secret);
    }

    public static fn unsignCookie(string signedValue, string secret) -> string {
        return Cookie.unsign(signedValue, secret);
    }

    public static fn createSessionStore(int defaultTtlSeconds) -> SessionStore {
        return SessionStore.create(defaultTtlSeconds);
    }
}
