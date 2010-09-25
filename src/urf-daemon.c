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
#  include "config.h"
#endif

#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "egg-debug.h"

#include "urf-polkit.h"
#include "urf-daemon.h"
#include "urf-killswitch.h"

#include "urf-daemon-glue.h"
#include "urf-marshal.h"

enum
{
	PROP_0,
	PROP_DAEMON_VERSION,
	PROP_LAST
};

enum
{
	SIGNAL_CHANGED,
	SIGNAL_LAST,
};

static guint signals[SIGNAL_LAST] = { 0 };

struct UrfDaemonPrivate
{
	DBusGConnection	*connection;
	DBusGProxy	*proxy;
	UrfPolkit	*polkit;
	UrfKillswitch   *killswitch;
};

static void urf_daemon_finalize (GObject *object);

G_DEFINE_TYPE (UrfDaemon, urf_daemon, G_TYPE_OBJECT)

#define URF_DAEMON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_DAEMON, UrfDaemonPrivate))

#define URF_DAEMON_STATES_STRUCT_TYPE (dbus_g_type_get_struct ("GValueArray",	\
								G_TYPE_UINT,	\
								G_TYPE_UINT,	\
								G_TYPE_INT,	\
								G_TYPE_STRING,	\
								G_TYPE_INVALID))

/**
 * urf_daemon_register_rfkill_daemon:
 **/
static gboolean
urf_daemon_register_rfkill_daemon (UrfDaemon *daemon)
{
	GError *error = NULL;
	gboolean ret = FALSE;
	UrfDaemonPrivate *priv = URF_DAEMON_GET_PRIVATE (daemon);

	priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (priv->connection == NULL) {
		if (error != NULL) {
			g_critical ("error getting system bus: %s", error->message);
			g_error_free (error);
		}
		goto out;
	}

	/* connect to DBUS */
	priv->proxy = dbus_g_proxy_new_for_name (priv->connection,
						 DBUS_SERVICE_DBUS,
						 DBUS_PATH_DBUS,
						 DBUS_INTERFACE_DBUS);

	/* register GObject */
	dbus_g_connection_register_g_object (priv->connection,
					     "/org/freedesktop/URfkill",
					     G_OBJECT (daemon));

	/* success */
	ret = TRUE;
out:
	return ret;
}

/**
 * urf_daemon_startup:
 **/
gboolean
urf_daemon_startup (UrfDaemon *daemon)
{
	gboolean ret;
	UrfDaemonPrivate *priv = URF_DAEMON_GET_PRIVATE (daemon);

	/* register on bus */
	ret = urf_daemon_register_rfkill_daemon (daemon);
	if (!ret) {
		egg_warning ("failed to register");
		goto out;
	}

out:
	return ret;
}

/**
 * urf_daemon_block:
 **/
gboolean
urf_daemon_block (UrfDaemon *daemon, const char *type_name, DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = URF_DAEMON_GET_PRIVATE (daemon);
	int type;
	gboolean ret;

	if (!urf_killswitch_has_killswitches (priv->killswitch))
		return FALSE;

	type = urf_killswitch_rf_type (priv->killswitch, type_name);
	if (type < 0)
		return FALSE;

	ret = urf_killswitch_set_state (priv->killswitch, type, KILLSWITCH_STATE_SOFT_BLOCKED);

	dbus_g_method_return (context, ret);

	return TRUE;
}

/**
 * urf_daemon_unblock:
 **/
gboolean
urf_daemon_unblock (UrfDaemon *daemon, const char *type_name, DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = URF_DAEMON_GET_PRIVATE (daemon);
	int type;
	gboolean ret;

	if (!urf_killswitch_has_killswitches (priv->killswitch))
		return FALSE;

	type = urf_killswitch_rf_type (priv->killswitch, type_name);
	if (type < 0)
		return FALSE;

	ret = urf_killswitch_set_state (priv->killswitch, type, KILLSWITCH_STATE_UNBLOCKED);

	dbus_g_method_return (context, ret);

	return TRUE;
}

/**
 * get_rfkill_name:
 **/
static char *
get_rfkill_name (guint index)
{
	char *filename;
	char *content;
	gsize length;
	GError *error = NULL;

	filename = g_strdup_printf ("/sys/class/rfkill/rfkill%u/name", index);

	g_file_get_contents(filename, &content, &length, &error);
	g_free (filename);

	if (!error) {
		g_warning ("Get rfkill name: %s", error->message);
		return NULL;
	}

	return content;
}

/**
 * urf_daemon_get_all_states:
 **/
