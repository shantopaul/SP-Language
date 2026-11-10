# Phase 40 slice C: SP surface + tests for the epoll loop

## §0. What was done

No production code changed in this slice: the `EventLoop` interface, the
`Scheduler` and the SP `EventLoop` class are unchanged by slices A–B, so
this slice is verification — new tests plus a check that the SP surface
still works through the new backend.

### New tests (6)

`tests/runtime/NetTest.cpp`:

- `EventLoopUsesEpollOnLinux` — a fresh loop reports `usesEpoll() == true`
  on Linux, false elsewhere. (A test process has plenty of fds, so
  `epoll_create1` succeeds; the fd-exhausted fallback is covered by the
  probe, not by a test.)
- `EventLoopEmptyPoll` — `poll(0)` / `poll(10)` on an empty loop return 0;
  `addSocket(SP_INVALID_SOCKET)` returns false.
- `EventLoopManyIdleSockets` — 500 loopback pairs registered READ:
  `poll(0)` returns 0; after writing to exactly one client, `poll(1000)`
  returns exactly 1 event, for the right fd, with `SP_EVENT_READ`.
- `EventLoopModifyAndRemove` — `modifySocket` / `removeSocket` on an
  unknown fd return false; a re-add overwrites the interest (proved by
  switching READ → WRITE, which is ready at once because the send buffer
  is empty, then back to READ); after `removeSocket`, pending data is no
  longer reported.
- `EventLoopErrorEvent` — a peer hard-closed with `SO_LINGER` 0 (RST)
  reports `SP_EVENT_ERROR` (arriving with READ). Guarded to
  `__unix__ || __APPLE__` for the `setsockopt` call.

`tests/runtime/AsyncTest.cpp`:

- `PendingCountTracksLiveTimersAtScale` — 5,000 far-future timers:
  `pendingCount()` is exactly 5,000, drops to 2,500 after cancelling
  half, is unchanged by a repeat cancel (returns false), drains to 0
  after cancelling the rest, and stays 0 after cancelling an unknown id
  (returns true — first call for that id, as before).

## §0.1 Test bug found by running (mine, fixed in-slice)

The first version of `EventLoopErrorEvent` closed the peer gracefully
and expected `SP_EVENT_ERROR`. It failed: a graceful FIN close reports
READ (EOF) only — the kernel sets `POLLIN|POLLRDHUP`, not `POLLHUP` —
on the `poll()` backend too. The ERROR bit is for RST / socket errors,
so the test now forces an RST (`SO_LINGER` 0, verified by a standalone
probe: `events=5`, i.e. READ|ERROR). The runtime code was correct; the
test's expectation was wrong. This is now documented in the test comment
so the next reader does not "fix" it back.

## §0.2 Verification

- `NetTest.*` + `AsyncTest.*`: **50/51** — only the standing
  `NetTest.UdpSocketSendRecv` EPERM failure.
- Full ctest: **890/891** (`~/workspace/ctest-phase11-slice-c.log`) —
  885 + 6 new; only the standing UDP test fails.
- SP surface: the `EventLoop` class in `stdlib/net/net.sp` is untouched
  (same `sp_rt_eventloop_*` ABI) and the existing `EventLoopPolling`
  behaviour is covered by the new tests at the same level; the SP-level
  scale proof is slice D.

## §1. Out of scope (unchanged)

- Scale proof through the SP program (slice D), docs (slice E).
- HTTP server migration onto the loop; zero-alloc tasks (Phase 41).
