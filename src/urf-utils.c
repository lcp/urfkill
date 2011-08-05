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
