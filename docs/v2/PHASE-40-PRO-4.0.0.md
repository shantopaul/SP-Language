# Phase 40 (pro-4.0.0): ADR-003 epoll event loop

## Status
DONE — 2026-11-10. Slices A–E all done; see §1.

## §0. Investigation (2026-11-10, base `d578d9e`, Phase 39 complete)

### §0.1 What already exists

- **`EventLoop`** (`runtime/src/net/EventLoop.cpp`, 809 lines incl. the async
  runtime): `addSocket` / `modifySocket` / `removeSocket` / `poll(timeoutMs)`.
  Backend is `poll()` on POSIX and `WSAPoll` on Windows. `poll()` rebuilds
  the entire `pollfd` array from `m_watched` on **every** call — O(n) setup
  plus the kernel's O(n) scan. Zero `epoll` anywhere in the tree.
- **`Scheduler`** (`sp::rt::async`, same file): timer heap
  (`priority_queue<Timer>`), ready deque, socket callbacks
  (`m_socketCallbacks`). `runOnceCapped` = fire due timers → drain ready →
  `m_loop.poll(timeout)` with the timeout derived from the next timer
  (`msUntilNextTimer()`), or a direct sleep when no sockets are watched.
  `runUntilIdle` / `runUntilTasksIdle` / `runUntil` / `runFor` drive it.
  Single-threaded by design; `Task` state is mutex/CV protected so a
  worker thread may complete a Task, but scheduling is loop-thread only.
- **`Task`**: `shared_ptr` + per-task `std::mutex` + `condition_variable` +
  `std::string` + `vector<continuations>`. Every `sleepTask` /
  `delayedValueTask` allocates a Task, two `std::function`s (timer callback
  + the Phase 6-C cancel hook) and a timer heap node.
- **Compiler async**: Phase 5 did the heap-frame state-machine lowering
  (`IRBuilder::AsyncSM`); `main` exits through
  `sp_rt_async_scheduler_run_tasks_until_idle` (`emitMainExitPump`).
- **HTTP server does NOT use the loop.** `HttpServer::listen` starts an
  accept thread (`serverLoop`, blocking `acceptTimeout(100)`) plus a worker
  thread pool (default `hardware_concurrency`, min 2); connections are
  handled with blocking I/O on worker threads. The ADR's "tens of
  thousands of idle connections" goal cannot be reached by the server as
  it stands — that needs a connection-state-machine redesign, not a
  backend swap (see §0.4).
- **SP surface**: `stdlib/net/net.sp` `EventLoop` class wraps
  `sp_rt_eventloop_create/add/remove/poll/destroy` (NetBridge.cpp:217–245).
  Tests: `tests/runtime/NetTest.cpp` (`EventLoopPolling`, 1 connection),
  `tests/runtime/AsyncTest.cpp` (~30 tests: timers, cancel, spawn, all/race,
  C ABI).

### §0.2 Measured: poll() scaling (probe `~/workspace/p11-probes/poll_scale.cpp`)

Real SP `EventLoop`, N idle loopback connections, `poll(0)`, 200 iterations
each, vs raw `epoll_wait(0)` on the same fd set (this VM, 2026-11-10;
log `~/workspace/poll_scale-phase11.log`):

| N idle fds | `EventLoop::poll(0)` (poll backend) | `EventLoop::poll(0)` (epoll backend) | ratio at N |
|---|---:|---:|---:|
| 100 | 0.013 ms/call | 0.001 ms/call | ~13x |
| 500 | 0.061 ms/call | 0.000 ms/call | ~150x |
| 1,000 | 0.179 ms/call | 0.000 ms/call | ~450x |
| 2,000 | 0.404 ms/call | 0.001 ms/call | ~400x |

The scheduler calls `poll()` on **every** `runOnce` while any socket is
registered, so a server with 2,000 idle connections burns ~0.4 ms per loop
iteration just to discover nothing is ready. epoll pays ~0.001 ms and
scales with the number of *ready* fds, not registered ones.

**Correction to the first version of this table (2026-11-10):** it claimed
N=5,000/10,000 rows (0.408/0.417 ms/call). Those rows were wrong: this
sandbox caps `RLIMIT_NOFILE` at 4096 (hard — `setrlimit` fails even as
root, "not allowed to raise maximum limit"), so past ~2,047 pairs
`socket()`/`accept()` fail EMFILE and only ~2,047 fds were ever
registered. The honest ceiling here is N=2,000 (4,001 fds). The probe now
aborts if any socket fails, so every row above is N real connections.

