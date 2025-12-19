# Known Bugs in TinyMUSH

## Critical Bugs

### sandbox() function - SIGSEGV crash (Resolved)

**Status:** Fixed (December 19, 2025)  
**Affected Version:** 4.0 alpha Revision 244 (pre-fix)  
**Severity:** Critical - caused server crash

**Issue:**  
`sandbox()` (implemented via `handle_ucall()` in `src/netmush/funvars.c`) could crash due to:
- Out-of-bounds access to `fargs[5]` when only 5 arguments were present.  
- Dereferencing a null `preserve` pointer when no registers were saved.

**Fix implemented:**
- Compute `extra_args` and pass `NULL` to `eval_expression_string()` when there are no extra args, avoiding `fargs[5]` OOB access.  
- Guard register restore/free loops with `if (preserve)` to avoid null dereference when no registers exist.

**Notes:** Regression tests for `sandbox()` remain commented out pending broader runtime validation, but the SIGSEGV root causes are addressed in code.

---

## Test Suite Status

Regression tests for `sandbox()` are commented out in `scripts/functions_execution_control.conf` pending end-to-end validation.
