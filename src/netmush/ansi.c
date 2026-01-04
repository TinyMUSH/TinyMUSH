/**
 * @file ansi.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief ANSI terminal control helpers and telnet negotiation support
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
#include "ansi.h"

#include <stdbool.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

// ANSI constants for escape sequences
static const char C_ANSI_ESC[] = "\033[";
static const char C_ANSI_RESET[] = "0";
static const char C_ANSI_BOLD[] = "1";
static const char C_ANSI_UNDERLINE[] = "4";
static const char C_ANSI_BLINK[] = "5";
static const char C_ANSI_REVERSE[] = "7";
static const char C_ANSI_NORMAL_INTENSITY[] = "22";
static const char C_ANSI_NO_UNDERLINE[] = "24";
static const char C_ANSI_NO_BLINK[] = "25";
static const char C_ANSI_NO_REVERSE[] = "27";
static const char C_ANSI_FOREGROUND_RESET[] = "39";
static const char C_ANSI_BACKGROUND_RESET[] = "49";
static const char C_ANSI_END[] = "m";
static const char C_ANSI_RESET_SEQUENCE[] = "\033[0m";
static const char C_ANSI_XTERM_PREFIX_FG[] = "38;5;";
static const char C_ANSI_XTERM_PREFIX_BG[] = "48;5;";
static const char C_ANSI_TRUECOLOR_PREFIX_FG[] = "38;2;";
static const char C_ANSI_TRUECOLOR_PREFIX_BG[] = "48;2;";

/**
 * @brief Converts an RGB color to CIELAB color space.
 *
 * This function applies the sRGB to XYZ transformation then XYZ to CIELAB
 * using the D65 reference white and CIE 1976 formulas.
 *
 * @param color The RGB color to convert (values 0-255).
 * @return The equivalent color in CIELAB.
 */
ColorCIELab ansi_rgb_to_cielab(ColorRGB color)
{
    ColorCIELab lab;

    // Normalize to [0, 1]
    double vr = (double)color.r / 255.0;
    double vg = (double)color.g / 255.0;
    double vb = (double)color.b / 255.0;

    // Inverse Gamma Correction (sRGB)
    vr = (vr > 0.04045) ? pow((vr + 0.055) / 1.055, 2.4) : (vr / 12.92);
    vg = (vg > 0.04045) ? pow((vg + 0.055) / 1.055, 2.4) : (vg / 12.92);
    vb = (vb > 0.04045) ? pow((vb + 0.055) / 1.055, 2.4) : (vb / 12.92);

    // Reference white D65
    double x_ref = (vr * 0.4124564 + vg * 0.3575761 + vb * 0.1804375) / 0.95047;
    double y_ref = (vr * 0.2126729 + vg * 0.7151522 + vb * 0.0721750) / 1.00000;
    double z_ref = (vr * 0.0193339 + vg * 0.1191920 + vb * 0.9503041) / 1.08883;

    // Convert to CIELAB - Use cbrt for cube roots
    double fx = (x_ref > 0.008856) ? cbrt(x_ref) : (7.787 * x_ref + 16.0 / 116.0);
    double fy = (y_ref > 0.008856) ? cbrt(y_ref) : (7.787 * y_ref + 16.0 / 116.0);
    double fz = (z_ref > 0.008856) ? cbrt(z_ref) : (7.787 * z_ref + 16.0 / 116.0);

    // Calculate L*, a*, b*
    lab.L = (116.0 * fy) - 16.0;
    lab.a = 500.0 * (fx - fy);
    lab.b = 200.0 * (fy - fz);

    return lab;
}

/**
 * @brief Calculates the CIEDE2000 distance between two CIELAB colors.
 *
 * This function implements the CIEDE2000 formula to measure the perceptual difference
 * between two colors, taking into account luminance, chroma, and hue with advanced corrections.
 *
 * @param lab1 The first CIELAB color.
 * @param lab2 The second CIELAB color.
 * @return The CIEDE2000 distance (smaller = more similar).
 */
double ansi_ciede2000(ColorCIELab lab1, ColorCIELab lab2)
{
    double L1 = lab1.L, a1 = lab1.a, b1 = lab1.b;
    double L2 = lab2.L, a2 = lab2.a, b2 = lab2.b;
    double kL = 1.0, kC = 1.0, kH = 1.0;

    double deltaL = L2 - L1;
    double Lbar = (L1 + L2) / 2.0;

    double C1 = sqrt(a1 * a1 + b1 * b1);
    double C2 = sqrt(a2 * a2 + b2 * b2);
    double Cbar = (C1 + C2) / 2.0;

    double ap1 = a1 + (a1 / 2.0) * (1.0 - sqrt(pow(Cbar, 7.0) / (pow(Cbar, 7.0) + 6103515625.0)));
    double ap2 = a2 + (a2 / 2.0) * (1.0 - sqrt(pow(Cbar, 7.0) / (pow(Cbar, 7.0) + 6103515625.0)));

    double Cp1 = sqrt(ap1 * ap1 + b1 * b1);
    double Cp2 = sqrt(ap2 * ap2 + b2 * b2);
    double Cpbar = (Cp1 + Cp2) / 2.0;

    double deltaCp = Cp2 - Cp1;

    double hp1 = atan2(b1, ap1);
    if (hp1 < 0)
        hp1 += 2 * M_PI;

    double hp2 = atan2(b2, ap2);
    if (hp2 < 0)
        hp2 += 2 * M_PI;

    double deltahp = hp2 - hp1;
    if (fabs(deltahp) > M_PI)
    {
        if (hp2 <= hp1)
            deltahp += 2 * M_PI;
        else
            deltahp -= 2 * M_PI;
    }

    double deltaHp = 2 * sqrt(Cp1 * Cp2) * sin(deltahp / 2.0);

    double Hpbar = (hp1 + hp2) / 2.0;
    if (fabs(hp1 - hp2) > M_PI)
        Hpbar += M_PI;

    double T = 1 - 0.17 * cos(Hpbar - M_PI / 6.0) + 0.24 * cos(2 * Hpbar) + 0.32 * cos(3 * Hpbar + M_PI / 30.0) - 0.20 * cos(4 * Hpbar - 63 * M_PI / 180.0);

    double SL = 1 + (0.015 * pow(Lbar - 50, 2)) / sqrt(20 + pow(Lbar - 50, 2));
    double SC = 1 + 0.045 * Cpbar;
    double SH = 1 + 0.015 * Cpbar * T;

    double RT = -sin(2 * (35 * M_PI / 180.0) * exp(-pow((Hpbar * 180 / M_PI - 275) / 25, 2))) * (2 * sqrt(pow(Cpbar, 7) / (pow(Cpbar, 7) + 6103515625.0)));

    double kLSC = deltaL / (kL * SL);
    double kCSC = deltaCp / (kC * SC);
    double kHSH = deltaHp / (kH * SH);

    double deltaE = sqrt(kLSC * kLSC + kCSC * kCSC + kHSH * kHSH + RT * kCSC * kHSH);

    return deltaE;
}

/**
 * @brief Finds the closest ANSI or XTerm color to a given CIELAB color.
 *
 * Uses CIEDE2000 distance to compare colors and select the best perceptual
 * approximation in the specified palette.
 *
 * @param lab The target CIELAB color.
 * @param type The palette type (ColorTypeAnsi or ColorTypeXTerm).
 * @return The closest color definition.
 */
ColorEntry ansi_find_closest_color_with_lab(ColorCIELab lab, ColorType type)
{
    ColorEntry closest_color;
    double min_distance = DBL_MAX, distance;

    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (colorDefinitions[i].type == type)
        {
            // Use pre-computed CIELAB values instead of converting RGB each time
            distance = ansi_ciede2000(lab, colorDefinitions[i].lab);
            if (distance < min_distance)
            {
                min_distance = distance;
                closest_color = colorDefinitions[i];
            }
        }
    }

    return closest_color;
}

/**
 * @brief Sets the foreground or background color from an RGB color.
 *
 * Converts the RGB color to perceptually close ANSI and XTerm approximations
 * and updates the color state.
 *
 * @param color Pointer to the color state to modify.
 * @param rgb The source RGB color.
 * @param is_background true to set background, false for foreground.
 */
void ansi_get_color_from_rgb(ColorState *color, ColorRGB rgb, bool is_background)
{
    if (!color)
        return;
    ColorCIELab lab = ansi_rgb_to_cielab(rgb);
    ColorEntry ansi = ansi_find_closest_color_with_lab(lab, ColorTypeAnsi);
    ColorEntry xterm = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm);
    ColorInfo cd = {
        .is_set = ColorStatusSet,
        .ansi_index = ansi.ansi_index,
        .xterm_index = xterm.xterm_index,
        .truecolor = rgb,
    };

    if (is_background)
    {

        color->background = cd;
    }
    else
    {
        color->foreground = cd;
    }
}

/**
 * @brief Sets the foreground or background color from an XTerm index.
 *
 * Looks up the corresponding color in the table and updates the state.
 *
 * @param color Pointer to the color state to modify.
 * @param index The XTerm index (0-255).
 * @param is_background true to set background, false for foreground.
 */
void ansi_get_color_from_index(ColorState *color, int index, bool is_background)
{
    if (color)
    {
        // Index validation for safety
        if (index >= 0 && colorDefinitions[index].name != NULL)
        {
            ColorInfo cd = {
                .is_set = ColorStatusSet,
                .ansi_index = colorDefinitions[index].ansi_index,
                .xterm_index = colorDefinitions[index].xterm_index,
                .truecolor = colorDefinitions[index].truecolor,
            };

            if (is_background)
            {
                color->background = cd;
            }
            else
            {
                color->foreground = cd;
                if (cd.ansi_index >= 8)
                    color->highlight = ColorStatusSet;
            }
        }
    }
}

/**
 * @brief Sets the foreground or background color from a color name.
 *
 * Searches the color table for the name and uses LAB to find the closest ANSI.
 *
 * @param color Pointer to the color state to modify.
 * @param name The color name (e.g. "red", "blue").
 * @param is_background true to set background, false for foreground.
 */
void ansi_get_color_from_name(ColorState *color, const char *name, bool is_background)
{
    if (!color)
        return;
    if (!name || !*name)
    {
        return; // Invalid name
    }

    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (strcasecmp(colorDefinitions[i].name, name) == 0)
        {
            ColorCIELab lab = ansi_rgb_to_cielab(colorDefinitions[i].truecolor);
            ColorEntry ansi = ansi_find_closest_color_with_lab(lab, ColorTypeAnsi);
            ColorInfo cd = {
                .is_set = ColorStatusSet,
                .ansi_index = ansi.ansi_index,
                .xterm_index = colorDefinitions[i].xterm_index,
                .truecolor = colorDefinitions[i].truecolor,
            };

            if (is_background)
            {
                color->background = cd;
            }
            else
            {
                color->foreground = cd;
                if (cd.ansi_index >= 8)
                    color->highlight = ColorStatusSet;
            }
            return; // Found, exit
        }
    }
    // Not found, do nothing
}

static void reset_color_data(ColorInfo *data)
{
    if (data)
        *data = (ColorInfo){ColorStatusReset, 0, 0, {0, 0, 0}};
}

static bool parse_color_value(const char *token, uint8_t *value)
{
    if (!token || !value)
        return false;
    errno = 0;
    long v = strtol(token[0] == '#' ? token + 1 : token, NULL, token[0] == '#' ? 16 : 10);
    if (errno == ERANGE || errno == EINVAL || v < 0 || v > 255)
        return false;
    *value = (uint8_t)v;
    return true;
}

/**
 * @brief Parses a hexadecimal or decimal string to set an RGB color.
 *
 * Supports formats like #RRGGBB, RRGGBB, RRRGGGBBB, etc., and sets the foreground
 * or background color accordingly.
 *
 * @param color Pointer to the color state to modify.
 * @param text The string to parse (modified in place).
 * @param is_background true to set background, false for foreground.
 * @return true if parsing succeeded, false otherwise.
 */
bool ansi_get_color_from_text(ColorState *color, char *text, bool is_background)
{
    if (!color || !text || *text == '\0')
        return false;

    bool success = false;
    char *lower_text = XSTRDUP(text, "lower_text");
    if (!lower_text)
        return false;

    // Convert to lowercase
    for (size_t i = 0; lower_text[i]; ++i)
    {
        lower_text[i] = tolower(lower_text[i]);
    }

    char *token[3] = {NULL}, *str = lower_text, *saveptr = NULL;
    size_t count = 0;
    ColorRGB rgb = {0, 0, 0};

    for (count = 0; count < 3; count++)
    {
        token[count] = strtok_r(str, " ", &saveptr);
        if (token[count] == NULL)
            break;
        str = NULL;
    }

    switch (count)
    {
    case 1:
        // Xterm index or truecolor value
        if (token[0][0] == '#')
        {
            // Try to parse as #RRGGBB
            if (sscanf(token[0] + 1, "%2hhx%2hhx%2hhx", &rgb.r, &rgb.g, &rgb.b) == 3)
            {
                fprintf(stderr, "DEBUG: parsed #%02x%02x%02x from '%s'\n", rgb.r, rgb.g, rgb.b, token[0]);
                ansi_get_color_from_rgb(color, rgb, is_background);
                success = true;
            }
            else
            {
                // Try as short hex #RGB or #R, treat as xterm index
                errno = 0;
                long idx = strtol(token[0] + 1, NULL, 16);
                if (errno != ERANGE && errno != EINVAL && idx >= 0 && idx <= 255)
                {
                    ansi_get_color_from_index(color, (int)idx, is_background);
                    success = true;
                }
            }
        }
        else
        {
            // Decimal or xterm index
            uint8_t val;
            if (parse_color_value(token[0], &val))
            {
                ansi_get_color_from_index(color, (int)val, is_background);
                success = true;
            }
        }
        break;
    case 2:
    case 3:
        // Truecolor value
        success = true; // Assume success until failure
        for (size_t i = 0; i < count; i++)
        {
            uint8_t val;
            if (!parse_color_value(token[i], &val))
            {
                success = false;
                break;
            }
            switch (i)
            {
            case 0:
                rgb.r = val;
                break;
            case 1:
                rgb.g = val;
                break;
            case 2:
                rgb.b = val;
                break;
            }
        }
        if (success)
        {
            ansi_get_color_from_rgb(color, rgb, is_background);
        }
        break;
    }

    XFREE(lower_text);
    return success;
}

/**
 * @brief Detects the type of color string (name, hex, MUSHcode, etc.).
 *
 * Analyzes the input string to determine if it represents a color name,
 * hexadecimal value, MUSHcode sequence, or invalid input.
 *
 * @param str The color string to analyze.
 * @param len The length of the string.
 * @return The detected ColorParseType.
 */
static ColorParseType detect_color_type(const char *str, size_t len)
{
    if (len == 0)
        return ColorParseTypeInvalid;
    const char *s = str;
    if (*s == '<' || *s == '+')
    {
        s++;
        if (*s == '<')
            s++; // +<VALUE>
    }
    // Now s points to the inner content
    size_t inner_len = len - (s - str);
    if (inner_len == 0)
        return ColorParseTypeInvalid;
    if (strncasecmp(s, "xterm", 5) == 0 || strncasecmp(s, "color", 5) == 0)
        return ColorParseTypeHex;
    if (isdigit(s[0]) || (s[0] == '#' && inner_len > 1 && isxdigit(s[1])))
        return ColorParseTypeHex;
    // Check for MUSHcode: only allowed chars
    // First, extract inner content
    const char *inner_start = s;
    if (*inner_start == '<' || *inner_start == '+')
    {
        inner_start++;
        if (*inner_start == '<')
            inner_start++;
    }
    const char *inner_end = inner_start + strlen(inner_start) - 1;
    while (inner_end > inner_start && *inner_end == '>')
        inner_end--;
    size_t inner_len2 = inner_end - inner_start + 1;
    char *inner_buf = XMALLOC(inner_len2 + 1, "inner buf for detect");
    if (!inner_buf)
        return ColorParseTypeInvalid;
    XMEMCPY(inner_buf, inner_start, inner_len2);
    inner_buf[inner_len2] = '\0';

    // Check if inner_buf is a color name
    bool is_color_name = false;
    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (strcasecmp(colorDefinitions[i].name, inner_buf) == 0)
        {
            is_color_name = true;
            break;
        }
    }
    XFREE(inner_buf);
    if (is_color_name)
        return ColorParseTypeName;

    // Check for MUSH
    bool is_mush = true;
    for (size_t i = 0; i < inner_len; i++)
    {
        char c = tolower(s[i]);
        if (strchr("fFhHuUiInNdDxXrRgGyYbBmMcCwW", c) == NULL)
        {
            is_mush = false;
            break;
        }
    }
    if (is_mush) {
        return ColorParseTypeMush;
    }

    return ColorParseTypeInvalid;
}

/**
 * @brief Trims whitespace from the beginning and end of a string using pointers.
 *
 * @param start Pointer to the start of the string (modified).
 * @param end Pointer to the end of the string (modified).
 */
static void trim_string(const char **start, const char **end)
{
    while (**start && isspace(**start))
        (*start)++;
    *end = *start + strlen(*start) - 1;
    while (*end > *start && isspace(**end))
        (*end)--;
}

/**
 * @brief Extracts the inner content of a string by removing < > or + wrappers.
 *
 * Allocates a new string for the inner content.
 *
 * @param start The source string.
 * @param out_len Pointer to the length of the extracted content.
 * @return The allocated extracted string, or NULL on error.
 */
static char *extract_inner_content(const char *start, size_t *out_len)
{
    const char *inner_start = start;
    if (*inner_start == '<' || *inner_start == '+')
    {
        inner_start++;
        if (*inner_start == '<')
            inner_start++;
    }

    const char *inner_end = inner_start + strlen(inner_start) - 1;
    while (inner_end > inner_start && *inner_end == '>')
        inner_end--;

    *out_len = inner_end - inner_start + 1;
    char *buf = XMALLOC(*out_len + 1, "buffer for inner content");
    if (!buf)
        return NULL;

    XMEMCPY(buf, inner_start, *out_len);
    buf[*out_len] = '\0';
    return buf;
}

/**
 * @brief Parses a complex color string and sets the corresponding color.
 *
 * Supports color names, hex values, and MUSH code.
 * Handles wrappers like <color> or +color.
 * If '/' is present, the part before is foreground, after is background.
 *
 * @param color Pointer to the color state to modify.
 * @param color_str The string to parse.
 * @param is_background true to set background, false for foreground (ignored if '/' present).
 * @return true if parsing succeeded, false otherwise.
 */
