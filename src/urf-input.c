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

char *input_dev_table[] = { /* vendor_id:product_id */
	"0001:0001", /* AT Translated Set 2 Keyboard */
	"17aa:5054", /* ThinkPad Extra Buttons */
	NULL
};

enum {
	RF_KEY_PRESSED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

#define URF_INPUT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                URF_TYPE_INPUT, UrfInputPrivate))
typedef struct {
	int fd;
	guint watch_id;
	GIOChannel *channel;
} InputChannel;

struct UrfInputPrivate {
	GList *channel_list;
	GHashTable *device_table;
};

G_DEFINE_TYPE(UrfInput, urf_input, G_TYPE_OBJECT)

static gboolean
input_dev_id_match (UrfInput *input, const char *vendor, const char *product)
{
	UrfInputPrivate *priv = URF_INPUT_GET_PRIVATE (input);
	gboolean ret = FALSE;
	char *key;

	key = g_strconcat (vendor, ":", product, NULL);
	if (g_hash_table_lookup (priv->device_table, key))
		ret = TRUE;

	g_free (key);
	return ret;
}

static GHashTable *
construct_device_table ()
{
	GHashTable *device_table = g_hash_table_new (g_str_hash, g_str_equal);
	int i;

	for (i = 0; input_dev_table[i] != NULL; i++)
		g_hash_table_insert (device_table, input_dev_table[i], "EXIST");

	return device_table;
}

static gboolean
input_event_cb (GIOChannel *source, GIOCondition condition, UrfInput *input)
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
input_dev_open_channel (UrfInput *input, const char *dev_node)
{
	UrfInputPrivate *priv = URF_INPUT_GET_PRIVATE (input);
	int fd;
	InputChannel *channel;

	fd = open(dev_node, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		if (errno == EACCES)
			g_warning ("Could not open %s", dev_node);
		return FALSE;
	}

	/* Setup a channel for the device node */
	channel = g_new0 (InputChannel, 1);

	channel->fd = fd;
	channel->channel = g_io_channel_unix_new (channel->fd);
	g_io_channel_set_encoding (channel->channel, NULL, NULL);
	priv->channel_list = g_list_append (priv->channel_list, (gpointer)channel);

	channel->watch_id = g_io_add_watch (channel->channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR,
				(GIOFunc) input_event_cb,
				input);

	return TRUE;
}

gboolean
urf_input_startup (UrfInput *input)
{
	struct udev *udev;
	struct udev_enumerate *enumerate;
	struct udev_list_entry *devices;
	struct udev_list_entry *dev_list_entry;
	struct udev_device *dev;
	struct udev_device *parent_dev;

	udev = udev_new ();
	if (!udev) {
		g_warning ("Cannot create udev object");
		return FALSE;
	}

	enumerate = udev_enumerate_new (udev);
	udev_enumerate_add_match_subsystem (enumerate, "input");
	udev_enumerate_scan_devices (enumerate);
	devices = udev_enumerate_get_list_entry (enumerate);

	udev_list_entry_foreach (dev_list_entry, devices) {
		const char *path, *dev_node;
		const char *vendor, *product;
		char *key;

		path = udev_list_entry_get_name (dev_list_entry);
		dev = udev_device_new_from_syspath (udev, path);
		parent_dev = udev_device_get_parent_with_subsystem_devtype (dev, "input", 0);
		if (!parent_dev) {
			udev_device_unref(dev);
			continue;
		}

		vendor = udev_device_get_sysattr_value (parent_dev,"id/vendor");
		product = udev_device_get_sysattr_value (parent_dev, "id/product");
		if (!input_dev_id_match (input, vendor, product)) {
			udev_device_unref(dev);
			continue;
		}

		dev_node = udev_device_get_devnode (dev);
		if (!dev_node) {
			udev_device_unref(dev);
			continue;
		}

		if (!input_dev_open_channel (input, dev_node)) {
			udev_device_unref(dev);
			udev_enumerate_unref(enumerate);
			udev_unref(udev);
			return FALSE;
		}

		udev_device_unref(dev);
	}
	udev_enumerate_unref(enumerate);
	udev_unref(udev);

	return TRUE;
}

/**
 * urf_input_init:
 **/
static void
urf_input_init (UrfInput *input)
{
	UrfInputPrivate *priv = URF_INPUT_GET_PRIVATE (input);
	priv->channel_list = NULL;
	priv->device_table = construct_device_table ();
}

/**
 * urf_input_finalize:
 **/
static void
urf_input_finalize (GObject *object)
{
	UrfInputPrivate *priv = URF_INPUT_GET_PRIVATE (object);
	GList *item = g_list_first (priv->channel_list);

	while (item) {
		InputChannel *channel = (InputChannel *)item->data;

		if (channel->fd > 0) {
			g_source_remove (channel->fd);
			channel->fd = 0;
			g_io_channel_shutdown (channel->channel, FALSE, NULL);
			g_io_channel_unref (channel->channel);
		}
		close (channel->fd);
		g_free (channel);
		item = g_list_next (item);
	}

	g_list_free (priv->channel_list);

	g_hash_table_destroy (priv->device_table);

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
