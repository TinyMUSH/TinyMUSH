# TinyMUSH Installation Guide

TinyMUSH 4.0 uses CMake as its build system. This guide covers installation, configuration, and basic setup.

## Quick Start

```bash
# Install dependencies (Ubuntu/Debian)
apt install build-essential cmake libcrypt-dev libgdbm-dev libpcre2-dev

# Build and install
mkdir -p build && cd build
cmake ..
cmake --build . --target install-upgrade -j$(nproc)

# Start server
cd ../game
./netmush
```

Default port: **6250**  
Default God password: **potrzebie**

## Prerequisites

### Ubuntu/Debian

```bash
apt install build-essential cmake libcrypt-dev libgdbm-dev libpcre2-dev
```

### macOS (Homebrew)

```bash
brew install cmake gdbm pcre2
```

### Optional Tools

- `ccache` - Speeds up recompilation (enabled by default)
- `ninja-build` - Faster build system than Make

## Build Process

### 1. Configure

Create a build directory and configure the project:

```bash
mkdir -p build && cd build
cmake ..
```

### 2. Build

Compile the server and modules:

```bash
cmake --build . -j$(nproc)
```

Or use the Makefile wrapper:

```bash
make -j$(nproc)
```

### 3. Install

Two installation targets are available:

**Install/Upgrade (Preserves Config)**

```bash
cmake --build . --target install-upgrade
```

- Installs to `../game/`
- Preserves existing configuration files
- Backs up configs to `game/backups/upgrade-<timestamp>/`

**Fresh Install (Clean Slate)**

```bash
cmake --build . --target install-fresh
```

- Removes `../game/` completely before installing
- Use for initial setup or when config corruption occurs

## Build Options

Configure CMake with these options:

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DTINYMUSH_USE_CCACHE=ON \
      -DTINYMUSH_USE_LTO=ON \
      -DTINYMUSH_PRESERVE_CONFIGS=ON \
      -DTINYMUSH_BACKUP_CONFIGS=ON \
      ..
```

### Available Options

| Option | Default | Description |
|--------|---------|-------------|
| `CMAKE_BUILD_TYPE` | `Release` | Build type: `Release` or `Debug` |
| `TINYMUSH_USE_CCACHE` | `ON` | Use ccache for faster rebuilds |
| `TINYMUSH_USE_LTO` | `ON` | Enable link-time optimization (Release only) |
| `TINYMUSH_PRESERVE_CONFIGS` | `ON` | Preserve existing config files during install |
| `TINYMUSH_BACKUP_CONFIGS` | `ON` | Backup configs before upgrading |

### Using Ninja (Faster Builds)

```bash
cmake -G Ninja ..
ninja
ninja install-upgrade
```

## Module Configuration

Modules are enabled/disabled in `modules.cmake`:

```cmake
# List modules here; enable by leaving uncommented, disable by prefixing with '#'
add_subdirectory(src/modules/mail)
add_subdirectory(src/modules/comsys)
#add_subdirectory(src/modules/skeleton)
```

After changing `modules.cmake`, reconfigure and rebuild:

```bash
cmake ..
make
```

To load modules at runtime, add to `game/configs/netmush.conf`:

```
module mail
module comsys
```

See [docs/Code/MODULES_DEVELOPMENT.md](docs/Code/MODULES_DEVELOPMENT.md) for module development.

## Starting the Server

### Daemon Mode (Background)

By default, the server forks to background:

```bash
cd game
./netmush
```

### Foreground Mode (Debug)

Use `-d` or `--debug` to prevent forking (useful for debugging):

```bash
cd game
./netmush -d
# or
./netmush --debug
```

### Check Server Status

```bash
# Check if running
ps aux | grep netmush

# Check port
netstat -tuln | grep 6250

# View logs
tail -f game/logs/netmush.log
```

## Initial Setup

After first start:

1. Connect as God: `telnet localhost 6250`
2. Login: `connect #1 potrzebie`
3. Change password: `@password old=potrzebie, new=<new_password>`
4. Set up basic configuration via `@admin` commands

## Configuration Files

Located in `game/configs/`:

- `netmush.conf` - Main server configuration
- `alias.conf` - Command aliases
- `compat.conf` - Compatibility settings

Edit these after installation, then `@restart` or restart the server.

## Common Issues

### Build Errors

