# SP Language — Full Codebase Analysis, Goal Definition & Future Roadmap Planning

## Role

Act as a **Principal Programming Language Engineer, Compiler Engineer, Runtime Engineer, Systems Architect, and Open-Source Language Project Maintainer**.

You are working on **SP Language**, an independent programming language project.

Your job is **NOT** to immediately decide what SP should become.

Your job is to:

1. Fully understand the existing SP Language codebase.
2. Understand the current compiler architecture and implementation.
3. Understand the current language specification and semantics.
4. Identify what is actually implemented versus documented/planned/claimed.
5. Identify architectural strengths, weaknesses, technical debt, missing foundations, and inconsistencies.
6. Research and compare relevant approaches used by languages such as C, C++, Rust, Go, Java, C#, Swift, Kotlin, Zig, and other relevant languages where useful.
7. Develop multiple possible future directions for SP.
8. Evaluate the trade-offs of each direction.
9. Recommend the strongest options with evidence.
10. Produce a detailed implementation roadmap.
11. Save the final planning documents inside the project's `/Docs` directory.

**Do not blindly follow the existing SP documentation or marketing claims. Verify everything against the actual source code.**

---

# 1. Most Important Rule

## Analyze first. Decide later.

Do NOT begin by assuming that SP should become:

- another Rust
- another C++
- another Go
- another Java
- another C#
- another scripting language
- another general-purpose language
- another systems language

Instead, determine what SP can realistically become based on:

- current architecture
- existing implementation
- developer experience
- compiler design
- runtime design
- performance model
- memory model
- type system
- standard library
- package ecosystem
- tooling
- current code quality
- maintainability
- implementation complexity
- realistic development capacity

The final direction must emerge from the analysis.

---

# 2. First Task — Full Codebase Discovery

Before making any recommendation, inspect the complete repository.

Do not only inspect:

- README
- website
- documentation
- examples
- marketing pages

You must inspect the actual source code.

Create a mental model of the complete repository.

Analyze:

```text
/
├── compiler
├── lexer
├── parser
├── AST
├── semantic analysis
├── type system
├── code generation
├── LLVM integration
├── runtime
├── standard library
├── package manager
├── registry
├── formatter
├── test runner
├── documentation generator
├── LSP
├── debugger
├── CLI
├── build system
├── examples
├── tests
├── benchmarks
├── scripts
├── CI/CD
├── documentation
└── other supporting infrastructure
```

Do not assume these directories exist.

First discover the real repository structure.

---

# 3. Determine the Actual Technology Stack

Identify exactly what technologies are being used.

For example:

- Implementation language(s)
- Compiler implementation language
- LLVM version
- LLVM APIs
- Parser technology
- Lexer implementation
- Parser generator, if any
- AST representation
- IR representation
- Build system
- Dependency manager
- Runtime implementation
- Standard library implementation
- C/C++ dependencies
- Third-party libraries
- Serialization formats
- Testing framework
- Benchmark framework
- LSP implementation
- Debugger implementation
- Package manager implementation
- Registry implementation
- CI/CD
- Supported operating systems
- Supported CPU architectures

For each technology, determine:

```text
Technology
Why it exists
Where it is used
How deeply it is integrated
Whether it is replaceable
Whether it creates technical debt
Whether it should remain
```

---

# 4. Compiler Architecture Analysis

Analyze the compiler from source code.

Document the actual pipeline.

For example:

```text
Source
  ↓
Lexer
  ↓
Tokens
  ↓
Parser
  ↓
AST
  ↓
Semantic Analysis
  ↓
Type Checking
  ↓
HIR
  ↓
MIR
  ↓
IR
  ↓
LLVM IR
  ↓
Optimization
  ↓
Machine Code
```

Do not assume this pipeline exists.

Determine the actual pipeline.

For every stage document:

- implementation location
- major files
- major classes/functions/modules
- inputs
- outputs
- invariants
- error handling
- limitations
- technical debt
- future scalability

---

# 5. Lexer Analysis

Analyze:

- token architecture
- keywords
- identifiers
- literals
- strings
- escape sequences
- comments
- operators
- punctuation
- numeric literals
- Unicode handling
- source locations
- error reporting
- lexer performance

Check for:

- duplicated logic
- hardcoded behavior
- fragile parsing
- missing diagnostics
- unnecessary allocations
- poor extensibility

