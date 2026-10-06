# SP Programming Language - Development Phases

This document outlines the detailed development phases for the SP Programming Language project, from initial setup to production release and beyond.

---

## Phase 0: Project Setup & Research (SP 0.0)
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
- All developers can clone, build, and run tests locally and in CI without manual configuration steps.

---

## Phase 1: Lexer (SP 0.1)
**Duration:** 2-3 weeks
**Goal:** Convert SP source code into a reliable stream of typed tokens.

### Tasks
- [x] 1. Define `TokenType` enum including all keywords, operators, literals, and punctuation.
  - *Keywords:* fn, class, interface, struct, enum, if, else, for, while, do, switch, match, case, default, break, continue, return, import, module, new, this, null, true, false, var, let, const, public, private, protected, internal, static, abstract, override, extends, implements, async, await, try, catch, finally, throw, unsafe, extern, void, and all type keywords.
  - *Operators:* +, -, *, /, %, ==, !=, >, <, >=, <=, &&, ||, !, &, |, ^, ~, <<, >>, =, +=, -=, *=, /=, %=, ++, --, ->, =>, ?., ??
  - *Literals:* INTEGER, FLOAT, STRING, CHAR, BOOLEAN
  - *Punctuation:* (, ), {, }, [, ], ;, :, ,, ., ..., ..
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
- `compiler/include/sp/lexer/TokenType.h`
- `compiler/include/sp/lexer/Token.h`
- `compiler/include/sp/lexer/Lexer.h`
- `compiler/src/lexer/Lexer.cpp`
- `compiler/src/lexer/Token.cpp`
- `compiler/include/sp/diagnostics/DiagnosticEngine.h`

### Testing Requirements
- Write comprehensive unit tests for every token type.
- Write golden tests: mapping `.sp` input files to expected token streams.
- Achieve 100% test coverage on the lexer components.

### Success Criteria
- The lexer correctly identifies all valid language constructs and gracefully reports errors for invalid characters, without crashing.

---

## Phase 2: Parser & AST (SP 0.2)
**Duration:** 4-6 weeks
**Goal:** Parse token streams into a well-structured Abstract Syntax Tree (AST).

### Tasks
- [x] 1. Define all AST node types (~30 types) including Program, Declarations (Class, Function, Variable), Statements (If, For, Return), and Expressions (Binary, Unary, Call).
- [x] 2. Implement the AST visitor pattern (`ASTVisitor` interface) for tree traversal.
- [x] 3. Implement an AST printer for debugging and visual verification.
- [x] 4. Build a hand-written recursive descent parser for declarations and statements.
- [x] 5. Implement a Pratt parser for handling complex expression precedence.
- [x] 6. Parse variable and function declarations.
- [x] 7. Parse control flow constructs (if/else, for, while, do-while, switch, match).
- [x] 8. Parse object-oriented constructs (classes, interfaces, structs, enums).
- [x] 9. Parse module and import declarations.
- [x] 10. Parse array and map literals.
- [x] 11. Implement the Syntax Compatibility Layer:
  - Normalize both `fn add(int a, int b) -> int` and `int add(int a, int b)`.
  - Normalize `extends` and `:` for inheritance.
  - Support both `switch/case` and `match` structures.
- [x] 12. Implement comprehensive error recovery so the parser continues scanning after encountering errors.
- [x] 13. Generate detailed syntax error messages with precise source locations.

### Deliverables
- Complete Parser handling all SP syntax.
- Working Syntax Compatibility Layer.
- AST data structures and pretty-printer.

### Key Files Affected
- `compiler/include/sp/ast/*`
- `compiler/include/sp/parser/Parser.h`
- `compiler/src/parser/Parser.cpp`
- `compiler/src/parser/ExpressionParser.cpp`

### Testing Requirements
- Unit tests for every grammar production rule.
- Golden tests: `.sp` source files mapped to expected serialized AST outputs.
- Test error recovery on malformed inputs.

### Success Criteria
- The parser generates the correct AST for all syntactically valid code and reports meaningful errors (without crashing) for invalid code.

---

## Phase 3: Semantic Analysis & Type System (SP 0.3)
**Duration:** 4-6 weeks
**Goal:** Validate program semantics and ensure type correctness.