bool ansi_parse_color_from_string(ColorState *color, const char *color_str, bool is_background)
{
    if (!color_str)
        return false;

    // Check for '/' separator
    const char *slash_pos = strstr(color_str, "/");
    if (slash_pos)
    {
        // Split into foreground and background parts
        size_t fg_len = slash_pos - color_str;
        size_t bg_len = strlen(slash_pos + 1);

        bool fg_success = true;
        if (fg_len > 0)
        {
            char *fg_str = XMALLOC(fg_len + 1, "foreground string");
            if (!fg_str)
                return false;
            XMEMCPY(fg_str, color_str, fg_len);
            fg_str[fg_len] = '\0';
            fg_success = ansi_parse_color_from_string(color, fg_str, false);
            XFREE(fg_str);
        }

        bool bg_success = true;
        if (bg_len > 0)
        {
            char *bg_str = XMALLOC(bg_len + 1, "background string");
            if (!bg_str)
                return false;
            XMEMCPY(bg_str, slash_pos + 1, bg_len + 1);
            bg_success = ansi_parse_color_from_string(color, bg_str, true);
            XFREE(bg_str);
        }

        // If both parts are empty, it's invalid
        if (fg_len == 0 && bg_len == 0)
            return false;

        return fg_success && bg_success;
    }

    // Original parsing logic if no '/'
    const char *start = color_str;
    const char *end;
    trim_string(&start, &end);
    size_t trimmed_len = end - start + 1;

    bool highlight = false;
    if (*start == '+')
    {
        highlight = true;
        start++;
    }
    if (*start == '<')
    {
        start++;
    }
    trimmed_len = end - start + 1;

    ColorParseType type = detect_color_type(start, trimmed_len);

    if (type == ColorParseTypeName)
    {
        size_t name_len;
        char *name_buf = extract_inner_content(start, &name_len);
        if (!name_buf)
            return false;

        ansi_get_color_from_name(color, name_buf, is_background);
        XFREE(name_buf);
    }
    else if (type == ColorParseTypeHex)
    {
        size_t hex_len;
        char *hex_buf = extract_inner_content(start, &hex_len);
        if (!hex_buf)
            return false;

        // Remove "xterm" or "color" prefix if present
        if ((strncasecmp(hex_buf, "xterm", 5) == 0) || (strncasecmp(hex_buf, "color", 5) == 0))
        {
            memmove(hex_buf, hex_buf + 5, hex_len - 5 + 1);
            hex_len -= 5;
        }

        ansi_get_color_from_text(color, hex_buf, is_background);
        XFREE(hex_buf);
    }
    else if (type == ColorParseTypeMush)
    {
        // Parsing MUSHcode with table
        for (size_t i = 0; i < trimmed_len; i++)
        {
            char c = tolower(start[i]);
            bool is_upper = isupper(start[i]);
            switch (c)
            {
            case 'n':
                color->reset = ColorStatusReset;
                break;
            case 'd':
                if (is_upper)
                    reset_color_data(&color->background);
                else
                    reset_color_data(&color->foreground);
                break;
            case 'f':
                color->flash = is_upper ? ColorStatusReset : ColorStatusSet;
                break;
            case 'h':
                color->highlight = is_upper ? ColorStatusReset : ColorStatusSet;
                break;
            case 'u':
                color->underline = is_upper ? ColorStatusReset : ColorStatusSet;
                break;
            case 'i':
                color->inverse = is_upper ? ColorStatusReset : ColorStatusSet;
                break;
            default:
                for (size_t k = 0; colorDefinitions[k].name != NULL; k++)
                {
                    if (colorDefinitions[k].mush_code == c)
                    {
                        ansi_get_color_from_index(color, colorDefinitions[k].xterm_index, is_upper);
                        break;
                    }
                }
                break;
            }
        }
    }

    if (highlight && !is_background)
        color->highlight = ColorStatusSet;

    return true;
}

/**
 * @brief Adds an ANSI attribute code to the buffer if applicable.
 *
 * @param buffer The output buffer.
 * @param buffer_size Buffer size.
 * @param offset Current offset in the buffer.
 * @param attr The attribute to add.
 * @param code The corresponding ANSI code.
 */
static void append_attribute(char *buffer, size_t buffer_size, size_t *offset, ColorStatus attr, const char *code)
{
    if (code)
    {
        if (*offset < buffer_size - (strlen(code) + 2))
        {
            if (attr != ColorStatusNone)
            {
                // Add separator if necessary
                *offset += XSPRINTF(buffer + *offset, "%s%s", (*offset > 2 ? ";" : ""), code);
            }
        }
    }
}

/**
 * @brief Adds an ANSI color code to the buffer according to the type.
 *
 * @param buffer The output buffer.
 * @param buffer_size Buffer size.
 * @param offset Current offset in the buffer.
 * @param color The color state.
 * @param type The color type (ANSI, XTerm, TrueColor).
 * @param is_foreground true for foreground, false for background.
 */
static void append_color(char *buffer, size_t buffer_size, size_t *offset, ColorState *color, ColorType type, bool is_foreground)
{
    ColorInfo color_data = is_foreground ? color->foreground : color->background;
    if (*offset >= buffer_size - 20)
        return; // Margin for truecolor
    if (color_data.is_set == ColorStatusNone)
        return;

    int len = XSPRINTF(buffer + *offset, "%s", (*offset > 2 ? ";" : ""));
    *offset += len;

    if (color_data.is_set == ColorStatusReset)
    {
        // Add reset sequence for foreground or background
        len = XSPRINTF(buffer + *offset, "%s", is_foreground ? C_ANSI_FOREGROUND_RESET : C_ANSI_BACKGROUND_RESET);
        *offset += len;
        return;
    }

    // Handle ColorStatusSet cases
    switch (type)
    {
    case ColorTypeAnsi:
    {
        int ansi_idx = color_data.ansi_index;
        if (color_data.ansi_index >= 0 && color_data.ansi_index < 8 && color->highlight == ColorStatusSet) {
            ansi_idx = color_data.ansi_index + 8;
        }
        int base = is_foreground ? ((ansi_idx & 0x08) ? 90 : 30) : ((ansi_idx & 0x08) ? 100 : 40);
        len = XSPRINTF(buffer + *offset, "%d", (ansi_idx & 0x07) + base);
        if (is_foreground && (ansi_idx & 0x08) && (color->highlight != ColorStatusSet))
        {
            len += XSPRINTF(buffer + *offset + len, ";1"); // Bold for bright colors if highlight not set
        }
        *offset += len;
        break;
    }
    case ColorTypeXTerm:
    {
        int xterm_idx = color_data.xterm_index;
        if (color_data.ansi_index >= 0 && color_data.ansi_index < 8 && color->highlight == ColorStatusSet) {
            xterm_idx = colorDefinitions[color_data.ansi_index + 8].xterm_index;
        }
        len = XSPRINTF(buffer + *offset, "%s%d", is_foreground ? C_ANSI_XTERM_PREFIX_FG : C_ANSI_XTERM_PREFIX_BG, xterm_idx);
        *offset += len;
        break;
    }
    case ColorTypeTrueColor:
    {
        ColorRGB rgb = color_data.truecolor;
        if (color_data.ansi_index >= 0 && color_data.ansi_index < 8 && color->highlight == ColorStatusSet) {
            rgb = colorDefinitions[color_data.ansi_index + 8].truecolor;
        }
        len = XSPRINTF(buffer + *offset, "%s%d;%d;%d", is_foreground ? C_ANSI_TRUECOLOR_PREFIX_FG : C_ANSI_TRUECOLOR_PREFIX_BG, rgb.r, rgb.g, rgb.b);
        *offset += len;
        break;
    }
    default:
        break;
    }
}

/**
 * @brief Generates an ANSI escape sequence from the color state.
 *
 * Builds an escape string for attributes and colors according to the specified type.
 * Resets the state after generation if a reset is requested.
 *
 * @param buffer The buffer to write the sequence to.
 * @param buffer_size The buffer size.
 * @param offset Pointer to the current offset in the buffer (updated).
 * @param to The color state to convert.
 * @param type The sequence type (ANSI, XTerm, TrueColor).
 * @return The resulting attribute (Set, Reset, or None).
 */
ColorStatus to_ansi_escape_sequence(char *buffer, size_t buffer_size, size_t *offset, ColorState *to, ColorType type)
{
    ColorStatus state = ColorStatusNone;

    if (buffer)
    {
        bool has_bg = to->background.is_set == ColorStatusSet || to->background.is_set == ColorStatusReset;
        bool has_fg = to->foreground.is_set == ColorStatusSet || to->foreground.is_set == ColorStatusReset;
        bool has_attr = to->reset == ColorStatusReset || to->highlight != ColorStatusNone || to->underline != ColorStatusNone || to->flash != ColorStatusNone || to->inverse != ColorStatusNone;

        if (has_bg || has_fg || has_attr)
        {
            *offset += XSPRINTF(buffer + *offset, "%s", C_ANSI_ESC);
            append_attribute(buffer, buffer_size, offset, to->reset, to->reset == ColorStatusReset ? C_ANSI_RESET : NULL);
            append_attribute(buffer, buffer_size, offset, to->highlight, to->highlight == ColorStatusReset ? C_ANSI_NORMAL_INTENSITY : C_ANSI_BOLD);
            append_attribute(buffer, buffer_size, offset, to->underline, to->underline == ColorStatusReset ? C_ANSI_NO_UNDERLINE : C_ANSI_UNDERLINE);
            append_attribute(buffer, buffer_size, offset, to->flash, to->flash == ColorStatusReset ? C_ANSI_NO_BLINK : C_ANSI_BLINK);
            append_attribute(buffer, buffer_size, offset, to->inverse, to->inverse == ColorStatusReset ? C_ANSI_NO_REVERSE : C_ANSI_REVERSE);
            if (has_fg)
            {
                append_color(buffer, buffer_size, offset, to, type, true);
            }
            if (has_bg)
            {
                append_color(buffer, buffer_size, offset, to, type, false);
            }
            *offset += XSPRINTF(buffer + *offset, "%s", C_ANSI_END);
            state = ColorStatusSet;
        }
        else
        {
            buffer[0] = '\0';
            state = ColorStatusNone;
        }

        if (to->reset == ColorStatusReset)
        {
            // After generating the sequence, reset the ColorState
            *to = (ColorState){
                .foreground = {ColorStatusReset, 0, 0, {0, 0, 0}},
                .background = {ColorStatusReset, 0, 0, {0, 0, 0}},
                .reset = ColorStatusNone,
                .flash = ColorStatusNone,
                .highlight = ColorStatusNone,
                .underline = ColorStatusNone,
                .inverse = ColorStatusNone,
            };
            state = ColorStatusReset;
        }
    }

    return state;
}

/**
 * @brief Build an ANSI escape sequence to transition between two ColorState values.
 *
 * Uses the modern ColorState API and honors the requested ColorType (ANSI, XTerm,
 * TrueColor). When attributes or colors are being cleared, the function emits a
 * reset (ESC[0m) before applying the target state to keep the transition safe.
 *
 * Caller must free the returned buffer with XFREE().
 */
char *ansi_transition_colorstate(const ColorState from, const ColorState to, ColorType type, bool no_default_bg)
{
    static const ColorState kEmpty = {0};

    char *buffer = XMALLOC(SBUF_SIZE, "ansi_transition_colorstate");
    size_t offset = 0;

    if (memcmp(&from, &to, sizeof(ColorState)) == 0)
    {
        buffer[0] = '\0';
        return buffer;
    }

    ColorState state = to;

    /* Respect no_default_bg: do not emit a background reset when target is default. */
    if (no_default_bg)
    {
        bool dst_bg_default = (state.background.is_set != ColorStatusSet) ||
                              (state.background.ansi_index == 0 && state.background.xterm_index == 0 &&
                               state.background.truecolor.r == 0 && state.background.truecolor.g == 0 && state.background.truecolor.b == 0);
        if (dst_bg_default)
        {
            state.background.is_set = ColorStatusNone;
        }
    }

    /* Emit a reset when clearing attributes or colors. */
    bool clearing_attr = (from.highlight == ColorStatusSet && state.highlight != ColorStatusSet) ||
                         (from.underline == ColorStatusSet && state.underline != ColorStatusSet) ||
                         (from.flash == ColorStatusSet && state.flash != ColorStatusSet) ||
                         (from.inverse == ColorStatusSet && state.inverse != ColorStatusSet);

    bool clearing_fg = (from.foreground.is_set == ColorStatusSet) && (state.foreground.is_set != ColorStatusSet);
    bool clearing_bg = (from.background.is_set == ColorStatusSet) && (state.background.is_set != ColorStatusSet);

    if (clearing_attr || clearing_fg || clearing_bg)
    {
        state.reset = ColorStatusReset;
    }

    if (to_ansi_escape_sequence(buffer, SBUF_SIZE, &offset, &state, type) == ColorStatusNone)
    {
        buffer[0] = '\0';
    }

    return buffer;
}

/**
 * @brief Parses embedded ANSI sequences in a string marked with '%x<code>'.
 *
 * Scans the input string for patterns like '%xred' or '%xred/blue', parses the code
 * using ansi_parse_color_from_string, and builds an array of ColorSequence with positions
 * and decoded ColorState.
 *
 * @param input The input string to parse.
 * @param sequences Pointer to the array of ColorSequence (allocated by the function).
 * @param count Pointer to the number of sequences found.
 * @return 0 on success, -1 on error (memory allocation failure).
 *
 * @note The caller is responsible for freeing the allocated sequences array.
 */
int ansi_parse_embedded_sequences(const char *input, ColorSequence *sequences)
{
    if (!input || !sequences)
        return -1;

    sequences->data = NULL;
    sequences->text = NULL;
    sequences->count = 0;

    bool current_highlight = false;

    size_t capacity = 0;
    size_t len = strlen(input);
    size_t text_capacity = len + 1;
    char *text_buffer = XMALLOC(text_capacity, "text_buffer");
    if (!text_buffer)
        return -1;
    size_t text_pos = 0;

    for (size_t i = 0; i < len;)
    {
        if (input[i] == '%' && i + 1 < len && input[i + 1] == 'x')
        {
            size_t pos = text_pos; // position in plain text
            i += 2;                // skip %x
            size_t code_start = i;
            size_t code_len = 0;
            // If code is bracketed (<...> or +<...>), limit to the first closing '>' if present
            if (code_start < len && (input[code_start] == '<' || input[code_start] == '+'))
            {
                size_t j = code_start;
                // find end of first bracketed part
                while (j < len && input[j] != '>')
                    j++;
                if (j < len && input[j] == '>')
                    j++; // include '>'

                // If there's a '/' directly after, include second part (bracketed or not)
                if (j < len && input[j] == '/')
                {
                    size_t k = j + 1;
                    if (k < len && (input[k] == '<' || input[k] == '+'))
                    {
                        // second is bracketed: find its closing '>'
                        size_t m = k;
                        while (m < len && input[m] != '>')
                            m++;
                        if (m < len && input[m] == '>')
                            j = m + 1; // include second closing
                        else
                            j = k; // malformed, end at start of second
                    }
                    else
                    {
                        // second unbracketed: read until space
                        size_t m = k;
                        while (m < len && input[m] != ' ')
                            m++;
                        j = m;
                    }
                }

                code_len = j - code_start;
                // advance i to j (after the combined parts)
                i = j;
            }
            else
            {
                // Non-bracketed code: read until space or end
                size_t j = code_start;
                while (j < len && input[j] != ' ')
                    j++;
                code_len = j - code_start;
                i = j;
            }
            if (code_len > 0)
            {
                char *full = XMALLOC(code_len + 1, "code buffer");
                if (!full)
                    return -1;
                XMEMCPY(full, input + code_start, code_len);
                full[code_len] = '\0';

                ColorState color = {0};
                bool found = false;

                // Decide search direction: if code contains special delimiters ('/','#','<','+') or digits,
                // prefer longest-first to capture combined forms like 'blue/green' or '#rrggbb'.
                bool contains_special = false;
                for (size_t si = 0; si < code_len; si++)
                {
                    char ch = full[si];
                    if (ch == '/' || ch == '#' || ch == '<' || ch == '+' || isdigit((unsigned char)ch))
                    {
                        contains_special = true;
                        break;
                    }
                }

                if (contains_special)
                {
                    // Try longest-first then shorten to find a valid color code prefix.
                    for (ssize_t L = (ssize_t)code_len; L >= 1; L--)
                    {
                        char *part = XMALLOC(L + 1, "code part");
                        if (!part)
                        {
                            XFREE(full);
                            return -1;
                        }
                        XMEMCPY(part, full, L);
                        part[L] = '\0';

                        // Attempt parse on this prefix
                        if (ansi_parse_color_from_string(&color, part, false))
                        {
                            // Ensure the parsed color actually sets something (not a noop)
                            bool has_effect = false;
                            if (color.foreground.is_set == ColorStatusSet || color.background.is_set == ColorStatusSet)
                                has_effect = true;
                            if (color.reset == ColorStatusReset || color.highlight != ColorStatusNone || color.underline != ColorStatusNone || color.flash != ColorStatusNone || color.inverse != ColorStatusNone)
                                has_effect = true;
                            if (!has_effect)
                            {
                                XFREE(part);
                                continue; // try shorter prefix
                            }
                            // Update current highlight state
                            if (color.highlight == ColorStatusSet) {
                                current_highlight = true;
                            } else if (color.highlight == ColorStatusReset) {
                                current_highlight = false;
                            }
                            if (color.reset == ColorStatusReset) {
                                current_highlight = false;
                            }
                            // Apply current highlight to color changes
                            if ((color.foreground.is_set == ColorStatusSet || color.background.is_set == ColorStatusSet) && color.highlight == ColorStatusNone) {
                                color.highlight = current_highlight ? ColorStatusSet : ColorStatusNone;
                            }
                            // Resize array if needed
                            if (sequences->count >= capacity)
                            {
                                capacity = capacity ? capacity * 2 : 4;
                                sequences->data = XREALLOC(sequences->data, capacity * sizeof(ColorSequenceData), "sequences array");
                                if (!sequences->data)
                                {
                                    XFREE(part);
                                    XFREE(full);
                                    return -1;
                                }
                            }
                            sequences->data[sequences->count].position = pos;
                            sequences->data[sequences->count].length = 2 + (size_t)L; // %x + accepted prefix length
                            sequences->data[sequences->count].color = color;
                            sequences->count++;

                            // advance main index to just after the accepted prefix
                            i = code_start + (size_t)L;
                            found = true;
                            XFREE(part);
                            break;
                        }
                        XFREE(part);
                    }
                }
                else
                {
                    // Prefer shortest-first for plain letter sequences so %xn in 'nlain' matches 'n' not 'nlain'.
                    for (size_t L = 1; L <= code_len; L++)
                    {
                        char *part = XMALLOC(L + 1, "code part");
                        if (!part)
                        {
                            XFREE(full);
                            return -1;
                        }
                        XMEMCPY(part, full, L);
                        part[L] = '\0';

                        if (ansi_parse_color_from_string(&color, part, false))
                        {
                            // Ensure the parsed color actually sets something (not a noop)
                            bool has_effect = false;
                            if (color.foreground.is_set == ColorStatusSet || color.background.is_set == ColorStatusSet)
                                has_effect = true;
                            if (color.reset == ColorStatusReset || color.highlight != ColorStatusNone || color.underline != ColorStatusNone || color.flash != ColorStatusNone || color.inverse != ColorStatusNone)
                                has_effect = true;
                            if (!has_effect)
                            {
                                XFREE(part);
                                continue; // try next
                            }
                            // Update current highlight state
                            if (color.highlight == ColorStatusSet) {
                                current_highlight = true;
                            } else if (color.highlight == ColorStatusReset) {
                                current_highlight = false;
                            }
                            if (color.reset == ColorStatusReset) {
                                current_highlight = false;
                            }
                            // Apply current highlight to color changes
                            if ((color.foreground.is_set == ColorStatusSet || color.background.is_set == ColorStatusSet) && color.highlight == ColorStatusNone) {
                                color.highlight = current_highlight ? ColorStatusSet : ColorStatusNone;
                            }

                            // Resize array if needed
                            if (sequences->count >= capacity)
                            {
                                capacity = capacity ? capacity * 2 : 4;
                                sequences->data = XREALLOC(sequences->data, capacity * sizeof(ColorSequenceData), "sequences array");
                                if (!sequences->data)
                                {
                                    XFREE(part);
                                    XFREE(full);
                                    return -1;
                                }
                            }
                            sequences->data[sequences->count].position = pos;
                            sequences->data[sequences->count].length = 2 + (size_t)L; // %x + accepted prefix length
                            sequences->data[sequences->count].color = color;
                            sequences->count++;

                            // advance main index to just after the accepted prefix
                            i = code_start + (size_t)L;
                            found = true;
                            XFREE(part);
                            break;
                        }
                        XFREE(part);
                    }
                }

                XFREE(full);
                // if nothing matched, add sequence for invalid %x and skip
                if (!found)
                {
                    if (sequences->count >= capacity)
                    {
                        capacity = capacity ? capacity * 2 : 4;
                        sequences->data = XREALLOC(sequences->data, capacity * sizeof(ColorSequenceData), "sequences array");
                        if (!sequences->data)
                        {
                            return -1;
                        }
                    }
                    sequences->data[sequences->count].position = pos;
                    sequences->data[sequences->count].length = 2 + code_len;
                    sequences->data[sequences->count].color = (ColorState){0};
                    sequences->data[sequences->count].color.reset = ColorStatusSet;
                    sequences->count++;
                }
                if (!found)
                    i = code_start + code_len;
            }
        }
        else
        {
            // copy character to text_buffer
            if (text_pos + 1 >= text_capacity)
            {
                text_capacity *= 2;
                text_buffer = XREALLOC(text_buffer, text_capacity, "text buffer");
                if (!text_buffer)
                    return -1;
            }
            text_buffer[text_pos++] = input[i++];
        }
    }

    text_buffer[text_pos] = 0;
    sequences->text = text_buffer;
    return 0;
}

