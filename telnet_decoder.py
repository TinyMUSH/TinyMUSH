#!/usr/bin/env python3
"""
Telnet Options Negotiation Decoder for MUSH Servers

This script connects to a MUSH server and decodes all Telnet options negotiation
messages exchanged during the connection establishment. It logs both the raw
bytes and human-readable descriptions of the negotiation process.

Usage: python3 telnet_decoder.py <host> <port>

Example: python3 telnet_decoder.py example.com 4201
"""

import socket
import sys
import time

# Telnet protocol constants
IAC = 255  # Interpret As Command
DONT = 254
DO = 253
WONT = 252
WILL = 251
SB = 250   # Subnegotiation Begin
SE = 240   # Subnegotiation End
NOP = 241
DM = 242   # Data Mark
BRK = 243  # Break
IP = 244   # Interrupt Process
AO = 245   # Abort Output
AYT = 246  # Are You There
EC = 247   # Erase Character
EL = 248   # Erase Line
GA = 249   # Go Ahead

# Option codes
OPTIONS = {
    0: "Binary Transmission",
    1: "Echo",
    2: "Reconnection",
    3: "Suppress Go Ahead",
    4: "Approx Message Size Negotiation",
    5: "Status",
    6: "Timing Mark",
    7: "Remote Controlled Trans and Echo",
    8: "Output Line Width",
    9: "Output Page Size",
    10: "Output Carriage-Return Disposition",
    11: "Output Horizontal Tab Stops",
    12: "Output Horizontal Tab Disposition",
    13: "Output Formfeed Disposition",
    14: "Output Vertical Tabstops",
    15: "Output Vertical Tab Disposition",
    16: "Output Linefeed Disposition",
    17: "Extended ASCII",
    18: "Logout",
    19: "Byte Macro",
    20: "Data Entry Terminal",
    21: "SUPDUP",
    22: "SUPDUP Output",
    23: "Send Location",
    24: "Terminal Type",
    25: "End of Record",
    26: "TACACS User Identification",
    27: "Output Marking",
    28: "Terminal Location Number",
    29: "Telnet 3270 Regime",
    30: "X.3 PAD",
    31: "Negotiate About Window Size",
    32: "Terminal Speed",
    33: "Remote Flow Control",
    34: "Linemode",
    35: "X Display Location",
    36: "Environment Option",
    37: "Authentication Option",
    38: "Encryption Option",
    39: "New Environment Option",
    40: "TN3270E",
    41: "XAUTH",
    42: "CHARSET",
    43: "Telnet Remote Serial Port",
    44: "Com Port Control Option",
    45: "Telnet Suppress Local Echo",
    46: "Telnet Start TLS",
    47: "KERMIT",
    48: "SEND-URL",
    49: "FORWARD_X",
    138: "TELOPT PRAGMA LOGON",
    139: "TELOPT SSPI LOGON",
    140: "TELOPT PRAGMA HEARTBEAT",
}

# Subnegotiation types for Terminal Type
TTYPE_COMMANDS = {
    1: "IS",      # Terminal type is...
    2: "SEND",    # Please send terminal type
}

# Subnegotiation types for New Environment Option
NEW_ENV_COMMANDS = {
    0: "IS",      # Variable is...
    1: "SEND",    # Please send variable
    2: "INFO",    # Variable information
}

def get_option_name(code):
    """Get human-readable name for a Telnet option code."""
    return OPTIONS.get(code, f"Unknown Option {code}")

def decode_command(cmd):
    """Decode a Telnet command byte."""
    commands = {
        IAC: "IAC",
        DONT: "DONT",
        DO: "DO",
        WONT: "WONT",
        WILL: "WILL",
        SB: "SB (Subnegotiation Begin)",
        SE: "SE (Subnegotiation End)",
        NOP: "NOP",
        DM: "DM (Data Mark)",
        BRK: "BRK (Break)",
        IP: "IP (Interrupt Process)",
        AO: "AO (Abort Output)",
        AYT: "AYT (Are You There)",
        EC: "EC (Erase Character)",
        EL: "EL (Erase Line)",
        GA: "GA (Go Ahead)",
    }
    return commands.get(cmd, f"Unknown Command {cmd}")

