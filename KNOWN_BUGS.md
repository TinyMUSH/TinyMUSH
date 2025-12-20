# Known Bugs in TinyMUSH

As of December 20, 2025, **no open critical bugs** are known. Recent fixes and status are tracked below for awareness.

## Recently Fixed

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
