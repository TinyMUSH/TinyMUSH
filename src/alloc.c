/**
 * @file alloc.c
 * @author TinyMUSH development team (https://github.com/TinyMUSH)
 * @brief memory management subsystem
 * @version 3.3
 * @date 2020-11-14
 * 
 * @copyright Copyright (C) 1989-2020 TinyMUSH development team.
 * 
 */
#include "copyright.h"
#include "config.h"
#include "system.h"

#include "typedefs.h"
#include "game.h"
#include "alloc.h"
#include "flags.h"
#include "htab.h"
#include "udb.h"
#include "udb_defs.h"
#include "mushconf.h"
#include "db.h"
#include "interface.h"
#include "externs.h"

/**
 * Macros and utilities.
 * 
 */
#define XLOGALLOC(x, y, z, s, ...)            \
	if (mudconf.malloc_logger)                \
	{                                         \
		log_write(x, y, z, s, ##__VA_ARGS__); \
	}

/**
 * Allocation functions
 * 
 */

/**
 * @brief Tracked version of malloc().
 * 
 * The __xmalloc() function allocates size bytes and returns a pointer to the allocated memory.
 * The memory is set to zero.  If size is 0, then malloc() returns NULL.
 * 
 * This variant doesn't check if the multiplication of nmemb and size would result in integer 
 * overflow, so don't be silly and don't try to allocate more than 18 exabytes of memory (or 
 * 4 gigabytes if your still on 32 bits).
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
	if (size > 0)
	{
		if (var)
		{
			MEMTRACK *mtrk = NULL;

			//mtrk = (MEMTRACK *)calloc(size + sizeof(MEMTRACK) + sizeof(uint64_t), sizeof(char));
			mtrk = (MEMTRACK *)malloc(size + sizeof(MEMTRACK) + sizeof(uint64_t));
			memset(mtrk, 0, size + sizeof(MEMTRACK) + sizeof(uint64_t));

			if (mtrk)
			{
				mtrk->bptr = (void *)((char *)mtrk + sizeof(MEMTRACK));
				mtrk->size = size;
				mtrk->file = file;
				mtrk->line = line;
				mtrk->function = function;
				mtrk->var = var;
				mtrk->next = mudstate.raw_allocs;
				mtrk->magic = (uint64_t *)((char *)mtrk->bptr + size);
				mudstate.raw_allocs = mtrk;
				*(mtrk->magic) = XMAGIC;
				XLOGALLOC(LOG_MALLOC, "MEM", "TRACE", "%s[%d]%s:%s Alloc %ld bytes", mtrk->file, mtrk->line, mtrk->function, mtrk->var, mtrk->size);
				return mtrk->bptr;
			}
		}
		else
		{
			return calloc(size, sizeof(char));
		}
	}
	return NULL;
}

/**
 * @brief Tracked version of calloc().
 * 
 * The __xcalloc() function allocates memory for an array of nmemb elements of size bytes each and
 * returns a pointer to the allocated memory. The memory is set to zero.  If nmemb or size is 0, then
 * calloc() returns NULL. If the multiplication of nmemb and size would result in integer overflow, 
 * then __xcalloc() returns an error.
 * 
 * This variant doesn't check if the multiplication of nmemb and size would result in integer 
 * overflow, so don't be silly and don't try to allocate more than 18 exabytes of memory (or 
 * 4 gigabytes if your still on 32 bits).
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
	if ((nmemb * size) > 0)
	{
		return __xmalloc(nmemb * size, file, line, function, var);
	}

	return NULL;
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
 * This variant doesn't check if the multiplication of nmemb and size would result in integer 
 * overflow, so don't be silly and don't try to allocate more than 18 exabytes of memory (or 
 * 4 gigabytes if your still on 32 bits).
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
	if (size > 0)
	{
		void *nptr = NULL;
		MEMTRACK *mtrk = NULL;

		if (var)
		{
			nptr = __xmalloc(size, file, line, function, var);
			if (ptr)
			{
				mtrk = __xfind(ptr);
				if (mtrk)
				{
					if (mtrk->size < size)
					{
						memmove(nptr, ptr, mtrk->size);
					}
					else
					{
						memmove(nptr, ptr, size);
					}
				}
				else
				{
					memmove(nptr, ptr, size);
				}
				__xfree(ptr);
			}
			ptr = nptr;
		}
		else
		{
			nptr = realloc(ptr, size);
			ptr = nptr;
		}
	}
	else
	{
		__xfree(ptr);
		ptr = NULL;
	}

	return ptr;
}

/**
 * @brief Search the tracking list for a pointer and return it's information.
 * 
 * __xfind will search thru all allocated blocks for the address pointed by ptr,
 * if this address is within the start and end of the allocaed block, it will return
 * the MEMTRACK information about that block.
 * 
 * It return NULL if nothing found.
 * 
 * @param ptr      Pointer to or somewhere in the allocated memory block.
 * @return         Pointer to a MEMTRACK structure with information on the memory block.
 *                 or NULL if not found.
 */
MEMTRACK *__xfind(void *ptr)
{
	if (ptr)
	{
		if (mudstate.raw_allocs)
		{
			MEMTRACK *curr = mudstate.raw_allocs;

			while (curr)
			{
				if (((uintptr_t)ptr >= (uintptr_t)curr->bptr) && ((uintptr_t)ptr < (uintptr_t)curr->magic))
				{
					return curr;
				}
				curr = curr->next;
			}
		}
	}
	return NULL;
}

/**
 * @brief Check if a buffer wasn't overrun.
 * 
 * @param ptr      Pointer to check.
 * @return int     1, if everything is allright. 0, if buffer overrun, 2 if the buffer
 *                 isn't track.
 */
int __xcheck(void *ptr)
{
	if (ptr)
	{
		if (mudstate.raw_allocs)
		{
			MEMTRACK *curr = mudstate.raw_allocs;

			while (curr)
			{
				if ((ptr >= (void *)curr->bptr) && (ptr < (void *)curr->magic))
				{
					if (*(curr->magic) == XMAGIC)
					{
						return 1;
					}
					return 0;
				}
				curr = curr->next;
			}
		}
	}
	return 2;
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
 * @return 1 if the buffer was tracked and it has been overrun, else 0.
 */
int __xfree(void *ptr)
{
	int overrun = 0;

	if (ptr)
	{
		MEMTRACK *prev = NULL;
		MEMTRACK *curr = mudstate.raw_allocs;

		while (curr)
		{
			if ((ptr >= (void *)curr->bptr) && (ptr < (void *)curr->magic))
			{
				if (*curr->magic != XMAGIC)
				{
					overrun = 1;
				}
				if (overrun)
				{
					XLOGALLOC(LOG_MALLOC, "MEM", "TRACE", "%s[%d]%s:%s Free %ld bytes", curr->file, curr->line, curr->function, curr->var, curr->size);
				}
				else
				{

					XLOGALLOC(LOG_MALLOC, "MEM", "TRACE", "%s[%d]%s:%s Free %ld bytes -- OVERRUN ---", curr->file, curr->line, curr->function, curr->var, curr->size);
				}
				if (mudstate.raw_allocs == curr)
				{
					mudstate.raw_allocs = curr->next;
				}
				else
				{
					prev->next = curr->next;
				}

				free(curr);
				ptr = NULL;
				break;
			}
			prev = curr;
			curr = curr->next;
		}
	}
	/*
	if (ptr)
	{
		XLOGALLOC(LOG_MALLOC, "MEM", "TRACE", "%p Free", ptr);
		free(ptr);
		ptr = NULL;
	}
	*/

	return overrun;
}

/**
 * String functions
 * 
 */

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
char *__xasprintf(const char *file, int line, const char *function, const char *var, const char *format, ...)
{
	int size = 0;
	char *str = NULL;
	va_list ap;

	va_start(ap, format);
	size = vsnprintf(str, size, format, ap);
	va_end(ap);

	if (size > 0)
	{
		size++;
		if (var)
		{
			str = (char *)__xmalloc(size, file, line, function, var);
		}
		else
		{
			str = (char *)calloc(size, sizeof(char));
		}

		if (str)
		{
			va_start(ap, format);
			size = vsnprintf(str, size, format, ap);
			va_end(ap);

			if (size < 0)
			{
				__xfree(str);
				str = NULL;
			}
		}
	}

	return str;
}

/**
 * @brief Tracked sprintf replacement.
 * 
 * Like sprintf, but make sure that the buffer does not overflow if the buffer is tracked.
 * Else, act exactly like vsprintf.
 * 
 * @param str     Buffer where to write the resulting string.
 * @param format  Format string.
 * @param ...     Variables argument list for the format string.
 * @return int    Number of byte written to the buffer.
 */
int __xsprintf(char *str, const char *format, ...)
{
	int size = 0;

	va_list ap;
	va_start(ap, format);
	size = __xvsprintf(str, format, ap);
	va_end(ap);

	return size;
}

/**
 * @brief Tracked vsprintf replacement.
 * 
 * Like vsprintf, but make sure that the buffer does not overflow if the buffer is tracked.
 * Else, act exactly like vsprintf.
 * 
 * @param str     Buffer where to write the resulting string.
 * @param format  Format string.
 * @param ap      Variables argument list for the format string.
 * @return int    Number of byte written to the buffer.
 */
int __xvsprintf(char *str, const char *format, va_list ap)
{
	int size = 0;

	if (str)
	{
		va_list vp;
		va_copy(vp, ap);
		size = vsnprintf(str, size, format, vp);
		va_end(vp);

		if (size > 0)
		{
			size++;
			va_copy(vp, ap);
			MEMTRACK *mtrk = __xfind(str);
			if (mtrk)
			{
				if ((str + size) < ((char *)mtrk->magic))
				{
					size = vsnprintf(str, size, format, vp);
				}
				else
				{
					size = vsnprintf(str, (size_t)(((char *)mtrk->magic) - str), format, vp);
				}
			}
			else
			{
				size = vsprintf(str, format, vp);
			}

			va_end(vp);
		}
	}

	return size;
}

/**
 * @brief Tracked snprintf replacement.
 * 
 * Like snprintf, but make sure that the buffer does not overflow if the buffer is tracked.
 * Else, act exactly like snprintf.
 * 
 * @param str      Buffer where to write the resulting string.
 * @param size     Maximum size to write to the buffer.
 * @param format   Format string.
 * @param ...      Variables arguments for the format string.
 * @return int     Number of byte written to the buffer.
 */
int __xsnprintf(char *str, size_t size, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	size = __xvsnprintf(str, size, format, ap);
	va_end(ap);

	return size;
}

/**
 * @brief Tracked vsnprintf replacement.
 * 
 * Like vsnprintf, but make sure that the buffer does not overflow if the buffer is tracked.
 * Else, act exactly like snprintf.
 * 
 * @param str      Buffer where to write the resulting string.
 * @param size     Maximum size to write to the buffer.
 * @param format   Format string.
 * @param ap       Variables argument list for the format string.
 * @return int     Number of byte written to the buffer.
 */
int __xvsnprintf(char *str, size_t size, const char *format, va_list ap)
{
	if ((size > 0) && str)
	{
		va_list vp;
		va_copy(vp, ap);
		MEMTRACK *mtrk = __xfind(str);
		if (mtrk)
		{
			if ((str + size) < ((char *)mtrk->magic))
			{
				size = vsnprintf(str, size, format, vp);
			}
			else
			{
				size = vsnprintf(str, (size_t)(((char *)mtrk->magic) - str), format, vp);
			}
		}
		else
		{
			size = vsnprintf(str, size, format, vp);
		}

		va_end(vp);
	}

	return size;
}

/**
 * @brief Tracked sprintfcat
 * 
 * Like sprintf, but append to the buffer and make sure it does not overflow
 * if the buffer is tracked.
 * 
 * @param str     Buffer where to write the resulting string.
 * @param format  Format string.
 * @param ...     Variables argument list for the format string.
 * @return int    Number of byte written to the buffer.
 */
int __xsprintfcat(char *str, const char *format, ...)
{
	int size = 0;
	va_list ap;

	va_start(ap, format);
	size = __xvsprintf(str + strlen(str), format, ap);
	va_end(ap);

	return size;
}

/**
 * @brief tracked safe_printf replacement.
 * 
 * @param buff		Buffer that will receive the result
 * @param bufp		Pointer to the location in the buffer
 * @param format	Format string.
 * @param ...		Variables argument list for the format string.
 * @return char*	Pointer to buffer
 */
char *__xsafesprintf(char *buff, char **bufp, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	*bufp += XVSPRINTF(*bufp, format, ap);
	va_end(ap);
	return (buff);
}

/**
 * @brief Tracked version of strdup
 * 
 * The __xstrdup() function returns a pointer to a new string which is a duplicate of the
 * string s. Memory for the new string is tracked, and must be freed with __xfree().
 * 
 * Do not call this directly, use the wrapper macro XSTRDUP(const char *str, size_t size, const char *var)
 * that will fill most of the banks for you.
 * 
 * @param s        String to duplicate.
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 * @return char*   Pointer to the new string buffer.
 */
char *__xstrdup(const char *s, const char *file, int line, const char *function, const char *var)
{
	char *r = NULL;

	if (s)
	{
		size_t l = strlen(s);
		r = (char *)__xmalloc(l + 1, file, line, function, var);
		strcpy(r, s);
	}

	return (r);
}

/**
 * @brief Tracked version of strndup
 * 
 * The __xstrdup() function returns a pointer to a new string which is a duplicate of the
 * string s. Memory for the new string is tracked, and must be freed with __xfree().
 * 
 * Do not call this directly, use the wrapper macro XSTRDUP(const char *str, size_t size, const char *var)
 * that will fill most of the banks for you.
 * 
 * @param s        String to duplicate.
 * @param n        Number of bytes to replicates.
 * @param file     Filename where the allocation is done.
 * @param line     Line number where the allocation is done.
 * @param function Function where the allocation is done.
 * @param var      Name of the variable that will receive the pointer (not the variable itself).
 * @return char*   Pointer to the new string buffer.
 */
char *__xstrndup(const char *s, size_t n, const char *file, int line, const char *function, const char *var)
{
	char *r = NULL;

	if (s)
	{
		r = (char *)__xmalloc(n + 1, file, line, function, var);
		strncpy(r, s, n);
		*(r + n) = 0x00;
	}

	return (r);
}

/**
 * @brief Tracked stpcpy replacement.
 * 
 * Copy a string from src to dest, returning a pointer to the end of the 
 * resulting string at dest. For tracked allocation, make sure that the 
 * destination buffer does not overrun.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @return Pointer to the end of the string dest (that is, the address of the 
 *         terminating null byte) rather than the beginning.
 */
char *__xstpcpy(char *dest, const char *src)
{
	char *ptr = NULL;

	if (dest)
	{
		size_t size = strlen(src);
		MEMTRACK *mtrk = __xfind(dest);

		if (mtrk)
		{
			if (dest + size < ((char *)mtrk->magic))
			{
				ptr = stpcpy(dest, src);
			}
			else
			{
				ptr = strncpy(dest, src, ((char *)mtrk->magic) - dest) + size;
			}
		}
		else
		{
			ptr = stpcpy(dest, src);
		}
	}
	return ptr;
}

/**
 * @brief Tracked strcat replacement.
 * 
 * Append the string src to string dest, returning a pointer to dest. For
 * tracked allocation, make sure that the destination buffer does not overrun.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @return Pointer to the destination string dest.
 */
char *__xstrcat(char *dest, const char *src)
{
	char *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - (dest + strlen(dest));
			if (size > 0)
			{
				ptr = strncat(dest, src, size);
			}
		}
		else
		{
			ptr = strcat(dest, src);
		}
	}
	return ptr;
}

