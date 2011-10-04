#include "acpi.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <syslog.h>
#include <sys/types.h>
#include <dirent.h>

#define POWER_SUPPLY_DIRECTORY "/sys/class/power_supply"

int scan_power_supply(acpi_psupply_t *dest)
{
	int batt_count = 0;
	FILE * psup_fp;
	char psup_path[MAX_POW_SUPPLY_PATH];
	char *name;
	DIR *psup_dir;
	struct dirent *psup_entry;

	psup_dir = opendir(POWER_SUPPLY_DIRECTORY);
	if (! psup_dir) {
		syslog(LOG_ERR, "can't open power supply dir");
		return -1;
	}

	while ((psup_entry = readdir(psup_dir))) {
		char tmp_type[8]; /* either Mains or Battery */
		name = psup_entry->d_name;

		/* skip .. and . */
		if (! strncmp(".", name, 1) || ! strncmp("..", name, 2))
			continue;
		sprintf(psup_path, POWER_SUPPLY_DIRECTORY "/%s/type", name);
		psup_fp = fopen(psup_path, "r");
		if (! psup_fp) {
			syslog(LOG_ERR, "could not open %s for reading: %s", psup_path,
			       strerror(errno));
			continue;
		}

		/* read type into tmp_type */
		fgets(tmp_type, 8, psup_fp);
		fclose(psup_fp);
		
		if (strncmp("Battery", tmp_type, 7) == 0) {
			sprintf(dest->batteries[batt_count],
				POWER_SUPPLY_DIRECTORY "/%s", name);
			++batt_count;
			if (batt_count == MAX_BATTERIES)
				break;
		}
		else if (strncmp("Mains", tmp_type, 5) == 0) {
			sprintf(dest->acdir,
				POWER_SUPPLY_DIRECTORY "/%s", name);
		}		
	}

	closedir(psup_dir);

	return batt_count;
}

int sysfs_read_int(const char *path)
{
	int result;
	FILE *fp;

	fp = fopen(path, "r");
	if (! fp) {
		syslog(LOG_ERR, "fopen: %d (%s)", errno, strerror(errno));
		return 0;
	}

	fscanf(fp, "%d", &result);
	fclose(fp);
	
	return result;
}

char *sysfs_read_str(char * dest, size_t len, const char *path)
{
	FILE *fp;

	fp = fopen(path, "r");
	if (! fp) {
		syslog(LOG_ERR, "fopen: %d (%s)", errno, strerror(errno));
		return NULL;
	}

	fgets(dest, len, fp);
	fclose(fp);
	
	return dest;
}
