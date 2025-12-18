#!/usr/bin/env python3
"""
Compute CIELAB coordinates for all color palettes.
Generates C code with pre-computed LAB values.
"""

import math
import sys
import re

def rgb_to_xyz(r, g, b):
    """Convert RGB to XYZ coordinates."""
    r = r / 255.0
    g = g / 255.0
    b = b / 255.0
    
    # Apply gamma correction (sRGB)
    r = r / 12.92 if r <= 0.04045 else math.pow((r + 0.055) / 1.055, 2.4)
    g = g / 12.92 if g <= 0.04045 else math.pow((g + 0.055) / 1.055, 2.4)
    b = b / 12.92 if b <= 0.04045 else math.pow((b + 0.055) / 1.055, 2.4)
    
    # Standard D65 illuminant transformation
    x = r * 0.4124564 + g * 0.3575761 + b * 0.1804375
    y = r * 0.2126729 + g * 0.7151522 + b * 0.0721750
    z = r * 0.0193339 + g * 0.1191920 + b * 0.9503041
    
    return (x, y, z)

def xyz_to_cielab(x, y, z):
    """Convert XYZ to CIELAB coordinates."""
    # D65 standard illuminant reference
    ref_x = 0.95047
    ref_y = 1.00000
    ref_z = 1.08883
    
    x = x / ref_x
    y = y / ref_y
    z = z / ref_z
    
    # Apply nonlinear transformation
    epsilon = 0.008856
    kappa = 903.3
    
    fx = x**(1.0/3.0) if x > epsilon else (kappa * x + 16.0) / 116.0
    fy = y**(1.0/3.0) if y > epsilon else (kappa * y + 16.0) / 116.0
    fz = z**(1.0/3.0) if z > epsilon else (kappa * z + 16.0) / 116.0
    
    l = 116.0 * fy - 16.0
    a = 500.0 * (fx - fy)
    b = 200.0 * (fy - fz)
    
    return (l, a, b)

def rgb_to_cielab(r, g, b):
    """Direct RGB to CIELAB conversion."""
    x, y, z = rgb_to_xyz(r, g, b)
    return xyz_to_cielab(x, y, z)

def generate_colorinfo_with_lab(palette_name, colors):
    """Generate C code for a color palette with embedded LAB values."""
    output = []
    
    for name, rgb in colors:
        if name is None:
            # Terminator entry
            output.append('    {(char *)NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}}')
        else:
            r, g, b = rgb
            l, a, b_val = rgb_to_cielab(r, g, b)
            output.append(
                f'    {{(char *)"{name}", {{{r}, {g}, {b}}}, {{{l:.1f}f, {a:.1f}f, {b_val:.1f}f}}}}'
            )
    
    return ',\n'.join(output)

# Define color palettes
ansi_colors = [
    ("black", (0, 0, 0)),
    ("maroon", (128, 0, 0)),
    ("green", (0, 128, 0)),
    ("olive", (128, 128, 0)),
    ("navy", (0, 0, 128)),
    ("purple", (128, 0, 128)),
    ("teal", (0, 128, 128)),
    ("silver", (192, 192, 192)),
    ("grey", (128, 128, 128)),
    ("red", (255, 0, 0)),
    ("lime", (0, 255, 0)),
    ("yellow", (255, 255, 0)),
    ("blue", (0, 0, 255)),
    ("fuchsia", (255, 0, 255)),
    ("aqua", (0, 255, 255)),
    ("white", (255, 255, 255)),
    (None, (0, 0, 0))  # Terminator
]

# For xterm and css colors, we'd read from vt100.c
# For now, just show how the format works
print("Example ANSI Color Palette with LAB:")
print("=" * 80)
print("COLORINFO ansiColor[] = {")
print(generate_colorinfo_with_lab("ansiColor", ansi_colors))
print("};")
