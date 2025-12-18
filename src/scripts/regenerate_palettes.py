#!/usr/bin/env python3
"""
Extract color palettes from vt100.c and regenerate with pre-computed LAB values.
"""

import re
import math

def rgb_to_cielab(r, g, b):
    """Convert RGB to CIELAB coordinates."""
    r = r / 255.0
    g = g / 255.0
    b = b / 255.0
    
    r = r / 12.92 if r <= 0.04045 else math.pow((r + 0.055) / 1.055, 2.4)
    g = g / 12.92 if g <= 0.04045 else math.pow((g + 0.055) / 1.055, 2.4)
    b = b / 12.92 if b <= 0.04045 else math.pow((b + 0.055) / 1.055, 2.4)
    
    x = r * 0.4124564 + g * 0.3575761 + b * 0.1804375
    y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750
    z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041
    
    ref_x, ref_y, ref_z = 0.95047, 1.00000, 1.08883
    x, y, z = x / ref_x, y / ref_y, z / ref_z
    
    epsilon = 0.008856
    kappa = 903.3
    
    fx = x**(1.0/3.0) if x > epsilon else (kappa * x + 16.0) / 116.0
    fy = y**(1.0/3.0) if y > epsilon else (kappa * y + 16.0) / 116.0
    fz = z**(1.0/3.0) if z > epsilon else (kappa * z + 16.0) / 116.0
    
    l = 116.0 * fy - 16.0
    a = 500.0 * (fx - fy)
    b_val = 200.0 * (fy - fz)
    
    return (l, a, b_val)

def extract_colors_from_palette(palette_text):
    """Extract color entries from palette definition."""
    colors = []
    # Match entries like: {(char *)"name", {r, g, b}}, /* comment */
    pattern = r'\{\\(char \*)\"([^\"]+)\",\s*\{(\d+),\s*(\d+),\s*(\d+)\}\}'
    for match in re.finditer(pattern, palette_text):
        name = match.group(1)
        r, g, b = int(match.group(2)), int(match.group(3)), int(match.group(4))
        colors.append((name, r, g, b))
    return colors

def generate_palette_with_lab(colors, include_terminator=True):
    """Generate palette definition with embedded LAB values."""
    lines = []
    for name, r, g, b in colors:
        l, a, b_val = rgb_to_cielab(r, g, b)
        line = f'    {{(char *)"{name}", {{{r}, {g}, {b}}}, {{{l:.1f}f, {a:.1f}f, {b_val:.1f}f}}}}'
        lines.append(line)
    
    if include_terminator:
        lines.append('    {(char *)NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}}')
    
    return ',\n'.join(lines) + '};\n'

# Read current vt100.c
with open('/home/eddy/source/TinyMUSH/netmush/vt100.c', 'r') as f:
    content = f.read()

# Find all palette blocks and extract their text before the NULL terminator
def extract_palette_block(content, palette_name):
    """Extract raw palette block text."""
    # Find the palette start
    start_pattern = f'COLORINFO {palette_name}\\[\\] = {{(.*?){{\\(char \\*\\)NULL'
    match = re.search(start_pattern, content, re.DOTALL)
    if match:
        return match.group(1)
    return None

# Extract ANSI colors
ansi_block = extract_palette_block(content, 'ansiColor')
if ansi_block:
    ansi_colors = extract_colors_from_palette(ansi_block)
    print(f"Found {len(ansi_colors)} ANSI colors")
    print(f"  First: {ansi_colors[0]}")
    print(f"  Last: {ansi_colors[-1]}")

# Extract Xterm colors
xterm_block = extract_palette_block(content, 'xtermColor')
if xterm_block:
    xterm_colors = extract_colors_from_palette(xterm_block)
    print(f"Found {len(xterm_colors)} Xterm colors")
    print(f"  First: {xterm_colors[0]}")
    print(f"  Last: {xterm_colors[-1]}")

# Extract CSS colors
css_block = extract_palette_block(content, 'cssColors')
if css_block:
    css_colors = extract_colors_from_palette(css_block)
    print(f"Found {len(css_colors)} CSS colors")
    print(f"  First: {css_colors[0]}")
    print(f"  Last: {css_colors[-1]}")

print("\n✓ Extraction complete")
