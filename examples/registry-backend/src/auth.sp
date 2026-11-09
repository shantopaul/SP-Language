module auth;

import std.web;
import std.crypto;

// ============================================================================
// Bearer-token authentication for the registry service.
//
// The expected token comes from SP_REGISTRY_TOKEN (see config.sp) and is
// NEVER hard-coded. Comparison uses Crypto.timingSafeEqual so a wrong token
// does not leak how many leading characters were correct. Missing,
// malformed, or wrong tokens are all rejected with 401 — there is no
// "allow all when unconfigured" fallback: if no token is configured the
// service refuses to start at all (see main.sp).
// ============================================================================

public class RegistryAuth {
    public static fn validateBearer(Context ctx, string expected) -> bool {
        if (expected == "") {
            return false;
        }
        let header = ctx.header("Authorization");
        if (!header.startsWith("Bearer ")) {
            return false;
        }
        let token = header.substring(7, header.length()).trim();
        if (token == "") {
            return false;
        }
        return Crypto.timingSafeEqual(token, expected);
    }

    public static fn unauthorizedResponse(Context ctx) -> HttpResponse {
        return ctx.jsonStatus("{\"error\":\"Unauthorized\",\"code\":\"UNAUTHORIZED\",\"message\":\"Missing or invalid Bearer token. Publish with --token or set SP_REGISTRY_TOKEN.\"}", 401);
    }
}
