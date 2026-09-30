# SP Programming Language - Project Rules

This document outlines the strict guidelines, coding standards, tools, and anti-patterns for the SP Programming Language compiler project. All contributors (including AI assistants) must adhere to these rules.

---

## 1. WHAT TO USE

### 1.1 Languages & Compilers
| Tool / Technology | Version | Purpose |
| :--- | :--- | :--- |
| **C++** | C++17 or C++20 | All compiler and runtime source code |
| **LLVM** | 17+ (C++ API) | Code generation, optimization, and target platforms |
| **CMake** | 3.20+ | Build system |
| **TOML** | - | Project configuration (`sp.toml`) |
| **Markdown** | - | All documentation |
| **TypeScript** | - | VS Code extension development |
| **JSON** | - | TextMate grammar for syntax highlighting |

### 1.2 Libraries — APPROVED
*   **LLVM**: Code generation, optimization, target platforms.
*   **GoogleTest** OR **Catch2**: C++ unit testing.
*   **nlohmann/json** OR **toml++**: TOML/JSON parsing for `sp.toml`.
*   **CLI11** OR **cxxopts**: Command-line argument parsing for `spc`/`spm`.
*   **fmt**: String formatting (if needed beyond C++20 `std::format`).
*   **spdlog**: Logging (optional).
*   **Standard C++ Library**: STL containers, algorithms, filesystem, `std::string_view`, `std::optional`, `std::variant`.

### 1.3 Coding Standards
*   **Memory Management**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`). **NO** raw `new`/`delete`. Use RAII for all resource management.
*   **Strings**: Use `std::string_view` for non-owning string references.
*   **Optionals & Variants**: Use `std::optional` for nullable returns. Use `std::variant` for tagged unions (e.g., AST node types).
*   **Types & Enums**: Prefer `auto` for type deduction where the type is obvious. Use `enum class` instead of plain `enum`.
*   **Constants**: Use `const` and `constexpr` wherever possible.
*   **Namespaces**: Group code logically: `sp::lexer`, `sp::parser`, `sp::ast`, `sp::semantic`, `sp::types`, `sp::ir`, `sp::codegen`, `sp::diagnostics`.
*   **Header Guards**: Use `#pragma once`.
*   **Formatting**:
    *   Indentation: 4 spaces (no tabs).
    *   Max line length: 120 characters.
*   **Naming Conventions**:
    *   **Files**: `PascalCase.h` / `PascalCase.cpp` (e.g., `Lexer.h`, `Lexer.cpp`).
    *   **Classes**: `PascalCase` (e.g., `class TypeChecker`).
    *   **Methods/Functions**: `camelCase` (e.g., `void resolveNames()`).
    *   **Variables**: `camelCase` (e.g., `int tokenCount`).
    *   **Constants**: `UPPER_SNAKE_CASE` (e.g., `constexpr int MAX_TOKEN_LENGTH = 256;`).

### 1.4 Error Handling Strategy
*   **Rust-Style Reporting**: Provide source location, caret pointers, and helpful suggestions.
*   **Error Codes**: Use the `E1000`-`E1009+` range.
*   **No Aborts**: **NEVER** use `exit()` or `abort()` in library code. Propagate errors to the caller.
*   **Recoverable Errors**: Use `std::expected` (C++23) or a custom `Result<T, Error>` type.
*   **Exceptions**: Use ONLY for truly exceptional/unrecoverable situations.
*   **Diagnostic Details**: All compiler errors must include:
    *   Error code
    *   File name, line number, column number
    *   Caret (`^`) pointing to the error location
    *   Expected vs. found types (if applicable)
    *   Help suggestion
*   **Severity Levels**: `Error`, `Warning`, `Info`, `Hint`.

