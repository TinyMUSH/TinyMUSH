/**
 * @file funstring_ansi.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief ANSI color string operations
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

#include <ctype.h>
#include <string.h>

/*
 * Determine the appropriate color type based on player capabilities.
 *
 * Checks the player's color flags to return the highest supported color type:
 * TrueColor if Color24Bit is set, XTerm if Color256 is set, Ansi if Ansi is set,
 * otherwise None.
 *
 * player: DBref of the player
 * cause: DBref of the cause (used if not NOTHING)
 * return: ColorType The resolved color type
 */
/**
 * @brief Check if two ColorState structures are equal.
 *
 * Compares the memory of two ColorState structs for equality.
 *
 * @param a Pointer to first ColorState
 * @param b Pointer to second ColorState
 * @return bool True if equal, false otherwise
 */
inline bool colorstate_equal(const ColorState *a, const ColorState *b)
{
return memcmp(a, b, sizeof(ColorState)) == 0;
}

/**
 * @brief Append ANSI escape sequence for color transition to buffer.
 *
 * Generates and appends the ANSI escape sequence needed to transition from
 * one color state to another, based on the specified color type. Does nothing
 * if color type is None or states are equal.
 *
 * @param from Pointer to source ColorState
 * @param to Pointer to target ColorState
 * @param type ColorType to use for the sequence
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 */
void append_color_transition(const ColorState *from, const ColorState *to, ColorType type, char *buff, char **bufc)
{
if (type == ColorTypeNone || colorstate_equal(from, to))
{
return;
}

char *seq = ansi_transition_colorstate(*from, *to, type, false);

if (seq)
{
XSAFELBSTR(seq, buff, bufc);
XFREE(seq);
}
}

/**
 * @brief Emit a range of text with appropriate color transitions.
 *
 * Outputs the specified range of text to the buffer, inserting ANSI escape
 * sequences to maintain color state transitions. Handles initial and final
 * state transitions, and skips color handling if type is None.
 *
 * @param text The text string
 * @param states Array of ColorState for each character
 * @param start Start index in the text
 * @param end End index in the text
 * @param initial_state Initial ColorState
 * @param final_state Final ColorState
 * @param type ColorType to use
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 */
void emit_colored_range(const char *text, const ColorState *states, int start, int end, ColorState initial_state, ColorState final_state, ColorType type, char *buff, char **bufc)
{
if (start < 0)
{
start = 0;
}

if (end < start)
{
end = start;
}

if (type == ColorTypeNone)
{
if (text && end > start)
{
XSAFESTRNCAT(buff, bufc, text + start, end - start, LBUF_SIZE);
}
return;
}

ColorState current = initial_state;

if (text && end > start)
{
append_color_transition(&current, &states[start], type, buff, bufc);
current = states[start];

for (int i = start; i < end; ++i)
{
XSAFELBCHR(text[i], buff, bufc);
ColorState next_state = (i + 1 < end) ? states[i + 1] : final_state;
append_color_transition(&current, &next_state, type, buff, bufc);
current = next_state;
}
}
else
{
append_color_transition(&current, &final_state, type, buff, bufc);
}
}

/**
 * @brief Consume an ANSI escape sequence and update ColorState.
 *
 * Parses an ANSI escape sequence at the cursor position and applies it to the
 * provided ColorState. Advances the cursor past the sequence if successful,
 * otherwise advances by one character.
 *
 * @param cursor Pointer to the current position in the string
 * @param state Pointer to ColorState to update
 */
inline void consume_ansi_sequence_state(char **cursor, ColorState *state)
{
const char *ptr = *cursor;

if (ansi_apply_sequence(&ptr, state))
{
*cursor = (char *)ptr;
}
else
{
++(*cursor);
}
}

/**
 * @brief Highlight a string using ANSI terminal effects.
 *
 * @code
 * +colorname
 * #RRGGBB  <#RRGGBB>
 * <RR GG BB>
 * XTERN
 * Old Style
 * @endcode
 *
 * @param buff Output buffer
 * @param bufc Output buffer tracker
 * @param player Executor (fallback for color capability)
 * @param caller Not used
 * @param cause Enactor; color capability is taken from here when available
 * @param fargs Function's arguments
 * @param nfargs Not used
 * @param cargs Not used
 * @param ncargs Not used
 */
void fun_ansi(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
ColorState color_state = {0};
ColorType color_type = ColorTypeAnsi;
dbref color_target = (cause != NOTHING) ? cause : player; // Prefer enactor flags for ANSI selection
char escape_buffer[256];
size_t escape_offset = 0;

// Vérifier si les couleurs ANSI sont activées
if (!mushconf.ansi_colors)
{
XSAFELBSTR(fargs[1], buff, bufc);
return;
}

// Vérifier les arguments
if (!fargs[0] || !*fargs[0] || !fargs[1])
{
XSAFELBSTR(fargs[1], buff, bufc);
return;
}

// Déterminer le type de couleur supporté par le joueur
if (color_target != NOTHING && Color24Bit(color_target))
{
color_type = ColorTypeTrueColor;
}
else if (color_target != NOTHING && Color256(color_target))
{
color_type = ColorTypeXTerm;
}
else if (color_target != NOTHING && Ansi(color_target))
{
color_type = ColorTypeAnsi; // Use basic ANSI instead of XTerm
}
else
{
// Si le joueur n'a pas de préférence, utiliser la configuration par défaut
color_type = ColorTypeNone;
}

// Parser la spécification de couleur
if (!ansi_parse_color_from_string(&color_state, fargs[0], false))
{
// Si le parsing échoue, retourner le texte sans couleur
XSAFELBSTR(fargs[1], buff, bufc);
return;
}

// Générer la séquence d'échappement pour la couleur
to_ansi_escape_sequence(escape_buffer, sizeof(escape_buffer), &escape_offset, &color_state, color_type);
if (escape_offset > 0)
{
XSAFELBSTR(escape_buffer, buff, bufc);
}

// Ajouter le texte
XSAFELBSTR(fargs[1], buff, bufc);

// Générer la séquence de reset
ColorState reset_state = {0};
reset_state.reset = ColorStatusReset;
escape_offset = 0;
to_ansi_escape_sequence(escape_buffer, sizeof(escape_buffer), &escape_offset, &reset_state, color_type);
if (escape_offset > 0)
{
XSAFELBSTR(escape_buffer, buff, bufc);
}
}

void fun_stripansi(char *buff, char **bufc, dbref player, dbref caller, dbref cause, char *fargs[], int nfargs, char *cargs[], int ncargs)
{
ColorSequence sequences;

if (!fargs[0] || !*fargs[0])
{
return;
}

if (ansi_parse_ansi_to_sequences(fargs[0], &sequences))
{
XSAFELBSTR(sequences.text, buff, bufc);
XFREE(sequences.text);
XFREE(sequences.data);
}
else
{
// Fallback: return original string if parsing fails
XSAFELBSTR(fargs[0], buff, bufc);
}
}
