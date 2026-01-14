/**
 * @file boolexp_internal.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Internal declarations shared between boolexp parsing and evaluation modules
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#ifndef __BOOLEXP_INTERNAL_H
#define __BOOLEXP_INTERNAL_H

/**
 * @brief Global recursion depth counter for parsing operations
 *
 * Prevents stack overflow attacks by limiting expression complexity.
 * Shared between parse_depth tracking across all parsing operations.
 */
extern int boolexp_parse_depth;

#endif /* __BOOLEXP_INTERNAL_H */
