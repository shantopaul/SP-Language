# Agent Instructions & Guidelines

## Graphify Architecture & Knowledge Graph Protocol

This project uses **Graphify** (`graphify-out/`) for deterministic AST-based code relationship tracking.

### 1. Before Starting Coding (Pre-Task)
- **Always read the Knowledge Graph**:
  - Consult `graphify-out/GRAPH_REPORT.md` or use graph query tools before modifying code or designing architecture.
  - Check dependencies, callers, and callees using `python -m graphify query "<concept>"` or `python -m graphify path "<from>" "<to>"`.
  - Respect existing module boundaries and god nodes identified in the report.

### 2. After Completing Coding (Post-Task)
- **Always update the graph automatically**:
  - Whenever code files are created, edited, or deleted, automatically run:
    ```bash
    python -m graphify update . --force
    ```
  - This keeps `graphify-out/graph.json` and `graphify-out/graph.html` completely synchronized for the next task.
  - This command uses local AST parsing with tree-sitter: it has **zero API cost** and requires no external tokens.
