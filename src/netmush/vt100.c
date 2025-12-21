/**
 * @file vt100.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief VT100/ANSI terminal control helpers and telnet negotiation support
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>

#define COLOR_PALETTE_CACHE_MAX 256
#define SGR_CODES_MAX 32

// Forward declaration of unified color palette
extern COLORINFO colorPalette[];

// ANSI 16-color lookup table (0-15)
static const rgbColor ANSI16[16] = {
    {0, 0, 0},       // 0: black
    {128, 0, 0},     // 1: red
    {0, 128, 0},     // 2: green
    {128, 128, 0},   // 3: yellow
    {0, 0, 128},     // 4: blue
    {128, 0, 128},   // 5: magenta
    {0, 128, 128},   // 6: cyan
    {192, 192, 192}, // 7: light gray
    {128, 128, 128}, // 8: dark gray
    {255, 0, 0},     // 9: bright red
    {0, 255, 0},     // 10: bright green
    {255, 255, 0},   // 11: bright yellow
    {0, 0, 255},     // 12: bright blue
    {255, 0, 255},   // 13: bright magenta
    {0, 255, 255},   // 14: bright cyan
    {255, 255, 255}  // 15: white
};

// Simple LRU cache for color matches
typedef struct
{
    uint32_t key; // packed rgb + schemes
    COLORMATCH match;
    uint32_t tick;
    bool valid;
} ColorCacheEntry;

static ColorCacheEntry g_colorCache[COLOR_PALETTE_CACHE_MAX];
static uint32_t g_colorCacheTick = 0;

/**
 * @brief Convert RGB color to XYZ coordonates
 *
 * The CIE 1931 color spaces are the first defined quantitative links between
 * distributions of wavelengths in the electromagnetic visible spectrum, and
 * physiologically perceived colors in human color vision. The mathematical
 * relationships that define these color spaces are essential tools for color
 * management, important when dealing with color inks, illuminated displays,
 * and recording devices such as digital cameras. The system was designed in
 * 1931 by the "Commission Internationale de l'éclairage", known in English
 * as the International Commission on Illumination.
 *
 * @param c RGB Color to conver
 * @return xyzColor XYZ Coordinates
 */
xyzColor rgbToXyz(rgbColor rgb)
{
    xyzColor xyz;

    float r = (rgb.r / 255.0);
    float g = (rgb.g / 255.0);
    float b = (rgb.b / 255.0);

    if (r > 0.04045)
    {
        r = (powf(((r + 0.055) / 1.055), 2.4)) * 100.0;
    }
    else
    {
        r = (r / 12.92) * 100.0;
    }

    if (g > 0.04045)
    {
        g = (powf(((g + 0.055) / 1.055), 2.4)) * 100.0;
    }
    else
    {
        g = (g / 12.92) * 100.0;
    }

    if (b > 0.04045)
    {
        b = (powf(((b + 0.055) / 1.055), 2.4)) * 100.0;
    }
    else
    {
        b = (b / 12.92) * 100.0;
    }

    xyz.x = r * 0.4124 + g * 0.3576 + b * 0.1805;
    xyz.y = r * 0.2126 + g * 0.7152 + b * 0.0722;
    xyz.z = r * 0.0193 + g * 0.1192 + b * 0.9505;

    return xyz;
}

/**
 * @brief Convert XYZ coordinates to CIELAB Color Space coordinates
 *
 * The CIELAB color space also referred to as L*a*b* is a color space defined
 * by the International Commission on Illumination (abbreviated CIE) in 1976.
 * (Referring to CIELAB as "Lab" without asterisks should be avoided to prevent
 * confusion with Hunter Lab.) It expresses color as three values: L* for
 * perceptual lightness, and a* and b* for the four unique colors of human
 * vision: red, green, blue, and yellow. CIELAB was intended as a perceptually
 * uniform space, where a given numerical change corresponds to similar
 * perceived change in color. While the LAB space is not truly perceptually
 * uniform, it nevertheless is useful in industry for detecting small
 * differences in color.
 * @param c
 * @return CIELABColorSpace
 */
CIELABColor xyzToCIELAB(xyzColor xyz)
{

    CIELABColor lab;

    float x = (xyz.x / 95.047);
    float y = (xyz.y / 100.0);
    float z = (xyz.z / 108.883);

    if (x > 0.008856)
    {
        x = powf(x, 1 / 3.0);
    }

    else
    {
        x = (7.787 * x) + (16.0 / 116.0);
    }

    if (y > 0.008856)
    {
        y = powf(y, 1 / 3.0);
    }
    else
    {
        y = (7.787 * y) + (16.0 / 116.0);
    }

    if (z > 0.008856)
    {
        z = powf(z, 1 / 3.0);
    }
    else
    {
        z = (7.787 * z) + (16.0 / 116.0);
    }

    lab.l = 116 * y - 16;
    lab.a = 500 * (x - y);
    lab.b = 200 * (y - z);

    return lab;
}

/**
 * @brief Find the closest color in a palette.
 *
 * @param rgb
 * @param palette
 * @return COLORMATCH
 */
COLORMATCH getColorMatch(rgbColor rgb, uint8_t schemes)
{
    // TODO: Future optimization - implement LRU cache using COLOR_PALETTE_CACHE_MAX (256 entries)
    // This would cache frequently requested RGB→COLORINFO mappings to avoid repeated deltaE calculations
    COLORMATCH cm = {101, {NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, 0}};

    // Treat 'no scheme specified' as 'all schemes'
    if (schemes == 0)
        schemes = COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS;

    // Pre-compute target color in CIELAB space (perceptually uniform distance metric)
    CIELABColor labTarget = xyzToCIELAB(rgbToXyz(rgb));

    // LRU cache lookup
    uint32_t key = (uint32_t)rgb.r | ((uint32_t)rgb.g << 8) | ((uint32_t)rgb.b << 16) | ((uint32_t)schemes << 24);
    uint32_t now = ++g_colorCacheTick;
    int evictIdx = -1;
    uint32_t oldestTick = UINT32_MAX;
    for (int i = 0; i < COLOR_PALETTE_CACHE_MAX; i++)
    {
        if (!g_colorCache[i].valid)
        {
            evictIdx = i; // Prefer first invalid slot
            oldestTick = 0;
            break;
        }
        if (g_colorCache[i].key == key)
        {
            g_colorCache[i].tick = now;
            return g_colorCache[i].match;
        }
        if (g_colorCache[i].tick < oldestTick)
        {
            oldestTick = g_colorCache[i].tick;
            evictIdx = i;
        }
    }

    // Track best squared distance to avoid powf/sqrtf in the loop
    float bestDist2 = cm.deltaE * cm.deltaE; // 101^2 sentinel
    const COLORINFO *best = NULL;

    // O(n) linear search through palette with scheme filtering
    for (const COLORINFO *cinfo = colorPalette; cinfo->name != NULL; cinfo++)
    {
        if ((cinfo->schemes & schemes) == 0)
            continue;

        float dL = labTarget.l - cinfo->lab.l;
        float dA = labTarget.a - cinfo->lab.a;
        float dB = labTarget.b - cinfo->lab.b;
        float dist2 = dL * dL + dA * dA + dB * dB;

        if (dist2 == 0.0f)
        {
            return (COLORMATCH){0.0f, *cinfo};
        }
        if (dist2 < bestDist2)
        {
            bestDist2 = dist2;
            best = cinfo;
        }
    }

    if (best != NULL)
    {
        cm.deltaE = sqrtf(bestDist2);
        cm.color = *best;
        // Populate cache
        if (evictIdx >= 0)
        {
            g_colorCache[evictIdx].key = key;
            g_colorCache[evictIdx].match = cm;
            g_colorCache[evictIdx].tick = now;
            g_colorCache[evictIdx].valid = true;
        }
    }

    return cm;
}

