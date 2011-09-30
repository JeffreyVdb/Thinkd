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

/* global variables */
extern power_prefs_t mode_performance;
extern power_prefs_t mode_powersave;
extern power_prefs_t mode_heavy_powersave;
extern power_prefs_t mode_critical;

extern int read_ini();

#endif /* _CONF_UTILS_H_ */
