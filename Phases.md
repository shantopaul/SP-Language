# SP Programming Language - Development Phases

This document outlines the detailed development phases for the SP Programming Language project, from initial setup to the SP 1.0 production release, subsequent 1.x foundation hardening, and the SP 2.0 next-generation evolution.

---

## Strategic Direction Notice
> **Ratified Strategic Compass: Direction B (High-Performance Native Application & Cloud Language)**  
> **Approved by:** Shanto Paul (Lead Architect & Maintainer) — October 7, 2026  
> **Core Pillars:** Zero Stop-the-World GC Pause Latency, Hybrid Value Semantics + Compiler ARC, Native LLVM AOT, Pure SP Generic Collections, Cloud Microservices, CLI Developer Tools, and In-Process LLD Linking.  
> See [`/docs/v2/DECISIONS/ADR-000-strategic-direction.md`](docs/v2/DECISIONS/ADR-000-strategic-direction.md) and [`/docs/v2/SP_LANGUAGE_GOALS.md`](docs/v2/SP_LANGUAGE_GOALS.md).

---

## Phase 0: Project Setup & Research (SP 0.0) ✅
**Duration:** 1-2 weeks  
**Goal:** Set up the development environment, establish the project structure, and finalize core design decisions.

### Tasks
- [x] 1. Initialize Git repository and set up the GitHub project.
- [x] 2. Set up the CMake build system with C++17/20 support.
- [x] 3. Configure LLVM 17+ development libraries as the core backend.
- [x] 4. Set up GoogleTest/Catch2 for C++ testing.
- [x] 5. Create the initial folder structure (`compiler/`, `runtime/`, `stdlib/`, `tools/`, `tests/`, `docs/`, `examples/`).
- [x] 6. Write core project documents: `README.md`, `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `LICENSE`.
- [x] 7. Create an `sp.toml` example project configuration.
- [x] 8. Set up a GitHub Actions CI pipeline for automated building and testing.
- [x] 9. Finalize core language specification documents: `LANGUAGE_SPEC.md`, `SYNTAX_SPEC.md`, `TYPE_SYSTEM.md`, `COMPILER_ARCHITECTURE.md`.
- [x] 10. Study reference compilers (Clang, Rustc, Go compiler, Swift compiler) for architectural insights.

### Deliverables
- Working CMake build system.
- Functional GitHub Actions CI pipeline.
- Complete specification documents.
- Development environment fully configured and ready.

### Key Files Affected
- `CMakeLists.txt`, `.github/workflows/ci.yml`
- `README.md`, `CONTRIBUTING.md`, `sp.toml`
- `docs/LANGUAGE_SPEC.md`, `docs/SYNTAX_SPEC.md`, `docs/TYPE_SYSTEM.md`

### Testing Requirements
- CI pipeline successfully builds the empty project.
- CMake can build and run a dummy C++ test.

### Success Criteria
- All developers can clone, build, and run tests locally and in CI without manual configuration steps. [100% VERIFIED]

---

## Phase 1: Lexer (SP 0.1) ✅
**Duration:** 2-3 weeks  
**Goal:** Convert SP source code into a reliable stream of typed tokens.

### Tasks
- [x] 1. Define `TokenType` enum including all keywords, operators, literals, and punctuation.
- [x] 2. Define `Token` struct to hold type, value, line, column, and file information.
- [x] 3. Implement the `Lexer` class with character-by-character scanning logic.
- [x] 4. Handle single-line (`//`) and multi-line (`/* */`) comments.
- [x] 5. Handle documentation comments (`///`).
- [x] 6. Handle string literals with escape sequences and interpolation (`"Hello ${name}"`).
- [x] 7. Handle character literals (`'A'`).
- [x] 8. Handle numeric literals (integers, floats, hex, binary).
- [x] 9. Handle all operators, ensuring correct parsing of multi-character operators (e.g., `==`, `->`, `??`).
- [x] 10. Track precise source location (file, line, column) for every token to enable accurate error reporting.
- [x] 11. Implement `DiagnosticEngine` for generating human-friendly error messages.

### Deliverables
- Complete Lexer capable of tokenizing any valid SP source file.
- Error reporting infrastructure for invalid tokens.

### Key Files Affected
- `compiler/include/sp/lexer/TokenType.h`, `compiler/src/lexer/TokenType.cpp`
- `compiler/include/sp/lexer/Token.h`, `compiler/src/lexer/Token.cpp`
- `compiler/include/sp/lexer/Lexer.h`, `compiler/src/lexer/Lexer.cpp`
- `compiler/include/sp/diagnostics/DiagnosticEngine.h`

### Testing Requirements
- Unit tests for every token type.
- Golden tests: mapping `.sp` input files to expected token streams.
- 100% test coverage on lexer components.

### Success Criteria
- The lexer correctly identifies all valid language constructs and gracefully reports errors for invalid characters without crashing. [100% VERIFIED]

---