/**
 * @brief Format RGB color to VT100 color sequence.
 *
 * @param rgb           RGB Color to format
 * @param background    True if background color
 * @return char*        VT100 color sequence
 */
char *TrueColor2VT100(rgbColor rgb, bool background)
{
    char *vt = NULL;

    XASPRINTF(vt, "\033[%d;2;%d;%d;%dm", background ? 48 : 38, rgb.r, rgb.g, rgb.b);

    return (vt);
}

/**
 * @brief Format X11 color to VT100 color sequence.
 *
 * @param color         X11 color code
 * @param background    True if background color
 * @return char*        VT100 color sequence
 */
char *X112VT100(uint8_t color, bool background)
{
    char *vt = NULL;

    XASPRINTF(vt, "\033[%d;5;%dm", background ? 48 : 38, color);

    return (vt);
}

/**
 * @brief Format ANSI color to VT100 color sequence.
 *
 * @param color         ANSI color code
 * @param background    True if background color
 * @return char*        VT100 color sequence
 */
char *Ansi2VT100(uint8_t color, bool background)
{
    char *vt = NULL;

    XASPRINTF(vt, "\033[%dm", (background ? 40 : 30) + (color & 7) + (color > 7 ? 60 : 0));

    return (vt);
}

/**
 * @brief Convert X11 color code to RGB value.
 *
 * @param color         X11 color code
 * @return rgbColor     RGB Color
 */
rgbColor X112RGB(int color)
{
    rgbColor rgb = {0, 0, 0};

    // Bounds checking for invalid color codes
    if (color < 0 || color > 255)
    {
        return rgb; // Return black for invalid colors
    }

    if (color < 16)
    {
        return ANSI16[color];
    }
    else if ((color >= 16) && (color <= 231))
    {
        int r = ((color - 16) / 36);
        int g = (((color - 16) % 36) / 6);
        int b = ((color - 16) % 6);

        rgb.r = r > 0 ? 55 + r * 40 : 0;
        rgb.g = g > 0 ? 55 + g * 40 : 0;
        rgb.b = b > 0 ? 55 + b * 40 : 0;
    }
    else if (color >= 232)
    {
        rgb.r = rgb.g = rgb.b = (color - 232) * 10 + 8;
    }
    return rgb;
}

/**
 * @brief Convert RGB values to X11 color code.
 *
 * @param rgb       RGB Color
 * @return uint8_t  X11 color code
 */
uint8_t RGB2X11(rgbColor rgb)
{
    // Pure black
    if (rgb.r + rgb.g + rgb.b == 0)
        return 0;

    // Default gray (192,192,192) - CHECK BEFORE bit-shifting!
    if (rgb.r == 192 && rgb.g == 192 && rgb.b == 192)
        return 7;

    // Bright standard colors (matching 0-15 bright variants) - CHECK BEFORE bit-shifting!
    if ((rgb.r == 255 || rgb.r == 0) && (rgb.g == 255 || rgb.g == 0) && (rgb.b == 255 || rgb.b == 0))
    {
        return (rgb.r & 1) + (rgb.g & 2) + (rgb.b & 4) + 8;
    }

    // Standard ANSI colors (0-15) - bit-shifting approach for other dark colors
    if ((rgb.r & 128) && (rgb.g & 128) && (rgb.b & 128))
    {
        uint8_t c = (rgb.r >> 7) + (rgb.g >> 6) + (rgb.b >> 5);
        return c == 7 ? 8 : c;
    }

    // Grayscale (232-255)
    if (rgb.r == rgb.g && rgb.r == rgb.b)
    {
        int level = (rgb.r - 8) / 10;

        if (level < 0)
            level = 0;
        else if (level > 23)
            level = 23;

        return (uint8_t)(level + 232);
    }

    // 216-color cube (16-231)
    {
        int xr = (rgb.r - 55) / 40;
        int xg = (rgb.g - 55) / 40;
        int xb = (rgb.b - 55) / 40;

        // Clamp to valid range
        if (xr < 0)
            xr = 0;
        if (xg < 0)
            xg = 0;
        if (xb < 0)
            xb = 0;

        return (uint8_t)(xr * 36 + xg * 6 + xb + 16);
    }
}

/**
 * @brief Convert X11 color code to RGB value.
 *
 * @param color         ANSI color code
 * @return rgbColor     RGB Color
 */
uint8_t X112Ansi(int color)
{
    if (color >= 0 && color < 16)
        return (uint8_t)color; /* Direct ANSI */
    if (color >= 16 && color < 232)
    {
        /* Xterm to ANSI: search first 8 colors for best match */
        rgbColor rgb = X112RGB(color);
        COLORMATCH cm = getColorMatch(rgb, COLOR_SCHEME_ANSI);
        return cm.color.name != NULL ? RGB2X11(rgb) & 0xF : 0;
    }
    if (color >= 232)
    {
        /* Grayscale to ANSI: map to dark (0) or bright white (7/15) */
        return color >= 244 ? 7 : 0;
    }
    return 0;
}

/**
 * @brief Convert RGB values to ANSI color code.
 *
 * @param rgb       RGB Color
 * @return uint8_t  ANSI color code
 */
