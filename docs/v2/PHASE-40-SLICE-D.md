# Phase 40 slice D: SP-level scale proof + the select() FD_SETSIZE fix

## §0. What was done

### The SP proof

`~/workspace/p11-probes/epoll_sp_proof.sp`: an SP program that opens
1,000 loopback connections, registers the server sides with the stdlib
`EventLoop` (READ), times 200 `poll(0)` calls, wakes exactly one
connection, and checks `poll()` reports exactly 1 ready event (and
reports it again on the next `poll(0)` — level-triggered re-report).

Results (same program, two binaries):

| | N=500 (200 calls) | N=1000 (200 calls) |
|---|---:|---:|
| poll backend (`build-phase10-fresh`) | 12,733,691 ns (~63.7 us/call) | **aborts** — see §0.1 |
| epoll backend (`build-phase11`) | 91,538 ns (~0.46 us/call) | 99,359 ns (~0.5 us/call) |

Both backends report `after write ready 1` and `still ready 1`. The
C++ probe (`poll_scale.cpp`, slice A) already showed the same win at the
`EventLoop` level (N=2000: 0.404 ms/call vs 0.001 ms/call); this slice
proves the SP surface delivers it.

### The fix the proof exposed (pre-existing bug, fixed in-slice)

**`select()` FD_SETSIZE abort.** `TcpSocket::connect` (timeout path) and
`TcpListener::acceptTimeout` waited with `select()` on an `fd_set`,
which is capped at `FD_SETSIZE` (1024). `FD_SET` on a larger fd aborts
the whole process via `__fdelt_chk`. Any SP program opening more than
~512 connections died in `connect()` before doing anything — the first
proof run aborted with `*** bit out of range 0 - FD_SETSIZE on fd_set
***` before printing anything, on BOTH binaries (pre-existing, not a
Phase 11 regression). The HTTP server's accept thread (`acceptTimeout`)
had the same cap.

Fix (`runtime/src/net/Net.cpp`, POSIX only; Windows keeps `select()`,
unchanged): both waits now use `poll()` on a single `pollfd`, which
takes the fd directly and has no cap. Semantics preserved:
- connect: wait for `POLLOUT|POLLERR`, then check `SO_ERROR` exactly
  as before (a refused connect reports POLLERR and is distinguished by
  `SO_ERROR`, as `select()` did).
- acceptTimeout: wait for `POLLIN|POLLERR|POLLHUP`, then `accept()`.
  The fd snapshot (Phase 31 race: `m_handle` changing mid-wait) is kept;
  its comment is updated (`select()` → wait).
- `<poll.h>` was already included by `Net.h`.

## §0.1 Two more things the proof exposed (NOT fixed — documented)

1. **`List<T>.get()` on a class element fails IR verification.**
   `List<TcpStream>.get(0)` dies with `'%v29' defined with type 'i64'
   but expected 'ptr'` — the generic method returns the element as i64
   and the caller uses it as a class pointer without a cast. This is the
   known Phase 7 follow-up ("generic List/Map methods fail IR
   verification"). The proof works around it: streams are held in Lists
   (add-only — `add` works), and the wake pair is created first and kept
   in locals. Fixing generic-method IR lowering is compiler work for a
   later phase, not this slice.
2. **A free function in a stdlib module does not bind through import.**
   `import std.time;` + bare `nowNanos()` fails with E1003; the working
   form is the class static `Time.nowNanos()`. (Compiler builtins like
   `tryReadLine` bind because they are registered in sema, not because
   they are stdlib free functions.) The proof uses `Time.nowNanos()`.

## §0.2 Verification

- SP proof: epoll backend N=1000 → `registered 1000`, `poll0 idle
  total 99359 ns for 200 calls`, `after write ready 1`,
  `still ready 1` (run twice, same output). N=500 on the poll backend →
  12,733,691 ns; N=1000 on the poll backend aborts (old `select()` code).
- C++ probes re-run: `poll_scale` (slice A numbers unchanged) and
  `sched_scale` (slice B numbers unchanged) — no regression from the
  `Net.cpp` change.
- Full ctest: **890/891** (`~/workspace/ctest-phase11-slice-d.log`) —
  only the standing `NetTest.UdpSocketSendRecv` EPERM failure. The
  connect/acceptTimeout paths are covered by
  `NetTest.TcpSocketListenConnectEcho` and the HTTP server tests, which
  all pass.

## §1. Out of scope (unchanged)

- Generic `List`/`Map` method IR lowering (follow-up, see §0.1.1).
- Stdlib free-function import binding (follow-up, see §0.1.2).
- HTTP server migration onto the loop; zero-alloc tasks (Phase 41).
- Docs (slice E).