/**
 * @brief Parses a single %x color code from the current position.
 *
 * This function is designed for incremental parsing in eval_expression_string().
 * It parses one color code starting at *input_ptr and advances the pointer past
 * the consumed characters.
 *
 * Supported formats:
 * - Bracketed: %x<color>, %x<color/bgcolor>, %x+<color>
 * - Non-bracketed: %xr, %xh, %xn, etc.
 * - Combined: %x<red>/<blue>
 *
 * @param input_ptr Pointer to pointer to current position (after "%x"). Updated on success.
 * @param color_out Output ColorState structure. Must be initialized by caller.
 * @param current_highlight Pointer to persistent highlight state tracker.
 * @return Number of characters consumed (>0) on success, 0 if invalid or no code found.
 *
 * @note The caller must initialize color_out before calling (typically zero it).
 * @note The input_ptr is only advanced on successful parsing.
 */
int ansi_parse_single_x_code(char **input_ptr, ColorState *color_out, bool *current_highlight)
{
    if (!input_ptr || !*input_ptr || !color_out)
        return 0;

    char *start = *input_ptr;
    char *pos = start;
    size_t code_len = 0;

    // Detect if bracketed (<...> or +<...>) or plain letter code
    bool is_bracketed = (*pos == '<' || *pos == '+');

    if (is_bracketed)
    {
        // Bracketed format: read until closing '>' or end
        char *bracket_start = pos;
        size_t bracket_depth = 0;

        // Find the end of the first bracketed part
        while (*pos)
        {
            if (*pos == '<')
                bracket_depth++;
            else if (*pos == '>')
            {
                if (bracket_depth > 0)
                    bracket_depth--;
                if (bracket_depth == 0)
                {
                    pos++; // include closing '>'
                    break;
                }
            }
            pos++;
        }

        // Check for '/' separator for background color
        if (*pos == '/')
        {
            pos++; // skip '/'
            // Read second part (may be bracketed or not)
            if (*pos == '<' || *pos == '+')
            {
                // Second part is bracketed
                bracket_depth = 0;
                while (*pos)
                {
                    if (*pos == '<')
                        bracket_depth++;
                    else if (*pos == '>')
                    {
                        if (bracket_depth > 0)
                            bracket_depth--;
                        if (bracket_depth == 0)
                        {
                            pos++; // include closing '>'
                            break;
                        }
                    }
                    pos++;
                }
            }
            else
            {
                // Second part is non-bracketed: read until space or special char
                while (*pos && !isspace(*pos) && *pos != '%' && *pos != '<' && *pos != '>')
                    pos++;
            }
        }

        code_len = pos - start;
    }
    else
    {
        // Non-bracketed: For simple letter codes like 'r', 'g', 'h', etc.
        // Read exactly ONE character
        // MUSHcode standard: %xr, %xg, %xb, %xh, %xn are all single-char codes
        if (*pos)
        {
            pos++;
        }
        code_len = pos - start;
    }

    if (code_len == 0)
        return 0;

    // Extract the code string
    char *code_buf = XMALLOC(code_len + 1, "single_x_code");
    if (!code_buf)
        return 0;
    XMEMCPY(code_buf, start, code_len);
    code_buf[code_len] = '\0';

    // Attempt to parse the color code
    ColorState temp_color = {0};
    bool parse_success = ansi_parse_color_from_string(&temp_color, code_buf, false);

    if (parse_success)
    {
        // Check if the parsed color actually does something
        bool has_effect = false;
        if (temp_color.foreground.is_set == ColorStatusSet || temp_color.background.is_set == ColorStatusSet)
            has_effect = true;
        if (temp_color.reset == ColorStatusReset || temp_color.highlight != ColorStatusNone ||
            temp_color.underline != ColorStatusNone || temp_color.flash != ColorStatusNone ||
            temp_color.inverse != ColorStatusNone)
            has_effect = true;

        if (has_effect)
        {
            // Update persistent highlight state if provided
            if (current_highlight)
            {
                if (temp_color.highlight == ColorStatusSet)
                    *current_highlight = true;
                else if (temp_color.highlight == ColorStatusReset)
                    *current_highlight = false;
                if (temp_color.reset == ColorStatusReset)
                    *current_highlight = false;

                // Apply persistent highlight to color changes
                if ((temp_color.foreground.is_set == ColorStatusSet || temp_color.background.is_set == ColorStatusSet) &&
                    temp_color.highlight == ColorStatusNone && *current_highlight)
                {
                    temp_color.highlight = ColorStatusSet;
                }
            }

            *color_out = temp_color;
            *input_ptr = pos; // Advance pointer
            XFREE(code_buf);
            return (int)code_len;
        }
    }

    // Parsing failed or had no effect
    XFREE(code_buf);
    return 0;
}

/**
 * @brief Sets a color by ANSI index for foreground or background.
 *
 * @param state The color state to update.
 * @param ansi_index The ANSI color index (0-15).
 * @param is_background true for background, false for foreground.
 */
static void set_color_by_ansi_index(ColorState *state, int ansi_index, bool is_background)
{
    if (ansi_index < 0 || ansi_index >= 16)
        return;

    ColorInfo *color_info = is_background ? &state->background : &state->foreground;
    color_info->is_set = true;
    color_info->ansi_index = ansi_index;
    color_info->xterm_index = colorDefinitions[ansi_index].xterm_index;
    color_info->truecolor = colorDefinitions[ansi_index].truecolor;
}

/**
 * @brief Parses an ANSI escape code and updates the color state.
 *
 * @param state The color state to update.
 * @param code The ANSI code string (e.g., "31" or "38;5;196").
 */
static bool ansi_parse_ansi_code(ColorState *state, const char *code)
{
    if (!code || !*code)
        return false;

    bool changed = false;
    state->reset = ColorStatusNone;

    char *copy = XMALLOC(strlen(code) + 1, "copy");
    strcpy(copy, code);
    char *token = strtok(copy, ";");

    while (token)
    {
        int num = atoi(token);
        switch (num)
        {
        case 0: // reset
            *state = (ColorState){0};
            state->reset = ColorStatusReset;
            changed = true;
            break;
        case 1: // bold
            state->highlight = ColorStatusSet;
            break;
        case 22: // no bold
            state->highlight = ColorStatusReset;
            break;
        // fg colors 30-37
        case 30: set_color_by_ansi_index(state, 0, false); changed = true; break;
        case 31: set_color_by_ansi_index(state, 1, false); changed = true; break;
        case 32: set_color_by_ansi_index(state, 2, false); changed = true; break;
        case 33: set_color_by_ansi_index(state, 3, false); changed = true; break;
        case 34: set_color_by_ansi_index(state, 4, false); changed = true; break;
        case 35: set_color_by_ansi_index(state, 5, false); changed = true; break;
        case 36: set_color_by_ansi_index(state, 6, false); changed = true; break;
        case 37: set_color_by_ansi_index(state, 7, false); changed = true; break;
        // bright fg 90-97
        case 90: set_color_by_ansi_index(state, 8, false); changed = true; break;
        case 91: set_color_by_ansi_index(state, 9, false); changed = true; break;
        case 92: set_color_by_ansi_index(state, 10, false); changed = true; break;
        case 93: set_color_by_ansi_index(state, 11, false); changed = true; break;
        case 94: set_color_by_ansi_index(state, 12, false); changed = true; break;
        case 95: set_color_by_ansi_index(state, 13, false); changed = true; break;
        case 96: set_color_by_ansi_index(state, 14, false); changed = true; break;
        case 97: set_color_by_ansi_index(state, 15, false); changed = true; break;
        // bg 40-47
        case 40: set_color_by_ansi_index(state, 0, true); changed = true; break;
        case 41: set_color_by_ansi_index(state, 1, true); changed = true; break;
        case 42: set_color_by_ansi_index(state, 2, true); changed = true; break;
        case 43: set_color_by_ansi_index(state, 3, true); changed = true; break;
        case 44: set_color_by_ansi_index(state, 4, true); changed = true; break;
        case 45: set_color_by_ansi_index(state, 5, true); changed = true; break;
        case 46: set_color_by_ansi_index(state, 6, true); changed = true; break;
        case 47: set_color_by_ansi_index(state, 7, true); changed = true; break;
        // bright bg 100-107
        case 100: set_color_by_ansi_index(state, 8, true); changed = true; break;
        case 101: set_color_by_ansi_index(state, 9, true); changed = true; break;
        case 102: set_color_by_ansi_index(state, 10, true); changed = true; break;
        case 103: set_color_by_ansi_index(state, 11, true); changed = true; break;
        case 104: set_color_by_ansi_index(state, 12, true); changed = true; break;
        case 105: set_color_by_ansi_index(state, 13, true); changed = true; break;
        case 106: set_color_by_ansi_index(state, 14, true); changed = true; break;
        case 107: set_color_by_ansi_index(state, 15, true); changed = true; break;
        case 39: // fg reset
            reset_color_data(&state->foreground);
            changed = true;
            break;
        case 49: // bg reset
            reset_color_data(&state->background);
            changed = true;
            break;
        case 38: // extended fg
        {
            token = strtok(NULL, ";");
            if (token)
            {
                int mode = atoi(token);
                if (mode == 5)
                {
                    token = strtok(NULL, ";");
                    if (token)
                    {
                        int idx = atoi(token);
                        state->foreground.is_set = true;
                        state->foreground.xterm_index = idx;
                        if (idx < 16)
                        {
                            state->foreground.ansi_index = idx;
                            state->foreground.xterm_index = colorDefinitions[idx].xterm_index;
                            state->foreground.truecolor = colorDefinitions[idx].truecolor;
                        }
                        else
                        {
                            state->foreground.truecolor = colorDefinitions[idx].truecolor;
                            state->foreground.ansi_index = colorDefinitions[idx].ansi_index;
                        }
                        changed = true;
                    }
                }
                else if (mode == 2)
                {
                    token = strtok(NULL, ";");
                    int r = token ? atoi(token) : 0;
                    token = strtok(NULL, ";");
                    int g = token ? atoi(token) : 0;
                    token = strtok(NULL, ";");
                    int b = token ? atoi(token) : 0;
                    state->foreground.is_set = true;
                    state->foreground.truecolor = (ColorRGB){r, g, b};
                    // Compute CIELAB once and reuse for both lookups
                    ColorCIELab lab = ansi_rgb_to_cielab(state->foreground.truecolor);
                    state->foreground.ansi_index = ansi_find_closest_color_with_lab(lab, ColorTypeAnsi).ansi_index;
                    state->foreground.xterm_index = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm).xterm_index;
                    changed = true;
                }
            }
            break;
        }
        case 48: // extended bg similar
        {
            token = strtok(NULL, ";");
            if (token)
            {
                int mode = atoi(token);
                if (mode == 5)
                {
                    token = strtok(NULL, ";");
                    if (token)
                    {
                        int idx = atoi(token);
                        state->background.is_set = true;
                        state->background.xterm_index = idx;
                        if (idx < 16)
                        {
                            state->background.ansi_index = idx;
                            state->background.xterm_index = colorDefinitions[idx].xterm_index;
                            state->background.truecolor = colorDefinitions[idx].truecolor;
                        }
                        else
                        {
                            state->background.truecolor = colorDefinitions[idx].truecolor;
                            state->background.ansi_index = colorDefinitions[idx].ansi_index;
                        }
                        changed = true;
                    }
                }
                else if (mode == 2)
                {
                    token = strtok(NULL, ";");
                    int r = token ? atoi(token) : 0;
                    token = strtok(NULL, ";");
                    int g = token ? atoi(token) : 0;
                    token = strtok(NULL, ";");
                    int b = token ? atoi(token) : 0;
                    state->background.is_set = true;
                    state->background.truecolor = (ColorRGB){r, g, b};
                    // Compute CIELAB once and reuse for both lookups
                    ColorCIELab lab = ansi_rgb_to_cielab(state->background.truecolor);
                    state->background.ansi_index = ansi_find_closest_color_with_lab(lab, ColorTypeAnsi).ansi_index;
                    state->background.xterm_index = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm).xterm_index;
                    changed = true;
                }
            }
            break;
        }
        default:
            break;
        }
        token = strtok(NULL, ";");
    }
    XFREE(copy);
    return changed;
}

/**
 * @brief Parses a string with ANSI escape sequences and converts it to a ColorSequence.
 *
 * This function scans the input string for ANSI escape sequences, interprets them to update the color state,
 * and records the color changes in the provided ColorSequence. It also returns the plain text with ANSI codes removed.
 *
 * @param input The input string containing ANSI sequences.
 * @param sequences The ColorSequence to populate with color changes.
 * @return A newly allocated string with ANSI sequences removed, or NULL on error.
 *
 * @note The caller is responsible for freeing the returned string and the sequences->data.
 */
bool ansi_parse_ansi_to_sequences(const char *input, ColorSequence *sequences)
{
    if (!input)
        return false;

    sequences->count = 0;
    sequences->data = NULL;
    sequences->text = NULL;
    size_t input_len = strlen(input);
    size_t output_pos = 0;
    ColorState current_color = {0};
    size_t i = 0;

    sequences->text = XMALLOC(input_len + 1, "sequences->text");
    if (!sequences->text)
        return false;

    while (i < input_len)
    {
        if (input[i] == '\033' && i + 1 < input_len && input[i + 1] == '[')
        {
            // Find the end of the sequence
            size_t start = i;
            i += 2;
            while (i < input_len && input[i] != 'm')
                i++;
            if (i < input_len)
                i++; // skip 'm'

            // Extract code
            size_t code_len = i - start - 3; // between [ and m
            char *code = XMALLOC(code_len + 1, "code");
            memcpy(code, input + start + 2, code_len);
            code[code_len] = 0;

            // Parse code
            ColorState new_color = current_color;
            bool changed = ansi_parse_ansi_code(&new_color, code);
            XFREE(code);

            // If color changed, add sequence
            if (changed && memcmp(&current_color, &new_color, sizeof(ColorState)) != 0)
            {
                sequences->data = XREALLOC(sequences->data, (sequences->count + 1) * sizeof(ColorSequenceData), "sequences array");
                sequences->data[sequences->count].position = output_pos;
                sequences->data[sequences->count].length = 0;
                sequences->data[sequences->count].color = new_color;
                sequences->count++;
                current_color = new_color;
            }
        }
        else
        {
            sequences->text[output_pos++] = input[i++];
        }
    }

    sequences->text[output_pos] = 0;
    return true;
}

/**
 * @brief Generates a mushcode string from a ColorState.
 *
 * @param color The color state.
 * @return A newly allocated string with the mushcode, or NULL on error.
 */
char *color_state_to_mush_code(const ColorState *color)
{
    char buffer[256];
    size_t len = 0;

    if (color->highlight == true)
        buffer[len++] = '+';

    buffer[len++] = '<';

    // Foreground
    if (color->foreground.is_set)
    {
        if (color->foreground.ansi_index >= 0 && color->foreground.ansi_index < 16)
        {
            // Find name
            for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
            {
                if (colorDefinitions[i].ansi_index == color->foreground.ansi_index)
                {
                    strcpy(buffer + len, colorDefinitions[i].name);
                    len += strlen(colorDefinitions[i].name);
                    break;
                }
            }
        }
        else if (color->foreground.xterm_index >= 0)
        {
            len += sprintf(buffer + len, "xterm%d", color->foreground.xterm_index);
        }
        else
        {
            len += sprintf(buffer + len, "#%02x%02x%02x", color->foreground.truecolor.r, color->foreground.truecolor.g, color->foreground.truecolor.b);
        }
    }

    // Background
    if (color->background.is_set)
    {
        buffer[len++] = '/';
        if (color->background.ansi_index >= 0 && color->background.ansi_index < 16)
        {
            for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
            {
                if (colorDefinitions[i].ansi_index == color->background.ansi_index)
                {
                    strcpy(buffer + len, colorDefinitions[i].name);
                    len += strlen(colorDefinitions[i].name);
                    break;
                }
            }
        }
        else if (color->background.xterm_index >= 0)
        {
            len += sprintf(buffer + len, "xterm%d", color->background.xterm_index);
        }
        else
        {
            len += sprintf(buffer + len, "#%02x%02x%02x", color->background.truecolor.r, color->background.truecolor.g, color->background.truecolor.b);
        }
    }

    buffer[len++] = '>';
    buffer[len] = 0;

    char *result = XMALLOC(len + 1, "result");
    if (result) {
        strcpy(result, buffer);
    }
    return result;
}

/**
 * @brief Generates a letter code string from a ColorState.
 *
 * @param color The color state.
 * @return A newly allocated string with the letter codes, or NULL on error.
 */
char *color_state_to_letters(const ColorState *color)
{
    char buffer[256];
    size_t len = 0;

    // Reset if needed
    if (color->reset == ColorStatusReset)
    {
        buffer[len++] = 'n';
    }

    // Highlight
    if (color->highlight == ColorStatusSet)
    {
        buffer[len++] = 'h';
    }

    // Underline
    if (color->underline == ColorStatusSet)
    {
        buffer[len++] = 'u';
    }

    // Flash/blink
    if (color->flash == ColorStatusSet)
    {
        buffer[len++] = 'f';
    }

    // Inverse
    if (color->inverse == ColorStatusSet)
    {
        buffer[len++] = 'i';
    }

    // Foreground color
    if (color->foreground.is_set == ColorStatusSet)
    {
        if (color->foreground.truecolor.r != 0 || color->foreground.truecolor.g != 0 || color->foreground.truecolor.b != 0)
        {
            // Truecolor is set, use hex format
            len += sprintf(buffer + len, "#%02x%02x%02x", color->foreground.truecolor.r, color->foreground.truecolor.g, color->foreground.truecolor.b);
        }
        else if (color->foreground.ansi_index >= 0 && color->foreground.ansi_index < 16)
        {
            // Find the mush_code for this ANSI index
            for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
            {
                if (colorDefinitions[i].ansi_index == color->foreground.ansi_index && colorDefinitions[i].mush_code != -1)
                {
                    buffer[len++] = (char)tolower(colorDefinitions[i].mush_code);
                    break;
                }
            }
        }
        else if (color->foreground.xterm_index >= 0)
        {
            // For XTerm colors, use 'x' followed by the index
            len += sprintf(buffer + len, "x%d", color->foreground.xterm_index);
        }
    }

    // Background color (if different from foreground)
    if (color->background.is_set == ColorStatusSet)
    {
        buffer[len++] = '/';
        if (color->background.truecolor.r != 0 || color->background.truecolor.g != 0 || color->background.truecolor.b != 0)
        {
            // Truecolor is set, use hex format
            len += sprintf(buffer + len, "#%02x%02x%02x", color->background.truecolor.r, color->background.truecolor.g, color->background.truecolor.b);
        }
        else if (color->background.ansi_index >= 0 && color->background.ansi_index < 16)
        {
            // Find the mush_code for this ANSI index
            for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
            {
                if (colorDefinitions[i].ansi_index == color->background.ansi_index && colorDefinitions[i].mush_code != -1)
                {
                    buffer[len++] = (char)toupper(colorDefinitions[i].mush_code);
                    break;
                }
            }
        }
        else if (color->background.xterm_index >= 0)
        {
            // For XTerm colors, use 'X' followed by the index
            len += sprintf(buffer + len, "X%d", color->background.xterm_index);
        }
    }

    buffer[len] = '\0';

    char *result = XMALLOC(len + 1, "result");
    if (result) {
        strcpy(result, buffer);
    }
    return result;
}

