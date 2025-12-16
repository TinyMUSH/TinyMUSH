# TinyMUSH Memory Management Subsystem (`alloc.c`)

## Overview

`alloc.c` implements a comprehensive, tracked memory management system for TinyMUSH. It provides safety-critical wrappers around standard C memory functions with built-in bounds checking, overflow detection, and detailed memory tracking capabilities.

**Key Features:**
- Tracked memory allocation with overflow detection via magic numbers
- Protection against integer overflow in size calculations
- Buffer overflow prevention with automatic bounds checking
- Comprehensive logging and memory statistics
- Safe versions of string operations (sprintf, strcat, strcpy, etc.)
- Support for aligned memory allocation (cache-line optimization)

---

## Architecture

### Memory Tracking Structure (MEMTRACK)

Each allocation is tracked with a `MEMTRACK` structure containing:

```c
struct {
    void *bptr;              // Pointer to user's memory block
    size_t size;             // Size of allocated memory
    const char *file;        // Source file where allocation occurred
    int line;                // Line number in source file
    const char *function;    // Function name
    const char *var;         // Variable name
    struct memtrack *next;   // Linked list pointer
    uint64_t *magic;         // Magic number for overflow detection
}
```

### Memory Layout

```
[MEMTRACK Header (variable)]
[Aligned allocation marker]
[User Data (size bytes)]
[Magic Number (uint64_t)]
^bptr points here         ^magic points here
```

The magic number (`0xDEADBEEFCAFEBABE`) is placed after the user data to detect buffer overruns.

---

## Core Functions

### Memory Allocation

#### `__xmalloc(size_t size, const char *file, int line, const char *function, const char *var)`

**Purpose:** Tracked malloc replacement with zero-initialization

**Usage:**
```c
// Macro wrapper (recommended):
char *buffer = XMALLOC(1024, "buffer");

// Direct call (not recommended):
char *buffer = __xmalloc(1024, __FILE__, __LINE__, __func__, "buffer");
```

**Features:**
- Aligned allocation using C11 `aligned_alloc`, POSIX `posix_memalign`, or standard `malloc`
- Automatic zero-initialization of allocated memory
- Magic number placement for overflow detection
- Logging support via `mushconf.malloc_logger`

**Returns:** Pointer to allocated memory, or NULL on failure

---

#### `__xcalloc(size_t nmemb, size_t size, const char *file, int line, const char *function, const char *var)`

**Purpose:** Tracked calloc replacement with overflow protection

**Usage:**
```c
// Allocate array of 100 elements, 16 bytes each
int *array = XCALLOC(100, sizeof(int), "array");
```

**Features:**
- **Integer overflow protection:** Checks if `nmemb * size > SIZE_MAX` before allocation
- Returns NULL if overflow would occur (no abort/crash)
- Delegates to `__xmalloc` for actual allocation

**Critical:** Always check for integer overflow when multiplying user-controlled dimensions:
```c
// Safe:
int *matrix = XCALLOC(rows, sizeof(int*), "matrix");

// NOT safe if rows comes from user input without validation:
// Could overflow if rows * sizeof(int*) > SIZE_MAX
```

---

#### `__xrealloc(void *ptr, size_t size, const char *file, int line, const char *function, const char *var)`

**Purpose:** Tracked realloc with safe resizing

**Behavior:**
- If `ptr` is NULL: equivalent to malloc
- If `size` is 0 and `ptr` is not NULL: equivalent to free
- Otherwise: allocates new block, copies min(old_size, new_size) bytes, frees old block

**Usage:**
```c
buffer = XREALLOC(buffer, 2048, "buffer");  // Grow
buffer = XREALLOC(buffer, 512, "buffer");   // Shrink
buffer = XREALLOC(buffer, 0, "buffer");     // Free
```

---

### Memory Deallocation

#### `__xfree(void *ptr)`

**Purpose:** Tracked free with overflow detection

**Usage:**
```c
XFREE(buffer);  // Macro wrapper

// or direct call:
int overrun = __xfree(buffer);
```

**Return Value:**
- `0`: Buffer was valid and not overrun
- `1`: Buffer was valid but **OVERRUN DETECTED** (magic number corrupted)
- (Function always succeeds; overrun is logged)

**Critical:** Always check for overruns in debugging:
```c
if (XFREE(buffer)) {
    log_write(LOG_ALWAYS, "MEM", "ERROR", "Buffer overrun detected!");
}
```

---

### Memory Inspection

#### `__xfind(void *ptr)`

