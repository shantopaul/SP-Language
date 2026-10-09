module std.security.middleware;

// ============================================================================
// Standard Library Security & Authentication Middleware (SP 2.6)
// ============================================================================

import std.security.jwt;

extern "C" {
    fn sp_rt_http_resp_set_header(ptr resp, string name, string val);
}

public class SecurityHeaders {
    public static fn apply(ptr respHandle) {
        // HTTP Strict Transport Security (HSTS)
        sp_rt_http_resp_set_header(respHandle, "Strict-Transport-Security", "max-age=31536000; includeSubDomains");
        // MIME Sniffing Protection
        sp_rt_http_resp_set_header(respHandle, "X-Content-Type-Options", "nosniff");
        // Clickjacking Defense
        sp_rt_http_resp_set_header(respHandle, "X-Frame-Options", "DENY");
        // Content Security Policy
        sp_rt_http_resp_set_header(respHandle, "Content-Security-Policy", "default-src 'self'");
        // XSS Filter
        sp_rt_http_resp_set_header(respHandle, "X-XSS-Protection", "0");
        // Referrer Policy
        sp_rt_http_resp_set_header(respHandle, "Referrer-Policy", "strict-origin-when-cross-origin");
    }
}

public class JwtAuthValidator {
    public string secret;
    public string expectedIssuer;
    public string expectedAudience;

    public fn init(string secret) {
        this.secret = secret;
        this.expectedIssuer = "";
        this.expectedAudience = "";
    }

    public static fn create(string secret) -> JwtAuthValidator {
        var v = new JwtAuthValidator();
        v.init(secret);
        return v;
    }

    public fn setIssuer(string iss) -> JwtAuthValidator {
        this.expectedIssuer = iss;
        return this;
    }

    public fn setAudience(string aud) -> JwtAuthValidator {
        this.expectedAudience = aud;
        return this;
    }

    /// Extract Bearer token from Authorization header value ("Bearer eyJhbG...")
    public static fn extractBearerToken(string authHeader) -> string {
        if (authHeader == "") {
            return "";
        }
        if (authHeader.startsWith("Bearer ") || authHeader.startsWith("bearer ")) {
            return authHeader.substring(7);
        }
        return "";
    }

    /// Validate token string, returning JwtValidationResult
    public fn validateToken(string token) -> JwtValidationResult {
        if (token == "") {
            var res = new JwtValidationResult();
            res.init();
            res.isValid = false;
            res.error = "Missing token";
            return res;
        }
        if (this.expectedIssuer != "" || this.expectedAudience != "") {
            return Jwt.verifyWithExpectations(token, this.secret, this.expectedIssuer, this.expectedAudience);
        }
        return Jwt.verify(token, this.secret);
    }
}

public class RoleAuthValidator {
    public string requiredRole;

    public fn init(string role) {
        this.requiredRole = role;
    }

    public static fn require(string role) -> RoleAuthValidator {
        var v = new RoleAuthValidator();
        v.init(role);
        return v;
    }

    /// Check if user claims JSON contains the required role
    public fn check(string userRole) -> bool {
        return userRole == this.requiredRole;
    }
}