/**
 * @brief Convert a ColorState to an ANSI escape sequence string.
 *
 * @param color The ColorState to convert
 * @param type The ColorType to use for the sequence
 * @return char* Allocated string containing the ANSI escape sequence, or NULL on failure
 */
char *color_state_to_escape(const ColorState *color, ColorType type)
{
    char buffer[256];
    size_t offset = 0;
    if (to_ansi_escape_sequence(buffer, sizeof(buffer), &offset, (ColorState *)color, type) == ColorStatusSet)
    {
        char *result = XMALLOC(offset + 1, "escape");
        if (result) {
            XMEMCPY(result, buffer, offset);
            result[offset] = '\0';
        }
        return result;
    }
    return NULL;
}

/**
 * @brief Safely append an ANSI reset using the ColorState pipeline.
 */
void xsafe_ansi_normal(char *buff, char **bufc)
{
    if (!buff || !bufc || !*bufc)
    {
        return;
    }

    ColorState reset_state = {
        .foreground = {ColorStatusReset, 0, 0, {0, 0, 0}},
        .background = {ColorStatusReset, 0, 0, {0, 0, 0}},
        .reset = ColorStatusReset,
        .flash = ColorStatusNone,
        .highlight = ColorStatusNone,
        .underline = ColorStatusNone,
        .inverse = ColorStatusNone,
    };

    char seq[32];
    size_t offset = 0;

    if (to_ansi_escape_sequence(seq, sizeof(seq), &offset, &reset_state, ColorTypeAnsi) != ColorStatusNone)
    {
        XSAFESTRNCAT(buff, bufc, seq, offset, LBUF_SIZE);
    }
}

/**
 * @brief Parses a %x color code and generates the corresponding ANSI escape sequence.
 *
 * Parses a color code starting from %x, advances the pointer, and generates the ANSI sequence
 * based on the specified color type (ANSI, XTerm, TrueColor).
 *
 * @param ptr Pointer to the string pointer (advanced after parsing).
 * @param type The color type to generate the sequence for.
 * @return A newly allocated string with the ANSI escape sequence, or NULL on error.
 */
char *ansi_parse_x_to_sequence(char **ptr, ColorType type)
{
    if (!ptr || !*ptr)
        return NULL;

    // No need to check for %x prefix since caller already did that
    // *ptr should point to the character after %x

    {
        FILE *debug_file = fopen("/tmp/debug_eval.log", "a");
        if (debug_file) {
            fprintf(debug_file, "DEBUG: ansi_parse_x_to_sequence called with ptr='%s'\n", *ptr);
            fclose(debug_file);
        }
    }

    ColorState color = {0};
    bool parsed = false;

    // Check for bracketed color <...>
    if (**ptr == '<' || **ptr == '+')
    {
        char *start = *ptr;
        (*ptr)++; // Skip < or +
        if (**ptr == '<') {
            (*ptr)++; // Skip second <
        }

        // Find the end >
        while (**ptr && **ptr != '>')
            (*ptr)++;
        if (**ptr == '>')
        {
            size_t len = *ptr - start + 1; // Include the closing >
            char *code = XMALLOC(len + 1, "code");
            if (code) {
                XMEMCPY(code, start, len);
                code[len] = '\0';
                {
                    FILE *debug_file = fopen("/tmp/debug_eval.log", "a");
                    if (debug_file) {
                        fprintf(debug_file, "DEBUG: parsing color code: '%s'\n", code);
                        fclose(debug_file);
                    }
                }
                parsed = ansi_parse_color_from_string(&color, code, false);
                {
                    FILE *debug_file = fopen("/tmp/debug_eval.log", "a");
                    if (debug_file) {
                        fprintf(debug_file, "DEBUG: ansi_parse_color_from_string returned %d\n", parsed);
                        fclose(debug_file);
                    }
                }
                XFREE(code);
            }
            (*ptr)++; // Skip >
        }
        else
        {
            {
                FILE *debug_file = fopen("/tmp/debug_eval.log", "a");
                if (debug_file) {
                    fprintf(debug_file, "DEBUG: no closing > found, ptr at: '%s'\n", *ptr);
                    fclose(debug_file);
                }
            }
            *ptr = start; // Reset on error
        }
    }
    else
    {
        // Single character code
        char code[2] = {**ptr, '\0'};
        if (**ptr) {
            parsed = ansi_parse_color_from_string(&color, code, false);
            (*ptr)++;
        }
    }

    if (parsed)
    {
        char buffer[256];
        size_t offset = 0;
        if (to_ansi_escape_sequence(buffer, sizeof(buffer), &offset, &color, type) == ColorStatusSet)
        {
            {
                FILE *debug_file = fopen("/tmp/debug_eval.log", "a");
                if (debug_file) {
                    fprintf(debug_file, "DEBUG: offset = %zu, buffer starts with: '%.*s'\n", offset, (int)offset > 10 ? 10 : (int)offset, buffer);
                    fclose(debug_file);
                }
            }
            char *result = XMALLOC(offset + 1, "sequence");
            if (result) {
                XMEMCPY(result, buffer, offset);
                result[offset] = '\0';
                {
                    FILE *debug_file = fopen("/tmp/debug_eval.log", "a");
                    if (debug_file) {
                        fprintf(debug_file, "DEBUG: generated sequence length: %zu, first 5 chars: %02x %02x %02x %02x %02x\n", strlen(result), (unsigned char)result[0], (unsigned char)result[1], (unsigned char)result[2], (unsigned char)result[3], (unsigned char)result[4]);
                        fclose(debug_file);
                    }
                }
            }
            return result;
        }
    }

    return NULL;
}

/**
 * @brief Convert ansi character code (%x?) to ansi sequence.
 * Replacement for the ansiChar lookup table using colorDefinitions.
 *
 * @param ch Character to convert
 * @return const char* ANSI sequence or empty string if not found
 */
const char *ansi_char_to_sequence(int ch)
{
    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (colorDefinitions[i].mush_code == ch)
        {
            // Build ANSI escape sequence for this color
            static char buffer[16];
            if (colorDefinitions[i].ansi_index >= 30 && colorDefinitions[i].ansi_index <= 37)
            {
                // Foreground color
                snprintf(buffer, sizeof(buffer), "\033[%dm", 30 + colorDefinitions[i].ansi_index);
                return buffer;
            }
            else if (colorDefinitions[i].ansi_index >= 40 && colorDefinitions[i].ansi_index <= 47)
            {
                // Background color
                snprintf(buffer, sizeof(buffer), "\033[%dm", 40 + (colorDefinitions[i].ansi_index - 8));
                return buffer;
            }
        }
    }
    return STRING_EMPTY;
}

/**
 * @brief Convert ansi character code (%x? uppercase) to bright ansi sequence.
 * Replacement for the ansiChar_Bright lookup table using colorDefinitions.
 *
 * @param ch Character to convert (should match lowercase versions)
 * @return const char* Bright ANSI sequence or empty string if not found
 */
const char *ansi_char_bright_to_sequence(int ch)
{
    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (colorDefinitions[i].mush_code == ch)
        {
            // Build bright ANSI escape sequence for this color
            static char buffer[16];
            if (colorDefinitions[i].ansi_index >= 0 && colorDefinitions[i].ansi_index <= 7)
            {
                // Bright foreground color (90-97)
                snprintf(buffer, sizeof(buffer), "\033[%dm", 90 + colorDefinitions[i].ansi_index);
                return buffer;
            }
        }
    }
    return STRING_EMPTY;
}

/**
 * @brief Convert ansi character code to numeric values.
 * Replacement for the ansiNum lookup table using colorDefinitions.
 *
 * @param ch ANSI character
 * @return int ANSI numeric value or 0 if not found
 */
int ansi_char_to_num(int ch)
{
    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (colorDefinitions[i].mush_code == ch)
        {
            return colorDefinitions[i].ansi_index;
        }
    }
    return 0; // Not found
}

/**
 * @brief Convert a mushcode color letter to an ANSI SGR number (30-37 / 40-47).
 *
 * Supports attribute letters used by ansiNum for compatibility (h,u,f,i,n).
 *
 * @param ch Mushcode character (foreground lowercase, background uppercase)
 * @return int Corresponding SGR code, or 0 if not found/unsupported
 */
int mushcode_to_sgr(int ch)
{
    /* Attribute shorthands retained for compatibility. */
    switch (ch)
    {
    case 'h':
        return 1; /* highlight/bold */
    case 'u':
        return 4; /* underline */
    case 'f':
        return 5; /* flash/blink */
    case 'i':
        return 7; /* inverse */
    case 'n':
        return 0; /* normal */
    default:
        break;
    }

    bool background = isupper(ch) ? true : false;
    int mush_code = tolower(ch);

    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (colorDefinitions[i].type != ColorTypeAnsi)
            continue;

        if (colorDefinitions[i].mush_code == mush_code &&
            colorDefinitions[i].ansi_index >= 0 && colorDefinitions[i].ansi_index <= 15)
        {
            int idx = colorDefinitions[i].ansi_index & 0xF;
            return (background ? 40 : 30) + idx;
        }
    }

    return 0;
}


