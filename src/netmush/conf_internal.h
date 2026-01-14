/**
 * @file conf_internal.h
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief Internal shared declarations for configuration subsystem modules
 * @version 4.0
 *
 * @copyright Copyright (C) 1989-2025 TinyMUSH development team.
 *            You may distribute under the terms the Artistic License,
 *            as specified in the COPYING file.
 *
 */

#pragma once

#include "typedefs.h"

/*
 * Internal helper functions shared between conf_*.c modules
 * Not part of the public API
 */

/**
 * @brief Return command status from success and failure info.
 *
 * Helper function used internally by configuration handlers to convert
 * success/failure counts into a CF_Result status code.
 *
 * @param player    DBref of player
 * @param cmd       Command name
 * @param success   Number of successful operations
 * @param failure   Number of failed operations
 * @return CF_Result (CF_Success, CF_Partial, or CF_Failure)
 */
CF_Result cf_status_from_succfail(dbref player, char *cmd, int success, int failure);
