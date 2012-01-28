#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <syslog.h>
#include "void.h"
#include "config.h"

#define LOGERR_FORMAT(key) key ": %d (%s)"
#define LOGERR_MSG(key) LOGERR_FORMAT(key), errno, strerror(errno)
#define LOG_SIMPLE_ERR(mesg) thinkd_log(LOG_ERR, LOGERR_MSG(mesg))
#define PRINT_SIMPLE_ERR(mesg)						\
	fprintf(stderr, LOGERR_FORMAT(mesg) "\n", errno, strerror(errno))

#ifndef MAX_LOG_SIZE
#  define MAX_LOG_SIZE 8192
#endif

#ifndef _DEBUG_LOG
#  define _DEBUG_LOG 1
#else
#  if _DEBUG_LOG > 1
#    define _DEBUG_LOG 1
#  endif
#endif

#ifndef LOG_DEBUG
#  ifdef USE_SYSLOG
#    define LOG_DEBUG LOG_INFO
#  else
#    define LOG_DEBUG LOG_INFO|LOG_NOTICE
#  endif
#endif

extern int thinkd_open_log();
extern void thinkd_close_log();
extern void thinkd_log(int priority, const char *format, ...) THINKD_ATTR_PRINTF(2);

#endif
