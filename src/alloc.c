/* alloc.c - memory allocation subsystem */

#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"  /* required by mushconf */
#include "game.h"	   /* required by mushconf */
#include "alloc.h"	   /* required by mushconf */
#include "flags.h"	   /* required by mushconf */
#include "htab.h"	   /* required by mushconf */
#include "ltdl.h"	   /* required by mushconf */
#include "udb.h"	   /* required by mushconf */
#include "udb_defs.h"  /* required by mushconf */
#include "mushconf.h"  /* required by code */
#include "db.h"		   /* required by externs.h */
#include "interface.h" /* required by code */
#include "externs.h"   /* required by code */

void list_bufstats(dbref player)
{
	notify(player, "This feature has been removed.");
}

void list_buftrace(dbref player)
{
	notify(player, "This feature has been removed.");
}

/**
 * Allocation functions
 */

/**
 * @brief Tracked version of malloc().
 * 
 * The __xmalloc() function allocates size bytes and returns a pointer to the allocated memory.
 * The memory is set to zero.  If size is 0, then malloc() returns NULL.
 * 
 * Do not call this directly, use the wrapper macro XMALLOC(size_t size, const char *var) that
 * will fill most of the banks for you.
 * 
 * @param size     Size of memory to allocate.
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 * @return void*   Pointer to the allocated buffer.
 */
void *__xmalloc(size_t size, const char *file, int line, const char *function, const char *var)
{
	void *r = NULL;

	if (size > 0)
	{
		if (mudconf.malloc_logger)
		{
			log_write(LOG_MALLOC, "MEM", "TRACE", "XMALLOC: %s/%d", var, size);
		}

		r = calloc(size, sizeof(char));
		__xalloctrace(size, r, file, line, function, var);
	}

	return r;
}

/**
 * @brief Tracked version of calloc().
 * 
 * The __xcalloc() function allocates memory for an array of nmemb elements of size bytes each and
 * returns a pointer to the allocated memory. The memory is set to zero.  If nmemb or size is 0, then
 * calloc() returns NULL. If the multiplication of nmemb and size would result in integer overflow, 
 * then __xcalloc() returns an error.
 * 
 * Do not call this directly, use the wrapper macro XCALLOC(size_t nmemb, size_t size, const char *var)
 * that will fill most of the banks for you.
 * 
 * @param nmemb    Number of elements.
 * @param size     Size of each elements.
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 * @return void*   Pointer to the allocated buffer.
 */
void *__xcalloc(size_t nmemb, size_t size, const char *file, int line, const char *function, const char *var)
{
	void *r = NULL;

	if ((nmemb * size) > 0)
	{
		if (mudconf.malloc_logger)
		{
			log_write(LOG_MALLOC, "MEM", "TRACE", "XCALLOC: %s/%d", var, nmemb * size);
		}

		r = calloc(nmemb, size);
		__xalloctrace(nmemb * size, r, file, line, function, var);
	}

	return r;
}

/**
 * @brief Tracked version of realloc().
 * 
 * The __xrealloc() function changes the size of the memory block pointed to by ptr to size bytes.  The
 * contents will be unchanged in the range from the start of the region up to the minimum of the old and
 * new sizes.  If the new size is larger than the old size, the added memory  will not be initialized.
 * If ptr is NULL, then the call is equivalent to malloc(size), for all values of size; if size is equal
 * to zero, and ptr is not NULL, then the call is equivalent to free(ptr).  Unless ptr is NULL, it must
 * have been returned by an earlier call to malloc(), calloc(), or realloc().  If the area pointed to was
 * moved, a free(ptr) is done.
 * 
 * Do not call this directly, use the wrapper macro XREALLOC(void *ptr, size_t size, const char *var)
 * that will fill most of the banks for you.
 * 
 * @param ptr      Pointer to the allocation to be resize.
 * @param size     Size of memory to allocate.
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 * @return void*   Pointer to the allocated buffer.
 */
void *__xrealloc(void *ptr, size_t size, const char *file, int line, const char *function, const char *var)
{
	void *r = NULL;

	if (size > 0)
	{
		if (mudconf.malloc_logger)
		{
			log_write(LOG_MALLOC, "MEM", "TRACE", "XREALLOC: %s/%d", var, size);
		}

		__xfreetrace(ptr);
		r = realloc(ptr, size);
		__xalloctrace(size, r, file, line, function, var);
	}
	else
	{
		__xfreetrace(ptr);
		free(ptr);
		ptr = NULL;
	}

	return r;
}

