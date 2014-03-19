/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005-2008 Marcel Holtmann <marcel@holtmann.org>
 * Copyright (C) 2006-2009 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2010-2011 Gary Ching-Pang Lin <glin@suse.com>
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
#include <sys/ioctl.h>

#include <glib.h>

#ifndef RFKILL_EVENT_SIZE_V1
#define RFKILL_EVENT_SIZE_V1    8
#endif

#include "urf-config.h"
#include "urf-arbitrator.h"
#include "urf-killswitch.h"
#include "urf-utils.h"

#include "urf-device.h"
#include "urf-device-kernel.h"

enum {
	DEVICE_ADDED,
	DEVICE_REMOVED,
	DEVICE_CHANGED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

#define URF_ARBITRATOR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                URF_TYPE_ARBITRATOR, UrfArbitratorPrivate))

struct UrfArbitratorPrivate {
	int		 fd;
	UrfConfig	*config;
	gboolean	 force_sync;
	gboolean	 persist;
	GIOChannel	*channel;
	guint		 watch_id;
	GList		*devices; /* a GList of UrfDevice */
	UrfKillswitch	*killswitch[NUM_RFKILL_TYPES];
};

G_DEFINE_TYPE(UrfArbitrator, urf_arbitrator, G_TYPE_OBJECT)

/**
 * urf_arbitrator_find_device:
 **/
static UrfDevice *
urf_arbitrator_find_device (UrfArbitrator *arbitrator,
                            guint          index)
{
	UrfArbitratorPrivate *priv = arbitrator->priv;
	UrfDevice *device;
	GList *item;

	for (item = priv->devices; item != NULL; item = item->next) {
		device = (UrfDevice *)item->data;
		if (urf_device_get_index (device) == index)
			return device;
	}

	return NULL;
}

/**
 * urf_arbitrator_set_block:
 **/
gboolean
urf_arbitrator_set_block (UrfArbitrator  *arbitrator,
			  const guint     type,
			  const gboolean  block)
{
	UrfArbitratorPrivate *priv = arbitrator->priv;
	gboolean result = FALSE;

	g_return_val_if_fail (type < NUM_RFKILL_TYPES, FALSE);

	g_message ("Setting %s devices to %s",
                   type_to_string (type),
                   block ? "blocked" : "unblocked");

	result = urf_killswitch_set_software_blocked (priv->killswitch[type], block);
	if (!result)
		g_warning ("No device with type %u to block", type);
	else {
		urf_config_set_persist_state (priv->config, type, block);
	}

	return result;
}

/**
 * urf_arbitrator_set_block_idx:
 **/
gboolean
urf_arbitrator_set_block_idx (UrfArbitrator  *arbitrator,
			      const guint     index,
			      const gboolean  block)
{
	UrfDevice *device;
	gboolean result = FALSE;

	device = urf_arbitrator_find_device (arbitrator, index);

	if (device) {
		g_message ("Setting device %u (%s) to %s",
                           index,
                           type_to_string (urf_device_get_device_type (device)),
                           block ? "blocked" : "unblocked");

		result = urf_device_set_software_blocked (device, block);
	} else {
		g_warning ("Block index: No device with index %u", index);
	}

	return result;
}

/**
 * urf_arbitrator_set_flight_mode:
 **/
gboolean
urf_arbitrator_set_flight_mode (UrfArbitrator  *arbitrator,
				const gboolean  block)
{
	UrfArbitratorPrivate *priv = arbitrator->priv;
	KillswitchState state = KILLSWITCH_STATE_NO_ADAPTER;
	KillswitchState saved_state = KILLSWITCH_STATE_NO_ADAPTER;
	gboolean want_state = FALSE;
	gboolean ret = FALSE;
	int i;

	g_message("set_flight_mode: %d:", (int) block);

        urf_config_set_persist_state (priv->config, RFKILL_TYPE_ALL, block);

	for (i = RFKILL_TYPE_ALL + 1; i < NUM_RFKILL_TYPES; i++) {
		state = urf_killswitch_get_state (priv->killswitch[i]);

		if (state != KILLSWITCH_STATE_NO_ADAPTER) {
			g_message("killswitch[%s] state: %s", type_to_string(i),
				  state_to_string(state));

			saved_state = urf_killswitch_get_saved_state(priv->killswitch[i]);
			g_debug("saved_state is: %s", state_to_string(saved_state));

			if (block)
				urf_killswitch_set_saved_state(priv->killswitch[i], state);

			if (!block && state == saved_state)
				want_state = (gboolean)(saved_state > KILLSWITCH_STATE_UNBLOCKED);
			else
				want_state = block;

			g_debug ("calling set_block %s %s",
				 type_to_string(i),
				 want_state ? "TRUE" : "FALSE");

			ret = urf_arbitrator_set_block (arbitrator, i, want_state);
		}
	}

	return ret;
}

/**
 * urf_arbitrator_get_state:
 **/
KillswitchState
urf_arbitrator_get_state (UrfArbitrator *arbitrator,
			  guint          type)
{
	UrfArbitratorPrivate *priv;
	int state = KILLSWITCH_STATE_NO_ADAPTER;

	g_return_val_if_fail (URF_IS_ARBITRATOR (arbitrator), state);
	g_return_val_if_fail (type < NUM_RFKILL_TYPES, state);

	priv = arbitrator->priv;

	if (type == RFKILL_TYPE_ALL)
		type = RFKILL_TYPE_WLAN;
	state = urf_killswitch_get_state (priv->killswitch[type]);

	g_debug ("devices %s state %s",
		 type_to_string (type), state_to_string (state));

	return state;
}

/**
 * urf_arbitrator_get_state_idx:
 **/
KillswitchState
urf_arbitrator_get_state_idx (UrfArbitrator *arbitrator,
			      guint          index)
{
	UrfArbitratorPrivate *priv;
	UrfDevice *device;
	int state = KILLSWITCH_STATE_NO_ADAPTER;

	g_return_val_if_fail (URF_IS_ARBITRATOR (arbitrator), state);

	priv = arbitrator->priv;

	if (priv->devices == NULL)
		return state;

	device = urf_arbitrator_find_device (arbitrator, index);
	if (device) {
		state = urf_device_get_state (device);
		g_debug ("killswitch %d is %s", index, state_to_string (state));
	}

	return state;
}

/**
 * urf_arbitrator_add_device:
 **/
gboolean
urf_arbitrator_add_device (UrfArbitrator *arbitrator, UrfDevice *device)
{
	UrfArbitratorPrivate *priv;
	guint type;
	guint index;
	gboolean soft;

	g_return_val_if_fail (URF_IS_ARBITRATOR (arbitrator), FALSE);
	g_return_val_if_fail (URF_IS_DEVICE (device), FALSE);

	priv = arbitrator->priv;
	type = urf_device_get_device_type (device);
	index = urf_device_get_index (device);
	soft = urf_device_is_software_blocked (device);

	arbitrator->priv->devices = g_list_append (arbitrator->priv->devices, device);

	urf_killswitch_add_device (arbitrator->priv->killswitch[type], device);

	g_signal_emit (G_OBJECT (arbitrator), signals[DEVICE_ADDED], 0,
		       urf_device_get_object_path (device));

	if (priv->force_sync && !urf_device_is_platform (device)) {
		urf_arbitrator_set_block_idx (arbitrator, index, soft);
	}

	if (priv->persist) {
		/* use the saved persistence state as a default state to
		 * use for the new killswitch.
		 *
		 * This makes sure devices that appear after urfkill has
		 * started still get to the right state from what was saved
		 * to the persistence file.
		 */
		soft = urf_config_get_persist_state (priv->config, type);
		urf_arbitrator_set_block_idx (arbitrator, index, soft);
	}

	return TRUE;
}

/**
 * urf_arbitrator_remove_device:
 **/
gboolean
urf_arbitrator_remove_device (UrfArbitrator *arbitrator, UrfDevice *device)
{
	guint type;

	g_return_val_if_fail (URF_IS_ARBITRATOR (arbitrator), FALSE);
	g_return_val_if_fail (URF_IS_DEVICE (device), FALSE);

	type = urf_device_get_device_type (device);

	arbitrator->priv->devices = g_list_remove (arbitrator->priv->devices, device);

	urf_killswitch_del_device (arbitrator->priv->killswitch[type], device);

	g_signal_emit (G_OBJECT (arbitrator), signals[DEVICE_REMOVED], 0,
	               urf_device_get_object_path (device));

	return TRUE;
}

/**
 * urf_arbitrator_has_devices:
 **/
gboolean
urf_arbitrator_has_devices (UrfArbitrator *arbitrator)
{
	g_return_val_if_fail (URF_IS_ARBITRATOR (arbitrator), FALSE);

	return (arbitrator->priv->devices != NULL);
}

/**
 * urf_arbitrator_get_devices:
 **/
GList*
urf_arbitrator_get_devices (UrfArbitrator *arbitrator)
{
	g_return_val_if_fail (URF_IS_ARBITRATOR (arbitrator), NULL);

	return arbitrator->priv->devices;
}

/**
 * urf_arbitrator_get_arbitrator:
 **/
UrfDevice *
urf_arbitrator_get_device (UrfArbitrator *arbitrator,
			   const guint    index)
{
	UrfDevice *device;

	g_return_val_if_fail (URF_IS_ARBITRATOR (arbitrator), NULL);

	device = urf_arbitrator_find_device (arbitrator, index);
	if (device)
		return URF_DEVICE (g_object_ref (device));

	return NULL;
}

/**
 * update_killswitch:
 **/
static void
update_killswitch (UrfArbitrator *arbitrator,
		   guint          index,
		   gboolean       soft,
		   gboolean       hard)
{
	UrfArbitratorPrivate *priv = arbitrator->priv;
	UrfDevice *device;
	gboolean changed, old_hard = FALSE;
	char *object_path;

	device = urf_arbitrator_find_device (arbitrator, index);
	if (device == NULL) {
		g_warning ("No device with index %u in the list", index);
		return;
	}

	old_hard = urf_device_is_hardware_blocked (device);

	changed = urf_device_update_states (device, soft, hard);

	if (changed == TRUE) {
		g_debug ("updating killswitch status %d to soft %d hard %d",
			 index, soft, hard);
		object_path = g_strdup (urf_device_get_object_path (device));
		g_signal_emit (G_OBJECT (arbitrator), signals[DEVICE_CHANGED], 0, object_path);
		g_free (object_path);

		if (priv->force_sync) {
			/* Sync soft and hard blocks */
			if (hard == TRUE && soft == FALSE)
				urf_arbitrator_set_block_idx (arbitrator, index, TRUE);
			else if (hard != old_hard && hard == FALSE)
				urf_arbitrator_set_block_idx (arbitrator, index, FALSE);
		}
	}
}

/**
 * remove_killswitch:
 **/
static void
remove_killswitch (UrfArbitrator *arbitrator,
		   guint          index)
{
	UrfArbitratorPrivate *priv = arbitrator->priv;
	UrfDevice *device;
	guint type;
	const char *name;
	char *object_path = NULL;

	device = urf_arbitrator_find_device (arbitrator, index);
	if (device == NULL) {
		g_warning ("No device with index %u in the list", index);
		return;
	}

	priv->devices = g_list_remove (priv->devices, device);
	type = urf_device_get_device_type (device);
	object_path = g_strdup (urf_device_get_object_path(device));

	name = urf_device_get_name (device);
	g_message ("removing killswitch idx %d %s", index, name);

	urf_killswitch_del_device (priv->killswitch[type], device);
	g_object_unref (device);

	g_signal_emit (G_OBJECT (arbitrator), signals[DEVICE_REMOVED], 0, object_path);
	g_free (object_path);
}

/**
 * add_killswitch:
 **/
static void
add_killswitch (UrfArbitrator *arbitrator,
		guint          index,
		guint          type,
		gboolean       soft,
		gboolean       hard)

{
	UrfArbitratorPrivate *priv = arbitrator->priv;
	UrfDevice *device;

	device = urf_arbitrator_find_device (arbitrator, index);
	if (device != NULL) {
		g_warning ("device with index %u already in the list", index);
		return;
	}

	g_message ("adding killswitch idx %d soft %d hard %d", index, soft, hard);

	device = urf_device_kernel_new (index, type, soft, hard);
	priv->devices = g_list_append (priv->devices, device);

	urf_killswitch_add_device (priv->killswitch[type], device);
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
	  UrfArbitrator *arbitrator)
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
				update_killswitch (arbitrator, event.idx, soft, hard);
			} else if (event.op == RFKILL_OP_DEL) {
				remove_killswitch (arbitrator, event.idx);
			} else if (event.op == RFKILL_OP_ADD) {
				add_killswitch (arbitrator, event.idx, event.type, soft, hard);
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

/**
 * urf_arbitrator_startup
 **/
gboolean
urf_arbitrator_startup (UrfArbitrator *arbitrator,
			UrfConfig     *config)
{
	UrfArbitratorPrivate *priv = arbitrator->priv;
	struct rfkill_event event;
	int fd;
	int i;

	priv->config = g_object_ref (config);
	priv->force_sync = urf_config_get_force_sync (config);
	priv->persist =	urf_config_get_persist (config);

	fd = open("/dev/rfkill", O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		if (errno == EACCES)
			g_warning ("Could not open RFKILL control device, please verify your installation");
		return FALSE;
	}

	/* Set initial flight mode state from persistence */
	if (priv->persist)
		urf_arbitrator_set_flight_mode (arbitrator,
		                                urf_config_get_persist_state (config, RFKILL_TYPE_ALL));

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

		add_killswitch (arbitrator, event.idx, event.type, event.soft, event.hard);
	}

	/* Setup monitoring */
	priv->channel = g_io_channel_unix_new (priv->fd);
	g_io_channel_set_encoding (priv->channel, NULL, NULL);
	priv->watch_id = g_io_add_watch (priv->channel,
					 G_IO_IN | G_IO_HUP | G_IO_ERR,
					 (GIOFunc) event_cb,
					 arbitrator);

	if (priv->persist) {
		/* Set all the devices that had saved state to what was saved */ 
		for (i = RFKILL_TYPE_ALL + 1; i < NUM_RFKILL_TYPES; i++) {
			urf_arbitrator_set_block (arbitrator, i, urf_config_get_persist_state (config, i));
		}
	}

	return TRUE;
}