/**
 * @brief Tracked strncat replacement.
 * 
 * Append at most n bytes from string src to string dest, returning a pointer
 * to dest. For tracked allocation, make sure that the destination buffer does
 * not overrun.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @param n Maximum numbers of characters to copy.
 * @return Pointer to the destination string dest.
 */
char *__xstrncat(char *dest, const char *src, size_t n)
{
	char *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - (dest + strlen(dest));
			if (size > n)
			{
				size = n;
			}
			if (size > 0)
			{
				ptr = strncat(dest, src, size);
			}
		}
		else
		{
			ptr = strncat(dest, src, n);
		}
	}
	return ptr;
}

/**
 * @brief Tracked strccat.
 * 
 * Append the character src to string dest, returning a pointer to dest. For
 * tracked allocation, make sure that the destination buffer does not overrun.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Source character.
 * @return Pointer to the end of destination string dest.
 */
char *__xstrccat(char *dest, const char src)
{
	char *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		ptr = dest + strlen(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - (dest + strlen(dest));
			if (size < 2)
			{
				return ptr;
			}
		}
		*(ptr++) = src;
		*(ptr) = 0x0;
	}
	return ptr;
}

/**
 * @brief Tracked strnccat.
 * 
 * Append the character src to string dest and limit dest lengt to n, returning
 * a pointer to dest. For tracked allocation, make sure that the destination 
 * buffer does not overrun.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Source character.
 * @param n Maximum length of dest.
 * @return Pointer to the end of destination string dest.
 */
