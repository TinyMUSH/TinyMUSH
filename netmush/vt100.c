#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <stdbool.h>
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
    COLORMATCH cm = {101, {NULL, {0,0,0}}};
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

char *TrueColor2VT100(rgbColor rgb, bool background) {
    char *vt = calloc(1024, sizeof(char));

    sprintf(vt, "\e[%d;2;%d;%d;%dm", background ? 48 : 38, rgb.r, rgb.g, rgb.b);

    return(vt);
}

char *X112VT100(uint8_t color, bool background) {
    char *vt = calloc(1024, sizeof(char));

    sprintf(vt, "\e[%d;5;%dm", background ? 48 : 38, color);

    return(vt);
}

char *Ansi2VT100(uint8_t color, bool background) {
    char *vt = calloc(1024, sizeof(char));

    sprintf(vt, "\e[%dm", (background ? 40 : 30) + (color & 7));

    return(vt);
}

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

uint8_t X112Ansi(int color) {
    return RGB2Ansi(X112RGB(color));
}

uint8_t RGB2Ansi(rgbColor rgb){
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

COLORINFO ansiColor[] = {
    {(char *)"black", {0, 0, 0}},
    {(char *)"maroon", {128, 0, 0}},
    {(char *)"green", {0, 128, 0}},
    {(char *)"olive", {128, 128, 0}},
    {(char *)"navy", {0, 0, 128}},
    {(char *)"purple", {128, 0, 128}},
    {(char *)"teal", {0, 128, 128}},
    {(char *)"silver", {192, 192, 192}},
    {(char *)"grey", {128, 128, 128}},
    {(char *)"red", {255, 0, 0}},
    {(char *)"lime", {0, 255, 0}},
    {(char *)"yellow", {255, 255, 0}},
    {(char *)"blue", {0, 0, 255}},
    {(char *)"fuchsia", {255, 0, 255}},
    {(char *)"aqua", {0, 255, 255}},
    {(char *)"white", {255, 255, 255}},
    {(char *)NULL, {0,0,0}}};

COLORINFO xtermColor[] = {
    {(char *)"black", {0, 0, 0}},
    {(char *)"maroon", {128, 0, 0}},
    {(char *)"green", {0, 128, 0}},
    {(char *)"olive", {128, 128, 0}},
    {(char *)"navy", {0, 0, 128}},
    {(char *)"purple", {128, 0, 128}},
    {(char *)"teal", {0, 128, 128}},
    {(char *)"silver", {192, 192, 192}},
    {(char *)"grey", {128, 128, 128}},
    {(char *)"red", {255, 0, 0}},
    {(char *)"lime", {0, 255, 0}},
    {(char *)"yellow", {255, 255, 0}},
    {(char *)"blue", {0, 0, 255}},
    {(char *)"fuchsia", {255, 0, 255}},
    {(char *)"aqua", {0, 255, 255}},
    {(char *)"white", {255, 255, 255}},
    {(char *)"grey0", {0, 0, 0}},
    {(char *)"navyblue", {0, 0, 95}},
    {(char *)"darkblue", {0, 0, 135}},
    {(char *)"trypanblue", {0, 0, 175}},
    {(char *)"mediumblue", {0, 0, 215}},
    {(char *)"blue", {0, 0, 255}},
    {(char *)"darkgreen", {0, 95, 0}},
    {(char *)"mosque", {0, 95, 95}},
    {(char *)"optimist", {0, 95, 135}},
    {(char *)"greenblue", {0, 95, 175}},
    {(char *)"royalblue", {0, 95, 215}},
    {(char *)"neonblue", {0, 95, 255}},
    {(char *)"indiagreen", {0, 135, 0}},
    {(char *)"motherearth", {0, 135, 95}},
    {(char *)"turgoise", {0, 135, 135}},
    {(char *)"bowie", {0, 135, 175}},
    {(char *)"tuftsblue", {0, 135, 215}},
    {(char *)"dodgerblue", {0, 135, 255}},
    {(char *)"yellowgreen", {0, 175, 0}},
    {(char *)"homerun", {0, 175, 95}},
    {(char *)"darkcyan", {0, 175, 135}},
    {(char *)"lightseagreen", {0, 175, 175}},
    {(char *)"cerulean", {0, 175, 215}},
    {(char *)"bluejeans", {0, 175, 255}},
    {(char *)"limegreen", {0, 215, 0}},
    {(char *)"malachite", {0, 215, 95}},
    {(char *)"parisgreen", {0, 215, 135}},
    {(char *)"caribbeangreen", {0, 215, 175}},
    {(char *)"darkturquoise", {0, 215, 215}},
    {(char *)"deepskyblue", {0, 215, 255}},
    {(char *)"greenlime", {0, 255, 0}},
    {(char *)"erin", {0, 255, 95}},
    {(char *)"springgreen", {0, 255, 135}},
    {(char *)"mediumspringgreen", {0, 255, 175}},
    {(char *)"seagreen", {0, 255, 215}},
    {(char *)"spanishskyblue", {0, 255, 255}},
    {(char *)"rosewood", {95, 0, 0}},
    {(char *)"midnight", {95, 0, 95}},
    {(char *)"indigo", {95, 0, 135}},
    {(char *)"blueviolet", {95, 0, 175}},
    {(char *)"mediumpurple", {95, 0, 215}},
    {(char *)"electricindigo", {95, 0, 255}},
    {(char *)"secretgarden", {95, 95, 0}},
    {(char *)"grey37", {95, 95, 95}},
    {(char *)"kimberly", {95, 95, 135}},
    {(char *)"liberty", {95, 95, 175}},
    {(char *)"slateblue", {95, 95, 215}},
    {(char *)"mediumslateblue", {95, 95, 255}},
    {(char *)"avocado", {95, 135, 0}},
    {(char *)"darkseagreen", {95, 135, 95}},
    {(char *)"steelteal", {95, 135, 135}},
    {(char *)"subzero", {95, 135, 175}},
    {(char *)"unitednationsblue", {95, 135, 215}},
    {(char *)"azure", {95, 135, 255}},
    {(char *)"kellygreen", {95, 175, 0}},
    {(char *)"fruitsalad", {95, 175, 95}},
    {(char *)"shinyshamrock", {95, 175, 135}},
    {(char *)"fountainblue", {95, 175, 175}},
    {(char *)"malibu", {95, 175, 215}},
    {(char *)"frenchskyblue", {95, 175, 255}},
    {(char *)"pistachio", {95, 215, 0}},
    {(char *)"digitalgreen", {95, 215, 95}},
    {(char *)"seaparisgreen", {95, 215, 135}},
    {(char *)"mediumaquamarine", {95, 215, 175}},
    {(char *)"mediumturquoise", {95, 215, 215}},
    {(char *)"vividskyblue", {95, 215, 255}},
    {(char *)"brightgreen", {95, 255, 0}},
    {(char *)"screamingreen", {95, 255, 95}},
    {(char *)"springseagreen", {95, 255, 135}},
    {(char *)"seagreen", {95, 255, 175}},
    {(char *)"aquamarine", {95, 255, 215}},
    {(char *)"darkslategrey", {95, 255, 255}},
    {(char *)"darkred", {135, 0, 0}},
    {(char *)"flirt", {135, 0, 95}},
    {(char *)"mardigras", {135, 0, 135}},
    {(char *)"darkviolet", {135, 0, 175}},
    {(char *)"frenchviolet", {135, 0, 215}},
    {(char *)"violet", {135, 0, 255}},
    {(char *)"goldenbrown", {135, 95, 0}},
    {(char *)"deeptaupe", {135, 95, 95}},
    {(char *)"chineseviolet", {135, 95, 135}},
    {(char *)"lilacbush", {135, 95, 175}},
    {(char *)"slatepurple", {135, 95, 215}},
    {(char *)"majorelleblue", {135, 95, 255}},
    {(char *)"lightolive", {135, 135, 0}},
    {(char *)"wilderness", {135, 135, 95}},
    {(char *)"grey53", {135, 135, 135}},
    {(char *)"beyond", {135, 135, 175}},
    {(char *)"moodyblue", {135, 135, 215}},
    {(char *)"lightslateblue", {135, 135, 255}},
    {(char *)"applegreen", {135, 175, 0}},
    {(char *)"chelseacucumber", {135, 175, 95}},
    {(char *)"bayleaf", {135, 175, 135}},
    {(char *)"ziggurat", {135, 175, 175}},
    {(char *)"poloblue", {135, 175, 215}},
    {(char *)"cornflowerblue", {135, 175, 255}},
    {(char *)"sheengreen", {135, 215, 0}},
    {(char *)"mantis", {135, 215, 95}},
    {(char *)"deyork", {135, 215, 135}},
    {(char *)"zeal", {135, 215, 175}},
    {(char *)"onepoto", {135, 215, 215}},
    {(char *)"lightskyblue", {135, 215, 255}},
    {(char *)"chartreuse", {135, 255, 0}},
    {(char *)"lightlime", {135, 255, 95}},
    {(char *)"palegreen", {135, 255, 135}},
    {(char *)"lightpalegreen", {135, 255, 175}},
    {(char *)"lightaquamarine", {135, 255, 215}},
    {(char *)"electricblue", {135, 255, 255}},
    {(char *)"internationalorange", {175, 0, 0}},
    {(char *)"jazzberryjam", {175, 0, 95}},
    {(char *)"mediumvioletred", {175, 0, 135}},
    {(char *)"darkmagenta", {175, 0, 175}},
    {(char *)"darkpurple", {175, 0, 215}},
    {(char *)"vividviolet", {175, 0, 255}},
    {(char *)"omega", {175, 95, 0}},
    {(char *)"quickstep", {175, 95, 95}},
    {(char *)"chinarose", {175, 95, 135}},
    {(char *)"purpureus", {175, 95, 175}},
    {(char *)"mediumorchid", {175, 95, 215}},
    {(char *)"darkorchid", {175, 95, 255}},
    {(char *)"darkgoldenrod", {175, 135, 0}},
    {(char *)"goldcoast", {175, 135, 95}},
    {(char *)"rosybrown", {175, 135, 135}},
    {(char *)"grey63", {175, 135, 175}},
    {(char *)"lavender", {175, 135, 215}},
    {(char *)"electricpurple", {175, 135, 255}},
    {(char *)"acidgreen", {175, 175, 0}},
    {(char *)"darkkhaki", {175, 175, 95}},
    {(char *)"earthstone", {175, 175, 135}},
    {(char *)"grey69", {175, 175, 175}},
    {(char *)"bauhaus", {175, 175, 215}},
    {(char *)"lightsteelblue", {175, 175, 255}},
    {(char *)"bitterlemon", {175, 215, 0}},
    {(char *)"happyhour", {175, 215, 95}},
    {(char *)"feijoa", {175, 215, 135}},
    {(char *)"fringyflower", {175, 215, 175}},
    {(char *)"lightcyan", {175, 215, 215}},
    {(char *)"babyblueeyes", {175, 215, 255}},
    {(char *)"greenyellow", {175, 255, 0}},
    {(char *)"darkolivegreen", {175, 255, 95}},
    {(char *)"palegreen", {175, 255, 135}},
    {(char *)"lightgreen", {175, 255, 175}},
    {(char *)"magicmint", {175, 255, 215}},
    {(char *)"italianskyblue", {175, 255, 255}},
    {(char *)"rossocorsa", {215, 0, 0}},
    {(char *)"rubinered", {215, 0, 95}},
    {(char *)"barbiepink", {215, 0, 135}},
    {(char *)"shockingpink", {215, 0, 175}},
    {(char *)"steelpink", {215, 0, 215}},
    {(char *)"psychedelicpurple", {215, 0, 255}},
    {(char *)"tawny", {215, 95, 0}},
    {(char *)"indianred", {215, 95, 95}},
    {(char *)"blush", {215, 95, 135}},
    {(char *)"mulberry", {215, 95, 175}},
    {(char *)"mediumfuchsia", {215, 95, 215}},
    {(char *)"heliotrope", {215, 95, 255}},
    {(char *)"harvestgold", {215, 135, 0}},
    {(char *)"copper", {215, 135, 95}},
    {(char *)"newyorkpink", {215, 135, 135}},
    {(char *)"middlepurple", {215, 135, 175}},
    {(char *)"brightlilac", {215, 135, 215}},
    {(char *)"darkmauve", {215, 135, 255}},
    {(char *)"lemon", {215, 175, 0}},
    {(char *)"equator", {215, 175, 95}},
    {(char *)"drab", {215, 175, 135}},
    {(char *)"pinkflare", {215, 175, 175}},
    {(char *)"frenchlilac", {215, 175, 215}},
    {(char *)"mauve", {215, 175, 255}},
    {(char *)"neva", {215, 215, 0}},
    {(char *)"springfever", {215, 215, 95}},
    {(char *)"deco", {215, 215, 135}},
    {(char *)"zen", {215, 215, 175}},
    {(char *)"grey84", {215, 215, 215}},
    {(char *)"periwinkle", {215, 215, 255}},
    {(char *)"spritzer", {215, 255, 0}},
    {(char *)"sublime", {215, 255, 95}},
    {(char *)"mindaro", {215, 255, 135}},
    {(char *)"reef", {215, 255, 175}},
    {(char *)"nyanza", {215, 255, 215}},
    {(char *)"cyan", {215, 255, 255}},
    {(char *)"red", {255, 0, 0}},
    {(char *)"radicalred", {255, 0, 95}},
    {(char *)"rose", {255, 0, 135}},
    {(char *)"hollywoodcerise", {255, 0, 175}},
    {(char *)"hotmagenta", {255, 0, 215}},
    {(char *)"magenta", {255, 0, 255}},
    {(char *)"orange", {255, 95, 0}},
    {(char *)"strawberry", {255, 95, 95}},
    {(char *)"brinkpink", {255, 95, 135}},
    {(char *)"hotpink", {255, 95, 175}},
    {(char *)"rosepink", {255, 95, 215}},
    {(char *)"ultrapink", {255, 95, 255}},
    {(char *)"darkorange", {255, 135, 0}},
    {(char *)"salmon", {255, 135, 95}},
    {(char *)"lightcoral", {255, 135, 135}},
    {(char *)"palevioletred", {255, 135, 175}},
    {(char *)"persianpink", {255, 135, 215}},
    {(char *)"orchid", {255, 135, 255}},
    {(char *)"chineseyellow", {255, 175, 0}},
    {(char *)"sandybrown", {255, 175, 95}},
    {(char *)"lightsalmon", {255, 175, 135}},
    {(char *)"lightpink", {255, 175, 175}},
    {(char *)"carnationpink", {255, 175, 215}},
    {(char *)"plum", {255, 175, 255}},
    {(char *)"gold", {255, 215, 0}},
    {(char *)"tinkerbell", {255, 215, 95}},
    {(char *)"cherokee", {255, 215, 135}},
    {(char *)"navajowhite", {255, 215, 175}},
    {(char *)"mistyrose", {255, 215, 215}},
    {(char *)"thistle", {255, 215, 255}},
    {(char *)"yellow", {255, 255, 0}},
    {(char *)"lightgoldenrod", {255, 255, 95}},
    {(char *)"khaki", {255, 255, 135}},
    {(char *)"wheat", {255, 255, 175}},
    {(char *)"cornsilk", {255, 255, 215}},
    {(char *)"grey100", {255, 255, 255}},
    {(char *)"grey3", {8, 8, 8}},
    {(char *)"grey7", {18, 18, 18}},
    {(char *)"grey11", {28, 28, 28}},
    {(char *)"grey15", {38, 38, 38}},
    {(char *)"grey19", {48, 48, 48}},
    {(char *)"grey23", {58, 58, 58}},
    {(char *)"grey27", {68, 68, 68}},
    {(char *)"grey30", {78, 78, 78}},
    {(char *)"grey35", {88, 88, 88}},
    {(char *)"grey39", {98, 98, 98}},
    {(char *)"grey42", {108, 108, 108}},
    {(char *)"grey46", {118, 118, 118}},
    {(char *)"grey50", {128, 128, 128}},
    {(char *)"grey54", {138, 138, 138}},
    {(char *)"grey58", {148, 148, 148}},
    {(char *)"grey62", {158, 158, 158}},
    {(char *)"grey66", {168, 168, 168}},
    {(char *)"grey70", {178, 178, 178}},
    {(char *)"grey74", {188, 188, 188}},
    {(char *)"grey78", {198, 198, 198}},
    {(char *)"grey82", {208, 208, 208}},
    {(char *)"grey85", {218, 218, 218}},
    {(char *)"grey89", {228, 228, 228}},
    {(char *)"grey93", {238, 238, 238}},
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