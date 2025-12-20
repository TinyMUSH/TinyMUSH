# Skeleton Module - Complete Template for TinyMUSH Modules

This skeleton is a working TinyMUSH module that shows the minimal but complete pattern: version metadata, configuration table, command table, function table, hash table usage, init/cleanup, and simple handlers.

## Quick Start (5 minutes)

```bash
# Copy skeleton as your new module
cp -r src/modules/skeleton src/modules/mymodule
cd src/modules/mymodule

# Rename files
mv skeleton.h mymodule.h
mv skeleton.c mymodule.c

# Replace all occurrences
sed -i 's/skeleton/mymodule/g' mymodule.h mymodule.c CMakeLists.txt

# Build
cd /home/eddy/source/TinyMUSH
rm -rf build && mkdir build && cd build
cmake .. && make

# Configure (add to game/configs/netmush.conf)
module mymodule
mymodule_enabled yes

# Restart
cd ../game && ./netmush -D
```

## File Structure

| File | Purpose | Lines |
|------|---------|-------|
| `skeleton.h` | Declarations, config struct, prototypes | ~45 |
| `skeleton.c` | Implementation with comments | ~200 |
| `CMakeLists.txt` | Minimal CMake build (4 lines) | 4 |
| `README.md` | This guide | ~160 |
| `QUICK_REFERENCE.md` | Cheat sheet | ~150 |

## Core Concepts

### 1. Module Initialization (REQUIRED)

```c
void mod_skeleton_init(void)
{
    /* Defaults */
    mod_skeleton_config.enabled = 1;

    /* Hash table */
    hashinit(&mod_skeleton_data_htab, 100, HT_KEYREF | HT_STR);

    /* Register tables */
    register_commands(mod_skeleton_cmdtable);
    register_functions(mod_skeleton_functable);

    log_write(LOG_ALWAYS, "MOD", "SKEL", "Module SKELETON initialized");
}
```

### 2. Configuration (netmush.conf)

```conf
module skeleton
skeleton_enabled yes
```

CONF table (in skeleton.c):

```c
CONF mod_skeleton_conftable[] = {
    {(char *)"skeleton_enabled", cf_bool, CA_GOD, CA_PUBLIC,
     &mod_skeleton_config.enabled, (long)"Enable skeleton module"},
    {NULL, NULL, 0, 0, NULL, 0}
};
```

Types used: `cf_bool`; Perms: `CA_GOD`, `CA_PUBLIC`.

### 3. Commands

Executed by players, can notify feedback:

```c
void mod_do_skeleton_command(dbref player, dbref cause, int key, char *arg1)
{
    if (!mod_skeleton_config.enabled)
    {
        notify(player, "The skeleton module is not enabled.");
        return;
    }
    if (!arg1 || !*arg1)
    {
        notify(player, "Usage: skeleton <message>");
        return;
    }

    notify_check(player, player, MSG_PUP_ALWAYS | MSG_ME, "Skeleton says: %s", arg1);
    log_write(LOG_ALWAYS, "MOD", "SKEL", "Command received from %s: %s", Name(player), arg1);
}
```

### 4. Functions

Called from softcode, return values via result buffer:

```c
void mod_do_skeleton_function(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    if (!mod_skeleton_config.enabled)
    {
        XSAFELBSTR("#-1 SKELETON DISABLED", buff, bufc);
        return;
    }

    XSAFESPRINTF(buff, bufc, "dbref: #%d, caller #%d, nfargs: %d, ncargs: %d", player, caller, nfargs, ncargs);

    if (nfargs > 0)
    {
        XSAFELBSTR(", fargs:", buff, bufc);
        for (int i = 1; i < nfargs; i++)
        {
            XSAFELBCHR(' ', buff, bufc);
            XSAFELBSTR(fargs[i], buff, bufc);
        }
    }

    if (ncargs > 0)
    {
        XSAFELBSTR(", cargs:", buff, bufc);
        for (int i = 1; i < ncargs; i++)
        {
            XSAFELBCHR(' ', buff, bufc);
            XSAFELBSTR(cargs[i], buff, bufc);
        }
    }

    log_write(LOG_ALWAYS, "MOD", "SKEL", "dbref: #%d, caller #%d, nfargs: %d, ncargs: %d", player, caller, nfargs, ncargs);
}
```

