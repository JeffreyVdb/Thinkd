#ifndef _LOGGER_H_
#define _LOGGER_H_

#define LOGERR_MSG(key) #key ": %d (%s)", errno, strerror(errno)

#include <syslog.h>

extern void thinkd_open_log();
extern void thinkd_close_log();
extern void thinkd_log(int priority, const char *format, ...);

#endif
