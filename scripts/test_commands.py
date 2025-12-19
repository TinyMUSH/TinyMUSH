#!/usr/bin/env python3
"""
TinyMUSH command smoke tester.

- Connects over telnet, reads a config file listing commands to run and the expected output substring.
- Reads credentials from environment to avoid hard-coding secrets.
- Skips intentionally disconnecting commands unless explicitly allowed.

Env vars:
    TINY_HOST   (default: 127.0.0.1)
    TINY_PORT   (default: 6250)
    TINY_USER   (required)
    TINY_PASS   (required)
    TINY_TIMEOUT (optional, seconds, default: 5)
    TINY_COMMANDS_FILE (optional, path to config file; default: scripts/test_commands.conf)

Usage:
    python3 scripts/test_commands.py --config scripts/test_commands.conf

Config file format:
    - One command per line.
    - Optional expected substring after a pipe ("|").
        Example:  say hello world | You say
    - Blank lines and lines starting with "#" are ignored.

Notes:
- If no expected substring is provided, any non-empty output is considered OK.
- Use --allow-disconnect to run commands that terminate the session (quit/logout/etc.).
"""

import argparse
import os
import sys
import time
import telnetlib
from typing import List, Tuple

HOST = os.getenv("TINY_HOST", "127.0.0.1")
PORT = int(os.getenv("TINY_PORT", "6250"))
# Default to developer debug account (#1 potrzebi) unless overridden.
USER = os.getenv("TINY_USER", "#1")
PASS = os.getenv("TINY_PASS", "potrzebi")
TIMEOUT = float(os.getenv("TINY_TIMEOUT", "5"))
DEFAULT_CONFIG = os.getenv("TINY_COMMANDS_FILE", "scripts/test_commands.conf")

DISCONNECTING = {
    "quit",
    "logout",
    "QUIT",
    "LOGOUT",
    "@reboot",
    "@shutdown",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="TinyMUSH telnet smoke tester")
    parser.add_argument("--config", default=DEFAULT_CONFIG, help="Path to the commands config file")
    parser.add_argument("--allow-disconnect", action="store_true", help="Run commands that may close the session")
    return parser.parse_args()


def ensure_creds() -> None:
    if not USER or not PASS:
        sys.stderr.write("Missing TINY_USER and/or TINY_PASS environment variables.\n")
        sys.exit(1)


def load_commands(path: str) -> List[Tuple[str, str]]:
    commands: List[Tuple[str, str]] = []

    if not os.path.exists(path):
        sys.stderr.write(f"Config file not found: {path}\n")
        sys.exit(1)

    with open(path, "r", encoding="utf-8") as cfg:
        for raw_line in cfg:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue

            if "|" in line:
                cmd, expected = line.split("|", 1)
                commands.append((cmd.strip(), expected.strip()))
            else:
                commands.append((line, ""))

    if not commands:
        sys.stderr.write("No commands found in config file.\n")
        sys.exit(1)

    return commands


def read_some(tn: telnetlib.Telnet, timeout: float) -> str:
    """Read until no data arrives for a short idle window or timeout expires."""
    chunks: List[str] = []
    end_time = time.time() + timeout
    last_data = None
    idle_grace = 0.2  # stop after this idle gap once we've seen data

    while time.time() < end_time:
        data = tn.read_very_eager()
        if data:
            chunks.append(data.decode(errors="replace"))
            last_data = time.time()
            continue

        # If we've seen data and have been idle long enough, stop.
        if last_data is not None and (time.time() - last_data) > idle_grace:
            break

        # Avoid busy loop; small sleep before next check.
        time.sleep(0.05)

    return "".join(chunks)


def send_cmd(tn: telnetlib.Telnet, cmd: str, timeout: float) -> str:
    tn.write((cmd + "\n").encode())
    return read_some(tn, timeout)


def main() -> None:
    args = parse_args()
    commands_with_expectations = load_commands(args.config)

    ensure_creds()
    print(f"Connecting to {HOST}:{PORT} as {USER}…", flush=True)
    print(f"Using config: {args.config}")

    tn = telnetlib.Telnet(HOST, PORT, timeout=TIMEOUT)
    welcome = read_some(tn, TIMEOUT)
    if welcome:
        print("[welcome]\n" + welcome.strip())

    login_resp = send_cmd(tn, f"connect {USER} {PASS}", TIMEOUT)
    print("[login]\n" + login_resp.strip())

    commands = []
    skipped_disconnect = []
    for cmd, expected in commands_with_expectations:
        base = cmd.split()[0].lower()
        if (base in DISCONNECTING) and not args.allow_disconnect:
            skipped_disconnect.append(cmd)
            continue
        commands.append((cmd, expected))

    if skipped_disconnect:
        print(f"Skipping {len(skipped_disconnect)} disconnecting commands (enable with --allow-disconnect): {', '.join(skipped_disconnect)}")

    results = []
    for cmd, expected in commands:
        resp = send_cmd(tn, cmd, TIMEOUT)

        if expected:
            status = "OK" if expected in resp else "ERR"
        else:
            status = "OK" if resp.strip() else "ERR"

        print(f"[{status}] {cmd}\n{resp.strip()}\n")
        results.append((cmd, expected, status, resp))

    # Graceful logout if possible
    try:
        tn.write(b"QUIT\n")
    except Exception:
        pass
    tn.close()

    # Summary
    ok = sum(1 for *_, status, __ in results if status == "OK")
    failures = [(cmd, expected, resp) for (cmd, expected, status, resp) in results if status != "OK"]
    err = len(failures)
    print(f"Summary: {ok} OK, {err} failed out of {len(results)} commands")

    if failures:
        print("Failures:")
        for cmd, expected, resp in failures:
            trimmed = resp.strip()
            preview = trimmed.splitlines()[0] if trimmed else "<no output>"
            expectation = f" (expected contains: '{expected}')" if expected else ""
            print(f" - {cmd}: {preview}{expectation}")

    # Exit with non-zero status if any command failed
    if err:
        sys.exit(1)


if __name__ == "__main__":
    main()
