export interface CodeSnippet {
  id: string;
  title: string;
  subtitle: string;
  code: string;
  output: string;
}

export const CODE_SNIPPETS: CodeSnippet[] = [
  {
    id: "web",
    title: "Web Service (std.web)",
    subtitle: "High-throughput HTTP service with Radix routing, JSON responses, and automatic OpenAPI.",
    code: `import std.io;
import std.web;

fn main() -> void {
    let app = new App();
    app.use(new Logger());
    app.use(new Cors());

    // Interactive OpenAPI Swagger UI mounted at /docs
    app.swagger("/docs", "/openapi.json", "User Cluster Service");

    app.get("/api/v1/users/:id", (ptr req) => {
        let ctx = Context.create(req);
        let id = ctx.param("id");
        return ctx.json("{\\"id\\":\\"" + id + "\\",\\"name\\":\\"Alice\\",\\"role\\":\\"admin\\",\\"verified\\":true}");
    });

    println("SP Web Platform listening on http://127.0.0.1:8080");
    app.listen(8080);
}`,
    output: `SP Web Platform listening on http://127.0.0.1:8080
[INFO] GET /api/v1/users/42 -> 200 OK (0.24ms)
[METRICS] Active connections: 1420 | Throughput: 185,420 req/s`,
  },
  {
    id: "basics",
    title: "Modern Basics",
    subtitle: "Type inference, clean string interpolation, and concise functions.",
    code: `import std.io;

fn greet(string name, int times = 2) -> void {
    for (int i = 0; i < times; i = i + 1) {
        println("Hello, \${name}! [Worker #\${i + 1}]");
    }
}

fn main() -> void {
    let language = "SP";
    let efficiency = 99.8;
    println("Welcome to \${language}! Native LLVM Efficiency: \${efficiency}%");
    greet(language, 2);
}`,
    output: `Welcome to SP! Native LLVM Efficiency: 99.8%
Hello, SP! [Worker #1]
Hello, SP! [Worker #2]`,
  },
  {
    id: "matching",
    title: "Pattern Matching",
    subtitle: "Exhaustive, expressive match constructs with value bindings.",
    code: `import std.io;

enum ServiceState {
    Starting,
    Healthy,
    Degraded,
    Stopped
}

fn evaluate(ServiceState state) -> string {
    return match (state) {
        ServiceState.Starting => "Provisioning worker coroutines...",
        ServiceState.Healthy  => "Cluster operating at peak efficiency",
        ServiceState.Degraded => "Failover replica taking active load",
        ServiceState.Stopped  => "All resources cleanly terminated"
    };
}

fn main() -> void {
    let status = ServiceState.Healthy;
    println("Telemetry Check: \${evaluate(status)}");
}`,
    output: `Telemetry Check: Cluster operating at peak efficiency`,
  },
  {
    id: "comptime",
    title: "Compile-Time Metaprogramming",
    subtitle: "Zero runtime cost with compile-time metaprogramming and constant folding.",
    code: `import std.io;

// Evaluated strictly during compiler compilation passes
comptime fn factorial(int n) -> int {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

fn main() -> void {
    // Constant folded directly into LLVM IR as an immediate scalar value 3628800
    let answer = comptime factorial(10);
    println("Precomputed Factorial(10): \${answer} (0 runtime cycles)");
}`,
    output: `Precomputed Factorial(10): 3628800 (0 runtime cycles)`,
  },
  {
    id: "database",
    title: "Database Pooling (std.db)",
    subtitle: "High-throughput thread-safe connection pooling for PostgreSQL & SQLite.",
    code: `import std.io;
import std.db;

fn main() -> void {
    // Thread-safe connection pool with automatic health check probes
    let pool = db.createPool("postgres://app:secret@db.internal:5432/production", 16);
    
    let client = pool.acquire();
    let rows = client.query("SELECT id, username, email FROM accounts WHERE active = $1", [true]);
    
    for (let row in rows) {
        println("User: \${row.get('username')} <\${row.get('email')}>");
    }
    
    client.release();
}`,
    output: `[DB] Acquired connection #3 from pool (pool size: 16)
User: shantopaul <shanto@splang.shantopaul.com>
User: dev_lead <lead@enterprise.com>
[DB] Connection #3 released cleanly in 0.8ms`,
  },
  {
    id: "coroutines",
    title: "Async Coroutines",
    subtitle: "Non-blocking async/await event loop powered by epoll and Windows IOCP.",
    code: `import std.io;
import std.sync;

async fn fetchMetrics(string nodeName) -> string {
    println("Polling telemetry node \${nodeName}...");
    return "Status: 200 OK | Latency: 0.8ms";
}

fn main() -> void {
    let handle = spawn fetchMetrics("sp-cluster-master");
    let result = await handle;
    println("Telemetry Response: \${result}");
}`,
    output: `Polling telemetry node sp-cluster-master...
Telemetry Response: Status: 200 OK | Latency: 0.8ms`,
  },
];
