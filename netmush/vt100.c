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
COLORMATCH getColorMatch(rgbColor rgb, COLORINFO palette[])
{
    COLORMATCH cm = {101, {NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}}};
    CIELABColor labTarget = xyzToCIELAB(rgbToXyz(rgb));

    for (COLORINFO *cinfo = palette; cinfo->name != NULL; cinfo++)
    {
        float deltaE = sqrtf(powf(labTarget.l - cinfo->lab.l, 2) + powf(labTarget.a - cinfo->lab.a, 2) + powf(labTarget.b - cinfo->lab.b, 2));

        if (deltaE == 0)
        {
            return (COLORMATCH){deltaE, *cinfo};
        }
        else if (deltaE < cm.deltaE)
        {
            cm.deltaE = deltaE;
            cm.color = *cinfo;
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

    XASPRINTF(vt, "\e[%d;2;%d;%d;%dm", background ? 48 : 38, rgb.r, rgb.g, rgb.b);

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

    XASPRINTF(vt, "\e[%d;5;%dm", background ? 48 : 38, color);

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

    XASPRINTF(vt, "\e[%dm", (background ? 40 : 30) + (color & 7) + (color > 7 ? 60 : 0));

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

    if (color <= 6 || ((color >= 8) && (color <= 15)))
    {
        rgb.r = color & 1 ? (color & 8 ? 255 : 128) : 0;
        rgb.g = color & 2 ? (color & 8 ? 255 : 128) : 0;
        rgb.b = color & 4 ? (color & 8 ? 255 : 128) : 0;
    }
    else if (color == 7)
    {
        rgb.r = 192;
        rgb.g = 192;
        rgb.b = 192;
    }
    else if (color == 8)
    {
        rgb.r = 128;
        rgb.g = 128;
        rgb.b = 128;
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
    if (rgb.r + rgb.g + rgb.b == 0)
    {
        return 0;
    }
    else if (((rgb.r & 128) == rgb.r) && ((rgb.g & 128) == rgb.g) && ((rgb.b & 128) == rgb.b))
    {
        uint8_t c = (rgb.r >> 7) + (rgb.g >> 6) + (rgb.b >> 5);
        return c == 7 ? 8 : c;
    }
    else if ((rgb.r == 192) && (rgb.g == 192) && (rgb.b == 192))
    {
        return 7;
    }
    else if ((rgb.r == 255 || rgb.r == 0) && (rgb.g == 255 || rgb.g == 0) && (rgb.b == 255 || rgb.b == 0))
    {
        return (rgb.r & 1) + (rgb.g & 2) + (rgb.b & 4) + 8;
    }
    else if ((rgb.r == rgb.g) && (rgb.r == rgb.b))
    {
        int level = (rgb.r - 8) / 10;

        if (level < 0)
        {
            level = 0;
        }
        else if (level > 23)
        {
            level = 23;
        }

        return (uint8_t)(level + 232);
    }
    else
    {
        int xr = (rgb.r - 55) / 40;
        int xg = (rgb.g - 55) / 40;
        int xb = (rgb.b - 55) / 40;

        return ((xr > 0 ? xr : 0) * 36) + ((xg > 0 ? xg : 0) * 6) + (xb > 0 ? xb : 0) + 16;
    }

    return 0;
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
        rgbColor rgb = xtermColor[color].rgb;
        COLORMATCH cm = getColorMatch(rgb, ansiColor);
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
    COLORMATCH cm = getColorMatch(rgb, ansiColor);

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

    return RGB2X11(cm.color.rgb) & 0xF;
}

VT100ATTR decodeVT100(const char **ansi)
{
    int *codes = NULL;
    int index = 0;
    VT100ATTR attr = {{ANSICOLORTYPE_NONE, {0, 0, 0}}, {ANSICOLORTYPE_NONE, {0, 0, 0}}, false};

    if (ansi == NULL || *ansi == NULL || **ansi != '\e')
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
                    int *tmp = (int *)XREALLOC(codes, (index + 1) * sizeof(int), "codes");
                    if (tmp != NULL)
                    {
                        codes = tmp;
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
                        XFREE(codes);
                        codes = NULL;
                        break;
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
        }
        else
        {
            if (codes != NULL)
            {
                XFREE(codes);
                codes = NULL;
            }
        }
    }

    if (codes != NULL)
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
                        attr.foreground.rgb = xtermColor[(uint8_t)codes[i]].rgb;
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
                        attr.background.rgb = xtermColor[(uint8_t)codes[i]].rgb;
                    }
                    break;
                }
                break;
            default:
                if (code >= 30 && code <= 37)
                {
                    attr.foreground.type = ANSICOLORTYPE_STANDARD;
                    attr.foreground.rgb = xtermColor[code - 30].rgb;
                }
                else if (code >= 40 && code <= 47)
                {
                    attr.background.type = ANSICOLORTYPE_STANDARD;
                    attr.background.rgb = xtermColor[code - 40].rgb;
                }
                else if (code >= 90 && code <= 97)
                {
                    attr.foreground.type = ANSICOLORTYPE_STANDARD;
                    attr.foreground.rgb = xtermColor[code - 90 + 8].rgb;
                }
                else if (code >= 100 && code <= 107)
                {
                    attr.background.type = ANSICOLORTYPE_STANDARD;
                    attr.background.rgb = xtermColor[code - 100 + 8].rgb;
                }
                break;
            }
        }
        XFREE(codes);
    }
    return attr;
}

