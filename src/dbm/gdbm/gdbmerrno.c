/* gdbmerrno.c - convert gdbm errors into english. */

/* $id$ */

#include "autoconf.h"

/* this is not static so that applications may access the array if they
   like. it must be in the same order as the error codes! */

const char *const gdbm_errlist[] = {
	"No error", "Malloc error", "Block size error", "File open error",
	"File write error", "File seek error", "File read error",
	"Bad magic number", "Empty database", "Can't be reader", "Can't be writer",
	"Reader can't delete", "Reader can't store", "Reader can't reorganize",
	"Unknown update", "Item not found", "Reorganize failed", "Cannot replace",
	"Illegal data", "Option already set", "Illegal option"
};

#define gdbm_errorcount	((sizeof(gdbm_errlist) / sizeof(gdbm_errlist[0])) - 1)

const char *
gdbm_strerror(error)
	gdbm_error error;
{
	if (((int)error < 0) || ((int)error > gdbm_errorcount)) {
		return ("Unknown error");
	} else {
		return (gdbm_errlist[(int)error]);
	}
}
