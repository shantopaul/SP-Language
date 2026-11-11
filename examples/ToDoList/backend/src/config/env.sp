module config.env;

import std.os;
import std.fs;
import std.string;

public class Env {
    /// Load key=value pairs from `.env` if present without overriding existing OS env vars.
    public static fn load(string path = ".env") {
        if (!exists(path)) {
            return;
        }
        let raw = readText(path);
        if (raw.length() == 0) {
            return;
        }
        let lines = String.split(raw, "\n");
        let i = 0;
        while (i < lines.length) {
            let line = lines[i].trim();
            i = i + 1;
            if (line.length() == 0 || line.startsWith("#")) {
                continue;
            }
            if (line.startsWith("export ")) {
                line = line.substring(7, line.length()).trim();
            }
            let eqIdx = Env.findFirstEquals(line);
            if (eqIdx <= 0) {
                continue;
            }
            let key = line.substring(0, eqIdx).trim();
            let val = line.substring(eqIdx + 1, line.length()).trim();
            val = Env.stripQuotes(val);
            if (key.length() > 0 && OS.getEnv(key).trim().length() == 0) {
                OS.setEnv(key, val);
            }
        }
    }

    public static fn getOrDefault(string key, string fallback) -> string {
        let val = OS.getEnv(key).trim();
        if (val.length() == 0) {
            return fallback;
        }
        return val;
    }

    public static fn port() -> int {
        let p = Env.parseIntOr(Env.getOrDefault("PORT", "8080"), 8080);
        if (p <= 0 || p > 65535) {
            return 8080;
        }
        return p;
    }

    public static fn dbPath() -> string {
        return Env.getOrDefault("TODOLIST_DB_PATH", "data/todolist.db");
    }

    public static fn jwtSecret() -> string {
        return OS.getEnv("JWT_SECRET").trim();
    }

    public static fn hasValidJwtSecret() -> bool {
        return Env.jwtSecret().length() >= 32;
    }

    public static fn jwtTtlSeconds() -> int {
        let ttl = Env.parseIntOr(Env.getOrDefault("JWT_TTL_SECONDS", "86400"), 86400);
        if (ttl < 300) {
            return 300;
        }
        return ttl;
    }

    public static fn corsOrigins() -> string {
        return Env.getOrDefault("CORS_ORIGINS", "http://localhost:3000");
    }

    public static fn rateLimitMax() -> int {
        let maxReq = Env.parseIntOr(Env.getOrDefault("RATE_LIMIT_MAX", "120"), 120);
        if (maxReq < 10) {
            return 10;
        }
        return maxReq;
    }

    public static fn rateLimitWindowSeconds() -> int {
        let win = Env.parseIntOr(Env.getOrDefault("RATE_LIMIT_WINDOW_SECONDS", "60"), 60);
        if (win < 1) {
            return 60;
        }
        return win;
    }

    public static fn appEnv() -> string {
        return Env.getOrDefault("APP_ENV", "development");
    }

    /// Ensure the parent directory for the SQLite database file exists.
    public static fn ensureDataDir() {
        let path = Env.dbPath();
        if (path == ":memory:" || path.length() == 0) {
            return;
        }
        let lastSlash = -1;
        let i = 0;
        while (i < path.length()) {
            let ch = path.charAt(i);
            if (ch == '/' || ch == '\\') {
                lastSlash = i;
            }
            i = i + 1;
        }
        if (lastSlash > 0) {
            let dir = path.substring(0, lastSlash);
            if (dir.length() > 0 && !exists(dir)) {
                createDir(dir);
            }
        }
    }

    /// Safe integer parser using explicit character comparison.
    public static fn parseIntOr(string raw, int fallback) -> int {
        let s = raw.trim();
        if (s.length() == 0) {
            return fallback;
        }
        let idx = 0;
        let negative = false;
        let first = s.charAt(0);
        if (first == '-') {
            negative = true;
            idx = 1;
        } else if (first == '+') {
            idx = 1;
        }
        if (idx >= s.length()) {
            return fallback;
        }
        let acc = 0;
        while (idx < s.length()) {
            let ch = s.charAt(idx);
            let d = -1;
            if (ch == '0') { d = 0; }
            else if (ch == '1') { d = 1; }
            else if (ch == '2') { d = 2; }
            else if (ch == '3') { d = 3; }
            else if (ch == '4') { d = 4; }
            else if (ch == '5') { d = 5; }
            else if (ch == '6') { d = 6; }
            else if (ch == '7') { d = 7; }
            else if (ch == '8') { d = 8; }
            else if (ch == '9') { d = 9; }
            else { return fallback; }
            acc = (acc * 10) + d;
            idx = idx + 1;
        }
        if (negative) {
            return 0 - acc;
        }
        return acc;
    }

    static fn findFirstEquals(string line) -> int {
        let i = 0;
        while (i < line.length()) {
            if (line.charAt(i) == '=') {
                return i;
            }
            i = i + 1;
        }
        return -1;
    }

    static fn stripQuotes(string val) -> string {
        let n = val.length();
        if (n >= 2) {
            let first = val.charAt(0);
            let last = val.charAt(n - 1);
            if ((first == '"' && last == '"') || (first == '\'' && last == '\'')) {
                return val.substring(1, n - 1);
            }
        }
        return val;
    }
}