char *__xstrnccat(char *dest, const char src, size_t n)
{
	char *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		ptr = dest + strlen(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - (dest + strlen(dest));
			if (size < 2)
			{
				return ptr;
			}
		}
		if (strlen(dest) + 1 <= n)
		{
			*(ptr++) = src;
			*(ptr) = 0x0;
		}
	}
	return ptr;
}

/**
 * @brief Tracked strcpy replacement.
 * 
 * Copy the string src to dest, returning a pointer to dest. For tracked
 * allocation, make sure that the destination buffer does not overrun.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @return Pointer to the destination string dest.
 */
char *__xstrcpy(char *dest, const char *src)
{
	char *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - dest;
			if (size > 0)
			{
				ptr = strncpy(dest, src, size);
			}
		}
		else
		{
			ptr = strcpy(dest, src);
		}
	}
	return ptr;
}

/**
 * @brief Tracked strncpy replacement.
 * 
 * Copy at most n bytes from string src to dest, returning a pointer to dest.
 * For tracked allocation, make sure that the destination buffer does not
 * overrun.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @param n Maximum numbers of characters to copy.
 * @return Pointer to the destination string dest.
 */
char *__xstrncpy(char *dest, const char *src, size_t n)
{
	char *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - dest;
			if (n < size)
			{
				size = n;
			}
			if (size > 0)
			{
				ptr = strncpy(dest, src, size);
			}
		}
		else
		{
			ptr = strncpy(dest, src, n);
		}
	}
	return ptr;
}