## Phase 2: Parser & AST (SP 0.2) ✅
**Duration:** 4-6 weeks  
**Goal:** Parse token streams into a well-structured Abstract Syntax Tree (AST).

### Tasks
- [x] 1. Define all AST node types (~30 types) including Program, Declarations, Statements, and Expressions.
- [x] 2. Implement the AST visitor pattern (`ASTVisitor` interface) for tree traversal.
- [x] 3. Implement an AST printer for debugging and visual verification.
- [x] 4. Build a hand-written recursive descent parser for declarations and statements.
- [x] 5. Implement a Pratt parser for handling complex expression precedence.
- [x] 6. Parse variable and function declarations.
- [x] 7. Parse control flow constructs (if/else, for, while, do-while, switch, match).
- [x] 8. Parse object-oriented constructs (classes, interfaces, structs, enums).
- [x] 9. Parse module and import declarations.
- [x] 10. Parse array and map literals.
- [x] 11. Implement error recovery mechanisms so parsing continues after encountering syntax errors.

### Deliverables
- Fully functional Parser converting any valid token stream into an AST.
- Complete set of AST classes with full visitor support.

### Key Files Affected
- `compiler/include/sp/ast/*`, `compiler/src/ast/*`
- `compiler/include/sp/parser/*`, `compiler/src/parser/*`

### Testing Requirements
- Unit tests for every grammar rule.
- Golden tests: parsing valid `.sp` source files and asserting the AST structure.
- Negative tests: ensuring invalid syntax produces accurate, clear error diagnostics.

### Success Criteria
- Parser handles all valid SP syntax combinations and reports precise line/column locations on syntax errors. [100% VERIFIED]

---

## Phase 3: Semantic Analysis & Type System (SP 0.3) ✅
**Duration:** 4-6 weeks  
**Goal:** Verify program correctness, resolve symbols, and enforce the static type system.

### Tasks
- [x] 1. Build the symbol table data structure with support for lexical scoping.
- [x] 2. Implement the `NameResolver` pass to resolve all identifiers to their declarations.
- [x] 3. Detect and report undeclared variables and duplicate declarations.
- [x] 4. Implement the core type representation (`Type` base class, primitive types, complex types).
- [x] 5. Implement the `TypeChecker` pass to validate expressions and statements.
- [x] 6. Implement type inference for `let` and `var` declarations.
- [x] 7. Validate function call signatures (argument types, count, return types).
- [x] 8. Enforce compile-time null safety (`?` types, null checking, null-coalescing).
- [x] 9. Validate class inheritance, method overriding, and interface implementations.
- [x] 10. Check control flow reachability and ensure all code paths return values where required.

### Deliverables
- Multi-pass semantic analyzer with symbol table management.
- Type checker covering all core types and inference rules.

### Key Files Affected
- `compiler/include/sp/sema/*`, `compiler/src/sema/*`

### Testing Requirements
- Unit tests for symbol resolution across various scope depths.
- Unit tests for type checking, subtyping, and implicit conversions.
- Type inference validation tests.
- Negative tests for type errors and null-safety violations.

### Success Criteria
- Semantic analysis catches all semantic errors before code generation, with zero false positives on valid programs. [100% VERIFIED]

---

## Phase 4: Intermediate Representation (IR) (SP 0.4) ✅
**Duration:** 3-4 weeks  
**Goal:** Lower the typed AST into a custom, linear Intermediate Representation (SP IR).

### Tasks
- [x] 1. Design the SP IR instruction set (SSA-based, basic blocks, control flow graph).
- [x] 2. Implement `IRModule`, `IRFunction`, `IRBasicBlock`, and `IRInstruction` classes.
- [x] 3. Implement the `IRBuilder` class to lower AST nodes into SP IR instructions.
- [x] 4. Implement `IRPrinter` to serialize IR into human-readable text for debugging.
- [x] 5. Lower variables, expressions, and arithmetic operations to IR.
- [x] 6. Lower control flow structures (if/else, loops, switch, match) into basic blocks and branch instructions.
- [x] 7. Lower function declarations, calls, and return statements.
- [x] 8. Lower arrays, maps, and indexing operations.
- [x] 9. Lower object creation, field access, and method invocations.

### Deliverables
- Complete SP IR data structures and instruction set.
- AST-to-IR lowering pass (`IRBuilder`).
- IR text printer for visual inspection.

### Key Files Affected
- `compiler/include/sp/ir/*`, `compiler/src/ir/*`

### Testing Requirements
- Unit tests verifying correct IR generation for every AST construct.
- Round-trip tests comparing emitted IR text against expected snapshots.

### Success Criteria
- Valid ASTs translate into well-formed SSA-form SP IR with verified control flow graphs. [100% VERIFIED]

---

## Phase 5: Code Generation & LLVM Backend (SP 0.5) ✅
**Duration:** 4-6 weeks  
**Goal:** Translate SP IR into LLVM IR and compile to native machine code.