ColorEntry colorDefinitions[] = {
    {"black", ColorTypeAnsi, 120, 0, 0, {0, 0, 0}, {0.000000000000000, 0.000000000000000, 0.000000000000000}},
    {"red", ColorTypeAnsi, 114, 1, 1, {128, 0, 0}, {25.535530963463174, 48.045128262358347, 38.057349239387428}},
    {"green", ColorTypeAnsi, 103, 2, 2, {0, 128, 0}, {46.227431468762596, -51.698495529891062, 49.896846001056097}},
    {"yellow", ColorTypeAnsi, 121, 3, 3, {128, 128, 0}, {51.868943377343967, -12.929464306735028, 56.674579008994250}},
    {"blue", ColorTypeAnsi, 98, 4, 4, {0, 0, 128}, {12.971966857430804, 47.502281324713167, -64.702162125995883}},
    {"magenta", ColorTypeAnsi, 109, 5, 5, {128, 0, 128}, {29.784666617920195, 58.927895811994119, -36.487077091203105}},
    {"cyan", ColorTypeAnsi, 99, 6, 6, {0, 128, 128}, {48.254093461861586, -28.846304196984779, -8.476885985257221}},
    {"white", ColorTypeAnsi, 119, 7, 7, {192, 192, 192}, {77.704366713431412, -0.000013463270276, 0.000005385308111}},
    {"brightblack", ColorTypeAnsi, -1, 8, 8, {128, 128, 128}, {53.585015771669404, -0.000009997846440, 0.000003999138576}},
    {"brightred", ColorTypeAnsi, -1, 9, 9, {255, 0, 0}, {53.240794141307191, 80.092459596411146, 67.203196515852966}},
    {"brightgreen", ColorTypeAnsi, -1, 10, 10, {0, 255, 0}, {87.734722352797917, -86.182716420534661, 83.179320502697834}},
    {"brightyellow", ColorTypeAnsi, -1, 11, 11, {255, 255, 0}, {97.139267224306309, -21.553748216377233, 94.477975053670306}},
    {"brightblue", ColorTypeAnsi, -1, 12, 12, {0, 0, 255}, {32.297010932850725, 79.187519845122182, -107.860161754148095}},
    {"brightmagenta", ColorTypeAnsi, -1, 13, 13, {255, 0, 255}, {60.324212128368742, 98.234311888004015, -60.824892208850059}},
    {"brightcyan", ColorTypeAnsi, -1, 14, 14, {0, 255, 255}, {91.113219812758601, -48.087528058758210, -14.131186091754454}},
    {"brightwhite", ColorTypeAnsi, -1, 15, 15, {255, 255, 255}, {100.000003866666546, -0.000016666666158, 0.000006666666463}},
    {"color16", ColorTypeXTerm, -1, 0, 16, {0, 0, 0}, {0.000000000000000, 0.000000000000000, 0.000000000000000}},
    {"color17", ColorTypeXTerm, -1, 4, 17, {0, 0, 95}, {7.460625651630473, 38.391183420853878, -52.344155180410709}},
    {"color18", ColorTypeXTerm, -1, 4, 18, {0, 0, 135}, {14.108799924144989, 49.366226717859504, -67.241014864392881}},
    {"color19", ColorTypeXTerm, -1, 4, 19, {0, 0, 175}, {20.416779685021325, 59.708756469680353, -81.328423261073439}},
    {"color20", ColorTypeXTerm, -1, 12, 20, {0, 0, 215}, {26.461218690484877, 69.619186213733286, -94.827274561598401}},
    {"color21", ColorTypeXTerm, -1, 12, 21, {0, 0, 255}, {32.297010932850725, 79.187519845122182, -107.860161754148095}},
    {"color22", ColorTypeXTerm, -1, 2, 22, {0, 95, 0}, {34.362921265610460, -41.841470850832756, 40.383330425825925}},
    {"color23", ColorTypeXTerm, -1, 6, 23, {0, 95, 95}, {36.003172453572269, -23.346362091224648, -6.860651827922104}},
    {"color24", ColorTypeXTerm, -1, 12, 24, {0, 95, 135}, {37.721074382988249, -8.280292131882833, -28.838129157871116}},
    {"color25", ColorTypeXTerm, -1, 12, 25, {0, 95, 175}, {40.044711958647966, 8.050351383641324, -49.077928688197261}},
    {"color26", ColorTypeXTerm, -1, 12, 26, {0, 95, 215}, {42.896244395419316, 24.232072157966023, -67.665859002333107}},
    {"color27", ColorTypeXTerm, -1, 12, 27, {0, 95, 255}, {46.179102555302748, 39.611554760542731, -84.835618823233546}},
    {"color28", ColorTypeXTerm, -1, 2, 28, {0, 135, 0}, {48.669178075007821, -53.727096470482103, 51.854751887732121}},
    {"color29", ColorTypeXTerm, -1, 6, 29, {0, 135, 95}, {49.680825029057942, -41.468212946969643, 12.871275515797853}},
    {"color30", ColorTypeXTerm, -1, 6, 30, {0, 135, 135}, {50.775364402099939, -29.978206377634375, -8.809511116931157}},
    {"color31", ColorTypeXTerm, -1, 6, 31, {0, 135, 175}, {52.309746769800341, -16.087684890932152, -29.668379827293776}},
    {"color32", ColorTypeXTerm, -1, 8, 32, {0, 135, 215}, {54.271652434110663, -0.984531349612072, -49.346593125232310}},
    {"color33", ColorTypeXTerm, -1, 8, 33, {0, 135, 255}, {56.628677140198647, 14.436592813425408, -67.825763830649663}},
    {"color34", ColorTypeXTerm, -1, 2, 34, {0, 175, 0}, {62.217770761444555, -64.983255400771270, 62.718643050354338}},
    {"color35", ColorTypeXTerm, -1, 2, 35, {0, 175, 95}, {62.913962945232470, -56.274790575092382, 30.552785867035091}},
    {"color36", ColorTypeXTerm, -1, 6, 36, {0, 175, 135}, {63.677487293548509, -47.533737732975425, 9.989760176164442}},
    {"color37", ColorTypeXTerm, -1, 6, 37, {0, 175, 175}, {64.765216147595893, -36.258825982995738, -10.655158169248713}},
    {"color38", ColorTypeXTerm, -1, 6, 38, {0, 175, 215}, {66.184273763155460, -23.179986428578381, -30.659175500468592}},
    {"color39", ColorTypeXTerm, -1, 7, 39, {0, 175, 255}, {67.928677568625176, -9.021871353134149, -49.792238009085679}},
    {"color40", ColorTypeXTerm, -1, 10, 40, {0, 215, 0}, {75.200317505009110, -75.769143857788706, 73.128652274185683}},
    {"color41", ColorTypeXTerm, -1, 10, 41, {0, 215, 95}, {75.714080550985500, -69.238116489090842, 46.415770919245404}},
    {"color42", ColorTypeXTerm, -1, 10, 42, {0, 215, 135}, {76.281325259270375, -62.437098531106628, 27.358874341349626}},
    {"color43", ColorTypeXTerm, -1, 14, 43, {0, 215, 175}, {77.096124548928913, -53.317791084919520, 7.414753612450053}},
    {"color44", ColorTypeXTerm, -1, 14, 44, {0, 215, 215}, {78.170586611146760, -42.277047911446097, -12.423696030219421}},
    {"color45", ColorTypeXTerm, -1, 14, 45, {0, 215, 255}, {79.508487474754645, -29.803889330068721, -31.743841087923453}},
    {"color46", ColorTypeXTerm, -1, 10, 46, {0, 255, 0}, {87.734722352797917, -86.182716420534661, 83.179320502697834}},
    {"color47", ColorTypeXTerm, -1, 10, 47, {0, 255, 95}, {88.132542757660701, -81.079313708878445, 60.784275876154091}},
    {"color48", ColorTypeXTerm, -1, 10, 48, {0, 255, 135}, {88.573417721235387, -75.649889406820435, 43.369239590672429}},
    {"color49", ColorTypeXTerm, -1, 10, 49, {0, 255, 175}, {89.209663616259860, -68.192330192837105, 24.408751698056520}},
    {"color50", ColorTypeXTerm, -1, 14, 50, {0, 255, 215}, {90.053902775072117, -58.903862939537092, 5.054881719426896}},
    {"color51", ColorTypeXTerm, -1, 14, 51, {0, 255, 255}, {91.113219812758601, -48.087528058758210, -14.131186091754454}},
    {"color52", ColorTypeXTerm, -1, 1, 52, {95, 0, 0}, {17.616214365015260, 38.884667979393775, 27.208175889909437}},
    {"color53", ColorTypeXTerm, -1, 5, 53, {95, 0, 95}, {21.055194238702356, 47.692487172917438, -29.530317215149260}},
    {"color54", ColorTypeXTerm, -1, 5, 54, {95, 0, 135}, {24.265489210328006, 55.109278732853205, -50.109928835317184}},
    {"color55", ColorTypeXTerm, -1, 12, 55, {95, 0, 175}, {28.188459992958727, 63.497258190811031, -68.189397622967604}},
    {"color56", ColorTypeXTerm, -1, 12, 56, {95, 0, 215}, {32.565033965778362, 72.278448064937379, -84.495139630790760}},
    {"color57", ColorTypeXTerm, -1, 12, 57, {95, 0, 255}, {37.209055336406315, 81.157733718029036, -99.539333944464076}},
    {"color58", ColorTypeXTerm, -1, 3, 58, {95, 95, 0}, {38.928801832503417, -10.464285243936084, 45.868796012003457}},
    {"color59", ColorTypeXTerm, -1, 8, 59, {95, 95, 95}, {40.317681573183130, -0.000008091620346, 0.000003236648161}},
    {"color60", ColorTypeXTerm, -1, 8, 60, {95, 95, 135}, {41.792414797738459, 9.716881294079826, -22.184768308096857}},
    {"color61", ColorTypeXTerm, -1, 12, 61, {95, 95, 175}, {43.816567993205950, 21.358548223725737, -42.829511326223326}},
    {"color62", ColorTypeXTerm, -1, 12, 62, {95, 95, 215}, {46.341283315320396, 33.910620696578079, -61.915173192259189}},
    {"color63", ColorTypeXTerm, -1, 12, 63, {95, 95, 255}, {49.295489502502932, 46.651030319845134, -79.609352284264006}},
    {"color64", ColorTypeXTerm, -1, 3, 64, {95, 135, 0}, {51.565360443715605, -31.106940712781139, 55.362292601038952}},
    {"color65", ColorTypeXTerm, -1, 2, 65, {95, 135, 95}, {52.493892460973584, -22.366057175051356, 17.186390849228527}},
    {"color66", ColorTypeXTerm, -1, 6, 66, {95, 135, 135}, {53.502317771880314, -13.755714234635564, -4.459619962025574}},
    {"color67", ColorTypeXTerm, -1, 8, 67, {95, 135, 175}, {54.922245628112549, -2.860324373983758, -25.412901389666942}},
    {"color68", ColorTypeXTerm, -1, 8, 68, {95, 135, 215}, {56.747662338017136, 9.522784801069829, -45.263794377358060}},
    {"color69", ColorTypeXTerm, -1, 8, 69, {95, 135, 255}, {58.953975004544873, 22.669707903308524, -63.961918568429923}},
    {"color70", ColorTypeXTerm, -1, 2, 70, {95, 175, 0}, {64.235030608533904, -48.203292100274687, 65.170137331236077}},
    {"color71", ColorTypeXTerm, -1, 2, 71, {95, 175, 95}, {64.897084178142862, -41.171043157843648, 33.487405569896509}},
    {"color72", ColorTypeXTerm, -1, 6, 72, {95, 175, 135}, {65.624131644125043, -33.963342558051338, 13.012988999486907}},
    {"color73", ColorTypeXTerm, -1, 6, 73, {95, 175, 175}, {66.661569146075919, -24.464552598424451, -7.626328228409895}},
    {"color74", ColorTypeXTerm, -1, 7, 74, {95, 175, 215}, {68.017831087992946, -13.189300268881542, -27.680081170662319}},
    {"color75", ColorTypeXTerm, -1, 7, 75, {95, 175, 255}, {69.689139413601225, -0.708181908591232, -46.900093254440463}},
    {"color76", ColorTypeXTerm, -1, 10, 76, {95, 215, 0}, {76.698000960306643, -62.880680606606340, 74.951857201934928}},
    {"color77", ColorTypeXTerm, -1, 10, 77, {95, 215, 95}, {77.195429465868614, -57.221495475743126, 48.537298261789360}},
    {"color78", ColorTypeXTerm, -1, 10, 78, {95, 215, 135}, {77.744942919548976, -51.270878468759221, 29.570907879869111}},
    {"color79", ColorTypeXTerm, -1, 14, 79, {95, 215, 175}, {78.534816211608202, -43.206115965756183, 9.664418553499887}},
    {"color80", ColorTypeXTerm, -1, 14, 80, {95, 215, 215}, {79.577356422023115, -33.321294872626972, -10.175417525310193}},
    {"color81", ColorTypeXTerm, -1, 14, 81, {95, 215, 255}, {80.876951634569693, -22.010117118455629, -29.524777351536670}},
    {"color82", ColorTypeXTerm, -1, 10, 82, {95, 255, 0}, {88.898350817232881, -75.968373374891257, 84.597226474858829}},
    {"color83", ColorTypeXTerm, -1, 10, 83, {95, 255, 95}, {89.287443061893399, -71.354716968962734, 62.392493039190015}},
    {"color84", ColorTypeXTerm, -1, 10, 84, {95, 255, 135}, {89.718758243120334, -66.422134329011556, 45.055575826214977}},
    {"color85", ColorTypeXTerm, -1, 10, 85, {95, 255, 175}, {90.341413671709518, -59.608535556590070, 26.140485231163012}},
    {"color86", ColorTypeXTerm, -1, 14, 86, {95, 255, 215}, {91.167985976429136, -51.063898476302491, 6.804467846860396}},
    {"color87", ColorTypeXTerm, -1, 14, 87, {95, 255, 255}, {92.205708638600356, -41.038767130076202, -12.384583364569203}},
    {"color88", ColorTypeXTerm, -1, 1, 88, {135, 0, 0}, {27.165346615094897, 49.930374465074031, 40.136737640044515}},
    {"color89", ColorTypeXTerm, -1, 5, 89, {135, 0, 95}, {29.358410461767988, 55.725043632748580, -15.903001261779925}},
    {"color90", ColorTypeXTerm, -1, 5, 90, {135, 0, 135}, {31.581214405506373, 61.240171704087466, -37.918796102520112}},
    {"color91", ColorTypeXTerm, -1, 5, 91, {135, 0, 175}, {34.491549215002507, 68.043424556733541, -57.611837164977210}},
    {"color92", ColorTypeXTerm, -1, 12, 92, {135, 0, 215}, {37.945003388948471, 75.652936440855555, -75.432962464054413}},
    {"color93", ColorTypeXTerm, -1, 12, 93, {135, 0, 255}, {41.798485948926867, 83.706895548619826, -91.791834460121848}},
    {"color94", ColorTypeXTerm, -1, 3, 94, {135, 95, 0}, {43.266004026343118, 9.134591738944820, 50.930049023455503}},
    {"color95", ColorTypeXTerm, -1, 8, 95, {135, 95, 95}, {44.465038699581896, 16.311046901619164, 6.512750645761322}},
    {"color96", ColorTypeXTerm, -1, 5, 96, {135, 95, 135}, {45.750667884303255, 23.372977602598766, -15.766712152778783}},
    {"color97", ColorTypeXTerm, -1, 5, 97, {135, 95, 175}, {47.534438511953063, 32.300942713031247, -36.702981823418625}},
    {"color98", ColorTypeXTerm, -1, 13, 98, {135, 95, 215}, {49.787277586634204, 42.444465301274491, -56.184495163539118}},
    {"color99", ColorTypeXTerm, -1, 13, 99, {135, 95, 255}, {52.457916635036284, 53.224022366874294, -74.320646070684631}},
    {"color100", ColorTypeXTerm, -1, 3, 100, {135, 135, 0}, {54.532057670342994, -13.436804475634501, 58.898436842899180}},
    {"color101", ColorTypeXTerm, -1, 3, 101, {135, 135, 95}, {55.385515762337135, -6.768114272451776, 21.580884304635962}},
    {"color102", ColorTypeXTerm, -1, 8, 102, {135, 135, 135}, {56.315467151319965, -0.000010390152627, 0.000004156061029}},
    {"color103", ColorTypeXTerm, -1, 8, 103, {135, 135, 175}, {57.630008431840906, 8.825705178205245, -21.021346545667519}},
    {"color104", ColorTypeXTerm, -1, 8, 104, {135, 135, 215}, {59.328133648389596, 19.179536201809789, -41.022228937455822}},
    {"color105", ColorTypeXTerm, -1, 13, 105, {135, 135, 255}, {61.391858334452010, 30.508200494133607, -59.920728447517632}},
    {"color106", ColorTypeXTerm, -1, 3, 106, {135, 175, 0}, {66.374922483879956, -33.335626528252391, 67.745824584235208}},
    {"color107", ColorTypeXTerm, -1, 3, 107, {135, 175, 95}, {67.003415494996261, -27.527170242870881, 36.582860514459689}},
    {"color108", ColorTypeXTerm, -1, 7, 108, {135, 175, 135}, {67.694487067707698, -21.482419115287200, 16.212525626028906}},
    {"color109", ColorTypeXTerm, -1, 7, 109, {135, 175, 175}, {68.682127178874609, -13.384693177367069, -4.410654019094640}},
    {"color110", ColorTypeXTerm, -1, 7, 110, {135, 175, 215}, {69.975891880284848, -3.594072909216428, -24.507224462390688}},
    {"color111", ColorTypeXTerm, -1, 7, 111, {135, 175, 255}, {71.574010298372684, 7.447857699898752, -43.809975113841368}},
    {"color112", ColorTypeXTerm, -1, 10, 112, {135, 215, 0}, {78.315904092243699, -50.585276719043563, 76.909138717598125}},
    {"color113", ColorTypeXTerm, -1, 10, 113, {135, 215, 95}, {78.796542671397788, -45.651433804236085, 50.818542254758285}},
    {"color114", ColorTypeXTerm, -1, 10, 114, {135, 215, 135}, {79.327805196698421, -40.421790106341945, 31.953766553489118}},
    {"color115", ColorTypeXTerm, -1, 14, 115, {135, 215, 175}, {80.091977512699671, -33.269832355739773, 12.092109527410621}},
    {"color116", ColorTypeXTerm, -1, 14, 116, {135, 215, 215}, {81.101527945994448, -24.409844024134099, -7.745062824675530}},
    {"color117", ColorTypeXTerm, -1, 14, 117, {135, 215, 255}, {82.361424977648824, -14.154994285491551, -27.121909172373691}},
    {"color118", ColorTypeXTerm, -1, 10, 118, {135, 255, 0}, {90.168532111002250, -65.770181894211305, 86.138290356110502}},
    {"color119", ColorTypeXTerm, -1, 10, 119, {135, 255, 95}, {90.548419717832132, -61.599051810111163, 64.141610627571993}},
    {"color120", ColorTypeXTerm, -1, 10, 120, {135, 255, 135}, {90.969646492912048, -57.119911215834321, 46.891522423829038}},
    {"color121", ColorTypeXTerm, -1, 10, 121, {135, 255, 175}, {91.577947846580159, -50.900805474440922, 28.027874517335484}},
    {"color122", ColorTypeXTerm, -1, 14, 122, {135, 255, 215}, {92.385839501402884, -43.052445250476467, 8.713289282625848}},
    {"color123", ColorTypeXTerm, -1, 14, 123, {135, 255, 255}, {93.400696336072485, -33.779293443122860, -10.477090149106093}},
    {"color124", ColorTypeXTerm, -1, 1, 124, {175, 0, 0}, {36.208753642449636, 60.391096658325175, 50.573834878772963}},
    {"color125", ColorTypeXTerm, -1, 5, 125, {175, 0, 95}, {37.739975259026338, 64.495259264266409, -2.438322550173100}},
    {"color126", ColorTypeXTerm, -1, 5, 126, {175, 0, 135}, {39.353431226360001, 68.650312668398939, -25.128729815674621}},
    {"color127", ColorTypeXTerm, -1, 5, 127, {175, 0, 175}, {41.549773040324659, 74.070366352668870, -45.863018355618912}},
    {"color128", ColorTypeXTerm, -1, 5, 128, {175, 0, 215}, {44.264011346717332, 80.458447605669164, -64.848646341021009}},
    {"color129", ColorTypeXTerm, -1, 13, 129, {175, 0, 255}, {47.410429079903253, 87.520358762166680, -82.356598100949640}},
    {"color130", ColorTypeXTerm, -1, 9, 130, {175, 95, 0}, {48.637024635137664, 27.330266658925627, 57.029238974415073}},
    {"color131", ColorTypeXTerm, -1, 9, 131, {175, 95, 95}, {49.649655151623165, 32.345899870208697, 14.536338418404515}},
    {"color132", ColorTypeXTerm, -1, 13, 132, {175, 95, 135}, {50.745208676398960, 37.483199023288726, -7.743369456228422}},
    {"color133", ColorTypeXTerm, -1, 13, 133, {175, 95, 175}, {52.280931410582610, 44.249598510837373, -28.930912921483287}},
    {"color134", ColorTypeXTerm, -1, 13, 134, {175, 95, 215}, {54.244424535236789, 52.280309659388124, -48.806029217093496}},
    {"color135", ColorTypeXTerm, -1, 13, 135, {175, 95, 255}, {56.603188754615118, 61.178271180936939, -67.411893709407053}},
    {"color136", ColorTypeXTerm, -1, 3, 136, {175, 135, 0}, {58.455995762322445, 5.073270072779579, 63.495100265143797}},
    {"color137", ColorTypeXTerm, -1, 8, 137, {175, 135, 95}, {59.223238664546486, 10.069965963635962, 27.347971012803885}},
    {"color138", ColorTypeXTerm, -1, 8, 138, {175, 135, 135}, {60.062286230391265, 15.267319559608417, 5.894810994668997}},
    {"color139", ColorTypeXTerm, -1, 8, 139, {175, 135, 175}, {61.253486637217236, 22.225762773314340, -15.176012108313429}},
    {"color140", ColorTypeXTerm, -1, 13, 140, {175, 135, 215}, {62.800706048384669, 30.633519141199162, -35.336720233800342}},
    {"color141", ColorTypeXTerm, -1, 13, 141, {175, 135, 255}, {64.692888930431934, 40.111206009376176, -54.465102828678823}},
    {"color142", ColorTypeXTerm, -1, 3, 142, {175, 175, 0}, {69.308959884924391, -16.251898099315763, 71.238023558021695}},
    {"color143", ColorTypeXTerm, -1, 3, 143, {175, 175, 95}, {69.895406410948311, -11.599340120606428, 40.796840110057218}},
    {"color144", ColorTypeXTerm, -1, 7, 144, {175, 175, 135}, {70.541245892837750, -6.687199686850553, 20.584981195931750}},
    {"color145", ColorTypeXTerm, -1, 7, 145, {175, 175, 175}, {71.466004679819548, -0.000012566953866, 0.000005026781569}},
    {"color146", ColorTypeXTerm, -1, 7, 146, {175, 175, 215}, {72.680406856610844, 8.238495232775255, -20.139565390411441}},
    {"color147", ColorTypeXTerm, -1, 7, 147, {175, 175, 255}, {74.184958936458202, 17.716312909865195, -39.540667596114034}},
    {"color148", ColorTypeXTerm, -1, 11, 148, {175, 215, 0}, {80.579919559485788, -35.513902967655611, 79.627399909235535}},
    {"color149", ColorTypeXTerm, -1, 10, 149, {175, 215, 95}, {81.038447570262619, -31.346985973221997, 53.992269321914236}},
    {"color150", ColorTypeXTerm, -1, 10, 150, {175, 215, 135}, {81.545636975547595, -26.892899894186861, 35.276027687830933}},
    {"color151", ColorTypeXTerm, -1, 7, 151, {175, 215, 175}, {82.275841603881773, -20.742162636621707, 15.484098974044191}},
    {"color152", ColorTypeXTerm, -1, 7, 152, {175, 215, 215}, {83.241675232090458, -13.032555969561487, -4.342368423408516}},
    {"color153", ColorTypeXTerm, -1, 7, 153, {175, 215, 255}, {84.448794030695282, -3.993322243321051, -23.750840977396436}},
    {"color154", ColorTypeXTerm, -1, 10, 154, {175, 255, 0}, {91.967823782293664, -52.701250713142187, 88.309654481122962}},
    {"color155", ColorTypeXTerm, -1, 10, 155, {175, 255, 95}, {92.335219518813915, -49.036718758133425, 66.608006292186175}},
    {"color156", ColorTypeXTerm, -1, 10, 156, {175, 255, 135}, {92.742744090458402, -45.081868499869948, 49.483561267144481}},
    {"color157", ColorTypeXTerm, -1, 10, 157, {175, 255, 175}, {93.331529879610386, -39.558175146381267, 30.696050369308779}},
    {"color158", ColorTypeXTerm, -1, 14, 158, {175, 255, 215}, {94.113989155126362, -32.535991484666461, 11.415203338914903}},
    {"color159", ColorTypeXTerm, -1, 14, 159, {175, 255, 255}, {95.097662984384854, -24.169464250042793, -7.773705014063825}},
    {"color160", ColorTypeXTerm, -1, 9, 160, {215, 0, 0}, {44.874336642169368, 70.414780888000180, 59.082944655242585}},
    {"color161", ColorTypeXTerm, -1, 5, 161, {215, 0, 95}, {46.012582444569894, 73.488281880257361, 10.528987625158393}},
    {"color162", ColorTypeXTerm, -1, 5, 162, {215, 0, 135}, {47.236695412360547, 76.706186258913618, -12.348561816954074}},
    {"color163", ColorTypeXTerm, -1, 13, 163, {215, 0, 175}, {48.940883508482770, 81.051412996766331, -33.681817987708861}},
    {"color164", ColorTypeXTerm, -1, 13, 164, {215, 0, 215}, {51.101855787047867, 86.364528972303290, -53.475339364085769}},
    {"color165", ColorTypeXTerm, -1, 13, 165, {215, 0, 255}, {53.674596918885584, 92.446329833108351, -71.879038275882550}},
    {"color166", ColorTypeXTerm, -1, 9, 166, {215, 95, 0}, {54.695303524864670, 43.548940042661741, 63.726907785303808}},
    {"color167", ColorTypeXTerm, -1, 9, 167, {215, 95, 95}, {55.544894573469293, 47.195327305074194, 23.494868325155693}},
    {"color168", ColorTypeXTerm, -1, 13, 168, {215, 95, 135}, {56.470786376854846, 51.029166002518224, 1.345906040834666}},
    {"color169", ColorTypeXTerm, -1, 13, 169, {215, 95, 175}, {57.779848115428678, 56.225392580026899, -20.000213477922713}},
    {"color170", ColorTypeXTerm, -1, 13, 170, {215, 95, 215}, {59.471313008641900, 62.597128824164905, -40.204567206281318}},
    {"color171", ColorTypeXTerm, -1, 13, 171, {215, 95, 255}, {61.527523522789380, 69.897355223593195, -59.241372879339124}},
    {"color172", ColorTypeXTerm, -1, 3, 172, {215, 135, 0}, {63.159653902392236, 22.859864645953408, 68.897395561378417}},
    {"color173", ColorTypeXTerm, -1, 9, 173, {215, 135, 95}, {63.839588332232111, 26.634208180961128, 34.185583098731584}},
    {"color174", ColorTypeXTerm, -1, 9, 174, {215, 135, 135}, {64.585756457585376, 30.632857611486376, 12.941229879405846}},
    {"color175", ColorTypeXTerm, -1, 13, 175, {215, 135, 175}, {65.649581247586909, 36.098152438407915, -8.134153096622111}},
    {"color176", ColorTypeXTerm, -1, 13, 176, {215, 135, 215}, {67.038831954950211, 42.864229372086896, -28.433915087787632}},
    {"color177", ColorTypeXTerm, -1, 13, 177, {215, 135, 255}, {68.748595742023355, 50.691293293820713, -47.788926882077497}},
    {"color178", ColorTypeXTerm, -1, 11, 178, {215, 175, 0}, {72.964214340962144, 1.430075559485933, 75.529187825435486}},
    {"color179", ColorTypeXTerm, -1, 3, 179, {215, 175, 95}, {73.503895451927661, 5.119471498841399, 45.996429081990954}},
    {"color180", ColorTypeXTerm, -1, 7, 180, {215, 175, 135}, {74.099223641858458, 9.062100957320684, 26.005434721061850}},
    {"color181", ColorTypeXTerm, -1, 7, 181, {215, 175, 175}, {74.953413309636247, 14.504782615611667, 5.492273466086472}},
    {"color182", ColorTypeXTerm, -1, 7, 182, {215, 175, 215}, {76.078172284731565, 21.324041979805976, -14.677148948241925}},
    {"color183", ColorTypeXTerm, -1, 13, 183, {215, 175, 255}, {77.476211400247891, 29.315741719058707, -34.177868409179048}},
    {"color184", ColorTypeXTerm, -1, 11, 184, {215, 215, 0}, {83.468498677313903, -18.949380074217647, 83.062075327303901}},
    {"color185", ColorTypeXTerm, -1, 11, 185, {215, 215, 95}, {83.900954622989872, -15.492586675406649, 58.010021464226050}},
    {"color186", ColorTypeXTerm, -1, 11, 186, {215, 215, 135}, {84.379702592030171, -11.768723875181964, 39.493278094874462}},
    {"color187", ColorTypeXTerm, -1, 7, 187, {215, 215, 175}, {85.069678098261363, -6.579078065016065, 19.801555051780429}},
    {"color188", ColorTypeXTerm, -1, 7, 188, {215, 215, 215}, {85.983568695953934, -0.000014652810687, 0.000005861124275}},
    {"color189", ColorTypeXTerm, -1, 7, 189, {215, 215, 255}, {87.127727522600125, 7.813054693670940, -19.437771212759714}},
    {"color190", ColorTypeXTerm, -1, 11, 190, {215, 255, 0}, {94.298344804313956, -37.668199870568188, 91.102418362869514}},
    {"color191", ColorTypeXTerm, -1, 11, 191, {215, 255, 95}, {94.650453001818860, -34.512372759927466, 69.782913728341200}},
    {"color192", ColorTypeXTerm, -1, 11, 192, {215, 255, 135}, {95.041191934707342, -31.089516851639409, 52.825518610913534}},
    {"color193", ColorTypeXTerm, -1, 10, 193, {215, 255, 175}, {95.606040087542837, -26.280220313504021, 34.142079395991921}},
    {"color194", ColorTypeXTerm, -1, 15, 194, {215, 255, 215}, {96.357250690502184, -20.119938570623439, 14.910613277872686}},
    {"color195", ColorTypeXTerm, -1, 15, 195, {215, 255, 255}, {97.302527760635385, -12.715983697448973, -4.270743684845124}},
    {"color196", ColorTypeXTerm, -1, 9, 196, {255, 0, 0}, {53.240794141307191, 80.092459596411146, 67.203196515852966}},
    {"color197", ColorTypeXTerm, -1, 9, 197, {255, 0, 95}, {54.125780748621750, 82.492191953193682, 22.910970114592100}},
    {"color198", ColorTypeXTerm, -1, 13, 198, {255, 0, 135}, {55.088767196464602, 85.054618226786118, 0.168144358656286}},
    {"color199", ColorTypeXTerm, -1, 13, 199, {255, 0, 175}, {56.447797595789112, 88.591016529755478, -21.450672025111615}},
    {"color200", ColorTypeXTerm, -1, 13, 200, {255, 0, 215}, {58.199846302615782, 93.025112399586277, -41.765997605847339}},
    {"color201", ColorTypeXTerm, -1, 13, 201, {255, 0, 255}, {60.324212128368742, 98.234311888004015, -60.824892208850059}},
    {"color202", ColorTypeXTerm, -1, 9, 202, {255, 95, 0}, {61.177752779237935, 58.007183531847637, 70.725237029063976}},
    {"color203", ColorTypeXTerm, -1, 9, 203, {255, 95, 95}, {61.892577053702368, 60.769075584414963, 32.940064175764292}},
    {"color204", ColorTypeXTerm, -1, 13, 204, {255, 95, 135}, {62.675958326785874, 63.722866728296857, 11.059156661480408}},
    {"color205", ColorTypeXTerm, -1, 13, 205, {255, 95, 175}, {63.790978568516692, 67.805179862458488, -10.333123749894391}},
    {"color206", ColorTypeXTerm, -1, 13, 206, {255, 95, 215}, {65.243976249225426, 72.929280656207126, -30.773134452993499}},
    {"color207", ColorTypeXTerm, -1, 13, 207, {255, 95, 255}, {67.027699749129525, 78.950490766589780, -50.165199089203604}},
    {"color208", ColorTypeXTerm, -1, 9, 208, {255, 135, 0}, {68.456201581224008, 39.347025379941726, 74.858462169491361}},
    {"color209", ColorTypeXTerm, -1, 9, 209, {255, 135, 95}, {69.054425981591265, 42.256401282508463, 41.778309973727268}},
    {"color210", ColorTypeXTerm, -1, 9, 210, {255, 135, 135}, {69.712953317704745, 45.379691148463074, 20.832601190623912}},
    {"color211", ColorTypeXTerm, -1, 13, 211, {255, 135, 175}, {70.655380814616620, 49.714783803172899, -0.184656618817103}},
    {"color212", ColorTypeXTerm, -1, 13, 212, {255, 135, 215}, {71.892132240065550, 55.183573337904079, -20.579895143635916}},
    {"color213", ColorTypeXTerm, -1, 13, 213, {255, 135, 255}, {73.423103641258905, 61.643523496324903, -40.132155958710889}},
    {"color214", ColorTypeXTerm, -1, 11, 214, {255, 175, 0}, {77.236080298550888, 18.715562917995587, 80.467682703251697}},
    {"color215", ColorTypeXTerm, -1, 7, 215, {255, 175, 95}, {77.727829006395311, 21.651858507549250, 52.000981476579923}},
    {"color216", ColorTypeXTerm, -1, 7, 216, {255, 175, 135}, {78.271171283587364, 24.819797233512851, 32.297655390434585}},
    {"color217", ColorTypeXTerm, -1, 7, 217, {255, 175, 175}, {79.052359332912246, 29.242702568118894, 11.899654015462445}},
    {"color218", ColorTypeXTerm, -1, 7, 218, {255, 175, 215}, {80.083759686012698, 34.862653558821123, -8.273975435095537}},
    {"color219", ColorTypeXTerm, -1, 13, 219, {255, 175, 255}, {81.369962333844086, 41.554729689551372, -27.861346693015744}},
    {"color220", ColorTypeXTerm, -1, 11, 220, {255, 215, 0}, {86.930569648725850, -1.923748704578399, 87.132036448965508}},
    {"color221", ColorTypeXTerm, -1, 11, 221, {255, 215, 95}, {87.334593926876991, 0.925620878367783, 62.778901529467859}},
    {"color222", ColorTypeXTerm, -1, 11, 222, {255, 215, 135}, {87.782260049090453, 4.015632266441138, 44.514667656673957}},
    {"color223", ColorTypeXTerm, -1, 15, 223, {255, 215, 175}, {88.428153614677782, 8.356442798800213, 24.958588893196975}},
    {"color224", ColorTypeXTerm, -1, 15, 224, {255, 215, 215}, {89.284922293488634, 13.915222051983701, 5.202570067212009}},
    {"color225", ColorTypeXTerm, -1, 15, 225, {255, 215, 255}, {90.359536166122794, 20.594186876914613, -14.254941580049806}},
    {"color226", ColorTypeXTerm, -1, 11, 226, {255, 255, 0}, {97.139267224306309, -21.553748216377233, 94.477975053670306}},
    {"color227", ColorTypeXTerm, -1, 11, 227, {255, 255, 95}, {97.473992753451952, -18.866927358040574, 73.623331931673718}},
    {"color228", ColorTypeXTerm, -1, 11, 228, {255, 255, 135}, {97.845623443996629, -15.939406762307373, 56.875583540546650}},
    {"color229", ColorTypeXTerm, -1, 11, 229, {255, 255, 175}, {98.383181690923635, -11.803188948198207, 38.326889201344393}},
    {"color230", ColorTypeXTerm, -1, 15, 230, {255, 255, 215}, {99.098696815160451, -6.467218863337343, 19.163906109500072}},
    {"color231", ColorTypeXTerm, -1, 15, 231, {255, 255, 255}, {100.000003866666546, -0.000016666666158, 0.000006666666463}},
    {"color232", ColorTypeXTerm, -1, 0, 232, {8, 8, 8}, {2.193388187529170, -0.000000945425849, 0.000000378170339}},
    {"color233", ColorTypeXTerm, -1, 0, 233, {18, 18, 18}, {5.463863025268839, -0.000002355113132, 0.000000942045253}},
    {"color234", ColorTypeXTerm, -1, 0, 234, {28, 28, 28}, {10.268185186836224, -0.000003774164273, 0.000001509665709}},
    {"color235", ColorTypeXTerm, -1, 0, 235, {38, 38, 38}, {15.159721168846239, -0.000004476971127, 0.000001790788440}},
    {"color236", ColorTypeXTerm, -1, 0, 236, {48, 48, 48}, {19.865534710049907, -0.000005153093674, 0.000002061237470}},
    {"color237", ColorTypeXTerm, -1, 0, 237, {58, 58, 58}, {24.421321253235426, -0.000005807660713, 0.000002323064285}},
    {"color238", ColorTypeXTerm, -1, 0, 238, {68, 68, 68}, {28.851903893463358, -0.000006444238670, 0.000002577695468}},
    {"color239", ColorTypeXTerm, -1, 8, 239, {78, 78, 78}, {33.175473749682190, -0.000007065441121, 0.000002826176448}},
    {"color240", ColorTypeXTerm, -1, 8, 240, {88, 88, 88}, {37.405892151988574, -0.000007673259861, 0.000003069303944}},
    {"color241", ColorTypeXTerm, -1, 8, 241, {98, 98, 98}, {41.554045224644327, -0.000008269258833, 0.000003307703533}},
    {"color242", ColorTypeXTerm, -1, 8, 242, {108, 108, 108}, {45.628691190834779, -0.000008854696476, 0.000003541878590}},
    {"color243", ColorTypeXTerm, -1, 8, 243, {118, 118, 118}, {49.637016560651290, -0.000009430605186, 0.000003772242074}},
    {"color244", ColorTypeXTerm, -1, 8, 244, {128, 128, 128}, {53.585015771669404, -0.000009997846440, 0.000003999138576}},
    {"color245", ColorTypeXTerm, -1, 8, 245, {138, 138, 138}, {57.477758837492289, -0.000010557148600, 0.000004222859440}},
    {"color246", ColorTypeXTerm, -1, 8, 246, {148, 148, 148}, {61.319585247464190, -0.000011109135056, 0.000004443654000}},
    {"color247", ColorTypeXTerm, -1, 7, 247, {158, 158, 158}, {65.114247741275165, -0.000011654345267, 0.000004661738107}},
    {"color248", ColorTypeXTerm, -1, 7, 248, {168, 168, 168}, {68.865021078911923, -0.000012193249466, 0.000004877299786}},
    {"color249", ColorTypeXTerm, -1, 7, 249, {178, 178, 178}, {72.574785783994670, -0.000012726261545, 0.000005090504618}},
    {"color250", ColorTypeXTerm, -1, 7, 250, {188, 188, 188}, {76.246093622283993, -0.000013253748377, 0.000005301499328}},
    {"color251", ColorTypeXTerm, -1, 7, 251, {198, 198, 198}, {79.881219505720821, -0.000013776036478, 0.000005510414591}},
    {"color252", ColorTypeXTerm, -1, 7, 252, {208, 208, 208}, {83.482203143391388, -0.000014293419059, 0.000005717367646}},
    {"color253", ColorTypeXTerm, -1, 7, 253, {218, 218, 218}, {87.050882835113157, -0.000014806160464, 0.000005922464186}},
    {"color254", ColorTypeXTerm, -1, 15, 254, {228, 228, 228}, {90.588923164444353, -0.000015314499446, 0.000006125799779}},
    {"color255", ColorTypeXTerm, -1, 15, 255, {238, 238, 238}, {94.097837898778110, -0.000015818653942, 0.000006327461533}},
    {"aliceblue", ColorTypeTrueColor, -1, 15, 231, {240, 248, 255}, {97.178649823061065, -1.348615859834423, -4.262854157273566}},
    {"antiquewhite", ColorTypeTrueColor, -1, 15, 255, {250, 235, 215}, {93.731332239389900, 1.838676986194332, 11.526165646584307}},
    {"aqua", ColorTypeTrueColor, -1, 14, 51, {0, 255, 255}, {91.113219812758601, -48.087528058758210, -14.131186091754454}},
    {"aquamarine", ColorTypeTrueColor, -1, 14, 122, {127, 255, 212}, {92.033978846348973, -45.524537815673561, 9.718128684127180}},
    {"azure", ColorTypeTrueColor, -1, 15, 231, {240, 255, 255}, {98.932415212394432, -4.880395251172509, -1.688275319531085}},
    {"beige", ColorTypeTrueColor, -1, 15, 230, {245, 245, 220}, {95.949088562669871, -4.192868939387306, 12.048995703858001}},
    {"bisque", ColorTypeTrueColor, -1, 15, 223, {255, 228, 196}, {92.013430898297855, 4.430873057462814, 19.012007146413133}},
    {"blanchedalmond", ColorTypeTrueColor, -1, 15, 223, {255, 235, 205}, {93.920261670901752, 2.130162565633753, 17.026145901390798}},
    {"blue", ColorTypeTrueColor, -1, 12, 21, {0, 0, 255}, {32.297010932850725, 79.187519845122182, -107.860161754148095}},
    {"blueviolet", ColorTypeTrueColor, -1, 12, 93, {138, 43, 226}, {42.187852724767055, 69.844799873801875, -74.763374222887563}},
    {"brown", ColorTypeTrueColor, -1, 1, 124, {165, 42, 42}, {37.526505242810693, 49.690346440810970, 30.543166542619637}},
    {"burlywood", ColorTypeTrueColor, -1, 7, 180, {222, 184, 135}, {77.018358910682210, 7.049925060326423, 30.018853082835605}},
    {"cadetblue", ColorTypeTrueColor, -1, 6, 73, {95, 158, 160}, {61.153147911545659, -19.679443840229681, -7.420779647830189}},
    {"chartreuse", ColorTypeTrueColor, -1, 10, 118, {127, 255, 0}, {89.872707939377449, -68.066128898354336, 85.779993123946824}},
    {"chocolate", ColorTypeTrueColor, -1, 9, 166, {210, 105, 30}, {55.990059499855889, 37.052651262226235, 56.740709528042679}},
    {"coral", ColorTypeTrueColor, -1, 9, 209, {255, 127, 80}, {67.295036831459228, 45.354290044060221, 47.493372815457001}},
    {"cornflowerblue", ColorTypeTrueColor, -1, 8, 69, {100, 149, 237}, {61.925937826475348, 9.332998515857671, -49.298105090170210}},
    {"cornsilk", ColorTypeTrueColor, -1, 15, 230, {255, 248, 220}, {97.455675951558547, -2.217672790183089, 14.293524985209793}},
    {"crimson", ColorTypeTrueColor, -1, 9, 197, {220, 20, 60}, {47.036445733718395, 70.921109900138504, 33.599672209471443}},
    {"cyan", ColorTypeTrueColor, -1, 14, 51, {0, 255, 255}, {91.113219812758601, -48.087528058758210, -14.131186091754454}},
    {"darkblue", ColorTypeTrueColor, -1, 4, 18, {0, 0, 139}, {14.753606410438852, 50.423447971171598, -68.681040459526983}},
    {"darkcyan", ColorTypeTrueColor, -1, 6, 30, {0, 139, 139}, {52.205417682190344, -30.620216088033004, -8.998174561624616}},
    {"darkgoldenrod", ColorTypeTrueColor, -1, 3, 136, {184, 134, 11}, {59.220700501110144, 9.864750526224153, 62.730459155923654}},
    {"darkgray", ColorTypeTrueColor, -1, 7, 248, {169, 169, 169}, {69.237798446836749, -0.000012246809400, 0.000004898723760}},
    {"darkgreen", ColorTypeTrueColor, -1, 2, 22, {0, 100, 0}, {36.202355701209150, -43.369671367899961, 41.858274427141183}},
    {"darkgrey", ColorTypeTrueColor, -1, 7, 248, {169, 169, 169}, {69.237798446836749, -0.000012246809400, 0.000004898723760}},
    {"darkkhaki", ColorTypeTrueColor, -1, 3, 143, {189, 183, 107}, {73.381980848063790, -8.787701661144954, 39.291672478552030}},
    {"darkmagenta", ColorTypeTrueColor, -1, 5, 90, {139, 0, 139}, {32.600208046956858, 62.551683954194722, -38.730860540191451}},
    {"darkolivegreen", ColorTypeTrueColor, -1, 2, 58, {85, 107, 47}, {42.233854170808776, -18.827827708622014, 30.598372896605099}},
    {"darkorange", ColorTypeTrueColor, -1, 9, 208, {255, 140, 0}, {69.485342176783078, 36.825741213627197, 75.487098537200936}},
    {"darkorchid", ColorTypeTrueColor, -1, 5, 128, {153, 50, 204}, {43.380241127805832, 65.153533027537790, -60.097712889134947}},
    {"darkred", ColorTypeTrueColor, -1, 1, 88, {139, 0, 0}, {28.089770555957962, 50.999677439595523, 41.290823945136765}},
    {"darksalmon", ColorTypeTrueColor, -1, 9, 209, {233, 150, 122}, {69.856285074833963, 28.174230126963849, 27.711709604142531}},
    {"darkseagreen", ColorTypeTrueColor, -1, 7, 108, {143, 188, 143}, {72.086676700934561, -23.819555602939801, 18.037752472502788}},
    {"darkslateblue", ColorTypeTrueColor, -1, 4, 61, {72, 61, 139}, {30.828347417822897, 26.050974227434189, -42.082532834088980}},
    {"darkslategray", ColorTypeTrueColor, -1, 6, 23, {47, 79, 79}, {31.255234910204962, -11.719854659342083, -3.723639950456592}},
    {"darkslategrey", ColorTypeTrueColor, -1, 6, 23, {47, 79, 79}, {31.255234910204962, -11.719854659342083, -3.723639950456592}},
    {"darkturquoise", ColorTypeTrueColor, -1, 14, 44, {0, 206, 209}, {75.290238362679403, -40.043272413653575, -13.513332720755834}},
    {"darkviolet", ColorTypeTrueColor, -1, 5, 92, {148, 0, 211}, {39.579760710466012, 76.321974002693167, -70.366364223494685}},
    {"deeppink", ColorTypeTrueColor, -1, 13, 198, {255, 20, 147}, {55.960839307671037, 84.538687164380676, -5.700009514268478}},
    {"deepskyblue", ColorTypeTrueColor, -1, 7, 39, {0, 191, 255}, {72.545920770516844, -17.658557723658653, -42.541170032401901}},
    {"dimgray", ColorTypeTrueColor, -1, 8, 242, {105, 105, 105}, {44.413562161601270, -0.000008680108965, 0.000003472043586}},
    {"dimgrey", ColorTypeTrueColor, -1, 8, 242, {105, 105, 105}, {44.413562161601270, -0.000008680108965, 0.000003472043586}},
    {"dodgerblue", ColorTypeTrueColor, -1, 8, 33, {30, 144, 255}, {59.378302464398672, 9.957589279274870, -63.387841049889573}},
    {"firebrick", ColorTypeTrueColor, -1, 1, 124, {178, 34, 34}, {39.117932238316428, 55.916771623952030, 37.649050983867873}},
    {"floralwhite", ColorTypeTrueColor, -1, 15, 231, {255, 250, 240}, {98.401648010495506, -0.036540423694109, 5.376192798848645}},
    {"forestgreen", ColorTypeTrueColor, -1, 2, 28, {34, 139, 34}, {50.593073105561558, -49.585382632805683, 45.015964451702942}},
    {"fuchsia", ColorTypeTrueColor, -1, 13, 201, {255, 0, 255}, {60.324212128368742, 98.234311888004015, -60.824892208850059}},
    {"gainsboro", ColorTypeTrueColor, -1, 7, 253, {220, 220, 220}, {87.760891568747311, -0.000014908173085, 0.000005963269212}},
    {"ghostwhite", ColorTypeTrueColor, -1, 15, 231, {248, 248, 255}, {97.757215645889971, 1.247116402219639, -3.345466101118277}},
    {"gold", ColorTypeTrueColor, -1, 11, 220, {255, 215, 0}, {86.930569648725850, -1.923748704578399, 87.132036448965508}},
    {"goldenrod", ColorTypeTrueColor, -1, 3, 178, {218, 165, 32}, {70.817974904535888, 8.524095050159664, 68.761861698722697}},
    {"gray", ColorTypeTrueColor, -1, 8, 244, {128, 128, 128}, {53.585015771669404, -0.000009997846440, 0.000003999138576}},
    {"green", ColorTypeTrueColor, -1, 2, 28, {0, 128, 0}, {46.227431468762596, -51.698495529891062, 49.896846001056097}},
    {"greenyellow", ColorTypeTrueColor, -1, 10, 154, {173, 255, 47}, {91.956826147119727, -52.480846861164110, 81.864480969513181}},
    {"grey", ColorTypeTrueColor, -1, 8, 244, {128, 128, 128}, {53.585015771669404, -0.000009997846440, 0.000003999138576}},
    {"honeydew", ColorTypeTrueColor, -1, 15, 195, {240, 255, 240}, {98.565561091148737, -7.564939131992188, 5.475317075314479}},
    {"hotpink", ColorTypeTrueColor, -1, 13, 205, {255, 105, 180}, {65.486158932577396, 64.238456641895596, -10.646352690102390}},
    {"indianred", ColorTypeTrueColor, -1, 9, 167, {205, 92, 92}, {53.395115393686041, 44.828284270314377, 22.117128186598112}},
    {"indigo", ColorTypeTrueColor, -1, 4, 54, {75, 0, 130}, {20.469442937165006, 51.685573451477204, -53.312623117694550}},
    {"ivory", ColorTypeTrueColor, -1, 15, 231, {255, 255, 240}, {99.639902822762735, -2.551393440697103, 7.162635096575398}},
    {"khaki", ColorTypeTrueColor, -1, 11, 186, {240, 230, 140}, {90.328176777815528, -9.009831825025072, 44.979271409297937}},
    {"lavender", ColorTypeTrueColor, -1, 15, 189, {230, 230, 250}, {91.827509908817220, 3.707838882965164, -9.661308832101746}},
    {"lavenderblush", ColorTypeTrueColor, -1, 15, 255, {255, 240, 245}, {96.068728306205571, 5.887335539538352, -0.593691092376880}},
    {"lawngreen", ColorTypeTrueColor, -1, 10, 118, {124, 252, 0}, {88.876481661056161, -67.856068770300425, 84.952479516624237}},
    {"lemonchiffon", ColorTypeTrueColor, -1, 15, 230, {255, 250, 205}, {97.648179448239986, -5.426768564686046, 22.233845208771985}},
    {"lightblue", ColorTypeTrueColor, -1, 7, 152, {173, 216, 230}, {83.812946201553416, -10.891784263162785, -11.476672117761355}},
    {"lightcoral", ColorTypeTrueColor, -1, 9, 210, {240, 128, 128}, {66.156847572842508, 42.809917674793866, 19.556811908356764}},
    {"lightcyan", ColorTypeTrueColor, -1, 15, 195, {224, 255, 255}, {97.867406794927803, -9.944510013456808, -3.375046117626201}},
    {"lightgoldenrodyellow", ColorTypeTrueColor, -1, 15, 230, {250, 250, 210}, {97.369116442225945, -6.481069629579395, 19.237243925687242}},
    {"lightgray", ColorTypeTrueColor, -1, 7, 252, {211, 211, 211}, {84.556120088230941, -0.000014447717411, 0.000005779086965}},
    {"lightgreen", ColorTypeTrueColor, -1, 10, 120, {144, 238, 144}, {86.548214852312199, -46.327954809357699, 36.949101159339094}},
    {"lightgrey", ColorTypeTrueColor, -1, 7, 252, {211, 211, 211}, {84.556120088230941, -0.000014447717411, 0.000005779086965}},
    {"lightpink", ColorTypeTrueColor, -1, 7, 217, {255, 182, 193}, {81.054591201641784, 27.962641144971933, 5.035951856911725}},
    {"lightsalmon", ColorTypeTrueColor, -1, 9, 216, {255, 160, 122}, {74.706118331198709, 31.477523633087976, 34.548660195899217}},
    {"lightseagreen", ColorTypeTrueColor, -1, 6, 37, {32, 178, 170}, {65.785332510484650, -37.513947621927279, -6.330951041241817}},
    {"lightskyblue", ColorTypeTrueColor, -1, 7, 117, {135, 206, 250}, {79.723003397653258, -10.831125840394584, -28.501786742100666}},
    {"lightslategray", ColorTypeTrueColor, -1, 8, 67, {119, 136, 153}, {55.916717227912727, -2.247686661817627, -11.107967380453054}},
    {"lightslategrey", ColorTypeTrueColor, -1, 8, 67, {119, 136, 153}, {55.916717227912727, -2.247686661817627, -11.107967380453054}},
    {"lightsteelblue", ColorTypeTrueColor, -1, 7, 153, {176, 196, 222}, {78.451579369681340, -1.281583913411988, -15.210996213841522}},
    {"lightyellow", ColorTypeTrueColor, -1, 15, 230, {255, 255, 224}, {99.285089463351383, -5.107293032951821, 14.837756269209867}},
    {"lime", ColorTypeTrueColor, -1, 10, 46, {0, 255, 0}, {87.734722352797917, -86.182716420534661, 83.179320502697834}},
    {"limegreen", ColorTypeTrueColor, -1, 10, 40, {50, 205, 50}, {72.606708433466181, -67.125547400551540, 61.437221754628332}},
    {"linen", ColorTypeTrueColor, -1, 15, 255, {250, 240, 230}, {95.311547684121379, 1.677444618621626, 6.022119660989844}},
    {"magenta", ColorTypeTrueColor, -1, 13, 201, {255, 0, 255}, {60.324212128368742, 98.234311888004015, -60.824892208850059}},
    {"maroon", ColorTypeTrueColor, -1, 1, 88, {128, 0, 0}, {25.535530963463174, 48.045128262358347, 38.057349239387428}},
    {"mediumaquamarine", ColorTypeTrueColor, -1, 14, 79, {102, 205, 170}, {75.691300986247342, -38.335641262158603, 8.307990947762489}},
    {"mediumblue", ColorTypeTrueColor, -1, 12, 20, {0, 0, 205}, {24.971427211092923, 67.176532102002284, -91.500171147878248}},
    {"mediumorchid", ColorTypeTrueColor, -1, 13, 134, {186, 85, 211}, {53.643760287459372, 59.060405029490560, -47.402328847545206}},
    {"mediumpurple", ColorTypeTrueColor, -1, 13, 98, {147, 112, 219}, {54.974803691373609, 36.797759304072365, -50.089466726524989}},
    {"mediumseagreen", ColorTypeTrueColor, -1, 2, 35, {60, 179, 113}, {65.271646984278533, -48.218190113850149, 24.290181695446766}},
    {"mediumslateblue", ColorTypeTrueColor, -1, 12, 99, {123, 104, 238}, {52.155986761267087, 41.068386209184347, -65.396190572291644}},
    {"mediumspringgreen", ColorTypeTrueColor, -1, 10, 49, {0, 250, 154}, {87.338528044272053, -70.686467909843401, 32.462836505571225}},
    {"mediumturquoise", ColorTypeTrueColor, -1, 14, 44, {72, 209, 204}, {76.881005052836286, -37.360186973096233, -8.354797318364815}},
    {"mediumvioletred", ColorTypeTrueColor, -1, 5, 162, {199, 21, 133}, {44.766615655642887, 70.992114916824164, -15.169223813968969}},
    {"midnightblue", ColorTypeTrueColor, -1, 4, 18, {25, 25, 112}, {15.857600599624735, 31.713343200450382, -49.574634483539583}},
    {"mintcream", ColorTypeTrueColor, -1, 15, 231, {245, 255, 250}, {99.156395175215252, -4.162938124707694, 1.246381480557357}},
    {"mistyrose", ColorTypeTrueColor, -1, 15, 224, {255, 228, 225}, {92.656337860685639, 8.747082134450689, 4.835717904967218}},
    {"moccasin", ColorTypeTrueColor, -1, 15, 223, {255, 228, 181}, {91.723177447460216, 2.439346935868447, 26.359832514614844}},
    {"navajowhite", ColorTypeTrueColor, -1, 15, 223, {255, 222, 173}, {90.101352066161866, 4.510130944058998, 28.272188134629150}},
    {"navy", ColorTypeTrueColor, -1, 4, 18, {0, 0, 128}, {12.971966857430804, 47.502281324713167, -64.702162125995883}},
    {"oldlace", ColorTypeTrueColor, -1, 15, 255, {253, 245, 230}, {96.780005715148562, 0.170955773497128, 8.166223847295306}},
    {"olive", ColorTypeTrueColor, -1, 3, 100, {128, 128, 0}, {51.868943377343967, -12.929464306735028, 56.674579008994250}},
    {"olivedrab", ColorTypeTrueColor, -1, 3, 64, {107, 142, 35}, {54.650499657738507, -28.221777083195455, 49.690724638504676}},
    {"orange", ColorTypeTrueColor, -1, 7, 214, {255, 165, 0}, {74.935650173060296, 23.933170767745093, 78.949775403418016}},
    {"orangered", ColorTypeTrueColor, -1, 9, 202, {255, 69, 0}, {57.581726990370342, 67.782743246801672, 68.958612652419774}},
    {"orchid", ColorTypeTrueColor, -1, 13, 170, {218, 112, 214}, {62.803212568914503, 55.282360871189816, -34.404443928286163}},
    {"palegoldenrod", ColorTypeTrueColor, -1, 11, 229, {238, 232, 170}, {91.141010833491663, -7.349102825780607, 30.971337773934039}},
    {"palegreen", ColorTypeTrueColor, -1, 10, 120, {152, 251, 152}, {90.749618473307933, -48.296798886442502, 38.527726143293364}},
    {"paleturquoise", ColorTypeTrueColor, -1, 14, 159, {175, 238, 238}, {90.059990595959405, -19.638379938145935, -6.399936733685485}},
    {"palevioletred", ColorTypeTrueColor, -1, 13, 168, {219, 112, 147}, {60.568036293191952, 45.519057328531233, 0.402260886988026}},
    {"papayawhip", ColorTypeTrueColor, -1, 15, 223, {255, 239, 213}, {95.076073938173323, 1.270732069875935, 14.525435575274814}},
    {"peachpuff", ColorTypeTrueColor, -1, 15, 223, {255, 218, 185}, {89.350030743180824, 8.085208765076068, 21.022465635993747}},
    {"peru", ColorTypeTrueColor, -1, 9, 172, {205, 133, 63}, {61.754422093925982, 21.395538148832717, 47.918328707637045}},
    {"pink", ColorTypeTrueColor, -1, 7, 217, {255, 192, 203}, {83.586518296094468, 24.143630849775167, 3.325893790885814}},
    {"plum", ColorTypeTrueColor, -1, 13, 182, {221, 160, 221}, {73.373904296952389, 32.530876959829754, -21.985652073908991}},
    {"powderblue", ColorTypeTrueColor, -1, 14, 152, {176, 224, 230}, {86.132405871991438, -14.092919363305977, -8.007606810320000}},
    {"purple", ColorTypeTrueColor, -1, 5, 90, {128, 0, 128}, {29.784666617920195, 58.927895811994119, -36.487077091203105}},
    {"rebeccapurple", ColorTypeTrueColor, -1, 5, 91, {102, 51, 153}, {32.902467667375610, 42.883074460311313, -47.148633770801098}},
    {"red", ColorTypeTrueColor, -1, 9, 196, {255, 0, 0}, {53.240794141307191, 80.092459596411146, 67.203196515852966}},
    {"rosybrown", ColorTypeTrueColor, -1, 8, 138, {188, 143, 143}, {63.607406337026092, 17.012669269312720, 6.609691877882717}},
    {"royalblue", ColorTypeTrueColor, -1, 12, 27, {65, 105, 225}, {47.830073605628023, 26.263097389935432, -65.263664927905367}},
    {"saddlebrown", ColorTypeTrueColor, -1, 1, 130, {139, 69, 19}, {37.469798326367538, 26.442584497776700, 40.983818845124652}},
    {"salmon", ColorTypeTrueColor, -1, 9, 210, {250, 128, 114}, {67.264092840420275, 45.226468609022739, 29.094269715625142}},
    {"sandybrown", ColorTypeTrueColor, -1, 7, 215, {244, 164, 96}, {73.954452317677934, 23.026975825974127, 46.791245442927234}},
    {"seagreen", ColorTypeTrueColor, -1, 2, 29, {46, 139, 87}, {51.533898679419877, -39.715339936871096, 20.052184958342313}},
    {"seashell", ColorTypeTrueColor, -1, 15, 231, {255, 245, 238}, {97.121436789443194, 2.162200636999512, 4.554110971324410}},
    {"sienna", ColorTypeTrueColor, -1, 1, 130, {160, 82, 45}, {43.799186138581248, 29.322324254717980, 35.638442812588821}},
    {"silver", ColorTypeTrueColor, -1, 7, 250, {192, 192, 192}, {77.704366713431412, -0.000013463270276, 0.000005385308111}},
    {"skyblue", ColorTypeTrueColor, -1, 7, 117, {135, 206, 235}, {79.207102837488520, -14.838968916051076, -21.276506647850567}},
    {"slateblue", ColorTypeTrueColor, -1, 12, 62, {106, 90, 205}, {45.335972350338395, 36.039454056955357, -57.771923490931741}},
    {"slategray", ColorTypeTrueColor, -1, 8, 67, {112, 128, 144}, {52.835656391023662, -2.142798907216192, -10.570981702672455}},
    {"slategrey", ColorTypeTrueColor, -1, 8, 67, {112, 128, 144}, {52.835656391023662, -2.142798907216192, -10.570981702672455}},
    {"snow", ColorTypeTrueColor, -1, 15, 231, {255, 250, 250}, {98.643894788561511, 1.656743701996644, 0.587466215100552}},
    {"springgreen", ColorTypeTrueColor, -1, 10, 48, {0, 255, 127}, {88.470123576092490, -76.901745444947764, 47.027782764763181}},
    {"steelblue", ColorTypeTrueColor, -1, 8, 67, {70, 130, 180}, {52.465517187685748, -4.077471012357281, -32.191861229813455}},
    {"tan", ColorTypeTrueColor, -1, 7, 180, {210, 180, 140}, {74.975716337265368, 5.021257635206222, 24.428135697283949}},
    {"teal", ColorTypeTrueColor, -1, 6, 30, {0, 128, 128}, {48.254093461861586, -28.846304196984779, -8.476885985257221}},
    {"thistle", ColorTypeTrueColor, -1, 7, 182, {216, 191, 216}, {80.077794990775843, 13.217587590567014, -9.228882166481188}},
    {"tomato", ColorTypeTrueColor, -1, 9, 203, {255, 99, 71}, {62.206929262837946, 57.851264102126819, 46.419810975648716}},
    {"turquoise", ColorTypeTrueColor, -1, 14, 44, {64, 224, 208}, {81.264433383990834, -44.081882135401592, -4.028385738801887}},
    {"violet", ColorTypeTrueColor, -1, 13, 213, {238, 130, 238}, {69.695768500699970, 56.356649735983424, -36.809864933124324}},
    {"wheat", ColorTypeTrueColor, -1, 15, 223, {245, 222, 179}, {89.351636346143806, 1.511524479513304, 24.007857146563772}},
    {"white", ColorTypeTrueColor, -1, 15, 231, {255, 255, 255}, {100.000003866666546, -0.000016666666158, 0.000006666666463}},
    {"whitesmoke", ColorTypeTrueColor, -1, 15, 255, {245, 245, 245}, {96.537493365485673, -0.000016169179051, 0.000006467671620}},
    {"yellow", ColorTypeTrueColor, -1, 11, 226, {255, 255, 0}, {97.139267224306309, -21.553748216377233, 94.477975053670306}},
    {"yellowgreen", ColorTypeTrueColor, -1, 10, 112, {154, 205, 50}, {76.534808212057499, -37.987912969071225, 66.585626206666078}},
    {NULL, ColorTypeNone, -1, -1, -1, {0, 0, 0}, {0.0, 0.0, 0.0}} // End marker
};

