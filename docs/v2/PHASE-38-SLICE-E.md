# Phase 38 (ADR-002) — Slice E: stdlib migration audit + docs + doc gate

Phase: 38 (pro-4.0.0) · Slice E · ADR-002 (Result/Option)
Base: Slice D commit `bf0d505` on `feature/pro-4.0.0`
Status: DONE (implementation follows; §0.6 records two real bugs found)

## 0. Audit — where Result/Option stands in the stdlib

Method: every `*.sp` under `stdlib/` (24 modules, 6,388 lines) was
read for its fallible surface — what a function returns when the
operation fails — and for any existing `Result<` / `Option<` use.

**Result: only 2 of 24 modules use Result/Option at all** (15
functions returning `Result<...>` / `Option<...>`, all in `std/fs`
and `std/io`). Every other module reports failure through one of
four older patterns:

| module | fallible surface | failure pattern today | Result/Option? |
|---|---|---|---|
| `std/fs` | read/write/append/delete/createDir | `try*` variants return `Result<..., IOError>` (real) | YES — except `listDir` (see §0.1) |
| `std/io` | `readLine` | `tryReadLine` returns `Result` but can never fail (EOF returns `""`) | PARTIAL (see §0.2) |
| `std/db` (pg/sqlite) | connect/query/prepare/txn | result object + `lastError()` (`PgResult.isValid()` / `error()`, `PgConnection.isConnected()` / `lastError()`); bool for prepare/ping/txn | NO |
| `std/json` | `JsonValue.parse` | returns a `JsonValue` whose kind is `Error` on bad input — check `isValid()` | NO |
| `std/crypto` | `base64Decode` | returns `""` on invalid input (silent); sha256/hmac are infallible | NO |
| `std/net` | `TcpStream.read/write/accept` | `read` returns `""` on disconnect/timeout (indistinguishable from empty data); `write` returns int bytes; `accept` returns a `TcpStream` (null on failure) | NO |
| `std/redis` | `get/set/del` | `get` returns `""` on a missing key AND on no-connection (indistinguishable from an empty value); `set`/`del` return bool | NO |
| `std/http` (client) | `HttpClient.get/post` | returns `HttpResponse` — check `status()` | NO |
| `std/os` | `getEnv` | returns `""` for a missing variable | NO |
| `std/process` | `exec/spawn/runCommand` | int exit code / string output, no error channel | NO |
| `std/collections` | `List.get` / `Map.get` | returns the zero value for a missing key — use `containsKey()` | NO |
| `std/string`, `std/math`, `std/time`, `std/sync`, `std/queue`, `std/log`, `std/metrics`, `std/health`, `std/security`, `std/async`, `std/test`, `std/mem`, `std/web` | — | infallible, or bool/int sentinels where failure is possible | NO |

The `try*` convention (established in `std/fs`) is: the plain
function keeps its old return type (a convenience that cannot report
failure), and a `try*` sibling returns `Result<T, E>` with a
domain error enum (`IOError` in fs/io). Migration is **additive
only** — no existing function changes its signature, so no call
site breaks.

## 0.1 Bounded migration — `std/fs`: `tryListDir`

Gap: `listDir` returns `string[]`, and the runtime returns an empty
array on error (missing dir, not-a-dir, iteration failure) —
indistinguishable from an empty directory.

**Implementation note (the §0.6 finding): the stdlib `.sp` files are
reference only — the fs/io API is hard-coded as compiler builtins.**
`tryListDir` therefore lives in four places, mirroring `tryReadText`:

- `compiler/src/sema/SymbolTable.cpp` — free `tryListDir`:
  `Result<string[], IOError>` (via `resolveType("Result<string[], IOError>")`).
- `compiler/src/sema/Type.cpp` — `FS.tryListDir` method:
  `specializeEnumType(resultEnum, {"string[]", "IOError"})`
  (the `File` class has no `listDir`, so nothing is added there).
