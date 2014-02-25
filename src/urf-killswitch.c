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
#include <gio/gio.h>
#include <linux/rfkill.h>

#include "urf-killswitch.h"
#include "urf-device.h"

#define BASE_OBJECT_PATH "/org/freedesktop/URfkill/"
#define URF_KILLSWITCH_INTERFACE "org.freedesktop.URfkill.Killswitch"

static const char introspection_xml[] =
"<node>"
"  <interface name='org.freedesktop.URfkill.Killswitch'>"
"    <signal name='StateChanged'/>"
"    <property name='state' type='i' access='read'/>"
"  </interface>"
"</node>";

enum
{
	PROP_0,
	PROP_STATE,
	PROP_LAST
};

struct UrfKillswitchPrivate
{
	GList		 *devices;
	enum rfkill_type  type;
	KillswitchState   saved_state;
	KillswitchState   state;
	char		 *object_path;
	GDBusConnection	 *connection;
	GDBusNodeInfo	 *introspection_data;
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
urf_killswitch_get_saved_state (UrfKillswitch *killswitch)

{
	return killswitch->priv->saved_state;
}

void
urf_killswitch_set_saved_state (UrfKillswitch *killswitch,
				KillswitchState state)
{
	killswitch->priv->saved_state = state;
}

KillswitchState
aggregate_states (KillswitchState platform,
		  KillswitchState non_platform)
{
	if (platform == KILLSWITCH_STATE_UNBLOCKED &&
	    non_platform != KILLSWITCH_STATE_NO_ADAPTER)
		return non_platform;
	else
		return platform;
}

/**
 * send_properites_changed
 **/
static void
emit_properites_changed (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	GVariantBuilder *builder;
	GError *error = NULL;

	builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add (builder,
	                       "{sv}",
	                       "state",
	                       g_variant_new_int32 (priv->state));

	g_dbus_connection_emit_signal (priv->connection,
	                               NULL,
	                               priv->object_path,
	                               "org.freedesktop.DBus.Properties",
	                               "PropertiesChanged",
	                               g_variant_new ("(sa{sv}as)",
	                                              URF_KILLSWITCH_INTERFACE,
	                                              builder,
	                                              NULL),
	                                &error);
	if (error) {
		g_warning ("Failed to emit PropertiesChanged: %s", error->message);
		g_error_free (error);
	}
}

/**
 * urf_killswitch_state_refresh:
 **/
static void
urf_killswitch_state_refresh (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	KillswitchState platform;
	KillswitchState new_state;
	gboolean platform_checked = FALSE;
	UrfDevice *device;
	GList *iter;
	GError *error = NULL;

	if (priv->devices == NULL) {
		priv->state = KILLSWITCH_STATE_NO_ADAPTER;
		priv->saved_state = KILLSWITCH_STATE_NO_ADAPTER;
		return;
	}

	platform = KILLSWITCH_STATE_NO_ADAPTER;
	new_state = KILLSWITCH_STATE_NO_ADAPTER;

	for (iter = priv->devices; iter; iter = iter->next) {
		KillswitchState state;
		device = (UrfDevice *)iter->data;
		state = urf_device_get_state (device);

		if (urf_device_is_platform (device) == TRUE) {
			/* Update the state of platform switch */
			platform_checked = TRUE;
			if (state > platform)
				platform = state;
		} else {
			/* Update the state of non-platform switch */
			if (state > new_state)
				new_state = state;
		}
	}

	if (platform_checked)
		new_state = aggregate_states (platform, new_state);

	g_message("killswitch state: %s new_state: %s",
		  state_to_string(priv->state), state_to_string(new_state));
	/* emit a signal for change */
	if (priv->state != new_state) {
		priv->state = new_state;
		emit_properites_changed (killswitch);
		g_dbus_connection_emit_signal (priv->connection,
		                               NULL,
		                               priv->object_path,
		                               URF_KILLSWITCH_INTERFACE,
		                               "StateChanged",
		                               NULL,
		                               &error);
		if (error) {
			g_warning ("Failed to emit StateChanged: %s",
			           error->message);
			g_error_free (error);
		}
	}
}

static void
device_changed_cb (UrfDevice     *device,
		   UrfKillswitch *killswitch)
{
	g_message("device_changed_cb: %s", urf_device_get_name(device));
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

	if (urf_device_get_device_type (device) != priv->type ||
	    g_list_find (priv->devices, (gconstpointer)device) != NULL)
		return;

	priv->devices = g_list_prepend (priv->devices,
					(gpointer)g_object_ref (device));
	g_signal_connect (G_OBJECT (device), "state-changed",
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

	if (urf_device_get_device_type (device) != priv->type ||
	    g_list_find (priv->devices, (gconstpointer)device) == NULL)
		return;

	priv->devices = g_list_remove (priv->devices, (gpointer)device);
	g_object_unref (device);

	urf_killswitch_state_refresh (killswitch);
}

/**
 * urf_killswitch_set_software_blocked:
 **/
gboolean
urf_killswitch_set_software_blocked (UrfKillswitch *killswitch,
                                     gboolean blocked)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	GList *dev;
	gboolean result, ret = TRUE;

	for (dev = priv->devices; dev; dev = dev->next) {
		g_debug ("Setting device %s to %s",
		         urf_device_get_object_path (URF_DEVICE (dev->data)),
		         blocked ? "blocked" : "unblocked");

		result = urf_device_set_software_blocked (URF_DEVICE (dev->data), blocked);

		if (!result)
			ret = FALSE;
	}

	return ret;
}

/**
 * urf_killswitch_dispose:
 **/
static void
urf_killswitch_dispose (GObject *object)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (object);
	UrfKillswitchPrivate *priv = killswitch->priv;

