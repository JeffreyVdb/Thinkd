#ifndef _ACPI_H_
#define _ACPI_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "conf_utils.h"

#define STANDARD_RFKILL_DIR	"/sys/class/rfkill"
#define THINKPAD_PROC_ACPI_DIR	"/proc/acpi/ibm"
#define THINKPAD_SYS_ACPI_DIR	"/sys/devices/platform/thinkpad_acpi"
#define MAX_SYSFS_STR_LEN 64
#define MAX_SYSFS_PATH_LEN 512
#define MAX_PROCFS_PATH_LEN MAX_SYSFS_PATH_LEN
#define MAX_PROCFS_STR_LEN MAX_SYSFS_STR_LEN
#define MAX_POW_SUPPLY_PATH 128
#define MAX_BATTERIES 2
#define sysfs_sprintf(str, format, ...) \
	snprintf(str, MAX_SYSFS_PATH_LEN, format, __VA_ARGS__)
#define procfs_sprintf(str, format, ...) \
	snprintf(str, MAX_PROCFS_PATH_LEN, format, __VA_ARGS__)
#define sysfs_gets(dest, path) \
	sysfs_read_str(dest, MAX_SYSFS_PATH_LEN, path)

typedef enum __audio_type {
	HDA_INTEL,
	AC97,
} audio_type_t;

typedef char sysfs_value_t[MAX_SYSFS_STR_LEN];
typedef char procfs_value_t[MAX_PROCFS_STR_LEN];
typedef char sysfs_path_t[MAX_SYSFS_PATH_LEN];
typedef char procfs_path_t[MAX_PROCFS_PATH_LEN];
typedef struct __acpi_psupply {
	char batteries[MAX_BATTERIES][MAX_POW_SUPPLY_PATH];
	char acdir[MAX_POW_SUPPLY_PATH];
} acpi_psupply_t;

extern int scan_power_supply(acpi_psupply_t *dest);
extern int sysfs_read_int(const char *path);
extern char *sysfs_read_str(char * dest, size_t len, const char *path);
extern void load_power_mode(const power_prefs_t *prefs);
extern void set_nmi_watchdog(bool state);
extern void set_audio_powersaving(const power_prefs_t *prefs);
extern void set_thinklight_state(bool state);

#endif /* _ACPI_H_ */
