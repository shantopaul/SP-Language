module std.test;

// ============================================================================
// Standard Library Testing Unified Facade (std.test) (SP 3.0)
// ============================================================================

import std.http;
import std.web.app;
import std.test.http;

public class Test {
    public static fn client(App app) -> TestClient {
        return TestClient.fromApp(app);
    }

    public static fn clientFromServer(HttpServer server) -> TestClient {
        return TestClient.fromServer(server);
    }
}
