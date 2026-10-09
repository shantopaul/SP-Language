module std.health;

import std.io;

// ============================================================================
// Standard Library Health & Readiness Telemetry (std.health) (SP 2.9)
// Provides system liveness (/healthz) and dependency readiness (/readyz) checks
// with structured JSON reports for Kubernetes container probes.
// ============================================================================

extern "C" {
    fn sp_rt_health_registry_create() -> ptr;
    fn sp_rt_health_registry_destroy(ptr reg);
    fn sp_rt_health_registry_default() -> ptr;
    fn sp_rt_health_set_status(ptr reg, string name, bool is_up, string message);
    fn sp_rt_health_check_liveness(ptr reg) -> string;
    fn sp_rt_health_check_readiness(ptr reg, ptr out_is_up) -> string;
}

public class HealthRegistry {
    public ptr handle;

    public HealthRegistry() {
        this.handle = sp_rt_health_registry_create();
    }

    public static fn create() -> HealthRegistry {
        return new HealthRegistry();
    }

    public static fn getGlobal() -> HealthRegistry {
        let r = new HealthRegistry();
        if (r.handle != null) {
            sp_rt_health_registry_destroy(r.handle);
        }
        r.handle = sp_rt_health_registry_default();
        return r;
    }

    public fn setStatus(string name, bool isUp, string message = "") {
        if (this.handle != null) {
            sp_rt_health_set_status(this.handle, name, isUp, message);
        }
    }

    public fn checkLiveness() -> string {
        if (this.handle == null) { return "{\"status\":\"DOWN\"}"; }
        return sp_rt_health_check_liveness(this.handle);
    }

    public fn checkReadiness() -> string {
        if (this.handle == null) { return "{\"status\":\"DOWN\"}"; }
        return sp_rt_health_check_readiness(this.handle, null);
    }
}

public class Health {
    public static fn setStatus(string name, bool isUp, string message = "") {
        HealthRegistry.getGlobal().setStatus(name, isUp, message);
    }

    public static fn liveness() -> string {
        return HealthRegistry.getGlobal().checkLiveness();
    }

    public static fn readiness() -> string {
        return HealthRegistry.getGlobal().checkReadiness();
    }
}