### Tasks
- [x] 1. Integrate LLVM text code generation pipeline (`LLVMGenerator`).
- [x] 2. Map SP types to LLVM types (`i32`, `i64`, `double`, `i1`, `ptr`).
- [x] 3. Translate SP IR instructions to LLVM IR instructions.
- [x] 4. Implement function code generation (declarations, bodies, parameter passing).
- [x] 5. Implement class layouts, member field offsets, and Virtual Method Tables (`VTableBuilder`).
- [x] 6. Implement closures and lambda lowering to environment structs.
- [x] 7. Configure LLVM optimization levels (`-O0`, `-O1`, `-O2`, `-O3`).
- [x] 8. Build native object file compilation and link step via Clang driver.
- [x] 9. Support target architecture triples (Windows MSVC/GNU, Linux x86_64, macOS arm64).

### Deliverables
- Working code generator emitting valid LLVM IR.
- Compiler driver executing end-to-end compilation from `.sp` source to native executable binary.

### Key Files Affected
- `compiler/include/sp/codegen/*`, `compiler/src/codegen/*`
- `compiler/src/ir/IRBuilder.cpp` (4,777 lines)

### Testing Requirements
- End-to-end compilation tests: compile `.sp` file, run resulting `.exe`, assert stdout.
- 43 native execution tests in `ExecutionTest.cpp` compiling and executing native binaries.

### Success Criteria
- "Hello, SP!" and complex algorithmic programs compile to native executables that run successfully on host OS. [100% VERIFIED]

---

## Phase 6: Standard Library - Core Modules (SP 0.6) ✅
**Duration:** 3-4 weeks  
**Goal:** Implement the fundamental standard library modules required by every SP program.

### Tasks
- [x] 1. Implement `std.io`: console print, println, readLine, formatted output, eprint, eprintf.
- [x] 2. Implement `std.string`: length, substring, concat, indexOf, replace, split, trim, case conversion.
- [x] 3. Implement `std.math`: trigonometric functions, powers, roots, rounding, abs, min, max, constants (PI, E).
- [x] 4. Implement `std.collections`: `List<T>`, `Map<K, V>`, `Set<T>` native runtime bindings.
- [x] 5. Implement the runtime C/C++ bridge (`Runtime.cpp`) to provide low-level OS operations.
- [x] 6. Support array slicing and collection iteration (`for-in`).

### Deliverables
- Functional core standard library modules.
- Native C++ runtime library (`sp_runtime`).

### Key Files Affected
- `runtime/include/sp/runtime/Runtime.h`, `runtime/src/Runtime.cpp`
- `stdlib/io/io.sp`, `stdlib/string/string.sp`, `stdlib/math/math.sp`, `stdlib/collections/collections.sp`

### Testing Requirements
- Unit tests for every stdlib function in `sp_tests.exe`.
- End-to-end SP programs exercising `std.io`, `std.string`, `std.math`, and `std.collections`.

### Success Criteria
- Standard programs can perform I/O, string manipulations, math calculations, and collection operations without manual external libraries. [100% VERIFIED]

---

## Phase 7: Standard Library - Advanced Modules (SP 0.7) ✅
**Duration:** 4-6 weeks  
**Goal:** Expand standard library with system-level and platform capabilities.

### Tasks
- [x] 1. Implement `std.fs`: file reading, writing, appending, existence checks, directory operations.
- [x] 2. Implement `std.time`: clock, high-resolution timestamps, sleep, duration representations.
- [x] 3. Implement `std.os`: environment variables, command-line arguments, platform queries, exit codes.
- [x] 4. Implement `std.process`: process spawning, execution, stdout capture, exit code retrieval.

### Deliverables
- Advanced standard library modules for system interaction.
- Runtime support for cross-platform OS APIs (Windows Win32, POSIX).

### Key Files Affected
- `stdlib/fs/fs.sp`, `stdlib/time/time.sp`, `stdlib/os/os.sp`, `stdlib/process/process.sp`
- `runtime/src/Runtime.cpp` (extended OS syscalls)

### Testing Requirements
- File I/O tests (create, read, append, delete temporary files).
- Process execution tests (spawn child process and verify exit code).
- Platform and environment query tests.

### Success Criteria
- SP programs can interact with the host operating system, filesystem, and external processes reliably. [100% VERIFIED]

---

## Phase 8: Package Manager & Build Tool (spm) (SP 0.8) ✅
**Duration:** 4-6 weeks  
**Goal:** Provide an integrated package manager and build orchestrator for SP projects.

### Tasks
- [x] 1. Design and implement `sp.toml` project manifest parser.
- [x] 2. Implement `spm init` to scaffold new applications and libraries.
- [x] 3. Implement `spm add` and `spm remove` to manage dependencies.
- [x] 4. Implement SemVer version constraint resolution (`DependencyResolver`).
- [x] 5. Implement `sp.lock` lockfile generation and verification with SHA-256 cryptographic checksums.
- [x] 6. Implement `spm build` and `spm run` orchestrating compiler invocations.
- [x] 7. Implement `spm clean` and target directory cleanup.

