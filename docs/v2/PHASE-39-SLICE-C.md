# Phase 39 slice C — bindgen on real headers (ADR-004)

## 0. Bug (verified 2026-11-10, `/usr/include/zlib.h` + `zconf.h`)

`spc bindgen /usr/include/zlib.h` produced **0 functions** and
output that did not parse (malformed `struct * {}`, unknown types).
Root causes, all in the hand-written tokenizer
(`tools/bindgen/Bindgen.cpp`):

1. **Macro prefixes defeat the function parser.** `ZEXTERN int
   ZEXPORT deflateInit(...)` — `parseCType` takes `ZEXTERN` as the
   return type, and the `ZEXPORT` before the name is never skipped.
   Same class: `FAR` (empty macro) between a type and `*` or an
   alias (`typedef Byte FAR Bytef;`, `typedef z_stream FAR
   *z_streamp;`), and `z_const` (const-or-empty macro) before a
   type (`z_const Bytef *next_in;`).
2. **Struct-returning functions dropped** (finding 5): `struct
   MiniPoint mini_make(int x, int y);` — the statement-level
   `struct` case pre-consumes `struct MiniPoint` and falls through,
   so the function parser starts at `mini_make` and treats it as
   the return type.
3. **`typedef struct X *Y;` misparsed as `struct * {}`**: the
   struct-typedef branch takes `*` as the alias name.
4. **Function-pointer typedefs/fields silently dropped**:
   `typedef voidpf (*alloc_func)(...)` and `voidpf
   (*zalloc)(...)` fields — the `(*name)(params)` declarator is
   never recognized.
5. **Typedefs never resolved at use sites**: `z_off64_t`-style
   names (here: `Bytef`, `uInt`, `uLong`, `voidpf`, `z_streamp`)
   stay as unknown base types in the output even though the
   typedef was recorded in `m_typedefAliases`.
6. **Anonymous struct/union fields misparsed**: an unnamed
   `struct { ... };` member breaks the field loop.
7. **`#include`d types invisible**: the tokenizer skips the
   directive, and the CLI has no `-I` flag even though
   `BindgenOptions.includeDirs` exists. zlib.h needs `zconf.h`
   (`Byte`, `uInt`, `uLong`, `Bytef`, `voidpf`, ...).
8. **Variadic `...` recorded but never emitted** by `generateSP()`,
   and the SP parser rejects `...` in extern declarations
   (finding 6: `fn printf(fmt: ptr<char>,...)` → E1000).

