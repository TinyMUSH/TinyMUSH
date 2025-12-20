# Skeleton Module - Quick Reference Card

## Installation (manual, 2 minutes)

```bash
cp -r src/modules/skeleton src/modules/mymodule
cd src/modules/mymodule
mv skeleton.h mymodule.h
mv skeleton.c mymodule.c
sed -i 's/skeleton/mymodule/g' mymodule.h mymodule.c CMakeLists.txt
```

## Essential Files

| File | Purpose |
|------|---------|
| `mymodule.h` | Declarations, config struct, prototypes |
| `mymodule.c` | Implementation, init function, commands |
| `CMakeLists.txt` | CMake build configuration |
| `README.md` | Quick start / full guide |

## Module Lifecycle (mod_mymodule_)

```c
// REQUIRED: Module initialization
void mod_mymodule_init(void)
{
    mod_mymodule_config.enabled = 1;

    hashinit(&mod_mymodule_htab, 100, HT_KEYREF | HT_STR);

    register_commands(mod_mymodule_cmdtable);
    register_functions(mod_mymodule_functable);

    log_write(LOG_ALWAYS, "MOD", "MYMOD", "Module initialized");
}

// OPTIONAL: Module cleanup on unload
void mod_mymodule_cleanup(void)
{
    // Free hash entries if you allocated data
}
```

## Configuration (netmush.conf)

```
# Module must be declared
module mymodule

# Configuration parameters
mymodule_enabled yes
```

## CONF Table Pattern

```c
CONF mod_mymodule_conftable[] = {
    {(char *)"mymodule_enabled", cf_bool, CA_GOD, CA_PUBLIC,
     &mod_mymodule_config.enabled, (long)"Enable module"},
    {NULL, NULL, 0, 0, NULL, 0}
};
```

### Config Types
- `cf_bool` - Boolean (yes/no)
- `cf_int` - Integer value
- `cf_string` - Text string
- `cf_const` - Read-only constant

### Permissions
- `CA_GOD` - Gods only
- `CA_ADMIN` - Admins
- `CA_PUBLIC` - Everyone

## Naming Conventions

```
Global variable:  mod_mymodule_<name>
Function:         mod_mymodule_<action>()
Hash table:       mod_mymodule_<type>_htab
Config:           mod_mymodule_config
Version:          mod_mymodule_version
```

## Hash Table Operations

```c
// Initialize in mod_mymodule_init()
hashinit(&mod_mymodule_htab, 100);

// Add entry
hashadd("key", data_ptr, &mod_mymodule_htab, "function");

// Find entry
void *data = hashfind("key", &mod_mymodule_htab);

// Delete entry
hashdelete("key", &mod_mymodule_htab);

// Iterate all
for (hp = hash_firstentry(&mod_mymodule_htab); hp;
     hp = hash_nextentry(&mod_mymodule_htab)) {
    // hp->key, hp->data
}

// Helper functions (already in skeleton)
int *mod_mymodule_get_data(char *key);
CF_Result mod_mymodule_set_data(char *key, int *data);
bool mod_mymodule_delete_data(char *key);
void mod_mymodule_list_data(dbref player);
```
```

## String Safety

```c
// SAFE: Copy with length check
XSAFESTRNCPY(dest, src, dest, sizeof(dest), "func");
xstrncpy(dest, src, sizeof(dest));

// SAFE: Concatenate
XSAFESTRCAT(dest, src, dest, sizeof(dest), "func");

// SAFE: formatted logging via log_write
log_write(LOG_ALWAYS, "MOD", "MYMOD", "Format: %s", arg);

// UNSAFE (never use):
// strcpy(dest, src);          // DANGER!
// strcat(dest, src);          // DANGER!
// sprintf(dest, "%s", src);   // DANGER!
```

## Memory Management

```c
// Allocate with tracking
void *ptr = XMALLOC(size, "function");
char *str = XSTRDUP("string", "function");