**Purpose:** Find MEMTRACK entry for any pointer in allocated block

**Usage:**
```c
MEMTRACK *info = __xfind(buffer);
if (info) {
    printf("Allocated at %s:%d in %s\n", 
           info->file, info->line, info->function);
}
```

**Returns:** Pointer to MEMTRACK structure, or NULL if not found

---

#### `__xcheck(void *ptr)`

**Purpose:** Verify buffer integrity without freeing

**Return Values:**
- `1`: Buffer is valid and not overrun
- `0`: Buffer is valid but **OVERRUN DETECTED**
- `2`: Buffer is not tracked (unknown allocation)

**Usage:**
```c
int status = __xcheck(buffer);
if (status == 0) {
    // ALERT: Buffer overrun!
}
```

---

## String Functions

### Dynamic String Creation

#### `__xasprintf(const char *file, int line, const char *function, const char *var, const char *format, ...)`

**Purpose:** Printf-style dynamic string allocation

**Usage:**
```c
char *name = XASPRINTF("player_name", "Hello %s!", "World");
// Returns: "Hello World!"
// Must be freed with XFREE(name)
```

**Features:**
- Calculates exact size needed (no buffer overflow possible)
- Returns NULL on formatting error
- Memory is tracked like all other X* allocations

---

#### `__xavsprintf(const char *file, int line, const char *function, const char *var, const char *format, va_list va)`

**Purpose:** va_list version of asprintf

**Usage:** In functions that receive variadic arguments:
```c
void log_message(const char *format, ...) {
    va_list ap;
    va_start(ap, format);
    char *msg = __xavsprintf(__FILE__, __LINE__, __func__, "msg", format, ap);
    va_end(ap);
    // Use msg...
    XFREE(msg);
}
```

---

### Safe String Operations

All safe string functions perform automatic bounds checking against the tracked buffer size.

#### `__xstrcpy(char *dest, const char *src)`
- Copies src to dest with buffer overflow protection
- If dest is tracked: copies at most `buffer_size - 1` bytes
- If dest is untracked: performs regular strcpy (less safe)

#### `__xstrncpy(char *dest, const char *src, size_t n)`
- Copies at most n bytes with bounds checking
- Automatically limits to buffer size if tracked

#### `__xstrcat(char *dest, const char *src)`
- Appends src to dest with overflow protection
- Calculates remaining space in buffer

#### `__xstrncat(char *dest, const char *src, size_t n)`
- Appends at most n bytes with bounds checking

#### `__xstrccat(char *dest, char c)` / `__xstrnccat(char *dest, char c, size_t n)`
- Appends single character to string
- Ensures null termination

#### `__xstrdup(const char *s, const char *file, int line, const char *function, const char *var)`
- Duplicates string with tracking
- Usage: `char *copy = XSTRDUP(original, "copy");`

#### `__xstrndup(const char *s, size_t n, ...)`
- Duplicates at most n bytes of string

---

### Printf Variants

#### `__xsprintf(char *str, const char *format, ...)`
- Printf to buffer with overflow protection
- If buffer is tracked: output limited to buffer size
- Returns bytes written

#### `__xsnprintf(char *str, size_t size, const char *format, ...)`
- Printf with explicit size limit and overflow protection

#### `__xsprintfcat(char *str, const char *format, ...)`
- Printf-append: appends formatted output to end of string

#### `__xsafesprintf(char *buff, char **bufp, const char *format, ...)`
- Printf to tracked buffer with pointer advancement
- **Usage pattern:**
```c
char *buffer = XMALLOC(LBUF_SIZE, "buffer");
char *bp = buffer;

SAFE_SPRINTF(buffer, &bp, "Line 1\n");
SAFE_SPRINTF(buffer, &bp, "Line 2\n");
SAFE_SPRINTF(buffer, &bp, "Line 3\n");

notify(player, buffer);
XFREE(buffer);
```

---

### Memory Operations

#### `__xmemmove(void *dest, const void *src, size_t n)`
- Overlapping-safe memory copy with bounds checking

#### `__xmempcpy(void *dest, const void *src, size_t n)`
- Memory copy that returns pointer to end of copied region

#### `__xmemccpy(void *dest, const void *src, int c, size_t n)`
- Copy until character c is found, with bounds checking

#### `__xmemset(void *s, int c, size_t n)`
- Fill memory with character, respecting buffer bounds

---

## Safe Buffer Operations

### `__xsafestrncpy(char *dest, char **destp, const char *src, size_t n, size_t size)`