uint8_t RGB2Ansi(rgbColor rgb)
{
    COLORMATCH cm = getColorMatch(rgb, COLOR_SCHEME_ANSI);

    if (cm.color.name == NULL)
    {
        return 0;
    }

    if (cm.color.rgb.r + cm.color.rgb.g + cm.color.rgb.b == 0)
    {
        return 0;
    }
    else if ((cm.color.rgb.r < 128) && (cm.color.rgb.g < 128) && (cm.color.rgb.b < 128))
    {
        uint8_t c = (cm.color.rgb.r >> 7) + (cm.color.rgb.g >> 6) + (cm.color.rgb.b >> 5);
        return c == 7 ? 8 : c;
    }
    else if ((cm.color.rgb.r == 192) && (cm.color.rgb.g == 192) && (cm.color.rgb.b == 192))
    {
        return 7;
    }
    else if ((cm.color.rgb.r == 255 || !cm.color.rgb.r) && (cm.color.rgb.g == 255 || !cm.color.rgb.g) && (cm.color.rgb.b == 255 || !cm.color.rgb.b))
    {
        return (cm.color.rgb.r & 1) + (cm.color.rgb.g & 2) + (cm.color.rgb.b & 4) + 8;
    }

    /* For other bright colors, prefer bright variants (8-15) for better contrast on dark backgrounds */
    uint8_t x11_color = RGB2X11(cm.color.rgb);
    uint8_t dark_base = x11_color & 0xF;
    
    /* If the color is bright (high luminance), use bright variant */
    uint16_t brightness = (uint16_t)cm.color.rgb.r + (uint16_t)cm.color.rgb.g + (uint16_t)cm.color.rgb.b;
    if (brightness > 384)  /* Sum > 3 * 128 midpoint */
    {
        return dark_base == 0 ? 8 : dark_base + 8;
    }
    
    return dark_base;
}

VT100ATTR decodeVT100(const char **ansi)
{
    int codes[SGR_CODES_MAX];
    int index = 0;
    bool has_m = false;
    VT100ATTR attr = {{ANSICOLORTYPE_NONE, {0, 0, 0}}, {ANSICOLORTYPE_NONE, {0, 0, 0}}, false};

    if (ansi == NULL || *ansi == NULL || **ansi != ESC_CHAR)
    {
        return attr;
    }

    (*ansi)++;

    if (**ansi == '[')
    {
        (*ansi)++;
        if (isdigit(**ansi))
        {
            while ((isdigit(**ansi) || (**ansi) == ';') && (index < SGR_CODES_MAX))
            {
                if (isdigit(**ansi))
                {
                    codes[index] = 0;
                    while (isdigit(**ansi))
                    {
                        codes[index] = codes[index] * 10 + (**ansi - '0');
                        (*ansi)++;
                    }
                    index++;
                    if (**ansi == ';')
                    {
                        (*ansi)++;
                    }
                }
                else
                {
                    /* Skip empty field (e.g., consecutive semicolons) */
                    (*ansi)++;
                }
            }
        }
        if (**ansi == 'm')
        {
            (*ansi)++;
            has_m = true;
        }
    }

    if (has_m && index > 0)
    {
        for (size_t i = 0; i < (size_t)index; i++)
        {
            uint8_t code = (uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i]));
            switch (code)
            {
            case 0:
                attr.reset = true;
                break;
            case 38:
                if (++i >= (size_t)index)
                    break;
                switch ((uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i])))
                {
                case 2:
                    if (++i >= (size_t)index)
                        break;
                    attr.foreground.type = ANSICOLORTYPE_TRUECOLORS;
                    attr.foreground.rgb.r = (uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i]));
                    if (++i >= (size_t)index)
                        break;
                    attr.foreground.rgb.g = (uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i]));
                    if (++i >= (size_t)index)
                        break;
                    attr.foreground.rgb.b = (uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i]));
                    break;
                case 5:
                    if (++i >= (size_t)index)
                        break;
                    if (codes[i] >= 0 && codes[i] <= 255)
                    {
                        attr.foreground.type = ANSICOLORTYPE_XTERM;
                        attr.foreground.rgb = X112RGB((uint8_t)codes[i]);
                    }
                    break;
                }
                break;
            case 48:
                if (++i >= (size_t)index)
                    break;
                switch ((uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i])))
                {
                case 2:
                    if (++i >= (size_t)index)
                        break;
                    attr.background.type = ANSICOLORTYPE_TRUECOLORS;
                    attr.background.rgb.r = (uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i]));
                    if (++i >= (size_t)index)
                        break;
                    attr.background.rgb.g = (uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i]));
                    if (++i >= (size_t)index)
                        break;
                    attr.background.rgb.b = (uint8_t)(codes[i] < 0 ? 0 : (codes[i] > 255 ? 255 : codes[i]));
                    break;
                case 5:
                    if (++i >= (size_t)index)
                        break;
                    if (codes[i] >= 0 && codes[i] <= 255)
                    {
                        attr.background.type = ANSICOLORTYPE_XTERM;
                        attr.background.rgb = X112RGB((uint8_t)codes[i]);
                    }
                    break;
                }
                break;
            default:
                if (code >= 30 && code <= 37)
                {
                    attr.foreground.type = ANSICOLORTYPE_STANDARD;
                    attr.foreground.rgb = X112RGB(code - 30);
                }
                else if (code >= 40 && code <= 47)
                {
                    attr.background.type = ANSICOLORTYPE_STANDARD;
                    attr.background.rgb = X112RGB(code - 40);
                }
                else if (code >= 90 && code <= 97)
                {
                    attr.foreground.type = ANSICOLORTYPE_STANDARD;
                    attr.foreground.rgb = X112RGB(code - 90 + 8);
                }
                else if (code >= 100 && code <= 107)
                {
                    attr.background.type = ANSICOLORTYPE_STANDARD;
                    attr.background.rgb = X112RGB(code - 100 + 8);
                }
                break;
            }
        }
    }
    return attr;
}