---

# 6. Parser Analysis

Analyze:

- grammar
- recursive descent / Pratt parser / parser generator / other approach
- precedence
- associativity
- ambiguity
- error recovery
- syntax extensibility
- source location tracking

Determine whether the parser architecture can realistically support future language features.

---

# 7. AST Analysis

Document:

- AST node types
- ownership model
- source spans
- annotations
- metadata
- mutability
- memory management
- traversal mechanisms

Evaluate whether the AST is:

- extensible
- maintainable
- efficient
- suitable for tooling
- suitable for compiler diagnostics

---

# 8. Semantic Analysis

Analyze:

- name resolution
- scopes
- symbol tables
- imports
- modules
- visibility
- type checking
- overload resolution
- generic checking
- interface checking
- inheritance
- pattern matching
- type inference
- nullability
- constant evaluation
- compile-time evaluation if present

Identify missing semantic foundations.

---

# 9. Type System Analysis

This is one of the highest-priority areas.

Fully document the type system.

Determine whether SP currently supports:

- primitive types
- structs
- classes
- interfaces
- enums
- generics
- type inference
- nullable types
- option types
- result/error types
- function types
- tuples
- arrays
- slices
- references
- pointers
- ownership
- borrowing
- lifetimes
- traits/typeclasses
- type aliases
- associated types
- variance
- dynamic dispatch
- static dispatch

For every feature distinguish:

```text
Fully implemented
Partially implemented
Prototype
Compiler-only
Runtime-dependent
Documented but not implemented
```

---

# 10. Memory Model — Critical Investigation

Do NOT assume SP's memory model.

Determine exactly how memory works.

Answer:

- Who owns an object?
- Who destroys an object?
- Is there garbage collection?
- Is there reference counting?
- Is memory manually managed?
- Is there ownership?
- Is there borrowing?
- Are references safe?
- Can dangling references occur?
- Can use-after-free occur?
- Can double-free occur?
- How are arrays represented?
- How are strings represented?
- How are objects represented?
- How are closures represented?
- How are temporary values handled?
- How are exceptions/errors represented?
- What happens at function return?
- What happens during panic/error?
- How does FFI affect memory safety?

If the current model is incomplete, clearly identify the problem.

Then propose multiple possible models.

For example:

### Option A
Manual memory management

### Option B
Reference counting

### Option C
Tracing garbage collection

### Option D
Ownership + borrowing

### Option E
Hybrid model

Do NOT automatically choose one.

Compare:

- safety
- performance
- complexity
- developer experience
- compiler complexity
- runtime complexity
- interoperability
- learning curve
- implementation cost

---

# 11. Concurrency Model

Determine the current state.

Check whether SP has:

- threads
- async
- await
- futures
- tasks
- channels
- actors
- mutexes
- atomics
- thread pools
- structured concurrency

If not implemented, investigate which model best fits SP.

Compare approaches from:

- Rust
- Go
- Java
- C#
- Swift
- Kotlin
- Erlang/Elixir

Do not simply copy another language.

---

# 12. Error Handling

Analyze the current model.

Determine whether SP uses:

- exceptions
- Result
- Option
- panic
- error values
- checked exceptions
- unchecked exceptions
- error propagation operators

Evaluate what should become SP's long-term error model.

Provide multiple alternatives and trade-offs.

---

# 13. Runtime Analysis

Determine whether SP has:

- runtime
- VM
- garbage collector
- scheduler
- allocator
- threading runtime
- async runtime
- reflection
- dynamic dispatch runtime
- exception runtime
- standard runtime ABI

If SP is native/LLVM based, determine how much runtime support is still required.

Clearly distinguish:

```text
Compiler
Runtime
Standard Library
Operating System
```

---

# 14. LLVM Architecture

Analyze exactly how LLVM is integrated.

Determine:

- LLVM version
- LLVM APIs used
- IR generation strategy
- optimization pipeline
- target configuration
- debug information
- object generation
- linking
- cross compilation
- static linking
- dynamic linking
- optimization levels
- LTO
- PGO
- SIMD support
- target-specific optimizations

Determine whether the current LLVM architecture can scale to SP 2.0+.

---

# 15. ABI & FFI

Analyze whether SP currently has a stable ABI.

Investigate:

