#!/usr/bin/env python3
import socket
import binascii
import sys

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

try:
    sock.bind(('127.0.0.1', 6250))
    sock.listen(1)
    print("✓ Listening on 127.0.0.1:6250", flush=True)
    print("Connect with: telnet localhost 6250", flush=True)
    sys.stdout.flush()
except Exception as e:
    print(f"✗ Error: {e}", flush=True)
    sys.exit(1)

while True:
    try:
        client, addr = sock.accept()
        print(f"\n[CONNECTED] {addr}", flush=True)
        client.send(b"Welcome! Type something and press CTRL+C\r\n> ")
        
        while True:
            data = client.recv(1024)
            if not data:
                print("[DISCONNECTED]", flush=True)
                break
            
            print(f"\n[RX] {len(data)} bytes", flush=True)
            print(f"  Hex: {binascii.hexlify(data).decode()}", flush=True)
            print(f"  Repr: {repr(data)}", flush=True)
            
            # Show individual bytes
            for i, b in enumerate(data):
                if 32 <= b <= 126:
                    print(f"    [{i}] = 0x{b:02X} '{chr(b)}'", flush=True)
                else:
                    print(f"    [{i}] = 0x{b:02X}", flush=True)
            
            client.send(b"OK\r\n> ")
        
        client.close()
    except KeyboardInterrupt:
        print("\nShutdown", flush=True)
        break
    except Exception as e:
        print(f"Error: {e}", flush=True)

sock.close()