// Unified color palette containing all ANSI, Xterm, and CSS colors with scheme flags
// COLOR_SCHEME_ANSI = 0x01, COLOR_SCHEME_XTERM = 0x02, COLOR_SCHEME_CSS = 0x04
COLORINFO colorPalette[] = {
    {(char *)"aliceblue", {240, 248, 255}, {97.2f, -1.3f, -4.3f}, COLOR_SCHEME_CSS},
    {(char *)"antiquewhite", {250, 235, 215}, {93.7f, 1.8f, 11.5f}, COLOR_SCHEME_CSS},
    {(char *)"aqua", {0, 255, 255}, {91.1f, -48.1f, -14.1f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"aquamarine", {127, 255, 212}, {92.0f, -45.5f, 9.7f}, COLOR_SCHEME_CSS},
    {(char *)"azure", {240, 255, 255}, {98.9f, -4.9f, -1.7f}, COLOR_SCHEME_CSS},
    {(char *)"beige", {245, 245, 220}, {95.9f, -4.2f, 12.0f}, COLOR_SCHEME_CSS},
    {(char *)"bisque", {255, 228, 196}, {92.0f, 4.4f, 19.0f}, COLOR_SCHEME_CSS},
    {(char *)"black", {0, 0, 0}, {0.0f, 0.0f, 0.0f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"blanchedalmond", {255, 235, 205}, {93.9f, 2.1f, 17.0f}, COLOR_SCHEME_CSS},
    {(char *)"blue", {0, 0, 255}, {32.3f, 79.2f, -107.9f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"blueviolet", {138, 43, 226}, {42.2f, 69.8f, -74.8f}, COLOR_SCHEME_CSS},
    {(char *)"brown", {165, 42, 42}, {37.5f, 49.7f, 30.5f}, COLOR_SCHEME_CSS},
    {(char *)"burlywood", {222, 184, 135}, {77.0f, 7.0f, 30.0f}, COLOR_SCHEME_CSS},
    {(char *)"cadetblue", {95, 158, 160}, {61.2f, -19.7f, -7.4f}, COLOR_SCHEME_CSS},
    {(char *)"chartreuse", {127, 255, 0}, {89.9f, -68.1f, 85.8f}, COLOR_SCHEME_CSS},
    {(char *)"chocolate", {210, 105, 30}, {56.0f, 37.1f, 56.7f}, COLOR_SCHEME_CSS},
    {(char *)"color100", {102, 102, 0}, {41.7f, -11.0f, 48.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color101", {102, 102, 51}, {42.1f, -8.1f, 28.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color102", {102, 102, 102}, {43.2f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color103", {102, 102, 153}, {45.1f, 12.5f, -27.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color104", {102, 102, 204}, {47.9f, 27.6f, -53.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color105", {102, 102, 255}, {51.3f, 43.6f, -76.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color106", {102, 153, 0}, {57.7f, -36.5f, 60.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color107", {102, 153, 51}, {57.9f, -34.2f, 46.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color108", {102, 153, 102}, {58.6f, -27.7f, 21.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color109", {102, 153, 153}, {59.8f, -16.9f, -5.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color110", {102, 153, 204}, {61.6f, -2.8f, -31.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color111", {102, 153, 255}, {64.0f, 13.2f, -55.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color112", {102, 204, 0}, {73.6f, -57.0f, 72.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color113", {102, 204, 51}, {73.7f, -55.3f, 62.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color114", {102, 204, 102}, {74.2f, -50.2f, 41.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color115", {102, 204, 153}, {75.0f, -41.5f, 16.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color116", {102, 204, 204}, {76.3f, -29.6f, -9.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color117", {102, 204, 255}, {78.0f, -15.2f, -33.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color118", {102, 255, 0}, {89.1f, -74.4f, 84.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color119", {102, 255, 51}, {89.2f, -73.1f, 77.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color120", {102, 255, 102}, {89.5f, -69.2f, 59.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color121", {102, 255, 153}, {90.2f, -62.2f, 36.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color122", {102, 255, 204}, {91.1f, -52.3f, 12.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color123", {102, 255, 255}, {92.4f, -40.0f, -12.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color124", {153, 0, 0}, {31.3f, 54.7f, 45.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color125", {153, 0, 51}, {31.8f, 56.2f, 17.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color126", {153, 0, 102}, {33.4f, 60.4f, -13.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color127", {153, 0, 153}, {36.1f, 67.1f, -41.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color128", {153, 0, 204}, {39.8f, 75.6f, -65.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color129", {153, 0, 255}, {44.2f, 85.3f, -87.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color130", {153, 51, 0}, {36.3f, 40.8f, 48.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color131", {153, 51, 51}, {36.7f, 42.5f, 23.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color132", {153, 51, 102}, {38.0f, 47.3f, -6.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color133", {153, 51, 153}, {40.3f, 55.0f, -34.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color134", {153, 51, 204}, {43.5f, 64.8f, -59.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color135", {153, 51, 255}, {47.4f, 75.7f, -82.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color136", {153, 102, 0}, {47.3f, 13.5f, 54.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color137", {153, 102, 51}, {47.6f, 15.4f, 36.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color138", {153, 102, 102}, {48.6f, 20.6f, 8.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color139", {153, 102, 153}, {50.2f, 29.1f, -19.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color140", {153, 102, 204}, {52.5f, 40.3f, -45.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color141", {153, 102, 255}, {55.6f, 53.0f, -69.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color142", {153, 153, 0}, {61.3f, -14.7f, 64.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color143", {153, 153, 51}, {61.5f, -13.1f, 51.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color144", {153, 153, 102}, {62.1f, -8.2f, 26.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color145", {153, 153, 153}, {63.2f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color146", {153, 153, 204}, {64.9f, 11.1f, -26.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color147", {153, 153, 255}, {67.1f, 24.3f, -50.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color148", {153, 204, 0}, {76.0f, -39.3f, 75.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color149", {153, 204, 51}, {76.2f, -37.9f, 65.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color150", {153, 204, 102}, {76.6f, -33.8f, 45.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color151", {153, 204, 153}, {77.4f, -26.6f, 20.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color152", {153, 204, 204}, {78.6f, -16.5f, -5.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color153", {153, 204, 255}, {80.3f, -4.0f, -30.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color154", {153, 255, 0}, {90.9f, -60.2f, 87.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color155", {153, 255, 51}, {91.0f, -59.0f, 79.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color156", {153, 255, 102}, {91.3f, -55.6f, 62.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color157", {153, 255, 153}, {91.9f, -49.5f, 39.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color158", {153, 255, 204}, {92.9f, -40.8f, 15.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color159", {153, 255, 255}, {94.1f, -29.7f, -9.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color160", {204, 0, 0}, {42.5f, 67.7f, 56.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color161", {204, 0, 51}, {42.9f, 68.7f, 33.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color162", {204, 0, 102}, {43.9f, 71.5f, 2.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color163", {204, 0, 153}, {45.8f, 76.4f, -25.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color164", {204, 0, 204}, {48.5f, 83.0f, -51.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color165", {204, 0, 255}, {51.9f, 91.0f, -74.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color166", {204, 51, 0}, {45.9f, 58.1f, 58.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color167", {204, 51, 51}, {46.2f, 59.1f, 37.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color168", {204, 51, 102}, {47.2f, 62.2f, 7.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color169", {204, 51, 153}, {48.9f, 67.5f, -20.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color17", {0, 0, 51}, {2.2f, 15.2f, -30.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color170", {204, 51, 204}, {51.3f, 74.8f, -46.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color171", {204, 51, 255}, {54.5f, 83.4f, -70.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color172", {204, 102, 0}, {54.4f, 35.7f, 62.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color173", {204, 102, 51}, {54.6f, 36.9f, 46.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color174", {204, 102, 102}, {55.4f, 40.3f, 18.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color175", {204, 102, 153}, {56.7f, 46.3f, -9.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color176", {204, 102, 204}, {58.7f, 54.4f, -35.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color177", {204, 102, 255}, {61.3f, 64.3f, -59.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color178", {204, 153, 0}, {66.2f, 8.3f, 70.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color179", {204, 153, 51}, {66.4f, 9.5f, 57.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color18", {0, 0, 102}, {8.6f, 40.4f, -55.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color180", {204, 153, 102}, {67.0f, 13.0f, 34.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color181", {204, 153, 153}, {68.0f, 19.1f, 7.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color182", {204, 153, 204}, {69.5f, 27.6f, -18.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color183", {204, 153, 255}, {71.4f, 38.1f, -43.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color184", {204, 204, 0}, {79.6f, -18.2f, 79.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color185", {204, 204, 51}, {79.8f, -17.1f, 70.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color186", {204, 204, 102}, {80.2f, -13.9f, 50.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color187", {204, 204, 153}, {80.9f, -8.2f, 25.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color188", {204, 204, 204}, {82.0f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color189", {204, 204, 255}, {83.6f, 10.3f, -24.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color19", {0, 0, 153}, {17.0f, 54.1f, -73.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color190", {204, 255, 0}, {93.6f, -41.9f, 90.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color191", {204, 255, 51}, {93.7f, -41.0f, 83.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color192", {204, 255, 102}, {94.0f, -38.1f, 66.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color193", {204, 255, 153}, {94.6f, -33.0f, 43.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color194", {204, 255, 204}, {95.5f, -25.6f, 19.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color195", {204, 255, 255}, {96.6f, -16.0f, -5.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color197", {255, 0, 51}, {53.5f, 80.8f, 47.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color198", {255, 0, 102}, {54.3f, 82.9f, 18.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color199", {255, 0, 153}, {55.7f, 86.5f, -9.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color20", {0, 0, 204}, {24.8f, 66.9f, -91.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color200", {255, 0, 204}, {57.7f, 91.7f, -36.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color202", {255, 51, 0}, {55.7f, 73.0f, 68.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color203", {255, 51, 51}, {56.0f, 73.7f, 50.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color204", {255, 51, 102}, {56.7f, 75.9f, 22.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color205", {255, 51, 153}, {58.0f, 79.8f, -6.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color206", {255, 51, 204}, {59.9f, 85.3f, -32.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color207", {255, 51, 255}, {62.4f, 92.2f, -57.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color208", {255, 102, 0}, {62.3f, 55.0f, 71.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color209", {255, 102, 51}, {62.5f, 55.8f, 56.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color210", {255, 102, 102}, {63.1f, 58.2f, 30.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color211", {255, 102, 153}, {64.2f, 62.5f, 2.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color212", {255, 102, 204}, {65.8f, 68.6f, -23.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color213", {255, 102, 255}, {68.0f, 76.2f, -48.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color214", {255, 153, 0}, {72.3f, 30.2f, 77.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color215", {255, 153, 51}, {72.4f, 31.0f, 65.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color216", {255, 153, 102}, {72.9f, 33.6f, 42.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color217", {255, 153, 153}, {73.8f, 38.1f, 16.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color218", {255, 153, 204}, {75.1f, 44.6f, -9.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color219", {255, 153, 255}, {76.8f, 52.8f, -34.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color22", {0, 51, 0}, {17.3f, -27.7f, 24.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color220", {255, 204, 0}, {84.2f, 3.7f, 85.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color221", {255, 204, 51}, {84.3f, 4.5f, 76.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color222", {255, 204, 102}, {84.7f, 7.0f, 56.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color223", {255, 204, 153}, {85.4f, 11.5f, 32.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color224", {255, 204, 204}, {86.4f, 18.0f, 6.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color225", {255, 204, 255}, {87.8f, 26.4f, -18.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color227", {255, 255, 51}, {97.2f, -20.8f, 87.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color228", {255, 255, 102}, {97.5f, -18.4f, 70.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color229", {255, 255, 153}, {98.1f, -14.2f, 48.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color23", {0, 51, 51}, {18.4f, -15.4f, -4.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color230", {255, 255, 204}, {98.9f, -8.1f, 24.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color24", {0, 51, 102}, {21.3f, 6.8f, -34.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color25", {0, 51, 153}, {25.8f, 28.9f, -59.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color26", {0, 51, 204}, {31.2f, 48.0f, -80.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color27", {0, 51, 255}, {37.1f, 64.7f, -99.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color28", {0, 102, 0}, {36.9f, -44.0f, 42.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color29", {0, 102, 51}, {37.4f, -38.5f, 21.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color30", {0, 102, 102}, {38.7f, -24.5f, -7.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color31", {0, 102, 153}, {40.9f, -5.3f, -34.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color32", {0, 102, 204}, {44.0f, 15.3f, -59.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color33", {0, 102, 255}, {47.9f, 35.2f, -82.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color34", {0, 153, 0}, {54.8f, -58.9f, 56.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color35", {0, 153, 51}, {55.1f, -55.7f, 42.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color36", {0, 153, 102}, {55.8f, -46.9f, 17.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color37", {0, 153, 153}, {57.2f, -32.8f, -9.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color38", {0, 153, 204}, {59.1f, -15.4f, -35.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color39", {0, 153, 255}, {61.7f, 3.6f, -59.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color40", {0, 204, 0}, {71.7f, -72.8f, 70.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color41", {0, 204, 51}, {71.8f, -70.8f, 60.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color42", {0, 204, 102}, {72.3f, -64.7f, 38.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color43", {0, 204, 153}, {73.2f, -54.5f, 13.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color44", {0, 204, 204}, {74.5f, -40.6f, -11.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color45", {0, 204, 255}, {76.3f, -24.4f, -36.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color47", {0, 255, 51}, {87.9f, -84.7f, 75.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color48", {0, 255, 102}, {88.2f, -80.3f, 57.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color49", {0, 255, 153}, {88.8f, -72.5f, 34.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color50", {0, 255, 204}, {89.8f, -61.6f, 10.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color52", {51, 0, 0}, {6.4f, 25.2f, 10.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color53", {51, 0, 51}, {8.5f, 31.5f, -19.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color54", {51, 0, 102}, {13.6f, 42.8f, -46.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color55", {51, 0, 153}, {20.1f, 55.3f, -68.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color56", {51, 0, 204}, {26.9f, 67.7f, -87.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color57", {51, 0, 255}, {33.8f, 79.7f, -105.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color58", {51, 51, 0}, {20.3f, -6.9f, 28.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color59", {51, 51, 51}, {21.2f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color60", {51, 51, 102}, {23.8f, 15.2f, -30.1f}, COLOR_SCHEME_XTERM},
    {(char *)"color61", {51, 51, 153}, {27.8f, 33.2f, -55.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color62", {51, 51, 204}, {32.8f, 50.3f, -77.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color63", {51, 51, 255}, {38.4f, 66.0f, -97.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color64", {51, 102, 0}, {38.2f, -33.3f, 44.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color65", {51, 102, 51}, {38.6f, -28.8f, 23.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color66", {51, 102, 102}, {39.9f, -17.0f, -5.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color67", {51, 102, 153}, {42.0f, -0.2f, -32.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color68", {51, 102, 204}, {45.0f, 18.7f, -57.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color69", {51, 102, 255}, {48.8f, 37.5f, -80.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color70", {51, 153, 0}, {55.6f, -52.5f, 57.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color71", {51, 153, 51}, {55.8f, -49.6f, 43.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color72", {51, 153, 102}, {56.5f, -41.5f, 18.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color73", {51, 153, 153}, {57.8f, -28.5f, -8.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color74", {51, 153, 204}, {59.7f, -12.0f, -34.5f}, COLOR_SCHEME_XTERM},
    {(char *)"color75", {51, 153, 255}, {62.3f, 6.1f, -58.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color76", {51, 204, 0}, {72.2f, -68.6f, 70.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color77", {51, 204, 51}, {72.3f, -66.6f, 60.8f}, COLOR_SCHEME_XTERM},
    {(char *)"color78", {51, 204, 102}, {72.8f, -60.9f, 39.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color79", {51, 204, 153}, {73.7f, -51.0f, 14.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color80", {51, 204, 204}, {75.0f, -37.7f, -11.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color81", {51, 204, 255}, {76.7f, -22.0f, -36.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color82", {51, 255, 0}, {88.1f, -83.1f, 83.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color83", {51, 255, 51}, {88.2f, -81.7f, 76.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color84", {51, 255, 102}, {88.5f, -77.4f, 58.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color85", {51, 255, 153}, {89.2f, -69.9f, 35.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color86", {51, 255, 204}, {90.1f, -59.2f, 10.9f}, COLOR_SCHEME_XTERM},
    {(char *)"color87", {51, 255, 255}, {91.4f, -46.0f, -13.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color88", {102, 0, 0}, {19.3f, 40.9f, 29.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color89", {102, 0, 51}, {20.3f, 43.5f, -0.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color90", {102, 0, 102}, {22.9f, 50.1f, -31.0f}, COLOR_SCHEME_XTERM},
    {(char *)"color91", {102, 0, 153}, {27.1f, 59.6f, -56.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color92", {102, 0, 204}, {32.2f, 70.4f, -78.6f}, COLOR_SCHEME_XTERM},
    {(char *)"color93", {102, 0, 255}, {37.9f, 81.5f, -98.3f}, COLOR_SCHEME_XTERM},
    {(char *)"color94", {102, 51, 0}, {27.3f, 19.6f, 37.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color95", {102, 51, 51}, {27.9f, 22.8f, 10.4f}, COLOR_SCHEME_XTERM},
    {(char *)"color96", {102, 51, 102}, {29.8f, 31.0f, -20.2f}, COLOR_SCHEME_XTERM},
    {(char *)"color97", {102, 51, 153}, {32.9f, 42.9f, -47.1f}, COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"color98", {102, 51, 204}, {37.0f, 56.2f, -70.7f}, COLOR_SCHEME_XTERM},
    {(char *)"color99", {102, 51, 255}, {41.9f, 69.7f, -91.8f}, COLOR_SCHEME_XTERM},
    {(char *)"coral", {255, 127, 80}, {67.3f, 45.4f, 47.5f}, COLOR_SCHEME_CSS},
    {(char *)"cornflowerblue", {100, 149, 237}, {61.9f, 9.3f, -49.3f}, COLOR_SCHEME_CSS},
    {(char *)"cornsilk", {255, 248, 220}, {97.5f, -2.2f, 14.3f}, COLOR_SCHEME_CSS},
    {(char *)"crimson", {220, 20, 60}, {47.0f, 70.9f, 33.6f}, COLOR_SCHEME_CSS},
    {(char *)"cyan", {0, 255, 255}, {91.1f, -48.1f, -14.1f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"darkblue", {0, 0, 139}, {14.8f, 50.4f, -68.7f}, COLOR_SCHEME_CSS},
    {(char *)"darkcyan", {0, 139, 139}, {52.2f, -30.6f, -9.0f}, COLOR_SCHEME_CSS},
    {(char *)"darkgoldenrod", {184, 134, 11}, {59.2f, 9.9f, 62.7f}, COLOR_SCHEME_CSS},
    {(char *)"darkgray", {169, 169, 169}, {69.2f, -0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"darkgreen", {0, 100, 0}, {36.2f, -43.4f, 41.9f}, COLOR_SCHEME_CSS},
    {(char *)"darkgrey", {169, 169, 169}, {69.2f, 0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"darkkhaki", {189, 183, 107}, {73.4f, -8.8f, 39.3f}, COLOR_SCHEME_CSS},
    {(char *)"darkmagenta", {139, 0, 139}, {32.6f, 62.6f, -38.7f}, COLOR_SCHEME_CSS},
    {(char *)"darkolivegreen", {85, 107, 47}, {42.2f, -18.8f, 30.6f}, COLOR_SCHEME_CSS},
    {(char *)"darkorange", {255, 140, 0}, {69.5f, 36.8f, 75.5f}, COLOR_SCHEME_CSS},
    {(char *)"darkorchid", {153, 50, 204}, {43.4f, 65.2f, -60.1f}, COLOR_SCHEME_CSS},
    {(char *)"darkred", {139, 0, 0}, {28.1f, 51.0f, 41.3f}, COLOR_SCHEME_CSS},
    {(char *)"darksalmon", {233, 150, 122}, {69.9f, 28.2f, 27.7f}, COLOR_SCHEME_CSS},
    {(char *)"darkseagreen", {143, 188, 143}, {72.1f, -23.8f, 18.0f}, COLOR_SCHEME_CSS},
    {(char *)"darkslateblue", {72, 61, 139}, {30.8f, 26.1f, -42.1f}, COLOR_SCHEME_CSS},
    {(char *)"darkslategray", {47, 79, 79}, {31.3f, -11.7f, -3.7f}, COLOR_SCHEME_CSS},
    {(char *)"darkslategrey", {47, 79, 79}, {31.3f, -11.7f, -3.7f}, COLOR_SCHEME_CSS},
    {(char *)"darkturquoise", {0, 206, 209}, {75.3f, -40.0f, -13.5f}, COLOR_SCHEME_CSS},
    {(char *)"darkviolet", {148, 0, 211}, {39.6f, 76.3f, -70.4f}, COLOR_SCHEME_CSS},
    {(char *)"deeppink", {255, 20, 147}, {56.0f, 84.5f, -5.7f}, COLOR_SCHEME_CSS},
    {(char *)"deepskyblue", {0, 191, 255}, {72.5f, -17.7f, -42.5f}, COLOR_SCHEME_CSS},
    {(char *)"dimgray", {105, 105, 105}, {44.4f, -0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"dimgrey", {105, 105, 105}, {44.4f, 0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"dodgerblue", {30, 144, 255}, {59.4f, 10.0f, -63.4f}, COLOR_SCHEME_CSS},
    {(char *)"firebrick", {178, 34, 34}, {39.1f, 55.9f, 37.6f}, COLOR_SCHEME_CSS},
    {(char *)"floralwhite", {255, 250, 240}, {98.4f, 0.0f, 5.4f}, COLOR_SCHEME_CSS},
    {(char *)"forestgreen", {34, 139, 34}, {50.6f, -49.6f, 45.0f}, COLOR_SCHEME_CSS},
    {(char *)"fuchsia", {255, 0, 255}, {60.3f, 98.2f, -60.8f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"gainsboro", {220, 220, 220}, {87.8f, 0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"ghostwhite", {248, 248, 255}, {97.8f, 1.2f, -3.3f}, COLOR_SCHEME_CSS},
    {(char *)"gold", {255, 215, 0}, {86.9f, -1.9f, 87.1f}, COLOR_SCHEME_CSS},
    {(char *)"goldenrod", {218, 165, 32}, {70.8f, 8.5f, 68.8f}, COLOR_SCHEME_CSS},
    {(char *)"gray", {128, 128, 128}, {53.6f, -0.0f, 0.0f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"green", {0, 128, 0}, {46.2f, -51.7f, 49.9f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"greenyellow", {173, 255, 47}, {92.0f, -52.5f, 81.9f}, COLOR_SCHEME_CSS},
    {(char *)"grey", {128, 128, 128}, {53.6f, 0.0f, 0.0f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"grey10", {78, 78, 78}, {33.2f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey11", {88, 88, 88}, {37.4f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey12", {98, 98, 98}, {41.6f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey13", {108, 108, 108}, {45.6f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey14", {118, 118, 118}, {49.6f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey16", {138, 138, 138}, {57.5f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey17", {148, 148, 148}, {61.3f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey18", {158, 158, 158}, {65.1f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey19", {168, 168, 168}, {68.9f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey20", {178, 178, 178}, {72.6f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey21", {188, 188, 188}, {76.2f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey22", {198, 198, 198}, {79.9f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey23", {208, 208, 208}, {83.5f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey24", {218, 218, 218}, {87.1f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey25", {228, 228, 228}, {90.6f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey26", {238, 238, 238}, {94.1f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey3", {8, 8, 8}, {2.2f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey4", {18, 18, 18}, {5.5f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey5", {28, 28, 28}, {10.3f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey6", {38, 38, 38}, {15.2f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey7", {48, 48, 48}, {19.9f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey8", {58, 58, 58}, {24.4f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"grey9", {68, 68, 68}, {28.9f, 0.0f, 0.0f}, COLOR_SCHEME_XTERM},
    {(char *)"honeydew", {240, 255, 240}, {98.6f, -7.6f, 5.5f}, COLOR_SCHEME_CSS},
    {(char *)"hotpink", {255, 105, 180}, {65.5f, 64.2f, -10.6f}, COLOR_SCHEME_CSS},
    {(char *)"indianred", {205, 92, 92}, {53.4f, 44.8f, 22.1f}, COLOR_SCHEME_CSS},
    {(char *)"indigo", {75, 0, 130}, {20.5f, 51.7f, -53.3f}, COLOR_SCHEME_CSS},
    {(char *)"ivory", {255, 255, 240}, {99.6f, -2.6f, 7.2f}, COLOR_SCHEME_CSS},
    {(char *)"khaki", {240, 230, 140}, {90.3f, -9.0f, 45.0f}, COLOR_SCHEME_CSS},
    {(char *)"lavender", {230, 230, 250}, {91.8f, 3.7f, -9.7f}, COLOR_SCHEME_CSS},
    {(char *)"lavenderblush", {255, 240, 245}, {96.1f, 5.9f, -0.6f}, COLOR_SCHEME_CSS},
    {(char *)"lawngreen", {124, 252, 0}, {88.9f, -67.9f, 85.0f}, COLOR_SCHEME_CSS},
    {(char *)"lemonchiffon", {255, 250, 205}, {97.6f, -5.4f, 22.2f}, COLOR_SCHEME_CSS},
    {(char *)"lightblue", {173, 216, 230}, {83.8f, -10.9f, -11.5f}, COLOR_SCHEME_CSS},
    {(char *)"lightcoral", {240, 128, 128}, {66.2f, 42.8f, 19.6f}, COLOR_SCHEME_CSS},
    {(char *)"lightcyan", {224, 255, 255}, {97.9f, -9.9f, -3.4f}, COLOR_SCHEME_CSS},
    {(char *)"lightgoldenrodyellow", {250, 250, 210}, {97.4f, -6.5f, 19.2f}, COLOR_SCHEME_CSS},
    {(char *)"lightgray", {211, 211, 211}, {84.6f, -0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"lightgreen", {144, 238, 144}, {86.5f, -46.3f, 36.9f}, COLOR_SCHEME_CSS},
    {(char *)"lightgrey", {211, 211, 211}, {84.6f, 0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"lightpink", {255, 182, 193}, {81.1f, 28.0f, 5.0f}, COLOR_SCHEME_CSS},
    {(char *)"lightsalmon", {255, 160, 122}, {74.7f, 31.5f, 34.5f}, COLOR_SCHEME_CSS},
    {(char *)"lightseagreen", {32, 178, 170}, {65.8f, -37.5f, -6.3f}, COLOR_SCHEME_CSS},
    {(char *)"lightskyblue", {135, 206, 250}, {79.7f, -10.8f, -28.5f}, COLOR_SCHEME_CSS},
    {(char *)"lightsteelblue", {176, 196, 222}, {78.5f, -1.3f, -15.2f}, COLOR_SCHEME_CSS},
    {(char *)"lightyellow", {255, 255, 224}, {99.3f, -5.1f, 14.8f}, COLOR_SCHEME_CSS},
    {(char *)"lime", {0, 255, 0}, {87.7f, -86.2f, 83.2f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"limegreen", {50, 205, 50}, {72.6f, -67.1f, 61.4f}, COLOR_SCHEME_CSS},
    {(char *)"linen", {250, 240, 230}, {95.3f, 1.7f, 6.0f}, COLOR_SCHEME_CSS},
    {(char *)"magenta", {255, 0, 255}, {60.3f, 98.2f, -60.8f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"maroon", {128, 0, 0}, {25.5f, 48.0f, 38.1f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"mediumaquamarine", {102, 205, 170}, {75.7f, -38.3f, 8.3f}, COLOR_SCHEME_CSS},
    {(char *)"mediumblue", {0, 0, 205}, {25.0f, 67.2f, -91.5f}, COLOR_SCHEME_CSS},
    {(char *)"mediumorchid", {186, 85, 211}, {53.6f, 59.1f, -47.4f}, COLOR_SCHEME_CSS},
    {(char *)"mediumpurple", {147, 112, 219}, {55.0f, 36.8f, -50.1f}, COLOR_SCHEME_CSS},
    {(char *)"mediumseagreen", {60, 179, 113}, {65.3f, -48.2f, 24.3f}, COLOR_SCHEME_CSS},
    {(char *)"mediumslateblue", {123, 104, 238}, {52.2f, 41.1f, -65.4f}, COLOR_SCHEME_CSS},
    {(char *)"mediumspringgreen", {0, 250, 154}, {87.3f, -70.7f, 32.5f}, COLOR_SCHEME_CSS},
    {(char *)"mediumturquoise", {72, 209, 204}, {76.9f, -37.4f, -8.4f}, COLOR_SCHEME_CSS},
    {(char *)"mediumvioletred", {199, 21, 133}, {44.8f, 71.0f, -15.2f}, COLOR_SCHEME_CSS},
    {(char *)"midnightblue", {25, 25, 112}, {15.9f, 31.7f, -49.6f}, COLOR_SCHEME_CSS},
    {(char *)"mintcream", {245, 255, 250}, {99.2f, -4.2f, 1.2f}, COLOR_SCHEME_CSS},
    {(char *)"mistyrose", {255, 228, 225}, {92.7f, 8.7f, 4.8f}, COLOR_SCHEME_CSS},
    {(char *)"moccasin", {255, 228, 181}, {91.7f, 2.4f, 26.4f}, COLOR_SCHEME_CSS},
    {(char *)"navajowhite", {255, 222, 173}, {90.1f, 4.5f, 28.3f}, COLOR_SCHEME_CSS},
    {(char *)"navy", {0, 0, 128}, {13.0f, 47.5f, -64.7f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"oldlace", {253, 245, 230}, {96.8f, 0.2f, 8.2f}, COLOR_SCHEME_CSS},
    {(char *)"olive", {128, 128, 0}, {51.9f, -12.9f, 56.7f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"olivedrab", {107, 142, 35}, {54.7f, -28.2f, 49.7f}, COLOR_SCHEME_CSS},
    {(char *)"orange", {255, 165, 0}, {74.9f, 23.9f, 78.9f}, COLOR_SCHEME_CSS},
    {(char *)"orangered", {255, 69, 0}, {57.6f, 67.8f, 69.0f}, COLOR_SCHEME_CSS},
    {(char *)"orchid", {218, 112, 214}, {62.8f, 55.3f, -34.4f}, COLOR_SCHEME_CSS},
    {(char *)"palegoldenrod", {238, 232, 170}, {91.1f, -7.3f, 31.0f}, COLOR_SCHEME_CSS},
    {(char *)"palegreen", {152, 251, 152}, {90.7f, -48.3f, 38.5f}, COLOR_SCHEME_CSS},
    {(char *)"paleturquoise", {175, 238, 238}, {90.1f, -19.6f, -6.4f}, COLOR_SCHEME_CSS},
    {(char *)"palevioletred", {219, 112, 147}, {60.6f, 45.5f, 0.4f}, COLOR_SCHEME_CSS},
    {(char *)"papayawhip", {255, 239, 213}, {95.1f, 1.3f, 14.5f}, COLOR_SCHEME_CSS},
    {(char *)"peachpuff", {255, 218, 185}, {89.4f, 8.1f, 21.0f}, COLOR_SCHEME_CSS},
    {(char *)"peru", {205, 133, 63}, {61.8f, 21.4f, 47.9f}, COLOR_SCHEME_CSS},
    {(char *)"pink", {255, 192, 203}, {83.6f, 24.1f, 3.3f}, COLOR_SCHEME_CSS},
    {(char *)"plum", {221, 160, 221}, {73.4f, 32.5f, -22.0f}, COLOR_SCHEME_CSS},
    {(char *)"powderblue", {176, 224, 230}, {86.1f, -14.1f, -8.0f}, COLOR_SCHEME_CSS},
    {(char *)"purple", {128, 0, 128}, {29.8f, 58.9f, -36.5f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"red", {255, 0, 0}, {53.2f, 80.1f, 67.2f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"rosybrown", {188, 143, 143}, {63.6f, 17.0f, 6.6f}, COLOR_SCHEME_CSS},
    {(char *)"royalblue", {65, 105, 225}, {47.8f, 26.3f, -65.3f}, COLOR_SCHEME_CSS},
    {(char *)"saddlebrown", {139, 69, 19}, {37.5f, 26.4f, 41.0f}, COLOR_SCHEME_CSS},
    {(char *)"salmon", {250, 128, 114}, {67.3f, 45.2f, 29.1f}, COLOR_SCHEME_CSS},
    {(char *)"sandybrown", {244, 164, 96}, {74.0f, 23.0f, 46.8f}, COLOR_SCHEME_CSS},
    {(char *)"seagreen", {46, 139, 87}, {51.5f, -39.7f, 20.1f}, COLOR_SCHEME_CSS},
    {(char *)"seashell", {255, 245, 238}, {97.1f, 2.2f, 4.6f}, COLOR_SCHEME_CSS},
    {(char *)"sienna", {160, 82, 45}, {43.8f, 29.3f, 35.6f}, COLOR_SCHEME_CSS},
    {(char *)"silver", {192, 192, 192}, {77.7f, 0.0f, 0.0f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"skyblue", {135, 206, 235}, {79.2f, -14.8f, -21.3f}, COLOR_SCHEME_CSS},
    {(char *)"slateblue", {106, 90, 205}, {45.3f, 36.0f, -57.8f}, COLOR_SCHEME_CSS},
    {(char *)"slategray", {119, 136, 153}, {55.9f, -2.2f, -11.1f}, COLOR_SCHEME_CSS},
    {(char *)"slategrey", {119, 136, 153}, {55.9f, -2.2f, -11.1f}, COLOR_SCHEME_CSS},
    {(char *)"snow", {255, 250, 250}, {98.6f, 1.7f, 0.6f}, COLOR_SCHEME_CSS},
    {(char *)"springgreen", {0, 255, 127}, {88.5f, -76.9f, 47.0f}, COLOR_SCHEME_CSS},
    {(char *)"steelblue", {70, 130, 180}, {52.5f, -4.1f, -32.2f}, COLOR_SCHEME_CSS},
    {(char *)"tan", {210, 180, 140}, {75.0f, 5.0f, 24.4f}, COLOR_SCHEME_CSS},
    {(char *)"teal", {0, 128, 128}, {48.3f, -28.8f, -8.5f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"thistle", {216, 191, 216}, {80.1f, 13.2f, -9.2f}, COLOR_SCHEME_CSS},
    {(char *)"tomato", {255, 99, 71}, {62.2f, 57.9f, 46.4f}, COLOR_SCHEME_CSS},
    {(char *)"turquoise", {64, 224, 208}, {81.3f, -44.1f, -4.0f}, COLOR_SCHEME_CSS},
    {(char *)"violet", {238, 130, 238}, {69.7f, 56.4f, -36.8f}, COLOR_SCHEME_CSS},
    {(char *)"wheat", {245, 222, 179}, {89.4f, 1.5f, 24.0f}, COLOR_SCHEME_CSS},
    {(char *)"white", {255, 255, 255}, {100.0f, 0.0f, 0.0f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"whitesmoke", {245, 245, 245}, {96.5f, 0.0f, 0.0f}, COLOR_SCHEME_CSS},
    {(char *)"yellow", {255, 255, 0}, {97.1f, -21.6f, 94.5f}, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS},
    {(char *)"yellowgreen", {154, 205, 50}, {76.5f, -38.0f, 66.6f}, COLOR_SCHEME_CSS},
    {(char *)NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}, 0}};