// Free with tracking (always match allocation)
XFREE(ptr, "function");
XFREE(str, "function");
```

## Command Implementation Pattern

```c
void mod_mymodule_command(dbref player, dbref cause, int key, char *arg)
{
    // 1. Check if enabled
    if (!mod_mymodule_config.enabled)
    {
        notify(player, "Module disabled");
        return;
    }
    
    // 2. Validate arguments
    if (!arg || !*arg)
    {
        notify(player, "Usage: @command <args>");
        return;
    }
    
    // 3. Check permissions
    if (!Admin(player))
    {
        notify(player, "Permission denied");
        return;
    }
    
    // 4. Do work
    notify(player, "Done");
    
    // 5. Log
    log_write(LOG_ALWAYS, "MOD", "MYMOD", "Command executed by %s", Name(player));
}
```

## Function Implementation Pattern

```c
void mod_mymodule_function(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
    // Check if enabled
    if (!mod_mymodule_config.enabled) {
        XSAFELBSTR("#-1 MODULE DISABLED", buff, bufc);
        return;
    }
    
    // Validate args
    if (nfargs < 1) {
        XSAFELBSTR("#-1 MISSING ARG", buff, bufc);
        return;
    }
    
    // Do work
    XSAFESPRINTF(buff, bufc, "nfargs=%d ncargs=%d", nfargs, ncargs);
    log_write(LOG_ALWAYS, "MOD", "MYMOD", "Function called");
}
```

## Build & Deploy

```bash
# Build
cd /home/eddy/source/TinyMUSH
rm -rf build && mkdir build && cd build
cmake .. && make

# Result: build/modules/mymodule.so

# Enable in config
echo "module mymodule" >> game/configs/netmush.conf

# Restart
cd game && ./netmush -D
```

## Debug Tips

```bash
# Enable debug output
# In netmush.conf: mymodule_debug 2

# Watch logs
tail -f game/logs/netmush.log

# GDB debugging
gdb --args ./build/src/netmush/netmush -D
(gdb) break mod_mymodule_init
(gdb) run

# In-game testing
@restart  # To reload module
@version  # Check version
```

## Verify Installation

```bash
# Check module loaded
ls build/modules/mymodule.so

# Check in game (if commands registered)
@mycommand

# Check logs
grep "initialized" game/logs/netmush.log | tail

# Check config
@config mymodule_*
```

## Common Mistakes

| Mistake | Fix |
|---------|-----|
| Forgot `mod_mymodule_init()` | REQUIRED - must exist and be named correctly |
| CONF table missing NULL | ALWAYS end with `{NULL, NULL, 0, 0, NULL, 0}` |
| Using `strcpy()` | Use `XSAFESTRNCPY()` or `xstrncpy()` |
| Module not in config | Add `module mymodule` to netmush.conf |
| Hash table not initialized | Call `hashinit()` in `mod_mymodule_init()` |
| Memory leak | Always `XFREE()` what you `XMALLOC()` |
| Using `log_printf()` directly | Use `log_write()` with module tag |
| Uninitialized hash table | Call `hashinit()` in init |

## File Size Reference

| Component | Lines |
|-----------|-------|
| Minimal module (header) | 50 |
| Minimal module (source) | 100 |
| Basic module (with config) | 200 |
| Full module (+ functions) | 400+ |
| Skeleton template | ~150 |

## Next Steps

1. **Customize** - Edit `mymodule.h` and `mymodule.c`
2. **Test** - Compile and run
3. **Document** - Add help text and examples
4. **Optimize** - Profile and improve performance
5. **Publish** - Share with TinyMUSH community

## Resources

- 📖 Full docs: `docs/MODULES_DEVELOPMENT.md`
- 📖 Adaptation: `ADAPTATION_GUIDE.md`
- 📖 Complete reference: `COMPLETE_DOCUMENTATION.md`
- 💻 Examples: Check `src/modules/mail/`, `src/modules/comsys/`
- 🏗️ API: `src/netmush/*.h` header files

---

**Pro Tip:** Read `ADAPTATION_GUIDE.md` first for step-by-step instructions.