gboolean
urf_daemon_get_all_states (UrfDaemon *daemon, DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = URF_DAEMON_GET_PRIVATE (daemon);
	GError *error;
	GPtrArray *array;
	GList *killswitches = NULL, *item = NULL;
	GValue *value;
	UrfIndKillswitch *ind;
	char *device_name;

	g_return_val_if_fail (URF_IS_DAEMON (daemon), FALSE);

	killswitches = urf_killswitch_get_killswitches (priv->killswitch);

	array = g_ptr_array_sized_new (g_list_length(killswitches));
	for (item = killswitches; item; item = g_list_next (item)) {
		ind = (UrfIndKillswitch *)item->data;

		device_name = get_rfkill_name (ind->index);

		value = g_new0 (GValue, 1);
		g_value_init (value, URF_DAEMON_STATES_STRUCT_TYPE);
		g_value_take_boxed (value, dbus_g_type_specialized_construct (URF_DAEMON_STATES_STRUCT_TYPE));
		dbus_g_type_struct_set (value,
					0, ind->index,
					1, ind->type,
					2, ind->state,
					3, device_name, -1);
		g_ptr_array_add (array, g_value_get_boxed (value));
		g_free (value);
	}

	dbus_g_method_return (context, array);

	return TRUE;
}

/**
 * urf_daemon_killswitch_state_cb:
 **/
static void
urf_daemon_killswitch_state_cb (UrfKillswitch *killswitch, KillswitchState state)
{
	/* TODO */
	/* Take care of state change */
}

/**
 * urf_daemon_init:
 **/
static void
urf_daemon_init (UrfDaemon *daemon)
{
	gboolean ret;
	GError *error = NULL;

	daemon->priv = URF_DAEMON_GET_PRIVATE (daemon);
	daemon->priv->polkit = urf_polkit_new ();

	daemon->priv->killswitch = urf_killswitch_new ();
	g_signal_connect (daemon->priv->killswitch, "state-changed",
			  G_CALLBACK (urf_daemon_killswitch_state_cb), daemon);
}

/**
 *  * urf_daemon_error_quark:
 *   **/
GQuark
urf_daemon_error_quark (void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string ("urf_daemon_error");
	return ret;
}

/**
 * urf_daemon_get_property:
 **/
static void
urf_daemon_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UrfDaemon *daemon = URF_DAEMON (object);
	UrfDaemonPrivate *priv = URF_DAEMON_GET_PRIVATE (daemon);

	switch (prop_id) {
	case PROP_DAEMON_VERSION:
		g_value_set_string (value, PACKAGE_VERSION);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_daemon_set_property:
 **/
static void
urf_daemon_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

#define ENUM_ENTRY(NAME, DESC) { NAME, "" #NAME "", DESC }
/**
 * urf_daemon_error_get_type:
 **/
GType
urf_daemon_error_get_type (void)
{
	static GType etype = 0;

	if (etype == 0) {
		static const GEnumValue values[] = {
			ENUM_ENTRY (URF_DAEMON_ERROR_GENERAL, "GeneralError"),
			ENUM_ENTRY (URF_DAEMON_ERROR_NOT_SUPPORTED, "NotSupported"),
			ENUM_ENTRY (URF_DAEMON_ERROR_NO_SUCH_DEVICE, "NoSuchDevice"),
			{ 0, 0, 0 }
		};
		g_assert (URF_DAEMON_NUM_ERRORS == G_N_ELEMENTS (values) - 1);
		etype = g_enum_register_static ("UrfDaemonError", values);
	}
	return etype;
}

/**
 * urf_daemon_class_init:
 **/
static void
urf_daemon_class_init (UrfDaemonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = urf_daemon_finalize;
	object_class->get_property = urf_daemon_get_property;
	object_class->set_property = urf_daemon_set_property;

	g_type_class_add_private (klass, sizeof (UrfDaemonPrivate));

	signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_object_class_install_property (object_class,
					 PROP_DAEMON_VERSION,
					 g_param_spec_string ("daemon-version",
							      "Daemon Version",
							      "The version of the running daemon",
							      NULL,
							      G_PARAM_READABLE));

	dbus_g_object_type_install_info (URF_TYPE_DAEMON, &dbus_glib_urf_daemon_object_info);
	dbus_g_error_domain_register (URF_DAEMON_ERROR, NULL, URF_DAEMON_TYPE_ERROR);
}

/**
 * urf_daemon_finalize:
 **/
static void
urf_daemon_finalize (GObject *object)
{
	UrfDaemon *daemon = URF_DAEMON (object);
	UrfDaemonPrivate *priv = URF_DAEMON_GET_PRIVATE (daemon);

	if (priv->proxy != NULL)
		g_object_unref (priv->proxy);
	if (priv->connection != NULL)
		dbus_g_connection_unref (priv->connection);

	g_object_unref (priv->polkit);
	g_object_unref (priv->killswitch);
	G_OBJECT_CLASS (urf_daemon_parent_class)->finalize (object);
}

/**
 * urf_daemon_new:
 **/
UrfDaemon *
urf_daemon_new (void)
{
	UrfDaemon *daemon;
	daemon = URF_DAEMON (g_object_new (URF_TYPE_DAEMON, NULL));
	return daemon;
}