/*
 * ---------------------------------------------------------------------------
 * ANSI → MUSH conversion helpers
 */

static char mushcode_for_index(int ansi_index)
{
    if (ansi_index < 0)
        return '\0';

    for (size_t i = 0; colorDefinitions[i].name != NULL; i++)
    {
        if (colorDefinitions[i].type == ColorTypeAnsi && colorDefinitions[i].ansi_index == ansi_index)
        {
            if (colorDefinitions[i].mush_code > 0 && colorDefinitions[i].mush_code < 256)
            {
                return (char)colorDefinitions[i].mush_code;
            }
        }
    }

    return '\0';
}

static void append_mush_from_state(const ColorState *state, char *buff, char **bp)
{
    /* Always start from a known baseline. */
    XSAFELBSTR("%xn", buff, bp);

    if (state->highlight == ColorStatusSet)
    {
        XSAFELBSTR("%xh", buff, bp);
    }

    if (state->underline == ColorStatusSet)
    {
        XSAFELBSTR("%xu", buff, bp);
    }

    if (state->flash == ColorStatusSet)
    {
        XSAFELBSTR("%xf", buff, bp);
    }

    if (state->inverse == ColorStatusSet)
    {
        XSAFELBSTR("%xi", buff, bp);
    }

    if (state->foreground.is_set == ColorStatusSet)
    {
        char mush = mushcode_for_index(state->foreground.ansi_index);
        if (mush)
        {
            char seq[4] = {'%', 'x', mush, '\0'};
            XSAFELBSTR(seq, buff, bp);
        }
    }

    if (state->background.is_set == ColorStatusSet)
    {
        char mush = mushcode_for_index(state->background.ansi_index);
        if (mush)
        {
            char seq[4] = {'%', 'x', (char)toupper(mush), '\0'};
            XSAFELBSTR(seq, buff, bp);
        }
    }
}

