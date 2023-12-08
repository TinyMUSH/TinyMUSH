#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <stdbool.h>
#include <ctype.h>
#include <math.h>

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
 * @brief Return the Delta-E between two colors
 *
 * Delta E is a metric for understanding how the human eye perceives color difference.
 * The term delta comes from mathematics, meaning change in a variable or function.
 * The suffix E references the German word Empfindung, which broadly means sensation.
 *
 * Delta E  Perception
 * <= 1.0   Not perceptible by human eyes.
 * 1 - 2    Perceptible through close observation.
 * 2 - 10   Perceptible at a glance.
 * 11 - 49  Colors are more similar than opposite
 * 100      Colors are exact opposite
 *
 * @param c1 First color to compare
 * @param c2 Second color to compare
 * @return float Delta-E between the two colors
 *
 */
float getColorDeltaE(rgbColor c1, rgbColor c2)
{
    CIELABColor labC1 = xyzToCIELAB(rgbToXyz(c1)), labC2 = xyzToCIELAB(rgbToXyz(c2));

    return sqrtf(powf(labC1.l - labC2.l, 2) + powf(labC1.a - labC2.a, 2) + powf(labC1.b - labC2.b, 2));
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
    COLORMATCH cm = {101, {NULL, {0, 0, 0}}};
    float deltaE;

    for (COLORINFO *cinfo = palette; cinfo->name != NULL; cinfo++)
    {
        deltaE = getColorDeltaE(rgb, cinfo->rgb);
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
    else if ((((rgb.r & 255) == 255) || rgb.r == 0) && (((rgb.g & 255) == 255) || rgb.g == 0) && (((rgb.b & 255) == 255) || rgb.b == 0))
    {
        return (rgb.r & 1) + (rgb.g & 2) + (rgb.b & 4) + 8;
    }
    else if ((rgb.r == rgb.g) && (rgb.r == rgb.b))
    {
        return ((rgb.r - 8) / 10) + 232;
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
    return RGB2Ansi(X112RGB(color));
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

    if (cm.color.rgb.r + cm.color.rgb.g + cm.color.rgb.b == 0)
    {
        return 0;
    }
    else if (((cm.color.rgb.r & 128) == cm.color.rgb.r) && ((cm.color.rgb.g & 128) == cm.color.rgb.g) && ((cm.color.rgb.b & 128) == cm.color.rgb.b))
    {
        uint8_t c = (cm.color.rgb.r >> 7) + (cm.color.rgb.g >> 6) + (cm.color.rgb.b >> 5);
        return c == 7 ? 8 : c;
    }
    else if ((cm.color.rgb.r == 192) && (cm.color.rgb.g == 192) && (cm.color.rgb.b == 192))
    {
        return 7;
    }
    else if ((((cm.color.rgb.r & 255) == 255) || cm.color.rgb.r == 0) && (((cm.color.rgb.g & 255) == 255) || cm.color.rgb.g == 0) && (((cm.color.rgb.b & 255) == 255) || cm.color.rgb.b == 0))
    {
        return (cm.color.rgb.r & 1) + (cm.color.rgb.g & 2) + (cm.color.rgb.b & 4) + 8;
    }

    return 0;
}

VT100ATTR decodeVT100(const char **ansi)
{
    int *codes = NULL;
    int index = 0;
    VT100ATTR attr = {{ANSICOLORTYPE_NONE, {0, 0, 0}}, {ANSICOLORTYPE_NONE, {0, 0, 0}}, false};

    if (ansi == NULL)
    {
        return attr;
    }

    if (*ansi == NULL)
    {
        return attr;
    }

    if (**ansi != '\e')
    {
        return attr;
    }

    (*ansi)++;

    if (**ansi == '[')
    {
        (*ansi)++;
        if (isdigit(**ansi))
        {
            while (isdigit(**ansi) || (**ansi) == ';')
            {
                if (isdigit(**ansi))
                {
                    int *tmp = (int *)realloc(codes, (index + 1) * sizeof(int));
                    if (tmp != NULL)
                    {
                        codes = tmp;
                        codes[index] = 0;
                        while (isdigit(**ansi))
                        {
                            codes[index] = codes[index] * 10 + (**ansi - '0');
                            (*ansi)++;
                        }
                    }
                    else
                    {
                        free(codes);
                        codes = NULL;
                        break;
                    }
                }
                else
                {
                    (*ansi)++;
                    index++;
                }
            }
        }
        if (**ansi == 'm')
        {
            if (codes != NULL)
            {
                index++;
                (*ansi)++;
            }
        }
        else
        {
            if (codes != NULL)
            {
                free(codes);
                codes = NULL;
            }
        }
    }

    if (codes != NULL)
    {
        for (int i = 0; i < index; i++)
        {
            switch (codes[i])
            {
            case 0:
                attr.reset = true;
                break;
            case 38:
                i++;
                switch (codes[i])
                {
                case 2:
                    i++;
                    attr.foreground.type = ANSICOLORTYPE_TRUECOLORS;
                    attr.foreground.rgb.r = codes[i];
                    i++;
                    attr.foreground.rgb.g = codes[i];
                    i++;
                    attr.foreground.rgb.b = codes[i];
                    break;
                case 5:
                    i++;
                    attr.foreground.type = ANSICOLORTYPE_XTERM;
                    attr.foreground.rgb = xtermColor[codes[i]].rgb;
                    break;
                }
                break;
            case 48:
                i++;
                switch (codes[i])
                {
                case 2:
                    i++;
                    attr.background.type = ANSICOLORTYPE_TRUECOLORS;
                    attr.background.rgb.r = codes[i];
                    i++;
                    attr.background.rgb.g = codes[i];
                    i++;
                    attr.background.rgb.b = codes[i];
                    break;
                case 5:
                    i++;
                    attr.background.type = ANSICOLORTYPE_XTERM;
                    attr.background.rgb = xtermColor[codes[i]].rgb;
                    break;
                }
                break;
            default:
                if (codes[i] >= 30 && codes[i] <= 37)
                {
                    attr.foreground.type = ANSICOLORTYPE_STANDARD;
                    attr.foreground.rgb = xtermColor[codes[i] - 30].rgb;
                }
                else if (codes[i] >= 40 && codes[i] <= 47)
                {
                    attr.background.type = ANSICOLORTYPE_STANDARD;
                    attr.background.rgb = xtermColor[codes[i] - 40].rgb;
                }
                else if (codes[i] >= 90 && codes[i] <= 97)
                {
                    attr.foreground.type = ANSICOLORTYPE_STANDARD;
                    attr.foreground.rgb = xtermColor[codes[i] - 90 + 8].rgb;
                }
                else if (codes[i] >= 100 && codes[i] <= 107)
                {
                    attr.background.type = ANSICOLORTYPE_STANDARD;
                    attr.background.rgb = xtermColor[codes[i] - 100 + 8].rgb;
                }
                break;
            }
        }
        free(codes);
    }
    return attr;
}

COLORINFO ansiColor[] = {
    {(char *)"black", {0, 0, 0}},        /* ANSI Foreground color 30, Background color 40 */
    {(char *)"maroon", {128, 0, 0}},     /* ANSI Foreground color 31, Background color 41 */
    {(char *)"green", {0, 128, 0}},      /* ANSI Foreground color 32, Background color 42 */
    {(char *)"olive", {128, 128, 0}},    /* ANSI Foreground color 33, Background color 43 */
    {(char *)"navy", {0, 0, 128}},       /* ANSI Foreground color 34, Background color 44 */
    {(char *)"purple", {128, 0, 128}},   /* ANSI Foreground color 35, Background color 45 */
    {(char *)"teal", {0, 128, 128}},     /* ANSI Foreground color 36, Background color 46 */
    {(char *)"silver", {192, 192, 192}}, /* ANSI Foreground color 37, Background color 47 */
    {(char *)"grey", {128, 128, 128}},   /* ANSI Foreground color 90, Background color 100 */
    {(char *)"red", {255, 0, 0}},        /* ANSI Foreground color 91, Background color 101 */
    {(char *)"lime", {0, 255, 0}},       /* ANSI Foreground color 92, Background color 102 */
    {(char *)"yellow", {255, 255, 0}},   /* ANSI Foreground color 93, Background color 103 */
    {(char *)"blue", {0, 0, 255}},       /* ANSI Foreground color 94, Background color 104 */
    {(char *)"fuchsia", {255, 0, 255}},  /* ANSI Foreground color 95, Background color 105 */
    {(char *)"aqua", {0, 255, 255}},     /* ANSI Foreground color 96, Background color 106 */
    {(char *)"white", {255, 255, 255}},  /* ANSI Foreground color 97, Background color 107 */
    {(char *)NULL, {0, 0, 0}}};

COLORINFO xtermColor[] = {
    {(char *)"black", {0, 0, 0}},                 /* Xterm Color 0 */
    {(char *)"maroon", {128, 0, 0}},              /* Xterm Color 1 */
    {(char *)"green", {0, 128, 0}},               /* Xterm Color 2 */
    {(char *)"olive", {128, 128, 0}},             /* Xterm Color 3 */
    {(char *)"navy", {0, 0, 128}},                /* Xterm Color 4 */
    {(char *)"purple", {128, 0, 128}},            /* Xterm Color 5 */
    {(char *)"teal", {0, 128, 128}},              /* Xterm Color 6 */
    {(char *)"silver", {192, 192, 192}},          /* Xterm Color 7 */
    {(char *)"grey", {128, 128, 128}},            /* Xterm Color 8 */
    {(char *)"red", {255, 0, 0}},                 /* Xterm Color 9 */
    {(char *)"lime", {0, 255, 0}},                /* Xterm Color 10 */
    {(char *)"yellow", {255, 255, 0}},            /* Xterm Color 11 */
    {(char *)"blue", {0, 0, 255}},                /* Xterm Color 12 */
    {(char *)"fuchsia", {255, 0, 255}},           /* Xterm Color 13 */
    {(char *)"aqua", {0, 255, 255}},              /* Xterm Color 14 */
    {(char *)"white", {255, 255, 255}},           /* Xterm Color 15 */
    {(char *)"grey0", {0, 0, 0}},                 /* Xterm Color 16 */
    {(char *)"navyblue", {0, 0, 95}},             /* Xterm Color 17 */
    {(char *)"darkblue", {0, 0, 135}},            /* Xterm Color 18 */
    {(char *)"blue1", {0, 0, 175}},               /* Xterm Color 19 */
    {(char *)"blue3", {0, 0, 215}},               /* Xterm Color 20 */
    {(char *)"blue2", {0, 0, 255}},               /* Xterm Color 21 */
    {(char *)"darkgreen", {0, 95, 0}},            /* Xterm Color 22 */
    {(char *)"deepskyblue", {0, 95, 95}},         /* Xterm Color 23 */
    {(char *)"deepskyblue1", {0, 95, 135}},       /* Xterm Color 24 */
    {(char *)"deepskyblue2", {0, 95, 175}},       /* Xterm Color 25 */
    {(char *)"dodgerblue", {0, 95, 215}},         /* Xterm Color 26 */
    {(char *)"dodgerblue1", {0, 95, 255}},        /* Xterm Color 27 */
    {(char *)"green1", {0, 135, 0}},              /* Xterm Color 28 */
    {(char *)"springgreen", {0, 135, 95}},        /* Xterm Color 29 */
    {(char *)"turquoise", {0, 135, 135}},         /* Xterm Color 30 */
    {(char *)"deepskyblue3", {0, 135, 175}},      /* Xterm Color 31 */
    {(char *)"deepskyblue4", {0, 135, 215}},      /* Xterm Color 32 */
    {(char *)"dodgerblue2", {0, 135, 255}},       /* Xterm Color 33 */
    {(char *)"green2", {0, 175, 0}},              /* Xterm Color 34 */
    {(char *)"springgreen1", {0, 175, 95}},       /* Xterm Color 35 */
    {(char *)"darkcyan", {0, 175, 135}},          /* Xterm Color 36 */
    {(char *)"lightseagreen", {0, 175, 175}},     /* Xterm Color 37 */
    {(char *)"deepskyblue5", {0, 175, 215}},      /* Xterm Color 38 */
    {(char *)"deepskyblue6", {0, 175, 255}},      /* Xterm Color 39 */
    {(char *)"green3", {0, 215, 0}},              /* Xterm Color 40 */
    {(char *)"springgreen2", {0, 215, 95}},       /* Xterm Color 41 */
    {(char *)"springgreen3", {0, 215, 135}},      /* Xterm Color 42 */
    {(char *)"cyan", {0, 215, 175}},              /* Xterm Color 43 */
    {(char *)"darkturquoise", {0, 215, 215}},     /* Xterm Color 44 */
    {(char *)"turquoise1", {0, 215, 255}},        /* Xterm Color 45 */
    {(char *)"green4", {0, 255, 0}},              /* Xterm Color 46 */
    {(char *)"springgreen4", {0, 255, 95}},       /* Xterm Color 47 */
    {(char *)"springgreen5", {0, 255, 135}},      /* Xterm Color 48 */
    {(char *)"mediumspringgreen", {0, 255, 175}}, /* Xterm Color 49 */
    {(char *)"cyan1", {0, 255, 215}},             /* Xterm Color 50 */
    {(char *)"cyan2", {0, 255, 255}},             /* Xterm Color 51 */
    {(char *)"darkred", {95, 0, 0}},              /* Xterm Color 52 */
    {(char *)"deeppink", {95, 0, 95}},            /* Xterm Color 53 */
    {(char *)"purple1", {95, 0, 135}},            /* Xterm Color 54 */
    {(char *)"purple2", {95, 0, 175}},            /* Xterm Color 55 */
    {(char *)"purple3", {95, 0, 215}},            /* Xterm Color 56 */
    {(char *)"blueviolet", {95, 0, 255}},         /* Xterm Color 57 */
    {(char *)"orange", {95, 95, 0}},              /* Xterm Color 58 */
    {(char *)"grey37", {95, 95, 95}},             /* Xterm Color 59 */
    {(char *)"mediumpurple", {95, 95, 135}},      /* Xterm Color 60 */
    {(char *)"slateblue", {95, 95, 175}},         /* Xterm Color 61 */
    {(char *)"slateblue1", {95, 95, 215}},        /* Xterm Color 62 */
    {(char *)"royalblue", {95, 95, 255}},         /* Xterm Color 63 */
    {(char *)"chartreuse", {95, 135, 0}},         /* Xterm Color 64 */
    {(char *)"darkseagreen", {95, 135, 95}},      /* Xterm Color 65 */
    {(char *)"paleturquoise", {95, 135, 135}},    /* Xterm Color 66 */
    {(char *)"steelblue", {95, 135, 175}},        /* Xterm Color 67 */
    {(char *)"steelblue1", {95, 135, 215}},       /* Xterm Color 68 */
    {(char *)"cornflowerblue", {95, 135, 255}},   /* Xterm Color 69 */
    {(char *)"chartreuse1", {95, 175, 0}},        /* Xterm Color 70 */
    {(char *)"darkseagreen1", {95, 175, 95}},     /* Xterm Color 71 */
    {(char *)"cadetblue", {95, 175, 135}},        /* Xterm Color 72 */
    {(char *)"cadetblue1", {95, 175, 175}},       /* Xterm Color 73 */
    {(char *)"skyblue", {95, 175, 215}},          /* Xterm Color 74 */
    {(char *)"steelblue2", {95, 175, 255}},       /* Xterm Color 75 */
    {(char *)"chartreuse2", {95, 215, 0}},        /* Xterm Color 76 */
    {(char *)"palegreen", {95, 215, 95}},         /* Xterm Color 77 */
    {(char *)"seagreen", {95, 215, 135}},         /* Xterm Color 78 */
    {(char *)"aquamarine", {95, 215, 175}},       /* Xterm Color 79 */
    {(char *)"mediumturquoise", {95, 215, 215}},  /* Xterm Color 80 */
    {(char *)"steelblue3", {95, 215, 255}},       /* Xterm Color 81 */
    {(char *)"chartreuse3", {95, 255, 0}},        /* Xterm Color 82 */
    {(char *)"seagreen1", {95, 255, 95}},         /* Xterm Color 83 */
    {(char *)"seagreen2", {95, 255, 135}},        /* Xterm Color 84 */
    {(char *)"seagreen3", {95, 255, 175}},        /* Xterm Color 85 */
    {(char *)"aquamarine1", {95, 255, 215}},      /* Xterm Color 86 */
    {(char *)"darkslategray", {95, 255, 255}},    /* Xterm Color 87 */
    {(char *)"darkred", {135, 0, 0}},             /* Xterm Color 88 */
    {(char *)"deeppink1", {135, 0, 95}},          /* Xterm Color 89 */
    {(char *)"darkmagenta", {135, 0, 135}},       /* Xterm Color 90 */
    {(char *)"darkmagenta", {135, 0, 175}},       /* Xterm Color 91 */
    {(char *)"darkviolet", {135, 0, 215}},        /* Xterm Color 92 */
    {(char *)"purple4", {135, 0, 255}},           /* Xterm Color 93 */
    {(char *)"orange1", {135, 95, 0}},            /* Xterm Color 94 */
    {(char *)"lightpink", {135, 95, 95}},         /* Xterm Color 95 */
    {(char *)"plum", {135, 95, 135}},             /* Xterm Color 96 */
    {(char *)"mediumpurple1", {135, 95, 175}},    /* Xterm Color 97 */
    {(char *)"mediumpurple2", {135, 95, 215}},    /* Xterm Color 98 */
    {(char *)"slateblue2", {135, 95, 255}},       /* Xterm Color 99 */
    {(char *)"yellow1", {135, 135, 0}},           /* Xterm Color 100 */
    {(char *)"wheat", {135, 135, 95}},            /* Xterm Color 101 */
    {(char *)"grey53", {135, 135, 135}},          /* Xterm Color 102 */
    {(char *)"lightslategrey", {135, 135, 175}},  /* Xterm Color 103 */
    {(char *)"mediumpurple3", {135, 135, 215}},   /* Xterm Color 104 */
    {(char *)"lightslateblue", {135, 135, 255}},  /* Xterm Color 105 */
    {(char *)"yellow2", {135, 175, 0}},           /* Xterm Color 106 */
    {(char *)"darkolivegreen", {135, 175, 95}},   /* Xterm Color 107 */
    {(char *)"darkseagreen2", {135, 175, 135}},   /* Xterm Color 108 */
    {(char *)"lightskyblue", {135, 175, 175}},    /* Xterm Color 109 */
    {(char *)"lightskyblue1", {135, 175, 215}},   /* Xterm Color 110 */
    {(char *)"skyblue1", {135, 175, 255}},        /* Xterm Color 111 */
    {(char *)"chartreuse4", {135, 215, 0}},       /* Xterm Color 112 */
    {(char *)"darkolivegreen1", {135, 215, 95}},  /* Xterm Color 113 */
    {(char *)"palegreen1", {135, 215, 135}},      /* Xterm Color 114 */
    {(char *)"darkseagreen3", {135, 215, 175}},   /* Xterm Color 115 */
    {(char *)"darkslategray1", {135, 215, 215}},  /* Xterm Color 116 */
    {(char *)"skyblue2", {135, 215, 255}},        /* Xterm Color 117 */
    {(char *)"chartreuse5", {135, 255, 0}},       /* Xterm Color 118 */
    {(char *)"lightgreen", {135, 255, 95}},       /* Xterm Color 119 */
    {(char *)"lightgreen1", {135, 255, 135}},     /* Xterm Color 120 */
    {(char *)"palegreen2", {135, 255, 175}},      /* Xterm Color 121 */
    {(char *)"aquamarine2", {135, 255, 215}},     /* Xterm Color 122 */
    {(char *)"darkslategray2", {135, 255, 255}},  /* Xterm Color 123 */
    {(char *)"red1", {175, 0, 0}},                /* Xterm Color 124 */
    {(char *)"deeppink2", {175, 0, 95}},          /* Xterm Color 125 */
    {(char *)"mediumvioletred", {175, 0, 135}},   /* Xterm Color 126 */
    {(char *)"magenta", {175, 0, 175}},           /* Xterm Color 127 */
    {(char *)"darkviolet", {175, 0, 215}},        /* Xterm Color 128 */
    {(char *)"purple5", {175, 0, 255}},           /* Xterm Color 129 */
    {(char *)"darkorange", {175, 95, 0}},         /* Xterm Color 130 */
    {(char *)"indianred", {175, 95, 95}},         /* Xterm Color 131 */
    {(char *)"hotpink", {175, 95, 135}},          /* Xterm Color 132 */
    {(char *)"mediumorchid", {175, 95, 175}},     /* Xterm Color 133 */
    {(char *)"mediumorchid1", {175, 95, 215}},    /* Xterm Color 134 */
    {(char *)"mediumpurple4", {175, 95, 255}},    /* Xterm Color 135 */
    {(char *)"darkgoldenrod", {175, 135, 0}},     /* Xterm Color 136 */
    {(char *)"lightsalmon", {175, 135, 95}},      /* Xterm Color 137 */
    {(char *)"rosybrown", {175, 135, 135}},       /* Xterm Color 138 */
    {(char *)"grey63", {175, 135, 175}},          /* Xterm Color 139 */
    {(char *)"mediumpurple5", {175, 135, 215}},   /* Xterm Color 140 */
    {(char *)"mediumpurple6", {175, 135, 255}},   /* Xterm Color 141 */
    {(char *)"gold", {175, 175, 0}},              /* Xterm Color 142 */
    {(char *)"darkkhaki", {175, 175, 95}},        /* Xterm Color 143 */
    {(char *)"navajowhite", {175, 175, 135}},     /* Xterm Color 144 */
    {(char *)"grey69", {175, 175, 175}},          /* Xterm Color 145 */
    {(char *)"lightsteelblue", {175, 175, 215}},  /* Xterm Color 146 */
    {(char *)"lightsteelblue1", {175, 175, 255}}, /* Xterm Color 147 */
    {(char *)"yellow3", {175, 215, 0}},           /* Xterm Color 148 */
    {(char *)"darkolivegreen2", {175, 215, 95}},  /* Xterm Color 149 */
    {(char *)"darkseagreen4", {175, 215, 135}},   /* Xterm Color 150 */
    {(char *)"darkseagreen5", {175, 215, 175}},   /* Xterm Color 151 */
    {(char *)"lightcyan", {175, 215, 215}},       /* Xterm Color 152 */
    {(char *)"lightskyblue2", {175, 215, 255}},   /* Xterm Color 153 */
    {(char *)"greenyellow", {175, 255, 0}},       /* Xterm Color 154 */
    {(char *)"darkolivegreen3", {175, 255, 95}},  /* Xterm Color 155 */
    {(char *)"palegreen3", {175, 255, 135}},      /* Xterm Color 156 */
    {(char *)"darkseagreen6", {175, 255, 175}},   /* Xterm Color 157 */
    {(char *)"darkseagreen7", {175, 255, 215}},   /* Xterm Color 158 */
    {(char *)"paleturquoise1", {175, 255, 255}},  /* Xterm Color 159 */
    {(char *)"red2", {215, 0, 0}},                /* Xterm Color 160 */
    {(char *)"deeppink3", {215, 0, 95}},          /* Xterm Color 161 */
    {(char *)"deeppink4", {215, 0, 135}},         /* Xterm Color 162 */
    {(char *)"magenta1", {215, 0, 175}},          /* Xterm Color 163 */
    {(char *)"magenta2", {215, 0, 215}},          /* Xterm Color 164 */
    {(char *)"magenta3", {215, 0, 255}},          /* Xterm Color 165 */
    {(char *)"darkorange1", {215, 95, 0}},        /* Xterm Color 166 */
    {(char *)"indianred1", {215, 95, 95}},        /* Xterm Color 167 */
    {(char *)"hotpink1", {215, 95, 135}},         /* Xterm Color 168 */
    {(char *)"hotpink2", {215, 95, 175}},         /* Xterm Color 169 */
    {(char *)"orchid", {215, 95, 215}},           /* Xterm Color 170 */
    {(char *)"mediumorchid2", {215, 95, 255}},    /* Xterm Color 171 */
    {(char *)"orange2", {215, 135, 0}},           /* Xterm Color 172 */
    {(char *)"lightsalmon1", {215, 135, 95}},     /* Xterm Color 173 */
    {(char *)"lightpink1", {215, 135, 135}},      /* Xterm Color 174 */
    {(char *)"pink", {215, 135, 175}},            /* Xterm Color 175 */
    {(char *)"plum1", {215, 135, 215}},           /* Xterm Color 176 */
    {(char *)"violet", {215, 135, 255}},          /* Xterm Color 177 */
    {(char *)"gold1", {215, 175, 0}},             /* Xterm Color 178 */
    {(char *)"lightgoldenrod", {215, 175, 95}},   /* Xterm Color 179 */
    {(char *)"tan", {215, 175, 135}},             /* Xterm Color 180 */
    {(char *)"mistyrose", {215, 175, 175}},       /* Xterm Color 181 */
    {(char *)"thistle", {215, 175, 215}},         /* Xterm Color 182 */
    {(char *)"plum2", {215, 175, 255}},           /* Xterm Color 183 */
    {(char *)"yellow4", {215, 215, 0}},           /* Xterm Color 184 */
    {(char *)"khaki", {215, 215, 95}},            /* Xterm Color 185 */
    {(char *)"lightgoldenrod1", {215, 215, 135}}, /* Xterm Color 186 */
    {(char *)"lightyellow", {215, 215, 175}},     /* Xterm Color 187 */
    {(char *)"grey84", {215, 215, 215}},          /* Xterm Color 188 */
    {(char *)"lightsteelblue2", {215, 215, 255}}, /* Xterm Color 189 */
    {(char *)"yellow5", {215, 255, 0}},           /* Xterm Color 190 */
    {(char *)"darkolivegreen4", {215, 255, 95}},  /* Xterm Color 191 */
    {(char *)"darkolivegreen5", {215, 255, 135}}, /* Xterm Color 192 */
    {(char *)"darkseagreen8", {215, 255, 175}},   /* Xterm Color 193 */
    {(char *)"honeydew", {215, 255, 215}},        /* Xterm Color 194 */
    {(char *)"lightcyan1", {215, 255, 255}},      /* Xterm Color 195 */
    {(char *)"red3", {255, 0, 0}},                /* Xterm Color 196 */
    {(char *)"deeppink5", {255, 0, 95}},          /* Xterm Color 197 */
    {(char *)"deeppink6", {255, 0, 135}},         /* Xterm Color 198 */
    {(char *)"deeppink7", {255, 0, 175}},         /* Xterm Color 199 */
    {(char *)"magenta4", {255, 0, 215}},          /* Xterm Color 200 */
    {(char *)"magenta5", {255, 0, 255}},          /* Xterm Color 201 */
    {(char *)"orangered", {255, 95, 0}},          /* Xterm Color 202 */
    {(char *)"indianred3", {255, 95, 95}},        /* Xterm Color 203 */
    {(char *)"indianred2", {255, 95, 135}},       /* Xterm Color 204 */
    {(char *)"hotpink3", {255, 95, 175}},         /* Xterm Color 205 */
    {(char *)"hotpink4", {255, 95, 215}},         /* Xterm Color 206 */
    {(char *)"mediumorchid3", {255, 95, 255}},    /* Xterm Color 207 */
    {(char *)"darkorange2", {255, 135, 0}},       /* Xterm Color 208 */
    {(char *)"salmon", {255, 135, 95}},           /* Xterm Color 209 */
    {(char *)"lightcoral", {255, 135, 135}},      /* Xterm Color 210 */
    {(char *)"palevioletred", {255, 135, 175}},   /* Xterm Color 211 */
    {(char *)"orchid1", {255, 135, 215}},         /* Xterm Color 212 */
    {(char *)"orchid2", {255, 135, 255}},         /* Xterm Color 213 */
    {(char *)"orange3", {255, 175, 0}},           /* Xterm Color 214 */
    {(char *)"sandybrown", {255, 175, 95}},       /* Xterm Color 215 */
    {(char *)"lightsalmon2", {255, 175, 135}},    /* Xterm Color 216 */
    {(char *)"lightpink2", {255, 175, 175}},      /* Xterm Color 217 */
    {(char *)"pink1", {255, 175, 215}},           /* Xterm Color 218 */
    {(char *)"plum3", {255, 175, 255}},           /* Xterm Color 219 */
    {(char *)"gold2", {255, 215, 0}},             /* Xterm Color 220 */
    {(char *)"lightgoldenrod2", {255, 215, 95}},  /* Xterm Color 221 */
    {(char *)"lightgoldenrod3", {255, 215, 135}}, /* Xterm Color 222 */
    {(char *)"navajowhite1", {255, 215, 175}},    /* Xterm Color 223 */
    {(char *)"mistyrose1", {255, 215, 215}},      /* Xterm Color 224 */
    {(char *)"thistle1", {255, 215, 255}},        /* Xterm Color 225 */
    {(char *)"yellow6", {255, 255, 0}},           /* Xterm Color 226 */
    {(char *)"lightgoldenrod4", {255, 255, 95}},  /* Xterm Color 227 */
    {(char *)"khaki1", {255, 255, 135}},          /* Xterm Color 228 */
    {(char *)"wheat1", {255, 255, 175}},          /* Xterm Color 229 */
    {(char *)"cornsilk", {255, 255, 215}},        /* Xterm Color 230 */
    {(char *)"grey100", {255, 255, 255}},         /* Xterm Color 231 */
    {(char *)"grey3", {8, 8, 8}},                 /* Xterm Color 232 */
    {(char *)"grey7", {18, 18, 18}},              /* Xterm Color 233 */
    {(char *)"grey11", {28, 28, 28}},             /* Xterm Color 234 */
    {(char *)"grey15", {38, 38, 38}},             /* Xterm Color 235 */
    {(char *)"grey19", {48, 48, 48}},             /* Xterm Color 236 */
    {(char *)"grey23", {58, 58, 58}},             /* Xterm Color 237 */
    {(char *)"grey27", {68, 68, 68}},             /* Xterm Color 238 */
    {(char *)"grey30", {78, 78, 78}},             /* Xterm Color 239 */
    {(char *)"grey35", {88, 88, 88}},             /* Xterm Color 240 */
    {(char *)"grey39", {98, 98, 98}},             /* Xterm Color 241 */
    {(char *)"grey42", {108, 108, 108}},          /* Xterm Color 242 */
    {(char *)"grey46", {118, 118, 118}},          /* Xterm Color 243 */
    {(char *)"grey50", {128, 128, 128}},          /* Xterm Color 244 */
    {(char *)"grey54", {138, 138, 138}},          /* Xterm Color 245 */
    {(char *)"grey58", {148, 148, 148}},          /* Xterm Color 246 */
    {(char *)"grey62", {158, 158, 158}},          /* Xterm Color 247 */
    {(char *)"grey66", {168, 168, 168}},          /* Xterm Color 248 */
    {(char *)"grey70", {178, 178, 178}},          /* Xterm Color 249 */
    {(char *)"grey74", {188, 188, 188}},          /* Xterm Color 250 */
    {(char *)"grey78", {198, 198, 198}},          /* Xterm Color 251 */
    {(char *)"grey82", {208, 208, 208}},          /* Xterm Color 252 */
    {(char *)"grey85", {218, 218, 218}},          /* Xterm Color 253 */
    {(char *)"grey89", {228, 228, 228}},          /* Xterm Color 254 */
    {(char *)"grey93", {238, 238, 238}},          /* Xterm Color 255 */
    {(char *)NULL, {0, 0, 0}}};

COLORINFO cssColors[] = {
    {(char *)"black", {0, 0, 0}},
    {(char *)"navy", {0, 0, 128}},
    {(char *)"darkblue", {0, 0, 139}},
    {(char *)"mediumblue", {0, 0, 205}},
    {(char *)"blue", {0, 0, 255}},
    {(char *)"darkgreen", {0, 100, 0}},
    {(char *)"green", {0, 128, 0}},
    {(char *)"teal", {0, 128, 128}},
    {(char *)"darkcyan", {0, 139, 139}},
    {(char *)"deepskyblue", {0, 191, 255}},
    {(char *)"darkturquoise", {0, 206, 209}},
    {(char *)"mediumspringgreen", {0, 250, 154}},
    {(char *)"lime", {0, 255, 0}},
    {(char *)"springgreen", {0, 255, 127}},
    {(char *)"aqua", {0, 255, 255}},
    {(char *)"cyan", {0, 255, 255}},
    {(char *)"midnightblue", {25, 25, 112}},
    {(char *)"dodgerblue", {30, 144, 255}},
    {(char *)"lightseagreen", {32, 178, 170}},
    {(char *)"forestgreen", {34, 139, 34}},
    {(char *)"seagreen", {46, 139, 87}},
    {(char *)"darkslategrey", {47, 79, 79}},
    {(char *)"limegreen", {50, 205, 50}},
    {(char *)"mediumseagreen", {60, 179, 113}},
    {(char *)"turquoise", {64, 224, 208}},
    {(char *)"royalblue", {65, 105, 225}},
    {(char *)"steelblue", {70, 130, 180}},
    {(char *)"darkslateblue", {72, 61, 139}},
    {(char *)"mediumturquoise", {72, 209, 204}},
    {(char *)"indigo", {75, 0, 130}},
    {(char *)"darkolivegreen", {85, 107, 47}},
    {(char *)"cadetblue", {95, 158, 160}},
    {(char *)"cornflowerblue", {100, 149, 237}},
    {(char *)"rebeccapurple", {102, 51, 153}},
    {(char *)"mediumaquamarine", {102, 205, 170}},
    {(char *)"dimgrey", {105, 105, 105}},
    {(char *)"slateblue", {106, 90, 205}},
    {(char *)"olivedrab", {107, 142, 35}},
    {(char *)"slategrey", {119, 136, 153}},
    {(char *)"lightslategrey", {119, 136, 153}},
    {(char *)"mediumslateblue", {123, 104, 238}},
    {(char *)"lawngreen", {124, 252, 0}},
    {(char *)"chartreuse", {127, 255, 0}},
    {(char *)"aquamarine", {127, 255, 212}},
    {(char *)"maroon", {128, 0, 0}},
    {(char *)"purple", {128, 0, 128}},
    {(char *)"olive", {128, 128, 0}},
    {(char *)"grey", {128, 128, 128}},
    {(char *)"skyblue", {135, 206, 235}},
    {(char *)"lightskyblue", {135, 206, 250}},
    {(char *)"blueviolet", {138, 43, 226}},
    {(char *)"darkred", {139, 0, 0}},
    {(char *)"darkmagenta", {139, 0, 139}},
    {(char *)"saddlebrown", {139, 69, 19}},
    {(char *)"darkseagreen", {143, 188, 143}},
    {(char *)"lightgreen", {144, 238, 144}},
    {(char *)"mediumpurple", {147, 112, 219}},
    {(char *)"darkviolet", {148, 0, 211}},
    {(char *)"palegreen", {152, 251, 152}},
    {(char *)"darkorchid", {153, 50, 204}},
    {(char *)"yellowgreen", {154, 205, 50}},
    {(char *)"sienna", {160, 82, 45}},
    {(char *)"brown", {165, 42, 42}},
    {(char *)"darkgrey", {169, 169, 169}},
    {(char *)"lightblue", {173, 216, 230}},
    {(char *)"greenyellow", {173, 255, 47}},
    {(char *)"paleturquoise", {175, 238, 238}},
    {(char *)"lightsteelblue", {176, 196, 222}},
    {(char *)"powderblue", {176, 224, 230}},
    {(char *)"firebrick", {178, 34, 34}},
    {(char *)"darkgoldenrod", {184, 134, 11}},
    {(char *)"mediumorchid", {186, 85, 211}},
    {(char *)"rosybrown", {188, 143, 143}},
    {(char *)"darkkhaki", {189, 183, 107}},
    {(char *)"silver", {192, 192, 192}},
    {(char *)"mediumvioletred", {199, 21, 133}},
    {(char *)"indianred", {205, 92, 92}},
    {(char *)"peru", {205, 133, 63}},
    {(char *)"chocolate", {210, 105, 30}},
    {(char *)"tan", {210, 180, 140}},
    {(char *)"lightgrey", {211, 211, 211}},
    {(char *)"thistle", {216, 191, 216}},
    {(char *)"orchid", {218, 112, 214}},
    {(char *)"goldenrod", {218, 165, 32}},
    {(char *)"palevioletred", {219, 112, 147}},
    {(char *)"crimson", {220, 20, 60}},
    {(char *)"gainsboro", {220, 220, 220}},
    {(char *)"plum", {221, 160, 221}},
    {(char *)"burlywood", {222, 184, 135}},
    {(char *)"lightcyan", {224, 255, 255}},
    {(char *)"lavender", {230, 230, 250}},
    {(char *)"darksalmon", {233, 150, 122}},
    {(char *)"violet", {238, 130, 238}},
    {(char *)"palegoldenrod", {238, 232, 170}},
    {(char *)"lightcoral", {240, 128, 128}},
    {(char *)"khaki", {240, 230, 140}},
    {(char *)"aliceblue", {240, 248, 255}},
    {(char *)"honeydew", {240, 255, 240}},
    {(char *)"azure", {240, 255, 255}},
    {(char *)"sandybrown", {244, 164, 96}},
    {(char *)"wheat", {245, 222, 179}},
    {(char *)"beige", {245, 245, 220}},
    {(char *)"whitesmoke", {245, 245, 245}},
    {(char *)"mintcream", {245, 255, 250}},
    {(char *)"ghostwhite", {248, 248, 255}},
    {(char *)"salmon", {250, 128, 114}},
    {(char *)"antiquewhite", {250, 235, 215}},
    {(char *)"linen", {250, 240, 230}},
    {(char *)"lightgoldenrodyellow", {250, 250, 210}},
    {(char *)"oldlace", {253, 245, 230}},
    {(char *)"red", {255, 0, 0}},
    {(char *)"fuchsia", {255, 0, 255}},
    {(char *)"magenta", {255, 0, 255}},
    {(char *)"deeppink", {255, 20, 147}},
    {(char *)"orangered", {255, 69, 0}},
    {(char *)"tomato", {255, 99, 71}},
    {(char *)"hotpink", {255, 105, 180}},
    {(char *)"coral", {255, 127, 80}},
    {(char *)"darkorange", {255, 140, 0}},
    {(char *)"lightsalmon", {255, 160, 122}},
    {(char *)"orange", {255, 165, 0}},
    {(char *)"lightpink", {255, 182, 193}},
    {(char *)"pink", {255, 192, 203}},
    {(char *)"gold", {255, 215, 0}},
    {(char *)"peachpuff", {255, 218, 185}},
    {(char *)"navajowhite", {255, 222, 173}},
    {(char *)"moccasin", {255, 228, 181}},
    {(char *)"bisque", {255, 228, 196}},
    {(char *)"mistyrose", {255, 228, 225}},
    {(char *)"blanchedalmond", {255, 235, 205}},
    {(char *)"papayawhip", {255, 239, 213}},
    {(char *)"lavenderblush", {255, 240, 245}},
    {(char *)"seashell", {255, 245, 238}},
    {(char *)"cornsilk", {255, 248, 220}},
    {(char *)"lemonchiffon", {255, 250, 205}},
    {(char *)"floralwhite", {255, 250, 240}},
    {(char *)"snow", {255, 250, 250}},
    {(char *)"yellow", {255, 255, 0}},
    {(char *)"lightyellow", {255, 255, 224}},
    {(char *)"ivory", {255, 255, 240}},
    {(char *)"white", {255, 255, 255}},
    {(char *)NULL, {255, 255, 255}}};