### Deliverables
- Standalone `spm` CLI executable.
- Project scaffolding templates.
- Lockfile specification and resolution engine.

### Key Files Affected
- `tools/spm/*` (BuildOrchestrator, DependencyResolver, Lockfile, ProjectConfig, SemVer, main)
- `tests/spm/*`

### Testing Requirements
- Unit tests covering TOML configs, SemVer constraints, and lockfile roundtrips.
- Integration tests scaffolding, adding dependencies, and building projects.

### Success Criteria
- Users can create a project, add an external dependency, and build the project successfully using only `spm`. [100% VERIFIED]

---

## Phase 9: Developer Tools (SP 0.9) ✅
**Duration:** 4-6 weeks  
**Goal:** Build a modern developer experience with formatting, linting, IDE support, and debugging.

### Tasks
- [x] 1. Implement `spc fmt` for standardized code formatting.
- [x] 2. Implement `spc test` as a native test runner for SP test blocks.
- [x] 3. Implement `spc doc` to generate HTML/Markdown documentation from `///` comments.
- [x] 4. Build the SP Language Server (`spls`) implementing JSON-RPC 2.0 LSP protocol.
- [x] 5. Build official VS Code Extension with TextMate grammar and language configuration.
- [x] 6. Implement Debug Adapter Protocol (`spd`) daemon for native debugging.

### Deliverables
- Working `spc fmt`, `spc test`, `spc doc` CLI commands.
- Standalone `spls.exe` Language Server.
- Official VS Code extension in `editors/vscode/`.
- Standalone `spd.exe` DAP debugger daemon.

### Key Files Affected
- `tools/fmt/*`, `tools/test/*`, `tools/doc/*`, `tools/spls/*`, `tools/debugger/*`
- `editors/vscode/*`
- `tests/fmt/*`, `tests/test/*`, `tests/doc/*`, `tests/spls/*`, `tests/debugger/*`

### Testing Requirements
- Unit tests for formatter idempotency.
- LSP integration tests simulating IDE JSON-RPC requests.
- DAP debugger tests for breakpoints, stepping, and inspection.
- 218 automated tests in `sp_tests.exe` with 100% pass rate.

### Success Criteria
- A developer can write, format, test, and debug SP code comfortably inside VS Code with autocomplete and hover support. [100% VERIFIED]

---

## Phase 10: Production Release (SP 1.0) ✅
**Duration:** 4-8 weeks  
**Goal:** Deliver a stable, performant, and well-documented 1.0 release of the SP Programming Language.

### Tasks
- [x] 1. Stabilize the language specification and syntax guides (1 to 100 complete serial examples).
- [x] 2. Build official production release packaging script (`tools/package_release.ps1`).
- [x] 3. Complete and deploy the official documentation website (`docs.splang.shantopaul.com`).
- [x] 4. Launch official website (`splang.shantopaul.com`) and package registry (`registry.splang.shantopaul.com`).
- [x] 5. Deploy one-line automated PowerShell installer (`install.ps1`).
- [x] 6. Publish official GitHub release v1.0.0 with cryptographic SHA-256 verification.
- [x] 7. Complete comprehensive 7-language benchmarks (SP, C, C++, Rust, Java, Node.js, Python).
- [x] 8. Add Complete Syntax Guide (1–100) interactive page with instant search and category filters.

### Deliverables
- SP 1.0.0 production toolchain archives (`sp-1.0.0-windows-x64.zip`, `install.ps1`).
- 3 live Next.js websites (Main, Docs, Registry).
- Complete 100-example syntax reference.

### Key Files Affected
- `SP-Syntax-Guide.md` (5,514 lines, 100 sections)
- `splang/*`, `splang-docs/*`, `splang-registry/*`
- `install.ps1`, `tools/package_release.ps1`

### Testing Requirements
- 218/218 automated tests passing in CTest.
- All 3 Next.js applications build statically with 100% route pass rate (9/9, 37/37, 25/25).

### Success Criteria
- SP Language v1.0.0 is live, publicly distributable, and capable of native execution out of the box. [100% VERIFIED]

---

# 🚀 Next-Generation Evolution: SP 1.x through SP 2.0 (Direction B)

---

## Phase 11: Memory Model Formalization & Leak-Free Runtime (SP 1.1) ✅
**Duration:** 4-6 weeks  
**Goal:** Eliminate unmanaged runtime memory leaks by implementing **Hybrid Value Semantics + Compiler-Automated Reference Counting (ARC)** and deterministic scope-exit destructors.  
**Strategic Reference:** [`ADR-001: Memory Management Architecture`](docs/v2/DECISIONS/ADR-001-memory-model.md) & [`SP_MEMORY_MODEL_OPTIONS.md`](docs/v2/SP_MEMORY_MODEL_OPTIONS.md)

