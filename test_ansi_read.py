#!/usr/bin/env python3
import telnetlib
import time

tn = telnetlib.Telnet("localhost", 6250, timeout=5)
time.sleep(0.5)
welcome = tn.read_very_eager()
print("WELCOME:", repr(welcome[:100]))

tn.write(b"connect #1 potrzebie\n")
time.sleep(0.5)
login_resp = tn.read_very_eager()
print("LOGIN:", repr(login_resp[:100]))

tn.write(b"think %xrHello\n")
time.sleep(0.5)
output = tn.read_very_eager()
print("RAW OUTPUT:", repr(output))
print("Contains ANSI?:", b'\x1b[' in output)
if b'\x1b[' in output:
    print("ANSI sequences found!")
    # Find and print the ANSI sequence
    idx = output.find(b'\x1b[')
    print("ANSI part:", repr(output[idx:idx+30]))

tn.write(b"quit\n")
time.sleep(0.2)
tn.close()
