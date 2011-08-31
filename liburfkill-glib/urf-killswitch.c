/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@suse.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * SECTION:urf-killswitch
 * @short_description: Client object for accessing information about killswitches
 * @title: UrfKillswitch
 * @include: urfkill.h
 * @see_also: #UrfClient, #UrfDevice
 *
 * A helper GObject for accessing killswitches
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "urf-killswitch.h"
#include "urf-enum.h"

#define URF_KILLSWITCH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					URF_TYPE_KILLSWITCH, UrfKillswitchPrivate))

#define BASE_OBJECT_PATH "/org/freedesktop/URfkill/"

struct _UrfKillswitchPrivate
{
	DBusGConnection	*bus;
	DBusGProxy	*proxy;
	DBusGProxy	*proxy_props;
	UrfSwitchType	 type;
	UrfSwitchState	 state;
	char		*object_path;
};

enum {
	URF_KILLSWITCH_STATE_CHANGED,
	URF_KILLSWITCH_LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_KILLSWITCH_STATE,
	PROP_LAST
};

static guint signals [URF_KILLSWITCH_LAST_SIGNAL] = { 0 };
static gpointer urf_killswitch_object[NUM_URFSWITCH_TYPES] = {
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };

G_DEFINE_TYPE (UrfKillswitch, urf_killswitch, G_TYPE_OBJECT)

/**
 * urf_killswitch_get_switch_type:
 * @killswitch: a #UrfKillswitch instance
 *
 * Get the type of the killswitch.
 *
 * Return value: The type of the killswitch
 *
 * Since: 0.3.0
 **/
UrfSwitchType
urf_killswitch_get_switch_type (UrfKillswitch *killswitch)
{
	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), NUM_URFSWITCH_TYPES);

	return killswitch->priv->type;
}

/**
 * urf_killswitch_get_remote_properties:
 **/
static GHashTable *
urf_killswitch_get_remote_properties (UrfKillswitch *killswitch,
				      GError        **error)
{
	gboolean ret;
	GError *error_local = NULL;
	GHashTable *hash_table = NULL;

	ret = dbus_g_proxy_call (killswitch->priv->proxy_props, "GetAll", &error_local,
				 G_TYPE_STRING, "org.freedesktop.URfkill.Killswitch",
				 G_TYPE_INVALID,
				 dbus_g_type_get_map ("GHashTable", G_TYPE_STRING, G_TYPE_VALUE),
				 &hash_table,
				 G_TYPE_INVALID);
	if (!ret) {
		g_set_error (error, 1, 0, "Couldn't call GetAll() to get properties for %s: %s",
			     killswitch->priv->object_path, error_local->message);
		g_error_free (error_local);
		goto out;
	}
out:
	return hash_table;
}

/**
 * urf_killswitch_collect_props_cb:
 **/
static void
urf_killswitch_collect_props_cb (const char    *key,
				 const GValue  *value,
				 UrfKillswitch *killswitch)
{
	if (g_strcmp0 (key, "state") == 0) {
		killswitch->priv->state = g_value_get_int (value);
	} else {
		g_warning ("unhandled property '%s'", key);
	}
}

/**
 * urf_killswitch_refresh_private:
 **/
static gboolean
urf_killswitch_refresh_private (UrfKillswitch *killswitch,
				GError        **error)
{
	GHashTable *hash;
	GError *error_local = NULL;

	/* get all the properties */
	hash = urf_killswitch_get_remote_properties (killswitch, &error_local);
	if (hash == NULL) {
		g_set_error (error, 1, 0, "Cannot get killswitch properties for %s: %s",
			     killswitch->priv->object_path, error_local->message);
		g_error_free (error_local);
		return FALSE;
	}
	g_hash_table_foreach (hash, (GHFunc) urf_killswitch_collect_props_cb, killswitch);
	g_hash_table_unref (hash);
	return TRUE;
}

/**
 * urf_killswitch_state_changed_cb:
 **/
static void
urf_killswitch_state_changed_cb (DBusGProxy    *proxy,
				 const int      state,
				 UrfKillswitch *killswitch)
{
	if (killswitch->priv->state != state) {
		killswitch->priv->state = state;
		g_signal_emit (killswitch, signals[URF_KILLSWITCH_STATE_CHANGED], 0, state);
	}
}

/**
 * urf_killswitch_set_object_path_sync:
 **/