### Tasks
- [x] 1. Add `SPObjectHeader` (strong_count, weak_count, dtor_ptr) to every heap-allocated `class` instance.
- [x] 2. Implement `sp::sema::EscapeAnalysis` pass to determine when a reference does not escape its enclosing stack frame.
- [x] 3. In `IRBuilder.cpp`, automatically emit `sp_rt_retain()` on variable assignment and passing.
- [x] 4. In `IRBuilder.cpp`, automatically emit `sp_rt_release()` at block scope termination and variable reassignment (deterministic RAII drop generation).
- [x] 5. Implement weak reference support (`weak ClassName`) to break cyclic data structures.
- [x] 6. Integrate high-performance multi-threaded memory allocator subsystem into `libsp_runtime` with memory metric tracking.
- [x] 7. Implement 16-byte value string representation with Short String Optimization and zero-copy string semantics.
- [x] 8. Implement Regional Memory Arenas (`std.mem.Arena`) for request-scoped batch allocations.

### Deliverables
- Leak-free SP runtime with automated deterministic ARC cleanup.
- Sub-millisecond p99 latency profile with **zero stop-the-world GC pauses**.
- High-throughput multi-threaded memory allocator and arena subsystem.

### Key Files Affected
- `compiler/include/sp/sema/EscapeAnalysis.h`, `compiler/src/sema/EscapeAnalysis.cpp`
- `compiler/src/ir/IRBuilder.cpp`
- `runtime/include/sp/runtime/Runtime.h`, `runtime/src/Runtime.cpp`
- `stdlib/mem/mem.sp`

### Testing Requirements
- Valgrind / AddressSanitizer (ASan) memory leak tests: 0 bytes leaked across 1,000,000 loop iterations.
- Scope-exit destructor order tests for nested blocks and early returns.
- Weak reference cycle-breaking tests.
- Native end-to-end execution tests (`ARCObjectAllocationAndLifecycle`, `WeakReferenceCycleBreakingNativeExecution`, `MemoryArenaNativeExecution`).

### Success Criteria
- Long-running SP backend loops run indefinitely with stable, flat RAM usage, deterministic RAII drops, and zero memory leaks. [100% VERIFIED]

---

## Phase 12: In-Process LLVM C-API & Embedded LLD Linker (SP 1.2)
**Duration:** 4-6 weeks  
**Goal:** Replace Clang shell invocation (`std::system`) with direct in-process `LLVM-C` code generation and embedded LLD linking for 5x–10x faster compilation.  
**Strategic Reference:** [`SP_COMPILER_ROADMAP.md`](docs/v2/SP_COMPILER_ROADMAP.md)

### Tasks
- [ ] 1. Link `sp_compiler` directly against LLVM C++ development libraries (`llvm::TargetMachine`, `llvm::LLVMContext`).
- [ ] 2. Lower SP IR directly to in-memory `llvm::Module` without serializing `.tmp.ll` text files to disk.
- [ ] 3. Run LLVM target-machine optimization passes (`mem2reg`, constant folding, dead code elimination, loop vectorization).
- [ ] 4. Emit native object files (`.o` / `.obj`) directly into memory buffers.
- [ ] 5. Embed the LLD linker library (`lld::coff` on Windows, `lld::elf` on Linux, `lld::macho` on macOS) inside `spc`.
- [ ] 6. Emit DWARF (Linux/macOS) and CodeView/PDB (Windows) debug information via `llvm::DIBuilder`.
- [ ] 7. Eliminate the external Clang requirement, allowing `spc` to run completely standalone on any machine.

### Deliverables
- Standalone `spc` compiler with zero external toolchain dependencies.
- Compilation speed increase: cold builds in `< 80ms` (down from 300ms).
- Full native source-level debugging in VS Code with exact variable names and line stepping.

### Key Files Affected
- `compiler/src/codegen/LLVMGenerator.cpp`
- `compiler/src/codegen/ObjectEmitter.cpp` (new)
- `tools/spc/main.cpp`
- `CMakeLists.txt` (LLVM & LLD linking)

### Testing Requirements
- Compilation benchmark tests asserting sub-100ms build times.
- Native debugger stepping tests verifying PDB/DWARF symbol resolution.
- Clean compilation on clean machines lacking Clang/GCC in PATH.

### Success Criteria
- `spc build` generates fully functional native `.exe` binaries on any Windows/Linux/macOS machine without requiring external compilers.

---

## Phase 13: Pure SP Generic Standard Collections (SP 1.3)
**Duration:** 3-5 weeks  
**Goal:** Self-host high-level collections in **pure SP generic code**, completely removing C++ STL dependencies (`std::vector`, `std::unordered_map`) from `Runtime.cpp`.  
**Strategic Reference:** [`SP_STDLIB_ROADMAP.md`](docs/v2/SP_STDLIB_ROADMAP.md)