/**
 * urf_arbitrator_init:
 **/
static void
urf_arbitrator_init (UrfArbitrator *arbitrator)
{
	UrfArbitratorPrivate *priv = URF_ARBITRATOR_GET_PRIVATE (arbitrator);
	int i;

	arbitrator->priv = priv;
	priv->devices = NULL;
	priv->fd = -1;

	priv->killswitch[RFKILL_TYPE_ALL] = NULL;
	for (i = RFKILL_TYPE_ALL + 1; i < NUM_RFKILL_TYPES; i++)
		priv->killswitch[i] = urf_killswitch_new (i);
}

/**
 * urf_arbitrator_dispose:
 **/
static void
urf_arbitrator_dispose (GObject *object)
{
	UrfArbitratorPrivate *priv = URF_ARBITRATOR_GET_PRIVATE (object);
	KillswitchState state;
	int i;

	if (priv->persist) {
		for (i = RFKILL_TYPE_ALL + 1; i < NUM_RFKILL_TYPES; i++) {
			state = urf_killswitch_get_state (priv->killswitch[i]);
                        g_debug("dispose arbitrator: state for %d is %d", i, state);
			urf_config_set_persist_state (priv->config, i, state);
		}
	}

	for (i = 0; i < NUM_RFKILL_TYPES; i++) {
		if (priv->killswitch[i]) {
			g_object_unref (priv->killswitch[i]);
			priv->killswitch[i] = NULL;
		}
	}

	if (priv->devices) {
		g_list_foreach (priv->devices, (GFunc) g_object_unref, NULL);
		g_list_free (priv->devices);
		priv->devices = NULL;
	}

	if (priv->config) {
		g_object_unref (priv->config);
		priv->config = NULL;
	}

	G_OBJECT_CLASS(urf_arbitrator_parent_class)->dispose(object);
}

