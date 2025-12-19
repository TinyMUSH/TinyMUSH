# Known Bugs in TinyMUSH

## Critical Bugs

### sandbox() function - SIGSEGV crash (Critical)

**Status:** Unresolved  
**Affected Version:** 4.0 alpha Revision 244  
**Severity:** Critical - causes server crash

**Description:**  
The `sandbox()` function (an advanced version of `ucall()` with execution limits and register control) causes a segmentation fault (SIGSEGV) when called.

**Root Cause:**  
In `src/netmush/funvars.c`, function `handle_ucall()` line ~1141:
```c
eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_EVAL, &str, 
                      (Is_Func(UCALL_SANDBOX)) ? &(fargs[5]) : &(fargs[3]), 
                      nfargs - ((Is_Func(UCALL_SANDBOX)) ? 5 : 3));
```

When `sandbox()` is called with exactly 5 arguments (the minimum), `fargs` contains indices 0-4. The code attempts to access `&(fargs[5])` which is out of bounds, causing undefined behavior and typically a crash.

**Reproduction:**
```
&TEST me=[add(1,2)]
think sandbox(me,,@_,@_,TEST)
# Server crashes with SIGSEGV
```

**Workaround:**  
Do not use `sandbox()`. Use `localize()`, `private()`, or `nofx()` for register/effect control, and regular `u()` for function calls.

**Suggested Fix:**  
Check if extra arguments exist before passing pointer:
```c
int extra_args = nfargs - ((Is_Func(UCALL_SANDBOX)) ? 5 : 3);
eval_expression_string(buff, bufc, obj, player, cause, EV_FCHECK | EV_EVAL, &str, 
                      (extra_args > 0) ? ((Is_Func(UCALL_SANDBOX)) ? &(fargs[5]) : &(fargs[3])) : NULL, 
                      extra_args);
```

However, this requires declaring `extra_args` variable at the top of the function (C89 compatibility).

---

## Test Suite Status

Regression tests for `sandbox()` are commented out in `scripts/functions_execution_control.conf` due to this bug.