### Tasks
- [x] 1. Implement `SymbolTable` supporting nested scopes.
- [x] 2. Implement `NameResolver` to check variable existence, prevent duplicate declarations, and resolve imports.
- [x] 3. Implement the `TypeSystem`: defining primitive types, user-defined types (classes/interfaces), and compatibility rules.
- [x] 4. Support implicit type conversions (widening) and require casts for explicit conversions (narrowing).
- [x] 5. Implement the `TypeChecker`:
  - Validate declarations, function returns, binary expressions, and function calls.
  - Implement method overload resolution.
  - Implement type inference for `var`/`let`.
- [x] 6. Enforce access modifiers (`public`, `private`, `protected`, `internal`).
- [x] 7. Validate inheritance hierarchies and interface implementations.
- [x] 8. Enforce null safety rules (`?` types, `?.` access, `??` operator).
- [x] 9. Implement return path analysis to guarantee all code paths return a value.
- [x] 10. Establish a comprehensive error code system (e.g., E1000-E1009).

### Deliverables
- Complete semantic analyzer and symbol table.
- Robust type checker with inference capabilities.
- Standardized error code reporting.

### Key Files Affected
- `compiler/include/sp/sema/SymbolTable.h`
- `compiler/include/sp/sema/NameResolver.h`
- `compiler/include/sp/sema/TypeChecker.h`
- `compiler/include/sp/sema/Type.h`
- `compiler/src/sema/*`

### Testing Requirements
- Unit tests for all semantic checks and type compatibility rules.
- Golden tests: `.sp` source files mapped to expected semantic error outputs.

### Success Criteria
- All invalid operations, type mismatches, and scoping errors are caught before code generation. Valid programs pass analysis cleanly.

---

## Phase 4: SP IR & LLVM Code Generation (SP 0.4)
**Duration:** 6-8 weeks
**Goal:** Generate native executables from typed SP source code via LLVM.

### Tasks
- [x] 1. Design the high-level SP Intermediate Representation (SP IR).
- [x] 2. Implement the IR Builder translating `TypedAST` to SP IR.
- [x] 3. Implement the LLVM Code Generator translating SP IR to LLVM IR.
- [x] 4. Generate code for arithmetic, boolean, and string operations.
- [x] 5. Generate code for variable assignments, function calls, and control flow.
- [x] 6. Link generated code with the SP runtime library.
- [x] 7. Configure backend to produce native executables (.exe, ELF, Mach-O).
- [x] 8. Implement `spc build`, `spc run`, and `spc check` commands in the CLI.
- [x] 9. Support debug (with debugging symbols) and release (with LLVM optimizations) build modes.

### Deliverables
- Working native compiler backend.
- Functional `spc` command-line interface.
- MILESTONE: `fn main() { println("Hello, SP!"); }` compiles and executes natively.

### Key Files Affected
- `compiler/include/sp/codegen/IRBuilder.h`
- `compiler/include/sp/codegen/LLVMGenerator.h`
- `compiler/src/codegen/*`
- `tools/spc/main.cpp`

### Testing Requirements
- End-to-end integration tests compiling and running small programs.
- Validation of LLVM IR output against expected patterns.

### Success Criteria
- The compiler successfully builds native executables for the host platform that run without segmentation faults and produce expected output.

---

## Phase 5: Object System (SP 0.5)
**Duration:** 4-6 weeks
**Goal:** Implement full Object-Oriented Programming (OOP) support including classes, inheritance, and interfaces.

### Tasks
- [x] 1. Generate LLVM code for class declarations (vtable construction, field memory layout).
- [x] 2. Generate code for object constructors and `new` allocation.
- [x] 3. Generate code for method calls, supporting both static and virtual dispatch.
- [x] 4. Manage `this` references in instance methods.
- [x] 5. Implement vtable chaining for inheritance.
- [x] 6. Generate code for interface implementations and dynamic casting.
- [x] 7. Enforce access modifiers at the code generation level if necessary.
- [x] 8. Support abstract classes and static members.
- [x] 9. Implement basic memory management (e.g., `malloc`/`free` wrappers or initial GC hooks via `sp_rt_alloc`/`sp_rt_free`).