/**
 * urf_arbitrator_finalize:
 **/
static void
urf_arbitrator_finalize (GObject *object)
{
	UrfArbitratorPrivate *priv = URF_ARBITRATOR_GET_PRIVATE (object);

	/* cleanup monitoring */
	if (priv->watch_id > 0) {
		g_source_remove (priv->watch_id);
		priv->watch_id = 0;
		g_io_channel_shutdown (priv->channel, FALSE, NULL);
		g_io_channel_unref (priv->channel);
	}
	close(priv->fd);

	G_OBJECT_CLASS(urf_arbitrator_parent_class)->finalize(object);
}

/**
 * urf_arbitrator_class_init:
 **/
static void
urf_arbitrator_class_init(UrfArbitratorClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	g_type_class_add_private(klass, sizeof(UrfArbitratorPrivate));
	object_class->dispose = urf_arbitrator_dispose;
	object_class->finalize = urf_arbitrator_finalize;

	signals[DEVICE_ADDED] =
		g_signal_new ("device-added",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfArbitratorClass, device_added),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[DEVICE_REMOVED] =
		g_signal_new ("device-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfArbitratorClass, device_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[DEVICE_CHANGED] =
		g_signal_new ("device-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfArbitratorClass, device_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);
}

/**
 * urf_arbitrator_new:
 **/
UrfArbitrator *
urf_arbitrator_new (void)
{
	UrfArbitrator *arbitrator;
	arbitrator = URF_ARBITRATOR(g_object_new (URF_TYPE_ARBITRATOR, NULL));
	return arbitrator;
}

