# Installation Instructions


TinyMUSH now uses the CMake build system. Use the CMake targets directly; install.sh has been removed.

## Build instructions:

1. Install dependencies:

    For Ubuntu/Debian:
    ```
    apt install build-essential cmake libcrypt-dev libgdbm-dev libpcre3-dev
    ```

    For macOS (Homebrew):
    ```
    brew install cmake gdbm pcre
    ```

2. Configure and build:
    ```
    mkdir -p build && cd build
    cmake ..
    cmake --build . --target install-upgrade
    ```
    - `install-upgrade` (default): installs to `../game`, preserves existing configs, and backs them up under `game/backups/upgrade-<timestamp>`.
    - `install-fresh`: removes `../game` before installing (clean reinstall).
    - To disable preservation/backups: `cmake -DTINYMUSH_PRESERVE_CONFIGS=OFF -DTINYMUSH_BACKUP_CONFIGS=OFF ..`

3. Start TinyMUSH:
    ```
    cd ../game
    ./netmush
    ```

Default port is '6250' and default God's password is 'potrzebie'.

## Structure

The structure of TinyMUSH has changed again, here's the new directory tree and a short descriptions of their uses.

```
├── game           Game binaries and pid file.
    ├── backups    Game Backups
    ├── configs    Configuration files
    ├── db         Databases
    ├── docs       Documentation and text files
    ├── logs       Game logs
    ├── modules    Optional modules
    ├── scripts    (not so anymore) Usefull Scripts
    └── systemd    Automatic start / stop / restart scrip for systemd
```

More documentation will be available as development goes.

