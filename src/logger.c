#define _BSD_SOURCE 1

#include "logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>

#define try_lock_log_files() __lock_log_files(LOCK_EX | LOCK_NB)
#define unlock_log_files() __lock_log_files(LOCK_UN | LOCK_NB)

static FILE *err_logfile;
static FILE *info_logfile;
#if _DEBUG_LOG == 1
static FILE *debug_logfile;
#endif

static void create_dirs();
static int __lock_log_files(int mode);

/*
  TODO: cut down on the #ifs
*/
int thinkd_open_log()
{
	int fds[2 + _DEBUG_LOG];
	int i, res = 0;
	FILE *nullfh;
	struct stat logstat;
	
	/* check if directory exists */
	create_dirs();

	/* open the log files */
	info_logfile = fopen(LOG_INFO_PATH, "a");
	err_logfile = fopen(LOG_ERR_PATH, "a");
#if _DEBUG_LOG == 1
	debug_logfile = fopen(LOG_DEBUG_PATH, "a");
	res = (int) (! debug_logfile);
#endif
	res += (int) (!info_logfile || !err_logfile);
	if (res)
		return 1;

	fds[0] = fileno(info_logfile);
	fds[1] = fileno(err_logfile);
#if _DEBUG_LOG == 1
	fds[2] = fileno(debug_logfile);
#endif
	if ((try_lock_log_files()) != 0) { /* can't get LOCK */
		int nullfd;
		
		nullfh = fopen("/dev/null", "w");
		if (! nullfh) {
			PRINT_SIMPLE_ERR("FOPEN");
			return 1;
		}
		
		nullfd = fileno(nullfh);

		/* redirect log files to void */
		for (i = 0; i < STATIC_ARRAY_LEN(fds,int); ++i) {
			if ((dup2(nullfd, fds[i])) != fds[i]) 
				PRINT_SIMPLE_ERR("dup2");
		}			

		fclose(nullfh);
		return 1;
	}

	/* at this point the lock is ours */
	for (i = 0; i < STATIC_ARRAY_LEN(fds, int); ++i) {
		fstat(fds[i], &logstat);
		if (logstat.st_size > MAX_LOG_SIZE) {
			freopen(LOG_INFO_PATH, "w", info_logfile);
			freopen(LOG_ERR_PATH, "w", err_logfile);
#if _DEBUG_LOG == 1
			freopen(LOG_DEBUG_PATH, "w", debug_logfile);
#endif
			break;
		}
	}
	
	setlinebuf(err_logfile);
	setlinebuf(info_logfile);
#if _DEBUG_LOG == 1
	setlinebuf(debug_logfile);
#endif
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

static int __lock_log_files(int mode)
{
	int fds[2 + _DEBUG_LOG];
	int i;
	
	fds[0] = fileno(info_logfile);
	fds[1] = fileno(err_logfile);
#if _DEBUG_LOG == 1
	fds[2] = fileno(debug_logfile);
#endif
	
        for (i = 0; i < STATIC_ARRAY_LEN(fds,int); ++i) {
		if ((flock(fds[i], mode)) == -1) {
			PRINT_SIMPLE_ERR("LOCK");
			return 1;
		}
	}

	return 0;
}

void thinkd_close_log()
{
#ifdef USE_SYSLOG
	closelog();
#else
	int res = 0;
 #if _DEBUG_LOG == 1
	res += (int) (!debug_logfile);
 #endif
	res += (int) (!err_logfile || !info_logfile);
	if (res)
		return;

	/* raise locks from files */
	unlock_log_files();
	
	fclose(err_logfile);
	fclose(info_logfile);
 #if _DEBUG_LOG == 1
	fclose(debug_logfile);
 #endif
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
#if _DEBUG_LOG == 1
	case LOG_DEBUG:
		vfprintf(debug_logfile, newl_format, args);
		break;
#endif
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
