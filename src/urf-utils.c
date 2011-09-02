#include <stdlib.h>
#include <libudev.h>
#include "urf-utils.h"

/**
 * get_dmi_info:
 **/
DmiInfo *
get_dmi_info ()
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;
	struct udev_device *dev;
	DmiInfo *info = NULL;

	udev = udev_new ();
	if (!udev) {
		g_warning ("Cannot create udev");
		return NULL;
	}

	info = g_new0 (DmiInfo, 1);

	enumerate = udev_enumerate_new (udev);
	udev_enumerate_add_match_subsystem (enumerate, "dmi");
	udev_enumerate_scan_devices (enumerate);
	devices = udev_enumerate_get_list_entry (enumerate);

	udev_list_entry_foreach (dev_list_entry, devices) {
		const char *path;
		const char *attribute;
		path = udev_list_entry_get_name (dev_list_entry);
		dev = udev_device_new_from_syspath (udev, path);

		attribute = udev_device_get_sysattr_value (dev, "sys_vendor");
		if (attribute)
			info->sys_vendor = g_strdup (attribute);

		attribute = udev_device_get_sysattr_value (dev, "bios_date");
		if (attribute)
			info->bios_date = g_strdup (attribute);

		attribute = udev_device_get_sysattr_value (dev, "bios_vendor");
		if (attribute)
			info->bios_vendor = g_strdup (attribute);

		attribute = udev_device_get_sysattr_value (dev, "bios_version");
		if (attribute)
			info->bios_version = g_strdup (attribute);

		attribute = udev_device_get_sysattr_value (dev, "product_name");
		if (attribute)
			info->product_name = g_strdup (attribute);

		attribute = udev_device_get_sysattr_value (dev, "product_version");
		if (attribute)
			info->product_version = g_strdup (attribute);

		udev_device_unref (dev);
	}

	udev_enumerate_unref (enumerate);
	udev_unref (udev);

	return info;
}

/**
 * dmi_info_free:
 **/
void
dmi_info_free (DmiInfo *info)
{
	g_free (info->sys_vendor);
	g_free (info->bios_date);
	g_free (info->bios_vendor);
	g_free (info->bios_version);
	g_free (info->product_name);
	g_free (info->product_version);
	g_free (info);
}

/**
 * get_rfkill_device_by_index:
 **/
struct udev_device *
get_rfkill_device_by_index (struct udev *udev,
			    guint        index)
{
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;
	struct udev_device *dev;

	enumerate = udev_enumerate_new(udev);
	udev_enumerate_add_match_subsystem(enumerate, "rfkill");
	udev_enumerate_scan_devices(enumerate);
	devices = udev_enumerate_get_list_entry(enumerate);

	udev_list_entry_foreach(dev_list_entry, devices) {
		const char *path, *index_c;
		path = udev_list_entry_get_name(dev_list_entry);
		dev = udev_device_new_from_syspath(udev, path);

		index_c = udev_device_get_sysattr_value (dev, "index");
		if (index_c && atoi(index_c) == index)
			break;

		udev_device_unref (dev);
		dev = NULL;
	}

	return dev;
}

KillswitchState
event_to_state (gboolean soft,
		gboolean hard)
{
	if (hard)
		return KILLSWITCH_STATE_HARD_BLOCKED;
	else if (soft)
		return KILLSWITCH_STATE_SOFT_BLOCKED;
	else
		return KILLSWITCH_STATE_UNBLOCKED;
}

const char *
state_to_string (KillswitchState state)
{
	switch (state) {
	case KILLSWITCH_STATE_NO_ADAPTER:
		return "KILLSWITCH_STATE_NO_ADAPTER";
	case KILLSWITCH_STATE_SOFT_BLOCKED:
		return "KILLSWITCH_STATE_SOFT_BLOCKED";
	case KILLSWITCH_STATE_UNBLOCKED:
		return "KILLSWITCH_STATE_UNBLOCKED";
	case KILLSWITCH_STATE_HARD_BLOCKED:
		return "KILLSWITCH_STATE_HARD_BLOCKED";
	default:
		g_assert_not_reached ();
	}
}

const char *
type_to_string (guint type)
{
	switch (type) {
	case RFKILL_TYPE_ALL:
		return "ALL";
	case RFKILL_TYPE_WLAN:
		return "WLAN";
	case RFKILL_TYPE_BLUETOOTH:
		return "BLUETOOTH";
	case RFKILL_TYPE_UWB:
		return "UWB";
	case RFKILL_TYPE_WIMAX:
		return "WIMAX";
	case RFKILL_TYPE_WWAN:
		return "WWAN";
	case RFKILL_TYPE_GPS:
		return "GPS";
	case RFKILL_TYPE_FM:
		return "FM";
	default:
		g_assert_not_reached ();
	}
}
