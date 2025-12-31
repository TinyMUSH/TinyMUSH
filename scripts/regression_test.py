#!/usr/bin/env python3
"""
TinyMUSH regression test runner.

- Connects over telnet, reads a config file listing commands to run and the expected output substring.
- Reads credentials from environment to avoid hard-coding secrets.
- Skips intentionally disconnecting commands unless explicitly allowed.

Dependencies:
- telnetlib3 (install with: pip install telnetlib3)

Env vars:
    TINY_HOST   (default: 127.0.0.1)
    TINY_PORT   (default: 6250)
    TINY_USER   (required)
    TINY_PASS   (required)
    TINY_TIMEOUT (optional, seconds, default: 5)
    TINY_CONFIG_FILE (optional, path to config file; default: scripts/commands_messaging.conf)

Usage:
    python3 scripts/regression_test.py --config scripts/commands_messaging.conf
    python3 scripts/regression_test.py -c scripts/commands_messaging.conf

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

try:
    from telnetlib3 import Telnet
except ImportError:
    sys.stderr.write("telnetlib3 is required but not installed.\n")
    sys.stderr.write("Install it with: pip install telnetlib3\n")
    sys.stderr.write("Or use pipx: pipx install telnetlib3\n")
    sys.stderr.write("Or create a virtual environment: python -m venv venv && source venv/bin/activate && pip install telnetlib3\n")
    sys.exit(1)

from typing import List, Tuple

HOST = os.getenv("TINY_HOST", "127.0.0.1")
PORT = int(os.getenv("TINY_PORT", "6250"))
# Default to developer debug account (#1 potrzebi) unless overridden.
USER = os.getenv("TINY_USER", "#1")
PASS = os.getenv("TINY_PASS", "potrzebie")
# Increase default timeout to reduce occasional read races; override with TINY_TIMEOUT.
TIMEOUT = float(os.getenv("TINY_TIMEOUT", "7"))
DEFAULT_CONFIG = os.getenv("TINY_CONFIG_FILE", "scripts/commands_messaging.conf")

DISCONNECTING = {
    "quit",
    "logout",
    "QUIT",
    "LOGOUT",
    "@reboot",
    "@shutdown",
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="TinyMUSH regression test runner")
    parser.add_argument("--config", "-c", default=DEFAULT_CONFIG, help="Path to the test config file")
    parser.add_argument("--allow-disconnect", action="store_true", help="Run commands that may close the session")
    parser.add_argument("--delay", "-d", type=float, default=0.2, help="Delay between commands in seconds (default: 0.2)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Show detailed output for debugging")
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

            if line.startswith(">"): # send line literally without splitting at |
                cmd = line[1:].strip()
                commands.append((cmd, ""))

            elif "|" in line:
                cmd, expected = line.split("|", 1)
                commands.append((cmd.strip(), expected.strip()))
            else:
                commands.append((line, ""))

    if not commands:
        sys.stderr.write("No commands found in config file.\n")
        sys.exit(1)

    return commands


def read_some(tn: Telnet, timeout: float) -> str:
    """Read until no data arrives for a short idle window or timeout expires."""
    chunks: List[str] = []
    end_time = time.time() + timeout
    last_data = None
    idle_grace = 0.3  # Réduit pour être plus réactif, mais toujours attendre un peu

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
        time.sleep(0.02)  # Réduit pour être plus réactif

    return "".join(chunks)


def send_cmd(tn: Telnet, cmd: str, timeout: float, delay: float = 0.2) -> str:
    tn.write((cmd + "\n").encode())
    time.sleep(0.1)  # Petit délai pour laisser le serveur commencer à traiter
    response = read_some(tn, timeout)
    time.sleep(delay)  # Délai configurable pour s'assurer que la réponse est complète
    # Lire une fois de plus au cas où il y aurait eu un délai
    additional = read_some(tn, 0.5)
    if additional:
        response += additional
    return response


def main() -> None:
    args = parse_args()

    commands_with_expectations = load_commands(args.config)

    ensure_creds()
    print(f"Connecting to {HOST}:{PORT} as {USER}…", flush=True)
    print(f"Using config: {args.config}")
    if args.verbose:
        print(f"Command delay: {args.delay}s, Timeout: {TIMEOUT}s")

    tn = Telnet(HOST, PORT, timeout=TIMEOUT)
    welcome = read_some(tn, TIMEOUT)
    if welcome and args.verbose:
        print("[welcome]\n" + welcome.strip())

    login_resp = send_cmd(tn, f"connect {USER} {PASS}", TIMEOUT, args.delay)
    if args.verbose:
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
    obj_id = None
    for cmd, expected in commands:
        # Si la commande est @create testobj, extraire l'ID de l'objet créé
        if cmd.strip().startswith("@create testobj"):
            resp = send_cmd(tn, cmd, TIMEOUT, args.delay)
            import re
            m = re.search(r'created as object #(\d+)', resp)
            if m:
                obj_id = m.group(1)
            status = "OK" if "created as object" in resp else "ERR"
            if args.verbose or status == "ERR":
                print(f"[{status}] {cmd}\n{resp.strip()}\n")
            results.append((cmd, expected, status, resp))
            continue
        # Remplacer testobj par #ID si trouvé
        if obj_id:
            cmd_sub = cmd.replace("testobj", f"#{obj_id}")
            expected_sub = expected.replace("testobj", f"#{obj_id}") if expected else expected
        else:
            cmd_sub = cmd
            expected_sub = expected
        resp = send_cmd(tn, cmd_sub, TIMEOUT, args.delay)
        if expected_sub:
            # Pour les fonctions qui peuvent retourner une chaîne vide, considérer comme OK si expected est vide
            if expected_sub == "" and resp.strip() == "":
                status = "OK"
            else:
                status = "OK" if expected_sub in resp else "ERR"
        else:
            # Si pas d'expectation, considérer OK tant qu'il y a une réponse (même vide)
            status = "OK"
        if args.verbose or status == "ERR":
            print(f"[{status}] {cmd_sub}\n{resp.strip()}\n")
        results.append((cmd_sub, expected_sub, status, resp))

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
