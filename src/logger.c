#include "logger.h"
#include "config.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>

FILE *err_logfile;
FILE *info_logfile;
	
static void create_dirs();

int thinkd_open_log()
{	
	/* check if directory exists */
	create_dirs();

	/* open the log files */
	info_logfile = fopen(LOG_INFO_PATH, "w");
	err_logfile = fopen(LOG_ERR_PATH, "w");
	if (!info_logfile || !err_logfile)
		return 1;
	
	setlinebuf(err_logfile);
	setlinebuf(info_logfile);
	return 0;
}

/*
  TODO: Create all parent dirs instead of the last one
*/
static void create_dirs()
{
	const char *log_paths[] = {
		LOG_INFO_PATH,
		LOG_ERR_PATH,
		NULL,
	};
	char buffer[1024];
	
	for (const char **paths = log_paths; *paths; ++paths) {
		DIR *directory;
		char *dir_sep_ptr;

		strcpy(buffer, *paths);
		dir_sep_ptr = strrchr(buffer, '/');
		if (! dir_sep_ptr)
			continue;

		*dir_sep_ptr = '\0';
		directory = opendir(buffer);
		if (! directory)
			mkdir(buffer, (mode_t) 0755);

		closedir(directory);
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
	size_t format_len;
	char * newl_format;
	
	/* we need to add an extra newline character */
	format_len = strlen(format);
	newl_format = (char *) calloc(format_len + 2, sizeof(char));
	strcpy(newl_format, format);
	newl_format[format_len] = '\n';
	
	/* syslog.h defines following constants */	
	switch (priority) {
	case LOG_INFO:
	case LOG_NOTICE:
		vfprintf(info_logfile, newl_format, args);
		break;

	case LOG_ERR:
		vfprintf(err_logfile, newl_format, args);
		break;

	default:
		vfprintf(info_logfile, newl_format, args);
		break;
	}
	
	free(newl_format);
#endif
	va_end(args);
}
