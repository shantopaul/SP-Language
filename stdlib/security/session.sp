module std.security.session;

// ============================================================================
// Standard Library Signed Cookies & Session Management (SP 2.6)
// ============================================================================

extern "C" {
    fn sp_rt_cookie_sign(string val, string secret) -> string;
    fn sp_rt_cookie_unsign(string signed_val, string secret) -> string;
    fn sp_rt_session_store_create(int default_ttl_secs) -> ptr;
    fn sp_rt_session_create(ptr store, string data_json, int ttl_secs) -> string;
    fn sp_rt_session_get(ptr store, string session_id) -> string;
    fn sp_rt_session_set(ptr store, string session_id, string data_json, int ttl_secs) -> bool;
    fn sp_rt_session_destroy(ptr store, string session_id) -> bool;
    fn sp_rt_session_regenerate(ptr store, string old_session_id, int ttl_secs) -> string;
    fn sp_rt_session_store_destroy(ptr store);
}

public class Cookie {
    public string name;
    public string value;
    public string path;
    public string domain;
    public int maxAge;
    public bool secure;
    public bool httpOnly;
    public string sameSite;

    public fn init(string name, string value) {
        this.name = name;
        this.value = value;
        this.path = "/";
        this.domain = "";
        this.maxAge = 0;
        this.secure = true;
        this.httpOnly = true;
        this.sameSite = "Lax";
    }

    public static fn create(string name, string value) -> Cookie {
        var c = new Cookie();
        c.init(name, value);
        return c;
    }

    public fn setPath(string path) -> Cookie {
        this.path = path;
        return this;
    }

    public fn setDomain(string domain) -> Cookie {
        this.domain = domain;
        return this;
    }

    public fn setMaxAge(int seconds) -> Cookie {
        this.maxAge = seconds;
        return this;
    }

    public fn setSecure(bool secure) -> Cookie {
        this.secure = secure;
        return this;
    }

    public fn setHttpOnly(bool httpOnly) -> Cookie {
        this.httpOnly = httpOnly;
        return this;
    }

    public fn setSameSite(string mode) -> Cookie {
        this.sameSite = mode;
        return this;
    }

    public fn serialize() -> string {
        var res = this.name + "=" + this.value;
        if (this.path != "") {
            res = res + "; Path=" + this.path;
        }
        if (this.domain != "") {
            res = res + "; Domain=" + this.domain;
        }
        if (this.maxAge > 0) {
            res = res + "; Max-Age=" + this.maxAge;
        }
        if (this.httpOnly) {
            res = res + "; HttpOnly";
        }
        if (this.secure) {
            res = res + "; Secure";
        }
        if (this.sameSite != "") {
            res = res + "; SameSite=" + this.sameSite;
        }
        return res;
    }

    /// Sign a cookie value with HMAC-SHA256 producing "s:<value>.<signature>"
    public static fn sign(string val, string secret) -> string {
        return sp_rt_cookie_sign(val, secret);
    }

    /// Verify and unsign a cookie value; returns original value if valid or empty string if tampered
    public static fn unsign(string signedVal, string secret) -> string {
        return sp_rt_cookie_unsign(signedVal, secret);
    }
}

public class SessionStore {
    public ptr handle;

    public fn init(int defaultTtlSeconds) {
        this.handle = sp_rt_session_store_create(defaultTtlSeconds);
    }

    public static fn create(int defaultTtlSeconds) -> SessionStore {
        var s = new SessionStore();
        s.init(defaultTtlSeconds);
        return s;
    }

    public fn createSession(string dataJson) -> string {
        if (this.handle == null) return "";
        return sp_rt_session_create(this.handle, dataJson, -1);
    }

    public fn createSessionWithTtl(string dataJson, int ttlSeconds) -> string {
        if (this.handle == null) return "";
        return sp_rt_session_create(this.handle, dataJson, ttlSeconds);
    }

    public fn get(string sessionId) -> string {
        if (this.handle == null) return "";
        return sp_rt_session_get(this.handle, sessionId);
    }

    public fn set(string sessionId, string dataJson) -> bool {
        if (this.handle == null) return false;
        return sp_rt_session_set(this.handle, sessionId, dataJson, -1);
    }

    public fn destroy(string sessionId) -> bool {
        if (this.handle == null) return false;
        return sp_rt_session_destroy(this.handle, sessionId);
    }

    public fn regenerate(string oldSessionId) -> string {
        if (this.handle == null) return "";
        return sp_rt_session_regenerate(this.handle, oldSessionId, -1);
    }

    public fn close() {
        if (this.handle != null) {
            sp_rt_session_store_destroy(this.handle);
            this.handle = null;
        }
    }
}
