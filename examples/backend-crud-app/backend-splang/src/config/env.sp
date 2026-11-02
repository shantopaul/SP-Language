module config.env;

import std.io;
import std.fs;
import std.os;
import std.string;

// ============================================================================
// Application Environment & Configuration Loader
// Supports reading local .env files and falling back to OS environment vars.
// ============================================================================

public class Env {
    public static fn port() -> int {
        let p = OS.getEnv("PORT");
        if (p != "") {
            return int.parse(p);
        }
        return 8000;
    }

    public static fn databaseUrl() -> string {
        let db = OS.getEnv("DATABASE_URL");
        if (db != "") {
            return db;
        }
        return "postgresql://user:password@localhost:5432/neondb";
    }

    public static fn apiKey() -> string {
        let key = OS.getEnv("API_KEY");
        if (key != "") {
            return key;
        }
        return "sp_live_dev_sample_api_key";
    }

    public static fn appName() -> string {
        let name = OS.getEnv("APP_NAME");
        if (name != "") {
            return name;
        }
        return "SP Lang Fullstack API";
    }

    public static fn appEnv() -> string {
        let env = OS.getEnv("APP_ENV");
        if (env != "") {
            return env;
        }
        return "production";
    }

    public static fn parseLine(string line) {
        if (line == "" || line.startsWith("#")) {
            return;
        }
        let eqIdx = line.indexOf("=");
        if (eqIdx > 0) {
            let key = line.substring(0, eqIdx).trim();
            let val = line.substring(eqIdx + 1, line.length()).trim();
            if (val.startsWith("\"") && val.endsWith("\"") && val.length() >= 2) {
                val = val.substring(1, val.length() - 1);
            }
            OS.setEnv(key, val);
        }
    }

    public static fn load(string path = ".env") {
        if (exists(path)) {
            let content = readText(path);
            let len = content.length();
            let start = 0;
            for (let i = 0; i < len; i = i + 1) {
                let ch = content.charAt(i);
                if (ch == '\n' || ch == '\r') {
                    if (i > start) {
                        let line = content.substring(start, i).trim();
                        Env.parseLine(line);
                    }
                    start = i + 1;
                }
            }
            if (start < len) {
                let line = content.substring(start, len).trim();
                Env.parseLine(line);
            }
        }
    }
}
