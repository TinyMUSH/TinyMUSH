# GitHub Copilot Instructions for TinyMUSH

## Project Overview

TinyMUSH is a multi-user dungeon (MUD) game engine written in C, focusing on modularity, maintainability, and modern C practices. This codebase emphasizes clean architecture, comprehensive documentation, and consistent naming conventions.

## Core Principles

### 1. Code Organization
- **Modular Architecture**: Break large files into focused modules by functional domain
- **Single Responsibility**: Each module should handle one specific aspect of functionality
- **Header Organization**: Use `#pragma once` instead of traditional include guards
- **No Redundancy**: Avoid duplicate declarations across headers (consolidate in 'constants.h', 'externs.h', 'macros.h', 'prototypes.h' or 'typedefs.h')

### 2. Naming Conventions

#### Functions
**Pattern**: `{module}_{action}_{target}` for public functions, `_{module}_{action}_{target}` for internal/static functions

**Examples**:
- `bsd_conn_new()` - BSD connection module, create new connection
- `bsd_dns_message_create_request()` - BSD DNS module, create message request
- `boolexp_parse()` - Boolean expression parsing (public)
- `_boolexp_check_attr()` - Boolean expression attribute check (internal)

**Key Rules**:
- Use snake_case, never camelCase for function names
- Public functions: module prefix without underscore
- Internal/static functions: module prefix with leading underscore
- Descriptive action verbs: `create`, `init`, `process`, `check`, `parse`, `eval`, etc.

#### Variables
- **Global variables**: Declare once in implementation file, extern in `externs.h`
- **Local variables**: Descriptive snake_case names
- **Constants**: UPPER_SNAKE_CASE for macros and defines

#### Files
- **Module files**: `{module}_{component}.c` (e.g., `bsd_connection.c`, `boolexp_parse.c`)
- **Headers**: Corresponding `.h` files, use `#pragma once`

### 3. Documentation Standards

#### Function Documentation
Every function must have complete Doxygen documentation:

```c
/**
 * @brief One-line summary of what the function does
 *
 * Detailed explanation of the function's purpose, behavior, and implementation notes.
 * Include important details about thread safety, memory management, or side effects.
 *
 * @param param_name Description of parameter, including valid ranges or special values
 * @param another_param Description continues here
 * @return Description of return value, including error conditions
 *
 * @note Important implementation details or caveats
 * @note Thread-safe: Yes/No with explanation
 * @attention Critical warnings about usage or side effects
 *
 * @see related_function() for related functionality
 * @todo Future improvements or known limitations
 */
```

#### File Headers
Every file must include:
- Brief description of the file's purpose
- Author information
- Version number
- Copyright notice
- Detailed explanation of the module's role

### 4. Module Structure

#### BSD Modules Pattern (Reference Implementation)
The BSD networking modules follow this structure:
- `bsd_main.c` - Main event loop and coordination
- `bsd_connection.c` - Connection lifecycle management
- `bsd_socket.c` - Low-level socket operations
- `bsd_io.c` - Input/output processing
- `bsd_dns.c` - DNS resolution (threaded)
- `bsd_signals.c` - Signal handling

Each module:
- Has clear, focused responsibility
- Uses consistent naming: `bsd_{module}_{action}_{target}`
- Provides public API with minimal surface area
- Keeps internal functions static with underscore prefix

#### Boolean Expression Modules Pattern
Another good example of modularization:
- `boolexp.c` - Main module that includes all components
- `boolexp_parse.c` - Parsing logic
- `boolexp_eval.c` - Evaluation logic
- `boolexp_memory.c` - Memory management
- `boolexp_internal.c` - Shared internal helpers

### 5. Code Style

#### General
- **Indentation**: Tabs for indentation
- **Braces**: Opening brace on same line for functions and control structures
- **Line Length**: Aim for 120 characters max, but prioritize readability

#### Comments
Use appropriate comment styles based on context:

**Doxygen Documentation Comments** (for functions, structs, enums):
```c
/**
 * @brief One-line summary
 *
 * Detailed description of purpose and behavior.
 *
 * @param param_name Description
 * @return Description of return value
 */
```

**Multi-line Explanatory Comments** (for complex logic):
```c
/* BOOLEXP_INDIR (i.e. @) is a unary operation which is replaced at
 * evaluation time by the lock of the object whose number is the 
 * argument of the operation. */
```

**Single-line Comments** (for clarifications):
```c
/* We can see control locks... else we'd break zones */
checkit = true;
```

**Inline Comments** (for brief explanations):
```c
lock_originator = NOTHING;  /* Reset after evaluation */
```

**Key Rules**:
- Use `/* */` for all code comments (single or multi-line)
- Use `/** */` only for Doxygen documentation blocks
- Never use `//` style comments (not consistent with project style)
- Place comments above the code they describe, not at the end of lines (except very brief inline comments)
- Keep comments concise but descriptive

