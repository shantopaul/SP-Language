# Contributing to the SP Programming Language

Thank you for your interest in contributing to the **SP Programming Language**! SP is an ambitious open-source general-purpose programming language designed to provide familiar syntax, modern type safety, and native performance through LLVM.

---

## 📌 Development Philosophy & Guidelines

Before contributing, please review our core architectural documents:
- [docs/architecture/ARCHITECTURE.md](docs/architecture/ARCHITECTURE.md) — System architecture, compiler pipeline, and internal design.
- [docs/planning/RULES.md](docs/planning/RULES.md) — Coding conventions, forbidden anti-patterns, and library guidelines.
- [docs/planning/PHASES.md](docs/planning/PHASES.md) — Current development roadmap and phase tasks.
- [docs/design/DESIGN.md](docs/design/DESIGN.md) — CLI output design, theme colors, and error diagnostics styling.

---

## 🛠️ Setting Up the Development Environment

### Prerequisites
- **C++ Compiler:** C++17 or C++20 compliant compiler (Clang 15+, GCC 11+, or MSVC 2019+)
- **CMake:** Version 3.20 or newer
- **Build Tool:** Ninja (recommended) or Make / Visual Studio Build Tools
- **LLVM:** LLVM 17+ development packages (headers and libraries)
- **Git:** Version control

### Building from Source

```bash
# 1. Clone the repository
git clone https://github.com/shantopaul/SP-Language.git
cd SP-Language

# 2. Configure build with CMake
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 3. Compile the project
cmake --build build

# 4. Run the test suite
ctest --test-dir build --output-on-failure
```

---

## 🧪 Testing Rules

- Every new component (Lexer, Parser, AST, Semantic Analyzer, Codegen) **must** be accompanied by unit tests.
- We use **GoogleTest** for C++ unit tests and golden tests.
- Golden tests compare `.sp` input source code directly with expected tokens, ASTs, or compiler diagnostic outputs.
- All existing and new tests must pass before submitting a Pull Request.

---

## 📝 Commit Convention

We adhere to the [Conventional Commits](https://www.conventionalcommits.org/) specification:

- `feat:` Introduces a new feature or language capability (e.g. `feat(lexer): add support for binary integer literals`)
- `fix:` Fixes a bug or compiler crash (e.g. `fix(parser): handle trailing comma in function argument list`)
- `refactor:` Code refactoring without changing observable behavior
- `test:` Adds or updates test cases
- `docs:` Documentation improvements
- `perf:` Performance improvements
- `chore:` Build scripts, dependency updates, and maintenance

---

## 🔀 Pull Request Process

1. Fork the repository and create your branch from `main`:
   ```bash
   git checkout -b feature/your-feature-name
   ```
2. Write clean, readable modern C++ code following [Rules.md](docs/planning/RULES.md).
3. Ensure no memory leaks and verify tests pass.
4. Open a Pull Request against `main` with a clear description of the problem solved.
5. Link any related issues in the PR description.