	if (priv->introspection_data) {
		g_dbus_node_info_unref (priv->introspection_data);
		priv->introspection_data = NULL;
	}

	if (priv->connection) {
		g_object_unref (priv->connection);
		priv->connection = NULL;
	}

	if (priv->devices) {
		g_list_foreach (priv->devices, (GFunc) g_object_unref, NULL);
		g_list_free (priv->devices);
		priv->devices = NULL;
	}

	G_OBJECT_CLASS (urf_killswitch_parent_class)->dispose (object);
}

/**
 * urf_killswitch_finalize:
 **/
static void
urf_killswitch_finalize (GObject *object)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (object);

	g_free (killswitch->priv->object_path);

	G_OBJECT_CLASS (urf_killswitch_parent_class)->finalize (object);
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
	object_class->finalize = urf_killswitch_finalize;
	object_class->get_property = urf_killswitch_get_property;

	g_type_class_add_private (klass, sizeof (UrfKillswitchPrivate));

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
	killswitch->priv->object_path = NULL;
	killswitch->priv->state = KILLSWITCH_STATE_NO_ADAPTER;
	killswitch->priv->saved_state = KILLSWITCH_STATE_NO_ADAPTER;
}

static GVariant *
handle_get_property (GDBusConnection *connection,
                     const gchar *sender,
                     const gchar *object_path,
                     const gchar *interface_name,
                     const gchar *property_name,
                     GError **error,
                     gpointer user_data)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (user_data);

	GVariant *retval = NULL;

	if (g_strcmp0 (property_name, "state") == 0)
		retval = g_variant_new_int32 (killswitch->priv->state);

	return retval;
}

static const GDBusInterfaceVTable interface_vtable =
{
	NULL, /* handle method_call */
	handle_get_property,
	NULL, /* handle set_property */
};

/**
 * urf_device_register_device:
 **/
static gboolean
urf_killswitch_register_switch (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = killswitch->priv;
	GDBusInterfaceInfo **infos;
	guint reg_id;
	GError *error = NULL;

	priv->introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
	g_assert (priv->introspection_data != NULL);

	priv->connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
	if (priv->connection == NULL) {
		g_error ("error getting system bus: %s", error->message);
		g_error_free (error);
		return FALSE;
	}

	priv->object_path = g_strdup_printf (BASE_OBJECT_PATH"%s",
					     type_to_string (priv->type));
	infos = priv->introspection_data->interfaces;
	reg_id = g_dbus_connection_register_object (priv->connection,
		                                    priv->object_path,
		                                    infos[0],
		                                    &interface_vtable,
		                                    killswitch,
		                                    NULL,
		                                    NULL);
	g_assert (reg_id > 0);

	return TRUE;
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

	if (!urf_killswitch_register_switch (killswitch)) {
		g_object_unref (killswitch);
		return NULL;
	}

	return killswitch;
}
