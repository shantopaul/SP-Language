# Phase 40 slice B: scheduler per-iteration cost (no heap copies)

## §0. What was done

`runtime/src/net/EventLoop.cpp` + `runtime/include/sp/runtime/Async.h`:
the scheduler no longer copies the timer heap on any per-iteration path.

- New member `m_pendingTimerIds` (`unordered_set<uint64_t>`, guarded by
  `m_cancelMtx` together with `m_cancelledTimers`): exactly the ids of
  timers currently in the heap that are still live. `scheduleAfter`
  inserts, `cancel` erases, `fireDueTimers` erases when a timer is popped
  (live or cancelled — the erase is idempotent, a cancelled id already
  left the set in `cancel()`).
- `hasPending()` / `pendingCount()` are now O(1) through the live set
  (plus `m_ready` / `m_socketCallbacks` sizes). Both stay `const` and
  public; `NetBridge`'s `sp_rt_async_scheduler_has_pending` and all
  `AsyncTest` callers are unchanged.
- `msUntilNextTimer()` no longer copies the heap. New private
  `pruneCancelledHead()` pops cancelled timers sitting at the heap head
  (they never fire, so removing them early changes nothing observable);
  the earliest live timer is then the heap head. `msUntilNextTimer()` is
  now non-const (it is private; its only caller is `runOnceCapped`). The
  round-up-to-whole-millisecond rule is unchanged.
- `runUntilTasksIdle()`'s drain check reads the live set instead of
  copying the heap. Side fix: the old copy-loop read `m_cancelledTimers`
  **without the lock**, which off-thread `cancel()` could race; the new
  read is locked.
- The `cancel()` return value is unchanged: true on the first `cancel()`
  call for an id (even if the timer already fired), false on a repeat.
- Lock discipline, stated once: `m_pendingTimerIds` and
  `m_cancelledTimers` are always accessed under `m_cancelMtx`. The prune
  check+erase runs under the lock; the pop itself runs without it (it
  destroys the `Timer`, whose `std::function` must never be destroyed
  while holding the mutex). Safe against off-thread `cancel()`: ids are
  monotonic and never reused, so a head seen cancelled stays cancelled,
  and a head seen live is left alone (if cancelled a moment later,
  `fireDueTimers()` skips it).

## §0.1 Why the old code was slow (measured, not assumed)

Probe `~/workspace/p11-probes/sched_scale.cpp` (BEFORE = slice A lib):
`hasPending()` + `pendingCount()` in a tight loop and `runOnce()`
iterations, with N far-future timers + 1,000 idle sockets + one
always-ready socket (so `poll()` never blocks):

| N timers | hasPending+pendingCount | runOnce |
|---:|---:|---:|
| 0 | 0.01 us/call | 1.17 us/call |
| 1,000 | 92.42 us/call | 7.81 us/call |
| 5,000 | 560.94 us/call | 32.07 us/call |
| 20,000 | 2827.96 us/call | 126.93 us/call |

Linear in N, as expected from the heap copies. Breakdown at N=20,000:
`pendingCount()` was the worst (~2.7 ms) because it drained the whole
copy — 20,000 heap pops plus 20,000 `std::function` destructions — while
`hasPending()` / `msUntilNextTimer()` each paid one copy (~130 us).

## §0.2 Verification

- Same probe, AFTER (slice B lib) — flat across N:

  | N timers | hasPending+pendingCount | runOnce |
  |---:|---:|---:|
  | 0 | 0.03 us/call | 1.54 us/call |
  | 1,000 | 0.03 us/call | 1.43 us/call |
  | 5,000 | 0.03 us/call | 1.35 us/call |
  | 20,000 | 0.06 us/call | 1.44 us/call |

  At 20,000 timers: query pair 2828 us → 0.06 us (~47000x);
  `runOnce` 127 us → 1.44 us (~88x). A loop iteration now costs
  O(ready), not O(registered) — for sockets (slice A) and timers
  (this slice).
- `AsyncTest.*` + `NetTest.*`: 44/45 — only the standing
  `NetTest.UdpSocketSendRecv` EPERM failure.
- Full ctest: **884/885** (`~/workspace/ctest-phase11-slice-b.log`) —
  same as the Phase 10 baseline; only the standing UDP test fails.
  (Timer/cancel behaviour is covered by the existing AsyncTest suite:
  `CancelledTimerNeverFires`, `CancelledTaskCancelsItsTimer`,
  `RunUntilTasksIdleDrainsTimersAndIgnoresSockets`,
  `CompletedTimerTaskLeavesNoPendingTimer` and the `hasPending()`
  assertions throughout all pass unchanged.)

## §1. Out of scope (unchanged)

- `Task` allocation (shared_ptr + mutex/CV + continuation vector) and
  the per-timer `std::function` allocations: that is the Phase 41
  zero-alloc-tasks work, not this slice. This slice removes the
  per-*iteration* allocations (heap copies), not the per-*task* ones.
- `runOnceCapped`'s copy of the ready-event list (`getReadyEvents()`):
  it is O(ready) — small with epoll — and protects against a re-entrant
  `poll()` inside a socket callback. Left alone deliberately.
- HTTP server migration onto the loop; zero-alloc tasks (Phase 41).
