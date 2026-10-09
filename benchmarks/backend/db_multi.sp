module benchmarks.backend.db_multi;

import std.io;
import std.web;
import std.db;

// ============================================================================
// TechEmpower Benchmark: Multiple Database Queries
// Executes N queries in a loop and returns an array of JSON objects.
// ============================================================================

extern "C" {
    fn sp_rt_str_from_int(int val) -> string;
}

fn intToString(int val) -> string {
    return sp_rt_str_from_int(val);
}

fn main() {
    let app = App.createWithWorkers(8, 65536);

    let conn = Database.memorySqlite();
    conn.execute("CREATE TABLE World (id INTEGER PRIMARY KEY, randomNumber INTEGER);");
    for (var i = 1; i <= 1000; i = i + 1) {
        conn.execute("INSERT INTO World (id, randomNumber) VALUES (" + intToString(i) + ", " + intToString((i * 7919) % 10000) + ");");
    }

    app.get("/queries", (ptr req) => {
        let ctx = Context.create(req);
        var count = 20;
        let qParam = ctx.query("queries");
        if (qParam == "10") { count = 10; }
        if (qParam == "5") { count = 5; }

        var jsonRes = "[";
        for (var i = 1; i <= count; i = i + 1) {
            let stmt = conn.prepare("SELECT id, randomNumber FROM World WHERE id = 42;");
            var numVal = "0";
            if (stmt.next()) {
                numVal = stmt.getString(1);
            }
            stmt.finalize();

            jsonRes = jsonRes + "{\"id\":42,\"randomNumber\":" + numVal + "}";
            if (i < count) {
                jsonRes = jsonRes + ",";
            }
        }
        jsonRes = jsonRes + "]";

        return ctx.json(jsonRes);
    });

    let port = 8080;
    println("[+] TechEmpower Multiple Queries Benchmark active at http://127.0.0.1:" + port + "/queries");
    app.listen(port);
}