### Deliverables
- End-to-end working OOP features (classes, constructors, methods, virtual dispatch, structs, enums).
- Successful native compilation and execution of object-oriented SP programs.
- Basic memory management infrastructure (`sp_rt_alloc` / `sp_rt_free` with 8-byte alignment).

### Key Files Affected
- `runtime/include/sp/runtime/Runtime.h`, `runtime/src/Runtime.cpp`
- `compiler/include/sp/codegen/VTableBuilder.h`, `compiler/src/codegen/VTableBuilder.cpp`
- `compiler/include/sp/codegen/ClassGenerator.h`, `compiler/src/codegen/ClassGenerator.cpp`
- `compiler/include/sp/ir/IRType.h`, `compiler/include/sp/ir/IRInstruction.h`, `compiler/include/sp/ir/IRModule.h`
- `compiler/src/ir/IR.cpp`, `compiler/src/ir/IRBuilder.cpp`, `compiler/src/ir/IRPrinter.cpp`
- `compiler/src/codegen/LLVMGenerator.cpp`
- `tests/codegen/VTableBuilderTest.cpp`, `tests/codegen/ClassGeneratorTest.cpp`, `tests/codegen/ExecutionTest.cpp`
- `examples/oop_phase5.sp`

### Testing Requirements
- Compile and test complex class hierarchies and interface dispatching.
- Verify memory allocation behavior.
- 108 automated tests in `sp_tests.exe` with 100% pass rate.

### Success Criteria
- Virtual method resolution works correctly. Memory for objects is allocated properly. Object instantiation, polymorphism, struct value types, and enum constants run cleanly.

---

## Phase 6: Generics & Collections (SP 0.6)
**Duration:** 4-6 weeks
**Goal:** Support generic programming and foundational standard collections.

### Tasks
- [x] 1. Implement generic type parameters for classes (e.g., `Box<T>`).
- [x] 2. Implement generic type parameters for functions.
- [x] 3. Add semantic checks for generic constraints.
- [x] 4. Implement a monomorphization strategy (or type erasure) for generics in code generation.
- [x] 5. Build and generate LLVM code for native `Array<T>`.
- [x] 6. Implement dynamic `List<T>`.
- [x] 7. Implement `Map<K,V>` (hash map) and `Set<T>`.
- [x] 8. Implement iterators and support for `for-each` loops over collections.
- [x] 9. Implement lambda functions and closures.
- [x] 10. Build higher-order function support (`map`, `filter`, `reduce`).

### Deliverables
- Working generics system with full monomorphization for generic classes and generic functions.
- High-performance native collection primitives (`Array<T>`, `List<T>`, `Map<K,V>`, `Set<T>`).
- Seamless `for-in` loop iteration lowering across arrays and collections.
- First-class lambda functions, closures, and higher-order functional patterns.

### Key Files Affected
- `runtime/include/sp/runtime/Runtime.h`, `runtime/src/Runtime.cpp`
- `compiler/include/sp/ast/*`, `compiler/src/parser/Parser.cpp`, `compiler/src/parser/ExpressionParser.cpp`
- `compiler/include/sp/sema/*`, `compiler/src/sema/Type.cpp`, `compiler/src/sema/TypeChecker.cpp`
- `compiler/include/sp/ir/*`, `compiler/src/ir/IR.cpp`, `compiler/src/ir/IRBuilder.cpp`
- `compiler/src/codegen/LLVMGenerator.cpp`, `compiler/src/codegen/VTableBuilder.cpp`
- `tests/codegen/ExecutionTest.cpp`, `tests/parser/ParserTest.cpp`, `tests/sema/TypeCheckerTest.cpp`

### Testing Requirements
- Comprehensive tests for generic type inference and monomorphization.
- Unit and execution tests for all collection data structures (`Array`, `List`, `Map`, `Set`), iterators, and lambdas.
- 124 automated tests in `sp_tests.exe` with 100% pass rate.

### Success Criteria
- Developers can write type-safe, generic data structures and algorithms without runtime type errors. All native collection and generic execution tests pass cleanly.

---

## Phase 7: Standard Library (SP 0.7)
**Duration:** 4-6 weeks
**Goal:** Develop the core standard library modules required for day-to-day programming.

