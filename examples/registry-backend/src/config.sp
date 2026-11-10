module config;

import std.os;

// ============================================================================
// Registry service configuration.
//
// Every value comes from the environment; nothing secret is hard-coded.
// The publish token MUST be supplied via SP_REGISTRY_TOKEN — the service
// refuses to start without it (fail closed, see main.sp).
// ============================================================================

public class RegistryConfig {
    public static fn port() -> int {
        let p = OS.getEnv("REGISTRY_PORT");
        if (p != "") {
            return int.parse(p);
        }
        return 8090;
    }

    public static fn dbPath() -> string {
        let db = OS.getEnv("REGISTRY_DB");
        if (db != "") {
            return db;
        }
        return "registry.db";
    }

    public static fn token() -> string {
        return OS.getEnv("SP_REGISTRY_TOKEN");
    }

    public static fn openApiPath() -> string {
        let p = OS.getEnv("REGISTRY_OPENAPI_PATH");
        if (p != "") {
            return p;
        }
        return "openapi.yaml";
    }
}