static bool parse_and_apply_ansi_sequence(const char **ptr, ColorState *state)
{
    if (!ptr || !*ptr || **ptr != ESC_CHAR)
        return false;

    const char *p = *ptr;

    /* Expect ESC [ ... m */
    if (*p != ESC_CHAR || *(p + 1) != '[')
        return false;

    p += 2; /* Skip ESC [ */
    const char *code_start = p;
    while (*p && *p != 'm')
    {
        p++;
    }

    if (*p != 'm')
    {
        return false; /* Unterminated sequence, do not advance */
    }

    size_t code_len = (size_t)(p - code_start);
    char *code = XMALLOC(code_len + 1, "ansi_code");
    XMEMCPY(code, code_start, code_len);
    code[code_len] = '\0';

    ansi_parse_ansi_code(state, code);

    XFREE(code);
    p++;          /* Skip trailing 'm' */
    *ptr = p;     /* Advance caller pointer */
    return true;
}

bool ansi_apply_sequence(const char **ptr, ColorState *state)
{
    if (!ptr || !*ptr || !state)
    {
        return false;
    }

    ColorState next = *state;
    const char *cursor = *ptr;

    if (!parse_and_apply_ansi_sequence(&cursor, &next))
    {
        return false;
    }

    *state = next;
    *ptr = cursor;
    return true;
}

/**
 * @brief Convert ANSI escape sequences to mushcode or strip ANSI codes using the new ANSI API.
 *
 * This mirrors translate_string() but relies solely on ansi.c helpers.
 *
 * @param str Input string (not modified)
 * @param type When 1 converts to mushcode, when 0 strips ANSI
 * @return Newly allocated string; caller frees with XFREE()
 */
