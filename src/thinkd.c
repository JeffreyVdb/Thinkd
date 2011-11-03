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

#include "config.h"
#include "logger.h"
#include "thinkd.h"
#include "conf_utils.h"
#include "acpi.h"

/* constants */
static const char *lockfile = THINKD_LOCKFILE;
static const char *pidfile = THINKD_PIDFILE;

/* default mode is powersave */
static power_prefs_t *current_mode = NULL;
static int sleep_time = BAT_SLEEP_TIME;
static bool probing = true;

/* Display help with options */
static void handle_cmd_args(int *argc, char ***argv);
static void open_log();
static bool daemonize();
static bool std2null();
static void clean_and_exit();
static void cleanup_before_exit();
static bool create_pidfile();
static void detect_psupply_mode();
static void print_usage(const struct option *opts, const char **opt_help);

int main(int argc, char *argv[])
{	
	/* open logging interface first off */
	open_log();
	
	handle_cmd_args(&argc, &argv);
	if (! daemonize()) {
		cleanup_before_exit();
		return 1;
	}

	/* read in configuration */
	read_ini();

	/* initialize power_supply before loop */
	detect_psupply_mode();

	/*
	  TODO: start thread listening
	*/
	
	if (! probing) {
		/* TODO: */
		/* simply wait for thread to end */
		return 0;
	}

	/* do the never ending loop */
	while (1) {		
		sleep(sleep_time);
		detect_psupply_mode();
	}
	
	return 0;
}

static void detect_psupply_mode()
{
	char buffer[256];
	int state;
	int num_batts;
	acpi_psupply_t power_supply;

	if ((num_batts = scan_power_supply(&power_supply)) < 0) {
		thinkd_log(LOG_ERR, "failed to detect acpi power supply information");
		current_mode = &mode_powersave;
		return;
	}

	/* check if ac adapter is online */
	snprintf(buffer, 256, "%s/%s", power_supply.acdir, "online");
	state = sysfs_read_int(buffer);
	if (state) {
		if (current_mode == &mode_performance) return;
		
		thinkd_log(LOG_INFO, "ac adapater is connected, enabling performance mode");
		load_power_mode(&mode_performance);
		current_mode = &mode_performance;
		sleep_time = AC_SLEEP_TIME;

		return;
	}

	/* battery is connected */
	if (current_mode == &mode_powersave)
		return;

	thinkd_log(LOG_INFO, "changing to powersave mode");
	load_power_mode(&mode_powersave);
	current_mode = &mode_powersave;
	sleep_time = BAT_SLEEP_TIME;
}

static void open_log()
{
#ifdef USE_SYSLOG
	int log_opts;

	log_opts = LOG_NDELAY|LOG_PID|LOG_CONS;
	openlog(DAEMON_NAME, log_opts, LOG_DAEMON);
#else
	thinkd_open_log();
#endif
	
	thinkd_log(LOG_INFO, "starting");
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

	thinkd_log(LOG_ERR, "could not create pidfile %s (%s)", pidfile, strerror(errno));
	return false;
}

static void cleanup_before_exit()
{
	thinkd_log(LOG_NOTICE, "exiting");
	thinkd_close_log();
	unlink(lockfile);
}

static void clean_and_exit()
{
	cleanup_before_exit();
	exit(EXIT_SUCCESS);
}

static void handle_cmd_args(int *argc, char ***argv)
{
	int c, option_index;
	const struct option  opts[] = {
		{"help", 0, 0, 'h'},
		{"version", 0, 0, 'v'},
		{"no-probe", 0, 0, 'n'},
		{NULL, 0, 0, 0}
	};

	const char * opts_help[] = {
		"print help message", /* help */
		"print version of this program", /* version */
		"do not try detecting the power mode" /* no-probe */
	};

	while ((c = getopt_long(*argc, *argv, "hn", opts, &option_index)) != -1) {
		switch (c) {
		case 0:
			/* this option sets a flag */
			break;
		case 'n':
			/* stop detecting power mode */
			probing = false;
			break;
		case 'h':
			print_usage(opts, opts_help);
			clean_and_exit();
		case '?':
		default:
			print_usage(opts, opts_help);
			cleanup_before_exit();
			exit(EXIT_FAILURE);
			break;
		}
	}
}

static void print_usage(const struct option *opts, const char **opt_help)
{
	const struct option *opt;
	const char **help;
	size_t longest_opt = (size_t) 0, siz;
	
	/* find longest options */
	for (opt = opts; opt->name; ++opt) {
		if ((siz = strlen(opt->name)) > longest_opt)
			longest_opt = siz;
	}

	fprintf(stdout, "Usage for %s\n\nOPTIONS:\n", DAEMON_NAME);
	for (opt = opts, help = opt_help; opt->name; ++opt, ++help) {
		fprintf(stdout, "\t-%-5c --%-*s\t%s\n",
			opt->val, (int) longest_opt, opt->name, *help);
	}
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
			thinkd_log(LOG_ERR, "unable to create lock file %s, code=%d (%s)", lockfile, errno, strerror(errno));
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
		thinkd_log(LOG_ERR, "could not set sid");
		return false;
	}

	std2null();

	/* set signals */
	s_action.sa_handler = clean_and_exit;
	sigemptyset(&s_action.sa_mask);
	s_action.sa_flags = 0;
	
	sigaction(SIGINT, &s_action, NULL);
	sigaction(SIGTERM, &s_action, NULL);
	sigaction(SIGQUIT, &s_action, NULL);
	
	/* chdir to root directory */
	if (chdir("/") < 0) {
		thinkd_log(LOG_ERR, "cannot into chdir. code %d (%s)", errno, strerror(errno));
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
		thinkd_log(LOG_ERR, "cannot open devnull: %s", strerror(errno));
		return false;
	}

	if (dup2(dnull_fd, STDIN_FILENO) != STDIN_FILENO ||
	    dup2(dnull_fd, STDOUT_FILENO) != STDOUT_FILENO ||
	    dup2(dnull_fd, STDERR_FILENO) != STDERR_FILENO) {
		thinkd_log(LOG_ERR, "dup2: %s", strerror(errno));
		return false;
	}

	close(dnull_fd);

	return true;
}
