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
 * @brief Context structure for level_ansi dynamic buffer management
 *
 * Encapsulates all state required for the level_ansi() function to manage
 * a dynamically allocated and expanding output buffer during ANSI color
 * conversion. Passed as opaque context (cast to void*) to the buffer
 * management and write callbacks used by convert_color_to_sequence().
 *
 * This structure coordinates between the input processing loop (in level_ansi())
 * and the color conversion machinery (convert_color_to_sequence()), enabling
 * both proactive buffer expansion and sequential writes to accumulate results.
 *
 * Unlike LevelAnsiStreamContext which uses a fixed stack buffer with periodic
 * flushing, LevelAnsiContext manages a dynamically allocated buffer that grows
 * as needed to hold the complete output. This approach trades peak memory usage
 * for simplicity and the ability to return a complete result string.
 *
 * @member p_ptr Pointer to current write position pointer (address of p in level_ansi);
 *               advanced as ANSI codes and text are written; dereferenced to get
 *               actual position within allocated buffer
 * @member end End boundary pointer of allocated buffer (typically buf + buf_size - 1);
 *             used for bounds checking in write operations; updated after reallocation
 * @member buf_size Pointer to current buffer size in bytes (address of buf_size in level_ansi);
 *                  doubled during reallocation; tracks allocated capacity excluding
 *                  space reserved for null terminator
 * @member buf Pointer to allocated buffer start pointer (address of buf in level_ansi);
 *             updated to new allocation during reallocation; passed to XREALLOC
 *
 * @note All members are pointers to variables in the calling level_ansi() function
 * @note Used with level_ansi_check_space() callback for proactive buffer expansion
 * @note Used with level_ansi_write() callback for sequential data accumulation
 * @note Buffer reallocation occurs via XREALLOC when space becomes insufficient
 * @note Initial buffer size is LBUF_SIZE; expands by doubling on each reallocation
 * @note Final null terminator added by level_ansi() after all processing completes
 * @note Pointer indirection enables callbacks to modify caller's local variables
 *
 * @see level_ansi() for initialization and buffer setup
 * @see level_ansi_check_space() for how this context is used in buffer expansion
 * @see level_ansi_write() for how this context is used in write operations
 * @see convert_color_to_sequence() for callback invocation pattern
 * @see LevelAnsiStreamContext for streaming variant with fixed buffer
 */
typedef struct LevelAnsiContext{
    char **p_ptr;
    const char *end;
    size_t *buf_size;
    char **buf;
} LevelAnsiContext;

