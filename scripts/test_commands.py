#!/usr/bin/env python3
"""
TinyMUSH command smoke tester.

- Connects over telnet and runs a list of commands.
- Reads credentials from environment to avoid hard-coding secrets.
- Skips obviously destructive commands by default; adjust the lists below if needed.

Env vars:
  TINY_HOST   (default: 127.0.0.1)
  TINY_PORT   (default: 6250)
  TINY_USER   (required)
  TINY_PASS   (required)
  TINY_TIMEOUT (optional, seconds, default: 3)

Usage:
  python3 scripts/test_commands.py

Notes:
- The default command set is conservative. Add/remove commands in SAFE_COMMANDS / DANGEROUS_COMMANDS.
- Outputs are printed to stdout and annotated with "OK"/"ERR" based on whether the server replied anything.
- If you really want to exercise dangerous commands, move them from DANGEROUS_COMMANDS into SAFE_COMMANDS at your own risk.
"""

import os
import sys
import time
import telnetlib
from typing import List, Set

HOST = os.getenv("TINY_HOST", "127.0.0.1")
PORT = int(os.getenv("TINY_PORT", "6250"))
# Default to developer debug account (#1 potrzebi) unless overridden.
USER = os.getenv("TINY_USER", "#1")
PASS = os.getenv("TINY_PASS", "potrzebi")
TIMEOUT = float(os.getenv("TINY_TIMEOUT", "5"))

# Full command list to exercise. Commands without arguments are sent as-is; those
# that are obviously destructive are skipped unless explicitly requested.
RAW_COMMANDS: List[str] = [
    "@@",
    "@addcommand",
    "@admin",
    "@alias",
    "@apply_marked",
    "@attribute",
    "@boot",
    "@chown",
    "@chownall",
    "@chzone",
    "@clone",
    "@colormap",
    "@cpattr",
    "@create",
    "@cron",
    "@crondel",
    "@crontab",
    "@cut",
    "@dbck",
    "@backup",
    "@decompile",
    "@delcommand",
    "@destroy",
    "@dig",
    "@disable",
    "@doing",
    "@dolist",
    "@drain",
    "@dump",
    "@edit",
    "@emit",
    "@enable",
    "@end",
    "@entrances",
    "@eval",
    "@femit",
    "@fixdb",
    "@floaters",
    "@force",
    "@fpose",
    "@fsay",
    "@freelist",
    "@function",
    "@halt",
    "@hashresize",
    "@hook",
    "@include",
    "@kick",
    "@last",
    "@link",
    "@list",
    "@listcommands",
    "@list_file",
    "@listmotd",
    "@lock",
    "@log",
    "@logrotate",
    "@mark",
    "@mark_all",
    "@motd",
    "@mvattr",
    "@name",
    "@newpassword",
    "@notify",
    "@oemit",
    "@open",
    "@parent",
    "@password",
    "@pcreate",
    "@pemit",
    "@npemit",
    "@poor",
    "@power",
    "@program",
    "@ps",
    "@quota",
    "@quitprogram",
    "@readcache",
    "@redirect",
    "@reference",
    "@restart",
    "@robot",
    "@search",
    "@set",
    "@stats",
    "@startslave",
    "@sweep",
    "@switch",
    "@teleport",
    "@timecheck",
    "@timewarp",
    "@toad",
    "@trigger",
    "@unlink",
    "@unlock",
    "@verb",
    "@wait",
    "@wall",
    "@wipe",
    "drop",
    "enter",
    "examine",
    "get",
    "give",
    "goto",
    "internalgoto",
    "inventory",
    "kill",
    "leave",
    "look",
    "page",
    "pose",
    "reply",
    "say",
    "score",
    "slay",
    "think",
    "use",
    "version",
    "whisper",
    "doing",
    "who",
    "session",
    "info",
    "outputprefix",
    "outputsuffix",
    "puebloclient",
    "DOING",
    "OUTPUTPREFIX",
    "OUTPUTSUFFIX",
    "SESSION",
    "WHO",
    "PUEBLOCLIENT",
    "INFO",
]

DANGEROUS: Set[str] = {
    "@dump",
    "@dbck",
    "@restart",
    "@backup",
    "@destroy",
    "@toad",
    "@boot",
    "@teleport",
    "@link",
    "@chown",
    "@chownall",
    "@chzone",
    "@power",
    "@quota",
    "@timewarp",
    "@wait",
    "@force",
    "@switch",
    "@trigger",
    "@program",
    "@pcreate",
    "@dig",
    "@open",
    "@lock",
    "@unlock",
    "@wipe",
    "@mark",
    "@mark_all",
    "@search",
    "@set",
    "@verb",
    "@include",
    "@cron",
    "@crondel",
    "@crontab",
    "@apply_marked",
    "@startslave",
    "@redirect",
    "@robot",
    "@poor",
    "@wait",
}