- `compiler/src/ir/IRBuilder.cpp` — lowering in both forms (member
  `FS.tryListDir` and free `tryListDir`): `sp_rt_fs_exists` →
  `Err(NotFound = 0)`; `sp_rt_fs_is_dir` → `Err(InvalidInput = 3)`;
  else `sp_rt_fs_list_dir` → `Ok(entries)`. Same shape as the
  `tryReadText` lowering (result slot in the entry block, then/else/
  merge blocks, `emitPopulateTaggedUnion`).
- `stdlib/fs/fs.sp` — the SP reference implementation (free fn +
  `FS.tryListDir` static), kept in sync with the builtin.

The pre-check has the same TOCTOU shape as `tryReadText` (a path
deleted between the check and the read lists as `Ok([])`) —
documented, not fixed here; a real fix needs the runtime to report
the error, which is out of scope.

## 0.2 Bounded migration — `std/io`: a real `tryReadLine`

Gap: `tryReadLine` always returns `Result.Ok(line)` — `sp_rt_read_line`
returns `""` on EOF, indistinguishable from an empty line.

- Runtime (`runtime/include/sp/runtime/Runtime.h` +
  `runtime/src/Runtime.cpp`, next to `sp_rt_read_line`):
  `bool sp_rt_stdin_eof();` — returns `std::cin.eof()`.
- `compiler/src/ir/IR.cpp` — the generated module must declare the
  new runtime function (`createFunction("sp_rt_stdin_eof", i1, {}, true)`)
  next to `sp_rt_read_line`, or IR verification fails with "call to
  undeclared function 'sp_rt_stdin_eof'".
- `compiler/src/ir/IRBuilder.cpp` — both forms (member
  `IO.tryReadLine` and free `tryReadLine`) rewritten: read the line,
  call `sp_rt_stdin_eof`, load the line's first byte, `"" + eof set`
  → `Err(UnexpectedEOF = 4)`, else `Ok(line)`.
- `stdlib/io/io.sp` — the SP reference implementation (free fn +
  `IO.tryReadLine`), kept in sync with the builtin.

EOF semantics (verified against `std::getline`): a last line without
a trailing newline returns its text with eof set → `Ok(text)` (the
line is non-empty, so the check does not fire); an empty line
(`"\n"`) returns `""` with eof clear → `Ok("")`; immediate EOF
returns `""` with eof set → `Err(UnexpectedEOF)`; a file ending
with `"\n"` yields `Ok("")` for the empty line, then `Err` on the
next read. Correct.

## 0.3 Not migrated (audit only — out of scope for this slice)

`std/db`, `std/json`, `std/crypto`, `std/net`, `std/redis`,
`std/http`, `std/os`, `std/process`, `std/collections`: migrating
any of them changes its error contract (a `PgResult` becomes
`Result<PgResult, DbError>`, a `get` becomes `Option<string>`) and
every call site that uses it. That is a per-module design decision
with its own error enums — a follow-up phase per module, not this
slice. The audit table above is the record for those decisions.

## 0.4 Docs

- **`docs/STDLIB_REFERENCE.md`** (not doc-gate-tested): the `std/fs`
  section documents functions that do not exist (`FS.readString` /
  `FS.writeString` / `FS.fileSize` / `FS.remove` / `FS.createDirectory`)
  — rewritten to the real API (`readText` / `writeText` /
  `appendText` / `delete` / `createDir` / `listDir` / `exists` /
  `isFile` / `isDir`) plus the `try*` variants and `tryListDir`;
  the `std/io` section gains `readLine` / `tryReadLine` (with the
  EOF rule). A short "Error handling" note states the `try*`
  convention and the audit status (fs/io migrated; the rest use
  result objects / sentinels — see the table in §0).
