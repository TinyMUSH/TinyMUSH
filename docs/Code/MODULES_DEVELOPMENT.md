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

### 1. Directory Structure (CMake-only)

```
src/modules/<name>/
├── CMakeLists.txt
├── <name>.c
└── <name>.h
```

Start from the provided skeleton template:

```bash
cp -r src/modules/skeleton src/modules/mymodule
cd src/modules/mymodule
mv skeleton.c mymodule.c
mv skeleton.h mymodule.h
sed -i 's/skeleton/mymodule/g' mymodule.c mymodule.h CMakeLists.txt
```

### 2. Build and Install

```bash
cd /home/eddy/source/TinyMUSH
mkdir -p build && cd build
cmake ..
make
```

Artifacts land in `build/modules/`; `make install` or the `install-upgrade` target copies them to `game/modules/`.

### 3. Enable/Disable Modules

- Add or remove module subdirectories in `modules.cmake` (leave uncommented to build, comment out to skip).
- In `game/configs/netmush.conf`, list modules to load at runtime:

```
module mymodule
```

Modules load on server start; restart after changing the list.

## Module Structure

### Header Template

Include the standard server headers plus your own module header:

```c
#include "config.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"
#include "mymodule.h"
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

Skeleton baseline:

```c
struct mod_<name>_confstorage {
    int enabled;
} mod_<name>_config;
```

### Configuration Table

Map config directives to fields:

```c
CONF mod_<name>_conftable[] = {
    {(char *)"<name>_enabled", cf_bool, CA_GOD, CA_PUBLIC,
     &mod_<name>_config.enabled, (long)"Enable module"},
    {NULL, NULL, 0, 0, NULL, 0}
};
```

### Initialization Function

Called at server startup; register tables and init hashes:

```c
void mod_<name>_init(void)
{
    mod_<name>_config.enabled = 1;

    hashinit(&mod_<name>_data_htab, 100, HT_KEYREF | HT_STR);

    register_commands(mod_<name>_cmdtable);
    register_functions(mod_<name>_functable);

    log_write(LOG_ALWAYS, "MOD", "XXXX", "Module <NAME> initialized");
}
```

## Configuration Management

### Boolean Parameters

```c
{(char *)"mymodule_enabled", cf_bool, CA_GOD, CA_PUBLIC,
 &mod_mymodule_config.enabled, (long)"Enable module"}
```

### Integer / String / Const Patterns

```c
{(char *)"mymodule_limit", cf_int, CA_GOD, CA_PUBLIC,
 &mod_mymodule_config.limit, 1000}

{(char *)"mymodule_label", cf_string, CA_GOD, CA_PUBLIC,
 (int *)&mod_mymodule_config.label, 128}

{(char *)"mymodule_version", cf_const, CA_STATIC, CA_PUBLIC,
 &mod_mymodule_version.version, (long)"1.0.0"}
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

Modules load at server start. After changes:

1. Rebuild (`cmake --build build --target install-upgrade`)
2. Restart the server to pick up the new `.so`

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
