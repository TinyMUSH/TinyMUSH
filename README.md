|Build status|
|------------|
|[![Build Status](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/ubuntu-latest.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/ubuntu-latest.yml)|
|[![Build Status](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/macos-latest.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/macos-latest.yml)|
|[![Build Status](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/Doxygen.yml/badge.svg)](https://github.com/TinyMUSH/TinyMUSH/actions/workflows/Doxygen.yml)|

- [TinyMUSH 4](#tinymush-4)
  - [Introduction](#introduction)
  - [What's New in TinyMUSH 4](#whats-new-in-tinymush-4)
  - [Quick Start](#quick-start)
  - [Key Features](#key-features)
  - [Development Goals](#development-goals)
  - [Project History](#project-history)
- [Installation](#installation)
- [Documentation](#documentation)
- [Contributing](#contributing)
- [Support](#support)

# TinyMUSH 4

## Introduction

**TinyMUSH 4** is a modern, actively maintained Multi-User Shared Hallucination (MUSH) server. It represents a comprehensive modernization of the TinyMUSH codebase, focusing on robustness, maintainability, and alignment with contemporary C standards.

TinyMUSH 4 builds upon decades of MUSH development heritage while introducing significant architectural improvements, enhanced safety mechanisms, and a more flexible module system. Whether you're running a social roleplaying game, an educational environment, or a creative collaborative space, TinyMUSH 4 provides a solid, extensible foundation.

**Current Status:** Active development (Alpha)

## What's New in TinyMUSH 4

- **Modern Build System**: CMake-based build with fast incremental compilation (ccache, LTO support)
- **Enhanced Safety**: Comprehensive stack-safe string operations, memory tracking, bounds checking
- **Modular Architecture**: Dynamic module loading system with template skeleton for rapid development
- **Improved Robustness**: Fixed critical bugs, eliminated buffer overflows, crash prevention
- **Better Developer Experience**: Streamlined API, clear documentation, regression test suites
- **Code Modernization**: Aligned with modern C standards, cleaned up legacy code patterns
- **Simplified Configuration**: Centralized module management, easier customization

## Quick Start

```bash
# Clone repository
git clone https://github.com/TinyMUSH/TinyMUSH.git
cd TinyMUSH

# Install dependencies (Ubuntu/Debian)
apt install build-essential cmake libcrypt-dev libgdbm-dev libpcre2-dev

# Build and install
mkdir -p build && cd build
cmake ..
cmake --build . --target install-upgrade -j$(nproc)

# Start server
cd ../game
./netmush

# Connect: telnet localhost 6250
# Login as God: connect #1 potrzebie
```

See [INSTALL.md](INSTALL.md) for comprehensive installation instructions.

## Key Features

### Core Capabilities
- **Rich Softcode Language**: Powerful scripting with 200+ built-in functions
- **Flexible Permissions**: Granular control with flags, powers, and locks
- **Persistent Storage**: GDBM-backed database with automatic checkpointing
- **Zone-based Organization**: Hierarchical object management
- **Attribute System**: Extensible object properties with inheritance

### Developer Features
- **Module System**: Load custom C modules dynamically at runtime (mail and comsys included by default)
- **Regression Testing**: Comprehensive test suites for functions and commands
- **Memory Debugging**: Built-in allocation tracking and leak detection
- **Safe String API**: Prevent buffer overflows with XSAFE* macros
- **API Documentation**: Auto-generated Doxygen docs at https://tinymush.github.io/TinyMUSH/

### Administration
- **Flexible Configuration**: Text-based config with runtime @admin adjustments
- **Backup Tools**: Automated database dumps and flatfile conversion
- **Logging System**: Configurable logging with multiple log types
- **Command Aliases**: Customize command names for user preference
- **Compatibility Modes**: Support for MUX/Penn command variations

## Development Goals

TinyMUSH 4 is guided by four core principles:

### 1. **Future-Proofing**
- Modernize codebase to ensure long-term viability
- Align with current C standards (C11+)
- Replace deprecated APIs and unsafe functions
- Support contemporary build tools and platforms

### 2. **Maintainability**
- Reduce code complexity and technical debt
- Improve code organization and modularity
- Enhance documentation at all levels
- Simplify contribution process for new developers

### 3. **Robustness**
- Eliminate undefined behavior and memory safety issues
- Add comprehensive bounds checking
- Implement thorough error handling
- Build extensive regression test coverage

### 4. **Extensibility**
- Provide clean module API for custom extensions
- Maintain backward compatibility where feasible
- Enable easy feature addition without core modifications
- Support diverse use cases through configuration

## Project History

### Origins (TinyMUSH 3)

TinyMUSH 3 emerged from a merger of the TinyMUSH 2.2 and TinyMUX codebases, combining the strengths of both servers. The project maintained near-complete backward compatibility while introducing stability improvements and new features.

**TinyMUSH 3 Development Team:**
- David Passmore (core developer)
- Lydia Leong (lead developer, 3.1-3.2)
- Robby Griffin (joined 3.0 beta 18)
- Scott Dorr (joined 3.0 beta 23)
- Eddy Beaupré (joined 3.1 p1, continuing into 4.0)

TinyMUSH 3 went through several major versions:
- **3.0**: Initial merged release, bug fixes, feature additions
- **3.1**: Enhanced stability, GDBM database, improved caching
- **3.2**: Performance optimizations, additional features
- **3.3**: Experimental branch (never released, discontinued)

### TinyMUSH 4 (Current)

TinyMUSH 4 represents a significant evolution, focusing on modernization rather than feature additions. While maintaining the essence of TinyMUSH 3, version 4 prioritizes code quality, safety, and developer experience.

**Major Changes:**
- Complete CMake conversion (removed Autoconf/Make)
- Stack-safe string operations throughout codebase
- Modular architecture with dynamic loading
- Enhanced error handling and crash prevention
- Modern memory management patterns
- Comprehensive documentation overhaul
- Active regression testing framework

The server is currently in alpha development, with ongoing work to ensure stability before beta release.

# Installation

See [INSTALL.md](INSTALL.md) for complete installation instructions.

**Quick install:**

```bash
mkdir -p build && cd build
cmake ..
cmake --build . --target install-upgrade -j$(nproc)
cd ../game && ./netmush
```

# Documentation

## User & Administrator Resources

| Document | Description |
|----------|-------------|
| [README.md](README.md) | This document - project overview and quick start |
| [INSTALL.md](INSTALL.md) | Comprehensive installation guide |
| [CONTRIBUTING.md](CONTRIBUTING.md) | How to contribute to TinyMUSH |
| [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) | Community guidelines and code of conduct |
| [SECURITY.md](SECURITY.md) | Security policy and vulnerability reporting |
| [KNOWN_BUGS.md](KNOWN_BUGS.md) | Current known issues and recent fixes |
| [NEWS.md](NEWS.md) | Important announcements and updates |
| [COPYING.md](COPYING.md) | License information (Artistic License) |
| [AUTHORS.md](AUTHORS.md) | Credits and acknowledgments |

## Developer Documentation

Located in [docs/Code/](docs/Code/):

| Document | Description |
|----------|-------------|
| [MODULES_DEVELOPMENT.md](docs/Code/MODULES_DEVELOPMENT.md) | Complete guide to module development |
| [ALLOC_DOCUMENTATION.md](docs/Code/ALLOC_DOCUMENTATION.md) | Memory management and safe string APIs |
| [COMMANDS_AND_REGISTRIES_REFERENCE.md](docs/Code/COMMANDS_AND_REGISTRIES_REFERENCE.md) | Commands, functions, flags, powers, attributes |

### Included Modules

TinyMUSH 4 includes the following modules enabled by default:

| Module | Description |
|--------|-------------|
| [mail](src/modules/mail/) | Internal mail system with PennMUSH-compatible syntax |
| [comsys](src/modules/comsys/) | Communication channels for multi-user chat |
| [skeleton](src/modules/skeleton/) | Template for creating new modules ([README](src/modules/skeleton/README.md)) |

See [INSTALL.md](INSTALL.md) for module configuration and [docs/Code/MODULES_DEVELOPMENT.md](docs/Code/MODULES_DEVELOPMENT.md) for creating custom modules.

## Historical Documentation

**Note:** Historical documents are preserved for reference but may not reflect current TinyMUSH 4 practices.

Located in [docs/Historical/](docs/Historical/):

| Document | Description |
|----------|-------------|
| [ChangeLog.md](docs/ChangeLog.md) | TinyMUSH 3 version history |
| [ChangeLog.History.md](docs/Historical/ChangeLog.History.md) | TinyMUSH 2.x and TinyMUX 1.0 history |
| [FAQ.md](docs/Historical/FAQ.md) | Frequently asked questions (TinyMUSH 3 era) |
| [MODULES.md](docs/Historical/MODULES.md) | Legacy module documentation |
| [CONVERTING.md](docs/Historical/CONVERTING.md) | Database conversion guide |


## API Documentation

Auto-generated Doxygen source code documentation:

**https://tinymush.github.io/TinyMUSH/**

Includes detailed API references for functions, structures, and macros used throughout the codebase.

# Contributing

We welcome contributions! See [CONTRIBUTING.md](CONTRIBUTING.md) for:
- How to report bugs and request features
- Development setup and workflow
- Code style guidelines
- Testing requirements
- Pull request process

For security vulnerabilities, see [SECURITY.md](SECURITY.md).

# Support

## Getting Help

1. Check [INSTALL.md](INSTALL.md) and [KNOWN_BUGS.md](KNOWN_BUGS.md)
2. Search [GitHub issues](https://github.com/TinyMUSH/TinyMUSH/issues)
3. Email: tinymush@googlegroups.com

## License

TinyMUSH is distributed under the Artistic License. See [COPYING.md](COPYING.md) for details.

---

**TinyMUSH 4.0** - Copyright © 1989-2025 TinyMUSH Development Team  
**Repository**: https://github.com/TinyMUSH/TinyMUSH  
**Website**: https://tinymush.github.io/TinyMUSH/
