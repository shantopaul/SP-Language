# SP Language — Runtime Subsystem Roadmap

**Document ID:** SP-RT-001  
**Target:** `runtime/` Directory & Native Dynamic Environment  

---

## 1. Current State vs. Target Architecture

Currently, `runtime/src/Runtime.cpp` serves as a monolithic runtime providing both low-level primitives (memory, asserts) and high-level wrappers for collections (using C++ STL) and OS operations.

### Target Architecture:
Split the runtime into clearly defined, lightweight layers:

```
┌────────────────────────────────────────────────────────┐
│  High-Level Standard Library in Pure SP (std.*)       │
│  List<T>, Map<K,V>, Set<T>, Channel<T>, HTTP, JSON     │
├────────────────────────────────────────────────────────┤
│  SP Core Native Runtime (libsp_runtime)                │
│  ├── Memory Manager (ARC, Mimalloc / Arena allocator)  │
│  ├── Threading & Work-Stealing Task Pool               │
│  ├── Non-Blocking Event Loop (epoll / IOCP / kqueue)   │
│  └── C-ABI Native Platform Layer                       │
└────────────────────────────────────────────────────────┘
```

---

## 2. Key Runtime Work Items

### 2.1 High-Performance Memory Allocator Integration
- **Implementation:** Integrate a battle-tested, embedded memory allocator such as **mimalloc** or **snmalloc** for multi-threaded allocation throughput that outperforms standard glibc/MSVC `malloc` by 2x–4x.
- **Arena Subsystem:** Expose `sp_rt_arena_create()` and `sp_rt_arena_destroy()` for bulk memory operations.

### 2.2 Leak-Free String Subsystem
- Introduce a 16-byte value string representation:
  ```c
  struct SPString {
      const char* data;
      uint64_t length;
  };
  ```
  - Short String Optimization (SSO): strings <= 14 bytes stored inline without heap allocation.
  - Slices (`string_view`): substrings take sub-slices with zero copying.

### 2.3 Cross-Platform Event Loop
- Integrate a lightweight asynchronous I/O driver:
  - Linux: `io_uring` or `epoll`
  - Windows: I/O Completion Ports (`IOCP`)
  - macOS: `kqueue`
