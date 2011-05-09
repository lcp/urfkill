/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005-2008 Marcel Holtmann <marcel@holtmann.org>
 * Copyright (C) 2006-2009 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2010-2011 Gary Ching-Pang Lin <glin@novell.com>
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
#include <string.h>
#include <sys/ioctl.h>

#include <glib.h>

#include <linux/rfkill.h>

#ifndef RFKILL_EVENT_SIZE_V1
#define RFKILL_EVENT_SIZE_V1    8
#endif

#include "urf-killswitch.h"
#include "urf-utils.h"

enum {
	RFKILL_ADDED,
	RFKILL_REMOVED,
	RFKILL_CHANGED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

#define URF_KILLSWITCH_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                URF_TYPE_KILLSWITCH, UrfKillswitchPrivate))

struct UrfKillswitchPrivate {
	int		 fd;
	gboolean	 force_sync;
	GIOChannel	*channel;
	guint		 watch_id;
	GList		*devices; /* a GList of UrfDevice */
	GHashTable	*type_map;
	UrfDevice	*type_pivot[NUM_RFKILL_TYPES];
};

G_DEFINE_TYPE(UrfKillswitch, urf_killswitch, G_TYPE_OBJECT)

static KillswitchState
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

static const char *
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

static const char *
type_to_string (unsigned int type)
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

/**
 * urf_killswitch_set_state:
 **/
gboolean
urf_killswitch_set_state (UrfKillswitch  *killswitch,
			  guint           type,
			  KillswitchState state)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	struct rfkill_event event;
	ssize_t len;

	g_return_val_if_fail (state != KILLSWITCH_STATE_HARD_BLOCKED, FALSE);
	g_return_val_if_fail (type < NUM_RFKILL_TYPES, FALSE);

	memset (&event, 0, sizeof(event));
	event.op = RFKILL_OP_CHANGE_ALL;
	event.type = type;
	if (state == KILLSWITCH_STATE_SOFT_BLOCKED)
		event.soft = 1;
	else if (state == KILLSWITCH_STATE_UNBLOCKED)
		event.soft = 0;
	else
		g_assert_not_reached ();

	g_debug ("Set %s to %s", type_to_string (type), state_to_string (state));
	len = write (priv->fd, &event, sizeof(event));
	if (len < 0) {
		g_warning ("Failed to change RFKILL state: %s",
			   g_strerror (errno));
		return FALSE;
	}
	return TRUE;
}

/**
 * urf_killswitch_set_state_idx:
 **/
gboolean
urf_killswitch_set_state_idx (UrfKillswitch  *killswitch,
			      guint           index,
			      KillswitchState state)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	UrfDevice *device;
	struct rfkill_event event;
	ssize_t len;
	GList *l;
	gboolean found = FALSE;

	g_return_val_if_fail (state != KILLSWITCH_STATE_HARD_BLOCKED, FALSE);

	for (l = priv->devices; l; l = l->next) {
		device = (UrfDevice *)l->data;
		if (urf_device_get_index (device) == index) {
			found = TRUE;
			break;
		}
	}

	if (!found) {
		g_warning ("Index not found: %u", index);
		return FALSE;
	}

	memset (&event, 0, sizeof(event));
	event.op = RFKILL_OP_CHANGE;
	event.idx = index;
	if (state == KILLSWITCH_STATE_SOFT_BLOCKED)
		event.soft = 1;
	else if (state == KILLSWITCH_STATE_UNBLOCKED)
		event.soft = 0;
	else
		g_assert_not_reached ();

	g_debug ("Set device %u to %s", index, state_to_string (state));
	len = write (priv->fd, &event, sizeof(event));
	if (len < 0) {
		g_warning ("Failed to change RFKILL state: %s",
			   g_strerror (errno));
		return FALSE;
	}
	return TRUE;
}

static KillswitchState
aggregate_pivot_state (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	UrfDevice *device;
	int state = KILLSWITCH_STATE_NO_ADAPTER;
	int i;
	gboolean soft, hard;

	for (i = 0; i < NUM_RFKILL_TYPES; i++) {
		if (!priv->type_pivot[i])
			continue;

		device = priv->type_pivot[i];
		soft = urf_device_get_soft (device);
		hard = urf_device_get_hard (device);
		switch (event_to_state (soft, hard)) {
			case KILLSWITCH_STATE_UNBLOCKED:
				if (state == KILLSWITCH_STATE_NO_ADAPTER)
					state = KILLSWITCH_STATE_UNBLOCKED;
				break;
			case KILLSWITCH_STATE_SOFT_BLOCKED:
				state = KILLSWITCH_STATE_SOFT_BLOCKED;
				break;
			case KILLSWITCH_STATE_HARD_BLOCKED:
				return KILLSWITCH_STATE_HARD_BLOCKED;
			default:
				break;
		}
	}

	return state;
}