static gboolean
urf_killswitch_set_object_path_sync (UrfKillswitch *killswitch,
				     const char    *object_path,
				     GError        **error)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	GError *error_local = NULL;
	gboolean ret = FALSE;
	DBusGProxy *proxy_props;

	if (killswitch->priv->object_path != NULL)
		return FALSE;
	if (object_path == NULL)
		return FALSE;

	/* invalid */
	if (object_path == NULL || object_path[0] != '/') {
		g_set_error (error, 1, 0, "Object path %s invalid", object_path);
		goto out;
	}

	/* connect to the bus */
	priv->bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error_local);
	if (priv->bus == NULL) {
		g_set_error (error, 1, 0, "Couldn't connect to system bus: %s", error_local->message);
		g_error_free (error_local);
		goto out;
	}

	/* connect to the correct path for properties */
	proxy_props = dbus_g_proxy_new_for_name (priv->bus,
						 "org.freedesktop.URfkill",
						 object_path,
						 "org.freedesktop.DBus.Properties");
	if (proxy_props == NULL) {
		g_set_error_literal (error, 1, 0, "Couldn't connect to proxy");
		goto out;
	}

	priv->proxy = dbus_g_proxy_new_for_name (priv->bus,
						 "org.freedesktop.URfkill",
						 object_path,
						 "org.freedesktop.URfkill.Killswitch");
	if (priv->proxy == NULL) {
		g_set_error_literal (error, 1, 0, "Couldn't connect to proxy");
		goto out;
	}

        killswitch->priv->proxy_props = proxy_props;
        killswitch->priv->object_path = g_strdup (object_path);

	ret = urf_killswitch_refresh_private (killswitch, &error_local);

	if (!ret) {
		g_set_error (error, 1, 0, "cannot refresh: %s", error_local->message);
		g_error_free (error_local);
	}

	/* connect signals */
	dbus_g_proxy_add_signal (priv->proxy, "StateChanged",
				 G_TYPE_INT,
				 G_TYPE_INVALID);

	/* callbacks */
	dbus_g_proxy_connect_signal (priv->proxy, "StateChanged",
				     G_CALLBACK (urf_killswitch_state_changed_cb), killswitch, NULL);
out:
	return ret;
}

/**
 * urf_killswitch_startup
 **/
static gboolean
urf_killswitch_startup (UrfKillswitch *killswitch)
{
	const char *object_path;
	GError *error = NULL;
	gboolean ret = FALSE;

	switch (killswitch->priv->type) {
	case URFSWITCH_TYPE_WLAN:
		object_path = BASE_OBJECT_PATH"WLAN";
		break;
	case URFSWITCH_TYPE_BLUETOOTH:
		object_path = BASE_OBJECT_PATH"BLUETOOTH";
		break;
	case URFSWITCH_TYPE_UWB:
		object_path = BASE_OBJECT_PATH"UWB";
		break;
	case URFSWITCH_TYPE_WIMAX:
		object_path = BASE_OBJECT_PATH"WIMAX";
		break;
	case URFSWITCH_TYPE_WWAN:
		object_path = BASE_OBJECT_PATH"WWAN";
		break;
	case URFSWITCH_TYPE_GPS:
		object_path = BASE_OBJECT_PATH"GPS";
		break;
	case URFSWITCH_TYPE_FM:
		object_path = BASE_OBJECT_PATH"FM";
		break;
	default:
		object_path = NULL;
		break;
	}

	ret = urf_killswitch_set_object_path_sync (killswitch,
						   object_path,
						   &error);

	if (error) {
		g_warning ("UrfKillswitch: %s", error->message);
		g_error_free (error);
	}

	return ret;
}

/**
 * urf_killswitch_set_block:
 **/
static void
urf_killswitch_set_block (UrfKillswitch  *killswitch,
			  UrfSwitchState  state)
{
	DBusGProxy *proxy;
	gboolean block;
	gboolean status, ret;
	GError *error = NULL;

	if (state == URFSWITCH_STATE_UNBLOCKED)
		block = FALSE;
	else
		block = TRUE;

	proxy = dbus_g_proxy_new_for_name (killswitch->priv->bus,
					   "org.freedesktop.URfkill",
					   "/org/freedesktop/URfkill",
					   "org.freedesktop.URfkill");
	if (proxy == NULL) {
		g_warning ("Couldn't connect to proxy to set block");
		return;
	}
	ret = dbus_g_proxy_call (proxy, "Block", &error,
				 G_TYPE_UINT, killswitch->priv->type,
				 G_TYPE_BOOLEAN, block,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &status,
				 G_TYPE_INVALID);

	if (error) {
		g_warning ("Couldn't sent BLOCK: %s", error->message);
		g_error_free (error);
	}

	g_object_unref (proxy);
}