- C ABI
- function calling conventions
- structs
- strings
- arrays
- pointers
- callbacks
- ownership across FFI
- error handling across FFI
- dynamic libraries
- static libraries

This is extremely important for a new language.

Determine whether C interoperability should become a major SP feature.

---

# 16. Standard Library Analysis

Analyze every existing standard library module.

For each module document:

```text
Module
Purpose
Implementation
API quality
Missing functionality
Performance
Portability
Safety
Future importance
```

Then compare SP's stdlib with:

- Rust std
- Go stdlib
- Java standard library
- C#/.NET
- C++ STL

Identify the minimum standard library required for a production-ready SP ecosystem.

Consider:

```text
IO
Filesystem
Process
OS
Networking
HTTP
TLS
JSON
Serialization
Time
Concurrency
Async
Collections
Crypto
Logging
Environment
CLI
Database interfaces
Compression
```

But do not automatically add all of them.

Prioritize.

---

# 17. Package Manager & Registry

Analyze the actual package manager implementation.

Check:

- dependency resolution
- semantic versioning
- lockfile
- checksums
- registry
- package publishing
- package metadata
- dependency graph
- caching
- offline mode
- reproducible builds
- security
- package signing
- malicious package protection
- namespaces
- package ownership

Compare:

- Cargo
- npm
- Go modules
- NuGet
- Maven
- Swift Package Manager

Identify the strongest model for SP.

---

# 18. Build System

Analyze:

- project configuration
- build graph
- incremental compilation
- caching
- parallel builds
- reproducibility
- cross compilation
- release builds
- debug builds
- dependency builds

Determine whether SP needs a more advanced build system.

---

# 19. Tooling Ecosystem

Analyze:

### Formatter

- `spfmt`

### Test runner

- test infrastructure

### Documentation

- docs generator

### LSP

- `spls`

### Debugger

- `spd`

### CLI

- compiler/package manager commands

Determine how these components communicate.

Evaluate whether SP can provide an experience similar to:

```text
cargo
rustfmt
rust-analyzer
clippy
rustdoc
```

or:

```text
go
gofmt
gopls
go test
go doc
```

---

# 20. Developer Experience

Test the entire developer workflow.

Evaluate:

```text
Install SP
 ↓
Create project
 ↓
Write code
 ↓
Build
 ↓
Run
 ↓
Test
 ↓
Format
 ↓
Debug
 ↓
Add dependency
 ↓
Publish package
 ↓
Generate docs
 ↓
Deploy
```

Identify every point of friction.

The goal is not only a technically impressive compiler.

The goal is a language developers actually enjoy using.

---

# 21. Performance Analysis

Do not rely only on existing benchmarks.

Analyze:

- compiler performance
- generated code quality
- binary size
- startup time
- runtime overhead
- memory usage
- allocation behavior
- optimization quality
- compile times
- incremental compilation
- debug build performance
- release build performance

Design a better benchmark suite.

Include realistic workloads:

```text
Algorithms
Memory
Strings
Collections
JSON
HTTP
Concurrency
Database
File I/O
Startup
Binary size
```

Compare against appropriate languages:

```text
C
C++
Rust
Go
Java
C#
```

Only make claims that the data supports.

---

# 22. Security Analysis

Analyze:

- package security
- supply chain
- unsafe features
- FFI safety
- memory safety
- dependency integrity
- lockfile security
- compiler security
- malicious code
- reproducible builds
- binary signing
- sandboxing possibilities

Determine what "safe" means in SP.

Do not use vague marketing terms such as "safe" without technical definition.

---

# 23. Documentation vs Reality Audit

This is mandatory.

Create a matrix:

| Feature | Documentation | Implementation | Tests | Status |
|---|---|---|---|---|
| Feature A | Yes | Yes | Yes | Stable |
| Feature B | Yes | Partial | Partial | Experimental |
| Feature C | Yes | No | No | Planned |

Find all mismatches.

The codebase is the source of truth.

---

# 24. Test Coverage Analysis

Analyze:

- unit tests
- compiler tests
- parser tests
- semantic tests
- code generation tests
- runtime tests
- integration tests
- end-to-end tests
- snapshot tests
- error diagnostic tests
- platform tests
- regression tests

Identify critical untested compiler behavior.

Recommend a testing architecture.

---

# 25. Technical Debt Analysis

