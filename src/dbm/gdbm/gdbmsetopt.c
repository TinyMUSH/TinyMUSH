/* gdbmsetopt.c - set options pertaining to a GDBM descriptor. */

#include "gdbmsystems.h"

/* operate on an already open descriptor. */

/* ARGSUSED */
int
gdbm_setopt(dbf, optflag, optval, optlen)
	gdbm_file_info *dbf;		/* descriptor to operate on. */
	int optflag;			/* option to set. */
	int *optval;			/* pointer to option value. */
	int optlen;			/* size of optval. */
{
	switch (optflag) {
	case GDBM_CACHESIZE:
		/* Optval will point to the new size of the cache. */
		if (dbf->bucket_cache != NULL) {
			gdbm_errno = GDBM_OPT_ALREADY_SET;
			return (-1);
		}
		return (_gdbm_init_cache(dbf, ((*optval) > 9) ? (*optval) : 10));

	case GDBM_FASTMODE:
		/* Obsolete form of SYNCMODE. */
		if ((*optval != TRUE) && (*optval != FALSE)) {
			gdbm_errno = GDBM_OPT_ILLEGAL;
			return (-1);
		}
		dbf->fast_write = *optval;
		break;

	case GDBM_SYNCMODE:
		/* Optval will point to either true or false. */
		if ((*optval != TRUE) && (*optval != FALSE)) {
			gdbm_errno = GDBM_OPT_ILLEGAL;
			return (-1);
		}
		dbf->fast_write = !(*optval);
		break;

	case GDBM_CENTFREE:
		/* Optval will point to either true or false. */
		if ((*optval != TRUE) && (*optval != FALSE)) {
			gdbm_errno = GDBM_OPT_ILLEGAL;
			return (-1);
		}
		dbf->central_free = *optval;
		break;

	case GDBM_COALESCEBLKS:
		/* Optval will point to either true or false. */
		if ((*optval != TRUE) && (*optval != FALSE)) {
			gdbm_errno = GDBM_OPT_ILLEGAL;
			return (-1);
		}
		dbf->coalesce_blocks = *optval;
		break;

	default:
		gdbm_errno = GDBM_OPT_ILLEGAL;
		return (-1);
	}

	return (0);
}
