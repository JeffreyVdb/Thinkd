#ifndef _VOID_HEY_
#define _VOID_HEY_

/*
 * This file contains general purpose macros and inline functions
 */

#define array_count(arr) (sizeof(arr) / sizeof(arr[0]))
#define THINKD_ATTR_NORET	__attribute__((noreturn))
#define THINKD_ATTR_PRINTF(n)	__attribute__((format(printf, n, n+1)))

#endif