Create a technical debt inventory.

Categorize:

### Critical

Could prevent SP from scaling.

### High

Should be fixed before major new features.

### Medium

Should be improved during normal development.

### Low

Nice-to-have cleanup.

For every debt item:

```text
Problem
Why it matters
Affected components
Risk
Recommended solution
Estimated complexity
Dependencies
```

---

# 26. Architecture Quality Assessment

Evaluate:

### Compiler

- modularity
- extensibility
- correctness
- performance
- maintainability

### Runtime

- safety
- complexity
- portability

### Standard library

- API consistency
- quality
- portability

### Toolchain

- integration
- developer experience

### Ecosystem

- package manager
- registry
- documentation
- IDE

Do not give arbitrary scores without explaining the evidence.

---

# 27. Future Direction — Generate Multiple Options

After the complete analysis, propose at least **3 possible strategic directions** for SP.

For example, only if supported by the analysis:

### Direction A — Systems Language

Focus:

```text
Memory safety
Performance
FFI
OS
Embedded
Infrastructure
```

### Direction B — Native Application Language

Focus:

```text
Backend
CLI
Cloud
Database
Networking
Developer productivity
```

### Direction C — General Purpose Language

Focus:

```text
Systems
Backend
Desktop
Cloud
Applications
```

You may propose completely different directions if the codebase suggests them.

---

# 28. Compare the Directions

Create a decision matrix.

Evaluate:

| Criteria | Direction A | Direction B | Direction C |
|---|---:|---:|---:|
| Technical feasibility | | | |
| Current codebase fit | | | |
| Development cost | | | |
| Differentiation | | | |
| Developer experience | | | |
| Ecosystem potential | | | |
| Performance potential | | | |
| Safety | | | |
| Community potential | | | |
| Long-term sustainability | | | |

Use qualitative reasoning, not fake precision.

---

# 29. Recommendation

After presenting alternatives, provide:

## Recommended Direction

Explain:

1. Why this fits the existing codebase.
2. What SP can realistically achieve.
3. What SP should NOT attempt.
4. Which users it should target.
5. Which languages it will compete with.
6. What makes SP different.
7. What technical foundations must be completed first.

But clearly mark this as:

> **Recommendation — Final decision requires Shanto + Claude review.**

Do not treat your recommendation as an irreversible decision.

---

# 30. Define SP's Future Identity

Propose:

### One-sentence definition

```text
SP is ...
```

### Target developers

Who should use SP?

### Primary use cases

What should SP be excellent at?

### Secondary use cases

What should SP support but not optimize for?

### Non-goals

What should SP explicitly NOT attempt?

### Core principles

For example:

```text
Safety
Performance
Simplicity
Predictability
Developer Experience
Portability
Interoperability
```

Only recommend principles supported by the analysis.

---

# 31. What SP Should NOT Become

This section is extremely important.

Identify areas that would unnecessarily increase complexity.

Potential examples:

```text
Do not add features just because Rust has them.
Do not add features just because C++ has them.
Do not add syntax without a practical use case.
Do not create a custom VM unless there is a strong reason.
Do not build a custom linker unnecessarily.
Do not duplicate LLVM functionality.
Do not create a huge standard library prematurely.
Do not promise enterprise readiness without testing.
Do not claim performance superiority without reproducible benchmarks.
Do not break language semantics repeatedly.
Do not optimize for every possible use case.
```

But these are examples only.

Derive the actual non-goals from the codebase.

---

# 32. Define the Language Core

Recommend what should become the stable core of SP.

Separate:

### Stable

Features that should rarely change.

### Experimental

Features that may change.

### Future

Features not yet implemented.

### Rejected

Features that should probably not be implemented.

---

# 33. SP 1.x Roadmap

Create a realistic roadmap after v1.0.

For example:

```text
SP 1.1
Language foundations

SP 1.2
Memory + FFI

SP 1.3
Concurrency

SP 1.4
Networking

SP 1.5
Database + backend

SP 1.6
Tooling + ecosystem

SP 1.7
Cross-platform + performance

SP 2.0
Stable language/platform
```

Do NOT use these versions blindly.

Generate the roadmap based on actual dependency order.

---

# 34. Dependency Graph

Create a technical dependency graph.

Example:

