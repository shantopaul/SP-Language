module middleware.auth;

import std.web;
import std.string;
import config.env;

// ============================================================================
// Security & API Key Authentication Middleware
// Validates X-API-Key and Authorization Bearer headers against configured secret.
// ============================================================================

public class AuthMiddleware {
    public static fn validateKey(Context ctx) -> bool {
        let expected = Env.apiKey().trim();
        // If no API key configured, allow all
        if (expected == "") {
            return true;
        }

        // 1. Check X-API-Key header
        let xKey = ctx.header("X-API-Key");
        if (xKey != "" && xKey.trim() == expected) {
            return true;
        }

        // 2. Check Authorization header: Bearer <key>
        let authHdr = ctx.header("Authorization");
        if (authHdr.startsWith("Bearer ")) {
            let token = authHdr.substring(7, authHdr.length()).trim();
            if (token == expected) {
                return true;
            }
        }

        // 3. Check query param fallback (?api_key=... or ?key=...)
        let qKey = ctx.query("api_key");
        if (qKey == "") {
            qKey = ctx.query("key");
        }
        if (qKey != "" && qKey.trim() == expected) {
            return true;
        }

        return false;
    }


    public static fn unauthorizedResponse(Context ctx) -> HttpResponse {
        return ctx.jsonStatus("{\"error\":\"Unauthorized\",\"message\":\"Invalid or missing API key. Provide X-API-Key header.\",\"code\":401}", 401);
    }
}