/**
 * @brief Tracked memmove replacement.
 * 
 * The __xmemmove() function copies n bytes from memory area src to memory area
 * dest. The memory areas may overlap: copying takes place as though the bytes
 * in src are first copied into a temporary array that does not overlap src or
 * dest, and the bytes are then copied from the temporary array to dest.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @param n Number of bytes to copy.
 * @return Pointer to the destination buffer.
 */
void *__xmemmove(void *dest, const void *src, size_t n)
{
	void *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - (char *)dest;
			if (n < size)
			{
				size = n;
			}
			if (size > 0)
			{
				ptr = memmove(dest, src, size);
			}
		}
		else
		{
			ptr = memmove(dest, src, n);
		}
	}
	return ptr;
}

/**
 * @brief Tracked mempcpy replacement.
 * 
 * The __xmempcpy() function is nearly identical to the memcpy(3) function. It
 * copies n bytes from the object beginning at src into the object pointed to 
 * by dest. But instead of returning the value of dest it returns a pointer to
 * the byte following the last written byte. The memory areas may overlap: 
 * copying takes place as though the bytes in src are first copied into a
 * temporary array that does not overlap src or dest, and the bytes are then
 * copied from the temporary array to dest.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @param n Number of bytes to copy.
 * @return Pointer to the byte following the last written byte.
 */