```text
Memory Model
     ↓
Ownership / References
     ↓
Type System
     ↓
Compiler IR
     ↓
Runtime / Codegen
     ↓
FFI
     ↓
Concurrency
     ↓
Async
     ↓
Networking
     ↓
Backend ecosystem
```

Identify which features must happen before others.

---

# 35. Implementation Plan

For every major feature define:

```text
Feature
Goal
Why
Current state
Affected components
Files/modules likely affected
Compiler changes
Runtime changes
Stdlib changes
Tooling changes
Tests required
Documentation required
Breaking changes
Migration strategy
Dependencies
Complexity
Risk
```

Do not invent exact file names if you have not inspected the repository.

Use actual file paths whenever possible.

---

# 36. Create a Master Implementation Checklist

Use checkboxes.

Example:

```markdown
## Compiler

- [ ] Memory model finalized
- [ ] Type system finalized
- [ ] Error model finalized
- [ ] IR architecture finalized
- [ ] FFI finalized
- [ ] Concurrency semantics finalized

## Runtime

- [ ] Allocator
- [ ] Runtime safety
- [ ] Threading
- [ ] Async runtime

## Standard Library

- [ ] HTTP
- [ ] JSON
- [ ] Networking
- [ ] TLS
- [ ] Database
- [ ] Logging

## Tooling

- [ ] LSP
- [ ] Debugger
- [ ] Formatter
- [ ] Test runner
- [ ] Documentation

## Ecosystem

- [ ] Registry
- [ ] Package security
- [ ] Cross compilation
- [ ] CI/CD
```

But generate the real checklist from the actual repository.

---

# 37. Documentation Files to Create

After the analysis, create/update planning documents inside:

```text
/Docs
```

Prefer this structure:

```text
/Docs
│
├── SP_LANGUAGE_ANALYSIS.md
│
├── SP_LANGUAGE_GOALS.md
│
├── SP_LANGUAGE_VISION.md
│
├── SP_LANGUAGE_PRINCIPLES.md
│
├── SP_LANGUAGE_NON_GOALS.md
│
├── SP_ARCHITECTURE_ROADMAP.md
│
├── SP_COMPILER_ROADMAP.md
│
├── SP_RUNTIME_ROADMAP.md
│
├── SP_STDLIB_ROADMAP.md
│
├── SP_TOOLING_ROADMAP.md
│
├── SP_ECOSYSTEM_ROADMAP.md
│
├── SP_FFI_DESIGN.md
│
├── SP_MEMORY_MODEL_OPTIONS.md
│
├── SP_CONCURRENCY_OPTIONS.md
│
├── SP_ERROR_HANDLING_OPTIONS.md
│
├── SP_PACKAGE_ECOSYSTEM.md
│
└── SP_IMPLEMENTATION_CHECKLIST.md
```

Do not create unnecessary documents if some topics are not relevant.

---

# 38. Most Important Planning Document

Create:

```text
/Docs/SP_LANGUAGE_GOALS.md
```

It should answer:

```text
What is SP?

Who is SP for?

What problems does SP solve?

What should SP be exceptionally good at?

What should SP never attempt?

What differentiates SP?

What is SP's long-term technical direction?

What is the 1.x roadmap?

What is the 2.0 vision?

What are the major technical risks?
```

---

# 39. Another Critical Document

Create:

```text
/Docs/SP_LANGUAGE_VISION.md
```

Include:

- Long-term vision
- Target users
- Core use cases
- Technical philosophy
- Performance philosophy
- Safety philosophy
- Developer experience philosophy
- Ecosystem philosophy
- Compatibility philosophy
- Platform strategy

---

# 40. Decision Log

Create:

```text
/Docs/DECISIONS/
```

For important future decisions use ADR-style documents:

```text
ADR-001-memory-model.md
ADR-002-error-handling.md
ADR-003-concurrency.md
ADR-004-ffi.md
ADR-005-package-system.md
```

Each should contain:

```text
Context
Options
Trade-offs
Decision
Consequences
Status
```

Do not create ADRs for trivial implementation details.

---

# 41. Do NOT Modify Code Yet

During this phase:

**DO NOT implement major changes.**

Do not:

- rewrite the compiler
- change the language syntax
- change the memory model
- replace LLVM
- redesign the package manager
- delete existing features
- introduce breaking changes

First produce the analysis and plan.

Implementation should begin only after the plan is reviewed.

