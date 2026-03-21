/**
 * @file eval_gender.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Gender resolution and pronoun substitution for the expression evaluator
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 * Handles the pronoun %-substitutions (%S/%O/%P/%A and their upper-case
 * variants) that the expression evaluator expands. The two public entry
 * points are:
 *
 *  - get_gender()              – map a player's A_SEX attribute to a numeric
 *                                gender code used as an index into the pronoun
 *                                tables.
 *  - _eval_gender_emit_pronoun() – write the appropriate pronoun string for a
 *                                  given substitution code and cause dbref.
 *                                  Module-internal: called only from
 *                                  eval_expression_string() in eval.c.
 */

#include "config.h"

#include "constants.h"
#include "typedefs.h"
#include "macros.h"
#include "externs.h"
#include "prototypes.h"

#include <ctype.h>
#include <string.h>

/**
 * @brief Lookup table for pronoun substitutions indexed by [pronoun_type][gender-1]
 *
 * Pronoun types: 0='o' (object), 1='p' (possessive), 2='s' (subject), 3='a' (absolute)
 * Gender values: 0=neuter/it, 1=feminine/she, 2=masculine/he, 3=plural/they
 */
static const char *pronoun_table[4][4] = {
	/* Object pronouns (o): it, her, him, them */
	{"it", "her", "him", "them"},
	/* Possessive adjectives (p): its, her, his, their */
	{"its", "her", "his", "their"},
	/* Subject pronouns (s): it, she, he, they */
	{"it", "she", "he", "they"},
	/* Absolute possessives (a): its, hers, his, theirs */
	{"its", "hers", "his", "theirs"}
};

/**
 * @brief Resolve a player's gender flag into the internal pronoun code.
 *
 * Looks up the A_SEX attribute and maps its first character to the pronoun
 * set used by %S/%O/%P/%A substitutions: 1=neuter/it, 2=feminine/she,
 * 3=masculine/he, 4=plural/they. Any unknown or missing value returns the
 * neutral form.
 *
 * @param player	DBref of the player to inspect
 * @return int	Pronoun code 1-4 as described above
 */
int get_gender(dbref player)
{
	dbref aowner = NOTHING;
	int aflags = 0, alen = 0;
	char *atr_gotten = atr_pget(player, A_SEX, &aowner, &aflags, &alen);

	/* Handle NULL or empty attribute safely */
	if (!atr_gotten || !*atr_gotten)
	{
		if (atr_gotten)
		{
			XFREE(atr_gotten);
		}
		return 1; /* default neuter */
	}

	char first = tolower(*atr_gotten);
	XFREE(atr_gotten);

	switch (first)
	{
	case 'p': /* plural */
		return 4;

	case 'm': /* masculine */
		return 3;

	case 'f': /* feminine */
	case 'w':
		return 2;

	default: /* neuter/unknown */
		return 1;
	}
}

/**
 * @brief Emit pronoun substitution for %O/%P/%S/%A (and lowercase variants).
 *
 * Uses the pronoun_table lookup table instead of nested switches for O(1)
 * performance with better cache locality. Centralizes gender lookup and
 * output logic. Called only from eval_expression_string() in eval.c.
 *
 * @param code    Pronoun code ('o', 'p', 's', 'a' – case insensitive)
 * @param gender  Pointer to cached gender (lazy-loaded from cause if < 0)
 * @param cause   DBref to get name/gender from
 * @param buff    Output buffer start
 * @param bufc    Output buffer cursor
 */
void _eval_gender_emit_pronoun(char code, int *gender, dbref cause, char *buff, char **bufc)
{
	code = tolower(code);

	if (*gender < 0)
	{
		*gender = get_gender(cause);
	}

	if (!*gender) /* Non-player, use name as fallback */
	{
		safe_name(cause, buff, bufc);
		if (code == 'p' || code == 'a')
		{
			XSAFELBCHR('s', buff, bufc);
		}
		return;
	}

	/* Map pronoun code to table index */
	int pronoun_idx = -1;
	switch (code)
	{
	case 'o': pronoun_idx = 0; break;
	case 'p': pronoun_idx = 1; break;
	case 's': pronoun_idx = 2; break;
	case 'a': pronoun_idx = 3; break;
	default: return;
	}

	/* Bounds check gender (should be 1-4) */
	if (*gender < 1 || *gender > 4)
	{
		return;
	}

	/* Lookup and output pronoun string */
	XSAFELBSTR(pronoun_table[pronoun_idx][*gender - 1], buff, bufc);
}
