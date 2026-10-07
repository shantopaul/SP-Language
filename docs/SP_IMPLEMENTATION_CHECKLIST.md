# SP Language — Master Implementation Checklist

**Document ID:** SP-CHK-001  
**Status:** Living Engineering Plan  
**Target:** Implementation Roadmap Progression  

---

## 1. Compiler Subsystem (`compiler/`)

### Memory Management & Safety
- [ ] Implement `EscapeAnalysis` pass in semantic analysis (`compiler/src/sema/EscapeAnalysis.cpp`)
- [ ] Add `sp_rt_retain` and `sp_rt_release` lowering in `IRBuilder.cpp` for class instances
- [ ] Implement scope-exit destructor calling (RAII drops) in `IRBuilder.cpp`
- [ ] Add Short String Optimization (SSO) representation in IR
- [ ] Add compiler support for weak references (`weak ClassName`)

### LLVM Backend Integration
- [ ] Migrate `LLVMGenerator.cpp` from `std::system("clang++ ...")` to direct `llvm::TargetMachine` C-API
- [ ] Emit object files (`.o` / `.obj`) directly into memory buffers
- [ ] Integrate embedded LLD linker (`lld::coff`, `lld::elf`, `lld::macho`)
- [ ] Add DWARF and PDB debug symbol emission using `llvm::DIBuilder`
- [ ] Add basic IR optimization passes (`mem2reg`, dead code elimination)

### Type System & Language Semantics
- [ ] Implement tagged union / algebraic data types for `enum Result<T, E>` and `Option<T>`
- [ ] Add syntactic `?` operator lowering for early error returns
- [ ] Formalize interface dynamic dispatch tables (itable layout)
- [ ] Add `#[repr(C)]` memory layout attribute for structs
- [ ] Support raw unmanaged pointers (`ptr<T>`) within `unsafe { ... }` blocks

---

## 2. Runtime Subsystem (`runtime/`)

- [ ] Split `Runtime.cpp` into modular files (`memory.cpp`, `strings.cpp`, `io.cpp`, `threading.cpp`)
- [ ] Integrate **mimalloc** / high-performance memory allocator
- [ ] Implement 16-byte value string structure with SSO and slicing (`string_view`)
- [ ] Implement thread pool and work-stealing task queue in `runtime/src/threading.cpp`
- [ ] Implement cross-platform non-blocking I/O event loop (epoll / IOCP / kqueue)
- [ ] Stabilize dynamic runtime C-ABI (`libsp_runtime.a` / `sp_runtime.dll`)

---

## 3. Standard Library Subsystem (`stdlib/`)

- [ ] Reimplement `List<T>` in pure SP generic code (zero C++ STL dependency)
- [ ] Reimplement `Map<K, V>` with open-addressing hash table in pure SP
- [ ] Reimplement `Set<T>` in pure SP
- [ ] Implement `Stack<T>`, `Queue<T>`, and `PriorityQueue<T>`
- [ ] Create `std.sync`: `Mutex`, `RwLock`, `Atomic<T>`, `Channel<T>`
- [ ] Create `std.net`: non-blocking TCP and UDP socket abstractions
- [ ] Create `std.json`: zero-copy streaming JSON parser & serializer
- [ ] Create `std.crypto`: SHA-256, HMAC, secure random bytes
- [ ] Create `std.http`: HTTP/1.1 client and server framework

---

## 4. Developer Tooling (`tools/`)

### Compiler Driver (`spc`)
- [ ] Add `spc bench` micro-benchmarking harness
- [ ] Add `spc bindgen` C header to SP declaration generator
- [ ] Add `--emit-asm` and `--emit-obj` flags to `spc`

### Package Manager (`spm`)
- [ ] Implement local offline package cache in `~/.sp/cache/`
- [ ] Add `spm tree` dependency visualization command
- [ ] Support workspace monorepos in `sp.toml`
- [ ] Add cryptographic package publishing flow (`spm publish`)

### Language Server (`spls`)
- [ ] Implement incremental document synchronization and caching
- [ ] Implement workspace-wide Find All References and Rename
- [ ] Add inlay type hints for inferred variables

### Debugger (`spd`)
- [ ] Complete DAP expression evaluation in debug console
- [ ] Support deep inspection of SP structs, arrays, and classes

---

## 5. Ecosystem & Infrastructure

- [ ] Finalize live publishing API on `registry.splang.shantopaul.com`
- [ ] Implement automated release artifact verification in GitHub Actions
- [ ] Expand 7-language benchmark suite with automated regression alerts
- [ ] Publish official SP VS Code extension to Visual Studio Marketplace