---

# 42. Evidence-Based Analysis Rule

Every major recommendation must identify its evidence.

Use:

```text
Evidence:
- Current code
- Existing tests
- Existing documentation
- Benchmark results
- Compiler architecture
- Comparable language design
- Developer experience
```

Avoid statements such as:

> "This is better."

Instead write:

> "This approach is preferable for SP because..."

and explain why.

---

# 43. Avoid Feature Checklist Thinking

Do NOT create a roadmap like:

```text
Rust feature X
Rust feature Y
Go feature Z
C# feature A
```

Instead think in terms of:

```text
Problem
 ↓
Design principle
 ↓
Language feature
 ↓
Compiler implementation
 ↓
Runtime
 ↓
Stdlib
 ↓
Tooling
 ↓
Real-world use case
```

---

# 44. Real-World Validation

Propose at least 10 real applications that should be built with SP after the roadmap.

For example:

```text
CLI tool
HTTP server
REST API
WebSocket server
Database service
Reverse proxy
File server
Background worker
Cloud service
System utility
```

Again, choose examples based on the final recommended direction.

For each application explain which SP features it validates.

---

# 45. "Dogfood SP"

One of the long-term goals should be to use SP itself to build meaningful software.

Determine what parts of SP's own ecosystem can eventually be rewritten in SP:

```text
CLI tools
stdlib utilities
package tools
documentation tools
servers
developer tools
```

But do not force self-hosting prematurely.

---

# 46. Open Source Strategy

Analyze:

- repository structure
- contribution workflow
- issue templates
- roadmap visibility
- governance
- license
- code of conduct
- contributor guide
- release process
- semantic versioning
- security policy

Recommend what should be improved to make SP attractive to external contributors.

---

# 47. Release Strategy

Design a release model.

For example:

```text
0.x
Experimental

1.x
Stable language core

2.x
Major evolution
```

Determine whether SP should use:

- semantic versioning
- language edition model
- feature flags
- experimental features
- deprecation cycles
- compatibility guarantees

---

# 48. Final Output Format

At the end of the analysis, produce this summary:

# SP Language — Strategic Assessment

## 1. Current State

## 2. What SP Already Does Well

## 3. Major Technical Weaknesses

## 4. Critical Missing Foundations

## 5. Current Architecture

## 6. Possible Future Directions

## 7. Recommended Direction

## 8. Why This Direction

## 9. What SP Should NOT Become

## 10. Language Core

## 11. Compiler Roadmap

## 12. Runtime Roadmap

## 13. Standard Library Roadmap

## 14. Tooling Roadmap

## 15. Package Ecosystem Roadmap

## 16. FFI Roadmap

## 17. Concurrency Roadmap

## 18. Performance Roadmap

## 19. Security Roadmap

## 20. Developer Experience Roadmap

## 21. SP 1.x Roadmap

## 22. SP 2.0 Vision

## 23. Implementation Dependency Graph

## 24. Technical Debt

## 25. Risk Register

## 26. Decision Points for Shanto

## 27. Final Implementation Checklist

---

# 49. Decision Points for Me

At the end, create a section:

# Decisions Required From Shanto

List only decisions that require human/project-owner judgment.

For each:

```text
Decision
Option A
Option B
Option C
Trade-offs
Your recommendation
Why
```

Examples:

```text
Memory model
Error model
Concurrency model
FFI strategy
Runtime strategy
Target platforms
Primary users
Primary use cases
Compatibility policy
Language stability policy
Package ecosystem strategy
```

I will make the final decisions together with you.

---

# 50. Final Rule

The goal of this task is NOT to make SP look impressive on paper.

The goal is to determine:

> **What can SP realistically become, what should it become, and what engineering work is required to get there?**

Prioritize:

```text
Correctness
Safety
Simplicity
Performance
Maintainability
Developer Experience
Interoperability
Ecosystem
Long-term sustainability
```

over:

```text
Number of features
Marketing claims
Benchmark cherry-picking
Syntax complexity
Feature count
Similarity to popular languages
```

The final roadmap must be:

- technically realistic
- dependency-aware
- implementation-oriented
- evidence-based
- maintainable
- incremental
- measurable

Do not make irreversible strategic decisions without presenting alternatives first.

**Analyze → Compare → Recommend → Ask for Decisions → Plan → Implement.**