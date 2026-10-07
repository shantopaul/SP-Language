# SP Language — Developer Tooling Roadmap

**Document ID:** SP-TOOL-001  
**Target:** `tools/` Directory & Developer Toolchain  

---

## 1. Toolchain Overview & Target Standards

The SP toolchain aims to provide the cohesion and speed of `cargo` and `go` tooling:

| Tool | CLI Command | Target Standard Comparable To |
|---|---|---|
| **Compiler** | `spc build`, `spc run` | `cargo build`, `go build` |
| **Package Manager** | `spm add`, `spm install` | `cargo add`, `npm install` |
| **Language Server** | `spls` (JSON-RPC) | `rust-analyzer`, `gopls` |
| **Debugger** | `spd` (DAP) | `lldb-dap`, `delve` |
| **Formatter** | `spc fmt` | `rustfmt`, `gofmt` |
| **Test Runner** | `spc test` | `cargo test`, `go test` |
| **Doc Generator** | `spc doc` | `rustdoc`, `godoc` |
| **Benchmarker** | `spc bench` (SP 1.4) | `cargo bench`, `go test -bench` |

---

## 2. Key Tooling Milestones

### 2.1 Language Server (`spls`) Upgrades
- **Incremental Ast Caching:** Parse and type-check only modified AST nodes on keystroke events.
- **Find References & Rename Refactoring:** Full workspace symbol index across all dependencies in `sp.lock`.
- **Inlay Type Hints:** Display inferred variable types (`let x /*: int */ = 42`) in VS Code.

### 2.2 Package Manager (`spm`) Upgrades
- **Offline Caching:** Store downloaded dependencies in `~/.sp/cache/packages/` so builds work without internet connection.
- **Dependency Tree Visualization:** `spm tree` displaying transitive dependency graph.
- **Workspace Support:** Support multi-project monorepos with shared dependencies.

### 2.3 Native DAP Debugger (`spd`) Integration
- Direct expression evaluation in VS Code debug console.
- Variable inspection for complex structs, classes, lists, and maps.
