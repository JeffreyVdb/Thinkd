#include "logger.h"
#include "thinkd.h"
#include "conf_utils.h"
#include "acpi.h"

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
#include <pthread.h>		

/* constants */
static const char *lockfile = THINKD_LOCKFILE;
static const char *pidfile = THINKD_PIDFILE;

/* static variables */
static power_prefs_t *current_mode = NULL;
static int sleep_time = BAT_SLEEP_TIME;
static bool probing = true;
static pthread_mutex_t conf_mutex;

/* function prototypes */
static void handle_cmd_args(int *argc, char ***argv);
static int open_log();
static bool daemonize();
static bool std2null();
static void clean_and_exit() THINKD_ATTR_NORET;
static void cleanup_before_exit();
static bool create_pidfile();
static void detect_psupply_mode();
static void load_psupply_mode(power_prefs_t *prefs);
static void print_usage(const struct option *opts, const char **opt_help);
static void load_config();
static void validate_user();
static void ipc_listen();

int main(int argc, char *argv[])
{
	handle_cmd_args(&argc, &argv);
	if (open_log() || ! daemonize()) {
		cleanup_before_exit();
		return 1;
	}

	/* Try to initialize the config mutex */
	if (pthread_mutex_init(&conf_mutex, NULL)) {
		LOG_SIMPLE_ERR("FATAL");
		clean_and_exit();
	}
	
	/* read in configuration */
	load_config();

	/* initial detecting of power supply */
	detect_psupply_mode();

	/* Listen for IPC requests */
	if (! probing) {
		ipc_listen();
		return 0;
	}

	/* do the never ending loop */
	while (1) {		
		sleep(sleep_time);
		detect_psupply_mode();
	}
	
	return 0;
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

	while ((c = getopt_long(*argc, *argv, "hvn", opts, &option_index)) != -1) {
		switch (c) {
		case 0:
			/* this option sets a flag */
			break;
		case 'n':
			/* stop detecting power mode */
			probing = false;
			break;
		case 'v':
			printf("%s %s\n", DAEMON_NAME, DAEMON_VERSION);
			clean_and_exit();
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

static int open_log()
{
	int retval = 0;
	
#ifdef USE_SYSLOG
	int log_opts;

	log_opts = LOG_NDELAY|LOG_PID|LOG_CONS;
	openlog(DAEMON_NAME, log_opts, LOG_DAEMON);
#else	
	if ((retval = thinkd_open_log()) != 0) {
		PRINT_SIMPLE_ERR("OPEN_LOG");
		return retval;
	}
#endif
	
	thinkd_log(LOG_INFO, "%s is starting, process %d (undaemonized).",
		   DAEMON_NAME, (int) getpid());
	return retval;
}

static bool daemonize()
{
	struct sigaction s_action;
	pid_t pid, sid;
	int lock_fd;

	/* Make sure the required user runs this process */
	validate_user();

	/* attempt to create the lockfile */
	if (lockfile && lockfile[0]) {
		lock_fd = open(lockfile, O_RDWR|O_CREAT|O_EXCL, (mode_t) 0640);
		if (lock_fd < 0) {
			lockfile = NULL;
			PRINT_SIMPLE_ERR("unable to create lock file");
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

	/* clean exit when these signals are sent */
	sigaction(SIGINT, &s_action, NULL);
	sigaction(SIGTERM, &s_action, NULL);
	sigaction(SIGQUIT, &s_action, NULL);

	/* SIGUSR1 to reload configuration */
	s_action.sa_handler = load_config;
	sigaction(SIGUSR1, &s_action, NULL);
	
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

	dnull_fd = open("/dev/null", O_RDWR, (mode_t) 0600);
	if (dnull_fd < 0) {
		thinkd_log(LOG_ERR, "cannot open devnull: %s", strerror(errno));
		return false;
	}

	/* redirect standard file handles to /dev/null
	   to prevent accidental output where not wished */
	if (dup2(dnull_fd, STDIN_FILENO) != STDIN_FILENO ||
	    dup2(dnull_fd, STDOUT_FILENO) != STDOUT_FILENO ||
	    dup2(dnull_fd, STDERR_FILENO) != STDERR_FILENO) {
		thinkd_log(LOG_ERR, "dup2: %s", strerror(errno));
		return false;
	}

	close(dnull_fd);

	return true;
}

static void clean_and_exit()
{
	cleanup_before_exit();
	exit(EXIT_SUCCESS);
}

static void cleanup_before_exit()
{
	thinkd_log(LOG_NOTICE, "%s process %d is stopping",
		   DAEMON_NAME, (int) getpid());
	thinkd_close_log();
	pthread_mutex_destroy(&conf_mutex);
	if (lockfile)
		unlink(lockfile);
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

static void detect_psupply_mode()
{
	char buffer[256];
	int state;
	int num_batts;
	acpi_psupply_t power_supply;

	if ((num_batts = scan_power_supply(&power_supply)) < 0) {
		thinkd_log(LOG_ERR, "failed to detect acpi power supply information");
		load_psupply_mode(&mode_powersave);
		return;
	}

	/* check if ac adapter is online */
	snprintf(buffer, 256, "%s/%s", power_supply.acdir, "online");
	state = sysfs_read_int(buffer);
	if (state) {
		if (current_mode == &mode_performance) return;
		load_psupply_mode(&mode_performance);
		return;
	}

	/* battery is connected */
	if (current_mode == &mode_powersave)
		return;

	load_psupply_mode(&mode_powersave);
}

static void load_psupply_mode(power_prefs_t *prefs)
{
	if (prefs == &mode_powersave) {
		thinkd_log(LOG_INFO, "Battery found. Enabling powersave mode");
		sleep_time = BAT_SLEEP_TIME;
	}
	else if (prefs == &mode_performance) {
		thinkd_log(LOG_INFO, "AC adapater is connected. Enabling performance mode");
		sleep_time = AC_SLEEP_TIME;
	}
	else {
		thinkd_log(LOG_INFO, "Mode unrecognized, setting sleep time to default");
		sleep_time = BAT_SLEEP_TIME;
		return;
	}
	
	pthread_mutex_lock(&conf_mutex);
	load_power_mode(prefs);
	current_mode = prefs;
	pthread_mutex_unlock(&conf_mutex);
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

static void load_config()
{	
	pthread_mutex_lock(&conf_mutex);
	read_ini();
	pthread_mutex_unlock(&conf_mutex);

	/* Load mode if one is already set by detect_psupply_mode() */
	if (current_mode)
		load_psupply_mode(current_mode);
}

static void validate_user()
{
	const uid_t required_uid = 0;
	if ((geteuid()) == required_uid)
		return;

	thinkd_log(LOG_ERR, "uid %d is required to run this process",
		   (int) required_uid);
	cleanup_before_exit();
	exit(EXIT_FAILURE);
}

static void ipc_listen()
{
	while (0) {
		/* wait for incoming socket requests */
	}
}