Bugs 9–16 were found DURING verification (the first zlib run
produced 63 functions, not 0 — the §0 list above is the
investigation's, these are what the real header exposed next):

9. **`extern "C" {` swallows the first declaration after it.**
   `skipAttributes` eats a bare `extern` at the parseTokens loop
   top, so the opener is never recognized; the function-case error
   recovery then skips to the next `;`, eating `alloc_func` (zlib
   lost exactly that typedef). Fix: check for `extern "C" {`
   BEFORE `skipAttributes`, and skip a stray `}` at statement
   level (the block's closer).
10. **`#ifdef` alternative macro definitions duplicate.**
    zconf.h's `#ifdef MAXSEG_64K → 8 / #else → 9` emitted
    `MAX_MEM_LEVEL` twice (a duplicate `const` is a hard SP
    error). Fix: dedupe macros by name in tokenizeC, last
    `#define` wins (C's rule; the `#else` is the default path).
11. **Pre-scan too greedy.** The `z_const` skip (`Ident Ident *`)
    also matched `z_stream FAR *z_streamp`, skipping the TYPE and
    leaving base `*`. Fix: skip only when the token after the
    candidate is a KNOWN type — then refined twice: (a) a
    qualifier (`const`) is not a type (`Byte const *voidpc` must
    not skip `Byte`); (b) the check skips qualifiers and allows
    convention macros between the type and `*`
    (`z_const unsigned char FAR *`).
12. **Blind base-type take.** `parseCType` took whatever token
    followed the qualifiers as the base: `unsigned *pending` →
    type `*`, `unsigned len` → type `len` (the parameter NAME).
    Fix: take only a primitive word or a real identifier; a
    qualifier with no type word after it means plain `int`.
13. **`#define NAME <type>` invisible.** `z_off64_t` is a macro
    (`#define z_off64_t off64_t`), not a typedef — the field
    `z_off64_t pos;` had no type. Fix: record single-identifier
    `#define` values as type aliases (separate map, see 15);
    `va_list` (`<stdarg.h>`, not on the include path) hard-maps
    to `ptr<void>` in `toSPType`; `off_t`/`off64_t` → `int64`.
14. **Cross-file `#define` clobbering.** zconf.h's inactive
    `#ifdef Z_PREFIX` renames (`Byte → z_Byte`, `voidpf →
    z_voidpf`, ...) are recorded as type aliases and overwrote
    real typedefs from earlier files at each file's parseContent.
    Fix (provenance split): real typedefs live in
    `m_typedefAliases`, `#define` aliases in
    `m_defineTypeAliases`; `resolveTypedef` consults real
    typedefs first.
15. **Struct names renamed through `#define`s.** With 14 fixed,
    `gz_header` still resolved to `z_gz_header` and
    `internal_state` to `z_internal_state`. Fix: `resolveTypedef`
    never `#define`-resolves a known struct/enum name.
16. **Macro before a typedef-named return type.** `ZEXTERN uLong
    ZEXPORT adler32(...)` — the type-loop all-caps skip only
    recognized type-start KEYWORDS, so `ZEXTERN` was taken as the
    return type, `uLong` as the function name, and the whole
    declaration was swallowed. 26 real functions were lost this
    way (adler32/crc32/compressBound/deflateBound/gzopen/gzseek/
    gztell/gzoffset/gzfread/gzfwrite/zlibCompileFlags + the 64-bit
    section). Fix: the all-caps skip also accepts a known
    typedef/`#define`-alias/struct name after the macro.

Note: this zlib (1.3) has **no `OF(...)` wrappers** (0 matches)
— the OF issue from the investigation does not apply here; no
OF-specific code is added.

## 0.1 Design

- **Macro prefixes**: `parseCType` skips all-caps identifiers
  (empty/convention macros: `FAR`, `ZEXPORT`, `ZEXTERN`) when the
  next token is a type-start keyword, `*`, another all-caps
  identifier, or a known typedef/`#define`-alias/struct name; and
  skips a non-keyword identifier followed by a known type and then
  `*` — the `z_const Bytef *p` pattern (qualifier-aware: `const`
  is not a type, so `Byte const *` never skips `Byte`). The
  function-declaration path additionally skips all-caps tokens
  followed by an identifier (the `ZEXPORT compress` position).
  A real all-caps typedef name is (almost) never eaten: it is
  followed by a declarator name, not by a type-start/`*`/macro/
  known-type.
- **Struct returns**: the statement-level `struct` case only
  consumes when the next token is `{` or `;`; otherwise it rewinds
  so the function parser sees `struct Name fn(...)`.
- **`typedef struct X *Y;`**: the struct-typedef branch records
  the alias in `m_typedefAliases` (`{X, pointerDepth 1}`) instead
  of creating a struct named `*`.
- **Fn pointers**: the `(*name)(params)` declarator is recognized
  in typedefs, function params, and struct fields. SP has no
  function-type syntax (Syntax Guide §30), so a fn pointer emits
  as `ptr<void>` — ABI-correct (8 bytes, pass-through works); the
  signature is not represented (documented). The typedef name is
  recorded in `m_typedefAliases` so use sites resolve to
  `ptr<void>`.
- **Typedef resolution**: `parseCType` resolves the base type
  through `m_typedefAliases` (chained, cycle-guarded), merging
  pointer depth and const/unsigned flags.
- **Anonymous struct/union fields**: an unnamed member flattens
  into the outer struct (layout-correct: C anonymous members sit
  at the outer offset); a NAMED anonymous struct/union field
  becomes a synthetic struct `<Outer>_<field>`.
- **`#include` following**: `parseFile` resolves `#include "..."`
  / `#include <...>` against the including file's directory +
  `includeDirs` (CLI: `spc bindgen -I <dir>` /
  `--include-dir <dir>`, repeatable) and recursively parses each
  file once (visited set). `parseHeader` (string input, used by
  tests) is unchanged.
- **Variadic**: `generateSP()` emits `, ...` for variadic fns, and
  the SP parser accepts `...` as the last parameter of an
  `extern "C"` declaration (recorded on the decl; the IR declares
  the function with its fixed params only — calls with extra args
  are a loud sema arity error, documented as a follow-up, not a
  silent miscompile). zlib HAS two variadic functions (`gzprintf`,
  `gzvprintf`, declared with `ZEXPORTVA`) — both are emitted with
  `...` and parse.

## 0.2 Verification (actual, 2026-11-10)

- `spc bindgen /usr/include/zlib.h -I /usr/include --module zlib
  -o zlib.sp` → **4 structs, 89 functions, 0 enums, 29
  constants**; `spc check zlib.sp` passes.
- **Function census (verified with a comment-stripping script):**
  the comment-stripped header has exactly 89 `ZEXTERN ... ZEXPORT`
  prototypes and the generated file has exactly the same 89 names
  — none missing, none extra. (A naive `grep -c ZEXPORT` says 111:
  5 of those are `deflateInit`/`deflateInit2`/`inflateInit`/
  `inflateInit2`/`inflateBackInit` prototypes shown inside `/* */`
  doc comments — correctly absent, the real API is the `*_`
  functions + `#define` macros; 2 more are `gzprintf`/`gzvprintf`
  declared with `ZEXPORTVA`, which the naive grep misses.)
- **Spot checks on the generated types:** `strm: ptr<z_stream>`,
  `zalloc`/`zfree: ptr<void>`, `state: ptr<internal_state>`,
  `head: ptr<gz_header>`, `opaque: ptr<uint8>` (voidpf's real
  typedef is the `Byte FAR *` `#ifdef` branch — ABI-identical),
  `pos: int64` (`z_off64_t` → `z_off_t` → `long`),
  `MAX_MEM_LEVEL` appears once with value 9 (the default branch),
  `pending: ptr<uint32>`, `in: ptr<void>` (`in_func`), variadic
  `fn gzprintf(file: ptr<gzFile_s>, format: ptr<char>, ...):
  int32;`. The 7 unnamed-parameter prototypes keep synthetic
  `arg0`/`arg1` names (they are unnamed in the header).
- **Live round-trip** (`~/workspace/p10-probes/zlib_roundtrip.sp`,
  imports the generated `zlib.sp`, linked with `--link z`):
  `compress rc 0`, `compressed 31 bytes`, `uncompress rc 0`,
  `recovered 23 bytes:`, `Hello from SP via zlib!` — the recovered
  string is byte-identical. (Note: the proof prints via `puts`,
  not `write` — `write(2)` bypasses the stdio buffer `println`
  uses, so its output appeared before the `println` lines.)
- **Minilib regression:** `spc bindgen mini.h` now emits 9
  functions (was 7) — `mini_make`/`mini_add_point` (struct
  returns) appear; output `spc check`s; `use_mini.sp` against the
  regenerated bindings still prints `add 42 / scale 14 / sum 7 /
  filled 9 10`.
- **Variadic declaration:** `fn printf(fmt: ptr<char>,...)` parses
  under `spc check` (declaration only; calls with extra args are
  still a loud sema arity error).
- **Tests:** 14 new `BindgenTest` cases (7 planned + 7 for bugs
  found during verification: extern-"C" guard, duplicate-macro
  dedupe, const-qualified pointer typedef, `#define` type alias +
  `va_list`, cross-file clobber, struct rename, macro-before-
  typedef-return). Total 880 tests.
- **Full ctest: 879/880** (log
  `~/workspace/ctest-phase10-slice-c.log`) — only the standing
  `NetTest.UdpSocketSendRecv` (UDP EPERM, environmental); PG
  skipped.

## 0.3 Out of scope

- libclang bindgen (§0.4 decision stands: tokenizer hardened).
- Variadic CALLS through a C fn (declaration only; see §0.1).
- Fn-pointer signatures in SP (no fn-type syntax — `ptr<void>`).
- Preprocessor conditionals (`#ifdef`): the tokenizer ignores
  directives, so BOTH branches' declarations are emitted when a
  header uses `#ifdef` for alternative declarations — verified on
  zlib: the `#ifdef Z_LARGE64` 64-bit variants (`gzopen64`,
  `gzseek64`, ...) AND the `#else` regulars (`gzopen`, `gzseek`,
  ...) are all present (89 total); a name declared in both
  branches keeps its FIRST occurrence (`m_knownFunctionNames`).
  For alternative `#define` VALUES the rule is the opposite —
  last wins (MAX_MEM_LEVEL 9; `z_off64_t` → `z_off_t` → `long`),
  matching C. Inactive-`#ifdef` renames (`Z_PREFIX`) are
  neutralized by the provenance split (§0 bugs 14–15), but a
  header whose two branches declare DIFFERENT signatures under
  one name will still silently keep the first — a libclang
  bindgen would evaluate the condition instead.
