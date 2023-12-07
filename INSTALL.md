# Installation Instructions


TinyMUSH now use the CMAKE build system, which is alot easier to maintain than Autotools. To make things even simpler, a build script is included.

## Build instructions:

1. Install dependencies:

    For Ubuntu/Debian:
    ```
    apt install build-essential cmake libcrypt-dev libgdbm-dev libpcre3-dev
    ```

    For OSX:
    ```
    brew install cmake gdbm pcre
    ```

2. Build TinyMUSH:
    ```
    ./install.sh
    ```

3. Start TinyMUSH:
    ```
    cd game
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

