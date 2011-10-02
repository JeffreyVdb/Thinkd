#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <signal.h>

#include "thinkd.h"
#include "conf_utils.h"
#include "acpi.h"

/* constants */
static const char *lockfile = THINKD_LOCKFILE;
static const char *pidfile = THINKD_PIDFILE;

/* default mode is powersave */
static power_prefs_t *current_mode = &mode_powersave;

/* Display help with options */
static void handle_cmd_args(int *argc, char ***argv);
static void open_log();
static bool daemonize();
static bool std2null();
static void clean_exit_status();
static bool create_pidfile();
static void detect_psupply_mode();

int main(int argc, char *argv[])
{	
	/* open logging interface first off */
	open_log();
	
	handle_cmd_args(&argc, &argv);
	if (! daemonize()) {
		clean_exit_status();
		return 1;
	}

	/* read in configuration */
	read_ini();

	/* initialize power_supply before loop */
	detect_psupply_mode();

	/* do the never ending loop */
	while (1) {
		sleep(30);
		detect_psupply_mode();
	}
	
	return 0;
}

static void detect_psupply_mode()
{
	int i = 0;
	int num_batts;
	acpi_psupply_t power_supply;

	if ((num_batts = scan_power_supply(&power_supply)) < 0) {
		syslog(LOG_ERR, "failed to detect acpi power supply information");
		current_mode = &mode_powersave;
		return;
	}

	/* some debugging */
	while (i < num_batts) {
		syslog(LOG_INFO, "found battery: %s", power_supply.batteries[i]);
		++i;
	}

	if (power_supply.acdir)
		syslog(LOG_INFO, "found ac adapter plugged in: %s", power_supply.acdir);
}

static void open_log()
{
	int log_opts;

	log_opts = LOG_NDELAY|LOG_PID|LOG_CONS;
	openlog(DAEMON_NAME, log_opts, LOG_DAEMON);

	syslog(LOG_INFO, "starting");
}

static bool create_pidfile()
{
	int pfd;

	/* delete pidfile if it exists */
	unlink(pidfile);
	
	/* check if the pidfile is defined */
	pfd = open(pidfile, O_WRONLY|O_CREAT|O_EXCL);
	if (pfd >= 0) {
		FILE *pfp;
		pfp = fdopen(pfd, "w");
		if (pfp) {
			fprintf(pfp, "%d\n", getpid());
			fclose(pfp);

			return true; /* leave pidfile open */
		}
		close(pfd);
	}

	syslog(LOG_ERR, "could not create pidfile %s (%s)", pidfile, strerror(errno));
	return false;
}

static void clean_exit_status()
{
	syslog(LOG_NOTICE, "exiting");
	closelog();
	unlink(lockfile);

	exit(EXIT_SUCCESS);
}

static void handle_cmd_args(int *argc, char ***argv)
{
	const struct option  opts[] = {
		{"help", 0, 0, 'h'},
		{NULL, 0, 0, 0}
	};

	const char * opts_help[] = {
		"print help message" /* help */
	};
}

static bool daemonize()
{
	struct sigaction s_action;
	pid_t pid, sid;
	int lock_fd;

	/* attempt to create the lockfile */
	if (lockfile && lockfile[0]) {
		lock_fd = open(lockfile, O_RDWR|O_CREAT|O_EXCL, (mode_t) 0640);
		if (lock_fd < 0) {
			syslog(LOG_ERR, "unable to create lock file %s, code=%d (%s)", lockfile, errno, strerror(errno));
			return false;
		}
	}

	/* try to fork */
	pid = fork();
	if (pid < 0) {
		return false;
	}

	/* Good pid, exit the parent process */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* now executing the child process */
	umask(0);

	/* create new sid for child process */
	sid = setsid();
	if (sid < 0) {
		syslog(LOG_ERR, "could not set sid");
		return false;
	}

	std2null();

	/* set signals */
	s_action.sa_handler = clean_exit_status;
	sigemptyset(&s_action.sa_mask);
	s_action.sa_flags = 0;
	
	sigaction(SIGINT, &s_action, NULL);
	sigaction(SIGTERM, &s_action, NULL);
	sigaction(SIGQUIT, &s_action, NULL);
	
	/* chdir to root directory */
	if (chdir("/") < 0) {
		syslog(LOG_ERR, "cannot into chdir. code %d (%s)", errno, strerror(errno));
		return false;
	}

	/* lastly create the pidfile */
	return create_pidfile();
}

static bool std2null()
{
	int dnull_fd;

	dnull_fd = open("/dev/null", O_RDWR);
	if (dnull_fd < 0) {
		syslog(LOG_ERR, "cannot open devnull: %s", strerror(errno));
		return false;
	}

	if (dup2(dnull_fd, STDIN_FILENO) != STDIN_FILENO ||
	    dup2(dnull_fd, STDOUT_FILENO) != STDOUT_FILENO ||
	    dup2(dnull_fd, STDERR_FILENO) != STDERR_FILENO) {
		syslog(LOG_ERR, "dup2: %s", strerror(errno));
		return false;
	}

	close(dnull_fd);

	return true;
}