/**
 * @brief Tracked version of free().
 * 
 * The __xfree() function frees the memory space pointed to by ptr, which must have been returned
 * by a previous call to malloc()/__xmalloc(), calloc()/__xcalloc(), or realloc()/__xrealloc().
 * Otherwise, or if __xfree(ptr) has already been called before, undefined behavior occurs.
 * If ptr is NULL, no operation is performed.
 * 
 * The wrapper macro XFREE(void *ptr) is identical to calling this function directly. Just keep
 * things more redeable.
 * 
 * @param ptr Pointer to the allocation to be freed.
 */
void __xfree(void *ptr)
{
	if (ptr != NULL)
	{
		__xfreetrace(ptr);
		free(ptr);
		ptr = NULL;
	}
}

/******************************************************************************
 * Tracking functions
 */

/**
 * @brief Add a buffer to the tracking list.
 * 
 * We maintain an unsorted list, most recently-allocated things at the head, based on the belief that 
 * it's basically a stack -- when we go to free something it's usually the most recent thing that we've
 * allocated.
 * 
 * @param size     Size of allocated memory.
 * @param ptr      Pointer to the allocated memory.
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 */
void __xalloctrace(size_t size, void *ptr, const char *file, int line, const char *function, const char *var)
{
	if (var)
	{
		MEMTRACK *tptr;
		tptr = (MEMTRACK *)malloc(sizeof(MEMTRACK));

		if (tptr)
		{
			if (mudconf.malloc_logger)
			{
				log_write(LOG_MALLOC, "MEM", "TRACE", "Alloc: %s (%d bytes)", tptr->var, tptr->size);
			}

			tptr->bptr = ptr;
			tptr->file = file;
			tptr->line = line;
			tptr->function = function;
			tptr->var = var;
			tptr->size = size;
			tptr->next = mudstate.raw_allocs;
			mudstate.raw_allocs = tptr;
		}
	}
}

/**
 * @brief Add a buffer from the tracking list.
 * 
 * Remove a tracker from the list, this function doesn't free the buffer itself, it will need to be freed
 * with free() by the calling process.
 * 
 * @param ptr      Pointer to the allocated memory.
 */
void __xfreetrace(void *ptr)
{
	MEMTRACK *tptr, *prev;
	prev = NULL;

	if (ptr != NULL)
	{
		for (tptr = mudstate.raw_allocs; tptr != NULL; tptr = tptr->next)
		{
			if (tptr->bptr == ptr)
			{
				if (mudconf.malloc_logger)
				{
					log_write(LOG_MALLOC, "MEM", "TRACE", "Free: %s (%d bytes)", tptr->var, tptr->size);
				}

				if (mudstate.raw_allocs == tptr)
				{
					mudstate.raw_allocs = tptr->next;
				}

				if (prev)
				{
					prev->next = tptr->next;
				}

				free(tptr);
				return;
			}

			prev = tptr;
		}
	}
}

/******************************************************************************
 * String functions
 */

/**
 * @brief Tracked function of xstrdup
 * 
 * The __xstrdup() function returns a pointer to a new string which is a duplicate of the
 * string s. Memory for the new string is tracked, and must be freed with __xfree().
 * 
 * Do not call this directly, use the wrapper macro XSTRDUP(const char *str, size_t size, const char *var)
 * that will fill most of the banks for you.
 * 
 * @param str      string to replicate.
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 * @return char*   Pointer to the new string buffer.
 */
char *__xstrdup(const char *str, const char *file, int line, const char *function, const char *var)
{
	char *r;

	if (mudconf.malloc_logger)
	{
		log_write(LOG_MALLOC, "MEM", "TRACE", "XSTRDUP: %s/%d", var, strlen(str) + 1);
	}

	if (str || *str)
	{
		r = strdup(str);
		__xalloctrace(strlen(str) + 1, r, file, line, function, var);
		return (r);
	}
	else
	{
		return (NULL);
	}
}

/**
 * @brief Tracked sprintf replacement that create a new formatted string buffer.
 * 
 * Much like sprintf, but create a new buffer sized for the resulting string. Memory 
 * for the new string is tracked, and must be freed with __xfree().
 * 
 * Do not call this directly, use the wrapper macro XSPRINTF(const char *var, const char *format, ...)
 * that will fill most of the banks for you.
 * 
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 * @param format   Format string.
 * @param ...      Variables arguments for the format string.
 * @return char*   Pointer to the new string buffer.
 */