- **`docs/SP-Syntax-Guide.md` §28** (doc-gate-tested — any new
  ```sp block must RUN): the note "match on a Result does not work
  … use `isOk()` / `isErr()` instead" is stale since slice B —
  replaced with a match-binding example (a `Result` matched with
  `Result.Ok(v)` / `Result.Err(e)` bindings) and a short paragraph
  on payload ownership (boxed struct / multi-payload / nested
  payloads work; `?` composes with `async fn`; a struct payload
  yielded by `?`/`unwrap` is a snapshot copy).

## 0.5 Findings — two real bugs (both fixed in-slice)

**1. The stdlib `.sp` files are not the source of truth for the
fs/io API.** The first implementation put `tryListDir` and the new
`tryReadLine` only in `stdlib/fs/fs.sp` / `stdlib/io/io.sp` and
rebuilt `spc` — and the probes still failed with E1003 "Undefined
function 'tryListDir'", while `tryReadText` kept working. A trivial
marker function added to `fs.sp` was equally invisible, proving the
import does not load the file's free functions at all. The real
registrations are compiler builtins: `SymbolTable.cpp` defines the
free functions (`readText`, `listDir`, `tryReadText`, …),
`Type.cpp` adds the `FS`/`File`/`IO` methods, and `IRBuilder.cpp`
lowers each one by hand to a runtime call (e.g. `tryReadText` at
IRBuilder.cpp:6572/9577). The `.sp` files are reference
implementations that must be kept in sync with the builtins — they
are not executed. **Lesson for future stdlib work: a new stdlib
function needs the sema registration + the IR lowering, not just
the `.sp` file; verify with `spc run`, never with `spc check` of
the `.sp` file alone.**

**2. Duplicate block names across repeated `try*` calls in one
function = infinite loop.** The `tryListDir` probe calls it four
times in one `main`. The program printed `home ok false` forever
(72 MB of output before it was killed). Root cause: `BranchInst` /
`CondBranchInst` store only the target label (a string), and the
`try*` lowerings created blocks with fixed names (`"try_list_ok"`,
`"try_list_end"`, …) — `createBasicBlock` does not uniquify. The
second call's branches resolved by name to the FIRST call's blocks,
jumping back to the first `println`. This is a latent bug in ALL
the pre-existing `try*` lowerings (`tryReadText`, `tryWriteText`,
`tryDelete`, `tryCreateDir` — same fixed names, same failure if
called twice in one function; no test ever did). Fix: all 43
`try*` block creations (mine + the 8 pre-existing lowerings) now
use `IRBuilder::createBlock(name)`, which suffixes a per-module
counter (`name_N`, reset in `build()`). No code looks these blocks
up by name, so the rename is safe. After the fix the probe prints
`home ok true / missing err true / file err true / fs ok true`
and terminates.

## 0.6 Verification

- New tests in `ExecutionTest.cpp`:
  1. `FsTryListDir` — `tryListDir` on a real directory (Ok, entries
     listed), on a missing path (Err NotFound), on a file (Err
     InvalidInput). (Uses the test harness's temp dir if one exists,
     else a directory created by the test itself.)
  2. `IoTryReadLineEof` — `tryReadLine` on input `"hello"` (no
     trailing newline): first read `Ok("hello")`, second read
     `Err(UnexpectedEOF)`; on input `"\n"`: first read `Ok("")`
     (empty line, not an error), second read `Err`.
- Full ctest after the change: **858/859** (log
  `~/workspace/ctest-phase9-slice-e.log`) — only the standing
  `NetTest.UdpSocketSendRecv` EPERM failure; PG skipped without a
  server. The two new tests (`ExecutionTest.FsTryListDir`,
  `ExecutionTest.IoTryReadLineEof`) pass.
- Doc gate re-run (log `~/workspace/doc-gate-phase9-slice-e.log`):
  **121/129 RUN-OK** (was 119/128 — one new block, the §28 match
  example ex034, passes). The 8 non-OK are the same known
  categories, numbering shifted +1 after §28: ex044/ex127/ex128
  RUN-FAIL (misleading "Unsupported backend compiler" message),
  ex045/ex046 SKIP-CHECK-FAIL, ex007/ex066/ex107 RUN-TIMEOUT
  (servers), ex121 remains the orphan-server false pass. No new
  failures.