COLORINFO ansiColor[] = {
    {(char *)"black", {0, 0, 0}, {0.0f, 0.0f, 0.0f}},
    {(char *)"maroon", {128, 0, 0}, {25.5f, 48.0f, 38.1f}},
    {(char *)"green", {0, 128, 0}, {46.2f, -51.7f, 49.9f}},
    {(char *)"olive", {128, 128, 0}, {51.9f, -12.9f, 56.7f}},
    {(char *)"navy", {0, 0, 128}, {13.0f, 47.5f, -64.7f}},
    {(char *)"purple", {128, 0, 128}, {29.8f, 58.9f, -36.5f}},
    {(char *)"teal", {0, 128, 128}, {48.3f, -28.8f, -8.5f}},
    {(char *)"silver", {192, 192, 192}, {77.7f, -0.0f, 0.0f}},
    {(char *)"grey", {128, 128, 128}, {53.6f, -0.0f, 0.0f}},
    {(char *)"red", {255, 0, 0}, {53.2f, 80.1f, 67.2f}},
    {(char *)"lime", {0, 255, 0}, {87.7f, -86.2f, 83.2f}},
    {(char *)"yellow", {255, 255, 0}, {97.1f, -21.6f, 94.5f}},
    {(char *)"blue", {0, 0, 255}, {32.3f, 79.2f, -107.9f}},
    {(char *)"fuchsia", {255, 0, 255}, {60.3f, 98.2f, -60.8f}},
    {(char *)"aqua", {0, 255, 255}, {91.1f, -48.1f, -14.1f}},
    {(char *)"white", {255, 255, 255}, {100.0f, -0.0f, 0.0f}},
    {(char *)NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}}};

