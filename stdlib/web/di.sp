module std.web.di;

import std.collections;

// ============================================================================
// Standard Library Dependency Injection Container (SP 2.7)
// Supports registering singletons, service instances, configuration values,
// and resolving them in route handlers.
// ============================================================================

public class Container {
    public HashMap<string, ptr> services;
    public HashMap<string, string> values;

    public Container() {
        this.services = new HashMap<string, ptr>();
        this.values = new HashMap<string, string>();
    }

    public fn init() {
        this.services = new HashMap<string, ptr>();
        this.values = new HashMap<string, string>();
    }

    public fn bind(string name, ptr service) {
        this.services.set(name, service);
    }

    public fn singleton(string name, ptr service) {
        this.services.set(name, service);
    }

    public fn resolve(string name) -> ptr {
        if (this.services.contains(name)) {
            return this.services.get(name);
        }
        return null;
    }

    public fn bindValue(string name, string val) {
        this.values.set(name, val);
    }

    public fn resolveValue(string name) -> string {
        if (this.values.contains(name)) {
            return this.values.get(name);
        }
        return "";
    }

    public fn has(string name) -> bool {
        return this.services.contains(name) || this.values.contains(name);
    }

    public fn clear() {
        this.services.clear();
        this.values.clear();
    }

    public fn size() -> int {
        return this.services.size() + this.values.size();
    }
}