void *__xmempcpy(void *dest, const void *src, size_t n)
{
	void *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - (char *)dest;
			if (n < size)
			{
				size = n;
			}
			if (size > 0)
			{
				ptr = (void *)((char *)memmove(dest, src, size) + size);
			}
		}
		else
		{
			ptr = (void *)((char *)memmove(dest, src, n) + n);
		}
	}
	return ptr;
}

/**
 * @brief Tracked memccpy replacement.
 * 
 * The memccpy() function copies no more than n bytes from memory area src to
 * memory area dest, stopping when the character c is found. The memory areas
 * may overlap: copying takes place as though the bytes in src are first copied
 * into a temporary array that does not overlap src or dest, and the bytes are
 * then copied from the temporary array to dest.
 * 
 * @param dest Pointer to destination buffer.
 * @param src Pointer to source buffer.
 * @param n Number of bytes to copy.
 * @return Pointer to the destination buffer.
 */
void *__xmemccpy(void *dest, const void *src, int c, size_t n)
{
	void *ptr = NULL;

	if (dest)
	{
		MEMTRACK *mtrk = __xfind(dest);
		if (mtrk)
		{
			void *ptr1 = __xmalloc(n, __FILE__, __LINE__, __func__, "ptr1");
			size_t size = ((char *)mtrk->magic) - (char *)dest;
			memmove(ptr1, src, n);
			if (n < size)
			{
				size = n;
			}
			if (size > 0)
			{
				ptr = memccpy(dest, ptr1, c, size);
			}
			__xfree(ptr1);
		}
		else
		{
			void *ptr1 = __xmalloc(n, __FILE__, __LINE__, __func__, "ptr1");
			memmove(ptr1, src, n);
			ptr = memccpy(dest, ptr1, c, n);
			__xfree(ptr1);
		}
	}
	return ptr;
}

