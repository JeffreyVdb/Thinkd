#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <glob.h>
#include <stdarg.h>

#include "acpi.h"
#include "logger.h"

#define POWER_SUPPLY_DIRECTORY "/sys/class/power_supply"
#define BACKLIGHT_DIRECTORY "/sys/class/backlight/acpi_video0"
#define HDA_INTEL_DIR "/sys/module/snd_hda_intel/"
#define AC97_DIR "/sys/module/snd_ac97_codec/"

static int pprintf(const char *path, const char *format, ...);
static void set_audio_state(audio_type_t type, const power_prefs_t *prefs);

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
		thinkd_log(LOG_ERR, "can't open power supply dir");
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
			thinkd_log(LOG_ERR, "could not open %s for reading: %s", psup_path,
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
		thinkd_log(LOG_ERR, "fopen: %d (%s)", errno, strerror(errno));
		return 0;
	}

	fscanf(fp, "%d", &result);
	fclose(fp);
	
	return result;
}

char *sysfs_read_str(char * dest, size_t len, const char *path)
{
	FILE *fp;
	char *pch;
	
	fp = fopen(path, "r");
	if (! fp) {
		thinkd_log(LOG_ERR, "fopen: %d (%s)", errno, strerror(errno));
		return NULL;
	}

	fgets(dest, len, fp);
	
	/* strip newline */
	pch = strchr(dest, '\n');
	if (pch)
		*pch = '\0';
	
	fclose(fp);
	
	return dest;
}

void set_nmi_watchdog(bool state)
{
	const char *nmi_path = "/proc/sys/kernel/nmi_watchdog";
	pprintf(nmi_path, "%u", (unsigned int) state);
}

void set_audio_powersaving(const power_prefs_t *prefs)
{
	DIR *dir;

	dir = opendir(HDA_INTEL_DIR);
	if (dir) {
		set_audio_state(HDA_INTEL, prefs);
		closedir(dir);
	}
	else if ((dir = opendir(AC97_DIR))) {
		set_audio_state(AC97, prefs);
		closedir(dir);
	}
}

static void set_audio_state(audio_type_t type, const power_prefs_t *prefs)
{
	const char *MUTE_ON = "mute";
	const char *MUTE_OFF = "unmute";
	const char *BASE_ACPI_PATH = THINKPAD_ACPI_DIR;
	const size_t BUFFER_SIZE = 512;
	int state = prefs->audio_powersave;
	char buffer[BUFFER_SIZE];
	
	switch (type) {
	case HDA_INTEL: {
		const char *work_path = HDA_INTEL_DIR "parameters";
		
		snprintf(buffer, BUFFER_SIZE, "%s/%s", work_path, "power_save_controller");
		pprintf(buffer, "%c", state ? 'Y' : 'N');
		snprintf(buffer, BUFFER_SIZE, "%s/%s", work_path, "power_save");
		pprintf(buffer, "%d", state ? 1 : 0);
		break;
	}
	case AC97:
		thinkd_log(LOG_INFO, "AC97 powersaving not supported");
		break;

	default:
		thinkd_log(LOG_ERR, "not found: %d", (int) type);
		break;
	}

	/* unmute or mute sound */
	snprintf(buffer, BUFFER_SIZE, "%s/%s", BASE_ACPI_PATH, "volume");
	pprintf(buffer, "%s", prefs->mute_state ? MUTE_ON : MUTE_OFF);
}

void load_power_mode(const power_prefs_t *prefs)
{
	const char *BASE_ACPI_PATH = THINKPAD_ACPI_DIR;
	const int VAL_SIZ = 256;
	char buffer[VAL_SIZ], state_path[VAL_SIZ];
	glob_t globbuf;
	int max_brightness, brightness_adjust;
	
	/* use glob to search for bluetooth rfkill */
	if (! glob("/sys/devices/platform/thinkpad_acpi/rfkill/rfkill?/name",
		 0, NULL, &globbuf) == 0) {
		thinkd_log(LOG_ERR, "glob: %s", strerror(errno));
		return;
	}
		
	for (char **p = globbuf.gl_pathv; *p; ++p) {
		memset(state_path, 0, VAL_SIZ);
		char *fsl_pch;
		size_t dirname_len;
		
		sysfs_read_str(buffer, VAL_SIZ, *p);
		fsl_pch = strrchr(*p, '/');
		dirname_len = ((fsl_pch+1) - *p);
		strncpy(state_path, *p, dirname_len);
		strcat(state_path, "state");
		
		if (strcmp(buffer, "tpacpi_bluetooth_sw") == 0) 
			pprintf(state_path, "%d", (int) prefs->bluetooth);
		else if (strcmp(buffer, "tpacpi_wwan_sw") == 0) 
			pprintf(state_path, "%d", (int) prefs->wwan);
	}
	
	globfree(&globbuf);

	/* set brightness */
	snprintf(buffer, VAL_SIZ, BACKLIGHT_DIRECTORY "/max_brightness");
	max_brightness = sysfs_read_int(buffer);

	brightness_adjust = prefs->brightness / (100 / max_brightness);
	if (brightness_adjust < 0)
		brightness_adjust = 0;
	else if (brightness_adjust > max_brightness)
		brightness_adjust = max_brightness;

	snprintf(buffer, VAL_SIZ, BACKLIGHT_DIRECTORY "/brightness");
	pprintf(buffer, "%d", brightness_adjust);

	/* set nmi_watchdog */
	set_nmi_watchdog(prefs->nmi_watchdog);
	set_audio_powersaving(prefs);

	snprintf(buffer, VAL_SIZ, "%s/%s", BASE_ACPI_PATH, "light");
	pprintf(buffer, "%s", prefs->thinklight_state ? "on" : "off");
}

static int pprintf(const char *path, const char *format, ...)
{
	va_list args;
	FILE *fp;
	int ret;
	
	fp = fopen(path, "w");
	if (! fp) {
		LOG_SIMPLE_ERR("fopen");
		return 0;
	}

	va_start(args, format);
	ret = vfprintf(fp, format, args);
	fclose(fp);

	va_end(args);
	return ret;
}
