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

/**
 * @brief ANSI color code constants and definitions.
 */


extern const int C_ANSI_BLACK;
extern const int C_ANSI_NUM;

#define C_ANSI_ESC '\033'
//#define C_ANSI_CSI '['
extern const char C_ANSI_CSI;
extern const char C_ANSI_RESET[];
extern const char C_ANSI_BOLD[];
extern const char C_ANSI_UNDERLINE[];
extern const char C_ANSI_BLINK[];
extern const char C_ANSI_REVERSE[];
extern const char C_ANSI_NORMAL_INTENSITY[];
extern const char C_ANSI_NO_UNDERLINE[];
extern const char C_ANSI_NO_BLINK[];
extern const char C_ANSI_NO_REVERSE[];
extern const char C_ANSI_FOREGROUND_RESET[];
extern const char C_ANSI_BACKGROUND_RESET[];
extern const char C_ANSI_END[];
extern const char C_ANSI_RESET_SEQUENCE[];
extern const char C_ANSI_XTERM_PREFIX_FG[];
extern const char C_ANSI_XTERM_PREFIX_BG[];
extern const char C_ANSI_TRUECOLOR_PREFIX_FG[];
extern const char C_ANSI_TRUECOLOR_PREFIX_BG[];

extern const char C_ANSI_NORMAL_SEQ[];
extern const char C_ANSI_BOLD_SEQ[];

extern ColorEntry colorDefinitions[];

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
bool ansi_parse_embedded_sequences(const char *input, ColorSequence *sequences);
bool ansi_parse_ansi_to_sequences(const char *input, ColorSequence *sequences);
ColorState ansi_parse_sequence(const char **ansi_ptr);
bool ansi_apply_sequence(const char **ptr, ColorState *state);
int ansi_parse_single_x_code(char **input_ptr, ColorState *color_out, bool *current_highlight);
void xsafe_ansi_normal(char *buff, char **bufc);
char *ansi_parse_x_to_sequence(char **ptr, ColorType type);
char *color_state_to_mush_code(const ColorState *color);
char *color_state_to_letters(const ColorState *color);
char *color_state_to_escape(const ColorState *color, ColorType type);
char *ansi_states_to_sequence(ColorState *states, int count, ColorType type);
char *ansi_to_mushcode(const char *input);
char *convert_mush_to_ansi(const char *input, ColorType type);
const char *ansi_char_to_sequence(int ch);
const char *ansi_char_bright_to_sequence(int ch);
int ansi_char_to_num(int ch);
ColorType resolve_color_type(dbref player, dbref cause);