char *translate_string_ansi(const char *str, int type)
{
    char *buff, *bp;

    bp = buff = XMALLOC(LBUF_SIZE, "buff");

    if (!str)
    {
        *bp = '\0';
        return buff;
    }

    if (type)
    {
        ColorState current = {0};
        const char *p = str;

        while (*p)
        {
            if (*p == ESC_CHAR && *(p + 1) == '[')
            {
                const char *cursor = p;
                ColorState next = current;

                if (parse_and_apply_ansi_sequence(&cursor, &next))
                {
                    if (memcmp(&next, &current, sizeof(ColorState)) != 0)
                    {
                        append_mush_from_state(&next, buff, &bp);
                        current = next;
                    }

                    p = cursor;
                    continue;
                }
            }

            switch (*p)
            {
            case ' ':
                if (*(p + 1) == ' ')
                {
                    XSAFESTRNCAT(buff, &bp, "%b", 2, LBUF_SIZE);
                }
                else
                {
                    XSAFELBCHR(' ', buff, &bp);
                }
                break;

            case '\\':
            case '%':
            case '[':
            case ']':
            case '{':
            case '}':
            case '(':
            case ')':
                XSAFELBCHR('%', buff, &bp);
                XSAFELBCHR(*p, buff, &bp);
                break;

            case '\r':
                break;

            case '\n':
                XSAFESTRNCAT(buff, &bp, "%r", 2, LBUF_SIZE);
                break;

            case '\t':
                XSAFESTRNCAT(buff, &bp, "%t", 2, LBUF_SIZE);
                break;

            default:
                XSAFELBCHR(*p, buff, &bp);
                break;
            }

            p++;
        }
    }
    else
    {
        const char *p = str;

        while (*p)
        {
            if (*p == ESC_CHAR && *(p + 1) == '[')
            {
                /* Skip ANSI sequence */
                const char *cursor = p;
                ColorState discard = {0};
                if (parse_and_apply_ansi_sequence(&cursor, &discard))
                {
                    p = cursor;
                    continue;
                }
            }

            switch (*p)
            {
            case '\r':
                break;

            case '\n':
            case '\t':
                XSAFELBCHR(' ', buff, &bp);
                break;

            default:
                XSAFELBCHR(*p, buff, &bp);
                break;
            }

            p++;
        }
    }

    *bp = '\0';
    return buff;
}

/**
 * @brief Remove ANSI escape codes using the ansi.c parser utilities.
 *
 * Caller frees the returned buffer with XFREE().
 */
char *ansi_strip_ansi(const char *str)
{
    if (!str || !*str)
    {
        char *empty = XMALLOC(1, "ansi_strip_ansi");
        *empty = '\0';
        return empty;
    }

    size_t len = strlen(str);
    char *buf = XMALLOC(len + 1, "ansi_strip_ansi");
    char *bp = buf;
    const char *p = str;
    ColorState discard = {0};

    while (*p)
    {
        if (*p == ESC_CHAR && *(p + 1) == '[')
        {
            const char *cursor = p;
            if (parse_and_apply_ansi_sequence(&cursor, &discard))
            {
                p = cursor;
                continue;
            }
        }

        *bp++ = *p++;
    }

    *bp = '\0';
    return buf;
}

/**
 * @brief Count visible characters ignoring ANSI escape sequences using ansi.c parser utilities.
 */
int ansi_strip_ansi_len(const char *str)
{
    if (!str || !*str)
    {
        return 0;
    }

    int len = 0;
    const char *p = str;
    ColorState discard = {0};

    while (*p)
    {
        if (*p == ESC_CHAR && *(p + 1) == '[')
        {
            const char *cursor = p;
            if (parse_and_apply_ansi_sequence(&cursor, &discard))
            {
                p = cursor;
                continue;
            }
        }

        ++len;
        ++p;
    }

    return len;
}

/**
 * @brief Map ANSI state for every character using modern ColorState format.
 * 
 * Supports ANSI basic, xterm 256-color, and truecolor.
 * 
 * Caller must free `*states` with XFREE(states, "ansi_map_states_colorstate_states")
 * and `*stripped` with XFREE(stripped, "ansi_map_states_colorstate_stripped").
 *
 * @param s Input string (may contain ANSI/xterm/truecolor escape sequences)
 * @param states Out: allocated array of ColorState for each character
 * @param stripped Out: stripped string without ANSI codes
 * @return int Number of characters mapped
 */
int ansi_map_states_colorstate(const char *s, ColorState **states, char **stripped)
{
    ColorState *color_states;
    char *text;
    const char *p;
    ColorState current_state = {0};
    int n = 0;
    const int map_cap = HBUF_SIZE - 1;
    const int strip_cap = LBUF_SIZE - 1;
    
    color_states = (ColorState *)XCALLOC(HBUF_SIZE, sizeof(ColorState), "ansi_map_states_colorstate_states");
    text = XMALLOC(LBUF_SIZE, "ansi_map_states_colorstate_stripped");
    
    if (!s || !*s)
    {
        color_states[0] = current_state;
        text[0] = '\0';
        *states = color_states;
        *stripped = text;
        return 0;
    }

    p = s;
    while (*p && n < map_cap && n < strip_cap)
    {
        if (*p == ESC_CHAR)
        {
            // Parse ANSI sequence and update current_state
            parse_and_apply_ansi_sequence(&p, &current_state);
        }
        else
        {
            // Record color state for this character
            color_states[n] = current_state;
            text[n++] = *p++;
        }
    }

    // Skip any trailing ANSI sequences
    while (*p)
    {
        if (*p == ESC_CHAR)
        {
            parse_and_apply_ansi_sequence(&p, &current_state);
        }
        else
        {
            ++p;
        }
    }

    color_states[n] = current_state;  // Final state for transition at end
    text[n] = '\0';
    *states = color_states;
    *stripped = text;
    return n;
}

/**
 * @brief Parse ANSI escape sequence and return ColorState.
 * 
 * @param ansi_ptr Pointer to string pointer, will be advanced past the parsed sequence
 * @return ColorState Parsed color state
 */
ColorState ansi_parse_sequence(const char **ansi_ptr)
{
    ColorState state = {0};
    
    if (!ansi_ptr || !*ansi_ptr || **ansi_ptr != '\033')
        return state;
    
    const char *start = *ansi_ptr;
    (*ansi_ptr)++; // skip ESC
    
    if (**ansi_ptr == '[')
    {
        (*ansi_ptr)++; // skip [
        
        // Find the end of the sequence (ends with 'm')
        const char *code_start = *ansi_ptr;
        while (**ansi_ptr && **ansi_ptr != 'm')
            (*ansi_ptr)++;
        
        if (**ansi_ptr == 'm')
        {
            // Extract the code between [ and m
            size_t code_len = *ansi_ptr - code_start;
            if (code_len > 0)
            {
                char *code = XMALLOC(code_len + 1, "code");
                memcpy(code, code_start, code_len);
                code[code_len] = 0;
                
                // Parse the ANSI code using the existing function
                ansi_parse_ansi_code(&state, code);
                
                XFREE(code);
            }
            (*ansi_ptr)++; // skip 'm'
        }
        else
        {
            // Invalid sequence, reset pointer
            *ansi_ptr = start;
        }
    }
    else
    {
        // Not a valid sequence, reset pointer
        *ansi_ptr = start;
    }
    
    return state;
}

/*
 * The following functions were moved from string_ansi.c to consolidate ANSI
 * helpers in a single translation unit.
 */

static void convert_color_to_sequence(const ColorState *attr, bool ansi, bool xterm,
                                      void (*check_space_fn)(size_t needed, void *ctx),
                                      void (*write_fn)(const char *data, size_t len, void *ctx),
                                      void *ctx)
{
    if (xterm)
    {
        bool has_fg = (attr->foreground.is_set == ColorStatusSet);
        bool has_bg = (attr->background.is_set == ColorStatusSet);

        if (has_fg || has_bg || (attr->reset == ColorStatusReset))
        {
            if (check_space_fn)
                check_space_fn(64, ctx);

            char seq[64];
            int seq_len = 0;

            seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "%c[", ESC_CHAR);

            if (has_fg)
            {
                int xterm_fg;
                if (attr->foreground.xterm_index >= 0 && attr->foreground.xterm_index <= 255)
                    xterm_fg = attr->foreground.xterm_index;
                else
                {
                    ColorCIELab lab = ansi_rgb_to_cielab(attr->foreground.truecolor);
                    ColorEntry closest = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm);
                    xterm_fg = closest.xterm_index;
                }

                seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "38;5;%d", xterm_fg);
            }

            if (has_bg)
            {
                int xterm_bg;
                if (attr->background.xterm_index >= 0 && attr->background.xterm_index <= 255)
                    xterm_bg = attr->background.xterm_index;
                else
                {
                    ColorCIELab lab = ansi_rgb_to_cielab(attr->background.truecolor);
                    ColorEntry closest = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm);
                    xterm_bg = closest.xterm_index;
                }

                if (has_fg)
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, ";48;5;%d", xterm_bg);
                else
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "48;5;%d", xterm_bg);
            }

            if (attr->reset == ColorStatusReset)
            {
                if (has_fg || has_bg)
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, ";0");
                else
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "0");
            }

            seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "m");

            write_fn(seq, seq_len, ctx);
        }
    }
    else if (ansi)
    {
        bool has_fg = (attr->foreground.is_set == ColorStatusSet);
        bool has_bg = (attr->background.is_set == ColorStatusSet);

        if (has_fg || has_bg || (attr->reset == ColorStatusReset))
        {
            if (check_space_fn)
                check_space_fn(64, ctx);

            char seq[64];
            int seq_len = 0;

            seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "%c[", ESC_CHAR);

            if (has_fg)
            {
                int f;
                if (attr->foreground.ansi_index >= 0 && attr->foreground.ansi_index <= 15)
                {
                    ColorCIELab lab = ansi_rgb_to_cielab(attr->foreground.truecolor);
                    ColorEntry closest = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm);
                    uint8_t idx = closest.xterm_index & 0xF;
                    f = (idx < 8) ? (30 + idx) : (90 + (idx - 8));
                }
                else
                {
                    ColorCIELab lab = ansi_rgb_to_cielab(attr->foreground.truecolor);
                    ColorEntry closest = ansi_find_closest_color_with_lab(lab, ColorTypeAnsi);
                    uint8_t a = closest.ansi_index;
                    f = (a < 8) ? (30 + a) : (90 + (a - 8));
                }
                seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "%d", f);
            }

            if (has_bg)
            {
                int b;
                if (attr->background.ansi_index >= 0 && attr->background.ansi_index <= 15)
                {
                    ColorCIELab lab = ansi_rgb_to_cielab(attr->background.truecolor);
                    ColorEntry closest = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm);
                    uint8_t idx = closest.xterm_index & 0xF;
                    b = (idx < 8) ? (40 + idx) : (100 + (idx - 8));
                }
                else
                {
                    ColorCIELab lab = ansi_rgb_to_cielab(attr->background.truecolor);
                    ColorEntry closest = ansi_find_closest_color_with_lab(lab, ColorTypeAnsi);
                    uint8_t a = closest.ansi_index;
                    b = (a < 8) ? (40 + a) : (100 + (a - 8));
                }

                if (has_fg)
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, ";%d", b);
                else
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "%d", b);
            }

            if (attr->reset == ColorStatusReset)
            {
                if (has_fg || has_bg)
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, ";0");
                else
                    seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "0");
            }

            seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "m");

            write_fn(seq, seq_len, ctx);
        }
    }
}

typedef struct LevelAnsiStreamContext
{
    char *buf;
    char **p_ptr;
    const char *end;
    size_t flush_threshold;
    void (*flush_fn)(const char *data, size_t len, void *ctx);
    void *flush_ctx;
} LevelAnsiStreamContext;

static inline void flush_if_needed(char **p_ptr, char *buf, size_t flush_threshold,
                                   void (*flush_fn)(const char *data, size_t len, void *ctx),
                                   void *ctx)
{
    size_t used = (size_t)(*p_ptr - buf);
    if (used >= flush_threshold)
    {
        **p_ptr = '\0';
        flush_fn(buf, used, ctx);
        *p_ptr = buf;
    }
}

static void level_ansi_stream_write(const char *data, size_t len, void *ctx)
{
    LevelAnsiStreamContext *context = (LevelAnsiStreamContext *)ctx;
    size_t copy_len = (len < (size_t)(context->end - *context->p_ptr)) ? len : (context->end - *context->p_ptr);
    XMEMCPY(*context->p_ptr, data, copy_len);
    *context->p_ptr += copy_len;

    size_t used = *context->p_ptr - context->buf;
    if (used >= context->flush_threshold)
    {
        **context->p_ptr = '\0';
        context->flush_fn(context->buf, used, context->flush_ctx);
        *context->p_ptr = context->buf;
    }
}

static inline void ensure_buffer_space(size_t needed, char **buf, char **p, char **end, size_t *buf_size)
{
    size_t used = (size_t)(*p - *buf);
    if (used + needed >= *buf_size - 1)
    {
        *buf_size *= 2;
        *buf = XREALLOC(*buf, *buf_size, "buf");
        *p = *buf + used;
        *end = *buf + *buf_size - 1;
    }
}

void level_ansi_stream(const char *s, bool ansi, bool xterm, bool truecolors,
               void (*flush_fn)(const char *data, size_t len, void *ctx), void *ctx)
{
    char buf[8192];
    char *p = buf;
    char *end = buf + sizeof(buf) - 1;
    const size_t flush_threshold = sizeof(buf) * 80 / 100;

    if (!s || !*s)
        return;

    while (*s)
    {
        if (*s == ESC_CHAR)
        {
            if (truecolors)
            {
                const char *s_start = s;
                s++;
                if (*s == '[')
                {
                    s++;
                    while (*s && !isalpha(*s))
                        s++;
                    if (*s)
                        s++;
                }

                while (s_start < s && *s_start)
                {
                    *p++ = *s_start++;
                    flush_if_needed(&p, buf, flush_threshold, flush_fn, ctx);
                }
            }
            else
            {
                ColorState attr = ansi_parse_sequence(&s);

                LevelAnsiStreamContext stream_ctx = {buf, &p, end, flush_threshold, flush_fn, ctx};
                convert_color_to_sequence(&attr, ansi, xterm, NULL, level_ansi_stream_write, &stream_ctx);

                p = *stream_ctx.p_ptr;
            }
        }
        else
        {
            *p++ = *s++;
            flush_if_needed(&p, buf, flush_threshold, flush_fn, ctx);
        }
    }

    if (p > buf)
    {
        *p = '\0';
        flush_fn(buf, (size_t)(p - buf), ctx);
    }
}

char *normal_to_white(const char *raw)
{
    char *buf, *p;
    const char *s = raw;
    const char *last_pos = s;
    size_t buf_size = LBUF_SIZE;

    buf = XMALLOC(buf_size, "buf");
    p = buf;

    if (!s || !*s)
    {
        *p = '\0';
        return buf;
    }

    while (*s)
    {
        if (*s == ESC_CHAR)
        {
            size_t text_len = s - last_pos;
            if (text_len > 0)
            {
                ensure_buffer_space(text_len, &buf, &p, &p + buf_size - 1, &buf_size);
                XMEMCPY(p, last_pos, text_len);
                p += text_len;
            }

            const char *seq_start = s;
            ColorState state = ansi_parse_sequence(&s);

            if (state.reset == ColorStatusReset)
            {
                ColorState white_state = {0};
                white_state.foreground.is_set = ColorStatusSet;
                white_state.foreground.ansi_index = 7;
                white_state.foreground.xterm_index = 7;
                white_state.foreground.truecolor = (ColorRGB){255, 255, 255};

                white_state.background = state.background;
                white_state.highlight = state.highlight;
                white_state.underline = state.underline;
                white_state.flash = state.flash;
                white_state.inverse = state.inverse;

                char temp_buf[128];
                size_t offset = 0;
                to_ansi_escape_sequence(temp_buf, sizeof(temp_buf), &offset, &white_state, ColorTypeAnsi);

                ensure_buffer_space(offset, &buf, &p, &p + buf_size - 1, &buf_size);
                XMEMCPY(p, temp_buf, offset);
                p += offset;
            }
            else
            {
                size_t seq_len = s - seq_start;
                ensure_buffer_space(seq_len, &buf, &p, &p + buf_size - 1, &buf_size);
                XMEMCPY(p, seq_start, seq_len);
                p += seq_len;
            }

            last_pos = s;
        }
        else
        {
            s++;
        }
    }

    size_t text_len = s - last_pos;
    if (text_len > 0)
    {
        ensure_buffer_space(text_len, &buf, &p, &p + buf_size - 1, &buf_size);
        XMEMCPY(p, last_pos, text_len);
        p += text_len;
    }

    *p = '\0';
    return buf;
}

void skip_esccode(char **s)
{
    char *p = *s + 1;

    if (!*p)
    {
        *s = p;
        return;
    }

    if (*p == ANSI_CSI)
    {
        while (*++p && (*p & 0xf0) == 0x30)
            ;
    }

    while (*p && (*p & 0xf0) == 0x20)
    {
        ++p;
    }

    if (*p)
    {
        ++p;
    }

    *s = p;
}

static inline int fg_sgr_from_state(const ColorState *state)
{
    if (!state || state->foreground.is_set != ColorStatusSet)
    {
        return -1;
    }

    int idx = state->foreground.ansi_index;
    if (idx >= 0 && idx < 8)
    {
        return 30 + idx;
    }

    if (idx >= 8 && idx < 16)
    {
        return 90 + (idx - 8);
    }

    return -1;
}

static inline int bg_sgr_from_state(const ColorState *state)
{
    if (!state || state->background.is_set != ColorStatusSet)
    {
        return -1;
    }

    int idx = state->background.ansi_index;
    if (idx >= 0 && idx < 8)
    {
        return 40 + idx;
    }

    if (idx >= 8 && idx < 16)
    {
        return 100 + (idx - 8);
    }

    return -1;
}

static inline void apply_sgr_to_state(ColorState *state, int sgr)
{
    if (!state)
    {
        return;
    }

    if (sgr >= 30 && sgr <= 37)
    {
        int idx = sgr - 30;
        state->foreground.is_set = ColorStatusSet;
        state->foreground.ansi_index = idx;
        state->foreground.xterm_index = idx;
    }
    else if (sgr >= 40 && sgr <= 47)
    {
        int idx = sgr - 40;
        state->background.is_set = ColorStatusSet;
        state->background.ansi_index = idx;
        state->background.xterm_index = idx;
    }
}

char *remap_colors(const char *s, int *cmap)
{
    char *buf;
    char *bp;
    int n;
    buf = XMALLOC(LBUF_SIZE, "buf");

    if (!s || !*s || !cmap)
    {
        if (s)
        {
            XSTRNCPY(buf, s, LBUF_SIZE - 1);
            buf[LBUF_SIZE - 1] = '\0';
        }
        else
        {
            buf[0] = '\0';
        }

        return (buf);
    }

    bp = buf;

    while (*s)
    {
        if (*s != ESC_CHAR)
        {
            XSAFELBCHR(*s, buf, &bp);
            ++s;
            continue;
        }

        const char *seq_start = s;
        ColorState state = ansi_parse_sequence(&s);

        if (s == seq_start)
        {
            XSAFELBCHR(*s, buf, &bp);
            ++s;
            continue;
        }

        n = fg_sgr_from_state(&state);
        if (n >= I_ANSI_BLACK && n < I_ANSI_NUM && cmap[n - I_ANSI_BLACK] != 0)
        {
            apply_sgr_to_state(&state, cmap[n - I_ANSI_BLACK]);
        }

        n = bg_sgr_from_state(&state);
        if (n >= I_ANSI_BLACK && n < I_ANSI_NUM && cmap[n - I_ANSI_BLACK] != 0)
        {
            apply_sgr_to_state(&state, cmap[n - I_ANSI_BLACK]);
        }

        char seq_buf[128];
        size_t offset = 0;
        ColorStatus status = to_ansi_escape_sequence(seq_buf, sizeof(seq_buf), &offset, &state, ColorTypeAnsi);
        if (status == ColorStatusNone || offset == 0)
        {
            for (const char *p = seq_start; p < s; ++p)
            {
                XSAFELBCHR(*p, buf, &bp);
            }
        }
        else
        {
            seq_buf[offset] = '\0';
            XSAFELBSTR(seq_buf, buf, &bp);
        }
    }

    return (buf);
}

ColorType resolve_color_type(dbref player, dbref cause)
{
	dbref target = (cause != NOTHING) ? cause : player;

	if (target != NOTHING && Color24Bit(target))
	{
		return ColorTypeTrueColor;
	}

	if (target != NOTHING && Color256(target))
	{
		return ColorTypeXTerm;
	}

	if (target != NOTHING && Ansi(target))
	{
		return ColorTypeAnsi;
	}

	return ColorTypeNone;
}