def decode_subnegotiation(data):
    """Decode subnegotiation data."""
    if not data:
        return "Empty subnegotiation"

    option = data[0]
    sub_data = data[1:]

    if option == 24:  # Terminal Type
        if sub_data:
            cmd = sub_data[0]
            if cmd == 0:  # IS
                terminal_type = bytes(sub_data[1:]).decode('ascii', errors='ignore')
                return f"Terminal Type IS: {terminal_type}"
            elif cmd == 1:  # SEND
                return "Terminal Type SEND (request for terminal type)"
        return f"Terminal Type subnegotiation: {sub_data}"

    elif option == 31:  # Window Size
        if len(sub_data) >= 4:
            width = (sub_data[0] << 8) + sub_data[1]
            height = (sub_data[2] << 8) + sub_data[3]
            return f"Window Size: {width}x{height}"
        return f"Window Size subnegotiation: {sub_data}"

    elif option == 32:  # Terminal Speed
        if sub_data:
            cmd = sub_data[0]
            if cmd == 0:  # IS
                speed_data = bytes(sub_data[1:]).decode('ascii', errors='ignore')
                return f"Terminal Speed IS: {speed_data}"
        return f"Terminal Speed subnegotiation: {sub_data}"

    elif option == 39:  # New Environment Option
        if sub_data:
            cmd = sub_data[0]
            if cmd in NEW_ENV_COMMANDS:
                return f"New Environment {NEW_ENV_COMMANDS[cmd]}: {bytes(sub_data[1:]).decode('ascii', errors='ignore')}"
        return f"New Environment subnegotiation: {sub_data}"

    else:
        return f"{get_option_name(option)} subnegotiation: {sub_data}"

def parse_telnet_data(data, direction="Server"):
    """Parse Telnet data and extract negotiation messages."""
    messages = []
    i = 0

    while i < len(data):
        if data[i] == IAC:
            if i + 1 < len(data):
                cmd = data[i + 1]

                if cmd in (DO, DONT, WILL, WONT):
                    if i + 2 < len(data):
                        option = data[i + 2]
                        messages.append({
                            'type': 'negotiation',
                            'direction': direction,
                            'command': decode_command(cmd),
                            'option': get_option_name(option),
                            'raw': data[i:i+3]
                        })
                        i += 3
                    else:
                        i += 2
                elif cmd == SB:
                    # Find the matching SE
                    start = i + 2
                    j = start
                    while j < len(data) - 1:
                        if data[j] == IAC and data[j + 1] == SE:
                            sub_data = data[start:j]
                            messages.append({
                                'type': 'subnegotiation',
                                'direction': direction,
                                'option': get_option_name(sub_data[0]) if sub_data else "Unknown",
                                'data': decode_subnegotiation(sub_data),
                                'raw': data[i:j+2]
                            })
                            i = j + 2
                            break
                        j += 1
                    else:
                        # No SE found, consume to end
                        i = len(data)
                else:
                    messages.append({
                        'type': 'command',
                        'direction': direction,
                        'command': decode_command(cmd),
                        'raw': data[i:i+2]
                    })
                    i += 2
            else:
                i += 1
        else:
            # Regular data
            start = i
            while i < len(data) and data[i] != IAC:
                i += 1
            if i > start:
                text = data[start:i].decode('ascii', errors='replace')
                messages.append({
                    'type': 'data',
                    'direction': direction,
                    'data': repr(text) if any(ord(c) < 32 for c in text) else text,
                    'raw': data[start:i]
                })

    return messages

def send_telnet_command(sock, cmd, option):
    """Send a Telnet command."""
    data = bytes([IAC, cmd, option])
    sock.send(data)
    return data

