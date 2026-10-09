module std.security.jwt;

// ============================================================================
// Standard Library JSON Web Tokens (RFC 7519) (SP 2.6)
// ============================================================================

extern "C" {
    fn sp_rt_jwt_sign(string payload_json, string secret, string algorithm, int expires_in_seconds, string issuer, string audience) -> string;
    fn sp_rt_jwt_verify(string token, string secret, string expected_issuer, string expected_audience) -> ptr;
    fn sp_rt_jwt_res_is_valid(ptr res) -> bool;
    fn sp_rt_jwt_res_error(ptr res) -> string;
    fn sp_rt_jwt_res_payload(ptr res) -> string;
    fn sp_rt_jwt_res_claim(ptr res, string claim_name) -> string;
    fn sp_rt_jwt_res_free(ptr res);
}

public class JwtClaims {
    public string issuer;
    public string subject;
    public string audience;
    public int expiresIn;
    public string payloadJson;

    public fn init() {
        this.issuer = "";
        this.subject = "";
        this.audience = "";
        this.expiresIn = 0;
        this.payloadJson = "{}";
    }

    public fn setSubject(string sub) -> JwtClaims {
        this.subject = sub;
        return this;
    }

    public fn setIssuer(string iss) -> JwtClaims {
        this.issuer = iss;
        return this;
    }

    public fn setAudience(string aud) -> JwtClaims {
        this.audience = aud;
        return this;
    }

    public fn setExpiresIn(int seconds) -> JwtClaims {
        this.expiresIn = seconds;
        return this;
    }

    public fn setPayload(string json) -> JwtClaims {
        this.payloadJson = json;
        return this;
    }
}

public class JwtValidationResult {
    public bool isValid;
    public string error;
    public string payloadJson;
    public ptr handle;

    public fn init() {
        this.isValid = false;
        this.error = "";
        this.payloadJson = "";
        this.handle = null;
    }

    public fn getClaim(string name) -> string {
        if (this.handle == null) {
            return "";
        }
        return sp_rt_jwt_res_claim(this.handle, name);
    }

    public fn getSubject() -> string {
        return this.getClaim("sub");
    }

    public fn getIssuer() -> string {
        return this.getClaim("iss");
    }

    public fn getAudience() -> string {
        return this.getClaim("aud");
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_jwt_res_free(this.handle);
            this.handle = null;
        }
    }
}

public class Jwt {
    /// Sign a payload JSON with secret using HMAC-SHA256 (HS256)
    public static fn sign(string payloadJson, string secret) -> string {
        return sp_rt_jwt_sign(payloadJson, secret, "HS256", 0, "", "");
    }

    /// Sign payload JSON with explicit expiration in seconds
    public static fn signExp(string payloadJson, string secret, int expiresInSeconds) -> string {
        return sp_rt_jwt_sign(payloadJson, secret, "HS256", expiresInSeconds, "", "");
    }

    /// Sign using fluent JwtClaims builder
    public static fn signClaims(JwtClaims claims, string secret) -> string {
        return sp_rt_jwt_sign(claims.payloadJson, secret, "HS256", claims.expiresIn, claims.issuer, claims.audience);
    }

    /// Verify a JWT token against a secret
    public static fn verify(string token, string secret) -> JwtValidationResult {
        var rawRes = sp_rt_jwt_verify(token, secret, "", "");
        var res = new JwtValidationResult();
        res.init();
        if (rawRes != null) {
            res.handle = rawRes;
            res.isValid = sp_rt_jwt_res_is_valid(rawRes);
            res.error = sp_rt_jwt_res_error(rawRes);
            res.payloadJson = sp_rt_jwt_res_payload(rawRes);
        } else {
            res.isValid = false;
            res.error = "Failed to verify JWT";
        }
        return res;
    }

    /// Verify a JWT token checking expected issuer and audience
    public static fn verifyWithExpectations(string token, string secret, string expectedIssuer, string expectedAudience) -> JwtValidationResult {
        var rawRes = sp_rt_jwt_verify(token, secret, expectedIssuer, expectedAudience);
        var res = new JwtValidationResult();
        res.init();
        if (rawRes != null) {
            res.handle = rawRes;
            res.isValid = sp_rt_jwt_res_is_valid(rawRes);
            res.error = sp_rt_jwt_res_error(rawRes);
            res.payloadJson = sp_rt_jwt_res_payload(rawRes);
        } else {
            res.isValid = false;
            res.error = "Failed to verify JWT";
        }
        return res;
    }
}