**The fd limit also proved the fallback works:** the first probe run at
N=5,000 built the `EventLoop` while the process held ~4,095 fds, so
`epoll_create1` failed EMFILE and the loop silently used the poll() path
(0.425 ms/call — identical to the poll backend). That is the designed
degradation (§0.3), and `usesEpoll()` now exposes it so callers and tests
can observe which backend is live instead of guessing from timings.

### §0.3 Design: epoll backend, same interface

- `EventLoop` keeps its exact interface (`addSocket` / `modifySocket` /
  `removeSocket` / `poll` / `getReadyEvent(s)`) and the NetBridge C ABI is
  untouched, so the SP `EventLoop` class, the `Scheduler`, and all existing
  tests work unchanged.
- Linux (`#if defined(__linux__)`): `epoll_create1(EPOLL_CLOEXEC)` in the
  constructor; `epoll_ctl ADD/MOD/DEL`; `epoll_wait` in `poll()`.
  Everything else keeps the existing `poll()` / `WSAPoll` path.
- **Level-triggered** (default) — identical readiness semantics to `poll()`;
  no `EPOLLET`. A ready fd keeps reporting until drained.
- Event mapping: `EPOLLIN → SP_EVENT_READ`, `EPOLLOUT → SP_EVENT_WRITE`,
  `EPOLLERR|EPOLLHUP → SP_EVENT_ERROR` (a HUP that arrives with IN yields
  READ|ERROR, exactly as `poll()`'s POLLIN|POLLHUP does).
- `addSocket` on an already-watched fd must `epoll_ctl MOD` (ADD would
  fail `EEXIST`); the current code overwrites the map entry, so MOD
  preserves that overwrite semantic. `modifySocket` on an unwatched fd
  still returns false. `removeSocket`: `epoll_ctl DEL` (ENOENT treated as
  success when the map held the fd — the fd may already be closed, which
  auto-removes it from the epoll set) + map erase.
- `poll()` with no watched sockets still returns 0 immediately (never
  `epoll_wait(-1)` on an empty set — that would block forever).
  Timeout passes through in ms (`-1` = infinite), same as `poll()`.
  `EINTR` returns -1, same as the `poll()` path (the scheduler sleeps 1 ms
  and does not spin).
- `epoll_event.data.fd` carries the fd; `userData` is recovered from
  `m_watched` (same lookup as today). The `epoll_event` output buffer is a
  member vector, grown only when the watched set grows — **no per-call
  allocation** (today `poll()` allocates a `std::vector<pollfd>` per call).
- Threading contract unchanged: the loop is driven from one thread;
  `addSocket`/`removeSocket` are loop-thread operations, as today.

### §0.4 DECISION: this phase is the epoll loop only

**Zero-alloc tasks is NOT in this phase; it is Phase 41.** The ADR's
evolution phase reads "async fn state machine lowering with zero-allocation
tasks" — the state-machine lowering exists (Phase 5); the "zero-allocation"
half was never scoped. It needs its own allocation census (per-task:
shared_ptr control block + mutex + CV + string + continuation vector;
per-timer: two `std::function` heap allocations; per-async-call: heap
frame) and a design decision about the `Task` mutex/CV contract (worker
threads complete tasks today). Designing it blind inside this phase would
repeat the Phase 39 slice-A mistake (first design proven wrong by
experiment). Overridable by one line from Shanto.

**The HTTP server migration onto the loop is NOT in this phase either.**
It is a follow-up: connection handling is blocking I/O on worker threads
and must become per-connection state machines. This phase delivers the
loop and proves it at the `EventLoop` / `Scheduler` level; the server
migration is the consumer that needs it (and the zero-alloc tasks of
Phase 41, since each connection's handler would become a cheap task).

### §0.5 Per-iteration allocations in the loop (slice B)

Beyond the `pollfd` rebuild that epoll removes, the scheduler copies the
**whole timer heap on every loop iteration**: `msUntilNextTimer()` copies
the `priority_queue` to skip lazily-cancelled timers at the head, and
`hasPending()` / `pendingCount()` copy it the same way. With N pending
timers that is an O(n) allocation per `runOnceCapped`. Slice B removes
those copies (prune cancelled timers at the head of the real heap +
a pending-timer set with an O(1) live count), so a loop iteration costs
O(ready), not O(registered) — for sockets *and* timers.

### §0.7 Repo AGENTS.md Graphify protocol — still impossible

Repo `AGENTS.md` mandates a Graphify protocol (`python -m graphify update .
--force` before/after coding). Verified again 2026-11-10: `graphify-out/`
does not exist, `python -m graphify` is not installed. Not fabricated;
same as Phases 1–10.

### §0.6 Slices

- **A — epoll backend** (`runtime/src/net/EventLoop.cpp`,
  `runtime/include/sp/runtime/EventLoop.h`): Linux epoll behind
  `__linux__`, poll/WSAPoll fallback preserved. Interface + ABI unchanged.
- **B — scheduler per-iteration cost**: timer bookkeeping without heap
  copies (prune + pending set + O(1) counts); probe measuring loop-iteration
  cost with N timers + N idle sockets, before/after.
- **C — SP surface + tests**: `NetTest` EventLoop tests (1,000 idle sockets
  functional, modify/remove, error events, timeout semantics); `AsyncTest`
  green unchanged; SP-level `EventLoop` smoke via the existing surface.
- **D — scale proof**: SP program driving N idle connections through the
  stdlib `EventLoop` + timing; C++ `poll_scale` probe re-run against the new
  runtime lib; before numbers are the §0.2 table (poll backend, Phase 10 lib).
- **E — docs + doc gate**: `std/net` event-loop section in
  `STDLIB_REFERENCE.md` (and a Syntax Guide note if a section fits) with a
  runnable example; doc gate; completion.

## §1. Completion

**Status: DONE 2026-11-10.** Range `d578d9e..76712b5` (Phase 39 head to
slice E) = 5 slice commits, 14 files, +1,086/−48. Branch
`feature/pro-4.0.0` only; `main` still `d10e1c2`.

- **A — epoll backend:** Linux waits with `epoll` (level-triggered,
  same interface + ABI, `poll()`/`WSAPoll` fallback preserved,
  `usesEpoll()` accessor). N=2000 idle fds: `poll(0)` 0.404 ms/call →
  0.001 ms/call (~400x).
- **B — scheduler per-iteration cost:** live-timer set +
  prune-cancelled-heads; no heap copies in `hasPending()` /
  `pendingCount()` / `msUntilNextTimer()` / `runUntilTasksIdle()`.
  N=20,000 timers: query pair 2828 us → 0.06 us; `runOnce` 127 us →
  1.44 us. A loop iteration now costs O(ready) for sockets and timers.
- **C — tests:** 6 new (5 NetTest EventLoop + 1 AsyncTest pending-count);
  NetTest+AsyncTest 50/51 (only standing UDP EPERM).
- **D — SP scale proof + `select()` fix:** SP program, 1,000 idle
  connections: `poll(0)` 200-call total ~99 us (~0.5 us/call) vs the
  poll backend's ~12.7 ms at N=500 (N=1000 aborts there). Proof exposed
  the pre-existing `select()` FD_SETSIZE abort in `connect()` /
  `acceptTimeout` — both now use `poll()` on POSIX (Windows unchanged).
  Also documented, not fixed: `List<T>.get()` on a class element fails
  IR verification; stdlib free functions do not bind through import.
- **E — docs + gate:** STDLIB_REFERENCE §8 `std/net`; Syntax Guide §83
  event-loop subsection with a verified runnable example. Gate
  **123/131 RUN-OK** (was 123/130; the new block passes, the 8 non-OK
  are the known baseline classes with the +1 shift).

**Final verification (fresh build):** ctest **890/891** (only standing
UDP EPERM; PG skipped). All live proofs re-run with the fresh binary
(see the completion log).

**Follow-ups (not this phase):** HTTP server migration onto the loop
(accept thread + worker pool today); zero-alloc tasks = **Phase 41**
(needs its own allocation census + design — see §0.4); generic
`List`/`Map` method IR lowering; stdlib free-function import binding;
`unwrap` panic on the wrong variant and the other Phase 38/39 follow-ups
(listed in the Phase 38/39 docs) are unchanged.
