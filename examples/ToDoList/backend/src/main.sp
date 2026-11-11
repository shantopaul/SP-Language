module main;

import std.os;
import config.env;
import db.schema;
import routes.router;
import util.logger;

fn main() -> int {
    Env.load(".env");

    if (!Env.hasValidJwtSecret()) {
        AppLog.error(
            "main",
            "Startup aborted: JWT_SECRET environment variable must be set to at least 32 characters."
        );
        return 1;
    }

    if (!DatabaseManager.initSchema()) {
        AppLog.error("main", "Startup aborted: failed to initialize SQLite schema.");
        return 1;
    }

    let port = Env.port();
    let app = Router.buildApp();

    AppLog.info("main", "ToDoList SP backend starting on http://127.0.0.1:${port}");
    let started = app.run(port);
    if (!started) {
        AppLog.error("main", "Failed to bind HTTP server on port ${port}.");
        return 1;
    }
    return 0;
}