/**
 * urf_killswitch_set_property:
 **/
static void
urf_killswitch_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (object);
	int state;

	switch (prop_id) {
	case PROP_KILLSWITCH_STATE:
		state = g_value_get_int (value);
		if (state == URFSWITCH_STATE_UNBLOCKED ||
		    state == URFSWITCH_STATE_SOFT_BLOCKED)
			urf_killswitch_set_block (killswitch, state);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
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
	case PROP_KILLSWITCH_STATE:
		g_value_set_int (value, priv->state);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_killswitch_finalize:
 **/
static void
urf_killswitch_finalize (GObject *object)
{
	UrfKillswitchPrivate *priv;

	priv = URF_KILLSWITCH (object)->priv;

	g_free (priv->object_path);

	G_OBJECT_CLASS(urf_killswitch_parent_class)->finalize(object);
}

/**
 * urf_killswitch_dispose:
 **/
static void
urf_killswitch_dispose (GObject *object)
{
	UrfKillswitchPrivate *priv;

	g_return_if_fail (URF_IS_KILLSWITCH (object));

	priv = URF_KILLSWITCH (object)->priv;

	if (priv->bus) {
		dbus_g_connection_unref (priv->bus);
		priv->bus = NULL;
	}

	if (priv->proxy) {
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	if (priv->proxy_props) {
		g_object_unref (priv->proxy_props);
		priv->proxy_props = NULL;
	}

	G_OBJECT_CLASS(urf_killswitch_parent_class)->dispose(object);
}

/**
 * urf_killswitch_class_init:
 * @klass: The UrfKillswitchClass
 **/
static void
urf_killswitch_class_init (UrfKillswitchClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GParamSpec *pspec;

	object_class->set_property = urf_killswitch_set_property;
	object_class->get_property = urf_killswitch_get_property;
	object_class->finalize = urf_killswitch_finalize;
	object_class->dispose = urf_killswitch_dispose;

	/**
	 * UrfKillswitch:state:
	 *
	 * The state of the killswitch. See #UrfSwitchState.
	 * <note>
	 *   <para>
	 *     Writing the states other than #URFSWITCH_STATE_UNBLOCKED
	 *     or #URFSWITCH_STATE_SOFT_BLOCKED will be ignored. Also,
	 *     the state writing may not take effect, and it depends on
	 *     the state of the hardware.
	 *   </para>
	 * </note>
	 *
	 * Since: 0.3.0
	 */
	pspec = g_param_spec_int ("state",
				  "State", "The state of the killswitch",
				  URFSWITCH_STATE_NO_ADAPTER,
				  URFSWITCH_STATE_HARD_BLOCKED,
				  URFSWITCH_STATE_NO_ADAPTER,
				  G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_STATE, pspec);

	/**
	 * UrfKillswitch::state-changed:
	 * @client: the #UrfKillswitch instance that emitted the signal
	 * @state: the new state
	 *
	 * The state-changed signal is emitted when the killswitch state is changed.
	 * See #UrfSwitchState.
	 *
	 * Since 0.3.0
	 **/
        signals[URF_KILLSWITCH_STATE_CHANGED] =
                g_signal_new ("state-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfKillswitchClass, state_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

	g_type_class_add_private(klass, sizeof(UrfKillswitchPrivate));
}

/**
 * urf_killswitch_init:
 * @client: This class instance
 **/
static void
urf_killswitch_init (UrfKillswitch *killswitch)
{
	killswitch->priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	killswitch->priv->object_path = NULL;
}

/**
 * urf_killswitch_new:
 * @type: The killswitch type
 *
 * Creates a new #UrfKillswitch object.
 *
 * Return value: a new #UrfKillswitch object.
 *
 * Since: 0.3.0
 **/
UrfKillswitch *
urf_killswitch_new (UrfSwitchType type)
{
	UrfKillswitch *killswitch;

	if (type == URFSWITCH_TYPE_ALL)
		return NULL;

	if (urf_killswitch_object[type] != NULL) {
		g_object_ref (urf_killswitch_object[type]);
	} else {
		killswitch = URF_KILLSWITCH (g_object_new (URF_TYPE_KILLSWITCH, NULL));
		killswitch->priv->type = type;
		if (urf_killswitch_startup (killswitch) == FALSE)
			return NULL;
		urf_killswitch_object[type] = (gpointer)killswitch;
		g_object_add_weak_pointer (urf_killswitch_object[type], &urf_killswitch_object[type]);
	}

	return URF_KILLSWITCH (urf_killswitch_object[type]);
}