### 5. Hash Tables

Store and retrieve data efficiently (integer data for the example):

```c
// Initialize (in mod_skeleton_init)
hashinit(&mod_skeleton_data_htab, 100);

// Use helper functions (all provided in skeleton)
mod_skeleton_set_data("key", data_ptr);
int *data = mod_skeleton_get_data("key");
mod_skeleton_delete_data("key");
mod_skeleton_list_data(player);  // Show all entries

// Or use API directly
hashadd("key", data, &mod_skeleton_data_htab, "func");
void *x = hashfind("key", &mod_skeleton_data_htab);
hashdelete("key", &mod_skeleton_data_htab);
```

## Helper Functions Provided

| Function | Purpose |
|----------|---------|
| `mod_skeleton_get_data(key)` | Get from hash table (int*) |
| `mod_skeleton_set_data(key, int *)` | Store in hash table |
| `mod_skeleton_delete_data(key)` | Delete from hash table |
| `mod_skeleton_list_data(player)` | Count/print entries |
| `mod_skeleton_reset_defaults()` | Reset config to defaults |
| `mod_skeleton_cleanup()` | Clean up on module unload |

## Safe String Operations

Always use safe functions - never use raw `strcpy`/`strcat`/`sprintf`:

```c
// SAFE - All of these are safe
XSAFESTRNCPY(dest, src, dest, sizeof(dest), "func");
xstrncpy(dest, src, sizeof(dest));
XSAFESTRCAT(dest, src, dest, sizeof(dest), "func");
/* Use XSAFE helpers; log_write handles formatting safely */

// UNSAFE - NEVER USE
strcpy(dest, src);              // BUFFER OVERFLOW!
strcat(dest, src);              // BUFFER OVERFLOW!
sprintf(dest, "%s", src);       // BUFFER OVERFLOW!
```

## Memory Management

Track all allocations and deallocate in cleanup:

```c
// Allocate
void *ptr = XMALLOC(size, "function");
char *str = XSTRDUP("text", "function");

// Must deallocate (same tag)
XFREE(ptr, "function");
XFREE(str, "function");
```

The `mod_skeleton_cleanup()` function shows the pattern for proper cleanup.

## Customization Steps

1. **Rename module** - Change `skeleton` → `mymodule` everywhere
2. **Update version** - Edit `MODVER` in mymodule.c
3. **Update config struct** - Add fields to struct in mymodule.h
4. **Update CONF table** - Map config directives in mymodule.c
5. **Implement logic** - Replace command/function bodies
6. **Register handlers** - Call `register_command()` and `register_function()` in init
7. **Build** - `cmake .. && make`
8. **Test** - Check logs and try in-game

## Testing

```bash
# In-game (after loading)
@version               # Check it loads
@skeleton test         # Try command
think skeleton(1,2,3)  # Try function

# Check logs
tail -f game/logs/netmush.log | grep skeleton

# Enable debug output
netmush.conf: skeleton_debug 2
```

## Common Issues

| Problem | Solution |
|---------|----------|
| Module not loading | Check `module mymodule` in netmush.conf |
| Command not found | Verify `register_command()` in init |
| Crash on hash lookup | Ensure `hashinit()` called in init |
| Config not applying | Check CONF table spelling and type |
| Memory leaks | Match `XMALLOC` with `XFREE` |
| Buffer overflow | Never use `strcpy`, use `xstrncpy` |

## Development Checklist

- [ ] Module renamed in all files
- [ ] MODVER updated
- [ ] Config struct populated
- [ ] CONF table entries added
- [ ] Commands/functions implemented
- [ ] Registrations in init
- [ ] Builds cleanly (`cmake .. && make`)
- [ ] Loads without errors (check logs)
- [ ] Commands/functions work in-game
- [ ] Cleanup function updated

## Reference

- **Quick patterns:** See `QUICK_REFERENCE.md`
- **Full API docs:** `docs/MODULES_DEVELOPMENT.md`
- **Example modules:** `src/modules/mail/`, `src/modules/comsys/`
- **Header files:** `src/netmush/*.h`

**⭐ Start with QUICK_REFERENCE.md for common code patterns!**