### Tasks
- [x] 1. Implement `std.io` (println, print, readLine, stdin/stdout/stderr).
- [x] 2. Implement `std.string` utilities (length, upper, lower, contains, split, trim, replace, substring, interpolation).
- [x] 3. Implement `std.math` (abs, min, max, sqrt, pow, floor, ceil, round, constants, trig functions).
- [x] 4. Polish and integrate native implementations of `std.collections`.
- [x] 5. Implement `std.fs` for file system operations (readText, writeText, exists, delete, listDir, createDir).
- [x] 6. Implement `std.time` (now, sleep, format, parse, Duration).
- [x] 7. Implement `std.os` (env, args, platform, exit).
- [x] 8. Implement `std.process` (exec, spawn).
- [x] 9. Ensure the module system and import resolution work flawlessly across the stdlib.

### Deliverables
- A robust core standard library consisting of 8 complete modules (`std.io`, `std.string`, `std.math`, `std.collections`, `std.fs`, `std.time`, `std.os`, `std.process`).
- Working module and import system with automated distribution packaging (`stdlib/` and `share/sp/stdlib/`).
- End-to-end multi-module native execution and integration verification.

### Key Files Affected
- `stdlib/io/*.sp`, `stdlib/string/*.sp`, `stdlib/math/*.sp`, `stdlib/collections/*.sp`, `stdlib/fs/*.sp`, `stdlib/time/*.sp`, `stdlib/os/*.sp`, `stdlib/process/*.sp`
- `runtime/include/sp/runtime/Runtime.h`, `runtime/src/Runtime.cpp`
- `compiler/src/sema/Type.cpp`, `compiler/src/sema/SymbolTable.cpp`
- `compiler/src/ir/IR.cpp`, `compiler/src/ir/IRBuilder.cpp`, `compiler/src/parser/Parser.cpp`
- `tests/codegen/ExecutionTest.cpp`, `tests/lexer/GoldenLexerTest.cpp`, `tests/parser/GoldenParserTest.cpp`, `tests/sema/GoldenSemanticTest.cpp`

### Testing Requirements
- Unit tests and golden tests for every function in the standard library across lexer, parser, semantic analyzer, and native execution.
- Cross-platform behavior verification for `std.fs`, `std.os`, and `std.process`.
- 165 automated tests in `sp_tests.exe` with 100% pass rate.

### Success Criteria
- The standard library provides sufficient functionality to write a basic command-line application without external dependencies.

---

## Phase 8: Package Manager (SP 0.8)
**Duration:** 3-4 weeks
**Goal:** Create the SPM (SP Package Manager) for dependency management and project building.

### Tasks
- [x] 1. Implement `spm init` to scaffold new projects and generate `sp.toml`.
- [x] 2. Implement `spm add <package>` and `spm remove <package>`.
- [x] 3. Implement `spm update` and `spm install`.
- [x] 4. Implement dependency resolution algorithms.
- [x] 5. Generate and parse `sp.lock` lockfiles for deterministic builds.
- [x] 6. Implement `spm build` to compile a project alongside its dependencies.
- [x] 7. Implement `spm run` for seamless execution.
- [x] 8. Prepare architecture for `spm publish` and `spm search` interfacing with a package registry.

### Deliverables
- Fully functional `spm` CLI tool installed to system toolchain (`C:\SP-Language\bin\spm.exe`).
- Reliable SemVer dependency resolution, cycle detection, conflict handling, and lockfile generation (`sp.lock`).
- End-to-end native project scaffolding, compilation (`spm build`), execution (`spm run`), and cleaning (`spm clean`).

### Key Files Affected
- `tools/spm/main.cpp`
- `tools/spm/ProjectConfig.h`, `tools/spm/ProjectConfig.cpp`
- `tools/spm/SemVer.h`, `tools/spm/SemVer.cpp`
- `tools/spm/DependencyResolver.h`, `tools/spm/DependencyResolver.cpp`
- `tools/spm/Lockfile.h`, `tools/spm/Lockfile.cpp`
- `tools/spm/BuildOrchestrator.h`, `tools/spm/BuildOrchestrator.cpp`
- `tests/spm/ProjectConfigTest.cpp`, `tests/spm/DependencyResolverTest.cpp`, `tests/spm/LockfileTest.cpp`, `tests/spm/BuildOrchestratorTest.cpp`

