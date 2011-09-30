#include "conf_utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>

#define MAX_SECTION_LEN 512

/* variables */
power_prefs_t mode_performance;
power_prefs_t mode_powersave;
power_prefs_t mode_heavy_powersave;
power_prefs_t mode_critical;

static void debug_output(const char *path, const char *out);
static void read_section(FILE *fp, power_prefs_t *prefs);

int read_ini()
{
	FILE *ini_fp;
	char buffer[MAX_SECTION_LEN];

	/* attempt to open the file */
	/* return errno on fail to be able to syslog it */
	ini_fp = fopen(THINKD_INI_FILE, "r");
	if (! ini_fp) {
		return errno;
	}

	/* read the ini file */
	while (fgets(buffer, MAX_SECTION_LEN, ini_fp)) {
		char *bptr = buffer;
		while (isblank(*bptr))
			++bptr;

		if (*bptr == ';')
			continue; /* skip comments */
		else if (*bptr == '[') {
			/* get section name */		       
			char *brackp = strchr(++bptr, ']');
			if (! brackp) break;
			*brackp = '\0';

			debug_output("/tmp/debug", bptr);

			if (strcmp(bptr, "powersave") == 0)
				read_section(ini_fp, &mode_powersave);
			else if (strcmp(bptr, "performance") == 0)
				read_section(ini_fp, &mode_performance);
			else if (strcmp(bptr, "critical") == 0)
				read_section(ini_fp, &mode_critical);
			else if (strcmp(bptr, "heavy_powersave") == 0)
				read_section(ini_fp, &mode_heavy_powersave);
		}
		else
			continue; /* either misplaced rule, newline or NULL character, just skip and ignore */
	}

	fclose(ini_fp);
	return 0;
}

static void read_section(FILE *fp, power_prefs_t *prefs)
{
	if (! fp)
		return;

	
}

static void debug_output(const char *path, const char *out)
{
	FILE *fp;
	fp = fopen(path, "w");
	if (! fp)
		return;

	fprintf(fp, out);
	fclose(fp);
}
