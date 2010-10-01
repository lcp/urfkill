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
#include <string.h>
#include <sys/ioctl.h>

#include <glib.h>

#include "rfkill.h"
#include "egg-debug.h"

#ifndef RFKILL_EVENT_SIZE_V1
#define RFKILL_EVENT_SIZE_V1    8
#endif

#include "urf-killswitch.h"

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
	int fd;
	GIOChannel *channel;
	guint watch_id;
	GList *killswitches; /* a GList of UrfIndKillswitch */
	GHashTable *type_map;
};

G_DEFINE_TYPE(UrfKillswitch, urf_killswitch, G_TYPE_OBJECT)

static KillswitchState
event_to_state (guint soft, guint hard)
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

static void
update_killswitch (UrfKillswitch *killswitch,
		   guint index, guint soft, guint hard)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	GList *l;
	guint type;
	gboolean changed = FALSE;

	for (l = priv->killswitches; l != NULL; l = l->next) {
		UrfIndKillswitch *ind = l->data;

		if (ind->index == index) {
			if (ind->soft != soft || ind->hard != hard) {
				ind->state = event_to_state (soft, hard);
				type = ind->type;
				ind->soft = soft;
				ind->hard = hard;
				changed = TRUE;
			}
			break;
		}
	}

	if (changed != FALSE) {
		egg_debug ("updating killswitch status %d to %s",
			   index,
			   state_to_string (urf_killswitch_get_state (killswitch, type)));
		g_signal_emit (G_OBJECT (killswitch), signals[RFKILL_CHANGED], 0, index);
	}
}

gboolean
urf_killswitch_set_state (UrfKillswitch *killswitch,
			  guint type,
			  KillswitchState state)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
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

	len = write (priv->fd, &event, sizeof(event));
	if (len < 0) {
		egg_warning ("Failed to change RFKILL state: %s",
			     g_strerror (errno));
		return FALSE;
	}
	return TRUE;
}

KillswitchState
urf_killswitch_get_state (UrfKillswitch *killswitch, guint type)
{
	UrfKillswitchPrivate *priv;
	int state = KILLSWITCH_STATE_UNBLOCKED;
	GList *l;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), state);
	g_return_val_if_fail (type < NUM_RFKILL_TYPES, state);

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	if (priv->killswitches == NULL)
		return KILLSWITCH_STATE_NO_ADAPTER;

	for (l = priv->killswitches ; l ; l = l->next) {
		UrfIndKillswitch *ind = l->data;

		if (ind->type != type && type != RFKILL_TYPE_ALL)
			continue;

		egg_debug ("killswitch %d is %s",
			   ind->index, state_to_string (ind->state));

		if (ind->state == KILLSWITCH_STATE_HARD_BLOCKED) {
			state = KILLSWITCH_STATE_HARD_BLOCKED;
			break;
		}

		if (ind->state == KILLSWITCH_STATE_SOFT_BLOCKED) {
			state = KILLSWITCH_STATE_SOFT_BLOCKED;
			continue;
		}

		state = ind->state;
	}

	egg_debug ("killswitches %s state %s",
		   type_to_string (type), state_to_string (state));

	return state;
}

gboolean
urf_killswitch_has_killswitches (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), FALSE);

	return (priv->killswitches != NULL);
}

GList*
urf_killswitch_get_killswitches (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), NULL);

	return priv->killswitches;
}

UrfIndKillswitch *
urf_killswitch_get_killswitch (UrfKillswitch *killswitch, const guint index)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	UrfIndKillswitch *ind;
	GList *item;
	
	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), NULL);

	for (item = priv->killswitches; item; item = g_list_next (item)) {
		ind = (UrfIndKillswitch *)item->data;
		if (ind->index == index)
			return ind;
	}

	return NULL;
}

static void
remove_killswitch (UrfKillswitch *killswitch,
		   guint index)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	GList *l;
	guint type;

	for (l = priv->killswitches; l != NULL; l = l->next) {
		UrfIndKillswitch *ind = l->data;
		if (ind->index == index) {
			type = ind->type;
			priv->killswitches = g_list_remove (priv->killswitches, ind);
			egg_debug ("removing killswitch idx %d", index);
			g_free (ind);
			g_signal_emit (G_OBJECT (killswitch), signals[RFKILL_REMOVED], 0, index);
			return;
		}
	}
}

static void
add_killswitch (UrfKillswitch *killswitch,
		guint index, guint type, guint soft, guint hard)

