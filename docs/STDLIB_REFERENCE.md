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

File reading, writing, path manipulation, and directory management.

```sp
import std.fs;

// Write text file
FS.writeString("config.txt", "port=8080\nenv=production");

// Read text file
let content: string = FS.readString("config.txt");

// Check file existence
if (FS.exists("config.txt")) {
    let size: int = FS.fileSize("config.txt");
    println("Config size: ${size} bytes");
}
```

### Methods
- `FS.readString(path: string) -> string`
- `FS.writeString(path: string, content: string) -> bool`
- `FS.appendString(path: string, content: string) -> bool`
- `FS.exists(path: string) -> bool`
- `FS.remove(path: string) -> bool`
- `FS.fileSize(path: string) -> int`
- `FS.createDirectory(path: string) -> bool`

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
