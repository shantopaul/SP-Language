# Phase 40 slice A: epoll backend for EventLoop

## §0. What was done

`runtime/src/net/EventLoop.cpp` + `runtime/include/sp/runtime/EventLoop.h`:
Linux now waits with `epoll`; every other platform keeps the existing
`poll()` / `WSAPoll` path. The `EventLoop` interface, the `Scheduler`, the
SP `EventLoop` class and the NetBridge C ABI are all untouched.

- Constructor: `epoll_create1(EPOLL_CLOEXEC)`. On failure `m_epollFd`
  stays `-1` and `poll()` transparently uses the `poll()` path (designed
  degradation, not a hard error).
- Destructor closes the epoll fd.
- `addSocket`: `epoll_ctl ADD` for a new fd, `MOD` for a re-add (the old
  code overwrote the map entry, so MOD preserves that semantic). ADD
  failing `EEXIST` (fd number recycled without a `removeSocket`) retries
  once with MOD; a failure of both erases the map entry and returns false.
- `modifySocket`: map update + `epoll_ctl MOD`. A MOD failure (fd closed
  behind our back) returns false but keeps the map entry — the same intent
  the `poll()` path would record. (No callers exist today: no NetBridge
  exposure, no SP surface; kept correct for the future.)
- `removeSocket`: map erase + `epoll_ctl DEL`; `DEL` failing `ENOENT`
  (fd already closed — closing a fd removes it from the epoll set
  automatically) is ignored; the map erase is authoritative.
- `poll()`: the epoll path runs `epoll_wait` into a **member** output
  buffer (`m_epollEvents`, grown only when the watched set grows), so it
  allocates nothing per call — the old path allocated a
  `std::vector<pollfd>` on every call. Empty watched set still returns 0
  immediately (never `epoll_wait(-1)` on an empty set). Timeout passes
  through in ms (`-1` = infinite); `EINTR` returns -1, as before.
- Event mapping: `EPOLLIN → READ`, `EPOLLOUT → WRITE`,
  `EPOLLERR|EPOLLHUP → ERROR`. A HUP arriving with IN yields READ|ERROR,
  exactly as `poll()`'s POLLIN|POLLHUP does.
- Level-triggered (no `EPOLLET`): readiness semantics are identical to
  `poll()` — a ready fd keeps reporting until drained.
- New accessor `usesEpoll()`: true on Linux with a live epoll instance,
  false otherwise (including the create-failed fallback), so callers and
  tests can observe the backend instead of guessing from timings.

## §0.1 Bug caught while writing (mine, fixed before building)

The first draft of `addSocket` called `epoll_ctl MOD` twice for a re-add:
the `if` condition ran the call, and the `else if` branch ran it again.
Rewritten to a single call with one ADD→MOD retry (see the phase doc
§0.3). No test caught it — it was found by re-reading the edit.

## §0.2 Verification

- Build: `~/workspace/build-phase11` (Release), clean.
- `NetTest.*` + `AsyncTest.*`: 44/45 — only the standing
  `NetTest.UdpSocketSendRecv` EPERM failure.
- Full ctest: **884/885** (`~/workspace/ctest-phase11-slice-a.log`) — same
  as the Phase 10 baseline; only the standing UDP test fails.
- Scale probe `~/workspace/p11-probes/poll_scale.cpp`
  (log `~/workspace/poll_scale-phase11.log`), N real idle connections:

  | N | poll backend | epoll backend |
  |---|---:|---:|
  | 100 | 0.013 ms/call | 0.001 ms/call |
  | 500 | 0.061 ms/call | 0.000 ms/call |
  | 1,000 | 0.179 ms/call | 0.000 ms/call |
  | 2,000 | 0.404 ms/call | 0.001 ms/call |

## §0.3 Findings

1. **The sandbox fd limit is a hard 4096** (`RLIMIT_NOFILE`; `setrlimit`
   fails even as root — "not allowed to raise maximum limit"). The honest
   measurement ceiling here is N=2,000 (4,001 fds). The phase doc §0.2
   table was corrected: its first version claimed N=5,000/10,000 rows
   that were really ~2,047 registered fds.
2. **The create-failed fallback is real and was observed, not assumed:**
   the first probe run built the loop while the process held ~4,095 fds,
   `epoll_create1` failed EMFILE, and the loop silently ran the poll()
   path at 0.425 ms/call. The probe now aborts if any socket fails, and
   `usesEpoll()` exposes the backend.
3. **An ABI trap for before/after probes:** the new header's inline
   `usesEpoll()` reads `m_epollFd`, which does not exist in the old
   object's layout — linking the new header against the Phase 10 lib
   prints garbage for that accessor. The "before" probe binary is built
   with the old header extracted from git (`d578d9e`) into
   `~/workspace/p11-probes/old_include/`; the "after" binary with the new
   header. The poll timings themselves are unaffected.

## §1. Out of scope (unchanged)

- Scheduler timer bookkeeping (slice B), new EventLoop tests (slice C),
  the SP-level scale proof (slice D), docs (slice E).
- HTTP server migration onto the loop (follow-up, see phase doc §0.4).
- Zero-alloc tasks (Phase 41, see phase doc §0.4).
