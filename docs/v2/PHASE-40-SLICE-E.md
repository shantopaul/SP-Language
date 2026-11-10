# Phase 40 slice E: docs (std/net event loop) + doc gate

## §0. What was done

- `docs/STDLIB_REFERENCE.md`: new **§8 `std/net`** — `TcpStream` /
  `TcpListener` essentials plus an `EventLoop` subsection (interest bits:
  read `1`, write `2`, error `4`; level-triggered semantics;
  `remove` / `destroy`; the Phase 40 epoll scaling note with measured
  numbers; the HTTP server is not driven by this loop yet).
- `docs/SP-Syntax-Guide.md` §83 (TCP Echo Server): new subsection
  **"Event loop: many connections, one thread"** with a runnable
  ```sp example (verified standalone BEFORE writing — probe
  `~/workspace/p11-probes/sg83_loop.sp` → `idle 0 / ready 1 / data hi`)
  plus its Output block and the epoll scaling note. No section was
  renumbered (a subsection, not a new §), so the doc-gate numbering
  shifts by exactly +1 after §83.

## §0.1 Verification

- Doc gate (`~/workspace/doc-gate-phase11-slice-e.log`,
  extract `~/workspace/extract-phase11-slice-e.log`):
  **123/131 RUN-OK** (was 123/130 in Phase 10). Total blocks 130 → 131:
  the new §83 block is RUN-OK.
- The 8 non-OK map exactly to the known baseline classes, with the +1
  shift after §83: ex045/ex046 SKIP-CHECK-FAIL (§38), ex007/ex067 /
  ex109 RUN-TIMEOUT (server examples; ex109 is Phase 10's ex108, the
  HTTP server block), ex044/ex129/ex130 RUN-FAIL misleading backend
  message (Phase 10's ex044/ex128/ex129). No new failure class.
- Gate orphan: the ex109 TIMEOUT left `sp_run_ex109_30` (pid 37786)
  listening on port 8080; killed by exact PID (never `pkill -f`),
  port confirmed free afterward.

## §1. Out of scope (unchanged)

- HTTP server migration onto the loop; zero-alloc tasks (Phase 41).
- Generic `List`/`Map` method IR lowering; stdlib free-function import
  binding (both documented in slice D).
