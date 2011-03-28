/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@novell.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <libudev.h>
#include <linux/input.h>

#include <glib.h>

#define KEY_RELEASE 0
#define KEY_PRESS 1
#define KEY_KEEPING_PRESSED 2

#include "urf-input.h"

enum {
	RF_KEY_PRESSED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

#define URF_INPUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                URF_TYPE_INPUT, UrfInputPrivate))

struct UrfInputPrivate {
	int fd;
	guint watch_id;
	GIOChannel *channel;
};

G_DEFINE_TYPE(UrfInput, urf_input, G_TYPE_OBJECT)

static gboolean
input_dev_name_match (GHashTable *device_table,
		      const char *dev_name)
{
	gboolean ret = FALSE;

	if (g_hash_table_lookup (device_table, dev_name))
		ret = TRUE;

	return ret;
}

static GHashTable *
construct_device_table ()
{
	GHashTable *device_table = g_hash_table_new_full (g_str_hash,
							  g_str_equal,
							  g_free,
							  NULL);
	int i;
	const char *input_devices[] = {
		"AT Translated Set 2 keyboard",
		"Acer WMI hotkeys",
		"Asus EeePC extra buttons",
		"Asus Laptop extra buttons",
		"cmpc_keys",
		"Dell WMI hotkeys",
		"Eee PC WMI hotkeys",
		"MSI Laptop hotkeys",
		"Sony Vaio Keys",
		"ThinkPad Extra Buttons",
		"Topstar Laptop extra buttons",
		"Toshiba input device",
		"Wistron laptop buttons",
		NULL
	};

	for (i = 0; input_devices[i] != NULL; i++)
		g_hash_table_insert (device_table, (gpointer)g_strdup (input_devices[i]), "EXIST");

	return device_table;
}

static gboolean
input_event_cb (GIOChannel   *source,
		GIOCondition  condition,
		UrfInput     *input)
{
	if (condition & G_IO_IN) {
		GIOStatus status;
		struct input_event event;
		gsize read;

		status = g_io_channel_read_chars (source,
						  (char *) &event,
						  sizeof(event),
						  &read,
						  NULL);

		while (status == G_IO_STATUS_NORMAL && read == sizeof(event)) {
			if (event.value == KEY_PRESS) {
				switch (event.code) {
				case KEY_WLAN:
				case KEY_BLUETOOTH:
				case KEY_UWB:
				case KEY_WIMAX:
#ifdef KEY_RFKILL
				case KEY_RFKILL:
#endif
					g_signal_emit (G_OBJECT (input),
						       signals[RF_KEY_PRESSED],
						       0,
						       event.code);
					break;
				default:
					break;
				}
			}

			status = g_io_channel_read_chars (source,
							  (char *) &event,
							  sizeof(event),
							  &read,
							  NULL);
		}
	} else {
		g_debug ("something else happened");
		return FALSE;
	}

	return TRUE;
}

static gboolean
input_dev_open_channel (UrfInput   *input,
			const char *dev_node)
{
	UrfInputPrivate *priv = input->priv;
	int fd;

	fd = open(dev_node, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		if (errno == EACCES)
			g_warning ("Could not open %s", dev_node);
		return FALSE;
	}

	/* Setup a channel for the device node */
	priv->fd = fd;
	priv->channel = g_io_channel_unix_new (priv->fd);
	g_io_channel_set_encoding (priv->channel, NULL, NULL);
	priv->watch_id = g_io_add_watch (priv->channel,
					 G_IO_IN | G_IO_HUP | G_IO_ERR,
					 (GIOFunc) input_event_cb,
					 input);

	return TRUE;
}

gboolean
urf_input_startup (UrfInput *input)
{
	UrfInputPrivate *priv = input->priv;
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;
	struct udev_device *dev;
	struct udev_device *parent_dev;
	char *dev_node = NULL;
	GHashTable *device_table = NULL;
	gboolean ret;

	udev = udev_new ();
	if (!udev) {
		g_warning ("Cannot create udev object");
		return FALSE;
	}

	device_table = construct_device_table ();

	enumerate = udev_enumerate_new (udev);
	udev_enumerate_add_match_subsystem (enumerate, "input");
	udev_enumerate_scan_devices (enumerate);
	devices = udev_enumerate_get_list_entry (enumerate);

	udev_list_entry_foreach (dev_list_entry, devices) {
		const char *path;
		const char *dev_name;
		char *key;

		path = udev_list_entry_get_name (dev_list_entry);
		dev = udev_device_new_from_syspath (udev, path);
		parent_dev = udev_device_get_parent_with_subsystem_devtype (dev, "input", 0);
		if (!parent_dev) {
			udev_device_unref(dev);
			continue;
		}

		dev_name = udev_device_get_sysattr_value (parent_dev, "name");
		if (!input_dev_name_match (device_table, dev_name)) {
			udev_device_unref(dev);
			continue;
		}

		g_free (dev_node);
		dev_node = g_strdup (udev_device_get_devnode (dev));
		if (!dev_node) {
			udev_device_unref(dev);
			continue;
		}

		/* Use the device from the platform driver if there is one. */
		if (g_strcmp0 (dev_name, "AT Translated Set 2 keyboard") != 0) {
			udev_device_unref(dev);
			break;
		}

		udev_device_unref(dev);
	}
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	ret = input_dev_open_channel (input, dev_node);
	g_free (dev_node);

	g_hash_table_destroy (device_table);

	return ret;
}

/**
 * urf_input_init:
 **/
static void
urf_input_init (UrfInput *input)
{
	input->priv = URF_INPUT_GET_PRIVATE (input);
	input->priv->fd = -1;
	input->priv->channel = NULL;
}

/**
 * urf_input_finalize:
 **/
static void
urf_input_finalize (GObject *object)
{
	UrfInputPrivate *priv = URF_INPUT_GET_PRIVATE (object);

	if (priv->fd > 0) {
		g_source_remove (priv->fd);
		g_io_channel_shutdown (priv->channel, FALSE, NULL);
		g_io_channel_unref (priv->channel);
		close (priv->fd);
		priv->fd = 0;
	}

	G_OBJECT_CLASS(urf_input_parent_class)->finalize(object);
}

/**
 * urf_input_class_init:
 **/
static void
urf_input_class_init(UrfInputClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	g_type_class_add_private(klass, sizeof(UrfInputPrivate));
	object_class->finalize = urf_input_finalize;

	signals[RF_KEY_PRESSED] =
		g_signal_new ("rf-key-pressed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfInputClass, rf_key_pressed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);
}

/**
 * urf_input_new:
 **/
UrfInput *
urf_input_new (void)
{
	UrfInput *input;
	input = URF_INPUT(g_object_new (URF_TYPE_INPUT, NULL));
	return input;
}
