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
#include <gio/gio.h>

#include "urf-killswitch.h"
#include "urf-enum.h"

#define URF_KILLSWITCH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					URF_TYPE_KILLSWITCH, UrfKillswitchPrivate))

#define BASE_OBJECT_PATH "/org/freedesktop/URfkill/"

struct _UrfKillswitchPrivate
{
	GDBusProxy	*proxy;
	UrfEnumType	 type;
	UrfEnumState	 state;
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
static gpointer urf_killswitch_object[URF_ENUM_TYPE_NUM] = {
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
UrfEnumType
urf_killswitch_get_switch_type (UrfKillswitch *killswitch)
{
	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), URF_ENUM_TYPE_NUM);

	return killswitch->priv->type;
}

/**
 * urf_killswitch_changed_cb:
 **/
static void
urf_killswitch_changed_cb (GDBusProxy *proxy,
                           GVariant   *changed_properties,
                           GStrv       invalidated_properties,
                           gpointer    user_data)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (user_data);
	UrfKillswitchPrivate *priv = killswitch->priv;
	GVariant *value;
	int state;

	value = g_dbus_proxy_get_cached_property (priv->proxy, "state");
	state = g_variant_get_int32 (value);
	if (priv->state != state) {
		priv->state = state;
		g_signal_emit (killswitch,
		               signals[URF_KILLSWITCH_STATE_CHANGED],
		               0, state);
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
	GVariant *value;
	GError *error_local = NULL;

	if (priv->object_path != NULL)
		return FALSE;
	if (object_path == NULL)
		return FALSE;

	/* invalid */
	if (object_path == NULL || object_path[0] != '/') {
		g_set_error (error, 1, 0, "Object path %s invalid", object_path);
		goto out;
	}

	/* connect to the correct path for properties */
	priv->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                             G_DBUS_PROXY_FLAGS_NONE,
	                                             NULL,
	                                             "org.freedesktop.URfkill",
	                                             object_path,
	                                             "org.freedesktop.URfkill.Killswitch",
	                                             NULL,
	                                             &error_local);
	if (error_local) {
		g_warning ("Couldn't connect to proxy: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		return FALSE;
	}

        priv->object_path = g_strdup (object_path);

	value = g_dbus_proxy_get_cached_property (priv->proxy, "state");
	priv->state = g_variant_get_int32 (value);

	/* connect signals */
	g_signal_connect (priv->proxy, "g-properties-changed",
	                  G_CALLBACK (urf_killswitch_changed_cb), killswitch);
out:
	return TRUE;
}

/**
 * urf_killswitch_startup:
 **/
static gboolean
urf_killswitch_startup (UrfKillswitch *killswitch)
{
	const char *object_path;
	GError *error = NULL;
	gboolean ret = FALSE;

	switch (killswitch->priv->type) {
	case URF_ENUM_TYPE_WLAN:
		object_path = BASE_OBJECT_PATH"WLAN";
		break;
	case URF_ENUM_TYPE_BLUETOOTH:
		object_path = BASE_OBJECT_PATH"BLUETOOTH";
		break;
	case URF_ENUM_TYPE_UWB:
		object_path = BASE_OBJECT_PATH"UWB";
		break;
	case URF_ENUM_TYPE_WIMAX:
		object_path = BASE_OBJECT_PATH"WIMAX";
		break;
	case URF_ENUM_TYPE_WWAN:
		object_path = BASE_OBJECT_PATH"WWAN";
		break;
	case URF_ENUM_TYPE_GPS:
		object_path = BASE_OBJECT_PATH"GPS";
		break;
	case URF_ENUM_TYPE_FM:
		object_path = BASE_OBJECT_PATH"FM";
		break;
	case URF_ENUM_TYPE_NFC:
		object_path = BASE_OBJECT_PATH"NFC";
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
 * set_block_cb:
 **/
static void
set_block_cb (GDBusProxy     *proxy,
              GAsyncResult   *res,
	      gpointer        user_data)
{
	GVariant *retval;
	gboolean status;
	GError *error = NULL;

	retval = g_dbus_proxy_call_finish (proxy, res, &error);
	if (retval)
		g_variant_get (retval, "(b)", &status);

	if (error) {
		g_warning ("Failed to set BLOCK: %s", error->message);
		g_error_free (error);
	} else if (!status) {
		g_warning ("Failed to set BLOCK");
	}

	g_object_unref (proxy);
}

/**
 * urf_killswitch_set_block:
 **/
static void
urf_killswitch_set_block (UrfKillswitch  *killswitch,
			  UrfEnumState    state)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	GDBusProxy *proxy;
	gboolean block;
	GError *error = NULL;

	if (state == URF_ENUM_STATE_UNBLOCKED)
		block = FALSE;
	else
		block = TRUE;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                       G_DBUS_PROXY_FLAGS_NONE,
	                                       NULL,
	                                       "org.freedesktop.URfkill",
	                                       "/org/freedesktop/URfkill",
	                                       "org.freedesktop.URfkill",
	                                       NULL,
	                                       &error);
	if (error) {
		g_warning ("Couldn't connect to proxy to set block: %s",
		           error->message);
		g_error_free (error);
		return;
	}
	g_dbus_proxy_call (proxy, "Block",
	                   g_variant_new ("(ub)",
	                                  priv->type,
	                                  block),
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1, NULL,
	                   (GAsyncReadyCallback) set_block_cb,
	                   killswitch);
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
		if (state == URF_ENUM_STATE_UNBLOCKED ||
		    state == URF_ENUM_STATE_SOFT_BLOCKED)
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

	if (priv->proxy) {
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
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
	 * The state of the killswitch. See #UrfEnumState.
	 * <note>
	 *   <para>
	 *     Writing the states other than #URF_ENUM_STATE_UNBLOCKED
	 *     or #URF_ENUM_STATE_SOFT_BLOCKED will be ignored. Also,
	 *     the state writing may not take effect since it depends on
	 *     the state of the hardware.
	 *   </para>
	 * </note>
	 *
	 * Since: 0.3.0
	 */
	pspec = g_param_spec_int ("state",
				  "State", "The state of the killswitch",
				  URF_ENUM_STATE_NO_ADAPTER,
				  URF_ENUM_STATE_HARD_BLOCKED,
				  URF_ENUM_STATE_NO_ADAPTER,
				  G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_STATE, pspec);

	/**
	 * UrfKillswitch::state-changed:
	 * @client: the #UrfKillswitch instance that emitted the signal
	 * @state: the new state
	 *
	 * The state-changed signal is emitted when the killswitch state is changed.
	 * See #UrfEnumState.
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
urf_killswitch_new (UrfEnumType type)
{
	UrfKillswitch *killswitch;

	if (type == URF_ENUM_TYPE_ALL)
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