COLORINFO xtermColor[] = {
    {(char *)"black", {0, 0, 0}, {0.0f, 0.0f, 0.0f}},
    {(char *)"maroon", {128, 0, 0}, {25.5f, 48.0f, 38.1f}},
    {(char *)"green", {0, 128, 0}, {46.2f, -51.7f, 49.9f}},
    {(char *)"olive", {128, 128, 0}, {51.9f, -12.9f, 56.7f}},
    {(char *)"navy", {0, 0, 128}, {13.0f, 47.5f, -64.7f}},
    {(char *)"purple", {128, 0, 128}, {29.8f, 58.9f, -36.5f}},
    {(char *)"teal", {0, 128, 128}, {48.3f, -28.8f, -8.5f}},
    {(char *)"silver", {192, 192, 192}, {77.7f, -0.0f, 0.0f}},
    {(char *)"grey", {128, 128, 128}, {53.6f, -0.0f, 0.0f}},
    {(char *)"red", {255, 0, 0}, {53.2f, 80.1f, 67.2f}},
    {(char *)"lime", {0, 255, 0}, {87.7f, -86.2f, 83.2f}},
    {(char *)"yellow", {255, 255, 0}, {97.1f, -21.6f, 94.5f}},
    {(char *)"blue", {0, 0, 255}, {32.3f, 79.2f, -107.9f}},
    {(char *)"fuchsia", {255, 0, 255}, {60.3f, 98.2f, -60.8f}},
    {(char *)"aqua", {0, 255, 255}, {91.1f, -48.1f, -14.1f}},
    {(char *)"white", {255, 255, 255}, {100.0f, -0.0f, 0.0f}},
    {(char *)"color16", {0, 0, 0}, {0.0f, 0.0f, 0.0f}},
    {(char *)"color17", {0, 0, 51}, {2.2f, 15.2f, -30.1f}},
    {(char *)"color18", {0, 0, 102}, {8.6f, 40.4f, -55.0f}},
    {(char *)"color19", {0, 0, 153}, {17.0f, 54.1f, -73.7f}},
    {(char *)"color20", {0, 0, 204}, {24.8f, 66.9f, -91.2f}},
    {(char *)"color21", {0, 0, 255}, {32.3f, 79.2f, -107.9f}},
    {(char *)"color22", {0, 51, 0}, {17.3f, -27.7f, 24.2f}},
    {(char *)"color23", {0, 51, 51}, {18.4f, -15.4f, -4.5f}},
    {(char *)"color24", {0, 51, 102}, {21.3f, 6.8f, -34.2f}},
    {(char *)"color25", {0, 51, 153}, {25.8f, 28.9f, -59.1f}},
    {(char *)"color26", {0, 51, 204}, {31.2f, 48.0f, -80.6f}},
    {(char *)"color27", {0, 51, 255}, {37.1f, 64.7f, -99.9f}},
    {(char *)"color28", {0, 102, 0}, {36.9f, -44.0f, 42.4f}},
    {(char *)"color29", {0, 102, 51}, {37.4f, -38.5f, 21.7f}},
    {(char *)"color30", {0, 102, 102}, {38.7f, -24.5f, -7.2f}},
    {(char *)"color31", {0, 102, 153}, {40.9f, -5.3f, -34.7f}},
    {(char *)"color32", {0, 102, 204}, {44.0f, 15.3f, -59.5f}},
    {(char *)"color33", {0, 102, 255}, {47.9f, 35.2f, -82.0f}},
    {(char *)"color34", {0, 153, 0}, {54.8f, -58.9f, 56.8f}},
    {(char *)"color35", {0, 153, 51}, {55.1f, -55.7f, 42.7f}},
    {(char *)"color36", {0, 153, 102}, {55.8f, -46.9f, 17.4f}},
    {(char *)"color37", {0, 153, 153}, {57.2f, -32.8f, -9.7f}},
    {(char *)"color38", {0, 153, 204}, {59.1f, -15.4f, -35.5f}},
    {(char *)"color39", {0, 153, 255}, {61.7f, 3.6f, -59.8f}},
    {(char *)"color40", {0, 204, 0}, {71.7f, -72.8f, 70.3f}},
    {(char *)"color41", {0, 204, 51}, {71.8f, -70.8f, 60.2f}},
    {(char *)"color42", {0, 204, 102}, {72.3f, -64.7f, 38.9f}},
    {(char *)"color43", {0, 204, 153}, {73.2f, -54.5f, 13.7f}},
    {(char *)"color44", {0, 204, 204}, {74.5f, -40.6f, -11.9f}},
    {(char *)"color45", {0, 204, 255}, {76.3f, -24.4f, -36.7f}},
    {(char *)"color46", {0, 255, 0}, {87.7f, -86.2f, 83.2f}},
    {(char *)"color47", {0, 255, 51}, {87.9f, -84.7f, 75.6f}},
    {(char *)"color48", {0, 255, 102}, {88.2f, -80.3f, 57.9f}},
    {(char *)"color49", {0, 255, 153}, {88.8f, -72.5f, 34.9f}},
    {(char *)"color50", {0, 255, 204}, {89.8f, -61.6f, 10.4f}},
    {(char *)"color51", {0, 255, 255}, {91.1f, -48.1f, -14.1f}},
    {(char *)"color52", {51, 0, 0}, {6.4f, 25.2f, 10.0f}},
    {(char *)"color53", {51, 0, 51}, {8.5f, 31.5f, -19.5f}},
    {(char *)"color54", {51, 0, 102}, {13.6f, 42.8f, -46.6f}},
    {(char *)"color55", {51, 0, 153}, {20.1f, 55.3f, -68.5f}},
    {(char *)"color56", {51, 0, 204}, {26.9f, 67.7f, -87.6f}},
    {(char *)"color57", {51, 0, 255}, {33.8f, 79.7f, -105.3f}},
    {(char *)"color58", {51, 51, 0}, {20.3f, -6.9f, 28.5f}},
    {(char *)"color59", {51, 51, 51}, {21.2f, -0.0f, 0.0f}},
    {(char *)"color60", {51, 51, 102}, {23.8f, 15.2f, -30.1f}},
    {(char *)"color61", {51, 51, 153}, {27.8f, 33.2f, -55.7f}},
    {(char *)"color62", {51, 51, 204}, {32.8f, 50.3f, -77.9f}},
    {(char *)"color63", {51, 51, 255}, {38.4f, 66.0f, -97.7f}},
    {(char *)"color64", {51, 102, 0}, {38.2f, -33.3f, 44.0f}},
    {(char *)"color65", {51, 102, 51}, {38.6f, -28.8f, 23.6f}},
    {(char *)"color66", {51, 102, 102}, {39.9f, -17.0f, -5.3f}},
    {(char *)"color67", {51, 102, 153}, {42.0f, -0.2f, -32.8f}},
    {(char *)"color68", {51, 102, 204}, {45.0f, 18.7f, -57.9f}},
    {(char *)"color69", {51, 102, 255}, {48.8f, 37.5f, -80.5f}},
    {(char *)"color70", {51, 153, 0}, {55.6f, -52.5f, 57.7f}},
    {(char *)"color71", {51, 153, 51}, {55.8f, -49.6f, 43.7f}},
    {(char *)"color72", {51, 153, 102}, {56.5f, -41.5f, 18.5f}},
    {(char *)"color73", {51, 153, 153}, {57.8f, -28.5f, -8.6f}},
    {(char *)"color74", {51, 153, 204}, {59.7f, -12.0f, -34.5f}},
    {(char *)"color75", {51, 153, 255}, {62.3f, 6.1f, -58.8f}},
    {(char *)"color76", {51, 204, 0}, {72.2f, -68.6f, 70.9f}},
    {(char *)"color77", {51, 204, 51}, {72.3f, -66.6f, 60.8f}},
    {(char *)"color78", {51, 204, 102}, {72.8f, -60.9f, 39.6f}},
    {(char *)"color79", {51, 204, 153}, {73.7f, -51.0f, 14.4f}},
    {(char *)"color80", {51, 204, 204}, {75.0f, -37.7f, -11.2f}},
    {(char *)"color81", {51, 204, 255}, {76.7f, -22.0f, -36.0f}},
    {(char *)"color82", {51, 255, 0}, {88.1f, -83.1f, 83.6f}},
    {(char *)"color83", {51, 255, 51}, {88.2f, -81.7f, 76.0f}},
    {(char *)"color84", {51, 255, 102}, {88.5f, -77.4f, 58.4f}},
    {(char *)"color85", {51, 255, 153}, {89.2f, -69.9f, 35.4f}},
    {(char *)"color86", {51, 255, 204}, {90.1f, -59.2f, 10.9f}},
    {(char *)"color87", {51, 255, 255}, {91.4f, -46.0f, -13.6f}},
    {(char *)"color88", {102, 0, 0}, {19.3f, 40.9f, 29.7f}},
    {(char *)"color89", {102, 0, 51}, {20.3f, 43.5f, -0.4f}},
    {(char *)"color90", {102, 0, 102}, {22.9f, 50.1f, -31.0f}},
    {(char *)"color91", {102, 0, 153}, {27.1f, 59.6f, -56.6f}},
    {(char *)"color92", {102, 0, 204}, {32.2f, 70.4f, -78.6f}},
    {(char *)"color93", {102, 0, 255}, {37.9f, 81.5f, -98.3f}},
    {(char *)"color94", {102, 51, 0}, {27.3f, 19.6f, 37.7f}},
    {(char *)"color95", {102, 51, 51}, {27.9f, 22.8f, 10.4f}},
    {(char *)"color96", {102, 51, 102}, {29.8f, 31.0f, -20.2f}},
    {(char *)"color97", {102, 51, 153}, {32.9f, 42.9f, -47.1f}},
    {(char *)"color98", {102, 51, 204}, {37.0f, 56.2f, -70.7f}},
    {(char *)"color99", {102, 51, 255}, {41.9f, 69.7f, -91.8f}},
    {(char *)"color100", {102, 102, 0}, {41.7f, -11.0f, 48.2f}},
    {(char *)"color101", {102, 102, 51}, {42.1f, -8.1f, 28.6f}},
    {(char *)"color102", {102, 102, 102}, {43.2f, -0.0f, 0.0f}},
    {(char *)"color103", {102, 102, 153}, {45.1f, 12.5f, -27.7f}},
    {(char *)"color104", {102, 102, 204}, {47.9f, 27.6f, -53.2f}},
    {(char *)"color105", {102, 102, 255}, {51.3f, 43.6f, -76.3f}},
    {(char *)"color106", {102, 153, 0}, {57.7f, -36.5f, 60.2f}},
    {(char *)"color107", {102, 153, 51}, {57.9f, -34.2f, 46.5f}},
    {(char *)"color108", {102, 153, 102}, {58.6f, -27.7f, 21.6f}},
    {(char *)"color109", {102, 153, 153}, {59.8f, -16.9f, -5.4f}},
    {(char *)"color110", {102, 153, 204}, {61.6f, -2.8f, -31.4f}},
    {(char *)"color111", {102, 153, 255}, {64.0f, 13.2f, -55.8f}},
    {(char *)"color112", {102, 204, 0}, {73.6f, -57.0f, 72.6f}},
    {(char *)"color113", {102, 204, 51}, {73.7f, -55.3f, 62.7f}},
    {(char *)"color114", {102, 204, 102}, {74.2f, -50.2f, 41.6f}},
    {(char *)"color115", {102, 204, 153}, {75.0f, -41.5f, 16.5f}},
    {(char *)"color116", {102, 204, 204}, {76.3f, -29.6f, -9.1f}},
    {(char *)"color117", {102, 204, 255}, {78.0f, -15.2f, -33.9f}},
    {(char *)"color118", {102, 255, 0}, {89.1f, -74.4f, 84.8f}},
    {(char *)"color119", {102, 255, 51}, {89.2f, -73.1f, 77.3f}},
    {(char *)"color120", {102, 255, 102}, {89.5f, -69.2f, 59.8f}},
    {(char *)"color121", {102, 255, 153}, {90.2f, -62.2f, 36.9f}},
    {(char *)"color122", {102, 255, 204}, {91.1f, -52.3f, 12.4f}},
    {(char *)"color123", {102, 255, 255}, {92.4f, -40.0f, -12.1f}},
    {(char *)"color124", {153, 0, 0}, {31.3f, 54.7f, 45.1f}},
    {(char *)"color125", {153, 0, 51}, {31.8f, 56.2f, 17.3f}},
    {(char *)"color126", {153, 0, 102}, {33.4f, 60.4f, -13.9f}},
    {(char *)"color127", {153, 0, 153}, {36.1f, 67.1f, -41.5f}},
    {(char *)"color128", {153, 0, 204}, {39.8f, 75.6f, -65.9f}},
    {(char *)"color129", {153, 0, 255}, {44.2f, 85.3f, -87.7f}},
    {(char *)"color130", {153, 51, 0}, {36.3f, 40.8f, 48.1f}},
    {(char *)"color131", {153, 51, 51}, {36.7f, 42.5f, 23.5f}},
    {(char *)"color132", {153, 51, 102}, {38.0f, 47.3f, -6.9f}},
    {(char *)"color133", {153, 51, 153}, {40.3f, 55.0f, -34.9f}},
    {(char *)"color134", {153, 51, 204}, {43.5f, 64.8f, -59.9f}},
    {(char *)"color135", {153, 51, 255}, {47.4f, 75.7f, -82.4f}},
    {(char *)"color136", {153, 102, 0}, {47.3f, 13.5f, 54.7f}},
    {(char *)"color137", {153, 102, 51}, {47.6f, 15.4f, 36.5f}},
    {(char *)"color138", {153, 102, 102}, {48.6f, 20.6f, 8.4f}},
    {(char *)"color139", {153, 102, 153}, {50.2f, 29.1f, -19.5f}},
    {(char *)"color140", {153, 102, 204}, {52.5f, 40.3f, -45.4f}},
    {(char *)"color141", {153, 102, 255}, {55.6f, 53.0f, -69.2f}},
    {(char *)"color142", {153, 153, 0}, {61.3f, -14.7f, 64.5f}},
    {(char *)"color143", {153, 153, 51}, {61.5f, -13.1f, 51.4f}},
    {(char *)"color144", {153, 153, 102}, {62.1f, -8.2f, 26.9f}},
    {(char *)"color145", {153, 153, 153}, {63.2f, -0.0f, 0.0f}},
    {(char *)"color146", {153, 153, 204}, {64.9f, 11.1f, -26.1f}},
    {(char *)"color147", {153, 153, 255}, {67.1f, 24.3f, -50.8f}},
    {(char *)"color148", {153, 204, 0}, {76.0f, -39.3f, 75.6f}},
    {(char *)"color149", {153, 204, 51}, {76.2f, -37.9f, 65.9f}},
    {(char *)"color150", {153, 204, 102}, {76.6f, -33.8f, 45.2f}},
    {(char *)"color151", {153, 204, 153}, {77.4f, -26.6f, 20.2f}},
    {(char *)"color152", {153, 204, 204}, {78.6f, -16.5f, -5.4f}},
    {(char *)"color153", {153, 204, 255}, {80.3f, -4.0f, -30.2f}},
    {(char *)"color154", {153, 255, 0}, {90.9f, -60.2f, 87.0f}},
    {(char *)"color155", {153, 255, 51}, {91.0f, -59.0f, 79.7f}},
    {(char *)"color156", {153, 255, 102}, {91.3f, -55.6f, 62.3f}},
    {(char *)"color157", {153, 255, 153}, {91.9f, -49.5f, 39.6f}},
    {(char *)"color158", {153, 255, 204}, {92.9f, -40.8f, 15.1f}},
    {(char *)"color159", {153, 255, 255}, {94.1f, -29.7f, -9.4f}},
    {(char *)"color160", {204, 0, 0}, {42.5f, 67.7f, 56.8f}},
    {(char *)"color161", {204, 0, 51}, {42.9f, 68.7f, 33.3f}},
    {(char *)"color162", {204, 0, 102}, {43.9f, 71.5f, 2.9f}},
    {(char *)"color163", {204, 0, 153}, {45.8f, 76.4f, -25.6f}},
    {(char *)"color164", {204, 0, 204}, {48.5f, 83.0f, -51.4f}},
    {(char *)"color165", {204, 0, 255}, {51.9f, 91.0f, -74.8f}},
    {(char *)"color166", {204, 51, 0}, {45.9f, 58.1f, 58.2f}},
    {(char *)"color167", {204, 51, 51}, {46.2f, 59.1f, 37.1f}},
    {(char *)"color168", {204, 51, 102}, {47.2f, 62.2f, 7.6f}},
    {(char *)"color169", {204, 51, 153}, {48.9f, 67.5f, -20.9f}},
    {(char *)"color170", {204, 51, 204}, {51.3f, 74.8f, -46.9f}},
    {(char *)"color171", {204, 51, 255}, {54.5f, 83.4f, -70.7f}},
    {(char *)"color172", {204, 102, 0}, {54.4f, 35.7f, 62.7f}},
    {(char *)"color173", {204, 102, 51}, {54.6f, 36.9f, 46.1f}},
    {(char *)"color174", {204, 102, 102}, {55.4f, 40.3f, 18.9f}},
    {(char *)"color175", {204, 102, 153}, {56.7f, 46.3f, -9.0f}},
    {(char *)"color176", {204, 102, 204}, {58.7f, 54.4f, -35.3f}},
    {(char *)"color177", {204, 102, 255}, {61.3f, 64.3f, -59.7f}},
    {(char *)"color178", {204, 153, 0}, {66.2f, 8.3f, 70.3f}},
    {(char *)"color179", {204, 153, 51}, {66.4f, 9.5f, 57.9f}},
    {(char *)"color180", {204, 153, 102}, {67.0f, 13.0f, 34.1f}},
    {(char *)"color181", {204, 153, 153}, {68.0f, 19.1f, 7.5f}},
    {(char *)"color182", {204, 153, 204}, {69.5f, 27.6f, -18.7f}},
    {(char *)"color183", {204, 153, 255}, {71.4f, 38.1f, -43.6f}},
    {(char *)"color184", {204, 204, 0}, {79.6f, -18.2f, 79.9f}},
    {(char *)"color185", {204, 204, 51}, {79.8f, -17.1f, 70.5f}},
    {(char *)"color186", {204, 204, 102}, {80.2f, -13.9f, 50.3f}},
    {(char *)"color187", {204, 204, 153}, {80.9f, -8.2f, 25.5f}},
    {(char *)"color188", {204, 204, 204}, {82.0f, -0.0f, 0.0f}},
    {(char *)"color189", {204, 204, 255}, {83.6f, 10.3f, -24.9f}},
    {(char *)"color190", {204, 255, 0}, {93.6f, -41.9f, 90.3f}},
    {(char *)"color191", {204, 255, 51}, {93.7f, -41.0f, 83.1f}},
    {(char *)"color192", {204, 255, 102}, {94.0f, -38.1f, 66.1f}},
    {(char *)"color193", {204, 255, 153}, {94.6f, -33.0f, 43.5f}},
    {(char *)"color194", {204, 255, 204}, {95.5f, -25.6f, 19.2f}},
    {(char *)"color195", {204, 255, 255}, {96.6f, -16.0f, -5.3f}},
    {(char *)"color196", {255, 0, 0}, {53.2f, 80.1f, 67.2f}},
    {(char *)"color197", {255, 0, 51}, {53.5f, 80.8f, 47.8f}},
    {(char *)"color198", {255, 0, 102}, {54.3f, 82.9f, 18.9f}},
    {(char *)"color199", {255, 0, 153}, {55.7f, 86.5f, -9.7f}},
    {(char *)"color200", {255, 0, 204}, {57.7f, 91.7f, -36.3f}},
    {(char *)"color201", {255, 0, 255}, {60.3f, 98.2f, -60.8f}},
    {(char *)"color202", {255, 51, 0}, {55.7f, 73.0f, 68.1f}},
    {(char *)"color203", {255, 51, 51}, {56.0f, 73.7f, 50.3f}},
    {(char *)"color204", {255, 51, 102}, {56.7f, 75.9f, 22.1f}},
    {(char *)"color205", {255, 51, 153}, {58.0f, 79.8f, -6.3f}},
    {(char *)"color206", {255, 51, 204}, {59.9f, 85.3f, -32.9f}},
    {(char *)"color207", {255, 51, 255}, {62.4f, 92.2f, -57.5f}},
    {(char *)"color208", {255, 102, 0}, {62.3f, 55.0f, 71.3f}},
    {(char *)"color209", {255, 102, 51}, {62.5f, 55.8f, 56.6f}},
    {(char *)"color210", {255, 102, 102}, {63.1f, 58.2f, 30.6f}},
    {(char *)"color211", {255, 102, 153}, {64.2f, 62.5f, 2.9f}},
    {(char *)"color212", {255, 102, 204}, {65.8f, 68.6f, -23.7f}},
    {(char *)"color213", {255, 102, 255}, {68.0f, 76.2f, -48.6f}},
    {(char *)"color214", {255, 153, 0}, {72.3f, 30.2f, 77.2f}},
    {(char *)"color215", {255, 153, 51}, {72.4f, 31.0f, 65.7f}},
    {(char *)"color216", {255, 153, 102}, {72.9f, 33.6f, 42.8f}},
    {(char *)"color217", {255, 153, 153}, {73.8f, 38.1f, 16.5f}},
    {(char *)"color218", {255, 153, 204}, {75.1f, 44.6f, -9.7f}},
    {(char *)"color219", {255, 153, 255}, {76.8f, 52.8f, -34.8f}},
    {(char *)"color220", {255, 204, 0}, {84.2f, 3.7f, 85.2f}},
    {(char *)"color221", {255, 204, 51}, {84.3f, 4.5f, 76.4f}},
    {(char *)"color222", {255, 204, 102}, {84.7f, 7.0f, 56.7f}},
    {(char *)"color223", {255, 204, 153}, {85.4f, 11.5f, 32.3f}},
    {(char *)"color224", {255, 204, 204}, {86.4f, 18.0f, 6.9f}},
    {(char *)"color225", {255, 204, 255}, {87.8f, 26.4f, -18.1f}},
    {(char *)"color226", {255, 255, 0}, {97.1f, -21.6f, 94.5f}},
    {(char *)"color227", {255, 255, 51}, {97.2f, -20.8f, 87.5f}},
    {(char *)"color228", {255, 255, 102}, {97.5f, -18.4f, 70.9f}},
    {(char *)"color229", {255, 255, 153}, {98.1f, -14.2f, 48.7f}},
    {(char *)"color230", {255, 255, 204}, {98.9f, -8.1f, 24.5f}},
    {(char *)"color231", {255, 255, 255}, {100.0f, -0.0f, 0.0f}},
    {(char *)"grey3", {8, 8, 8}, {2.2f, -0.0f, 0.0f}},
    {(char *)"grey4", {18, 18, 18}, {5.5f, -0.0f, 0.0f}},
    {(char *)"grey5", {28, 28, 28}, {10.3f, -0.0f, 0.0f}},
    {(char *)"grey6", {38, 38, 38}, {15.2f, -0.0f, 0.0f}},
    {(char *)"grey7", {48, 48, 48}, {19.9f, -0.0f, 0.0f}},
    {(char *)"grey8", {58, 58, 58}, {24.4f, -0.0f, 0.0f}},
    {(char *)"grey9", {68, 68, 68}, {28.9f, -0.0f, 0.0f}},
    {(char *)"grey10", {78, 78, 78}, {33.2f, -0.0f, 0.0f}},
    {(char *)"grey11", {88, 88, 88}, {37.4f, -0.0f, 0.0f}},
    {(char *)"grey12", {98, 98, 98}, {41.6f, -0.0f, 0.0f}},
    {(char *)"grey13", {108, 108, 108}, {45.6f, -0.0f, 0.0f}},
    {(char *)"grey14", {118, 118, 118}, {49.6f, -0.0f, 0.0f}},
    {(char *)"grey15", {128, 128, 128}, {53.6f, -0.0f, 0.0f}},
    {(char *)"grey16", {138, 138, 138}, {57.5f, -0.0f, 0.0f}},
    {(char *)"grey17", {148, 148, 148}, {61.3f, -0.0f, 0.0f}},
    {(char *)"grey18", {158, 158, 158}, {65.1f, -0.0f, 0.0f}},
    {(char *)"grey19", {168, 168, 168}, {68.9f, -0.0f, 0.0f}},
    {(char *)"grey20", {178, 178, 178}, {72.6f, -0.0f, 0.0f}},
    {(char *)"grey21", {188, 188, 188}, {76.2f, -0.0f, 0.0f}},
    {(char *)"grey22", {198, 198, 198}, {79.9f, -0.0f, 0.0f}},
    {(char *)"grey23", {208, 208, 208}, {83.5f, -0.0f, 0.0f}},
    {(char *)"grey24", {218, 218, 218}, {87.1f, -0.0f, 0.0f}},
    {(char *)"grey25", {228, 228, 228}, {90.6f, -0.0f, 0.0f}},
    {(char *)"grey26", {238, 238, 238}, {94.1f, -0.0f, 0.0f}},
    {(char *)NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}}};

