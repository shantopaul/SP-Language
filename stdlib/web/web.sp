module std.web;

// ============================================================================
// Standard Library Backend Framework Unified Facade (SP 2.7)
// ============================================================================

import std.web.context;
import std.web.di;
import std.web.app;
import std.web.sse;
import std.http;
import std.security.middleware;

public class Web {
    public static fn createApp() -> App {
        return new App();
    }

    public static fn createContext(ptr reqHandle) -> Context {
        return Context.create(reqHandle);
    }

    public static fn createContainer() -> Container {
        return new Container();
    }
}
