# Module Development Guide

TinyMUSH 4.0 provides a module system that allows developers to extend server functionality through dynamically loadable modules. This document covers module development, architecture, and best practices.

## Table of Contents

- [Overview](#overview)
- [Module Architecture](#module-architecture)
- [Building a Module](#building-a-module)
- [Module Structure](#module-structure)
- [Module Initialization](#module-initialization)
- [Configuration Management](#configuration-management)
- [Example Modules](#example-modules)
- [Best Practices](#best-practices)

## Overview

Modules are compiled C code that extends TinyMUSH without modifying the core server. This approach allows:

- **Upgradability**: Core updates don't affect module code
- **Modularity**: Features can be toggled on/off via configuration
- **Performance**: Hardcoded functionality vs. softcode
- **Extensibility**: Add new commands, functions, and behaviors

### Module Safety Considerations

Modules have direct access to server internals and operate without the safety guarantees of softcode. Therefore:

- **Stability**: Buggy modules can crash or corrupt the database
- **Responsibility**: Module authors must thoroughly test their code
- **Documentation**: Ensure clear documentation of module behavior and dependencies

## Module Architecture

Each module consists of:

1. **Source code** - C implementation in `src/modules/<name>/`
2. **Header files** - Interface definitions
3. **Build configuration** - `configure.in` and `Makefile.inc.in`
4. **Compiled artifact** - `.so` file loaded at startup

### Module Entry Points

Every module must define:

- **Initialization function**: `void mod_<name>_init()` - called at server startup
- **Configuration table**: `CONF mod_<name>_conftable[]` - defines config parameters
- **Version info**: `MODVER mod_<name>_version` - version metadata

## Building a Module

### 1. Directory Structure

```
src/modules/<name>/
├── configure.in
├── configure
├── Makefile.inc.in
├── .depend
└── <name>.c
```

### 2. Starting a New Module

The easiest approach is to copy an existing module and adapt it:

```bash
cp -r src/modules/mail src/modules/mymodule
cd src/modules/mymodule
# Edit configure.in, Makefile.inc.in, and .c files
```

### 3. Configuration Script

Update `configure.in` with your module name:

```autoconf
AC_INIT([src/mymodule.c])
AC_OUTPUT(Makefile)
```

Generate the `configure` script:

```bash
autoconf configure.in > configure
chmod +x configure
```

### 4. Build and Install

The CMake build system handles module compilation:

```bash
cd build
cmake ..
make
```

Compiled modules are placed in `game/modules/`.

### 5. Loading Modules

Add to `netmush.conf`:

```
module <name>
```

Modules are only loaded at server startup, not during reload.

## Module Structure

### Header Template

Every module should include:

```c
#include "config.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"
#include "../../api.h"
```

### Namespace Conventions

Avoid symbol collisions by prefixing:

- **Functions**: `mod_<name>_<function_name>()`
- **Variables**: `mod_<name>_<variable_name>`
- **Macros**: `MOD_<NAME>_<CONSTANT_NAME>`
- **Structures**: `struct mod_<name>_<structure_name>`

Example:

```c
int mod_mail_send_message(dbref player, const char *recipient);
struct mod_mail_confstorage mod_mail_config;
#define MOD_MAIL_MAX_MSG_SIZE 4096
```

## Module Initialization

### Configuration Storage

Define a configuration structure:

```c
struct mod_<name>_confstorage {
    int param1;
    char *param2;
    int param3;
} mod_<name>_config;
```

### Configuration Table

Map configuration parameters:

```c
CONF mod_<name>_conftable[] = {
    {(char *)"param1", cf_int, CA_GOD, CA_PUBLIC, 
     &mod_<name>_config.param1, 0},
    {(char *)"param2", cf_string, CA_GOD, CA_PUBLIC,
     (int *)&mod_<name>_config.param2, 256},
    {NULL, NULL, 0, 0, NULL, 0}
};
```

### Initialization Function

Called at server startup:

```c
void mod_<name>_init()
{
    /* Set default configuration values */
    mod_<name>_config.param1 = 10;
    mod_<name>_config.param2 = XSTRDUP("default", "mod_<name>_init");

    /* Initialize hash tables */
    hashinit(&mod_<name>_htab, 100);

    /* Register commands, functions, etc. */
    // ...
}
```

## Configuration Management

### Boolean Parameters

```c
CONF mod_<name>_conftable[] = {
    {(char *)"feature_enabled", cf_bool, CA_GOD, CA_PUBLIC,
     &mod_<name>_config.enabled, (long)"Enable feature"},
    {NULL, NULL, 0, 0, NULL, 0}
};
```

### Integer Parameters

```c
{(char *)"max_messages", cf_int, CA_GOD, CA_PUBLIC,
 &mod_<name>_config.max_messages, 10000}
```

### String Parameters

```c
{(char *)"database_path", cf_string, CA_GOD, CA_PUBLIC,
 (int *)&mod_<name>_config.db_path, 256}
```

### Read-Only Parameters

```c
{(char *)"version", cf_const, CA_STATIC, CA_PUBLIC,
 &mod_<name>_config.version, (long)"1.0.0"}
```

## Example Modules

### Mail Module

Location: `src/modules/mail/`

**Features**:
- In-game mail delivery system
- Mailbox management
- Mail aliases
- PennMUSH-compatible syntax

**Configuration**:
- `mail_expiration`: Days before mail is deleted
- `mail_database`: Database file location

**Key Functions**:
- `@mail` - Send and manage messages
- `@malias` - Create mail aliases

### Comsys Module

Location: `src/modules/comsys/`

**Features**:
- Communication channels
- Channel management
- Moderated channels
- History storage

**Key Commands**:
- `@channel` - Manage channels
- `+channel` or `+ch` - Communicate on channels

## Best Practices

### 1. Memory Management

Always use server allocation macros:

```c
XMALLOC()  /* Allocate tracked memory */
XSTRDUP()  /* Duplicate string */
XFREE()    /* Free tracked memory */
```

### 2. Error Handling

Check all return values and validate input:

```c
if (!notify(player, "error message"))
{
    return;
}
```

### 3. Hash Tables

Initialize and use hash tables for efficient lookups:

```c
hashinit(&mod_<name>_htab, 100);
hashfind(key, &mod_<name>_htab);
hashadd(key, data, &mod_<name>_htab, "source");
```

### 4. Database Integrity

Protect modifications to the database:

```c
db_touch(player);  /* Mark database as modified */
```

### 5. Testing

Test modules thoroughly:

- Unit tests for critical functions
- Integration tests with server
- Database corruption scenarios
- Memory leak detection

### 6. Documentation

Document your module:

- Configuration parameters
- Command syntax
- Function behavior
- Known limitations

### 7. Version Management

Define version information:

```c
MODVER mod_<name>_version = {
    "module_name",
    1,  /* major */
    0,  /* minor */
    0,  /* revision */
};
```

## Debugging Modules

### Compilation Errors

Check:
1. `api.h` is included
2. All symbols are declared or extern
3. Spelling and case sensitivity

### Runtime Errors

- Use `notify()` to log debug messages
- Check server logs for crash dumps
- Verify configuration parameters are correct

### Module Reload

To reload a module after code changes:

1. Rename old `.so` file in `game/modules/`
2. Rebuild the module
3. Restart the server

## Limitations and Constraints

- Modules cannot be reloaded without server restart
- Module symbols must not conflict with core or other modules
- Only one backup of old `.so` files is kept
- Modules have full access to database and server state

## References

- Mail module: `src/modules/mail/mail.c`, `src/modules/mail/mail.h`
- Comsys module: `src/modules/comsys/comsys.c`
- Server API: `src/netmush/api.h`

## Support and Contributions

For questions or to contribute module improvements:

1. Review existing module code
2. Follow the naming and coding conventions
3. Test thoroughly before publishing
4. Submit pull requests with documentation

---

**Note**: This guide applies to TinyMUSH 4.0 and later. Module API may change between versions.