#### Error Handling
- Check return values from all system calls
- Use `log_perror()` for system errors
- Use `log_write()` for application logging
- Never silently ignore errors

#### Memory Management
- Use XMALLOC/XCALLOC/XFREE macros (not raw malloc/free)
- Document ownership and lifecycle of allocated memory
- Free resources in reverse order of allocation
- Check for NULL before dereferencing

### 6. Thread Safety

When working with threaded code (e.g., DNS resolver):
- Document thread safety explicitly in function documentation
- Use proper synchronization primitives
- Avoid global mutable state when possible
- Use message queues for inter-thread communication
- Prefer blocking operations over busy-waiting

### 7. Building and Running

#### Building the Server
Use the `rebuild.sh` script in the project root to build the server:
```bash
./rebuild.sh
```

This script:
- Configures CMake with appropriate settings
- Compiles all modules
- Automatically starts the server after successful build (when testing)

#### Running the Server
The server **must always be started from the `game/` directory**:
```bash
cd game
./netmush          # Normal operation - forks to background
./netmush -d       # Debug mode - stays in foreground
./netmush --debug  # Same as -d
```

**Important Runtime Behavior**:
- **Normal mode**: Server automatically forks to background after successful startup
- **Debug mode** (`-d` or `--debug`): Server stays in foreground (useful for gdb debugging)
- **Working directory**: Server expects to be run from `game/` directory for correct config/db paths
- **Automatic startup**: When using `rebuild.sh`, server starts automatically after build

**Do NOT**:
- Run `./netmush` from any directory other than `game/`
- Add parameters unless you specifically want debug mode
- Expect it to work correctly from build/ or src/ directories

### 8. Git Commit Messages

Follow these conventions:
- **Format**: `Type: Brief description (50 chars)`
- **Types**: `Refactor`, `Fix`, `Feature`, `Clean`, `Docs`, `Test`
- **Body**: Explain WHY and WHAT, not HOW (code shows how)
- **Examples**:
  - `Refactor: Modularize bsd.c into 7 focused modules by functional domain`
  - `Clean: Remove redundant declarations from bsd_internal.h`
  - `Fix: Correct memory leak in bsd_conn_shutdown`

### 9. Testing and Validation

- Compile after every significant change using `rebuild.sh`
- Test basic functionality (server startup, connections)
- Check for memory leaks with valgrind when possible
- Verify no regression in existing features
- Use debug mode (`-d`) when running under gdb or when detailed logging is needed

## Project-Specific Guidance

### Working with Descriptors (DESC)
- Descriptors represent client connections
- Always check for NULL before use
- Maintain descriptor_list consistency
- Use bsd_conn_shutdown() for proper cleanup

### Database Operations
- Use dbref types for object references
- Check validity with Good_obj() or similar macros
- Lock database during critical sections
- Use transaction patterns for multi-step operations

### Attribute System
- Attributes are key-value pairs on objects
- Check permissions with See_attr()
- Use atr_pget() for safe attribute retrieval
- Free attribute strings with XFREE()

### Command Processing
- Commands follow strict parsing conventions
- Check permissions before execution
- Log security-relevant commands
- Provide clear error messages to users

## Anti-Patterns to Avoid

❌ **Don't**:
- Create monolithic files >1000 lines (split into modules)
- Use camelCase for C function names
- Duplicate extern declarations across multiple files
- Leave TODO comments without @todo annotation
- Use raw malloc/free instead of XMALLOC/XFREE
- Ignore return values from system calls
- Mix tabs and spaces for indentation
- Create circular dependencies between modules

✅ **Do**:
- Break large functions into smaller, focused helpers
- Use descriptive variable names
- Document all non-obvious behavior
- Keep functions under 100 lines when reasonable
- Validate inputs at API boundaries
- Use const for read-only pointers
- Initialize all variables
- Clean up resources in error paths

## When Making Changes

1. **Understand the context**: Read related code and comments
2. **Follow patterns**: Match existing style and conventions
3. **Document thoroughly**: Explain WHY, not just WHAT
4. **Test compilation**: Ensure code builds without warnings
5. **Check side effects**: Consider impact on other modules
6. **Update prototypes**: Keep prototypes.h synchronized
7. **Commit logically**: Group related changes together

## Resources

- **Project Structure**: See `src/netmush/` for main implementation
- **Build System**: CMake-based, see `CMakeLists.txt`
- **Documentation**: Doxygen-compatible comments throughout
- **Reference Modules**: `bsd_*.c` and `boolexp*.c` show best practices

## Questions?

When uncertain about implementation approach:
1. Look for similar existing functionality
2. Follow the module pattern (bsd_* or boolexp* as examples)
3. Prioritize clarity and maintainability over cleverness
4. Document assumptions and limitations
5. Ask for review on architectural decisions

---

**Remember**: Code is read far more often than it's written. Prioritize clarity, consistency, and maintainability.
