# TinyMUSH Commands and Registries Reference

This reference consolidates how TinyMUSH registers and resolves commands, functions, flags, powers, attributes, and related switch/config tables. It is intended for contributors navigating or extending the server.

## At-a-Glance Index

- Commands: builtin `command_table[]` and dynamic `@addcommand`
- Functions: builtin `FUN flist[]` and user-defined functions
- Flags: `FLAGENT gen_flags[]`
- Powers: `POWERENT gen_powers[]`
- Attributes: builtin `ATTR attr[]`
- Switch Tables: many `NAMETAB` arrays used by individual commands
- Config Keywords: `CONF conftable[]`

## Core Registries

### Commands
- Table: `CMDENT command_table[]`
- Where: [netmush/nametabs.c](netmush/nametabs.c#L621)
- Loader: `init_cmdtab()` builds `mushstate.command_htab` and adds `__` aliases. See [netmush/command.c](netmush/command.c#L48).
- Dynamic: `@addcommand` stores entries in linked lists (`ADDENT`) and attaches to `CMDENT.info.added`.

CMD entries define name, switches, permissions (`CA_*`), extra data, call sequence (`CS_*`), optional hooks, and a handler pointer.

Example (excerpt):

```c
CMDENT command_table[] = {
    { (char*)"@@", NULL, CA_PUBLIC, 0, CS_NO_ARGS, NULL, NULL, NULL, { do_comment } },
    { (char*)"@eval", NULL, CA_NO_SLAVE, 0, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, { do_eval } },
    { (char*)"look", look_sw, CA_LOCATION, LOOK_LOOK, CS_ONE_ARG | CS_INTERP, NULL, NULL, NULL, { do_look } },
    /* ... */
};
```

Call sequence flags (`CS_*`) determine the handler signature and pre/post processing (ARGV, INTERP, CMDARG, etc.). Permission bits (`CA_*`) gate access.

### Functions
- Table: `FUN flist[]`
- Where: [netmush/functions.c](netmush/functions.c#L497)
- Loader: `init_functab()` inserts into `mushstate.func_htab`. See [netmush/functions.c](netmush/functions.c#L38).
- User-defined: Managed via `@function` (`do_function`), tracked by `UFUN` list and `mushstate.ufunc_htab`.

Example (excerpt):

```c
FUN flist[] = {
  { "@@", fun_null, 1, FN_VARARGS | FN_NO_EVAL, CA_PUBLIC, NULL },
  { "ABS", fun_abs, 1, 0, CA_PUBLIC, NULL },
  { "HASPOWER", fun_haspower, 2, 0, CA_PUBLIC, NULL },
  /* ... */
};
```

Function flags (e.g., `FN_VARARGS`, `FN_NO_EVAL`) and permission bits (`CA_*`) control evaluation and access.

### Flags
- Table: `FLAGENT gen_flags[]`
- Where: [netmush/flags.c](netmush/flags.c#L362)
- Loader: `init_flagtab()` → `mushstate.flags_htab`. See [netmush/flags.c](netmush/flags.c#L459).
- Also see `OBJENT object_types[]` for type metadata in [netmush/flags.c](netmush/flags.c#L444).

### Powers
- Table: `POWERENT gen_powers[]`
- Where: [netmush/powers.c](netmush/powers.c#L171)
- Loader: `init_powertab()` → `mushstate.powers_htab`. See [netmush/powers.c](netmush/powers.c#L219).

### Attributes
- Table: `ATTR attr[]`
- Where: [netmush/nametabs.c](netmush/nametabs.c#L1063)
- Note: Attributes with `!(AF_NOCMD)` get auto-exposed as `@<AttrName>` commands inside `init_cmdtab()`.

## Switch Tables (NAMETAB)

Switch tables define per-command switches and decode into bitmasks or constants passed via `CMDENT.extra`. Common fields: name (lowercase), min abbrev length, permission, and value.

Representative tables (all in [netmush/nametabs.c](netmush/nametabs.c)):
- `addcmd_sw` [L27], `attrib_sw` [L31], `boot_sw` [L40], `clone_sw` [L50]
- `emit_sw` [L99], `end_sw` [L107], `examine_sw` [L121]
- `function_sw` [L163], `lock_sw` [L214], `switch_sw` [L548]
- Global access names: `access_nametab[]` [L243]
- Attribute access names: `attraccess_nametab[]` [L261], `indiv_attraccess_nametab[]` [L280]
- List names for `@list`: `list_names[]` [L300]
- Boolean names: `bool_names[]` [L332]
- Help file keys: `list_files[]` [L348]
- Logging: `logdata_nametab[]` [L371], `logoptions_nametab[]` [L381]
- Feature toggles: `enable_names[]` [L402]
- Signals: `sigactions_nametab[]` [L418]

## Config Keywords

- Table: `CONF conftable[]`
- Where: [netmush/nametabs.c](netmush/nametabs.c#L765)
- Purpose: Maps `mush.cnf` keys to handlers and backing fields. Many entries reference nametabs to parse bitmasks (e.g., `access_nametab`).

## Initialization Summary

- Commands: `init_cmdtab()` → [netmush/command.c](netmush/command.c#L48)
- Functions: `init_functab()` → [netmush/functions.c](netmush/functions.c#L38)
- Flags: `init_flagtab()` → [netmush/flags.c](netmush/flags.c#L459)
- Powers: `init_powertab()` → [netmush/powers.c](netmush/powers.c#L219)

These populate the `mushstate` hash tables used at runtime.

## Handler Signatures (by CS_* flags)

Common patterns (see `CS_*` in [netmush/constants.h](netmush/constants.h)):

- `CS_NO_ARGS`: `void cmd(dbref player, dbref cause, int key)`
- `CS_ONE_ARG`: `void cmd(dbref player, dbref cause, int key, char *arg1)`
- `CS_TWO_ARG`: `void cmd(dbref player, dbref cause, int key, char *arg1, char *arg2)`
- `... | CS_ARGV`: add `char *argv[], int argc`
- `... | CS_CMDARG`: add `char *cargs[], int ncargs`

`CS_INTERP` enables evaluation of arguments prior to dispatch; `CS_NOINTERP` disables it. `CS_STRIP_AROUND` controls brace stripping.

## Execution Flow (Commands)

1. Parse input → extract command token and args
2. Hash lookup (`mushstate.command_htab`) → resolve `CMDENT`
3. Permission check (`CA_*`) and any user-defined permits
4. Pre-hook (if present)
5. Dispatch to handler per `CS_*` signature
6. Post-hook (if present)

## Interactions and Lookups

- Builtin commands are in `command_table[]`.
- Attribute commands (`@Attr`) are auto-created from `ATTR attr[]` in `init_cmdtab()`.
- Dynamic `@addcommand` entries chain to the base `CMDENT` for matching and dispatch.
- Functions are looked up through `mushstate.func_htab` (builtin + user-defined via `@function`).
- Flags and powers are found via `mushstate.flags_htab` and `mushstate.powers_htab`.

## How to Extend

### Add a Builtin Command
1. Implement handler (match signature to intended `CS_*`).
2. Add an entry to `command_table[]` in [netmush/nametabs.c](netmush/nametabs.c#L621): name (lowercase for words, special for lead-ins), switches (or `NULL`), `CA_*` perms, `extra`, `CS_*`, hooks (or `NULL`), handler in `info.handler`.
3. Optionally define a `NAMETAB` for switches in the same file and reference it from the command entry.
4. Build and verify the dispatcher resolves it (`@list commands` can help).

### Add a Dynamic Command (Game Content)
Use `@addcommand` at runtime to create content-level commands attached to objects; these live under the `CMDENT.info.added` chain and follow the base handler’s mechanics with user permissions and hooks.

### Add a Builtin Function
1. Implement the function handler (`fun_*` prototype) respecting evaluation flags and varargs usage.
2. Add it to `FUN flist[]` in [netmush/functions.c](netmush/functions.c#L497) with argument count, function flags (e.g., `FN_VARARGS`, `FN_NO_EVAL`), and `CA_*`.
3. If you need an extension-only permission, ensure appropriate `CA_*` exists and is surfaced in `access_nametab[]`.

### Add a Flag
1. Add a constant in `constants.h` and integrate with flag word (FLAG_WORD1/2/3).
2. Add an entry to `FLAGENT gen_flags[]` in [netmush/flags.c](netmush/flags.c#L362) with a setter policy (e.g., `fh_wiz`, `fh_god`).
3. If needed, add aliases via the `flag_alias` config.

### Add a Power
1. Add a constant in `constants.h` and specify main vs extended word (`POWER_EXT`).
2. Add an entry to `POWERENT gen_powers[]` in [netmush/powers.c](netmush/powers.c#L171) with a policy (`ph_wiz`, `ph_god`, etc.).
3. Use `power_alias` for aliases.

## Practical Checks

- Find where a command is registered:

```sh
grep -n '"<command>"' netmush/nametabs.c
```

- Find a handler definition:

```sh
grep -n "void do_<name>\s*\(" netmush/*.c
```

- Inspect function registry entry:

```sh
grep -n '"<FUNCTIONNAME>"' netmush/functions.c
```

## Notes on Placeholders

Handlers like `do_comment()` and `do_eval()` are intentionally minimal or no-op but are still valid command targets. If they appear in `command_table[]`, they are part of the user command surface and must not be removed without re-mapping or migration.

## Related Source Pointers

- Commands table and switches: [netmush/nametabs.c](netmush/nametabs.c)
- Dispatcher and init: [netmush/command.c](netmush/command.c)
- Functions registry: [netmush/functions.c](netmush/functions.c)
- Flags: [netmush/flags.c](netmush/flags.c)
- Powers: [netmush/powers.c](netmush/powers.c)
- Attributes: [netmush/nametabs.c](netmush/nametabs.c#L1063)