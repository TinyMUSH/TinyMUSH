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
 * @brief Flush streaming buffer when the threshold is reached
 */
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

/**
 * @brief Check buffer space for level_ansi_stream (no-op, buffer is fixed)
 */
static void level_ansi_stream_check_space(size_t needed, void *ctx)
{
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

	p = buf = XMALLOC(buf_size, "buf");
	end = buf + buf_size - 1;

	if (!s || !*s)
		return buf;

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

				ensure_buffer_space((size_t)(s - s_start + 1), &buf, &p, &end, &buf_size);

				while (s_start < s && *s_start && p < end)
				{
					append_ch(&p, end, *s_start++);
				}
			}
			else
			{
				ColorState attr = ansi_parse_sequence(&s);

				LevelAnsiContext ctx = {&p, end, &buf_size, &buf};
				convert_color_to_sequence(&attr, ansi, xterm, level_ansi_check_space, level_ansi_write, &ctx);

				p = *ctx.p_ptr;
				end = (char *)ctx.end;
				buf_size = *ctx.buf_size;
			}
		}
		else
		{
			ensure_buffer_space(1, &buf, &p, &end, &buf_size);
			append_ch(&p, end, *s++);
		}
	}

	*p = '\0';
	return buf;
}

/**
 * @brief Convert ANSI codes based on player capabilities with streaming output
 */
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

				while (s_start < s && *s_start && p < end)
				{
					*p++ = *s_start++;
				}
				flush_if_needed(&p, buf, flush_threshold, flush_fn, ctx);
			}
			else
			{
				ColorState attr = ansi_parse_sequence(&s);

				LevelAnsiStreamContext stream_ctx = {buf, &p, end, flush_threshold, flush_fn, ctx};
				convert_color_to_sequence(&attr, ansi, xterm, level_ansi_stream_check_space, level_ansi_stream_write, &stream_ctx);

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
				char *escape_start = p;
				skip_esccode(&p);
				for (char *seq = escape_start; seq < p; ++seq)
				{
					XSAFELBCHR(*seq, buf, &q);
				}
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