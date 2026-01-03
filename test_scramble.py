#!/usr/bin/env python3

import subprocess
import sys

def test_scramble():
    """Test that scramble() preserves ANSI colors"""
    
    # Start netmush in debug mode
    proc = subprocess.Popen(
        ['./game/netmush', '-d'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True
    )
    
    try:
        # Send test command
        test_input = 'think scramble(%xrRed%xn)\nQUIT\n'
        stdout, stderr = proc.communicate(test_input, timeout=5)
        
        print("=== Output ===")
        print(stdout)
        print("=== Stderr ===")
        print(stderr)
        
        # Check if ANSI codes are present
        if '\x1b[' in stdout:
            print("\n✓ ANSI codes found in output")
            return True
        else:
            print("\n✗ No ANSI codes in output")
            return False
            
    except subprocess.TimeoutExpired:
        proc.kill()
        print("Process timeout")
        return False
    except Exception as e:
        print(f"Error: {e}")
        return False

if __name__ == '__main__':
    success = test_scramble()
    sys.exit(0 if success else 1)
