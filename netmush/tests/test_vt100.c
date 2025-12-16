#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "config.h"
#include "constants.h"
#include "typedefs.h"
#include "macros.h"

// Minimal prototypes to avoid pulling in full prototypes.h (which declares main)
extern rgbColor X112RGB(int color);
extern VT100ATTR decodeVT100(const char **ansi);
extern COLORMATCH getColorMatch(rgbColor rgb, uint8_t schemes);

static void test_X112RGB_basic()
{
    rgbColor c0 = X112RGB(0); // black
    assert(c0.r == 0 && c0.g == 0 && c0.b == 0);

    rgbColor c7 = X112RGB(7); // light gray
    assert(c7.r == 192 && c7.g == 192 && c7.b == 192);

    rgbColor c8 = X112RGB(8); // dark gray
    assert(c8.r == 128 && c8.g == 128 && c8.b == 128);

    rgbColor c12 = X112RGB(12); // bright blue
    assert(c12.r == 0 && c12.g == 0 && c12.b == 255);

    rgbColor c196 = X112RGB(196); // xterm red in 6x6x6 cube
    assert(c196.r == 255 && c196.g == 0 && c196.b == 0);
}

static void test_decodeVT100_truecolor()
{
    const char *s = "\x1b[38;2;255;0;255m"; // magenta foreground
    const char *p = s;
    VT100ATTR attr = decodeVT100(&p);
    assert(attr.foreground.type == ANSICOLORTYPE_TRUECOLORS);
    assert(attr.foreground.rgb.r == 255 && attr.foreground.rgb.g == 0 && attr.foreground.rgb.b == 255);
}

static void test_decodeVT100_xterm()
{
    const char *s = "\x1b[48;5;196m"; // red background via xterm index
    const char *p = s;
    VT100ATTR attr = decodeVT100(&p);
    assert(attr.background.type == ANSICOLORTYPE_XTERM);
    assert(attr.background.rgb.r == 255 && attr.background.rgb.g == 0 && attr.background.rgb.b == 0);
}

static void test_getColorMatch_stability()
{
    rgbColor mag = {255, 0, 255};
    COLORMATCH a = getColorMatch(mag, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS);
    COLORMATCH b = getColorMatch(mag, COLOR_SCHEME_ANSI | COLOR_SCHEME_XTERM | COLOR_SCHEME_CSS);
    assert(a.color.name != NULL && b.color.name != NULL);
    assert(strcmp(a.color.name, b.color.name) == 0);
}

static void test_decodeVT100_extra_semicolons()
{
    const char *s = "\x1b[38;;5;;196m"; // extra semicolons should be ignored
    const char *p = s;
    VT100ATTR attr = decodeVT100(&p);
    assert(attr.foreground.type == ANSICOLORTYPE_XTERM);
    assert(attr.foreground.rgb.r == 255 && attr.foreground.rgb.g == 0 && attr.foreground.rgb.b == 0);
}

static void test_decodeVT100_clamped_values()
{
    const char *s = "\x1b[38;2;0;300;999m"; // should clamp to 0,255,255
    const char *p = s;
    VT100ATTR attr = decodeVT100(&p);
    assert(attr.foreground.type == ANSICOLORTYPE_TRUECOLORS);
    assert(attr.foreground.rgb.r == 0 && attr.foreground.rgb.g == 255 && attr.foreground.rgb.b == 255);
}

static void test_decodeVT100_bright_variants()
{
    // Bright blue foreground 94
    const char *s1 = "\x1b[94m";
    const char *p1 = s1;
    VT100ATTR a1 = decodeVT100(&p1);
    assert(a1.foreground.type == ANSICOLORTYPE_STANDARD);
    assert(a1.foreground.rgb.r == 0 && a1.foreground.rgb.g == 0 && a1.foreground.rgb.b == 255);

    // Bright red background 101
    const char *s2 = "\x1b[101m";
    const char *p2 = s2;
    VT100ATTR a2 = decodeVT100(&p2);
    assert(a2.background.type == ANSICOLORTYPE_STANDARD);
    assert(a2.background.rgb.r == 255 && a2.background.rgb.g == 0 && a2.background.rgb.b == 0);
}

static void test_decodeVT100_missing_m()
{
    // Sequence without trailing 'm' should be ignored
    const char *s = "\x1b[38;2;255;0;255";
    const char *p = s;
    VT100ATTR attr = decodeVT100(&p);
    assert(attr.foreground.type == ANSICOLORTYPE_NONE);
    assert(attr.background.type == ANSICOLORTYPE_NONE);
}

static void test_getColorMatch_default_schemes()
{
    rgbColor c = {0, 255, 255};
    COLORMATCH a = getColorMatch(c, 0); // default to all
    COLORMATCH b = getColorMatch(c, (uint8_t)(1 | 2 | 4));
    assert(a.color.name && b.color.name);
    assert(strcmp(a.color.name, b.color.name) == 0);
}

int main(void)
{
    test_X112RGB_basic();
    test_decodeVT100_truecolor();
    test_decodeVT100_xterm();
    test_getColorMatch_stability();
    test_decodeVT100_extra_semicolons();
    test_decodeVT100_clamped_values();
    test_decodeVT100_bright_variants();
    test_decodeVT100_missing_m();
    test_getColorMatch_default_schemes();
    printf("vt100 tests passed\n");
    return 0;
}