# Commands that intentionally close the session; skipped from the main loop so
# the remaining tests can continue. Run manually if needed.
DISCONNECTING: Set[str] = {
    "quit",
    "logout",
    "QUIT",
    "LOGOUT",
    "@reboot",
    "@shutdown",
}

# Commands that need explicit arguments to produce output. Keys are the lowercase
# base command; values are the full command line to send.
ARGUMENT_OVERRIDES = {
    "say": "say [test] scripted",
    "think": "think [test] scripted",
    "pose": "pose strikes a testing pose",
    "look": "look here",
    "drop": "drop here",
    "enter": "enter here",
    "examine": "examine here",
    "get": "get here",
    "give": "give #1=0",
    "goto": "goto here",
    "@emit": "@emit [test] scripted emit",
    "@pemit": "@pemit #1=[test] scripted pemit",
    "@npemit": "@npemit #1=[test] scripted npemit",
    "@oemit": "@oemit #1=[test] scripted oemit",
    "@fsay": "@fsay [test] scripted fsay",
    "@fpose": "@fpose [test] scripted fpose",
    "@list": "@list functions",
    "@list_file": "@list_file motd",
    "@listmotd": "@listmotd/brief",
    "@motd": "@motd show",
    "@log": "@log dump",
    "@mark": "@mark set",
    "@mark_all": "@mark_all",
    "@last": "@last #1",
    "@set": "@set here=SAFE",
    "@lock": "@lock here=me",
    "@unlock": "@unlock here",
    "@link": "@link here=here",
    "@open": "@open __test_exit=here",
    "@dig": "@dig __test_room",
    "@create": "@create __test_thing",
    "@teleport": "@teleport me=here",
    "@name": "@name here=TestRoom",
    "@chown": "@chown here=#1",
    "@quota": "@quota",
    "@stats": "@stats",
    "@timecheck": "@timecheck",
    "@timewarp": "@timewarp 0",
    "@poor": "@poor here=0",
    "@notify": "@notify here=[test] notify",
    "page": "page #1=[test] scripted page",
    "whisper": "whisper #1=[test] scripted whisper",
}


def ensure_creds() -> None:
    if not USER or not PASS:
        sys.stderr.write("Missing TINY_USER and/or TINY_PASS environment variables.\n")
        sys.exit(1)


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
    include_dangerous = "--include-dangerous" in sys.argv or os.getenv("TINY_INCLUDE_DANGEROUS") == "1"

    ensure_creds()
    print(f"Connecting to {HOST}:{PORT} as {USER}…", flush=True)

    tn = telnetlib.Telnet(HOST, PORT, timeout=TIMEOUT)
    welcome = read_some(tn, TIMEOUT)
    if welcome:
        print("[welcome]\n" + welcome.strip())

    login_resp = send_cmd(tn, f"connect {USER} {PASS}", TIMEOUT)
    print("[login]\n" + login_resp.strip())

    commands = []
    for cmd in RAW_COMMANDS:
        base = cmd.split()[0].lower()
        if base in DISCONNECTING:
            continue
        if (base in DANGEROUS) and not include_dangerous:
            continue
        override = ARGUMENT_OVERRIDES.get(base)
        commands.append(override if override else cmd)

    if not include_dangerous:
        skipped = [c for c in RAW_COMMANDS if c.split()[0].lower() in DANGEROUS]
        print(f"Skipping {len(skipped)} dangerous commands (use --include-dangerous or TINY_INCLUDE_DANGEROUS=1 to run them)")

    skipped_disconnect = [c for c in RAW_COMMANDS if c.split()[0].lower() in DISCONNECTING]
    if skipped_disconnect:
        print(f"Skipping {len(skipped_disconnect)} disconnecting commands: {', '.join(skipped_disconnect)}")

    results = []
    for cmd in commands:
        resp = send_cmd(tn, cmd, TIMEOUT)
        status = "OK" if resp.strip() else "ERR"
        print(f"[{status}] {cmd}\n{resp.strip()}\n")
        results.append((cmd, status, resp))

    # Graceful logout if possible
    try:
        tn.write(b"QUIT\n")
    except Exception:
        pass
    tn.close()

    # Summary
    ok = sum(1 for _, status, _ in results if status == "OK")
    failures = [(cmd, resp) for (cmd, status, resp) in results if status != "OK"]
    err = len(failures)
    print(f"Summary: {ok} OK, {err} failed (empty response) out of {len(results)} commands")

    if failures:
        print("Failures:")
        for cmd, resp in failures:
            trimmed = resp.strip()
            preview = trimmed.splitlines()[0] if trimmed else "<no output>"
            print(f" - {cmd}: {preview}")

    # Exit with non-zero status if any command failed
    if err:
        sys.exit(1)


if __name__ == "__main__":
    main()