**Purpose:** Copy with pointer tracking and overflow prevention

**Parameters:**
- `dest`: Destination buffer
- `destp`: Pointer-to-pointer tracking current position
- `src`: Source string
- `n`: Maximum bytes to copy
- `size`: Buffer size (if untracked)

**Returns:** Number of bytes from src that didn't fit

**Typical Usage:**
```c
char *buffer = XMALLOC(LBUF_SIZE, "buffer");
char *bp = buffer;

__xsafestrncpy(buffer, &bp, "Hello ", 6, LBUF_SIZE);
__xsafestrncpy(buffer, &bp, "World!", 6, LBUF_SIZE);
// buffer now contains: "Hello World!"
// bp points to end position
XFREE(buffer);
```

---

### `__xsafestrncat(char *dest, char **destp, const char *src, size_t n, size_t size)`

**Purpose:** Append with pointer tracking and bounds checking

**Returns:** Number of bytes from src that didn't fit

---

### `__xsafestrcatchr(char *dest, char **destp, char c, size_t size)`

**Purpose:** Append single character with pointer tracking

**Parameters:**
- `dest`: Destination buffer
- `destp`: Pointer-to-pointer tracking position
- `c`: Character to append
- `size`: Buffer size (if untracked)

**Returns:** 0 on success, 1 if buffer full

---

## Statistics and Debugging

### `list_rawmemory(dbref player)`

**Purpose:** Display memory allocation statistics to player

**Output Format:**
```
File              Line Function            Variable             Allocs    Bytes
--------------- ------ ------------------- -------------------- ------ --------
predicates.c     2156   do_prog             program              1       512
commands.c       897    do_command          cmd_line             4       2.5K
...
Total: 234 raw allocations in 45 unique tags, 512.3K bytes used.
```

**Usage:**
```
@list raw_memory      (In-game command)
```

**Caution:** Can cause lag on large muds with many allocations.

---

### `total_rawmemory(void)`

**Purpose:** Calculate total bytes currently allocated

**Returns:** Total memory in bytes across all tracked allocations

---

## Macro Wrappers

### Standard Allocation Macros

All X* functions have macro wrappers that automatically provide `__FILE__`, `__LINE__`, and `__func__`:

```c
XMALLOC(size, "variable_name")
XCALLOC(nmemb, size, "variable_name")
XREALLOC(ptr, size, "variable_name")
XFREE(ptr)

XSTRDUP(str, "variable_name")
XSTRNDUP(str, n, "variable_name")

XASPRINTF("variable_name", "format", ...)
XAVSPRINTF("variable_name", "format", va_list)

XSPRINTF(str, "format", ...)
XSNPRINTF(str, size, "format", ...)
XSPRINTFCAT(str, "format", ...)
```

### Safe Buffer Macros

Used for safe pointer-advancing buffer operations:

```c
SAFE_STRNCPY(dest, destp, src, n, size)
SAFE_STRNCAT(dest, destp, src, n, size)
SAFE_SPRINTF(dest, destp, "format", ...)
```

---

## Common Patterns

### Pattern 1: Dynamic String Building

```c
char *buffer = XMALLOC(LBUF_SIZE, "buffer");
char *bp = buffer;

SAFE_SPRINTF(buffer, &bp, "Name: %s\n", player_name);
SAFE_SPRINTF(buffer, &bp, "Level: %d\n", player_level);
SAFE_SPRINTF(buffer, &bp, "Status: %s\n", player_status);

notify(player, buffer);
XFREE(buffer);
```

### Pattern 2: Array Allocation with Overflow Check

```c
if (count > SIZE_MAX / sizeof(OBJECT*)) {
    notify(player, "Count too large!");
    return;
}

OBJECT **array = XCALLOC(count, sizeof(OBJECT*), "object_array");
if (!array) {
    notify(player, "Memory allocation failed!");
    return;
}

// Use array...

XFREE(array);
```

### Pattern 3: Safe String Duplication

```c
char *original = "Important Data";
char *copy = XSTRDUP(original, "copy");

if (!copy) {
    notify(player, "Memory allocation failed!");
    return;
}

// Modify copy without affecting original...

XFREE(copy);
```

### Pattern 4: Buffer Overflow Detection

```c
// During debug/testing
int status = __xcheck(buffer);
if (status == 0) {
    // CRITICAL: Buffer was overrun!
    log_write(LOG_ALWAYS, "BUG", "OVERRUN", 
              "Buffer overflow detected at %p", buffer);
}
```

---

## Configuration

### Memory Logging

