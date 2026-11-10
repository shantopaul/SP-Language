# SP Standard Library API Reference (v1.0.0)

The SP Standard Library is built directly into the compiler distribution, providing clean, performant, and memory-safe abstractions for common operating system and application tasks.

---

## 1. `std/io` — Input / Output & Terminal

Provides standard console output, formatted printing, and terminal stream operations.

```sp
import std.io;

fn main() -> int {
    println("Hello to stdout");
    eprintln("Error to stderr");
    print("No newline: ");
    println("appended text");
    return 0;
}
```

### Functions
- `fn print(text: string) -> void`: Write string to stdout without trailing newline.
- `fn println(text: string) -> void`: Write string to stdout with trailing newline.
- `fn eprint(text: string) -> void`: Write string to stderr without trailing newline.
- `fn eprintln(text: string) -> void`: Write string to stderr with trailing newline.
- `fn readLine() -> string`: Read one line from stdin (without the trailing newline). Returns `""` both for an empty line and for EOF — use `tryReadLine()` when the distinction matters.
- `fn tryReadLine() -> Result<string, IOError>`: Read one line from stdin. An empty line is `Ok("")`; end-of-input is `Err(IOError.UnexpectedEOF)`. A last line without a trailing newline is still `Ok(text)`. Also available as `IO.tryReadLine()`.

---

## 2. `std/math` — Mathematics & Numeric Operations

High-performance mathematical calculations and constants.

```sp
import std.math;

let root: float = Math.sqrt(144.0);     // 12.0
let power: float = Math.pow(2.0, 10.0);  // 1024.0
let maximum: int = Math.max(10, 25);     // 25
let absolute: int = Math.abs(-42);       // 42
```

### Methods
- `Math.abs(x)`: Absolute value of integer or float.
- `Math.sqrt(x: float) -> float`: Square root.
- `Math.pow(base: float, exp: float) -> float`: Exponentiation.
- `Math.min(a, b)`: Minimum of two numbers.
- `Math.max(a, b)`: Maximum of two numbers.
- `Math.sin(rad: float)`, `Math.cos(rad: float)`, `Math.tan(rad: float)`: Trigonometric functions.
- `Math.PI`: `3.141592653589793`
- `Math.E`: `2.718281828459045`

---

## 3. `std/string` — String Manipulation

```sp
import std.string;

let str: string = "  Hello, SP World!  ";
let trimmed: string = String.trim(str);
let upper: string = String.toUpperCase(trimmed);
let containsWord: bool = String.contains(trimmed, "SP");
let replaced: string = String.replace(trimmed, "World", "Developers");
```

### Methods
- `String.length(s: string) -> int`
- `String.trim(s: string) -> string`
- `String.toUpperCase(s: string) -> string`
- `String.toLowerCase(s: string) -> string`
- `String.startsWith(s: string, prefix: string) -> bool`
- `String.endsWith(s: string, suffix: string) -> bool`
- `String.contains(s: string, sub: string) -> bool`
- `String.substring(s: string, start: int, len: int) -> string`
- `String.replace(s: string, target: string, replacement: string) -> string`

---

## 4. `std/collections` — Lists, Maps, & Sets

Generic container classes for organizing application data.

```sp
import std.collections;

let list = new ArrayList<string>();
list.add("First");
list.add("Second");
println("Item count: ${list.size()}");

let map = new HashMap<string, int>();
map.put("Alice", 95);
map.put("Bob", 88);
println("Alice score: ${map.get("Alice")}");
```

- A builtin `Map<K, V>` index-read returns the plain value type
  (`map[k]`), the same as `HashMap.get()`; a missing key reads back
  the zero value — use `containsKey()` to test presence.
- A `Set<T>` prints as `{1, 2, 3}` and iterates with
  `for (x in set)`; iteration order is hash order, not insertion order.

---

## 5. `std/fs` — File System Operations

File reading, writing, path manipulation, and directory management. Every operation is available both as a free function and as a static method on `FS` (e.g. `readText(path)` and `FS.readText(path)`); `File` exposes the file operations only (no `listDir` / `createDir`).

```sp
import std.fs;

// Write text file
writeText("config.txt", "port=8080\nenv=production");

// Read text file
let content: string = readText("config.txt");

// Check file existence
if (exists("config.txt")) {
    println("Config exists");
}
```

### Methods
- `readText(path: string) -> string` / `FS.readText(path)`: Read the whole file as text. Returns `""` if the file cannot be read.
- `writeText(path: string, content: string) -> bool` / `FS.writeText(path, content)`: Overwrite (or create) a file.
- `appendText(path: string, content: string) -> bool` / `FS.appendText(path, content)`: Append to a file.
- `exists(path: string) -> bool` / `FS.exists(path)`: True if the path exists.
- `delete(path: string) -> bool` / `FS.delete(path)` (also `deleteFile`): Delete a file or directory.
- `createDir(path: string) -> bool` / `FS.createDir(path)`: Create a directory.
- `listDir(path: string) -> string[]` / `FS.listDir(path)`: Sorted entry names of a directory. Returns `[]` both for an empty directory and for an error — use `tryListDir()` when the distinction matters.
- `isFile(path: string) -> bool` / `FS.isFile(path)`: True if the path is a file.
- `isDir(path: string) -> bool` / `FS.isDir(path)`: True if the path is a directory.