### 1.5 Testing Rules
*   **Coverage Requirement**: Every compiler phase MUST have unit tests. Minimum **80%** code coverage target.
*   **Golden Tests**: Use golden tests mapping input `.sp` files to expected output (tokens, AST, errors).
*   **Structure**: The test directory must mirror the source directory structure.
*   **Integration Tests**: Compile example `.sp` programs end-to-end.
*   **CI**: Continuous Integration must pass ALL tests before a branch can be merged.
*   **Naming**: `TEST(TestSuiteName, TestCaseName)` (e.g., `TEST(LexerTest, TokenizesIntegerLiteral)`).

### 1.6 Git Rules
*   **Branch Naming**: `feature/<name>`, `bugfix/<name>`, `refactor/<name>`.
*   **Commit Messages**: Use Conventional Commits (`feat:`, `fix:`, `refactor:`, `test:`, `docs:`, `chore:`).
*   **Pull Requests**: PRs require at least 1 review.
*   **Main Branch**: The `main` branch must always compile and pass all tests.
*   **Releases**: Tag releases using semantic versioning (e.g., `v0.1.0`, `v0.2.0`).

### 1.7 Documentation Rules
*   **Doc Comments**: Every public class/function MUST have a doc comment using `///`.
*   **Module READMEs**: Every module must have a `README.md` explaining its purpose.
*   **Architecture**: Document major architecture decisions in ADR (Architecture Decision Record) format.
*   **Changelog**: Maintain a `CHANGELOG.md`.

---

## 2. WHAT TO AVOID

### 2.1 Libraries — BANNED
*   **Boost**: Too heavy, pulls in excessive dependencies.
*   **Qt**: Not needed for a CLI compiler.
*   **Any GUI Framework**: This is strictly a CLI tool.
*   **Antlr / Bison / Flex**: We use a hand-written parser for superior error message quality.
*   **Any JVM-based tools**: Must be native.
*   **Any Python-based build tools**: Use CMake exclusively.

### 2.2 Anti-Patterns — FORBIDDEN
*   **Raw Memory Management**: No raw `new` / `delete`.
*   **Namespace Pollution**: `using namespace std;` in headers is strictly forbidden.
*   **Global Mutable State**: Use dependency injection instead.
*   **C-style Casts**: Forbidden. Use `static_cast`, `dynamic_cast`, or `reinterpret_cast`.
*   **Macros for Constants**: Use `constexpr`.
*   **`goto` statements**: Never.
*   **Circular Dependencies**: Keep module boundaries clean.
*   **God Classes**: Avoid classes that do too many things; follow SRP.
*   **Magic Numbers**: Define named constants.
*   **Silencing Errors**: Do not suppress warnings/errors—fix them.
*   **Compiler Extensions**: Avoid compiler-specific extensions unless guarded behind `#ifdef`.
*   **Hard-coded Paths**: Do not hard-code file paths.
*   **Mixed Indentation**: Do not mix tabs and spaces.

---

## 3. Boundaries for AI Assistance

AI assistants working on this project MUST adhere strictly to the following boundaries:

*   **Conventions**: AI MUST follow the established naming conventions.
*   **Dependencies**: AI MUST NOT introduce new dependencies without explicit approval.
*   **Architecture**: AI MUST NOT modify the compiler pipeline order.
*   **Testing**: AI MUST NOT skip writing tests for new code. AI SHOULD generate golden test cases for any new parser/lexer feature.
*   **Documentation**: AI MUST preserve all existing comments and documentation.
*   **Namespaces**: AI MUST use the established namespace hierarchy.
*   **Modern C++**: AI MUST NOT use deprecated C++ features.
*   **Error Handling**: AI MUST follow the error handling strategy (no `exit`/`abort` in library code).
*   **LLVM**: AI MUST NOT change the LLVM version or API surface without discussion.
*   **State Management**: AI MUST check `Memory.md` before starting any new task, and MUST update `Memory.md` after completing any task.
*   **Design**: AI SHOULD prefer composition over inheritance in compiler internals. AI SHOULD keep functions under 50 lines where possible.
*   **Scope**: AI MUST NOT attempt to implement all 135 spec sections at once — follow `Phases.md`.