### Tasks
- [ ] 1. Reimplement `List<T>` as a pure SP generic value struct with contiguous buffer allocation, dynamic growth, and index bounds checking.
- [ ] 2. Reimplement `Map<K, V>` as an unboxed, open-addressing hash table in pure SP with Robin Hood hashing.
- [ ] 3. Reimplement `Set<T>` in pure SP utilizing the hash table backend.
- [ ] 4. Implement `Stack<T>`, `Queue<T>`, and `PriorityQueue<T>` in `std.collections`.
- [ ] 5. Remove `SPListImpl`, `SPMapImpl`, and `SPSetImpl` from `Runtime.cpp`.
- [ ] 6. Enable LLVM inline optimization passes across collection operations (`.add()`, `.get()`, `.contains()`).

### Deliverables
- Self-hosted `stdlib/collections/collections.sp` with unboxed memory layouts.
- Zero C++ STL footprint in final compiled binaries.
- 2x–3x faster collection lookup and iteration benchmarks.

### Key Files Affected
- `stdlib/collections/collections.sp` (full generic implementation)
- `runtime/src/Runtime.cpp` (stripping C++ STL wrapper code)
- `tests/codegen/ExecutionTest.cpp`

### Testing Requirements
- Comprehensive collection tests (100,000 element insertion, deletion, lookups, resizing).
- Monomorphization tests across custom structs and primitive types.
- Benchmarks comparing SP `List<T>` and `Map<K, V>` against C++ `std::vector` and `std::unordered_map`.

### Success Criteria
- All standard collection operations compile to specialized, unboxed machine instructions with zero C++ runtime overhead.

---

## Phase 14: Modern Typed Error Handling & C-ABI FFI (SP 1.4)
**Duration:** 4-6 weeks  
**Goal:** Implement zero-cost algebraic `Result<T, E>` and `Option<T>` with the syntactic `?` operator, and provide zero-overhead C-ABI interoperability.  
**Strategic Reference:** [`ADR-002: Error Handling`](docs/v2/DECISIONS/ADR-002-error-handling.md) & [`ADR-004: FFI`](docs/v2/DECISIONS/ADR-004-ffi.md)

### Tasks
- [ ] 1. Add compiler support for tagged unions / enums with payloads (`enum Result<T, E> { Ok(T), Err(E) }`).
- [ ] 2. Lower `Result<T, E>` to a discriminator tag + union payload struct with zero heap allocation.
- [ ] 3. Implement the syntactic `?` operator in Parser and IRBuilder for early-return error propagation.
- [ ] 4. Add `#[repr(C)]` attribute for structs to guarantee byte-for-byte platform C-ABI layout.
- [ ] 5. Implement raw pointer types (`ptr<T>`) and pointer dereferencing within explicit `unsafe { ... }` blocks.
- [ ] 6. Implement `spc bindgen` CLI tool to parse `.h` C header files via libclang and emit SP `extern "C"` declarations.
- [ ] 7. Update standard library I/O and file system APIs to return `Result<T, IOError>`.

### Deliverables
- Crash-free, zero-cost error propagation across all standard library APIs.
- Seamless C interoperability (SQLite, OpenSSL, raylib callable with zero glue code).
- Automated C header binding tool (`spc bindgen`).

### Key Files Affected
- `compiler/include/sp/ast/Expression.h`, `compiler/src/parser/Parser.cpp` (`?` operator)
- `compiler/src/sema/TypeChecker.cpp` (Result & Option validation)
- `compiler/src/ir/IRBuilder.cpp` (early-return lowering)
- `tools/spc/Bindgen.cpp` (new)
- `stdlib/fs/fs.sp`, `stdlib/io/io.sp` (migrating to Result)

### Testing Requirements
- Unit tests for `?` operator on successful and error return paths.
- FFI integration tests linking and calling native C libraries (e.g. SQLite3).
- C struct layout alignment verification tests matching native `sizeof()` and `alignof()`.

### Success Criteria
- A developer can interact with native C libraries without writing C++ wrappers, and handle I/O errors cleanly using `?` with zero crashes.

---

## Phase 15: Concurrency, Thread Pool & Lock-Free Channels (SP 1.5)
**Duration:** 4-6 weeks  
**Goal:** Implement high-throughput concurrent processing with a cross-platform work-stealing thread pool, message-passing channels, and atomic memory operations.  
**Strategic Reference:** [`ADR-003: Concurrency`](docs/v2/DECISIONS/ADR-003-concurrency.md) & [`SP_CONCURRENCY_OPTIONS.md`](docs/v2/SP_CONCURRENCY_OPTIONS.md)

