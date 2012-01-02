#ifndef _LOGGER_H_
#define _LOGGER_H_

#define LOGERR_FORMAT(key) key ": %d (%s)"
#define LOGERR_MSG(key) LOGERR_FORMAT(key), errno, strerror(errno)
#define LOG_SIMPLE_ERR(mesg) thinkd_log(LOG_ERR, LOGERR_MSG(mesg))
#define PRINT_SIMPLE_ERR(mesg) \
	fprintf(stderr, LOGERR_FORMAT(mesg) "\n", errno, strerror(errno))

#include <syslog.h>

extern int thinkd_open_log();
extern void thinkd_close_log();
extern void thinkd_log(int priority, const char *format, ...);

#endif