/**
 * urf_killswitch_get_state:
 **/
KillswitchState
urf_killswitch_get_state (UrfKillswitch *killswitch,
			  guint          type)
{
	UrfKillswitchPrivate *priv;
	UrfDevice *device;
	int state = KILLSWITCH_STATE_NO_ADAPTER;
	gboolean soft, hard;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), state);
	g_return_val_if_fail (type < NUM_RFKILL_TYPES, state);

	priv = killswitch->priv;

	if (priv->devices == NULL)
		return KILLSWITCH_STATE_NO_ADAPTER;

	if (type == RFKILL_TYPE_ALL)
		return aggregate_pivot_state (killswitch);

	if (priv->type_pivot[type]) {
		device = priv->type_pivot[type];
		state = event_to_state (urf_device_get_soft (device),
					urf_device_get_hard (device));
	}

	g_debug ("devices %s state %s",
		 type_to_string (type), state_to_string (state));

	return state;
}

/**
 * urf_killswitch_get_state_idx:
 **/
KillswitchState
urf_killswitch_get_state_idx (UrfKillswitch *killswitch,
			      guint          index)
{
	UrfKillswitchPrivate *priv;
	UrfDevice *device;
	GList *l;
	gboolean soft, hard;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), KILLSWITCH_STATE_NO_ADAPTER);

	priv = killswitch->priv;

	if (priv->devices == NULL)
		return KILLSWITCH_STATE_NO_ADAPTER;

	for (l = priv->devices ; l ; l = l->next) {
		device = (UrfDevice *)l->data;
		if (urf_device_get_index (device) == index) {
			int state;
			soft = urf_device_get_soft (device);
			hard = urf_device_get_hard (device);
			state = event_to_state (soft, hard);
			g_debug ("killswitch %d is %s", index, state_to_string (state));
			return state;
		}
	}

	return KILLSWITCH_STATE_NO_ADAPTER;
}

/**
 * urf_killswitch_has_devices:
 **/
gboolean
urf_killswitch_has_devices (UrfKillswitch *killswitch)
{
	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), FALSE);

	return (killswitch->priv->devices != NULL);
}

/**
 * urf_killswitch_get_devices:
 **/
GList*
urf_killswitch_get_devices (UrfKillswitch *killswitch)
{
	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), NULL);

	return killswitch->priv->devices;
}

/**
 * urf_killswitch_get_killswitch:
 **/
UrfDevice *
urf_killswitch_get_device (UrfKillswitch *killswitch,
			   const guint    index)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	UrfDevice *device;
	GList *item;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), NULL);

	for (item = priv->devices; item; item = g_list_next (item)) {
		device = (UrfDevice *)item->data;
		if (urf_device_get_index (device) == index)
			return URF_DEVICE (g_object_ref (device));
	}

	return NULL;
}

static gboolean
match_platform_vendor (const char *name) {
	/* The vendor names which are generated by platform drivers */
	static const char *platform_vendors[] = {
		"acer", /* acer-wmi */
		"asus", /* asus-laptop */
		"cmpc", /* classmate-laptop */
		"compal", /* compal-laptop */
		"dell", /* dell-laptop */
		"eeepc", /* eeepc-laptop, eeepc-wmi */
		"hp", /* hp-wmi */
		"ideapad", /* ideapad-laptop */
		"msi", /* msi-laptop */
		"sony", /* sony-laptop */
		"tpacpi", /* thinkpad_acpi */
		"Toshiba", /* toshiba_acpi */
		NULL
	};

	int i;

	for (i = 0; platform_vendors[i] != NULL; i++) {
		if ( g_str_has_prefix (name, platform_vendors[i]))
			return TRUE;
	}

	return FALSE;
}

/**
 * update_killswitch:
 **/
static void
update_killswitch (UrfKillswitch *killswitch,
		   guint          index,
		   gboolean       soft,
		   gboolean       hard)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	UrfDevice *device;
	GList *l;
	guint type;
	gboolean old_soft, old_hard;
	gboolean changed = FALSE;

	for (l = priv->devices; l != NULL; l = l->next) {
		device = (UrfDevice *)l->data;
		if (urf_device_get_index (device) == index) {
			old_soft = urf_device_get_soft (device);
			old_hard = urf_device_get_hard (device);
			if (old_soft != soft || old_hard != hard) {
				g_object_set (device,
					      "soft", soft,
					      "hard", hard,
					      NULL);
				changed = TRUE;
			}
			break;
		}
	}

	if (changed != FALSE) {
		g_debug ("updating killswitch status %d to soft %d hard %d",
			 index, soft, hard);
		g_signal_emit (G_OBJECT (killswitch), signals[RFKILL_CHANGED], 0, index);

		if (priv->force_sync) {
			/* Sync soft and hard blocks */
			if (hard == 1 && soft == 0)
				urf_killswitch_set_state_idx (killswitch, index, KILLSWITCH_STATE_SOFT_BLOCKED);
			else if (hard != old_hard && hard == 0)
				urf_killswitch_set_state_idx (killswitch, index, KILLSWITCH_STATE_UNBLOCKED);
		}
	}
}