Enable memory allocation logging via configuration:

```c
mushconf.malloc_logger = 1;  // Enable logging
mushconf.malloc_logger = 0;  // Disable logging
```

When enabled, allocations/deallocations are logged to:
```
LOG_MALLOC facility with format:
Alloc: file[line]function:variable Alloc N bytes
Free:  file[line]function:variable Free N bytes
```

---

## Performance Considerations

### Overhead

- **Per-allocation overhead:** `sizeof(MEMTRACK) + 8 bytes` (magic number)
  - Typically 64-80 bytes on 64-bit systems
- **Lookup time:** O(n) where n = number of active allocations
  - For buffer bounds checking on every sprintf/strcpy call
  - Cache-friendly due to linked list traversal

### Optimization Tips

1. **Minimize realloc() calls:** Pre-allocate with expected size
2. **Batch operations:** Combine multiple sprintf calls before notify
3. **Reuse buffers:** When possible, reuse allocated buffers instead of freeing/reallocating
4. **Profile:** Use `@list raw_memory` to identify hot allocation sites

---

## Error Handling

### Allocation Failures

All X* functions return NULL on failure. Always check:

```c
char *buffer = XMALLOC(size, "buffer");
if (!buffer) {
    log_write(LOG_ALWAYS, "MEM", "ERROR", 
              "Failed to allocate %zu bytes", size);
    return; // or error handling
}
```

### Buffer Overruns

Detected at free time via magic number. Action:

```c
if (__xfree(buffer)) {
    // Buffer was overrun - investigate caller
    log_write(LOG_ALWAYS, "MEM", "OVERRUN",
              "Stack trace: %s", stack_trace());
}
```

### Integer Overflow

Only in `__xcalloc`. Always handle:

```c
void *result = XCALLOC(user_rows, user_cols, "matrix");
if (!result) {
    // Could be overflow or true OOM
    log_write(LOG_ALWAYS, "MEM", "ERROR",
              "XCALLOC(%zu, %zu) failed - possible overflow",
              user_rows, user_cols);
    return;
}
```

---

## Security Implications

### Buffer Overflow Protection

The magic number system detects overwrites **after** they occur, not during. Never rely on this for security:

```c
// BAD - exploitable:
char *buffer = XMALLOC(10, "buffer");
strcpy(buffer, user_input);  // User input > 10 bytes?

// GOOD - safe:
char *buffer = XMALLOC(MAX_USER_INPUT, "buffer");
strncpy(buffer, user_input, MAX_USER_INPUT - 1);
buffer[MAX_USER_INPUT - 1] = '\0';
```

### Integer Overflow Protection

Only `XCALLOC` checks for overflow. Manual calculations must be validated:

```c
// BAD:
size_t total = rows * cols;  // Could overflow
char *matrix = XMALLOC(total, "matrix");

// GOOD:
if (rows > SIZE_MAX / cols) {
    notify(player, "Size too large!");
    return;
}
char *matrix = XCALLOC(rows, cols, "matrix");
```

---

## Debugging Tips

### Find Allocation Source

```c
MEMTRACK *info = __xfind(pointer);
if (info) {
    printf("Allocated: %s:%d %s() var=%s size=%zu\n",
           info->file, info->line, info->function, 
           info->var, info->size);
}
```

### Trace All Allocations

Enable malloc_logger and grep logs:
```bash
grep "MEM.*TRACE" game.log | grep "allocation_site"
```

### Memory Leak Detection

```c
list_rawmemory(player);
// Check if size grows unexpectedly over time
// Compare @list raw_memory output periodically
```

---

## API Reference Summary

| Function | Purpose | Safe? |
|----------|---------|-------|
| `XMALLOC` | Allocate & zero | Yes |
| `XCALLOC` | Array allocate with overflow check | Yes |
| `XREALLOC` | Resize allocation | Yes |
| `XFREE` | Free with overrun detection | Yes |
| `XSTRDUP` | Duplicate string | Yes |
| `XASPRINTF` | Formatted string allocation | Yes |
| `XSPRINTF` | Printf to buffer (bounded) | Yes |
| `XSTRCAT` | Append string (bounded) | Yes |
| `SAFE_SPRINTF` | Printf with pointer tracking | Yes |
| `SAFE_STRNCPY` | Copy with pointer tracking | Yes |

---

## Version Information

- **File:** `netmush/alloc.c`
- **Version:** Part of TinyMUSH 4.x
- **License:** Artistic License (see COPYING)
- **Compiled:** With C11 support when available

