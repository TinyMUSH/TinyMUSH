# Known Bugs in TinyMUSH

As of January 3, 2026, **no open critical bugs** are known. Recent fixes and status are tracked below for awareness.

## Recently Fixed

### Graceful shutdown with database dump (Resolved)
- **Status:** Fixed (January 2, 2026)
- **Scope:** `do_shutdown()` in `src/netmush/game.c` and SIGTERM handler in `src/netmush/bsd.c`
- **Impact:** Server now performs complete database dump before shutdown via `@shutdown` command or SIGTERM signal, ensuring all changes are saved. Previously, `@shutdown` would set shutdown flag without dumping, risking data loss.

### Flatfile loading continues server startup (Resolved)
- **Status:** Fixed (January 2, 2026)
- **Scope:** `--load-flatfile` parameter handling in `src/netmush/game.c`
- **Impact:** Server now continues running after loading flatfile instead of exiting, allowing seamless database restoration and migration workflows. Added support for database backend migration (GDBM to LMDB).

### Empty config lines cause parser error (Resolved)
- **Status:** Fixed (January 2, 2026)
- **Scope:** Configuration file parser in `src/netmush/conf_files.c`
- **Impact:** Server now correctly skips empty lines in configuration files, preventing "Config directive not found" errors that could prevent startup.

### KILL/CRASH flatfiles reused incorrectly (Resolved)
- **Status:** Fixed (January 2, 2026)
- **Scope:** Crash dump file handling in `src/netmush/game.c`
- **Impact:** KILL and CRASH flatfiles are now renamed with timestamps to prevent accidental reuse of old crash dumps as recovery sources.

### Telnet line endings missing carriage return (Resolved)
- **Status:** Fixed (January 2, 2026)
- **Scope:** Network output in `src/netmush/netcommon.c`
- **Impact:** Fixed network output to use proper `\r\n` line endings instead of bare `\n`, improving compatibility with telnet clients.

### ANSI color parsing loses characters (Resolved)
- **Status:** Fixed (December 31, 2025)
- **Scope:** ANSI color handler `%x` in `src/netmush/string_ansi.c`
- **Impact:** Fixed character loss during ANSI color code parsing, ensuring all text is preserved when using color codes.

### ANSI color sequences lack reset (Resolved)
- **Status:** Fixed (December 31, 2025)
- **Scope:** `fun_ansi()` in `src/netmush/funstring.c`
- **Impact:** ANSI color output now always terminates with proper reset sequences (`\033[0m`), preventing color bleed to subsequent output.

### ANSI truecolor support incomplete (Resolved)
- **Status:** Fixed (December 31, 2025)
- **Scope:** Color handling in `src/netmush/ansi.c` and `src/netmush/vt100.c`
- **Impact:** Complete truecolor (24-bit RGB) support now integrated with proper sequence generation and ansipos() compatibility.

### RGB color mapping incorrect (Resolved)
- **Status:** Fixed (December 21, 2025)
- **Scope:** `RGB2X11()` conversion function in `src/netmush/ansi.c`
- **Impact:** Fixed white color mapping: `(192,192,192)` now correctly maps to X11 color 7 instead of bright green, `(255,255,255)` now maps to 15 as expected.

### Xterm 256 color rendering broken (Resolved)
- **Status:** Fixed (December 21, 2025)
- **Scope:** `COLOR256` flag handling in `src/netmush/string_ansi.c`
- **Impact:** Xterm 256-color palette rendering now works correctly for both standard and truecolor modes.

### Bright ANSI color codes inverted (Resolved)
- **Status:** Fixed (December 21, 2025)
- **Scope:** Bright color handling in `src/netmush/ansi.c`
- **Impact:** `%xh` (bright/high intensity) flag now correctly activates bright variants (codes 90-97) instead of producing dark codes with bold attribute.

### Boolean expression parenthesis parsing fails (Resolved)
- **Status:** Fixed (December 25, 2025)
- **Scope:** `parse_boolexp_L()` in `src/netmush/boolexp.c`
- **Impact:** Fixed parsing bug affecting parenthesized boolean expressions in lock evaluation.

### Numeric attribute locks not evaluated internally (Resolved)
- **Status:** Fixed (December 25, 2025)
- **Scope:** Lock evaluation in `src/netmush/boolexp.c`
- **Impact:** Numeric attribute locks now correctly evaluated when check is internal, fixing permission evaluation issues.

### Attribute %= substitution parsing broken (Resolved)
- **Status:** Fixed (December 22, 2025)
- **Scope:** Parameter substitution in `src/netmush/eval.c`
- **Impact:** Fixed parsing of `%=` substitution parameters in attributes and softcode.

### @trigger/now always executes action (Resolved)
- **Status:** Fixed (December 22, 2025)
- **Scope:** `did_it()` function in `src/netmush/predicates.c`
- **Impact:** Fixed `@trigger/now` to respect conditions instead of always executing the action attribute unconditionally.

### Attributes lost on server dump (Resolved)
- **Status:** Fixed (December 22, 2025)
- **Scope:** Attribute persistence before dumps in `src/netmush/game.c`
- **Impact:** Added `al_store()` before all database dumps to persist in-memory attribute lists, preventing loss of attribute changes.

### stack-safe writes in `fun_write()` (Resolved)
- **Status:** Fixed (December 19, 2025)
- **Scope:** `src/netmush/alloc.c` safe string helpers (`__xsafestrncpy`, `__xsafestrncat`)
- **Impact:** Prevented stack buffer overflow when writing to untracked (stack) buffers, which previously could abort the server during `fun_write()` execution.

### `sandbox()` SIGSEGV (Resolved)
- **Status:** Fixed (December 19, 2025)
- **Scope:** `handle_ucall()` in `src/netmush/funvars.c`
- **Impact:** Out-of-bounds access to `fargs[5]` with too few args and null `preserve` dereference when no registers were saved.

### List operations crash (Resolved)
- **Status:** Fixed (December 2025)
- **Scope:** List operation functions (commit: "List ops crash fix + regression suite")
- **Impact:** Crash in list ops corrected; regression suite added to prevent recurrence.

## Test Coverage Notes
- Structure lifecycle regression suite: 22/22 passing.
- Variable functions regression suite: 40/40 passing. Regex capture tests are deferred due to config parser parenthesis conflicts, not due to runtime defects.
- `sandbox()` regression tests are now active in scripts/functions_execution_control.conf and pass; crash root causes were fixed in code.
