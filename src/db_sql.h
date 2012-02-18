/* db_sql.h */
/* $Id: db_sql.h,v 1.7 2003/02/24 18:05:22 rmg Exp $ */

#include "copyright.h"

#ifndef __DB_SQL_H
#define __DB_SQL_H

/*
 * If we are already connected to the database, do nothing. Otherwise,
 * attempt to connect: return a socket fd if successful, return -1 if failed,
 * stamp a message to the logfile. Select a database. On failure, write to
 * the logfile, close the socket, and return -1. Save the database socket fd
 * as mudstate.sql_socket (which is initialized to -1 at start time).
 *
 */
int		NDECL      (sql_init);

/*
 * Send a query string to the database and obtain a result string (we pass an
 * lbuf and a pointer to the appropriate place in it), with the rows
 * separated by a delimiter and the fields separated by another delimiter. If
 * we encounter an error, set the result string to #-1. If buff is NULL, we
 * are in interactive mode and we simply notify the player of results, rather
 * than writing into the result string. On success, return 0. On failure,
 * return -1.
 */
int
FDECL(sql_query, (dbref, char *, char *, char **,
                  const Delim *, const Delim *));

/*
 * Shut down the database connection.
 */
void		NDECL     (sql_shutdown);

#endif				/* __DB_SQL_H */
