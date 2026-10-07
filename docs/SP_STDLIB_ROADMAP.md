# SP Language — Standard Library (stdlib) Roadmap

**Document ID:** SP-STDLIB-001  
**Target:** `stdlib/` Directory & Core Modules  

---

## 1. Current State vs. Target Architecture

Currently, `stdlib/` contains 8 hybrid modules (`std.io`, `std.string`, `std.math`, `std.collections`, `std.fs`, `std.time`, `std.os`, `std.process`) which primarily wrap `extern "C"` functions implemented in `Runtime.cpp`.

### Target Architecture:
Standard library modules should be written in **pure, idiomatic SP code** wherever possible, calling into runtime C-APIs only for bare-metal OS operations.

---

## 2. Standard Library Modular Blueprint

```
stdlib/
├── std.io          ── Console I/O, buffered readers, formatters
├── std.string      ── UTF-8 string manipulation, regex, StringBuilder
├── std.math        ── Numerics, trigonometry, big integers, fast math
├── std.collections ── List<T>, Map<K,V>, Set<T>, Stack<T>, Queue<T>, PriorityQueue<T>
├── std.fs          ── Path handling, files, directory trees, streams
├── std.time        ── Instant, Duration, DateTime, Timezone, Stopwatch
├── std.os          ── Environment, system metrics, hardware specs, exit
├── std.process     ── Process spawning, pipes, IPC, signals
├── std.net         ── TCP/UDP sockets, DNS resolution, non-blocking streams
├── std.json        ── High-performance JSON parser, serializer, DOM tree
├── std.crypto      ── SHA-256, HMAC, random bytes generator
└── std.sync        ── Mutex, RwLock, Semaphore, Channel<T>, Atomic<T>
```

---

## 3. High-Priority Migration: Pure SP Collections

Migrate `List<T>`, `Map<K, V>`, and `Set<T>` from C++ STL wrappers to native generic structs in SP:
```sp
module std.collections;

public struct List<T> {
    private ptr<T> data;
    private int length;
    private int capacity;

    public fn add(T item) { ... }
    public fn get(int index) -> T { ... }
    public fn size() -> int { return this.length; }
}
```
**Benefits:**
- Inlined element accesses with zero boxing.
- Cache-friendly contiguous memory buffers.
- Completely removes C++ STL from the final compiled binary.