/**
 * @brief Ensure sufficient space in dynamic buffer for pending write operation
 *
 * Callback function invoked by convert_color_to_sequence() before writing
 * ANSI escape sequences to verify that the dynamically managed output buffer
 * has sufficient remaining capacity. If space is insufficient, automatically
 * reallocates the buffer with doubled size and updates all pointers.
 *
 * This function enables proactive buffer expansion during color conversion,
 * allowing level_ansi() to accumulate output without premature truncation.
 * The callback pattern decouples buffer management from the conversion logic.
 *
 * Behavior:
 * 1. Casts context pointer to LevelAnsiContext structure
 * 2. Calculates current buffer usage as bytes written (position - start)
 * 3. Checks if usage + needed bytes would exceed buffer capacity
 *    (condition reserves 1 byte for null terminator: buf_size - 1)
 * 4. If space insufficient:
 *    - Doubles buffer size via *buf_size *= 2
 *    - Reallocates memory via XREALLOC (in-place when possible)
 *    - Updates write position pointer to offset within new buffer
 *    - Updates end boundary pointer to new buffer end
 * 5. If space sufficient: returns immediately without allocation
 *
 * @param needed Number of bytes required for the pending write operation
 * @param ctx LevelAnsiContext pointer cast as void*; must not be NULL
 *
 * @note Context must point to valid LevelAnsiContext allocated in level_ansi()
 * @note Pointer updates ensure consistency across buffer reallocation
 * @note If existing usage + needed >= size - 1, reallocation ALWAYS occurs
 * @note Doubles buffer size on each reallocation (amortized cost strategy)
 * @note Reserving buf_size - 1 ensures space for final null terminator
 * @note Called via convert_color_to_sequence() callback before write_fn
 * @note Paired with level_ansi_write() callback for coordinated buffer access
 * @note This function is callback for convert_color_to_sequence check_space_fn param
 *
 * @see level_ansi() for setup and buffer initialization
 * @see level_ansi_write() for writing callback (called after space is ensured)
 * @see convert_color_to_sequence() for caller context
 * @see LevelAnsiContext for structure details
 * @see ensure_buffer_space() for similar manual buffer expansion logic
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
 * @brief Write converted ANSI data to dynamically expanding buffer
 *
 * Callback function used by convert_color_to_sequence() to write generated
 * ANSI escape sequences to the dynamically managed output buffer used by
 * level_ansi(). Implements bounded memory copy with automatic buffer expansion
 * when space is insufficient.
 *
 * Behavior:
 * 1. Casts context pointer to LevelAnsiContext structure
 * 2. Calculates safe copy length (limited by available buffer space)
 * 3. Copies data bytes to current write position via XMEMCPY
 * 4. Advances write position pointer by bytes copied
 * 5. Truncates silently if copy length exceeds available space (no reallocation)
 *
 * Unlike level_ansi_stream_write(), this function does NOT trigger flushing
 * because the entire output is buffered in memory. Buffer expansion is handled
 * by the caller (level_ansi()) via level_ansi_check_space() before writes.
 *
 * @param data Pointer to source buffer with ANSI codes to write (not null-terminated)
 * @param len Number of bytes to write from data buffer
 * @param ctx LevelAnsiContext pointer cast as void*; must not be NULL
 *
 * @note Context maintains pointers to dynamically allocated buffer
 * @note Bounds checking uses end pointer; truncates silently if insufficient space
 * @note No null termination added; caller responsible for final termination
 * @note Called via convert_color_to_sequence() callback mechanism
 * @note Paired with level_ansi_check_space() for proactive buffer management
 *
 * @see level_ansi() for usage context and buffer setup
 * @see level_ansi_check_space() for proactive buffer expansion
 * @see convert_color_to_sequence() for caller context
 * @see level_ansi_stream_write() for streaming variant with flushing
 */
static void level_ansi_write(const char *data, size_t len, void *ctx)
{
    LevelAnsiContext *context = (LevelAnsiContext *)ctx;
    size_t copy_len = (len < (size_t)(context->end - *context->p_ptr)) ? len : (context->end - *context->p_ptr);
    XMEMCPY(*context->p_ptr, data, copy_len);
    *context->p_ptr += copy_len;
}

/**
 * @brief Context structure for level_ansi_stream buffer and callback management
 *
 * Encapsulates all state required for streaming ANSI conversion with automatic
 * flushing. Passed as opaque context (cast to void*) to the write and check-space
 * callbacks used by convert_color_to_sequence().
 *
 * This structure bridges between the streaming input processing loop and the
 * color conversion machinery, enabling coordination of buffer management and
 * progressive output delivery.
 *
 * @member buf Fixed buffer pointer (8KB buffer created in level_ansi_stream stack)
 * @member p_ptr Pointer to current write position within buf; advanced as data written
 * @member end End boundary pointer of buf (buf + 8191); used for bounds checking
 * @member flush_threshold Buffer usage threshold in bytes (~6553 for 80% of 8KB);
 *                         when write position exceeds this, flush_fn is invoked
 * @member flush_fn Callback function to invoke when buffer reaches threshold;
 *                  responsible for sending flushed data to final destination
 *                  (network socket, file descriptor, message queue, etc.)
 * @member flush_ctx Opaque context pointer passed through to flush_fn;
 *                   typically queue descriptor or player connection handle
 *
 * @note Used as context parameter in level_ansi_stream_write() callback
 * @note Reused across multiple write operations during single input processing
 * @note Buffer is NOT owned by this structure; allocated on stack in level_ansi_stream()
 * @note Caller must ensure flush_fn is a valid function pointer
 * @note Caller must ensure flush_ctx is valid for the duration of processing
 *
 * @see level_ansi_stream() for structure initialization and buffer setup
 * @see level_ansi_stream_write() for how callbacks use this context
 * @see convert_color_to_sequence() for how context is passed through callbacks
 */
typedef struct LevelAnsiStreamContext{
    char *buf;
    char **p_ptr;
    const char *end;
    size_t flush_threshold;
    void (*flush_fn)(const char *data, size_t len, void *ctx);
    void *flush_ctx;
} LevelAnsiStreamContext;