### Tasks
- [ ] 1. Implement cross-platform OS thread abstractions (`std.sync.Thread`).
- [ ] 2. Implement a high-performance work-stealing thread pool in `libsp_runtime`.
- [ ] 3. Implement lock-free, typed message-passing channels (`Channel<T>`) supporting `send()`, `recv()`, and buffered capacities.
- [ ] 4. Implement synchronization primitives in `std.sync`: `Mutex`, `RwLock`, `Semaphore`, `ConditionVariable`.
- [ ] 5. Implement lock-free atomic primitives (`AtomicInt`, `AtomicBool`, `AtomicPtr`) utilizing LLVM atomic instructions (`atomicrmw`, `cmpxchg`).
- [ ] 6. Enforce memory safety rules preventing data races across thread boundaries.

### Deliverables
- `std.sync` standard library module.
- High-throughput thread pool and message-passing channels.
- Multi-threaded computational benchmark demonstrating linear CPU core scaling.

### Key Files Affected
- `stdlib/sync/sync.sp` (new)
- `runtime/src/threading/ThreadPool.cpp` (new)
- `runtime/src/threading/Channel.cpp` (new)
- `compiler/src/ir/IRBuilder.cpp` (lowering atomics)

### Testing Requirements
- Concurrent stress tests: 1,000,000 channel messages passed across 16 threads with 0 drops or deadlocks.
- Mutex race-condition tests asserting atomic counter consistency under heavy contention.
- Thread-pool task scheduling and load-balancing benchmarks.

### Success Criteria
- Multi-threaded SP applications efficiently utilize all CPU cores with safe, deadlock-free channel communication.

---

## Phase 16: Non-Blocking Networking, JSON & HTTP Engine (SP 1.6)
**Duration:** 5-7 weeks  
**Goal:** Provide the core foundations for high-scale cloud microservices: non-blocking network sockets, zero-copy streaming JSON, and an enterprise HTTP/1.1 and HTTP/2 web server framework.  
**Strategic Reference:** [`SP_STDLIB_ROADMAP.md`](docs/v2/SP_STDLIB_ROADMAP.md) & [`SP_LANGUAGE_GOALS.md`](docs/v2/SP_LANGUAGE_GOALS.md)

### Tasks
- [ ] 1. Implement `std.net` cross-platform non-blocking socket driver (IOCP on Windows, epoll on Linux, kqueue on macOS).
- [ ] 2. Implement TCP client, TCP listener, UDP socket, and DNS resolution in `std.net`.
- [ ] 3. Implement `std.json`: zero-allocation streaming tokenizer, DOM tree parser, and compile-time reflection serializer.
- [ ] 4. Implement `std.crypto`: SHA-256, HMAC, and cryptographically secure random bytes.
- [ ] 5. Implement `std.http`: high-performance HTTP/1.1 and HTTP/2 client and server with request routing, middleware, and streaming responses.
- [ ] 6. Build the reference REST microservice benchmark achieving 85,000+ requests/sec on a standard node.

### Deliverables
- `std.net`, `std.json`, `std.crypto`, and `std.http` modules.
- Production-grade HTTP server framework capable of powering enterprise cloud APIs.
- Official tech-stack benchmarks proving throughput on par with Go and Rust.

### Key Files Affected
- `stdlib/net/*`, `stdlib/json/*`, `stdlib/crypto/*`, `stdlib/http/*`
- `runtime/src/net/EventLoop.cpp` (IOCP/epoll integration)

### Testing Requirements
- HTTP conformance tests (pipelining, chunked transfer encoding, headers, keep-alive).
- JSON round-trip compliance tests against JSON Schema and RFC 8259 suites.
- Load testing with `wrk` / `k6` validating high throughput and zero memory leaks under load.

### Success Criteria
- An SP developer can spin up a microservice in `< 30` lines of code that boots in 2ms and handles 85k+ req/sec with flat RAM usage.

---

## Phase 17: Package Registry GA, Supply Chain Security & Tooling (SP 1.7)
**Duration:** 4-6 weeks  
**Goal:** Launch the production Package Registry (`registry.splang.shantopaul.com`), enforce end-to-end supply chain security, and harden the developer tooling suite.  
**Strategic Reference:** [`ADR-005: Package Registry`](docs/v2/DECISIONS/ADR-005-package-system.md) & [`SP_TOOLING_ROADMAP.md`](docs/v2/SP_TOOLING_ROADMAP.md)

### Tasks
- [ ] 1. Finalize the public registry publishing API on `registry.splang.shantopaul.com` with API token authentication.
- [ ] 2. Implement `spm publish` CLI command with automated tarball generation and checksum hashing.
- [ ] 3. Implement local offline package cache in `~/.sp/cache/packages/` for instant builds without internet access.
- [ ] 4. Implement `spm tree` displaying transitive dependency trees and version conflict diagnostics.
- [ ] 5. Implement workspace monorepos in `sp.toml` for multi-package projects.
- [ ] 6. Upgrade `spls` (LSP) with incremental document synchronization, workspace-wide symbol indexing, and inlay type hints.
- [ ] 7. Implement `spc bench` micro-benchmarking harness with automated statistical regression tracking.
- [ ] 8. Publish the official SP VS Code Extension to the Visual Studio Marketplace.