COLORINFO cssColors[] = {
    {(char *)"black", {0, 0, 0}, {0.0f, 0.0f, 0.0f}},
    {(char *)"navy", {0, 0, 128}, {13.0f, 47.5f, -64.7f}},
    {(char *)"darkblue", {0, 0, 139}, {14.8f, 50.4f, -68.7f}},
    {(char *)"mediumblue", {0, 0, 205}, {25.0f, 67.2f, -91.5f}},
    {(char *)"blue", {0, 0, 255}, {32.3f, 79.2f, -107.9f}},
    {(char *)"darkgreen", {0, 100, 0}, {36.2f, -43.4f, 41.9f}},
    {(char *)"green", {0, 128, 0}, {46.2f, -51.7f, 49.9f}},
    {(char *)"teal", {0, 128, 128}, {48.3f, -28.8f, -8.5f}},
    {(char *)"darkcyan", {0, 139, 139}, {52.2f, -30.6f, -9.0f}},
    {(char *)"deepskyblue", {0, 191, 255}, {72.5f, -17.7f, -42.5f}},
    {(char *)"darkturquoise", {0, 206, 209}, {75.3f, -40.0f, -13.5f}},
    {(char *)"mediumspringgreen", {0, 250, 154}, {87.3f, -70.7f, 32.5f}},
    {(char *)"lime", {0, 255, 0}, {87.7f, -86.2f, 83.2f}},
    {(char *)"springgreen", {0, 255, 127}, {88.5f, -76.9f, 47.0f}},
    {(char *)"aqua", {0, 255, 255}, {91.1f, -48.1f, -14.1f}},
    {(char *)"cyan", {0, 255, 255}, {91.1f, -48.1f, -14.1f}},
    {(char *)"midnightblue", {25, 25, 112}, {15.9f, 31.7f, -49.6f}},
    {(char *)"dodgerblue", {30, 144, 255}, {59.4f, 10.0f, -63.4f}},
    {(char *)"lightseagreen", {32, 178, 170}, {65.8f, -37.5f, -6.3f}},
    {(char *)"forestgreen", {34, 139, 34}, {50.6f, -49.6f, 45.0f}},
    {(char *)"seagreen", {46, 139, 87}, {51.5f, -39.7f, 20.1f}},
    {(char *)"darkslategrey", {47, 79, 79}, {31.3f, -11.7f, -3.7f}},
    {(char *)"limegreen", {50, 205, 50}, {72.6f, -67.1f, 61.4f}},
    {(char *)"mediumseagreen", {60, 179, 113}, {65.3f, -48.2f, 24.3f}},
    {(char *)"turquoise", {64, 224, 208}, {81.3f, -44.1f, -4.0f}},
    {(char *)"royalblue", {65, 105, 225}, {47.8f, 26.3f, -65.3f}},
    {(char *)"steelblue", {70, 130, 180}, {52.5f, -4.1f, -32.2f}},
    {(char *)"darkslateblue", {72, 61, 139}, {30.8f, 26.1f, -42.1f}},
    {(char *)"mediumturquoise", {72, 209, 204}, {76.9f, -37.4f, -8.4f}},
    {(char *)"indigo", {75, 0, 130}, {20.5f, 51.7f, -53.3f}},
    {(char *)"darkolivegreen", {85, 107, 47}, {42.2f, -18.8f, 30.6f}},
    {(char *)"cadetblue", {95, 158, 160}, {61.2f, -19.7f, -7.4f}},
    {(char *)"cornflowerblue", {100, 149, 237}, {61.9f, 9.3f, -49.3f}},
    {(char *)"rebeccapurple", {102, 51, 153}, {32.9f, 42.9f, -47.1f}},
    {(char *)"mediumaquamarine", {102, 205, 170}, {75.7f, -38.3f, 8.3f}},
    {(char *)"dimgrey", {105, 105, 105}, {44.4f, -0.0f, 0.0f}},
    {(char *)"slateblue", {106, 90, 205}, {45.3f, 36.0f, -57.8f}},
    {(char *)"olivedrab", {107, 142, 35}, {54.7f, -28.2f, 49.7f}},
    {(char *)"slategrey", {119, 136, 153}, {55.9f, -2.2f, -11.1f}},
    {(char *)"lightslategrey", {119, 136, 153}, {55.9f, -2.2f, -11.1f}},
    {(char *)"mediumslateblue", {123, 104, 238}, {52.2f, 41.1f, -65.4f}},
    {(char *)"lawngreen", {124, 252, 0}, {88.9f, -67.9f, 85.0f}},
    {(char *)"chartreuse", {127, 255, 0}, {89.9f, -68.1f, 85.8f}},
    {(char *)"aquamarine", {127, 255, 212}, {92.0f, -45.5f, 9.7f}},
    {(char *)"maroon", {128, 0, 0}, {25.5f, 48.0f, 38.1f}},
    {(char *)"purple", {128, 0, 128}, {29.8f, 58.9f, -36.5f}},
    {(char *)"olive", {128, 128, 0}, {51.9f, -12.9f, 56.7f}},
    {(char *)"grey", {128, 128, 128}, {53.6f, -0.0f, 0.0f}},
    {(char *)"skyblue", {135, 206, 235}, {79.2f, -14.8f, -21.3f}},
    {(char *)"lightskyblue", {135, 206, 250}, {79.7f, -10.8f, -28.5f}},
    {(char *)"blueviolet", {138, 43, 226}, {42.2f, 69.8f, -74.8f}},
    {(char *)"darkred", {139, 0, 0}, {28.1f, 51.0f, 41.3f}},
    {(char *)"darkmagenta", {139, 0, 139}, {32.6f, 62.6f, -38.7f}},
    {(char *)"saddlebrown", {139, 69, 19}, {37.5f, 26.4f, 41.0f}},
    {(char *)"darkseagreen", {143, 188, 143}, {72.1f, -23.8f, 18.0f}},
    {(char *)"lightgreen", {144, 238, 144}, {86.5f, -46.3f, 36.9f}},
    {(char *)"mediumpurple", {147, 112, 219}, {55.0f, 36.8f, -50.1f}},
    {(char *)"darkviolet", {148, 0, 211}, {39.6f, 76.3f, -70.4f}},
    {(char *)"palegreen", {152, 251, 152}, {90.7f, -48.3f, 38.5f}},
    {(char *)"darkorchid", {153, 50, 204}, {43.4f, 65.2f, -60.1f}},
    {(char *)"yellowgreen", {154, 205, 50}, {76.5f, -38.0f, 66.6f}},
    {(char *)"sienna", {160, 82, 45}, {43.8f, 29.3f, 35.6f}},
    {(char *)"brown", {165, 42, 42}, {37.5f, 49.7f, 30.5f}},
    {(char *)"darkgrey", {169, 169, 169}, {69.2f, -0.0f, 0.0f}},
    {(char *)"lightblue", {173, 216, 230}, {83.8f, -10.9f, -11.5f}},
    {(char *)"greenyellow", {173, 255, 47}, {92.0f, -52.5f, 81.9f}},
    {(char *)"paleturquoise", {175, 238, 238}, {90.1f, -19.6f, -6.4f}},
    {(char *)"lightsteelblue", {176, 196, 222}, {78.5f, -1.3f, -15.2f}},
    {(char *)"powderblue", {176, 224, 230}, {86.1f, -14.1f, -8.0f}},
    {(char *)"firebrick", {178, 34, 34}, {39.1f, 55.9f, 37.6f}},
    {(char *)"darkgoldenrod", {184, 134, 11}, {59.2f, 9.9f, 62.7f}},
    {(char *)"mediumorchid", {186, 85, 211}, {53.6f, 59.1f, -47.4f}},
    {(char *)"rosybrown", {188, 143, 143}, {63.6f, 17.0f, 6.6f}},
    {(char *)"darkkhaki", {189, 183, 107}, {73.4f, -8.8f, 39.3f}},
    {(char *)"silver", {192, 192, 192}, {77.7f, -0.0f, 0.0f}},
    {(char *)"mediumvioletred", {199, 21, 133}, {44.8f, 71.0f, -15.2f}},
    {(char *)"indianred", {205, 92, 92}, {53.4f, 44.8f, 22.1f}},
    {(char *)"peru", {205, 133, 63}, {61.8f, 21.4f, 47.9f}},
    {(char *)"chocolate", {210, 105, 30}, {56.0f, 37.1f, 56.7f}},
    {(char *)"tan", {210, 180, 140}, {75.0f, 5.0f, 24.4f}},
    {(char *)"lightgrey", {211, 211, 211}, {84.6f, -0.0f, 0.0f}},
    {(char *)"thistle", {216, 191, 216}, {80.1f, 13.2f, -9.2f}},
    {(char *)"orchid", {218, 112, 214}, {62.8f, 55.3f, -34.4f}},
    {(char *)"goldenrod", {218, 165, 32}, {70.8f, 8.5f, 68.8f}},
    {(char *)"palevioletred", {219, 112, 147}, {60.6f, 45.5f, 0.4f}},
    {(char *)"crimson", {220, 20, 60}, {47.0f, 70.9f, 33.6f}},
    {(char *)"gainsboro", {220, 220, 220}, {87.8f, -0.0f, 0.0f}},
    {(char *)"plum", {221, 160, 221}, {73.4f, 32.5f, -22.0f}},
    {(char *)"burlywood", {222, 184, 135}, {77.0f, 7.0f, 30.0f}},
    {(char *)"lightcyan", {224, 255, 255}, {97.9f, -9.9f, -3.4f}},
    {(char *)"lavender", {230, 230, 250}, {91.8f, 3.7f, -9.7f}},
    {(char *)"darksalmon", {233, 150, 122}, {69.9f, 28.2f, 27.7f}},
    {(char *)"violet", {238, 130, 238}, {69.7f, 56.4f, -36.8f}},
    {(char *)"palegoldenrod", {238, 232, 170}, {91.1f, -7.3f, 31.0f}},
    {(char *)"lightcoral", {240, 128, 128}, {66.2f, 42.8f, 19.6f}},
    {(char *)"khaki", {240, 230, 140}, {90.3f, -9.0f, 45.0f}},
    {(char *)"aliceblue", {240, 248, 255}, {97.2f, -1.3f, -4.3f}},
    {(char *)"honeydew", {240, 255, 240}, {98.6f, -7.6f, 5.5f}},
    {(char *)"azure", {240, 255, 255}, {98.9f, -4.9f, -1.7f}},
    {(char *)"sandybrown", {244, 164, 96}, {74.0f, 23.0f, 46.8f}},
    {(char *)"wheat", {245, 222, 179}, {89.4f, 1.5f, 24.0f}},
    {(char *)"beige", {245, 245, 220}, {95.9f, -4.2f, 12.0f}},
    {(char *)"whitesmoke", {245, 245, 245}, {96.5f, -0.0f, 0.0f}},
    {(char *)"mintcream", {245, 255, 250}, {99.2f, -4.2f, 1.2f}},
    {(char *)"ghostwhite", {248, 248, 255}, {97.8f, 1.2f, -3.3f}},
    {(char *)"salmon", {250, 128, 114}, {67.3f, 45.2f, 29.1f}},
    {(char *)"antiquewhite", {250, 235, 215}, {93.7f, 1.8f, 11.5f}},
    {(char *)"linen", {250, 240, 230}, {95.3f, 1.7f, 6.0f}},
    {(char *)"lightgoldenrodyellow", {250, 250, 210}, {97.4f, -6.5f, 19.2f}},
    {(char *)"oldlace", {253, 245, 230}, {96.8f, 0.2f, 8.2f}},
    {(char *)"red", {255, 0, 0}, {53.2f, 80.1f, 67.2f}},
    {(char *)"fuchsia", {255, 0, 255}, {60.3f, 98.2f, -60.8f}},
    {(char *)"magenta", {255, 0, 255}, {60.3f, 98.2f, -60.8f}},
    {(char *)"deeppink", {255, 20, 147}, {56.0f, 84.5f, -5.7f}},
    {(char *)"orangered", {255, 69, 0}, {57.6f, 67.8f, 69.0f}},
    {(char *)"tomato", {255, 99, 71}, {62.2f, 57.9f, 46.4f}},
    {(char *)"hotpink", {255, 105, 180}, {65.5f, 64.2f, -10.6f}},
    {(char *)"coral", {255, 127, 80}, {67.3f, 45.4f, 47.5f}},
    {(char *)"darkorange", {255, 140, 0}, {69.5f, 36.8f, 75.5f}},
    {(char *)"lightsalmon", {255, 160, 122}, {74.7f, 31.5f, 34.5f}},
    {(char *)"orange", {255, 165, 0}, {74.9f, 23.9f, 78.9f}},
    {(char *)"lightpink", {255, 182, 193}, {81.1f, 28.0f, 5.0f}},
    {(char *)"pink", {255, 192, 203}, {83.6f, 24.1f, 3.3f}},
    {(char *)"gold", {255, 215, 0}, {86.9f, -1.9f, 87.1f}},
    {(char *)"peachpuff", {255, 218, 185}, {89.4f, 8.1f, 21.0f}},
    {(char *)"navajowhite", {255, 222, 173}, {90.1f, 4.5f, 28.3f}},
    {(char *)"moccasin", {255, 228, 181}, {91.7f, 2.4f, 26.4f}},
    {(char *)"bisque", {255, 228, 196}, {92.0f, 4.4f, 19.0f}},
    {(char *)"mistyrose", {255, 228, 225}, {92.7f, 8.7f, 4.8f}},
    {(char *)"blanchedalmond", {255, 235, 205}, {93.9f, 2.1f, 17.0f}},
    {(char *)"papayawhip", {255, 239, 213}, {95.1f, 1.3f, 14.5f}},
    {(char *)"lavenderblush", {255, 240, 245}, {96.1f, 5.9f, -0.6f}},
    {(char *)"seashell", {255, 245, 238}, {97.1f, 2.2f, 4.6f}},
    {(char *)"cornsilk", {255, 248, 220}, {97.5f, -2.2f, 14.3f}},
    {(char *)"lemonchiffon", {255, 250, 205}, {97.6f, -5.4f, 22.2f}},
    {(char *)"floralwhite", {255, 250, 240}, {98.4f, -0.0f, 5.4f}},
    {(char *)"snow", {255, 250, 250}, {98.6f, 1.7f, 0.6f}},
    {(char *)"yellow", {255, 255, 0}, {97.1f, -21.6f, 94.5f}},
    {(char *)"lightyellow", {255, 255, 224}, {99.3f, -5.1f, 14.8f}},
    {(char *)"ivory", {255, 255, 240}, {99.6f, -2.6f, 7.2f}},
    {(char *)"white", {255, 255, 255}, {100.0f, -0.0f, 0.0f}},
    {(char *)NULL, {0, 0, 0}, {0.0f, 0.0f, 0.0f}}};
