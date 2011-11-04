#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

#include "config.h"
#include "conf_utils.h"
#include "logger.h"

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
	{"bluetooth", OFFSET_OF(power_prefs_t, bluetooth), str_read_bool},
	{"nmi_watchdog", OFFSET_OF(power_prefs_t, nmi_watchdog), str_read_bool},
	{"wireless", OFFSET_OF(power_prefs_t, wireless), str_read_bool},
	{"wwan", OFFSET_OF(power_prefs_t, wwan), str_read_bool},
	{"brightness", OFFSET_OF(power_prefs_t, brightness), str_read_int},
	{"audio_powersave", OFFSET_OF(power_prefs_t, audio_powersave),str_read_bool}
};

/* static void debug_output(const char *path, const char *out, ...); */
static void read_section(FILE *fp, power_prefs_t *prefs);
static void search_tab_mv_end(unsigned int idx, unsigned int last_non_null);

int read_ini()
{
	FILE *ini_fp;
	char buffer[MAX_SECTION_LEN];

	/* attempt to open the file */
	/* return errno on fail to be able to thinkd_log it */
	ini_fp = fopen(THINKD_INI_FILE, "r");
	if (! ini_fp)
		return -1;

	/* read the ini file */
	while (fgets(buffer, MAX_SECTION_LEN, ini_fp)) {
		char *bptr = buffer;
		while (isblank(*bptr))
			++bptr;

		if (*bptr == ';' || *bptr != '[')
			continue; /* skip comments */
		
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

	fclose(ini_fp);
	
	return 0;
}

static void read_section(FILE *fp, power_prefs_t *prefs)
{
	char buffer[MAX_KEYVAL_LEN];
	size_t num_elems;
#ifdef CONF_DEBUG
	size_t tries = 0;
#endif
	
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
#ifdef EXTENDED_INI_SUPPORT
			size_t buflen;
			char *buf_pch = buffer;
			
			/* skip whitespace */
			while (isblank(*buf_pch))
				++buf_pch;

			switch (*buf_pch) {
			case ';':
			case '\n':
				continue;
				
			case '[':
				buflen = strlen(buf_pch);

				/* set read pointer back */
				fseek(fp, -buflen, SEEK_CUR);
				goto clean_table;
				
			default:
				thinkd_log(LOG_ERR,
				       "unresolved error in section, check ini file syntax");
				goto clean_table;
			}

#else /* EXTENDED_INI_SUPPORT = disabled */
			thinkd_log(LOG_INFO, "no '=' found, assuming next section.");
			break;
#endif
		}
		
		*eq_pch = '\0';

		/* find key */
		while (++idx < num_elems) {
			char *val_pch, *newl_pch;
#ifdef CONF_DEBUG
			++tries;
#endif
			if (! search_tab[idx])
				break; /* NULL elements in search tab
					  is not allowed */
			
			if (! strcasecmp(buffer, search_tab[idx]->key) == 0)
				continue;

			/* found key! */
			val_pch = eq_pch + 1;
			if (! val_pch)
				break;
			
			newl_pch = strchr(val_pch, '\n');
			if (! newl_pch) {
				thinkd_log(LOG_ERR, "Fatal error in ini file");
				goto clean_table;
			}

			*newl_pch = '\0';
			
			thinkd_log(LOG_INFO, "found %s in ini file with value %s.", buffer, val_pch);
			search_tab[idx]->handler((char *) prefs + search_tab[idx]->store_offset,
						 val_pch);
			
			/* Decrease length of search table, set this element to the end
			 so it becomes unreachable */
			--num_elems;
			search_tab_mv_end(idx,num_elems);
			break;
		}
	}
	
clean_table:
#ifdef CONF_DEBUG
	thinkd_log(LOG_INFO, "Read section in %d tries\n", (int) tries);
#endif
	free_ini_table();
}

static void search_tab_mv_end(unsigned int idx, unsigned int last_non_null)
{
	if (idx == last_non_null)
		search_tab[idx] = NULL;
	else {
		search_tab[idx] = search_tab[last_non_null];
		search_tab[last_non_null] = NULL;
	}
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
	search_tab = NULL;
}

void str_read_bool(void *store, const char *value)
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

void str_read_int(void *store, const char *value)
{
	int *dest = (int*) store;
	int power = 1;
	const char *a_end = value + strlen(value);

	*dest = 0;
	while (--a_end >= value) {
		*dest += (*a_end - '0') * power;
		power *= 10;
	}
}
