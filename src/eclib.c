#include "eclib.h"

static inline void *ec_allocated(void *ptr)
{
	if (ptr)
		return ptr;

	/* TODO: error handling */
	return NULL;
}

void *ec_malloc(size_t nbytes)
{
	return ec_allocated(malloc(nbytes));
}

void *ec_calloc(size_t nelems, size_t nbytes)
{
	return ec_allocated(calloc(nelems, nbytes));
}