**Missing dependencies:**
```bash
# Verify installations
dpkg -l | grep -E 'cmake|gdbm|pcre|crypt'
```

**CMake cache issues:**
```bash
rm -rf build
mkdir build && cd build
cmake ..
```

### Runtime Errors

**Port already in use:**
- Change `port` in `netmush.conf`
- Or: `killall netmush` to stop existing instance

**Database corruption:**
```bash
cd game
cp db/*.db backups/
./netmush  # Will attempt recovery
```

**Module fails to load:**
- Check `module <name>` is in `netmush.conf`
- Verify `.so` exists in `game/modules/`
- Check logs: `tail game/logs/netmush.log`


## Directory Structure

### Repository Layout

```
TinyMUSH/
├── CMakeLists.txt          Main build configuration
├── modules.cmake           Module enable/disable list
├── build/                  Build artifacts (generated)
├── game/                   Installed game directory
├── src/
│   ├── netmush/           Core server sources
│   ├── modules/           Loadable modules
│   │   ├── mail/          Mail system module
│   │   ├── comsys/        Communication channels
│   │   └── skeleton/      Template for new modules
│   ├── configs/           Configuration templates
│   ├── scripts/           Maintenance scripts
│   ├── docs/              Documentation sources
│   └── systemd/           systemd service templates
├── docs/
│   ├── Code/              Development documentation
│   └── Historical/        Legacy documentation
└── scripts/               Regression test suites
```

### Game Directory Layout (after install)

```
game/
├── netmush                 Server executable
├── netmush.pid             Process ID file
├── backups/                Database backups
├── configs/                Configuration files
│   ├── netmush.conf       Main configuration
│   ├── alias.conf         Command aliases
│   └── compat.conf        Compatibility settings
├── db/                     Database files
├── docs/                   Installed documentation
├── logs/                   Server logs
├── modules/                Compiled module .so files
├── scripts/                Helper scripts
└── systemd/                systemd service file
```

## Backup and Recovery

### Manual Backup

```bash
cd game
# Backup database
tar czf backups/db-$(date +%Y%m%d-%H%M%S).tar.gz db/

# Backup everything
tar czf backups/full-$(date +%Y%m%d-%H%M%S).tar.gz .
```

### Restore from Backup

```bash
cd game
tar xzf backups/db-<timestamp>.tar.gz
./netmush
```

### Automated Backups

Set up `@daily` in-game on God (#1):

```
@daily me = @dump/flatfile; @wait 5={@shutdown}
```

Or use cron:

```bash
0 3 * * * cd /path/to/game && ./netmush @dump/flatfile
```

## Upgrading

### From Previous Version

1. Stop the server: `killall netmush`
2. Backup database: `tar czf db-backup.tar.gz game/db/`
3. Pull latest code: `git pull`
4. Rebuild: `cd build && cmake .. && make install-upgrade`
5. Review config changes in `game/configs/netmush.conf.example`
6. Restart: `cd game && ./netmush`

### Config Migration

Config backups are stored in `game/backups/upgrade-<timestamp>/`:

```bash
# Compare old and new configs
diff game/backups/upgrade-*/netmush.conf game/configs/netmush.conf
```

## Development Setup

For active development:

```bash
# Debug build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Quick rebuild script
cat > ../rebuild.sh << 'EOF'
#!/bin/bash
killall -9 netmush 2>/dev/null
cd build
cmake --build . --target install-upgrade -j$(nproc)
cd ../game
./netmush
EOF
chmod +x ../rebuild.sh
```

## Additional Resources

- **Main README**: [README.md](README.md)
- **Module Development**: [docs/Code/MODULES_DEVELOPMENT.md](docs/Code/MODULES_DEVELOPMENT.md)
- **Skeleton Module**: [src/modules/skeleton/README.md](src/modules/skeleton/README.md)
- **Known Issues**: [KNOWN_BUGS.md](KNOWN_BUGS.md)
- **Security**: [SECURITY.md](SECURITY.md)

## Support

For issues, questions, or contributions:

- **Issues**: https://github.com/TinyMUSH/TinyMUSH/issues
- **Email**: tinymush@googlegroups.com
- **Documentation**: Check `docs/` directory

---

**TinyMUSH 4.0** - Copyright (C) 1989-2025 TinyMUSH Development Team
