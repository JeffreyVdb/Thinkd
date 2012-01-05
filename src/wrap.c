#include "wrap.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#if _DEBUG_LOG > 0
void log_free(void *ptr)
{
	thinkd_log(LOG_DEBUG, "free: address=%p", ptr);
	(free)(ptr);
}
void *ec_malloc(unsigned int bytes)
{
	void *ptr;
	ptr = (malloc)(bytes);
	thinkd_log(LOG_DEBUG, "malloc: address=%p; size=%u", ptr, bytes);
	
	if (!ptr) 
		LOG_SIMPLE_ERR("MALLOC");

	return ptr;
}

void *ec_calloc(unsigned int nmem, unsigned int bytes)
{
	void *ptr;
	ptr = (calloc)(nmem,bytes);
	thinkd_log(LOG_DEBUG, "calloc: address=%p; nmem=%u; size=%u", ptr, nmem, bytes); 
	
	if (!ptr)
		LOG_SIMPLE_ERR("CALLOC");

	return ptr;
}
#endif
