module benchmarks.backend.db_single;

import std.io;
import std.web;
import std.db;

// ============================================================================
// TechEmpower Benchmark: Single Database Query
// Tests connection acquisition, single row query by ID, and JSON serialization.
// Returns {"id": <id>, "randomNumber": <number>}.
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

    app.get("/db", (ptr req) => {
        let ctx = Context.create(req);
        let idVal = 42;
        let stmt = conn.prepare("SELECT id, randomNumber FROM World WHERE id = 42;");
        let numVal = "0";
        if (stmt.next()) {
            numVal = stmt.getString(1);
        }
        stmt.finalize();

        return ctx.json("{\"id\":" + intToString(idVal) + ",\"randomNumber\":" + numVal + "}");
    });

    let port = 8080;
    println("[+] TechEmpower Single Query Benchmark active at http://127.0.0.1:" + port + "/db");
    app.listen(port);
}
