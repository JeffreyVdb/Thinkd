#ifndef _CONF_UTILS_H_
#define _CONF_UTILS_H_

#include <stdbool.h>

/* this makes our configuration a little more clear */
#define ENABLED true
#define DISABLED false

#define THINKD_INI_FILE "/etc/thinkd.ini"

typedef struct __power_prefs {
	bool bluetooth;
	bool nmi_watchdog;
} power_prefs_t;

typedef struct __ini_table {
	const char *key;
	void (*handler)(void *, const char*);
} ini_table_t;

/* global variables */
extern ini_table_t ini_table_defs[];
extern power_prefs_t mode_performance;
extern power_prefs_t mode_powersave;
extern power_prefs_t mode_heavy_powersave;
extern power_prefs_t mode_critical;

extern ini_table_t* alloc_ini_table();
extern void free_ini_table();
extern int read_ini();
extern void ini_read_bool(void *store, const char *value);

#endif /* _CONF_UTILS_H_ */