char *__xsprintf(const char *file, int line, const char *function, const char *var, const char *format, ...)
{
	int size = 0, vsize = 0;
	char *str = NULL;
	va_list ap;

	va_start(ap, format);
	size = vsnprintf(str, size, format, ap);
	va_end(ap);

	if (size < 0)
	{
		return NULL;
	}

	size++;
	str = calloc(size, sizeof(char));

	if (str == NULL)
	{
		return NULL;
	}

	va_start(ap, format);
	vsize = vsnprintf(str, size, format, ap);
	va_end(ap);

	if (vsize < 0)
	{
		free(str);
		return NULL;
	}

	if (var)
	{
		__xalloctrace(size, str, file, line, function, var);
	}

	return str;
}

/******************************************************************************
 * MUSH interfaces
 */

/**
 * @brief Helper function to sort the trace table.
 * 
 * This function is used by list_rawmemory for sorting the trace table via qsort()
 * 
 * @param p1       Pointer to the first MEMTRACK struct to compare.
 * @param p2       Pointer to the second MEMTRACK struct to compare.
 * @return int     Result of the comparison (see man qsort()).
 */
int __xsorttrace(const void *p1, const void *p2)
{
	char *s1, *s2;
	int r;

	s1 = XNSPRINTF("%s:%s", (*(MEMTRACK **)p1)->function, (*(MEMTRACK **)p1)->var);
	s2 = XNSPRINTF("%s:%s", (*(MEMTRACK **)p2)->function, (*(MEMTRACK **)p2)->var);

	r = strcmp(s1, s2);

	free(s2);
	free(s1);

	return (r);
}

/**
 * @brief Show to the player a summary of all allocations.
 * 
 * Called by @list raw_memory. This is primary a debug function and
 * may cause lag on a very large game. Use with caution.
 * 
 * @param player   Dbref of the player making the request.
 */
void list_rawmemory(dbref player)
{
	MEMTRACK *tptr = NULL, **t_array = NULL;
	size_t n_tags = 0, total = 0, c_tags = 0, c_total = 0, u_tags = 0, l1 = 0, l2 = 0;
	int i = 0, j = 0;
	//const char *ntmp = NULL;
	char *s1 = NULL, *s2 = NULL;

	n_tags = total = 0;
	notify(player, "Memory Tag                                                       Allocs Bytes   ");
	notify(player, "---------------------------------------------------------------- ------ --------");

	for (tptr = mudstate.raw_allocs; tptr != NULL; tptr = tptr->next)
	{
		n_tags++;
		total += tptr->size;
	}

	t_array = (MEMTRACK **)calloc(total, sizeof(MEMTRACK *));

	for (i = 0, tptr = mudstate.raw_allocs; tptr != NULL; i++, tptr = tptr->next)
	{
		t_array[i] = tptr;
	}

	qsort((void *)t_array, n_tags, sizeof(MEMTRACK *), (int (*)(const void *, const void *))__xsorttrace);

	for (i = 0; i < n_tags;)
	{
		u_tags++;

		s1 = XNSPRINTF("%s:%s", t_array[i]->function, t_array[i]->var);
		s2 = XNSPRINTF("%s:%s", t_array[i + 1]->function, t_array[i + 1]->var);

		if ((i < n_tags - 1) && !strcmp(s1, s2))
		{
			c_tags = 2;
			c_total = t_array[i]->size + t_array[i + 1]->size;

			free(s2);

			for (j = i + 2; (j < n_tags); j++)
			{

				s2 = XNSPRINTF("%s:%s", t_array[j]->function, t_array[j]->var);

				if (strcmp(s1, s2))
				{
					free(s2);
					break;
				}
				free(s2);

				c_tags++;
				c_total += t_array[j]->size;
			}

			i = j;
		}
		else
		{
			c_tags = 1;
			c_total = t_array[i]->size;
			i++;
		}

		if (c_total < 1024)
		{
			raw_notify(player, "%-64.64s %6d %8d", s1, c_tags, c_total);
		}
		else if (c_total < 1048756)
		{
			raw_notify(player, "%-64.64s %6d %7.2fK", s1, c_tags, (float)c_total / 1024.0);
		}
		else
		{
			raw_notify(player, "%-64.64s %6d %7.2fM", s1, c_tags, (float)c_total / 1048756.0);
		}
		free(s1);
	}

	free(t_array);
	notify(player, "--------------------------------------------------------------------------------");
	raw_notify(player, "Total: %d raw allocations in %d unique tags. %d bytes (%0.2fK).", n_tags, u_tags, total, (float)total / 1024.0);
}

/**
 * @brief Calculate the total size of all allocations.
 * 
 * @return size_t  Memory usage.
 */
size_t total_rawmemory(void)
{
	MEMTRACK *tptr = NULL;
	size_t total_bytes = 0;

	for (tptr = mudstate.raw_allocs; tptr != NULL; tptr = tptr->next)
	{
		total_bytes += tptr->size;
	}

	return (total_bytes);
}