### Deliverables
- Live, secure public package publishing and distribution ecosystem.
- Hardened LSP and DAP debugging experience in VS Code.
- Built-in benchmarking harness (`spc bench`).

### Key Files Affected
- `tools/spm/Publish.cpp`, `tools/spm/Cache.cpp`
- `tools/spls/*` (incremental indexer)
- `tools/spc/Benchmark.cpp` (new)
- `splang-registry/*` (production edge API deployment)

### Testing Requirements
- Package publish, download, and integrity verification integration tests.
- Malicious package protection tests (immutable versions, path traversal prevention).
- LSP stress testing across multi-package workspaces with 100,000+ lines of code.

### Success Criteria
- Third-party developers can publish, discover, and install community packages securely via `spm`, with full IDE support in VS Code.

---

## Phase 18: SP 2.0 Evolution, Comptime & Language Editions (SP 2.0)
**Duration:** 8-12 weeks  
**Goal:** Deliver the mature, self-sustaining SP 2.0 platform featuring language editions, compile-time function execution (`comptime`), self-hosting, and WebAssembly support.  
**Strategic Reference:** [`SP_LANGUAGE_GOALS.md`](docs/v2/SP_LANGUAGE_GOALS.md) & [`SP_LANGUAGE_VISION.md`](docs/v2/SP_LANGUAGE_VISION.md)

### Tasks
- [ ] 1. Implement the **Language Edition Mechanism** (`edition = "2026"`, `edition = "2028"` in `sp.toml`), ensuring perpetual backward compatibility across ecosystem crates.
- [ ] 2. Implement **Compile-Time Function Execution (`comptime`)**, allowing pure SP functions to execute during compilation to generate constants, lookup tables, and type assertions.
- [ ] 3. Implement the **Self-Hosted SP Compiler Frontend**, writing the lexer, parser, and semantic analyzer entirely in SP.
- [ ] 4. Implement **First-Class WebAssembly Target (`wasm32-wasi`)**, enabling SP programs to compile directly for serverless edge workers and browser runtimes.
- [ ] 5. Implement structured `async`/`await` coroutine transformation for zero-allocation cooperative concurrency.
- [ ] 6. Publish the formal SP 2.0 Language Specification and Migration Guide (`spc fix`).

### Deliverables
- SP 2.0.0 General Availability release.
- Self-hosted compiler front-end written in SP.
- WebAssembly (WASI) official compilation target.
- Edition migration tool (`spc fix`).

### Key Files Affected
- `compiler/src/sema/ComptimeEvaluator.cpp` (new)
- `compiler/self_hosted/` (SP-in-SP compiler sources)
- `tools/spc/main.cpp` (WASM target support)
- Documentation and Specification suite.

### Testing Requirements
- Self-hosting bootstrap verification: SP compiler compiles itself, and the resulting binary compiles the test suite with 100% parity.
- Edition interop tests: compiling a project combining Edition 2026 and Edition 2028 dependencies.
- WASI integration tests executing in Wasmtime and Node.js WASM runtimes.

### Success Criteria
- SP Language stands as a mature, self-hosting, enterprise-grade programming language powering production cloud systems and developer tooling worldwide.

---

## Phase Progression Summary

```
SP 0.0 – 1.0 (Completed Baseline) ✅
  ├── Phase 0: Setup & Research
  ├── Phase 1: Lexer
  ├── Phase 2: Parser & AST
  ├── Phase 3: Semantic Analysis & Type System
  ├── Phase 4: Intermediate Representation (IR)
  ├── Phase 5: Code Generation & LLVM Backend
  ├── Phase 6: Core Standard Library
  ├── Phase 7: Advanced Standard Library
  ├── Phase 8: Package Manager (spm)
  ├── Phase 9: Developer Tools (spls, spd, fmt, test, doc)
  └── Phase 10: Production Release 1.0 (Live Websites & Distribution)
           │
           ▼
SP 1.x Evolution (Direction B: Native Application & Cloud) 🚀
  ├── Phase 11: Memory Model Formalization & Leak-Free ARC (SP 1.1)
  ├── Phase 12: In-Process LLVM C-API & Embedded LLD Linker (SP 1.2)
  ├── Phase 13: Pure SP Generic Standard Collections (SP 1.3)
  ├── Phase 14: Typed Error Handling (Result<T,E>) & C-ABI FFI (SP 1.4)
  ├── Phase 15: Concurrency, Thread Pool & Lock-Free Channels (SP 1.5)
  ├── Phase 16: Non-Blocking Networking, JSON & HTTP Engine (SP 1.6)
  └── Phase 17: Package Registry GA, Security & Tooling Hardening (SP 1.7)
           │
           ▼
SP 2.0 Enterprise Milestone 🏆
  └── Phase 18: Language Editions, Comptime, Self-Hosting & WebAssembly (SP 2.0)
```
