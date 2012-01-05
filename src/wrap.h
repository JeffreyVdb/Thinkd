#ifndef _THINKD_WRAP_H_
#define _THINKD_WRAP_H_ 1

#include <stdlib.h>
#include "config.h"
#include "logger.h"

#if _DEBUG_LOG > 0
#define	malloc(bytes) ec_malloc(bytes)
#define	calloc(nmem,bytes) ec_calloc(nmem,bytes)
#define free(ptr) log_free(ptr)

void *ec_malloc(unsigned int bytes);
void *ec_calloc(unsigned int nmem, unsigned int bytes);
void log_free(void *ptr);
#endif

#endif /* _WRAP_H_ */
