# Contributing to TinyMUSH

Thank you for your interest in contributing to TinyMUSH! This document provides guidelines and instructions for contributing to the project.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [How to Contribute](#how-to-contribute)
  - [Reporting Bugs](#reporting-bugs)
  - [Suggesting Features](#suggesting-features)
  - [Security Vulnerabilities](#security-vulnerabilities)
- [Development Setup](#development-setup)
- [Code Guidelines](#code-guidelines)
- [Testing](#testing)
- [Pull Request Process](#pull-request-process)
- [Module Development](#module-development)

## Code of Conduct

This project is focused exclusively on the development, maintenance, and support of TinyMUSH. All interactions should be:

- **Respectful and Constructive**: Treat all contributors with courtesy and professionalism
- **Welcoming**: Help newcomers get started and answer questions patiently
- **On-Topic**: Discussions must relate to TinyMUSH development, usage, or support
- **Apolitical**: Politics, personal opinions, and unrelated social issues have no place in this project

Keep discussions focused on:
- Code contributions and technical decisions
- Bug reports and feature requests
- Documentation improvements
- Installation and configuration help
- Module development

For topics unrelated to TinyMUSH development and usage, please use appropriate external platforms.

## How to Contribute

### Reporting Bugs

Before creating a bug report:
1. Check [KNOWN_BUGS.md](KNOWN_BUGS.md) for known issues
2. Search [existing issues](https://github.com/TinyMUSH/TinyMUSH/issues) to avoid duplicates
3. Test with the latest version if possible

When filing a bug report, include:
- **TinyMUSH version**: Output of `@version` command
- **Operating system**: Distribution and version (e.g., Ubuntu 22.04, macOS 14)
- **Build configuration**: Debug or Release, any custom CMake flags
- **Steps to reproduce**: Clear, numbered steps to reproduce the issue
- **Expected behavior**: What should happen
- **Actual behavior**: What actually happens
- **Error messages**: Complete error output, logs, or stack traces
- **Configuration**: Relevant sections of netmush.conf if applicable

**Example:**

```
**TinyMUSH Version:** 4.0.0-alpha (commit abc1234)
**OS:** Ubuntu 22.04 LTS
**Build:** Debug build with default CMake options

**Steps to Reproduce:**
1. Start server with default config
2. Connect as #1
3. Execute: think structure(test,x y,i i,0 0)
4. Execute: think construct(named,test,x y,1 2,~)

**Expected:** Should create structure successfully
**Actual:** Server crashes with SIGSEGV

**Error Log:**
[see attached netmush.log]
```

### Suggesting Features

Feature requests are welcome! Please:
- Check if the feature already exists or is planned
- Explain the use case and benefits
- Consider backward compatibility implications
- Be open to discussion about implementation approaches

### Security Vulnerabilities

**Do NOT report security vulnerabilities through public GitHub issues.**

See [SECURITY.md](SECURITY.md) for responsible disclosure procedures.

## Development Setup

### Prerequisites

Install required dependencies:

**Ubuntu/Debian:**
```bash
apt install build-essential cmake libcrypt-dev libgdbm-dev libpcre2-dev git
```

**macOS:**
```bash
brew install cmake gdbm pcre2 git
```

**Optional but recommended:**
```bash
# For faster builds
apt install ccache ninja-build

# For debugging
apt install gdb valgrind
```

### Fork and Clone

1. Fork the repository on GitHub
2. Clone your fork:
```bash
git clone https://github.com/YOUR_USERNAME/TinyMUSH.git
cd TinyMUSH
```

3. Add upstream remote:
```bash
git remote add upstream https://github.com/TinyMUSH/TinyMUSH.git
```

### Build for Development

```bash
# Create debug build
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Or use Ninja for faster builds
cmake -GNinja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

### Running Tests

```bash
# Setup Python virtual environment (only needed once)
./setup_venv.sh

# Build and install
cd build
cmake --build . --target install-upgrade

# Run regression tests
cd ../scripts
source ../venv/bin/activate
TINY_USER="#1" TINY_PASS="potrzebie" python3 regression_test.py
```

## Code Guidelines

### General Principles

1. **Safety First**: Always use safe string functions
2. **Memory Management**: Track all allocations, no leaks
3. **Error Handling**: Check return values, handle edge cases
4. **Documentation**: Document public APIs and complex logic
5. **Consistency**: Follow existing code style and patterns

### String Operations

**ALWAYS use safe string functions:**

```c
// CORRECT - Safe operations
XSAFESTRNCPY(dest, src, dest, sizeof(dest), "function_name");
xstrncpy(dest, src, sizeof(dest));
XSAFESTRCAT(dest, src, dest, sizeof(dest), "function_name");
XSAFESPRINTF(buff, bufc, "format %s", arg);

// NEVER use these - Buffer overflow risks!
strcpy(dest, src);      // FORBIDDEN
strcat(dest, src);      // FORBIDDEN
sprintf(dest, ...);     // FORBIDDEN
```

### Memory Management

All dynamic allocations must be tracked:

```c
// Allocate
void *ptr = XMALLOC(size, "function_name");
char *str = XSTRDUP("text", "function_name");

// Always free with matching function
XFREE(ptr, "function_name");
XFREE(str, "function_name");

// Never use raw malloc/free
malloc(size);   // FORBIDDEN
free(ptr);      // FORBIDDEN
```

### Naming Conventions

```c
// Module functions and variables
mod_<modulename>_function_name()
mod_<modulename>_variable_name

// Module constants and macros
MOD_<MODULENAME>_CONSTANT_NAME

// Core server functions (lowercase with underscores)
do_something_useful()
get_player_name()
```

### Code Style

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style for functions, separate line for control structures
- **Line length**: Aim for 80-100 characters, hard limit at 120
- **Comments**: Use `/* */` for multi-line, `//` for single-line
- **Documentation**: Doxygen format for public APIs

**Example:**

```c
/**
 * @brief Get player name by dbref
 * @param player The dbref of the player
 * @return Pointer to player name string, or NULL if invalid
 */
char *get_player_name(dbref player)
{
    if (!Good_obj(player))
    {
        return NULL;
    }

    return Name(player);
}
```

### Error Checking

Always validate inputs and check return values:

```c
// Check function parameters
if (!player || !arg || !*arg)
{
    return;
}

// Check allocations
char *buffer = XMALLOC(BUFFER_SIZE, "function");
if (!buffer)
{
    log_write(LOG_ALWAYS, "ERR", "MEM", "Failed to allocate buffer");
    return;
}

// Check file operations
FILE *fp = fopen(path, "r");
if (!fp)
{
    log_write(LOG_ALWAYS, "ERR", "FILE", "Cannot open %s: %s", path, strerror(errno));
    return;
}
```

## Testing

### Regression Tests

Add tests for new functions and commands:

```bash
# Test file location
scripts/functions_<category>.conf
scripts/commands_<category>.conf

# Run specific test
python3 regression_test.py --filter function_name
```

### Manual Testing

1. Build in debug mode
2. Start server with `-d` flag (foreground)
3. Test your changes thoroughly
4. Check logs for errors or warnings
5. Use valgrind to check for memory leaks:

```bash
valgrind --leak-check=full --track-origins=yes ./game/netmush -d
```

### Memory Leak Detection

```bash
# Build with debug symbols
cmake -DCMAKE_BUILD_TYPE=Debug ..
make

# Run with valgrind
valgrind --leak-check=full --show-leak-kinds=all \
         --track-origins=yes --log-file=valgrind.log \
         ./game/netmush -d

# Test your feature, then shutdown
# Review valgrind.log for leaks
```

## Pull Request Process

### Before Submitting

1. **Update from upstream:**
   ```bash
   git fetch upstream
   git rebase upstream/master
   ```

2. **Run tests:**
   ```bash
   cd scripts
   python3 regression_test.py
   ```

3. **Check for memory leaks** (if applicable)

4. **Update documentation:**
   - Add/update function documentation in code
   - Update relevant README files
   - Add KNOWN_BUGS.md entry if fixing a bug

### Creating the Pull Request

1. **Create a feature branch:**
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** following code guidelines

3. **Commit with clear messages:**
   ```bash
   git commit -m "type: brief description

   Detailed explanation of what changed and why.
   Reference any related issues.

   Fixes #123"
   ```

   Commit types:
   - `feat:` New feature
   - `fix:` Bug fix
   - `docs:` Documentation changes
   - `refactor:` Code refactoring
   - `test:` Adding or updating tests
   - `chore:` Maintenance tasks

4. **Push to your fork:**
   ```bash
   git push origin feature/your-feature-name
   ```

5. **Open pull request** on GitHub with:
   - Clear title describing the change
   - Detailed description of what and why
   - Reference to related issues
   - Testing performed
   - Any breaking changes or migration notes

### Review Process

- Maintainers will review your PR
- Address any feedback or requested changes
- Once approved, your PR will be merged
- You may be asked to rebase if conflicts arise

## Module Development

Creating custom modules is encouraged!

### Quick Start

```bash
# Copy skeleton template
cp -r src/modules/skeleton src/modules/mymodule
cd src/modules/mymodule

# Rename files
mv skeleton.c mymodule.c
mv skeleton.h mymodule.h

# Replace all references
sed -i 's/skeleton/mymodule/g' *.c *.h CMakeLists.txt

# Edit mymodule.c and mymodule.h with your logic
```

### Module Structure

Every module needs:

1. **Header file** (`mymodule.h`):
   - Configuration structure
   - Function prototypes
   - External declarations

2. **Source file** (`mymodule.c`):
   - MODVER metadata
   - CONF table
   - Command/function tables
   - `mod_mymodule_init()` function
   - Implementation

3. **CMakeLists.txt**:
   ```cmake
   add_library(mymodule MODULE mymodule.c)
   add_dependencies(mymodule netmush)
   target_link_libraries(mymodule netmush)
   install(TARGETS mymodule DESTINATION modules)
   ```

### Module Guidelines

- Use `mod_<name>_` prefix for all functions
- Initialize all config values in `mod_<name>_init()`
- Register commands/functions in init
- Initialize hash tables with appropriate flags
- Provide cleanup function if needed
- Document configuration parameters

### Resources

- [Module Development Guide](docs/Code/MODULES_DEVELOPMENT.md)
- [Skeleton Module README](src/modules/skeleton/README.md)
- [Skeleton Quick Reference](src/modules/skeleton/QUICK_REFERENCE.md)
- Existing modules: [mail](src/modules/mail/), [comsys](src/modules/comsys/)

## Getting Help

- **Documentation**: Check [docs/](docs/) directory
- **Issues**: Search [GitHub issues](https://github.com/TinyMUSH/TinyMUSH/issues)
- **Email**: tinymush@googlegroups.com

## License

By contributing to TinyMUSH, you agree that your contributions will be licensed under the Artistic License 2.0. See [COPYING.md](COPYING.md) for details.

---

Thank you for contributing to TinyMUSH!
