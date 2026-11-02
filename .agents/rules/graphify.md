---
trigger: always_on
description: Mandatory Graphify workflow for codebase comprehension and automated graph synchronization.
---

# Graphify Agent Operating Protocol

This codebase is indexed by **Graphify** (`graphify-out/`). All AI coding agents working in this workspace must adhere strictly to this two-phase protocol:

---

## 1. Before Starting Any Coding Task (Pre-Task)
1. **Always consult the Knowledge Graph first** before proposing architectures, writing new features, or modifying existing code.
2. **Query relationships**:
   - Use MCP tool `query_graph` or CLI `python -m graphify query "<question>"` to trace concepts.
   - Use `shortest_path` or CLI `python -m graphify path "<Source>" "<Target>"` to detect dependency paths and import chains.
   - Use `get_node` or CLI `python -m graphify explain "<symbol>"` to inspect incoming/outgoing edges.
3. **Broad architectural context**: Read `graphify-out/GRAPH_REPORT.md` to identify central "god nodes" and avoid accidental breaking changes to core shared modules.

---

## 2. When Completing Any Coding Task (Post-Task)
1. **Mandatory Automatic Graph Update**:
   Immediately after creating, modifying, or refactoring any code file, run the update command before declaring the task complete:
   ```bash
   python -m graphify update . --force
   ```
2. **Deterministic & Zero-Cost**:
   This runs local tree-sitter AST extraction. It incurs **zero API cost**, requires no external keys, and completes in a few seconds.
3. **Re-sync Visualization**:
   If significant structural changes were made (new files, major exports), ensure visualization is updated:
   ```bash
   python -m graphify export html
   ```

---

## 3. Git Hooks Active
Git hooks (`post-commit`, `post-merge`, `post-checkout`, `post-rewrite`) are installed in `.git/hooks/` to ensure graph consistency across git operations.
