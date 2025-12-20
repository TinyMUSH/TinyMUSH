# Security Policy

## Supported Versions

TinyMUSH 4.0 is currently in active development (alpha). Security updates and bug fixes are provided for:

| Version | Status | Support |
|---------|--------|---------|
| 4.0.x (alpha) | Active development | :white_check_mark: Full support |
| 3.3.x | Discontinued | :x: No longer maintained |
| 3.2.x | Legacy | :x: No longer maintained |
| 3.1.x | Legacy | :x: No longer maintained |
| < 3.0 | Legacy | :x: No longer maintained |

**Recommendation**: Always run the latest version from the master branch for current security fixes.

## Reporting a Vulnerability

**DO NOT report security vulnerabilities through public GitHub issues.**

Security vulnerabilities should be reported privately to allow time for fixes before public disclosure.

### How to Report

Send a detailed report to: **tinymush@googlegroups.com**

Include:
- **Description**: Clear explanation of the vulnerability
- **Impact**: What could an attacker do with this vulnerability?
- **Reproduction**: Step-by-step instructions to reproduce the issue
- **Affected Versions**: Which versions are vulnerable
- **Proof of Concept**: Code or commands demonstrating the vulnerability (if applicable)
- **Suggested Fix**: If you have ideas for fixing it (optional)

### What to Expect

1. **Acknowledgment**: We will acknowledge receipt within 72 hours
2. **Assessment**: We will investigate and assess the severity
3. **Fix Development**: We will work on a fix for confirmed vulnerabilities
4. **Disclosure Timeline**: We aim to release fixes within 90 days
5. **Credit**: You will be credited in the security advisory (unless you prefer to remain anonymous)

### Disclosure Policy

- We follow responsible disclosure practices
- We will coordinate with you on the disclosure timeline
- Public disclosure will occur after a fix is available
- Security advisories will be published on GitHub

## Security Considerations

### Running TinyMUSH Safely

#### Network Security

**Port Exposure:**
- Default port is 6250 (configurable in netmush.conf)
- Use a firewall to restrict access if needed
- Consider running behind a reverse proxy for additional security

**SSL/TLS:**
- TinyMUSH does not natively support SSL/TLS
- Use stunnel or similar tools to provide encrypted connections
- Example: `stunnel4 -d 6251 -r localhost:6250 -P none`

#### File System Security

**Permissions:**
```bash
# Server executable - read/execute only
chmod 755 game/netmush

# Database files - restrict to server user
chmod 600 game/db/*

# Configuration files - read only by server user
chmod 600 game/configs/*.conf

# Logs - append only
chmod 640 game/logs/*.log
```

**Run as Non-Root User:**
```bash
# NEVER run TinyMUSH as root
# Create a dedicated user
useradd -r -s /bin/false tinymush
chown -R tinymush:tinymush /path/to/game
sudo -u tinymush ./game/netmush
```

#### Configuration Security

**God Password:**
- Change the default God password immediately after installation
- Default is `potrzebie` - this is well-known and must be changed
- Use a strong, unique password

```
connect #1 potrzebie
@password potrzebie=<strong_new_password>
```

**Wizard Access:**
- Limit the number of wizard characters
- Audit wizard actions regularly
- Use `@log wizard` to track wizard commands

**Guest Access:**
- Disable guest access if not needed: `guests_disabled yes` in netmush.conf
- If enabled, restrict guest permissions appropriately
- Monitor guest activity

#### Input Validation

TinyMUSH 4 includes enhanced input validation, but administrators should:

- Monitor logs for unusual activity
- Set reasonable limits on user input (configured in netmush.conf)
- Use `@quota` to limit database object creation per player
- Review and adjust configuration parameters for your security needs

### Module Security

**Built-in Modules:**
- Only load modules you trust and need
- mail and comsys modules are enabled by default
- Disable unused modules by commenting them out in modules.cmake

**Custom Modules:**
- Review code before loading third-party modules
- Modules have full access to server internals
- A malicious module can compromise the entire server
- Only use modules from trusted sources

**Module Configuration:**
```conf
# In netmush.conf, only load necessary modules
module mail
module comsys
# module untrusted_module  # commented out
```

### Database Security

**Backups:**
- Regular backups are essential for security and disaster recovery
- Store backups securely with appropriate permissions
- Test backup restoration periodically

```bash
# Automated daily backup
0 3 * * * cd /path/to/game && tar czf backups/db-$(date +\%Y\%m\%d).tar.gz db/
```

**Database Integrity:**
- Monitor database size for unexpected growth
- Check logs for unusual patterns
- Use `@dbcheck` regularly to verify database integrity

### Server Security Updates

**Operating System:**
- Keep your OS and dependencies up to date
- Regularly update: gdbm, pcre, libc, and other system libraries
- Subscribe to security mailing lists for your distribution

**TinyMUSH Updates:**
- Watch the GitHub repository for security updates
- Subscribe to GitHub notifications for the repository
- Review [KNOWN_BUGS.md](KNOWN_BUGS.md) for recent fixes

## Common Security Issues

### What Constitutes a Security Vulnerability

**Security Vulnerabilities (report privately):**
- Remote code execution exploits
- Authentication bypass
- Privilege escalation (non-wizard gaining wizard access)
- Database corruption through crafted input
- Server crashes exploitable for denial of service
- Information disclosure (reading protected data)
- Buffer overflows or memory corruption bugs

**Regular Bugs (report publicly):**
- Softcode execution errors
- Feature malfunctions
- Performance issues
- Non-exploitable crashes
- Documentation errors

### Known Security Enhancements in TinyMUSH 4

TinyMUSH 4 includes significant security improvements:

- **Stack-safe string operations**: Eliminated buffer overflow risks with XSAFE* macros
- **Memory tracking**: Prevents memory leaks and use-after-free vulnerabilities
- **Input validation**: Enhanced bounds checking throughout
- **Crash prevention**: Fixed multiple critical crashes (structure operations, sandbox, list operations)
- **Safe defaults**: More secure default configuration

See [KNOWN_BUGS.md](KNOWN_BUGS.md) for recently fixed security-relevant issues.

## Security Best Practices Summary

1. **Change default passwords** immediately
2. **Run as non-root** dedicated user
3. **Restrict file permissions** appropriately
4. **Keep software updated** (OS and TinyMUSH)
5. **Use firewall rules** to limit access
6. **Enable SSL/TLS** via stunnel or proxy
7. **Backup regularly** and test restores
8. **Monitor logs** for suspicious activity
9. **Limit wizards** and audit their actions
10. **Only load trusted modules**

## Additional Resources

- [INSTALL.md](INSTALL.md) - Installation and configuration
- [KNOWN_BUGS.md](KNOWN_BUGS.md) - Recent security fixes
- [CONTRIBUTING.md](CONTRIBUTING.md) - Code guidelines including safety requirements

## Contact

For security concerns: **tinymush@googlegroups.com**

For general support: Open an issue on [GitHub](https://github.com/TinyMUSH/TinyMUSH/issues)

---

**Remember**: Security is a continuous process, not a one-time setup. Stay vigilant and keep your installation updated.
