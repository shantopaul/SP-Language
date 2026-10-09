module benchmarks.backend.json;

import std.io;
import std.web;

// ============================================================================
// TechEmpower Benchmark: JSON Serialization
// Tests HTTP engine JSON response serialization performance.
// Returns {"message":"Hello, World!"} with application/json content type.
// ============================================================================

fn main() {
    let app = App.createWithWorkers(8, 65536);

    app.get("/json", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.json("{\"message\":\"Hello, World!\"}");
    });

    let port = 8080;
    println("[+] TechEmpower JSON Serialization Benchmark active at http://127.0.0.1:" + port + "/json");
    app.listen(port);
}