/**
 * remove_killswitch:
 **/
static void
remove_killswitch (UrfKillswitch *killswitch,
		   guint          index)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	UrfDevice *device;
	GList *l;
	guint type;
	const char *name;
	gboolean pivot_changed = FALSE;
	char *object_path = NULL;

	for (l = priv->devices; l != NULL; l = l->next) {
		device = (UrfDevice *)l->data;
		if (urf_device_get_index (device) == index) {

			type = urf_device_get_rf_type (device);
			priv->devices = g_list_remove (priv->devices, device);
			object_path = g_strdup (urf_device_get_object_path(device));

			name = urf_device_get_name (device);
			g_debug ("removing killswitch idx %d %s", index, name);

			if (priv->type_pivot[type] == device) {
				priv->type_pivot[type] = NULL;
				pivot_changed = TRUE;
			}

			g_object_unref (device);
			break;
		}
	}

	/* Find the next pivot */
	if (pivot_changed) {
		for (l = priv->devices; l != NULL; l = l->next) {
			device = (UrfDevice *)l->data;
			name = urf_device_get_name (device);
			if (urf_device_get_rf_type (device) == type &&
			    (priv->type_pivot[type] == NULL || match_platform_vendor (name))) {
				priv->type_pivot[type] = device;
				g_debug ("assign killswitch idx %d %s as a pivot",
					 urf_device_get_index (device), name);
			}
		}
	}
	g_signal_emit (G_OBJECT (killswitch), signals[RFKILL_REMOVED], 0, object_path);
	if (object_path != NULL)
		g_free (object_path);
}

/**
 * add_killswitch:
 **/
static void
add_killswitch (UrfKillswitch *killswitch,
		guint          index,
		guint          type,
		gboolean       soft,
		gboolean       hard)

{
	UrfKillswitchPrivate *priv = killswitch->priv;
	UrfDevice *device;
	const char *name;

	g_debug ("adding killswitch idx %d soft %d hard %d", index, soft, hard);

	device = urf_device_new (index, type, soft, hard);
	priv->devices = g_list_append (priv->devices, device);

	/* Assume that only one platform vendor in a machine */
	name = urf_device_get_name (device);
	if (priv->type_pivot[type] == NULL || match_platform_vendor (name)) {
		priv->type_pivot[type] = device;
		g_debug ("assign killswitch idx %d %s as a pivot", index, name);
	}

	g_signal_emit (G_OBJECT (killswitch), signals[RFKILL_ADDED], 0,
		       urf_device_get_object_path (device));
	if (priv->force_sync && priv->type_pivot[type] != device) {
		int state = event_to_state (soft, hard);
		urf_killswitch_set_state_idx (killswitch, index, state);
	}
}

/**
 * urf_killswitch_rf_type:
 **/
gint
urf_killswitch_rf_type (UrfKillswitch *killswitch,
			const char    *type_name)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	gint *type;
	char *key = g_ascii_strup (type_name, -1);

	type = g_hash_table_lookup(priv->type_map, key);
	g_free (key);

	if (type == NULL)
		return -1;

	return *type;
}

static const char *
op_to_string (unsigned int op)
{
	switch (op) {
	case RFKILL_OP_ADD:
		return "ADD";
	case RFKILL_OP_DEL:
		return "DEL";
	case RFKILL_OP_CHANGE:
		return "CHANGE";
	case RFKILL_OP_CHANGE_ALL:
		return "CHANGE_ALL";
	default:
		g_assert_not_reached ();
	}
}

static void
print_event (struct rfkill_event *event)
{
	g_debug ("RFKILL event: idx %u type %u (%s) op %u (%s) soft %u hard %u",
		 event->idx,
		 event->type, type_to_string (event->type),
		 event->op, op_to_string (event->op),
		 event->soft, event->hard);
}

/**
 * event_cb:
 **/
