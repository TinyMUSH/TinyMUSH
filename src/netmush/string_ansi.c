/**
 * @file string_ansi.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief ANSI color and escape sequence handling utilities
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
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
#include <string.h>

/**
 * @brief Common function to convert ColorState to ANSI escape sequence based on player capabilities
 * 
 * @param attr The parsed color state
 * @param ansi Player supports basic ANSI colors
 * @param xterm Player supports xterm colors
 * @param check_space_fn Callback to check/ensure buffer space (can be NULL)
 * @param write_fn Callback to write data to buffer
 * @param ctx Context pointer passed to callbacks
 */
static void convert_color_to_sequence(const ColorState *attr, bool ansi, bool xterm,
                                     void (*check_space_fn)(size_t needed, void *ctx),
                                     void (*write_fn)(const char *data, size_t len, void *ctx),
                                     void *ctx)
{
    if (xterm)
    {
        // Player support xterm colors
        bool has_fg = (attr->foreground.is_set == ColorStatusSet);
        bool has_bg = (attr->background.is_set == ColorStatusSet);
        
        if (has_fg || has_bg || (attr->reset == ColorStatusReset))
        {
            if (check_space_fn)
                check_space_fn(64, ctx);
            
            char seq[64];
            int seq_len = 0;
            
            // Build complete escape sequence using XSNPRINTF
            seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "%c[", ESC_CHAR);
            
            if (has_fg)
            {
                int xterm_fg;
                // Use original xterm index if available to avoid lossy RGB conversion
                if (attr->foreground.xterm_index >= 0 && attr->foreground.xterm_index <= 255)
                    xterm_fg = attr->foreground.xterm_index;
                else {
                    ColorCIELab lab = ansi_rgb_to_cielab(attr->foreground.truecolor);
                    ColorEntry closest = ansi_find_closest_color_with_lab(lab, ColorTypeXTerm);
                    xterm_fg = closest.xterm_index;
                }
                
                seq_len += XSNPRINTF(seq + seq_len, sizeof(seq) - seq_len, "38;5;%d", xterm_fg);
            }
            
            if (has_bg)
            {
                int xterm_bg;
                // Use original xterm index if available to avoid lossy RGB conversion
                if (attr->background.xterm_index >= 0 && attr->background.xterm_index <= 255)
                    xterm_bg = attr->background.xterm_index;
                else {
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
        // Player support ansi colors, convert to ansi
        bool has_fg = (attr->foreground.is_set == ColorStatusSet);
        bool has_bg = (attr->background.is_set == ColorStatusSet);
        
        if (has_fg || has_bg || (attr->reset == ColorStatusReset))
        {
            if (check_space_fn)
                check_space_fn(64, ctx);
            
            char seq[64];
            int seq_len = 0;
            
            // Build escape sequence
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

/**
 * @brief Convert ansi character code (%x?) to ansi sequence.
 * Lookup table for fast O(1) character to ANSI code conversion.
 *
 * @param ch Character to convert
 * @return char* Ansi sequence
 */
const char *ansiChar(int ch)
{
	static const char *ansi_table[256] = {
		['B'] = ANSI_BBLUE,
		['C'] = ANSI_BCYAN,
		['G'] = ANSI_BGREEN,
		['M'] = ANSI_BMAGENTA,
		['R'] = ANSI_BRED,
		['W'] = ANSI_BWHITE,
		['X'] = ANSI_BBLACK,
		['Y'] = ANSI_BYELLOW,
		['b'] = ANSI_BLUE,
		['c'] = ANSI_CYAN,
		['f'] = ANSI_BLINK,
		['g'] = ANSI_GREEN,
		['h'] = ANSI_HILITE,
		['i'] = ANSI_INVERSE,
		['m'] = ANSI_MAGENTA,
		['n'] = ANSI_NORMAL,
		['r'] = ANSI_RED,
		['u'] = ANSI_UNDER,
		['w'] = ANSI_WHITE,
		['x'] = ANSI_BLACK,
		['y'] = ANSI_YELLOW,
	};
	
	unsigned char uch = (unsigned char)ch;
	return ansi_table[uch] ? ansi_table[uch] : STRING_EMPTY;
}

/**
 * @brief Convert ansi character code (%x? uppercase) to bright ansi sequence.
 * Returns the bright/high intensity variant (90-97) for foreground colors.
 *
 * @param ch Character to convert (should match lowercase versions)
 * @return char* Bright ansi sequence
 */
const char *ansiChar_Bright(int ch)
{
	static const char *bright_table[256] = {
		['b'] = ANSI_BRIGHT_BLUE,
		['c'] = ANSI_BRIGHT_CYAN,
		['g'] = ANSI_BRIGHT_GREEN,
		['m'] = ANSI_BRIGHT_MAGENTA,
		['r'] = ANSI_BRIGHT_RED,
		['w'] = ANSI_BRIGHT_WHITE,
		['x'] = ANSI_BRIGHT_BLACK,
		['y'] = ANSI_BRIGHT_YELLOW,
	};
	
	unsigned char uch = (unsigned char)ch;
	return bright_table[uch] ? bright_table[uch] : STRING_EMPTY;
}

/**
 * @brief Convert ansi character code (%x?) to numeric values.
 * Lookup table for fast O(1) character to ANSI numeric code conversion.
 *
 * @param ch ANSI character
 * @return int ANSI numeric values
 */
const int ansiNum(int ch)
{
	static const int ansi_num_table[256] = {
		['B'] = 44,
		['C'] = 46,
		['G'] = 42,
		['M'] = 45,
		['R'] = 41,
		['W'] = 47,
		['X'] = 40,
		['Y'] = 43,
		['b'] = 34,
		['c'] = 36,
		['f'] = 5,
		['g'] = 32,
		['h'] = 1,
		['i'] = 7,
		['m'] = 35,
		['n'] = 0,
		['r'] = 31,
		['u'] = 4,
		['w'] = 37,
		['x'] = 30,
		['y'] = 33,
	};
	
	unsigned char uch = (unsigned char)ch;
	return ansi_num_table[uch];
}

/**
 * @brief Convert ansi numeric code to ansi character code.
 * Lookup table for fast O(1) numeric to ANSI character conversion.
 *
 * @param num ANSI numeric code
 * @return char ANSI character
 */
const char ansiLetter(int num)
{
	static const char letter_table[48] = {
		[0] = 'n', [1] = 'h', [4] = 'u', [5] = 'f', [7] = 'i',
		[30] = 'X', [31] = 'R', [32] = 'G', [33] = 'Y',
		[34] = 'B', [35] = 'M', [36] = 'C', [37] = 'W',
		[40] = 'x', [41] = 'r', [42] = 'g', [43] = 'y',
		[44] = 'b', [45] = 'm', [46] = 'c', [47] = 'w',
	};
	
	return (num >= 0 && num < 48) ? letter_table[num] : '\0';
}

/**
 * @brief Convert ansi numeric code to mushcode character for foreground/background
 * Lookup table for fast O(1) numeric to ANSI mushcode character conversion.
 *
 * @param num ANSI numeric code (0-7)
 * @param bg true for background color, false for foreground
 * @return char ANSI mushcode character
 */
const char ansiMushCode(int num, bool bg)
{
	static const char fg_table[8] = {'x', 'r', 'g', 'y', 'b', 'm', 'c', 'w'};
	static const char bg_table[8] = {'X', 'R', 'G', 'Y', 'B', 'M', 'C', 'W'};
	
	if (num < 0 || num > 7)
		return '\0';
	
	return bg ? bg_table[num] : fg_table[num];
}

/*
int ansi_mask_bits[I_ANSI_LIM] = {

	0x1fff, 0x1100, 0x1100, 0x0000, 0x1200, 0x1400, 0x0000, 0x1800, 0x0000, 0x0000,
	0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
	0x0000, 0x1100, 0x1100, 0x0000, 0x1200, 0x1400, 0x0000, 0x1800, 0x0000, 0x0000,
	0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x100f, 0x0000, 0x0000,
	0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x10f0, 0x0000, 0x0000};
*/

/**
 * @brief ANSI packed state definitions -- number-to-bitmask translation
 *
 * The mask specifies the state bits that are altered by a particular ansi
 * code. Bits are laid out as follows:
 *
 *  0x2000 -- Bright color flags
 *  0x1000 -- No ansi. Every valid ansi code clears this bit.
 *  0x0800 -- inverse
 *  0x0400 -- flash
 *  0x0200 -- underline
 *  0x0100 -- highlight
 *  0x0080 -- "use default bg", set by ansi normal, cleared by other bg's
 *  0x0070 -- three bits of bg color
 *  0x0008 -- "use default fg", set by ansi normal, cleared by other fg's
 *  0x0007 -- three bits of fg color
 *
 * @param num ANSI number
 * @return int bitmask
 */
const int ansiBitsMask(int num)
{
	static const int mask_table[48] = {
		[0] = 0x1fff,
		[1] = 0x1100, [2] = 0x1100, [21] = 0x1100, [22] = 0x1100,
		[4] = 0x1200, [24] = 0x1200,
		[5] = 0x1400, [25] = 0x1400,
		[7] = 0x1800, [27] = 0x1800,
		[30] = 0x100f, [31] = 0x100f, [32] = 0x100f, [33] = 0x100f,
		[34] = 0x100f, [35] = 0x100f, [36] = 0x100f, [37] = 0x100f,
		[40] = 0x10f0, [41] = 0x10f0, [42] = 0x10f0, [43] = 0x10f0,
		[44] = 0x10f0, [45] = 0x10f0, [46] = 0x10f0, [47] = 0x10f0,
	};
	
	if (num >= 0 && num < 48)
		return mask_table[num];
	return 0;
}

/**
 * \brief ANSI packed state definitions -- number-to-bitvalue translation table.
 */

/**
 * @brief ANSI packed state definitions -- number-to-bitvalue translation
 *
 * @param num ANSI number
 * @return int ANSI bitvalue.
 */
const int ansiBits(int num)
{
	static const int bits_table[48] = {
		[0] = 0x0099,
		[1] = 0x0100,
		[4] = 0x0200, [5] = 0x0400, [7] = 0x0800,
		[31] = 0x0001, [32] = 0x0002, [33] = 0x0003, [34] = 0x0004,
		[35] = 0x0005, [36] = 0x0006, [37] = 0x0007,
		[41] = 0x0010, [42] = 0x0020, [43] = 0x0030, [44] = 0x0040,
		[45] = 0x0050, [46] = 0x0060, [47] = 0x0070,
	};
	
	if (num >= 0 && num < 48)
		return bits_table[num];
	return 0;
}

/**
 * @brief Append a character to buffer with bounds checking
 * @param p_ptr Pointer to current position pointer
 * @param end End of buffer
 * @param ch Character to append
 */
static inline void append_ch(char **p_ptr, const char *end, char ch)
{
	if (*p_ptr < end)
	{
		**p_ptr = ch;
		(*p_ptr)++;
	}
}

/**
 * @brief Append a string to buffer with bounds checking
 * @param p_ptr Pointer to current position pointer
 * @param end End of buffer
 * @param str String to append
 */
static inline void append_str(char **p_ptr, const char *end, const char *str)
{
	while (*str && (*p_ptr < end))
	{
		**p_ptr = *str++;
		(*p_ptr)++;
	}
}

/**
 * @brief Context structure for level_ansi buffer management
 */
typedef struct {
    char **p_ptr;
    const char *end;
    size_t *buf_size;
    char **buf;
} LevelAnsiContext;

/**
 * @brief Check buffer space for level_ansi
 */
static void level_ansi_check_space(size_t needed, void *ctx)
{
    LevelAnsiContext *context = (LevelAnsiContext *)ctx;
    size_t used = *context->p_ptr - *context->buf;
    if (used + needed >= *context->buf_size - 1) {
        *context->buf_size *= 2;
        *context->buf = XREALLOC(*context->buf, *context->buf_size, "buf");
        *context->p_ptr = *context->buf + used;
        context->end = *context->buf + *context->buf_size - 1;
    }
}

/**
 * @brief Write data to level_ansi buffer
 */
static void level_ansi_write(const char *data, size_t len, void *ctx)
{
    LevelAnsiContext *context = (LevelAnsiContext *)ctx;
    size_t copy_len = (len < (size_t)(context->end - *context->p_ptr)) ? len : (context->end - *context->p_ptr);
    XMEMCPY(*context->p_ptr, data, copy_len);
    *context->p_ptr += copy_len;
}
typedef struct {
    char *buf;
    char **p_ptr;
    const char *end;
    size_t flush_threshold;
    void (*flush_fn)(const char *data, size_t len, void *ctx);
    void *flush_ctx;
} LevelAnsiStreamContext;

/**
 * @brief Check buffer space for level_ansi_stream (no-op, buffer is fixed)
 */
static void level_ansi_stream_check_space(size_t needed, void *ctx)
{
    // No-op for streaming - buffer space is managed by FLUSH_IF_NEEDED
    (void)needed;
    (void)ctx;
}

/**
 * @brief Write data to level_ansi_stream buffer
 */
static void level_ansi_stream_write(const char *data, size_t len, void *ctx)
{
    LevelAnsiStreamContext *context = (LevelAnsiStreamContext *)ctx;
    size_t copy_len = (len < (size_t)(context->end - *context->p_ptr)) ? len : (context->end - *context->p_ptr);
    XMEMCPY(*context->p_ptr, data, copy_len);
    *context->p_ptr += copy_len;
    
    // Check if we need to flush
    size_t used = *context->p_ptr - context->buf;
    if (used >= context->flush_threshold) {
        **context->p_ptr = '\0';
        context->flush_fn(context->buf, used, context->flush_ctx);
        *context->p_ptr = context->buf;
    }
}

/**
 * @brief Write mushcode transition between ANSI states into a caller buffer.
 */
static size_t ansi_transition_mushcode_into(int ansi_before, int ansi_after, char *buffer, size_t cap)
{
	char *p = buffer;
	char *end = (cap > 0) ? (buffer + cap - 1) : buffer;

	if (!cap)
		return 0;

	*p = '\0';

	if (ansi_before != ansi_after)
	{
		int ansi_bits_set = (~ansi_before) & ansi_after;
		int ansi_bits_clr = ansi_before & (~ansi_after);

		if ((ansi_bits_clr & 0xf00) || (ansi_bits_set & 0x088) || (ansi_bits_clr == 0x1000))
		{
			append_str(&p, end, "%xn");
			ansi_bits_set = (~ansiBits(0)) & ansi_after;
			ansi_bits_clr = ansiBits(0) & (~ansi_after);
		}

		if (ansi_bits_set & 0x100)
			append_str(&p, end, "%xh");

		if (ansi_bits_set & 0x200)
			append_str(&p, end, "%xu");

		if (ansi_bits_set & 0x400)
			append_str(&p, end, "%xf");

		if (ansi_bits_set & 0x800)
			append_str(&p, end, "%xi");

		if ((ansi_bits_set | ansi_bits_clr) & 0x00f)
			append_ch(&p, end, ansiMushCode(ansi_after & 0x00f, false));

		if ((ansi_bits_set | ansi_bits_clr) & 0x0f0)
			append_ch(&p, end, ansiMushCode((ansi_after & 0x0f0) >> 4, true));
	}

	*p = '\0';
	return (size_t)(p - buffer);
}

/**
 * @brief Go to a string and convert ansi to the lowest supported level.
 *
 * @param s String to convert
 * @param ansi Player support ansi
 * @param xterm Player support xterm colors
 * @param truecolors Player system support true colors
 * @return char* Converted string
 */
char *level_ansi(const char *s, bool ansi, bool xterm, bool truecolors)
{
	char *buf, *p, *end;
	size_t buf_size = LBUF_SIZE;
	size_t used;
	
	p = buf = XMALLOC(buf_size, "buf");
	end = buf + buf_size - 1;

	if (!s || !*s)
		return buf;
	
	// Macro pour vérifier et agrandir le buffer si nécessaire
	#define CHECK_BUFFER_SPACE(needed) \
		do { \
			used = p - buf; \
			if (used + (needed) >= buf_size - 1) { \
				buf_size *= 2; \
				buf = XREALLOC(buf, buf_size, "buf"); \
				p = buf + used; \
				end = buf + buf_size - 1; \
			} \
		} while(0)

	while (*s)
	{
		if (*s == ESC_CHAR)
		{
			// Got an escape code
			if (truecolors)
			{
				// Player support everything, copy entire escape sequence without parsing
				// Find the end of the escape sequence (ends with a letter)
				const char *s_start = s;
				s++; // Skip ESC
				if (*s == '[')
				{
					s++;
					// Skip until we find a letter (SGR end marker)
					while (*s && !isalpha(*s))
						s++;
					if (*s)
						s++; // Skip the final letter
				}
				
				// Ensure we have space for the escape sequence (max ~30 chars for 24-bit color)
				CHECK_BUFFER_SPACE(s - s_start + 1);
				
				// Debug: print what we're copying
				{
					char temp[256];
					int len = s - s_start;
					if (len < 256) {
						XMEMCPY(temp, s_start, len);
						temp[len] = '\0';
						fprintf(stderr, "DEBUG level_ansi: copying sequence: '%s'\n", temp);
					}
				}
				
				// Copy the entire escape sequence from source to destination
				while (s_start < s && *s_start && p < end)
				{
					append_ch(&p, end, *s_start++);
				}
			}
			else
			{
				// Player doesn't support true colors, find what is the color.
				ColorState attr = ansi_parse_sequence(&s);
				
				LevelAnsiContext ctx = {&p, end, &buf_size, &buf};
				convert_color_to_sequence(&attr, ansi, xterm, level_ansi_check_space, level_ansi_write, &ctx);
				
				// Update pointers after potential reallocation
				p = *ctx.p_ptr;
				end = (char *)ctx.end;
				buf_size = *ctx.buf_size;
			}
		}
		else
		{
			CHECK_BUFFER_SPACE(1);
			append_ch(&p, end, *s++);
		}
	}
	
	#undef CHECK_BUFFER_SPACE
	
	*p = '\0';
	
	// Debug
	fprintf(stderr, "DEBUG level_ansi: output='%s'\n", buf);
	
	return buf;
}

/**
 * @brief Convert ANSI codes based on player capabilities with streaming output
 *
 * This function processes ANSI escape sequences and flushes output when buffer
 * reaches 80% capacity. This allows handling arbitrarily large messages without
 * allocating huge buffers.
 *
 * @param s The input string containing ANSI codes
 * @param ansi Player supports basic ANSI colors (16 colors)
 * @param xterm Player supports xterm colors (256 colors)
 * @param truecolors Player supports 24-bit true colors
 * @param flush_fn Callback function to flush buffer chunks
 * @param ctx Context pointer passed to flush_fn
 */
void level_ansi_stream(const char *s, bool ansi, bool xterm, bool truecolors,
                       void (*flush_fn)(const char *data, size_t len, void *ctx), void *ctx)
{
	char buf[8192];  // 8KB buffer
	char *p = buf;
	char *end = buf + sizeof(buf) - 1;
	const size_t flush_threshold = sizeof(buf) * 80 / 100;  // 80% capacity
	
	if (!s || !*s)
		return;
	
	// Helper macro to flush buffer when threshold reached
	#define FLUSH_IF_NEEDED() \
		do { \
			size_t used = p - buf; \
			if (used >= flush_threshold) { \
				*p = '\0'; \
				flush_fn(buf, used, ctx); \
				p = buf; \
			} \
		} while(0)
	
	while (*s)
	{
		if (*s == ESC_CHAR)
		{
			// Got an escape code
			if (truecolors)
			{
				// Player support everything, copy entire escape sequence without parsing
				const char *s_start = s;
				s++; // Skip ESC
				if (*s == '[')
				{
					s++;
					// Skip until we find a letter (SGR end marker)
					while (*s && !isalpha(*s))
						s++;
					if (*s)
						s++; // Skip the final letter
				}
				
				// Copy the entire escape sequence from source to destination
				while (s_start < s && *s_start && p < end)
				{
					*p++ = *s_start++;
				}
				FLUSH_IF_NEEDED();
			}
			else
			{
				// Player doesn't support true colors, find what is the color.
				ColorState attr = ansi_parse_sequence(&s);
				
				LevelAnsiStreamContext stream_ctx = {buf, &p, end, flush_threshold, flush_fn, ctx};
				convert_color_to_sequence(&attr, ansi, xterm, level_ansi_stream_check_space, level_ansi_stream_write, &stream_ctx);
				
				// Update pointer after writing
				p = *stream_ctx.p_ptr;
			}
		}
		else
		{
			*p++ = *s++;
			FLUSH_IF_NEEDED();
		}
	}
	
	// Final flush
	if (p > buf)
	{
		*p = '\0';
		flush_fn(buf, p - buf, ctx);
	}
	
	#undef FLUSH_IF_NEEDED
}

/**
 * @brief Remove ANSI escape codes from a string
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param s The input string (may be NULL)
 * @return char* New string without ANSI codes (newly allocated)
 */
char *strip_ansi(const char *s)
{
	char *buf, *p;
	const char *s1 = s;

	if (!s || !*s)
	{
		buf = XMALLOC(1, "buf");
		*buf = '\0';
		return buf;
	}

	// Allocate buffer based on input length since output <= input
	size_t len = strlen(s);
	buf = XMALLOC(len + 1, "buf");
	p = buf;

	while (*s1)
	{
		if (*s1 == ESC_CHAR)
		{
			// Skip the entire ANSI escape sequence
			ansi_parse_sequence(&s1);
		}
		else
		{
			// Copy non-escape character
			*p++ = *s1++;
		}
	}

	*p = '\0';
	return buf;
}

/**
 * @brief Count visible characters (ignoring ANSI escape codes)
 *
 * @param s The input string
 * @return int Number of non-escape characters
 */
int strip_ansi_len(const char *s)
{
	int n = 0;

	if (!s)
	{
		return 0;
	}

	while (*s == ESC_CHAR)
	{
		skip_esccode((char **)&s);
	}

	while (*s)
	{
		++n;
		++s;

		while (*s == ESC_CHAR)
		{
			skip_esccode((char **)&s);
		}
	}

	return n;
}

/**
 * @brief Implement the NOBLEED flag by translating ANSI normal to white
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param raw Input string to process
 * @return char* String with ANSI normal mapped to white (newly allocated)
 */
char *normal_to_white(const char *raw)
{
	char *buf, *q;
	char *p = (char *)raw;
	char *just_after_csi;
	char *just_after_esccode = p;
	unsigned int param_val;
	int has_zero;
	buf = XMALLOC(LBUF_SIZE, "buf");
	q = buf;

	while (p && *p)
	{
		if (*p == ESC_CHAR)
		{
			XSAFESTRNCAT(buf, &q, just_after_esccode, p - just_after_esccode, LBUF_SIZE);

			if (p[1] == ANSI_CSI)
			{
				XSAFELBCHR(*p, buf, &q);
				++p;
				XSAFELBCHR(*p, buf, &q);
				++p;
				just_after_csi = p;
				has_zero = 0;

				while ((*p & 0xf0) == 0x30)
				{
					if (*p == '0')
					{
						has_zero = 1;
					}

					++p;
				}

				while ((*p & 0xf0) == 0x20)
				{
					++p;
				}

				if (*p == ANSI_END && has_zero)
				{
					/*
					 * it really was an ansi code,
					 * * go back and fix up the zero
					 */
					p = just_after_csi;
					param_val = 0;

					while ((*p & 0xf0) == 0x30)
					{
						if (*p < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*p & 0x0f);
							XSAFELBCHR(*p, buf, &q);
						}
						else
						{
							if (param_val == 0)
							{
								/*
								 * ansi normal
								 */
								XSAFESTRNCAT(buf, &q, "m\033[37m\033[", 8, LBUF_SIZE);
							}
							else
							{
								/*
								 * some other color
								 */
								XSAFELBCHR(*p, buf, &q);
							}

							param_val = 0;
						}

						++p;
					}

					while ((*p & 0xf0) == 0x20)
					{
						++p;
					}

					XSAFELBCHR(*p, buf, &q);
					++p;

					if (param_val == 0)
					{
						XSAFESTRNCAT(buf, &q, ANSI_WHITE, 5, LBUF_SIZE);
					}
				}
				else
				{
					++p;
					XSAFESTRNCAT(buf, &q, just_after_csi, p - just_after_csi, LBUF_SIZE);
				}
			}
			else
			{
				safe_copy_esccode(&p, buf, &q);
			}

			just_after_esccode = p;
		}
		else
		{
			++p;
		}
	}

	XSAFESTRNCAT(buf, &q, just_after_esccode, p - just_after_esccode, LBUF_SIZE);
	return (buf);
}

/**
 * @brief Build ANSI escape sequence to transition between states
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param ansi_before Current ANSI state (packed bits)
 * @param ansi_after Target ANSI state (packed bits)
 * @return char* Escape sequence performing the transition (newly allocated)
 */
char *ansi_transition_esccode(int ansi_before, int ansi_after, bool no_default_bg)
{
	int ansi_bits_set, ansi_bits_clr;
	char *p;
	char *buffer;
	buffer = XMALLOC(SBUF_SIZE, "buffer");
	*buffer = '\0';

	if (ansi_before != ansi_after)
	{
		buffer[0] = ESC_CHAR;
		buffer[1] = ANSI_CSI;
		p = buffer + 2;
		/*
		 * If they turn off any highlight bits, or they change from some color
		 * * to default color, we need to use ansi normal first.
		 */
		ansi_bits_set = (~ansi_before) & ansi_after;
		ansi_bits_clr = ansi_before & (~ansi_after);

		if ((ansi_bits_clr & 0xf00) || /* highlights off */
			(ansi_bits_set & 0x088) || /* normal to color */
			(ansi_bits_clr == 0x1000))
		{ /* explicit normal */
			XSTRCPY(p, "0;");
			p += 2;
			ansi_bits_set = (~ansiBits(0)) & ansi_after;
			ansi_bits_clr = ansiBits(0) & (~ansi_after);
		}

		/*
		 * Next reproduce the highlight state
		 */

		if (ansi_bits_set & 0x100)
		{
			XSTRCPY(p, "1;");
			p += 2;
		}

		if (ansi_bits_set & 0x200)
		{
			XSTRCPY(p, "4;");
			p += 2;
		}

		if (ansi_bits_set & 0x400)
		{
			XSTRCPY(p, "5;");
			p += 2;
		}

		if (ansi_bits_set & 0x800)
		{
			XSTRCPY(p, "7;");
			p += 2;
		}

		/*
		 * Foreground color
		 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x00f)
		{
			XSTRCPY(p, "30;");
			p += 3;
			p[-2] |= (ansi_after & 0x00f);
		}

		   /*
			* Background color
			* Si no_default_bg est vrai, on ne génère pas de background si la couleur est 0 (noir par défaut)
			*/
		   if ((ansi_bits_set | ansi_bits_clr) & 0x0f0) {
			   int bg = (ansi_after & 0x0f0) >> 4;
			   if (!(no_default_bg && bg == 0)) {
				   XSTRCPY(p, "40;");
				   p += 3;
				   p[-2] |= bg;
			   }
		   }

		/*
		 * Terminate
		 */
		if (p > buffer + 2)
		{
			p[-1] = ANSI_END;
			/*
			 * Buffer is already null-terminated by XSTRCPY
			 */
		}
		else
		{
			buffer[0] = '\0';
		}
	}

	return buffer;
}

/**
 * @brief Build mushcode sequence to transition between ANSI states
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param ansi_before Current ANSI state (packed bits)
 * @param ansi_after Target ANSI state (packed bits)
 * @return char* Mushcode sequence performing the transition (newly allocated)
 */
char *ansi_transition_mushcode(int ansi_before, int ansi_after)
{
	char *buffer = XMALLOC(SBUF_SIZE, "buffer");
	size_t len = ansi_transition_mushcode_into(ansi_before, ansi_after, buffer, SBUF_SIZE);
	buffer[len] = '\0';
	return buffer;
}

/**
 * @brief Build mushcode letter sequence to transition between ANSI states
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param ansi_before Current ANSI state (packed bits)
 * @param ansi_after Target ANSI state (packed bits)
 * @return char* Mushcode letters performing the transition (newly allocated)
 */
char *ansi_transition_letters(int ansi_before, int ansi_after)
{
	int ansi_bits_set, ansi_bits_clr;
	char *p;
	char *buffer;
	buffer = XMALLOC(SBUF_SIZE, "buffer");
	*buffer = '\0';

	if (ansi_before != ansi_after)
	{
		p = buffer;
		/*
		 * If they turn off any highlight bits, or they change from some color
		 * * to default color, we need to use ansi normal first.
		 */
		ansi_bits_set = (~ansi_before) & ansi_after;
		ansi_bits_clr = ansi_before & (~ansi_after);

		if ((ansi_bits_clr & 0xf00) || /* highlights off */
			(ansi_bits_set & 0x088) || /* normal to color */
			(ansi_bits_clr == 0x1000))
		{ /* explicit normal */
			*p++ = 'n';
			ansi_bits_set = (~ansiBits(0)) & ansi_after;
			ansi_bits_clr = ansiBits(0) & (~ansi_after);
		}

		/*
		 * Next reproduce the highlight state
		 */

		if (ansi_bits_set & 0x100)
		{
			*p++ = 'h';
		}

		if (ansi_bits_set & 0x200)
		{
			*p++ = 'u';
		}

		if (ansi_bits_set & 0x400)
		{
			*p++ = 'f';
		}

		if (ansi_bits_set & 0x800)
		{
			*p++ = 'i';
		}

		/*
		 * Foreground color
		 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x00f)
		{
			*p++ = ansiMushCode(ansi_after & 0x00f, false);
		}

		/*
		 * Background color
		 */
		if ((ansi_bits_set | ansi_bits_clr) & 0x0f0)
		{
			*p++ = ansiMushCode((ansi_after & 0x0f0) >> 4, true);
		}

		/*
		 * Terminate
		 */
		*p = '\0';
	}

	return buffer;
}

/**
 * @brief Map ANSI state for every character in a string
 *
 * Caller must free `*m` with XFREE(m, "ansi_map_states_ansi_map") and
 * `*p` with XFREE(p, "ansi_map_states_stripped").
 *
 * @param s Input string
 * @param m Out: allocated array of ANSI states per character
 * @param p Out: stripped string without ANSI codes
 * @return int Number of characters mapped
 */
int ansi_map_states(const char *s, int **m, char **p)
{
	int *ansi_map;
	char *stripped;
	const char *s1;
	int n = 0, ansi_state = ANST_NORMAL;
	if (!s)
	{
		ansi_map = (int *)XCALLOC(1, sizeof(int), "ansi_map");
		stripped = XMALLOC(1, "stripped");
		ansi_map[0] = ANST_NORMAL;
		stripped[0] = '\0';
		*m = ansi_map;
		*p = stripped;
		return 0;
	}

	size_t len = strlen(s);
	ansi_map = (int *)XCALLOC(len + 1, sizeof(int), "ansi_map");
	stripped = XMALLOC(len + 1, "stripped");

	s1 = s;
	while (*s1 && (size_t)n < len)
	{
		if (*s1 == ESC_CHAR)
		{
			do
			{
				int ansi_mask = 0;
				int ansi_diff = 0;
				unsigned int param_val = 0;
				++(s1);
				if (!*(s1))
				{
					break;
				}
				if (*(s1) == ANSI_CSI)
				{
					++(s1);
					while (*(s1) && (*(s1) & 0xf0) == 0x30)
					{
						if (*(s1) < 0x3a)
						{
							param_val <<= 1;
							param_val += (param_val << 2) + (*(s1) & 0x0f);
						}
						else
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							param_val = 0;
						}
						++(s1);
					}
				}
				while (*(s1) && (*(s1) & 0xf0) == 0x20)
				{
					++(s1);
				}
				if (*(s1) == ANSI_END)
				{
					if (param_val < I_ANSI_LIM)
					{
						ansi_mask |= ansiBitsMask(param_val);
						ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
					}
					ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
					++(s1);
				}
				else if (*(s1))
				{
					++(s1);
				}
			} while (0);
		}
		else
		{
			ansi_map[n] = ansi_state;
			stripped[n++] = *s1++;
		}
	}

	while (*s1)
	{
		if (*s1 == ESC_CHAR)
		{
			char *mutable = (char *)s1;
			skip_esccode(&mutable);
			s1 = mutable;
		}
		else
		{
			++s1;
		}
	}

	ansi_map[n] = ANST_NORMAL;
	stripped[n] = '\0';
	*m = ansi_map;
	*p = stripped;
	return n;
}

/**
 * @brief Advance pointer past a single ANSI escape sequence
 *
 * @param s Pointer to the string pointer to advance
 */
void skip_esccode(char **s)
{
	++(*s);

	if (!**s)
	{
		return;
	}

	if (**s == ANSI_CSI)
	{
		do
		{
			++(*s);
			if (!**s)
			{
				return;
			}
		} while ((**s & 0xf0) == 0x30);
	}

	while (**s && (**s & 0xf0) == 0x20)
	{
		++(*s);
	}

	if (**s)
	{
		++(*s);
	}
}

/**
 * @brief Copy a single ANSI escape sequence to another buffer
 *
 * @param s Pointer to source string pointer (advanced past sequence)
 * @param t Pointer to destination pointer (advanced after copy)
 */
void copy_esccode(char **s, char **t)
{
	**t = **s;
	++(*s);
	++(*t);

	if (!**s)
	{
		return;
	}

	if (**s == ANSI_CSI)
	{
		do
		{
			**t = **s;
			++(*s);
			++(*t);
			if (!**s)
			{
				return;
			}
		} while ((**s & 0xf0) == 0x30);
	}

	while (**s && (**s & 0xf0) == 0x20)
	{
		**t = **s;
		++(*s);
		++(*t);
	}

	if (**s)
	{
		**t = **s;
		++(*s);
		++(*t);
	}
}

/**
 * @brief Safely copy a single ANSI escape sequence into a bounded buffer
 *
 * @param s Pointer to source string pointer (advanced past sequence)
 * @param buff Destination buffer
 * @param bufc Pointer to current write position within destination buffer
 */
void safe_copy_esccode(char **s, char *buff, char **bufc)
{
	XSAFELBCHR(**s, buff, bufc);
	++(*s);

	if (!**s)
	{
		return;
	}

	if (**s == ANSI_CSI)
	{
		do
		{
			XSAFELBCHR(**s, buff, bufc);
			++(*s);
			if (!**s)
			{
				return;
			}
		} while ((**s & 0xf0) == 0x30);
	}

	while (**s && (**s & 0xf0) == 0x20)
	{
		XSAFELBCHR(**s, buff, bufc);
		++(*s);
	}

	if (**s)
	{
		XSAFELBCHR(**s, buff, bufc);
		++(*s);
	}
}

/**
 * @brief Remap ANSI color indices according to a color map
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param s Input string to remap
 * @param cmap Array of color remappings indexed from I_ANSI_BLACK
 * @return char* Remapped string (newly allocated)
 */
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

	do
	{
		while (*s && (*s != ESC_CHAR))
		{
			XSAFELBCHR(*s, buf, &bp);
			s++;
		}

		if (*s == ESC_CHAR)
		{
			XSAFELBCHR(*s, buf, &bp);
			s++;

			if (*s == ANSI_CSI)
			{
				XSAFELBCHR(*s, buf, &bp);
				s++;

				do
				{
					n = (int)strtol(s, (char **)NULL, 10);

					if ((n >= I_ANSI_BLACK) && (n < I_ANSI_NUM) && (cmap[n - I_ANSI_BLACK] != 0))
					{
						XSAFELTOS(buf, &bp, cmap[n - I_ANSI_BLACK], LBUF_SIZE);

						while (isdigit((unsigned char)*s))
						{
							s++;
						}
					}
					else
					{
						while (isdigit((unsigned char)*s))
						{
							XSAFELBCHR(*s, buf, &bp);
							s++;
						}
					}

					if (*s == ';')
					{
						XSAFELBCHR(*s, buf, &bp);
						s++;
					}
				} while (*s && (*s != ANSI_END));

				if (*s == ANSI_END)
				{
					XSAFELBCHR(*s, buf, &bp);
					s++;
				}
			}
			else if (*s)
			{
				XSAFELBCHR(*s, buf, &bp);
				s++;
			}
		}
	} while (*s);

	return (buf);
}
/**
 * @brief Convert raw ANSI to mushcode or strip ANSI codes
 *
 * Caller is responsible for freeing the returned buffer with XFREE().
 *
 * @param str String to translate (modified as it is consumed)
 * @param type When 1 converts to mushcode, when 0 strips ANSI
 * @return char* Translated string (newly allocated)
 */
char *translate_string(char *str, int type)
{
	char *buff, *bp;
	bp = buff = XMALLOC(LBUF_SIZE, "buff");

	if (type)
	{
		int ansi_state = ANST_NORMAL;
		int ansi_state_prev = ANST_NORMAL;

		while (*str)
		{
			switch (*str)
			{
			case ESC_CHAR:
				while (*str == ESC_CHAR)
				{
					do
					{
						int ansi_mask = 0;
						int ansi_diff = 0;
						unsigned int param_val = 0;
						++(str);
						if (!*(str))
						{
							break;
						}
						if (*(str) == ANSI_CSI)
						{
							++(str);
							while (*(str) && (*(str) & 0xf0) == 0x30)
							{
								if (*(str) < 0x3a)
								{
									param_val <<= 1;
									param_val += (param_val << 2) + (*(str) & 0x0f);
								}
								else
								{
									if (param_val < I_ANSI_LIM)
									{
										ansi_mask |= ansiBitsMask(param_val);
										ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
									}
									param_val = 0;
								}
								++(str);
							}
						}
						while (*(str) && (*(str) & 0xf0) == 0x20)
						{
							++(str);
						}
						if (*(str) == ANSI_END)
						{
							if (param_val < I_ANSI_LIM)
							{
								ansi_mask |= ansiBitsMask(param_val);
								ansi_diff = ((ansi_diff & ~ansiBitsMask(param_val)) | ansiBits(param_val));
							}
							ansi_state = (ansi_state & ~ansi_mask) | ansi_diff;
							++(str);
						}
						else if (*(str))
						{
							++(str);
						}
					} while (0);
				}

				char transition[SBUF_SIZE];
				size_t tlen = ansi_transition_mushcode_into(ansi_state_prev, ansi_state, transition, sizeof transition);
				transition[tlen] = '\0';
				XSAFELBSTR(transition, buff, &bp);
				ansi_state_prev = ansi_state;
				continue;

			case ' ':
				if (str[1] == ' ')
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
				XSAFELBCHR(*str, buff, &bp);
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
				XSAFELBCHR(*str, buff, &bp);
			}

			str++;
		}
	}
	else
	{
		while (*str)
		{
			switch (*str)
			{
			case ESC_CHAR:
				skip_esccode(&str);
				continue;

			case '\r':
				break;

			case '\n':
			case '\t':
				XSAFELBCHR(' ', buff, &bp);
				break;

			default:
				XSAFELBCHR(*str, buff, &bp);
			}

			str++;
		}
	}

	*bp = '\0';
	return buff;
}

/*
 * ---------------------------------------------------------------------------
 * rgb2xterm -- Convert an RGB value to xterm color
 */

/**
 * @brief Convert an RGB value to an xterm color index
 *
 * @param rgb RGB value (0xRRGGBB)
 * @return int xterm color index, or nearest match
 */
int rgb2xterm(long rgb)
{
	int r = (rgb >> 16) & 0xff;
	int g = (rgb >> 8) & 0xff;
	int b = rgb & 0xff;

	static const struct
	{
		long rgb;
		int idx;
	} base16[] = {
		{0x000000, 0}, {0x800000, 1}, {0x008000, 2}, {0x808000, 3},
		{0x000080, 4}, {0x800080, 5}, {0x008080, 6}, {0xc0c0c0, 7},
		{0x808080, 8}, {0xff0000, 9}, {0x00ff00, 10}, {0xffff00, 11},
		{0x0000ff, 12}, {0xff00ff, 13}, {0x00ffff, 14}, {0xffffff, 15},
	};

	for (size_t i = 0; i < sizeof(base16) / sizeof(base16[0]); ++i)
	{
		if (rgb == base16[i].rgb)
		{
			return base16[i].idx;
		}
	}

	if (r == g && r == b)
	{
		if (rgb <= 0x080808)
		{
			return 232;
		}

		int gray = r;
		int idx = 232 + (gray - 8 + 5) / 10; /* nearest match */
		if (idx < 232)
			idx = 232;
		if (idx > 255)
			idx = 255;
		return idx;
	}

	int r_lvl = (r + 25) / 51;
	int g_lvl = (g + 25) / 51;
	int b_lvl = (b + 25) / 51;

	int xterm = 16 + (36 * r_lvl) + (6 * g_lvl) + b_lvl;
	if (xterm < 16)
		xterm = 16;
	if (xterm > 231)
		xterm = 231;
	return xterm;
}

/**
 * @brief Parse a color string into an xterm color index
 *
 * Accepts hex (#rrggbb), decimal triplets (r g b), 24-bit integer, or
 * direct xterm index.
 *
 * @param str Color string to parse
 * @return int xterm color index, or -1 on error
 */
int str2xterm(char *str)
{
	char *p = str;
	char *end = NULL;

	if (*p == '#')
	{
		p++;
		long rgb = strtol(p, &end, 16);
		return (p == end) ? -1 : rgb2xterm(rgb);
	}

	long first = strtol(p, &end, 10);
	if (p == end)
	{
		return -1;
	}

	while (*end && !isdigit((unsigned char)*end))
		end++;

	if (*end == '\0')
	{
		if (first < 256)
			return (int)first;
		return rgb2xterm(first);
	}

	long second = strtol(end, &p, 10);
	if (end == p)
		return -1;
	while (*p && !isdigit((unsigned char)*p))
		p++;
	if (*p == '\0')
		return -1;
	long third = strtol(p, &end, 10);
	if (p == end)
		return -1;

	long rgb = ((first & 0xff) << 16) | ((second & 0xff) << 8) | (third & 0xff);
	return rgb2xterm(rgb);
}