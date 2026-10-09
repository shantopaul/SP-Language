module benchmarks.backend.plaintext;

import std.io;
import std.web;

// ============================================================================
// TechEmpower Benchmark: Plaintext
// High-throughput raw HTTP plaintext response ("Hello, World!").
// Tests minimal HTTP framing and routing overhead.
// ============================================================================

fn main() {
    let app = App.createWithWorkers(8, 65536);

    app.get("/plaintext", (ptr req) => {
        let ctx = Context.create(req);
        return ctx.text("Hello, World!");
    });

    let port = 8080;
    println("[+] TechEmpower Plaintext Benchmark active at http://127.0.0.1:" + port + "/plaintext");
    app.listen(port);
}
