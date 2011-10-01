#include "conf_utils.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>
#include <stdarg.h>
#include <syslog.h>

#define OFFSET_OF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define MAX_SECTION_LEN 512
#define MAX_KEYVAL_LEN 512

/* variables */
static ini_table_t **search_tab;

power_prefs_t mode_performance;
power_prefs_t mode_powersave;
power_prefs_t mode_heavy_powersave;
power_prefs_t mode_critical;

ini_table_t ini_table_defs[] = {
	{"bluetooth", OFFSET_OF(power_prefs_t, bluetooth), ini_read_bool},
	{"nmi_watchdog", OFFSET_OF(power_prefs_t, nmi_watchdog), ini_read_bool},
	{"wireless", OFFSET_OF(power_prefs_t, wireless), ini_read_bool}
};

/* static void debug_output(const char *path, const char *out, ...); */
static void read_section(FILE *fp, power_prefs_t *prefs);

int read_ini()
{
	FILE *ini_fp;
	char buffer[MAX_SECTION_LEN];

	/* attempt to open the file */
	/* return errno on fail to be able to syslog it */
	ini_fp = fopen(THINKD_INI_FILE, "r");
	if (! ini_fp)
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
	char buffer[MAX_KEYVAL_LEN];
	size_t num_elems;
	
	if (! fp)
		return;

	/* initialize ini table */
	num_elems = alloc_ini_table();
	if (!num_elems)
		return;

	while (fgets(buffer, MAX_KEYVAL_LEN, fp)) {
		int idx = -1;
		char *eq_pch;
		/* TODO: support whitespace */
		eq_pch = strchr(buffer, '=');
		if (! eq_pch) {
			syslog(LOG_INFO, "no '=' found, assuming next section.");
			break;
		}

		*eq_pch = '\0';

		/* find key */
		while (++idx < num_elems) {
			char *val_pch, *newl_pch;
			if (!search_tab[idx] || ! strcasecmp(buffer, search_tab[idx]->key) == 0)
				continue;

			/* found key! */
			val_pch = eq_pch + 1;
			if (! val_pch)
				break;
			
			newl_pch = strchr(val_pch, '\n');
			if (! newl_pch) {
				syslog(LOG_ERR, "Fatal error in ini file");
				goto clean_table;
			}

			*newl_pch = '\0';
			
			syslog(LOG_INFO, "found %s in ini file with value %s.", buffer, val_pch);
			search_tab[idx]->handler((char *) prefs + search_tab[idx]->store_offset,
						 val_pch);
			
			/* remove this entry from the search tab */
			search_tab[idx] = NULL;
			break;
		}
	}
	
clean_table:
	free_ini_table();
}

/* static void debug_output(const char *path, const char *out, ...) 
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
} */

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
	search_tab = NULL;
}

void ini_read_bool(void *store, const char *value)
{
	bool *dest = (bool*) store;
	
	if (strcasecmp(value, "enabled") == 0 ||
	    strcasecmp(value, "on") == 0 ||
	    strcasecmp(value, "true") == 0 ||
	    strcasecmp(value, "1") == 0)
		*dest = true;
	else
		*dest = false;
}
