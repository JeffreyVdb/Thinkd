#include "logger.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

FILE *err_logfile;
FILE *info_logfile;

static char *log_paths[] = {
	LOG_INFO_PATH,
	LOG_ERR_PATH,
	NULL
};
	
static void create_dirs();

void thinkd_open_log()
{	
	/* check if directory exists */
	create_dirs();

	/* open the log files */
	info_logfile = fopen(log_paths[0], "w");
	err_logfile = fopen(log_paths[1], "w");
}

static void create_dirs()
{
	char *dir_sep_ptr;
	char **paths = log_paths;

	while (*paths) {
		DIR *directory;
		
		dir_sep_ptr = strrchr(*paths, '/');
		if (! dir_sep_ptr)
			continue;

		*dir_sep_ptr = '\0';
		directory = opendir(*paths);
		if (! directory) {
			mkdir(*paths, (mode_t) 0755);
			++paths;
			continue;
		}

		closedir(directory);
		++paths;
	}
}

void thinkd_close_log()
{
#ifdef USE_SYSLOG
	closelog();
#else
	if (!err_logfile || !info_logfile)
		return;

	fclose(err_logfile);
	fclose(info_logfile);
#endif
}

void thinkd_log(int priority, const char *format, ...)
{
	va_list args;

	va_start(args, format);
#ifdef USE_SYSLOG
	vsyslog(priority,format,args);
#else
	/* syslog.h defines following constants */
	switch (priority) {
	case LOG_INFO:
	case LOG_NOTICE:
		vfprintf(info_logfile, format, args);
		break;

	case LOG_ERR:
		vfprintf(err_logfile, format, args);
		break;
	}
#endif
	va_end(args);
}