/**
 * @brief Check streaming buffer usage and flush to callback when threshold reached
 *
 * Helper function used within level_ansi_stream() to implement threshold-based
 * flushing. Monitors the streaming buffer's current usage and automatically
 * triggers output when the data reaches a configurable percentage of capacity.
 *
 * This enables progressive output without waiting for the entire input to be
 * processed, reducing peak memory usage and enabling real-time network delivery.
 *
 * Behavior:
 * 1. Calculates buffer usage as distance between current position and buffer start
 * 2. Compares usage against flush_threshold (typically 80% of 8KB buffer ≈ 6.5KB)
 * 3. If threshold NOT exceeded: returns without action (data remains in buffer)
 * 4. If threshold exceeded:
 *    - Adds null terminator at current position
 *    - Invokes flush_fn callback with buffer content and byte count
 *    - Resets write position to buffer start for next batch
 *
 * The flush callback (typically queue_write or similar) is responsible for
 * sending the flushed data to its final destination (socket, file, queue, etc.).
 *
 * @param p_ptr Pointer to current write position pointer (will be reset on flush)
 * @param buf Start of the fixed streaming buffer (typically 8KB)
 * @param flush_threshold Buffer usage threshold in bytes (e.g., 6553 for 80% of 8KB)
 * @param flush_fn Callback function to invoke when threshold is exceeded
 *                 Signature: void callback(const char *data, size_t len, void *ctx)
 *                 The data parameter points to null-terminated buffer content
 * @param ctx Opaque context pointer passed to flush_fn callback
 *
 * @note Buffer is NOT null-terminated normally; terminator added only at flush time
 * @note write position is reset to buffer start after flush to reuse buffer
 * @note Pending data is lost after flush; caller must not reference old buffer area
 * @note Used by level_ansi_stream() for both plain text and escape sequences
 * @note Threshold typically set to 80% of buffer size to avoid overflow
 *
 * @see level_ansi_stream() for usage context and buffer setup
 * @see level_ansi_stream_write() for similar flushing in callback context
 * @see flush_fn signature for understanding callback requirements
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
 * @brief Write converted ANSI data to streaming buffer with auto-flush
 *
 * Callback function used by convert_color_to_sequence() to write generated
 * ANSI escape sequences to the streaming buffer. Implements a simple write
 * operation with automatic flush when the buffer reaches a configurable
 * threshold, enabling progressive output without allocating the full result.
 *
 * Behavior:
 * 1. Casts context pointer to LevelAnsiStreamContext structure
 * 2. Calculates safe copy length (limited by available buffer space)
 * 3. Copies data bytes to current write position via XMEMCPY
 * 4. Advances write position pointer by bytes copied
 * 5. Checks if buffer usage exceeds flush_threshold (80% of 8KB ≈ 6.5KB)
 * 6. If threshold exceeded: null-terminates, calls flush callback, resets position
 * 7. Otherwise: leaves data in buffer for next call
 *
 * This function does NOT allocate memory; it uses the fixed 8KB buffer from
 * level_ansi_stream(). The flush callback (typically queue_write or similar)
 * is responsible for sending data to the appropriate destination.
 *
 * @param data Pointer to source buffer with ANSI codes to write (not null-terminated)
 * @param len Number of bytes to write from data buffer
 * @param ctx LevelAnsiStreamContext pointer cast as void*; must not be NULL
 *
 * @note Context is reused across multiple calls within level_ansi_stream()
 * @note Buffer is NOT null-terminated until flush occurs
 * @note Truncation occurs silently if len exceeds available space
 * @note Flush resets position to buffer start, losing any pending data
 * @note Called via convert_color_to_sequence() callback mechanism
 *
 * @see level_ansi_stream() for setup and flush_fn callback definition
 * @see convert_color_to_sequence() for caller context
 * @see flush_if_needed() for similar flushing logic
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

/**
 * @brief Dynamically expand buffer when needed to accommodate new data
 *
 * This inline utility function checks if there is sufficient space in the
 * output buffer to hold the requested amount of data. If the buffer is
 * insufficient, it doubles the buffer size and reallocates memory, then
 * updates all related pointers to maintain consistency.
 *
 * Buffer management:
 * - Tracks usage as bytes written (difference between current position and start)
 * - Ensures at least 1 byte remains for null terminator
 * - Doubles buffer size on reallocation to amortize allocation cost
 * - Updates position pointer to offset into new buffer
 * - Updates end pointer to new boundary
 * - Updates size variable for subsequent checks
 *
 * This function is used by level_ansi() and normal_to_white() for dynamic
 * buffer growth when processing ANSI strings of unknown final size.
 *
 * @param needed Number of bytes required for next write operation
 * @param buf Pointer to buffer pointer (will be updated on reallocation)
 * @param p Pointer to current write position (will be updated on reallocation)
 * @param end Pointer to buffer end boundary (will be updated on reallocation)
 * @param buf_size Pointer to current buffer size (will be doubled on reallocation)
 *
 * @note All pointer parameters must be valid; buf_size should point to actual
 *       allocated size, not including null terminator space
 * @note After reallocation, original pointers are invalid; use updated values
 * @note Assumes XREALLOC will not fail (caller must handle allocation failures)
 * @note Condition reserves 1 byte minimum for null terminator (buf_size - 1)
 *
 * @see level_ansi() for usage with ColorState conversion
 * @see normal_to_white() for usage with ANSI reset replacement
 */
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
 * @brief Convert ANSI codes to match the player's color capability level
 *
 * Processes an ANSI-encoded string and down-converts color codes based on the
 * player's terminal capabilities. If the player supports truecolor, sequences
 * are passed through unchanged. Otherwise, the function parses each ANSI escape
 * sequence and regenerates appropriate output using the ColorState API.
 *
 * Capability levels (from highest to lowest):
 * - truecolor=true: 24-bit RGB colors (ESC[38;2;R;G;Bm) - pass through as-is
 * - xterm=true: 256-color extended palette (ESC[38;5;Nm)
 * - ansi=true: 8-color basic ANSI codes (ESC[30-37m, ESC[90-97m for bright)
 * - all false: Strip ANSI codes entirely (returns text only)
 *
 * Algorithm:
 * 1. Allocates output buffer (initially LBUF_SIZE, expands as needed)
 * 2. Scans input for ESC character markers
 * 3. If truecolors: copies escape sequences verbatim
 * 4. Otherwise: parses sequence into ColorState using ansi_parse_sequence()
 * 5. Calls convert_color_to_sequence() to generate appropriate output format
 * 6. Uses level_ansi_check_space() and level_ansi_write() callbacks
 * 7. Returns complete output in newly allocated buffer
 *
 * @param s Input string to convert (may be NULL or empty)
 * @param ansi Player supports basic ANSI colors (codes 30-37, 90-97)
 * @param xterm Player supports extended xterm 256-color palette
 * @param truecolors Player supports 24-bit RGB truecolor sequences
 *
 * @return char* Newly allocated string with ANSI codes converted to player capability
 *               Caller must free with XFREE()
 *               Returns empty string if input is NULL or empty
 *
 * @note Buffer is dynamically allocated; initial size is LBUF_SIZE
 * @note Buffer expands (doubles) as needed via ensure_buffer_space()
 * @note If no flags are true, returns string with all ANSI codes removed
 * @note Uses ColorState internal representation for parsing/generation
 *
 * @see level_ansi_stream() for streaming variant with fixed buffer
 * @see convert_color_to_sequence() for color state to sequence conversion
 * @see ansi_parse_sequence() for sequence parsing logic
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
					*p++ = *s_start++;
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
			if (p < end)
				*p++ = *s++;
			else
				s++;
		}
	}

	*p = '\0';
	return buf;
}

/**
 * @brief Convert ANSI codes based on player capabilities with streaming output
 *
 * Processes an ANSI-encoded string and converts color codes based on the player's
 * capabilities (ANSI, xterm, or truecolor support). Unlike level_ansi() which
 * allocates the full output buffer in memory, this function uses a streaming
 * approach with a fixed internal buffer (8192 bytes) and periodically flushes
 * data to the provided callback function.
 *
 * The streaming approach is optimized for:
 * - Reducing peak memory usage for large strings
 * - Handling real-time output without buffering entire messages
 * - Progressive output to network sockets or file descriptors
 *
 * Algorithm:
 * 1. Scans input string for ANSI escape sequences (ESC char)
 * 2. If truecolors: passes through sequences unchanged
 * 3. Otherwise: parses each sequence into ColorState using ansi_parse_sequence()
 * 4. Calls convert_color_to_sequence() to generate appropriate output
 * 5. Writes output to internal buffer with size limit 8KB
 * 6. Flushes buffer at 80% capacity via flush_fn callback
 * 7. Sends final data chunk when processing completes
 *
 * @param s Input string to process (may be NULL or empty)
 * @param ansi Player supports basic ANSI colors (0-7)
 * @param xterm Player supports extended xterm colors (0-255)
 * @param truecolors Player supports 24-bit RGB truecolor sequences
 * @param flush_fn Callback function invoked when buffer reaches flush threshold
 *                 Signature: void callback(const char *data, size_t len, void *ctx)
 *                 The data parameter points to null-terminated buffer content
 * @param ctx Opaque context pointer passed to flush_fn callback
 *
 * @note If input is NULL or empty, function returns immediately without callbacks
 * @note Internal buffer is 8KB fixed size; flush_fn must handle chunks of up to 6.4KB
 * @note Flush threshold is 80% of buffer size (~6553 bytes)
 * @note This function does NOT allocate output; all data flows through flush_fn
 * @note Must pair with appropriate flush_fn that writes to socket/file/queue
 *
 * @see level_ansi() for non-streaming variant with full output buffering
 * @see convert_color_to_sequence() for color state conversion logic
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

/**
 * @brief Implement the NOBLEED flag by translating ANSI normal to white
 *
 * Processes an ANSI-encoded string and replaces ANSI reset sequences (ESC[0m)
 * with white foreground color (ESC[37m) to prevent color "bleeding" across
 * message boundaries when the NOBLEED flag is set on a player.
 *
 * Uses the modern ColorState API:
 * - Parses each ANSI escape sequence using ansi_parse_sequence()
 * - Detects reset status via ColorStatusReset flag
 * - Replaces reset with white foreground ColorState
 * - Generates replacement sequences using to_ansi_escape_sequence()
 * - Preserves other ANSI attributes (background, underline, etc.)
 *
 * The function handles buffer management and automatically expands the output
 * buffer as needed using ensure_buffer_space().
 *
 * @param raw Input string to process (may be NULL or empty)
 * @return char* Newly allocated string with ANSI resets converted to white
 *               Caller must free with XFREE()
 *
 * @note If input is NULL or empty, returns an allocated empty string
 * @note Output buffer is dynamically allocated and sized to LBUF_SIZE initially
 */
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
			/* Copy text before this escape sequence */
			size_t text_len = s - last_pos;
			if (text_len > 0)
			{
				ensure_buffer_space(text_len, &buf, &p, &p + buf_size - 1, &buf_size);
				XMEMCPY(p, last_pos, text_len);
				p += text_len;
			}
			
			const char *seq_start = s;
			ColorState state = ansi_parse_sequence(&s);
			
			/* If this sequence contains a reset, replace it with white foreground */
			if (state.reset == ColorStatusReset)
			{
				/* Create a modified state with white foreground instead of reset */
				ColorState white_state = {0};
				white_state.foreground.is_set = ColorStatusSet;
				white_state.foreground.ansi_index = 7;  /* White */
				white_state.foreground.xterm_index = 7;
				white_state.foreground.truecolor = (ColorRGB){255, 255, 255};
				
				/* Copy other attributes if they were set */
				white_state.background = state.background;
				white_state.highlight = state.highlight;
				white_state.underline = state.underline;
				white_state.flash = state.flash;
				white_state.inverse = state.inverse;
				
				/* Generate the replacement sequence */
				char temp_buf[128];
				size_t offset = 0;
				to_ansi_escape_sequence(temp_buf, sizeof(temp_buf), &offset, &white_state, ColorTypeAnsi);
				
				ensure_buffer_space(offset, &buf, &p, &p + buf_size - 1, &buf_size);
				XMEMCPY(p, temp_buf, offset);
				p += offset;
			}
			else
			{
				/* Copy the original sequence as-is */
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
	
	/* Copy any remaining text */
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

/**
 * @brief Advance pointer past a single ANSI escape sequence
 *
 * Optimized to use a local pointer to minimize dereferences.
 *
 * @param s Pointer to the string pointer to advance
 */
void skip_esccode(char **s)
{
	char *p = *s + 1;  /* Skip ESC */

	if (!*p)
	{
		*s = p;
		return;
	}

	if (*p == ANSI_CSI)  /* '[' */
	{
		/* Skip parameter digits (0x30-0x3f) */
		while (*++p && (*p & 0xf0) == 0x30)
			;
	}

	/* Skip intermediate bytes (0x20-0x2f) */
	while (*p && (*p & 0xf0) == 0x20)
	{
		++p;
	}

	/* Skip final byte */
	if (*p)
	{
		++p;
	}

	*s = p;
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