{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	UrfIndKillswitch *ind;
	int state = event_to_state (soft, hard);

	egg_debug ("adding killswitch idx %d state %s", index, state_to_string (state));
	ind = g_new0 (UrfIndKillswitch, 1);
	ind->index = index;
	ind->type  = type;
	ind->state = state;
	ind->soft  = soft;
	ind->hard  = hard;
	priv->killswitches = g_list_append (priv->killswitches, ind);

	g_signal_emit (G_OBJECT (killswitch), signals[RFKILL_ADDED], 0, index);
}

gint
urf_killswitch_rf_type (UrfKillswitch *killswitch,
			const char *type_name)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
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
	egg_debug ("RFKILL event: idx %u type %u (%s) op %u (%s) soft %u hard %u",
		   event->idx,
		   event->type, type_to_string (event->type),
		   event->op, op_to_string (event->op),
		   event->soft, event->hard);
}

static gboolean
event_cb (GIOChannel *source,
	  GIOCondition condition,
	  UrfKillswitch *killswitch)
{
	if (condition & G_IO_IN) {
		GIOStatus status;
		struct rfkill_event event;
		gsize read;

		status = g_io_channel_read_chars (source,
						  (char *) &event,
						  sizeof(event),
						  &read,
						  NULL);

		while (status == G_IO_STATUS_NORMAL && read == sizeof(event)) {
			print_event (&event);

			if (event.op == RFKILL_OP_CHANGE) {
				update_killswitch (killswitch, event.idx, event.soft, event.hard);
			} else if (event.op == RFKILL_OP_DEL) {
				remove_killswitch (killswitch, event.idx);
			} else if (event.op == RFKILL_OP_ADD) {
				add_killswitch (killswitch, event.idx, event.type, event.soft, event.hard);
			}

			status = g_io_channel_read_chars (source,
							  (char *) &event,
							  sizeof(event),
							  &read,
							  NULL);
		}
	} else {
		egg_debug ("something else happened");
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

static void
urf_killswitch_init (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	struct rfkill_event event;
	int fd;

	priv->type_map = construct_type_map ();
	priv->killswitches = NULL;

	fd = open("/dev/rfkill", O_RDWR);
	if (fd < 0) {
		if (errno == EACCES)
			egg_warning ("Could not open RFKILL control device, please verify your installation");
		return;
	}

	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0) {
		egg_debug ("Can't set RFKILL control device to non-blocking");
		close(fd);
		return;
	}

	/* Disable rfkill input */
	ioctl(fd, RFKILL_IOCTL_NOINPUT);

	while (1) {
		KillswitchState state;
		ssize_t len;

		len = read(fd, &event, sizeof(event));
		if (len < 0) {
			if (errno == EAGAIN)
				break;
			egg_debug ("Reading of RFKILL events failed");
			break;
		}

		if (len != RFKILL_EVENT_SIZE_V1) {
			egg_warning("Wrong size of RFKILL event\n");
			continue;
		}

		if (event.op != RFKILL_OP_ADD)
			continue;
		if (event.type >= NUM_RFKILL_TYPES)
			continue;

		add_killswitch (killswitch, event.idx, event.type, event.soft, event.hard);
	}

	/* Setup monitoring */
	priv->fd = fd;
	priv->channel = g_io_channel_unix_new (priv->fd);
	priv->watch_id = g_io_add_watch (priv->channel,
				G_IO_IN | G_IO_HUP | G_IO_ERR,
				(GIOFunc) event_cb,
				killswitch);

	/* TODO emit a reasonable signal */
}

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

	g_list_foreach (priv->killswitches, (GFunc) g_free, NULL);
	g_list_free (priv->killswitches);
	priv->killswitches = NULL;

	g_hash_table_destroy (priv->type_map);

	G_OBJECT_CLASS(urf_killswitch_parent_class)->finalize(object);
}

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
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);

	signals[RFKILL_REMOVED] =
		g_signal_new ("rfkill-removed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfKillswitchClass, rfkill_removed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_UINT);

	signals[RFKILL_CHANGED] =
		g_signal_new ("rfkill-changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfKillswitchClass, rfkill_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__UINT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

}

UrfKillswitch *
urf_killswitch_new (void)
{
	UrfKillswitch *killswitch;
	killswitch = URF_KILLSWITCH(g_object_new (URF_TYPE_KILLSWITCH, NULL));
	return killswitch;
}

