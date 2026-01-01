# TinyMUSH ANSI Regression Tests

import socket
import time
import re

HOST = 'localhost'
PORT = 6250
USER = '#1'
PASS = 'potrzebie'

# ANSI regex helpers
ANSI_TRUECOLOR = re.compile(r'\x1b\[38;2;([0-9]+);([0-9]+);([0-9]+)m')
ANSI_RESET = '\x1b[0m'


def connect_and_login():
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((HOST, PORT))
    s.recv(4096)
    s.send(f'connect {USER} {PASS}\n'.encode())
    time.sleep(0.2)
    s.recv(4096)
    return s


def send_think(s, expr):
    s.send(f'think {expr}\n'.encode())
    time.sleep(0.2)
    return s.recv(4096).decode('utf-8', errors='ignore')


def test_truecolor():
    s = connect_and_login()
    out = send_think(s, '%x<#ff00ff>test!')
    s.close()
    assert '\x1b[38;2;255;0;255mtest!\x1b[0m' in out or '\x1b[38;2;255;0;255mtest!\x1b[0m' in out.replace('\r',''), f"Truecolor failed: {repr(out)}"
    print('Truecolor: OK')

def test_ansi():
    s = connect_and_login()
    out = send_think(s, '%xrtest!')
    s.close()
    assert '\x1b[31mtest!\x1b[0m' in out or '\x1b[31mtest!\x1b[0m' in out.replace('\r',''), f"ANSI red failed: {repr(out)}"
    print('ANSI red: OK')

def test_reset():
    s = connect_and_login()
    out = send_think(s, '%xrtest%xntest')
    s.close()
    assert '\x1b[31mtest\x1b[0mtest' in out.replace('\r',''), f"ANSI reset failed: {repr(out)}"
    print('ANSI reset: OK')

def test_no_skip():
    s = connect_and_login()
    out = send_think(s, '%x<#ff00ff>test!')
    s.close()
    assert 'test!' in out, f"No skip failed: {repr(out)}"
    print('No skip: OK')

def run_all():
    test_truecolor()
    test_ansi()
    test_reset()
    test_no_skip()
    print('All ANSI regression tests passed.')

if __name__ == '__main__':
    run_all()