static gboolean
event_cb (GIOChannel    *source,
	  GIOCondition   condition,
	  UrfKillswitch *killswitch)
{
	if (condition & G_IO_IN) {
		GIOStatus status;
		struct rfkill_event event;
		gsize read;
		gboolean soft, hard;

		status = g_io_channel_read_chars (source,
						  (char *) &event,
						  sizeof(event),
						  &read,
						  NULL);

		while (status == G_IO_STATUS_NORMAL && read == sizeof(event)) {
			print_event (&event);

			soft = (event.soft > 0)?TRUE:FALSE;
			hard = (event.hard > 0)?TRUE:FALSE;

			if (event.op == RFKILL_OP_CHANGE) {
				update_killswitch (killswitch, event.idx, soft, hard);
			} else if (event.op == RFKILL_OP_DEL) {
				remove_killswitch (killswitch, event.idx);
			} else if (event.op == RFKILL_OP_ADD) {
				add_killswitch (killswitch, event.idx, event.type, soft, hard);
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

static GHashTable*
construct_type_map ()
{
	GHashTable *map;
	unsigned int *value;
	int i;

	map = g_hash_table_new_full (g_str_hash, g_int_equal, g_free, g_free);

	for (i =0 ; i< NUM_RFKILL_TYPES; i++) {
		value = g_malloc0 (sizeof(unsigned int));
		*value = i;
		g_hash_table_insert(map, g_strdup (type_to_string (i)), value);
	}

	return map;
}

/**
 * urf_killswitch_startup
 **/
gboolean
urf_killswitch_startup (UrfKillswitch *killswitch,
			UrfConfig     *config)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	struct rfkill_event event;
	int fd;

	priv->force_sync = urf_config_get_force_sync (config);

	fd = open("/dev/rfkill", O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		if (errno == EACCES)
			g_warning ("Could not open RFKILL control device, please verify your installation");
		return FALSE;
	}

	/* Disable rfkill input */
	ioctl(fd, RFKILL_IOCTL_NOINPUT);

	priv->fd = fd;

	while (1) {
		ssize_t len;

		len = read(fd, &event, sizeof(event));
		if (len < 0) {
			if (errno == EAGAIN)
				break;
			g_debug ("Reading of RFKILL events failed");
			break;
		}

		if (len != RFKILL_EVENT_SIZE_V1) {
			g_warning("Wrong size of RFKILL event\n");
			continue;
		}

		if (event.op != RFKILL_OP_ADD)
			continue;
		if (event.type >= NUM_RFKILL_TYPES)
			continue;

		add_killswitch (killswitch, event.idx, event.type, event.soft, event.hard);
	}

	/* Setup monitoring */
	priv->channel = g_io_channel_unix_new (priv->fd);
	g_io_channel_set_encoding (priv->channel, NULL, NULL);
	priv->watch_id = g_io_add_watch (priv->channel,
					 G_IO_IN | G_IO_HUP | G_IO_ERR,
					 (GIOFunc) event_cb,
					 killswitch);
	return TRUE;
}

/**
 * urf_killswitch_init:
 **/
static void
urf_killswitch_init (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	int i;

	killswitch->priv = priv;
	priv->type_map = construct_type_map ();
	priv->devices = NULL;
	priv->fd = -1;

	for (i = 0; i < NUM_RFKILL_TYPES; i++)
		priv->type_pivot[i] = NULL;
}

/**
 * urf_killswitch_finalize:
 **/
static void
urf_killswitch_finalize (GObject *object)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (object);

	/* cleanup monitoring */
	if (priv->watch_id > 0) {
		g_source_remove (priv->watch_id);
		priv->watch_id = 0;
		g_io_channel_shutdown (priv->channel, FALSE, NULL);
		g_io_channel_unref (priv->channel);
	}
	close(priv->fd);

	g_list_foreach (priv->devices, (GFunc) g_object_unref, NULL);
	g_list_free (priv->devices);
	priv->devices = NULL;

	g_hash_table_destroy (priv->type_map);

	G_OBJECT_CLASS(urf_killswitch_parent_class)->finalize(object);
}

/**
 * urf_killswitch_class_init:
 **/
static void
urf_killswitch_class_init(UrfKillswitchClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	g_type_class_add_private(klass, sizeof(UrfKillswitchPrivate));
	object_class->finalize = urf_killswitch_finalize;

	signals[RFKILL_ADDED] =
		g_signal_new ("rfkill-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfKillswitchClass, rfkill_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[RFKILL_REMOVED] =
		g_signal_new ("rfkill-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfKillswitchClass, rfkill_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[RFKILL_CHANGED] =
		g_signal_new ("rfkill-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfKillswitchClass, rfkill_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);

}

/**
 * urf_killswitch_new:
 **/
UrfKillswitch *
urf_killswitch_new (void)
{
	UrfKillswitch *killswitch;
	killswitch = URF_KILLSWITCH(g_object_new (URF_TYPE_KILLSWITCH, NULL));
	return killswitch;
}

