#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Enumeration of supported color output types.
 *
 * Defines the different color rendering modes supported by the ANSI library.
 * Each type corresponds to a different ANSI escape sequence format and color depth.
 */
typedef enum ColorType
{
    ColorTypeNone,      /**< No color output - plain text only */
    ColorTypeAnsi,      /**< 16-color ANSI escape sequences (basic colors) */
    ColorTypeXTerm,     /**< 256-color XTerm escape sequences */
    ColorTypeTrueColor, /**< 24-bit TrueColor escape sequences (16.7 million colors) */
} ColorType;

/**
 * @brief Status enumeration for color and attribute states.
 *
 * Used to track whether colors or text attributes are set, reset, or unchanged.
 * This allows for incremental updates to color states.
 */
typedef enum ColorStatus
{
    ColorStatusReset = -1, /**< Reset the attribute/color to default */
    ColorStatusNone,       /**< Attribute/color is not specified (no change) */
    ColorStatusSet         /**< Attribute/color is actively set */
} ColorStatus;

/**
 * @brief Enumeration of color string parsing types.
 *
 * Identifies how a color string should be interpreted during parsing.
 * Used internally by the color parsing functions.
 */
typedef enum ColorParseType
{
    ColorParseTypeName,   /**< Color specified by name (e.g., "red", "blue") */
    ColorParseTypeHex,    /**< Color specified in hexadecimal (e.g., "#FF0000", "xterm196") */
    ColorParseTypeMush,   /**< Color specified in MUSHcode format (e.g., "r", "g", "xh") */
    ColorParseTypeInvalid /**< Invalid or unrecognized color format */
} ColorParseType;

/**
 * @brief RGB color representation.
 *
 * Stores a color as 24-bit RGB values with 8 bits per channel.
 * Used for TrueColor representation and conversion to other color spaces.
 */
typedef struct ColorRGB
{
    uint8_t r; /**< Red component (0-255) */
    uint8_t g; /**< Green component (0-255) */
    uint8_t b; /**< Blue component (0-255) */
} ColorRGB;

/**
 * @brief CIELAB color space representation.
 *
 * Stores a color in the CIE L*a*b* color space, which is perceptually uniform.
 * Used for accurate color distance calculations and color matching.
 */
typedef struct ColorCIELab
{
    double L; /**< Lightness component (0-100) */
    double a; /**< Green-red axis (-128 to 128) */
    double b; /**< Blue-yellow axis (-128 to 128) */
} ColorCIELab;

/**
 * @brief Color information for a single color (foreground or background).
 *
 * Contains all the information needed to represent a color in different formats.
 * Supports ANSI 16-color, XTerm 256-color, and TrueColor representations.
 */
typedef struct ColorInfo
{
    ColorStatus is_set;    /**< Whether this color is set or reset */
    int ansi_index;        /**< ANSI 16-color index (0-15) */
    int xterm_index;       /**< XTerm 256-color index (0-255) */
    ColorRGB truecolor;    /**< 24-bit RGB color values */
} ColorInfo;

/**
 * @brief Complete text formatting state.
 *
 * Represents the full state of text formatting including colors and attributes.
 * This is the main structure used to track and manipulate text appearance.
 */
typedef struct ColorState
{
    ColorInfo foreground;     /**< Foreground (text) color information */
    ColorInfo background;     /**< Background color information */
    ColorStatus reset;        /**< Reset all attributes flag */
    ColorStatus flash;        /**< Text flashing/blinking attribute */
    ColorStatus highlight;    /**< Text highlighting/bold attribute */
    ColorStatus underline;    /**< Text underline attribute */
    ColorStatus inverse;      /**< Text inverse/reverse attribute */
} ColorState;

/**
 * @brief Color definition entry.
 *
 * Contains all information about a named color including its various representations.
 * Used in the global colorDefinitions table for color name lookups.
 */
typedef struct ColorEntry
{
    char *name;           /**< Color name (e.g., "red", "brightblue") */
    ColorType type;       /**< Color type classification */
    int mush_code;        /**< MUSHcode character code (-1 if none) */
    int ansi_index;       /**< ANSI 16-color index */
    int xterm_index;      /**< XTerm 256-color index */
    ColorRGB truecolor;   /**< RGB color values */
    ColorCIELab lab;      /**< Pre-computed CIELAB coordinates for matching */
} ColorEntry;

/**
 * @brief Individual color sequence data.
 *
 * Represents a single color change at a specific position in text.
 * Part of the ColorSequence structure for embedded color parsing.
 */
typedef struct ColorSequenceData
{
    size_t position;   /**< Position in the text where this color applies */
    size_t length;     /**< Length of the color code that was parsed */
    ColorState color;  /**< The color state to apply at this position */
} ColorSequenceData;

/**
 * @brief Parsed color sequence information.
 *
 * Contains the plain text and an array of color changes that occur at specific positions.
 * Used for parsing embedded MUSHcode sequences and converting between formats.
 */
typedef struct ColorSequence
{
    size_t count;              /**< Number of color sequences */
    char *text;                /**< Plain text with color codes removed */
    ColorSequenceData *data;   /**< Array of color change data */
} ColorSequence;

// Function declarations
ColorCIELab ansi_rgb_to_cielab(ColorRGB color);
double ansi_ciede2000(ColorCIELab lab1, ColorCIELab lab2);
ColorEntry ansi_find_closest_color_with_lab(ColorCIELab lab, ColorType type);
void ansi_get_color_from_rgb(ColorState *color, ColorRGB rgb, bool is_background);
void ansi_get_color_from_index(ColorState *color, int index, bool is_background);
void ansi_get_color_from_name(ColorState *color, const char *name, bool is_background);
bool ansi_get_color_from_text(ColorState *color, char *text, bool is_background);
bool ansi_parse_color_from_string(ColorState *color, const char *color_str, bool is_background);
ColorStatus to_ansi_escape_sequence(char *buffer, size_t buffer_size, size_t *offset, ColorState *to, ColorType type);
ColorState ansi_packed_to_colorstate(int packed);
int ansi_colorstate_to_packed(ColorState state);
char *ansi_generate_transition_sequence(int current, int target);
int ansi_parse_embedded_sequences(const char *input, ColorSequence *sequences);
char *ansi_apply_sequences(const ColorSequence *sequences, ColorType color_type);
bool ansi_parse_ansi_to_sequences(const char *input, ColorSequence *sequences);
ColorState ansi_parse_sequence(const char **ansi_ptr);
char *ansi_sequences_to_embedded(const ColorSequence *sequences);

const char *ansi_char_to_sequence(int ch);
const char *ansi_char_bright_to_sequence(int ch);
int ansi_char_to_num(int ch);

extern ColorEntry colorDefinitions[];