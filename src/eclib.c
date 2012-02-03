#include "eclib.h"
#include "logger.h"
#include <stdlib.h>

static inline void *ec_allocated(void *ptr)
{
	if (ptr)
		return ptr;

	thinkd_log(LOG_ERR, "failed to allocate memory, exiting.");
	exit(EXIT_FAILURE);
}

void *ec_malloc(size_t nbytes)
{
	void *ptr;

	ptr = ec_allocated(malloc(nbytes));
#if _DEBUG_LOG
	thinkd_log(LOG_DEBUG, "malloc: address=%p; size=%zu", ptr, nbytes);
#endif
	return ptr;
}

void *ec_calloc(size_t nelems, size_t nbytes)
{
	void *ptr;
	
	ptr = ec_allocated(calloc(nelems, nbytes));
#if _DEBUG_LOG
	thinkd_log(LOG_DEBUG, "calloc: address=%p; nmem=%zu; size=%zu", ptr, nelems, nbytes); 
#endif
	return ptr;
}

