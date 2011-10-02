#ifndef _ACPI_H_
#define _ACPI_H_

#include <stdint.h>
#include <stdlib.h>

#define MAX_POW_SUPPLY_PATH 128
#define MAX_BATTERIES 2

typedef struct __acpi_psupply {
	char batteries[MAX_BATTERIES][MAX_POW_SUPPLY_PATH];
	char acdir[MAX_POW_SUPPLY_PATH];
} acpi_psupply_t;

extern int scan_power_supply(acpi_psupply_t *dest);
extern int sysfs_read_int(const char *path);
extern char *sysfs_read_str(char * dest, size_t len, const char *path);

#endif /* _ACPI_H_ */