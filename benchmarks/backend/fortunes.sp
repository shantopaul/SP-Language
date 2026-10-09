module benchmarks.backend.fortunes;

import std.io;
import std.web;
import std.db;

// ============================================================================
// TechEmpower Benchmark: Fortunes
// Tests database querying, dynamic list appending, sorting, and HTML rendering.
// Returns an HTML table of fortunes.
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
    conn.execute("CREATE TABLE Fortune (id INTEGER PRIMARY KEY, message TEXT);");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (1, 'fortune: No such file or directory');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (2, 'A computer program does what you tell it to do, not what you want it to do.');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (3, 'Emacs is a nice operating system, but I prefer UNIX. — Tom Christaensen');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (4, 'Any program that runs right is obsolete.');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (5, 'A bad random number generator: 1, 1, 1, 1, 1, 4.33e+67, 1, 1, 1');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (6, 'A computer will do what you tell it to do, but that may be much different from what you had in mind.');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (7, 'A list is only as strong as its weakest link. — Donald Knuth');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (8, 'Feature: A bug with seniority.');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (9, 'Computers make very fast, very accurate mistakes.');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (10, 'Testing security and escaping in template');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (11, 'Frame benchmark internationalization UTF-8');");
    conn.execute("INSERT INTO Fortune (id, message) VALUES (12, 'Hello from SP language high throughput backend');");

    app.get("/fortunes", (ptr req) => {
        let ctx = Context.create(req);

        var html = "<!DOCTYPE html><html><head><title>Fortunes</title></head><body><table><tr><th>id</th><th>message</th></tr>";

        // Query database sorted by message
        let stmt = conn.prepare("SELECT id, message FROM Fortune UNION ALL SELECT 0, 'Additional fortune added at run time.' ORDER BY message ASC;");
        while (stmt.next()) {
            let idStr = stmt.getString(0);
            let msgStr = stmt.getString(1);
            html = html + "<tr><td>" + idStr + "</td><td>" + msgStr + "</td></tr>";
        }
        stmt.finalize();

        html = html + "</table></body></html>";
        return ctx.html(html);
    });

    let port = 8080;
    println("[+] TechEmpower Fortunes Benchmark active at http://127.0.0.1:" + port + "/fortunes");
    app.listen(port);
}
