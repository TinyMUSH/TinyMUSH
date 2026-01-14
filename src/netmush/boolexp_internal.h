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
 * @brief Error message constants for boolean expression operations
 *
 * These constants define error messages used throughout the boolean expression
 * system for logging and debugging purposes. Each message corresponds to a
 * specific error condition that can occur during parsing or evaluation.
 */
#define ERR_BOOLEXP_ATR_NULL "ERROR: boolexp.c BOOLEXP_ATR has NULL sub1\n"          /*!< BOOLEXP_ATR node has null sub1 pointer */
#define ERR_BOOLEXP_EVAL_NULL "ERROR: boolexp.c BOOLEXP_EVAL has NULL sub1\n"       /*!< BOOLEXP_EVAL node has null sub1 pointer */
#define ERR_BOOLEXP_IS_NULL "ERROR: boolexp.c BOOLEXP_IS attribute check has NULL sub1->sub1\n"     /*!< BOOLEXP_IS node has invalid sub1->sub1 */
#define ERR_BOOLEXP_CARRY_NULL "ERROR: boolexp.c BOOLEXP_CARRY attribute check has NULL sub1->sub1\n" /*!< BOOLEXP_CARRY node has invalid sub1->sub1 */
#define ERR_BOOLEXP_UNKNOWN_TYPE "ABORT! boolexp.c, unknown boolexp type in eval_boolexp().\n"       /*!< Unknown BOOLEXP type encountered */
#define ERR_ATTR_NUM_OVERFLOW "ERROR: boolexp.c attribute number overflow or invalid\n"             /*!< Attribute number out of valid range */
#define ERR_PARSE_DEPTH_EXCEEDED "ERROR: boolexp.c parse depth exceeded limit\n"                    /*!< Recursion depth limit exceeded during parsing */

/**
 * @brief Global recursion depth counter for parsing operations
 *
 * Prevents stack overflow attacks by limiting expression complexity.
 * Shared between parse_depth tracking across all parsing operations.
 */
extern int boolexp_parse_depth;

#endif /* __BOOLEXP_INTERNAL_H */