def send_subnegotiation(sock, option, sub_data):
    """Send a Telnet subnegotiation."""
    data = bytes([IAC, SB, option]) + sub_data + bytes([IAC, SE])
    sock.send(data)
    return data

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 telnet_decoder.py <host> <port>")
        sys.exit(1)

    host = sys.argv[1]
    port = int(sys.argv[2])

    print(f"Connecting to {host}:{port}...")
    print("=" * 60)

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((host, port))

        # Set a timeout for receiving data
        sock.settimeout(1.0)

        # Send initial telnet negotiations
        print("Sending initial telnet negotiations...")
        
        # Request Linemode
        response = send_telnet_command(sock, DO, 34)  # DO Linemode
        print(f"Sent: DO Linemode")
        print(f"Raw: {[hex(b) for b in response]}")

        negotiation_complete = False
        message_count = 0

        while not negotiation_complete:
            try:
                data = sock.recv(4096)
                if not data:
                    break

                messages = parse_telnet_data(data, "Server")

                for msg in messages:
                    message_count += 1
                    print(f"[{message_count:2d}] {msg['direction']}: ", end="")

                    if msg['type'] == 'negotiation':
                        print(f"{msg['command']} {msg['option']}")
                        print(f"      Raw: {[hex(b) for b in msg['raw']]}")

                        # Auto-respond to some common negotiations
                        if msg['command'] == 'DO' and msg['option'] == 'Terminal Type':
                            response = send_telnet_command(sock, WILL, 24)
                            print(f"      Response: WILL Terminal Type")
                            print(f"      Raw: {[hex(b) for b in response]}")

                        elif msg['command'] == 'DO' and msg['option'] == 'Suppress Go Ahead':
                            response = send_telnet_command(sock, WILL, 3)
                            print(f"      Response: WILL Suppress Go Ahead")
                            print(f"      Raw: {[hex(b) for b in response]}")

                        elif msg['command'] == 'DO' and msg['option'] == 'Negotiate About Window Size':
                            response = send_telnet_command(sock, WILL, 31)
                            print(f"      Response: WILL Negotiate About Window Size")
                            print(f"      Raw: {[hex(b) for b in response]}")

                        elif msg['command'] == 'DO' and msg['option'] == 'Linemode':
                            response = send_telnet_command(sock, WILL, 34)
                            print(f"      Response: WILL Linemode")
                            print(f"      Raw: {[hex(b) for b in response]}")

                        elif msg['command'] == 'WILL' and msg['option'] == 'Suppress Go Ahead':
                            # Server is offering Suppress Go Ahead, accept it
                            response = send_telnet_command(sock, DO, 3)
                            print(f"      Response: DO Suppress Go Ahead")
                            print(f"      Raw: {[hex(b) for b in response]}")

                        elif msg['command'] == 'WILL' and msg['option'] == 'Terminal Type':
                            # Server is offering Terminal Type, accept it
                            response = send_telnet_command(sock, DO, 24)
                            print(f"      Response: DO Terminal Type")
                            print(f"      Raw: {[hex(b) for b in response]}")

                        elif msg['command'] == 'WILL' and msg['option'] == 'Negotiate About Window Size':
                            # Server is offering Window Size, accept it
                            response = send_telnet_command(sock, DO, 31)
                            print(f"      Response: DO Negotiate About Window Size")
                            print(f"      Raw: {[hex(b) for b in response]}")

                        elif msg['command'] == 'WILL' and msg['option'] == 'Linemode':
                            # Server is offering Linemode, accept it
                            response = send_telnet_command(sock, DO, 34)
                            print(f"      Response: DO Linemode")
                            print(f"      Raw: {[hex(b) for b in response]}")

                    elif msg['type'] == 'subnegotiation':
                        print(f"Subnegotiation: {msg['data']}")
                        print(f"      Raw: {[hex(b) for b in msg['raw']]}")

                        # Respond to terminal type requests
                        if "Terminal Type SEND" in msg['data']:
                            # Send terminal type
                            sub_data = bytes([0]) + b"xterm"  # IS command + terminal type
                            response = send_subnegotiation(sock, 24, sub_data)
                            print(f"      Response: Terminal Type IS xterm")
                            print(f"      Raw: {[hex(b) for b in response]}")

                    elif msg['type'] == 'command':
                        print(f"{msg['command']}")
                        print(f"      Raw: {[hex(b) for b in msg['raw']]}")

                    elif msg['type'] == 'data':
                        if "Welcome" in msg['data'] or "login:" in msg['data'].lower():
                            negotiation_complete = True
                        print(f"Data: {msg['data'][:100]}{'...' if len(msg['data']) > 100 else ''}")

            except socket.timeout:
                # Check if we've received enough to consider negotiation done
                if message_count > 5:
                    negotiation_complete = True
                continue

        print("=" * 60)
        print(f"Telnet negotiation decoding complete. Received {message_count} messages.")

        # Close the connection
        sock.close()

    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()