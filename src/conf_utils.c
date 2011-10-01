#include "conf_utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>

#define MAX_SECTION_LEN 512

/* variables */
static ini_table_t **search_tab;

power_prefs_t mode_performance;
power_prefs_t mode_powersave;
power_prefs_t mode_heavy_powersave;
power_prefs_t mode_critical;

ini_table_t ini_table_defs[] = {
	{"bluetooth", ini_read_bool},
	{"nmi_watchdog", ini_read_bool}
};

static void debug_output(const char *path, const char *out, ...);
static void read_section(FILE *fp, power_prefs_t *prefs, size_t elems);

int read_ini()
{
	size_t num_elems;
	FILE *ini_fp;
	char buffer[MAX_SECTION_LEN];

	/* attempt to open the file */
	/* return errno on fail to be able to syslog it */
	ini_fp = fopen(THINKD_INI_FILE, "r");
	if (! ini_fp)
		return -1;

	/* initialize ini table */
	num_elems = alloc_ini_table();
	if (!num_elems)
		return -1;

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

			if (strcmp(bptr, "powersave") == 0)
				read_section(ini_fp,
					     &mode_powersave, num_elems);
			else if (strcmp(bptr, "performance") == 0)
				read_section(ini_fp,
					     &mode_performance, num_elems);
			else if (strcmp(bptr, "critical") == 0)
				read_section(ini_fp,
					     &mode_critical, num_elems);
			else if (strcmp(bptr, "heavy_powersave") == 0)
				read_section(ini_fp,
					     &mode_heavy_powersave, num_elems);
		}
		else
			continue; /* either misplaced rule, newline or NULL character, just skip and ignore */
	}

	fclose(ini_fp);
	free_ini_table();
	
	return 0;
}

static void read_section(FILE *fp, power_prefs_t *prefs, size_t elems)
{
	int idx = 0;
	
	if (! fp)
		return;

	while (idx < elems) {
		/* TODO: read key, value */
		/* check with key stored in search tab */
		/* if matches, set pointer to 0 */
		
		debug_output("/tmp/stab", "%s\n", search_tab[idx]->key);
		++idx;
	}
}

static void debug_output(const char *path, const char *out, ...)
{
	va_list args;
	FILE *fp;
	fp = fopen(path, "w");
	if (! fp)
		return;

	va_start(args, out);
	vfprintf(fp, out, args);
	fclose(fp);

	va_end(args);
}

size_t alloc_ini_table()
{
	int idx = 0;
	size_t tab_size, elems, alloc_size;
	tab_size = sizeof(ini_table_defs);
	elems = tab_size / sizeof(struct __ini_table);
	alloc_size = elems * sizeof(struct __ini_table*);
	
	search_tab = (ini_table_t**) malloc(alloc_size);
	if (! search_tab)
		return (size_t) 0;

	/* copy ini_table_defs */
	while (idx < elems) {
		search_tab[idx] = &ini_table_defs[idx];
		++idx;
	}
	
	return elems;
}

void free_ini_table()
{
	free(search_tab);
}

void ini_read_bool(void *store, const char *value)
{
}