### Testing Requirements
- Test dependency graph resolution (including cycle detection and version conflicts).
- Unit tests and integration tests covering TOML configs, SemVer constraints, lockfile roundtrips, and project build/run.
- 186 automated tests in `sp_tests.exe` with 100% pass rate.

### Success Criteria
- Users can create a project, add an external dependency, and build the project successfully using only `spm`. [100% VERIFIED]

---

## Phase 9: Developer Tools (SP 0.9)
**Duration:** 4-6 weeks
**Goal:** Build a modern developer experience with formatting, linting, IDE support, and debugging.

### Tasks
- [ ] 1. Implement `spc fmt` for configurable, standardized code formatting.
- [ ] 2. Implement `spc test` as a native test runner for SP test blocks.
- [ ] 3. Implement `spc doc` to generate HTML/Markdown documentation from `///` comments.
- [ ] 4. Build the SP Language Server (`spls`) implementing the LSP protocol:
  - Autocomplete, Go to definition, Hover documentation, Diagnostics, Code formatting, Rename symbol.
- [ ] 5. Build a VS Code Extension:
  - TextMate grammar for syntax highlighting.
  - Integration with `spls`.
  - Basic `sp.toml` project support.
- [ ] 6. Implement a basic Debug Adapter Protocol (DAP) debugger:
  - Breakpoints, stepping (over/into/out), variable inspection, call stack viewing.

### Deliverables
- Code formatter, test runner, and documentation generator.
- SP Language Server and official VS Code Extension.
- Basic interactive debugger.

### Key Files Affected
- `tools/fmt/*`, `tools/spls/*`, `editors/vscode/*`
- `tools/debugger/*`

### Testing Requirements
- Unit tests for formatter idempotency.
- LSP integration tests simulating IDE interactions.

### Success Criteria
- A developer can write, format, test, and debug SP code comfortably inside VS Code with full autocomplete and hover support.

---

## Phase 10: Production Release (SP 1.0)
**Duration:** 4-8 weeks
**Goal:** Deliver a stable, performant, and well-documented 1.0 release of the SP Programming Language.

### Tasks
- [ ] 1. Stabilize the language specification (no more breaking changes).
- [ ] 2. Optimize compiler performance and generated code quality.
- [ ] 3. Solidify cross-compilation support via `--target` flags.
- [ ] 4. Complete and deploy the official documentation website.
- [ ] 5. Write comprehensive getting-started guides, tutorials, and a language reference.
- [ ] 6. Conduct a security audit (package signing, lockfile integrity, checksums).
- [ ] 7. Perform and publish benchmarks against other languages (e.g., Fibonacci, sorting, JSON parsing).
- [ ] 8. Resolve outstanding bugs and handle edge cases gracefully.
- [ ] 9. Launch the official website (`splang.shantopaul.com`) and package registry (`registry.splang.shantopaul.com`).
- [ ] 10. Publish official GitHub releases containing pre-compiled binaries for Windows, Linux, and macOS.
- [ ] 11. Write the v1.0.0 announcement blog post.

### Deliverables
- SP 1.0.0 stable release binaries.
- Complete, polished toolchain and documentation.
- Live website and basic package registry.

### Key Files Affected
- Documentation repository.
- Release scripts and CI deployment workflows.

### Testing Requirements
- Massive real-world code compilation tests.
- Performance profiling and regression testing.

### Success Criteria
- The language is ready for public adoption. It compiles fast, runs fast, and has no known major bugs in the core features.

---

## Future Phases (Post 1.0)

- **Phase 11:** Async/Await & Concurrency (SP 1.1)
- **Phase 12:** Advanced Memory Management — Ownership/Borrowing (SP 1.2)
- **Phase 13:** Traits & Extension Methods (SP 1.3)
- **Phase 14:** Native Interop — C FFI, Java JNI, .NET Bridge (SP 1.4)
- **Phase 15:** WebAssembly Target (SP 1.5)
- **Phase 16:** Reflection & Annotations (SP 2.0)
- **Phase 17:** GUI Framework (SP 2.x)
- **Phase 18:** Mobile Targets — Android/iOS (SP 3.x)