/**
 * @brief Tracked memset replacement.
 * 
 * The __xmemset() function fills the first n bytes of the memory area pointed to by s
 * with the constant byte c. For tracked allocation, make sure that the buffer does not
 * overrun.
 * 
 * @param s Pointer to the memory area to fill.
 * @param c Character to fill with.
 * @param n Size of the memory area to fill.
 * @return Pointer to the memory area.
 */
void *__xmemset(void *s, int c, size_t n)
{
	if (s)
	{
		MEMTRACK *mtrk = __xfind(s);
		if (mtrk)
		{
			size_t size = ((char *)mtrk->magic) - (char *)s;
			if (size < n)
			{
				n = size;
			}
		}
		if (n > 0)
		{
			memset(s, c, n);
		}
	}
	return (s);
}

/******************************************************************************
 * MUSH interfaces
 */

/**
 * @brief Function placeholder. Not used for now.
 * 
 * @param player dbref of the player who did the command
 */
void list_bufstats(dbref player)
{
	notify(player, "This feature has been removed.");
}

/**
 * @brief Function placeholder. Not used for now.
 * 
 * @param player dbref of the player who did the command
 */
void list_buftrace(dbref player)
{
	notify(player, "This feature has been removed.");
}

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

	s1 = XNASPRINTF("%s:%s", (*(MEMTRACK **)p1)->function, (*(MEMTRACK **)p1)->var);
	s2 = XNASPRINTF("%s:%s", (*(MEMTRACK **)p2)->function, (*(MEMTRACK **)p2)->var);

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
	char *s1 = NULL, *s2 = NULL;

	notify(player, "Memory Tag                                                       Allocs Bytes   ");
	notify(player, "---------------------------------------------------------------- ------ --------");

	if (mudstate.raw_allocs != NULL)
	{
		for (tptr = mudstate.raw_allocs; tptr != NULL; tptr = tptr->next)
		{
			n_tags++;
			total += tptr->size;
		}

		if (n_tags > 1)
		{
			t_array = (MEMTRACK **)calloc(total, sizeof(MEMTRACK *));

			for (i = 0, tptr = mudstate.raw_allocs; tptr != NULL; i++, tptr = tptr->next)
			{
				t_array[i] = tptr;
			}

			qsort((void *)t_array, n_tags, sizeof(MEMTRACK *), (int (*)(const void *, const void *))__xsorttrace);

			for (i = 0; i < n_tags;)
			{
				u_tags++;

				s1 = XNASPRINTF("%s:%s", t_array[i]->function, t_array[i]->var);
				if (t_array[i + 1] != NULL)
				{
					s2 = XNASPRINTF("%s:%s", t_array[i + 1]->function, t_array[i + 1]->var);
				}
				else
				{
					s2 = NULL;
				}

				if ((i < n_tags - 1) && !strcmp(s1, s2))
				{
					free(s2);
					c_tags = 2;
					c_total = t_array[i]->size + t_array[i + 1]->size;

					for (j = i + 2; (j < n_tags); j++)
					{

						s2 = XNASPRINTF("%s:%s", t_array[j]->function, t_array[j]->var);

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
					free(s2);
					c_tags = 1;
					c_total = t_array[i]->size;
					i++;
				}

				if (c_total < 1024)
				{
					raw_notify(player, "%-64.64s %6ld %8ld", s1, c_tags, c_total);
				}
				else if (c_total < 1048756)
				{
					raw_notify(player, "%-64.64s %6ld %7.2fK", s1, c_tags, (float)c_total / 1024.0);
				}
				else
				{
					raw_notify(player, "%-64.64s %6ld %7.2fM", s1, c_tags, (float)c_total / 1048756.0);
				}
				free(s1);
			}

			free(t_array);
		}
		else
		{
			u_tags = 1;
			s1 = XNASPRINTF("%s:%s", mudstate.raw_allocs->function, mudstate.raw_allocs->var);
			if (c_total < 1024)
			{
				raw_notify(player, "%-64.64s %6ld %8ld", s1, n_tags, total);
			}
			else if (c_total < 1048756)
			{
				raw_notify(player, "%-64.64s %6ld %7.2fK", s1, n_tags, (float)total / 1024.0);
			}
			else
			{
				raw_notify(player, "%-64.64s %6ld %7.2fM", s1, n_tags, (float)total / 1048756.0);
			}
			free(s1);
		}
	}

	notify(player, "--------------------------------------------------------------------------------");
	raw_notify(player, "Total: %ld raw allocations in %ld unique tags. %ld bytes (%0.2fK).", n_tags, u_tags, total, (float)total / 1024.0);
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

/**
 * Replacement for the safe_* functions.
 * 
 */

/**
 * @brief Copy a string with at most n character from src and update
 * the position pointer to the end of the destination buffer.
 * 
 * @param dest Pointer to destination buffer.
 * @param destp Pointer tracking the desstination buffer.
 * @param src Pointer to the string to concatenate.
 * @param n Maximum number of characters to concatenate.
 * @param size Maximum size of dest buffer if untracked.
 * @return Number of characters that where not copied if the buffer
 *         (whichever is smaller) isn't big enough to hold the result.
 */
size_t __xsafestrncpy(char *dest, char **destp, const char *src, size_t n, size_t size)
{
	if (src)
	{
		if (dest)
		{
			if (__xfind(dest))
			{
				size = n < XCALSIZE(XGETSIZE(dest), dest, destp) ? n : XCALSIZE(XGETSIZE(dest), dest, destp);
			}

			if (size > 0)
			{
				**destp = 0;
				strncpy(*destp, src, size);
				*destp += size;
				**destp = 0;
			}
		}
		return ((int64_t)(strlen(src) - size)) > 0 ? ((int64_t)(strlen(src) - size)) : 0;
	}
	return 0;
}

/**
 * @brief Copy char 'c' to dest and update the position pointer to the end of
 *        the destination buffer.
 * 
 * @param c Char to copy
 * @param dest Pointer to destination buffer.
 * @param destp Pointer tracking the desstination buffer.
 * @param size Maximum size of the destination buffer.
 */
int __xsafestrcatchr(char *dest, char **destp, char c, size_t size)
{
	if (dest)
	{
		if (__xfind(dest))
		{
			size = XCALSIZE(XGETSIZE(dest), dest, destp);
		}
		else
		{
			size = XCALSIZE(size, dest, destp);
		}
		if (size > 0)
		{
			char *tp = *destp;
			*tp++ = c;
			*tp = 0;
			*destp = tp;
			return 0;
		}
	}
	return 1;
}

/**
 * @brief Concatenate two string with at most n character from src and update
 * the position pointer to the end of the destination buffer.
 * 
 * @param dest Pointer to destination buffer.
 * @param destp Pointer tracking the desstination buffer.
 * @param src Pointer to the string to concatenate.
 * @param n Maximum number of characters to concatenate.
 * @param size Maximum size of dest buffer if untracked.
 * @return Number of characters that where not copied if the buffer
 *         (whichever is smaller) isn't big enough to hold the result.
 */
size_t __xsafestrncat(char *dest, char **destp, const char *src, size_t n, size_t size)
{
	if (src)
	{
		if (dest)
		{
			if (__xfind(dest))
			{
				size = n < XCALSIZE(XGETSIZE(dest), dest, destp) ? n : XCALSIZE(XGETSIZE(dest), dest, destp);
			}

			if (size > 0)
			{
				**destp = 0;
				strncat(*destp, src, size);
				*destp += size;
				**destp = 0;
			}
		}
		return ((int64_t)(strlen(src) - size)) > 0 ? ((int64_t)(strlen(src) - size)) : 0;
	}
	return 0;
}

/**
 * @brief Convert a long signed number into string.
 * 
 * @param dest Pointer to destination buffer.
 * @param destp Pointer tracking the desstination buffer.
 * @param num Number to convert.
 * @param size Maximum size of the destination buffer.
 */
void __xsafeltos(char *dest, char **destp, long num, size_t size)
{
	char *buff = XLTOS(num);
	__xsafestrncat(dest, destp, buff, strlen(buff), size);
	__xfree(buff);
}

/**
 * @brief Return a string with 'count' number of 'ch' characters.
 * 
 * It is the responsibility of the caller to free the resulting buffer.
 * 
 * @param size Size of the string to build. 
 * @param c Character to fill the string with.
 * @return Pointer to the build string.
 */
char *__xrepeatchar(size_t size, char c)
{
	void *ptr = (char *)XMALLOC(size + 1, "ptr");

	if (ptr)
	{
		return (char *)__xmemset(ptr, c, size);
	}

	return NULL;
}