### `try*` variants — `Result<T, IOError>`
The plain functions above report failure silently (`""`, `false`, `[]`). The `try*` siblings return a `Result` whose `Err` is an `IOError` (`NotFound`, `PermissionDenied`, `AlreadyExists`, `InvalidInput`, `UnexpectedEOF`, `Other`) — use them when the caller must tell *why* an operation failed.

- `tryReadText(path) -> Result<string, IOError>` / `FS.tryReadText(path)`: `Err(NotFound)` when the file does not exist, else `Ok(contents)`.
- `tryWriteText(path, content) -> Result<bool, IOError>` / `FS.tryWriteText(path, content)`
- `tryAppendText(path, content) -> Result<bool, IOError>` / `FS.tryAppendText(path, content)`
- `tryDelete(path) -> Result<bool, IOError>` / `FS.tryDelete(path)`
- `tryCreateDir(path) -> Result<bool, IOError>` / `FS.tryCreateDir(path)`
- `tryListDir(path) -> Result<string[], IOError>` / `FS.tryListDir(path)`: `Err(NotFound)` for a missing path, `Err(InvalidInput)` when the path is not a directory, else `Ok(sorted entry names)`. Note: the existence and directory checks happen before the listing, so a path that disappears between the check and the listing still yields `Ok([])`.

```sp
import std.fs;

var r = tryListDir("notes");
match r {
    Result.Ok(names) => println("entries: ${names.length()}"),
    Result.Err(e) => println("cannot list: ${e}")
}
```

> **Error-handling note (Phase 38):** only `std/fs` and `std/io` use `Result<T, IOError>` so far. The other modules keep their older conventions: `std/db` returns a result object with `isValid()` / `error()` and booleans for prepare/ping/transactions; `std/json` returns a `JsonValue` whose kind is `Error` on bad input; `std/crypto`'s `base64Decode` returns `""` on invalid input; `std/net` returns `""` on disconnect/timeout; `std/redis`'s `get` returns `""` for a missing key and for no connection alike; `std/http` returns an `HttpResponse` with `status()`; `std/os`'s `getEnv` returns `""` for a missing variable; `std/process` returns an int exit code; `std/collections`' `get` returns the zero value (`containsKey()` to test). Migrating those modules to `Result` is a per-module follow-up — their signatures do not change in this phase.

---

## 6. `std/time` — Clock, Sleep & Timestamps

> Phase 4 slice D (2026-11-09): `Time.nowMillis()` / `Time.sleepMillis()`
> do not exist (verified with `spc check`). Epoch values are `int64`
> (a 32-bit `int` truncates ~1.7e12 ms to a negative number).

```sp
import std.time;

int64 startMs = Time.now();
Time.sleep(100);
int64 duration = Time.now() - startMs;
println("Elapsed: ${duration} ms");
```

---

## 7. `std/os` & `std/process` — System Environment & Process Execution

```sp
import std.os;
import std.process;

let homeDir: string = OS.getEnv("HOME");
let platform: string = OS.platform(); // "windows", "linux", "darwin"

// OS.args() returns the process arguments, program name excluded
// (argv[1..]). Note `spc run` does not forward extra arguments;
// run a built binary to see them.
let args: string[] = OS.args();

let exitCode: int = Process.run("git --version");
println("Process exit status: ${exitCode}");
```

## 8. `std/net` — TCP Sockets & Event Loop

```sp
import std.net;

// Client: connect returns a stream (handle null when refused).
let stream = TcpStream.connect("127.0.0.1", 9000);
stream.write("hello");
let reply: string = stream.read(1024);
stream.close();

// Server: bind(..., 0) picks an ephemeral port; port() reads it back.
let listener = TcpListener.bind("127.0.0.1", 0);
let conn = listener.accept();
listener.close();
```

### Event loop (`EventLoop`)

One thread watches many sockets; `poll(timeoutMs)` returns the number of
sockets that became ready (0 on timeout). Interest is `1` for read,
`2` for write (combine with `|`); a ready event is reported with the
same bits, plus `4` for error (peer reset, socket error — a graceful
FIN close reports read/EOF only, not error).

```sp
import std.net;

fn main() {
    let listener = TcpListener.bind("127.0.0.1", 0);
    let port = listener.port();
    let loop = EventLoop.create();
    let client = TcpStream.connect("127.0.0.1", port);
    let server = listener.accept();
    loop.add(server, 1);
    println("idle ${loop.poll(0)}");
    client.write("hi");
    println("ready ${loop.poll(1000)}");
    println("data ${server.read(16)}");
}
```

Readiness is **level-triggered**: a socket with unread data keeps being
reported until it is drained. `remove(stream)` stops watching a socket;
`destroy()` frees the loop.

**Scaling (Phase 40):** on Linux the loop waits with `epoll`, so a
`poll()` call costs O(ready), not O(registered) — 1,000 idle connections
cost ~0.5 us per `poll(0)` (measured; the old `poll()` backend cost
~180 us). Other platforms use `poll()`/`WSAPoll` with the same
interface. The per-iteration timer cost is O(1) too (no timer-heap copy).
What the loop does *not* do: the HTTP server still uses an accept thread
plus a worker pool — it is not driven by this loop (a follow-up).
