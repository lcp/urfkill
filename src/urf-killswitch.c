/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@suse.com>
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
#  include "config.h"
#endif

#include <glib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <linux/rfkill.h>

#include "urf-killswitch.h"
#include "urf-device.h"

enum
{
	PROP_0,
	PROP_STATE,
	PROP_LAST
};

enum
{
	SIGNAL_STATE_CHANGED,
	SIGNAL_LAST,
};

static guint signals[SIGNAL_LAST] = { 0 };

struct UrfKillswitchPrivate
{
	GList		 *devices;
	enum rfkill_type  type;
	KillswitchState   state;
};

G_DEFINE_TYPE (UrfKillswitch, urf_killswitch, G_TYPE_OBJECT)

#define URF_KILLSWITCH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_KILLSWITCH, UrfKillswitchPrivate))

KillswitchState
urf_killswitch_get_state (UrfKillswitch *killswitch)
{
	return killswitch->priv->state;
}

KillswitchState
aggregate_states (KillswitchState platform,
		  KillswitchState non_platform)
{
	if (platform == KILLSWITCH_STATE_NO_ADAPTER &&
	    non_platform == KILLSWITCH_STATE_NO_ADAPTER)
		return KILLSWITCH_STATE_NO_ADAPTER;
	else if (platform == KILLSWITCH_STATE_NO_ADAPTER)
		return non_platform;
	else
		return platform;

	if (platform == KILLSWITCH_STATE_UNBLOCKED)
		return non_platform;
	else
		return platform;
}

/**
 * urf_killswitch_state_refresh:
 **/
static void
urf_killswitch_state_refresh (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	KillswitchState platform;
	KillswitchState non_platform;
	KillswitchState new_state;
	UrfDevice *device;
	GList *iter;

	if (priv->devices == NULL) {
		priv->state = KILLSWITCH_STATE_NO_ADAPTER;
		return;
	}

	platform = KILLSWITCH_STATE_NO_ADAPTER;
	non_platform = KILLSWITCH_STATE_NO_ADAPTER;
	new_state = KILLSWITCH_STATE_NO_ADAPTER;

	/* check the states of platform switches */
	for (iter = priv->devices; iter; iter = iter->next) {
		KillswitchState state;
		device = (UrfDevice *)iter->data;

		if (urf_device_is_platform (device) != TRUE)
			continue;
		state = urf_device_get_state (device);
		if (state > platform)
			platform = state;
	}

	/* check the states of non-platform switches */
	for (iter = priv->devices; iter; iter = iter->next) {
		KillswitchState state;
		device = (UrfDevice *)iter->data;

		if (urf_device_is_platform (device) != FALSE)
			continue;
		state = urf_device_get_state (device);
		if (state > non_platform)
			non_platform = state;
	}

	new_state = aggregate_states (platform, non_platform);
	/* emit a signal for change */
	if (priv->state != new_state) {
		priv->state = new_state;
		g_signal_emit (G_OBJECT (killswitch),
			       signals[SIGNAL_STATE_CHANGED],
			       0,
			       priv->state);
	}
}

static void
device_changed_cb (UrfDevice     *device,
		   UrfKillswitch *killswitch)
{
	urf_killswitch_state_refresh (killswitch);
}

/**
 * urf_killswitch_add_device:
 **/
void
urf_killswitch_add_device (UrfKillswitch *killswitch,
			   UrfDevice     *device)
{
	UrfKillswitchPrivate *priv = killswitch->priv;

	if (urf_device_get_rf_type (device) != priv->type ||
	    g_list_find (priv->devices, (gconstpointer)device) != NULL)
		return;

	priv->devices = g_list_prepend (priv->devices,
					(gpointer)g_object_ref (device));
	g_signal_connect (G_OBJECT (device), "changed",
			  G_CALLBACK (device_changed_cb), killswitch);

	urf_killswitch_state_refresh (killswitch);
}

/**
 * urf_killswitch_del_device:
 **/
void
urf_killswitch_del_device (UrfKillswitch *killswitch,
			   UrfDevice     *device)
{
	UrfKillswitchPrivate *priv = killswitch->priv;

	if (urf_device_get_rf_type (device) != priv->type ||
	    g_list_find (priv->devices, (gconstpointer)device) == NULL)
		return;

	priv->devices = g_list_remove (priv->devices, (gpointer)device);
	g_object_unref (device);

	urf_killswitch_state_refresh (killswitch);
}

/**
 * urf_killswitch_dispose:
 **/
static void
urf_killswitch_dispose (GObject *object)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (object);
	UrfKillswitchPrivate *priv = killswitch->priv;

	if (priv->devices) {
		g_list_foreach (priv->devices, (GFunc) g_object_unref, NULL);
		g_list_free (priv->devices);
		priv->devices = NULL;
	}

	G_OBJECT_CLASS (urf_killswitch_parent_class)->dispose (object);
}

/**
 * urf_killswitch_get_property:
 **/
static void
urf_killswitch_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (object);
	UrfKillswitchPrivate *priv = killswitch->priv;

	switch (prop_id) {
	case PROP_STATE:
		g_value_set_int (value, priv->state);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_killswitch_class_init:
 **/
static void
urf_killswitch_class_init (UrfKillswitchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->dispose = urf_killswitch_dispose;
	object_class->get_property = urf_killswitch_get_property;

	g_type_class_add_private (klass, sizeof (UrfKillswitchPrivate));

	signals[SIGNAL_STATE_CHANGED] =
		g_signal_new ("state-changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

	g_object_class_install_property (object_class,
					 PROP_STATE,
					 g_param_spec_int ("state",
							   "Killswitch State",
							   "The state of the killswitch",
							   KILLSWITCH_STATE_NO_ADAPTER,
							   KILLSWITCH_STATE_HARD_BLOCKED,
							   KILLSWITCH_STATE_NO_ADAPTER,
							   G_PARAM_READABLE));
}

/**
 * urf_killswitch_init:
 **/
static void
urf_killswitch_init (UrfKillswitch *killswitch)
{
	killswitch->priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	killswitch->priv->devices = NULL;
}

/**
 * urf_killswitch_new:
 **/
UrfKillswitch *
urf_killswitch_new (enum rfkill_type type)
{
	UrfKillswitch *killswitch;

	if (type <= RFKILL_TYPE_ALL || type >= NUM_RFKILL_TYPES)
		return NULL;

	killswitch = URF_KILLSWITCH (g_object_new (URF_TYPE_KILLSWITCH, NULL));
	killswitch->priv->type = type;

	return killswitch;
}
