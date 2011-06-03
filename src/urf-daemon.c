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
#include <linux/input.h>
#include <linux/rfkill.h>

#include "urf-polkit.h"
#include "urf-daemon.h"
#include "urf-killswitch.h"
#include "urf-input.h"
#include "urf-utils.h"
#include "urf-config.h"
#include "urf-consolekit.h"

#include "urf-daemon-glue.h"
#include "urf-marshal.h"

enum
{
	PROP_0,
	PROP_DAEMON_VERSION,
	PROP_KEY_CONTROL,
	PROP_LAST
};

enum
{
	SIGNAL_DEVICE_ADDED,
	SIGNAL_DEVICE_REMOVED,
	SIGNAL_DEVICE_CHANGED,
	SIGNAL_LAST,
};

static guint signals[SIGNAL_LAST] = { 0 };

struct UrfDaemonPrivate
{
	UrfConfig	*config;
	DBusGConnection	*connection;
	UrfPolkit	*polkit;
	UrfKillswitch   *killswitch;
	UrfInput	*input;
	UrfConsolekit	*consolekit;
	gboolean	 key_control;
	gboolean	 master_key;
};

static void urf_daemon_dispose (GObject *object);

G_DEFINE_TYPE (UrfDaemon, urf_daemon, G_TYPE_OBJECT)

#define URF_DAEMON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_DAEMON, UrfDaemonPrivate))

/**
 * urf_daemon_register_rfkill_daemon:
 **/
static gboolean
urf_daemon_register_rfkill_daemon (UrfDaemon *daemon)
{
	GError *error = NULL;
	gboolean ret = FALSE;
	UrfDaemonPrivate *priv = daemon->priv;

	priv->connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (priv->connection == NULL) {
		if (error != NULL) {
			g_critical ("error getting system bus: %s", error->message);
			g_error_free (error);
		}
		goto out;
	}

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
 * urf_daemon_input_event_cb:
 **/
static void
urf_daemon_input_event_cb (UrfInput *input,
			   guint     code,
			   gpointer  data)
{
	UrfDaemon *daemon = URF_DAEMON (data);
	UrfDaemonPrivate *priv = daemon->priv;
	UrfKillswitch *killswitch = priv->killswitch;
	gint type;
	gboolean block = FALSE;

	if (urf_consolekit_is_inhibited (priv->consolekit))
		return;

	switch (code) {
	case KEY_WLAN:
		type = RFKILL_TYPE_WLAN;
		break;
	case KEY_BLUETOOTH:
		type = RFKILL_TYPE_BLUETOOTH;
		break;
	case KEY_UWB:
		type = RFKILL_TYPE_UWB;
		break;
	case KEY_WIMAX:
		type = RFKILL_TYPE_WIMAX;
		break;
#ifdef KEY_RFKILL
	case KEY_RFKILL:
		type = RFKILL_TYPE_ALL;
		break;
#endif
	default:
		return;
	}

	switch (urf_killswitch_get_state (killswitch, type)) {
	case KILLSWITCH_STATE_UNBLOCKED:
	case KILLSWITCH_STATE_HARD_BLOCKED:
		block = TRUE;
		break;
	case KILLSWITCH_STATE_SOFT_BLOCKED:
		block = FALSE;
		break;
	case KILLSWITCH_STATE_NO_ADAPTER:
	default:
		return;
	}

	if (priv->master_key)
		type = RFKILL_TYPE_ALL;

	urf_killswitch_set_block (killswitch, type, block);
}

/**
 * urf_daemon_startup:
 **/
gboolean
urf_daemon_startup (UrfDaemon *daemon)
{
	UrfDaemonPrivate *priv = daemon->priv;
	gboolean ret;

	/* register on bus */
	ret = urf_daemon_register_rfkill_daemon (daemon);
	if (!ret) {
		g_warning ("failed to register");
		goto out;
	}

	/* start up the killswitch */
	ret = urf_killswitch_startup (priv->killswitch, priv->config);
	if (!ret) {
		g_warning ("failed to setup killswitch");
		goto out;
	}

	if (priv->key_control) {
		/* start up input device monitor */
		ret = urf_input_startup (priv->input);
		if (!ret) {
			g_warning ("failed to setup input device monitor");
			goto out;
		}

		/* start up consolekit checker */
		ret = urf_consolekit_startup (priv->consolekit);
		if (!ret) {
			g_warning ("failed to setup consolekit session checker");
			goto out;
		}
	}
out:
	return ret;
}

/**
 * urf_daemon_block:
 **/
gboolean
urf_daemon_block (UrfDaemon             *daemon,
		  const guint            type,
		  const gboolean         block,
		  DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = daemon->priv;
	PolkitSubject *subject = NULL;
	gboolean ret = FALSE;

	if (!urf_killswitch_has_devices (priv->killswitch))
		goto out;

	subject = urf_polkit_get_subject (priv->polkit, context);
	if (subject == NULL)
		goto out;

	if (!urf_polkit_check_auth (priv->polkit, subject, "org.freedesktop.urfkill.block", context))
		goto out;

	ret = urf_killswitch_set_block (priv->killswitch, type, block);
out:
	if (subject != NULL)
		g_object_unref (subject);

	dbus_g_method_return (context, ret);

	return ret;
}

/**
 * urf_daemon_block_idx:
 **/
gboolean
urf_daemon_block_idx (UrfDaemon             *daemon,
		      const guint            index,
		      const gboolean         block,
		      DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = daemon->priv;
	PolkitSubject *subject = NULL;
	gboolean ret = FALSE;

	if (!urf_killswitch_has_devices (priv->killswitch))
		goto out;

	subject = urf_polkit_get_subject (priv->polkit, context);
	if (subject == NULL)
		goto out;

	if (!urf_polkit_check_auth (priv->polkit, subject, "org.freedesktop.urfkill.blockidx", context))
		goto out;

	ret = urf_killswitch_set_block_idx (priv->killswitch, index, block);
out:
	if (subject != NULL)
		g_object_unref (subject);

	dbus_g_method_return (context, ret);

	return ret;
}

/**
 * urf_daemon_get_all:
 **/
gboolean
urf_daemon_enumerate_devices (UrfDaemon             *daemon,
			      DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = daemon->priv;
	UrfDevice *device;
	GPtrArray *object_paths;
	GList *devices = NULL, *item = NULL;

	g_return_val_if_fail (URF_IS_DAEMON (daemon), FALSE);

	devices = urf_killswitch_get_devices (priv->killswitch);

	object_paths = g_ptr_array_sized_new (g_list_length(devices));
	g_ptr_array_set_free_func (object_paths, g_free);
	for (item = devices; item; item = g_list_next (item)) {
		device = (UrfDevice *)item->data;
		g_ptr_array_add (object_paths, g_strdup (urf_device_get_object_path (device)));
	}

	dbus_g_method_return (context, object_paths);

	g_ptr_array_unref (object_paths);

	return TRUE;
}

/**
 * urf_daemon_is_inhibited:
 **/
gboolean
urf_daemon_is_inhibited (UrfDaemon             *daemon,
			 DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = daemon->priv;

	dbus_g_method_return (context,
			      urf_consolekit_is_inhibited (priv->consolekit));

	return TRUE;
}

/**
 * urf_daemon_inhibit:
 **/
gboolean
urf_daemon_inhibit (UrfDaemon             *daemon,
		    const char            *reason,
		    DBusGMethodInvocation *context)
{
	UrfDaemonPrivate *priv = daemon->priv;
	char *bus_name;
	guint cookie = 0;
	GError *error = NULL;

	bus_name = dbus_g_method_get_sender (context);
	cookie = urf_consolekit_inhibit (priv->consolekit, bus_name, reason);
	dbus_g_method_return (context, cookie);

	return TRUE;
}

/**
 * urf_daemon_uninhibit:
 **/
void
urf_daemon_uninhibit (UrfDaemon             *daemon,
		      const guint            cookie,
		      DBusGMethodInvocation *context)
{
	urf_consolekit_uninhibit (daemon->priv->consolekit, cookie);
}

/**
 * urf_daemon_device_added_cb:
 **/
static void
urf_daemon_device_added_cb (UrfKillswitch *killswitch,
			    const char    *object_path,
			    UrfDaemon     *daemon)
{
	g_return_if_fail (URF_IS_DAEMON (daemon));
	g_return_if_fail (URF_IS_KILLSWITCH (killswitch));

	if (object_path == NULL) {
		g_warning ("Invalid object path");
		return;
	}
	g_signal_emit (daemon, signals[SIGNAL_DEVICE_ADDED], 0, object_path);
}

/**
 * urf_daemon_device_removed_cb:
 **/
static void
urf_daemon_device_removed_cb (UrfKillswitch *killswitch,
			      const char    *object_path,
			      UrfDaemon     *daemon)
{
	g_return_if_fail (URF_IS_DAEMON (daemon));
	g_return_if_fail (URF_IS_KILLSWITCH (killswitch));
	if (object_path == NULL) {
		g_warning ("Invalid object path");
		return;
	}
	g_signal_emit (daemon, signals[SIGNAL_DEVICE_REMOVED], 0, object_path);
}

/**
 * urf_daemon_device_changed_cb:
 **/
static void
urf_daemon_device_changed_cb (UrfKillswitch *killswitch,
			      const guint    index,
			      UrfDaemon     *daemon)
{
	UrfDevice *device;

	g_return_if_fail (URF_IS_DAEMON (daemon));
	g_return_if_fail (URF_IS_KILLSWITCH (killswitch));

	device = urf_killswitch_get_device (killswitch, index);

	if (!device)
		return;

	g_signal_emit (daemon, signals[SIGNAL_DEVICE_CHANGED], 0,
		       urf_device_get_object_path (device),
		       urf_device_get_soft (device),
		       urf_device_get_hard (device));
	g_object_unref (device);
}

/**
 * urf_daemon_init:
 **/
static void
urf_daemon_init (UrfDaemon *daemon)
{
	daemon->priv = URF_DAEMON_GET_PRIVATE (daemon);
	daemon->priv->polkit = urf_polkit_new ();

	daemon->priv->killswitch = urf_killswitch_new ();
	g_signal_connect (daemon->priv->killswitch, "device-added",
			  G_CALLBACK (urf_daemon_device_added_cb), daemon);
	g_signal_connect (daemon->priv->killswitch, "device-removed",
			  G_CALLBACK (urf_daemon_device_removed_cb), daemon);
	g_signal_connect (daemon->priv->killswitch, "device-changed",
			  G_CALLBACK (urf_daemon_device_changed_cb), daemon);

	daemon->priv->input = urf_input_new ();
	g_signal_connect (daemon->priv->input, "rf-key-pressed",
			  G_CALLBACK (urf_daemon_input_event_cb), daemon);

	daemon->priv->consolekit = urf_consolekit_new ();
}

/**
 * urf_daemon_get_property:
 **/
static void
urf_daemon_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	UrfDaemon *daemon = URF_DAEMON (object);

	switch (prop_id) {
	case PROP_DAEMON_VERSION:
		g_value_set_string (value, PACKAGE_VERSION);
		break;
	case PROP_KEY_CONTROL:
		g_value_set_boolean (value, daemon->priv->key_control);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_daemon_error_quark:
 **/
GQuark
urf_daemon_error_quark (void)
{
	static GQuark ret = 0;
	if (ret == 0)
		ret = g_quark_from_static_string ("urf_daemon_error");
	return ret;
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
	object_class->dispose = urf_daemon_dispose;
	object_class->get_property = urf_daemon_get_property;

	g_type_class_add_private (klass, sizeof (UrfDaemonPrivate));

	signals[SIGNAL_DEVICE_ADDED] =
		g_signal_new ("device-added",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[SIGNAL_DEVICE_REMOVED] =
		g_signal_new ("device-removed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[SIGNAL_DEVICE_CHANGED] =
		g_signal_new ("device-changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      urf_marshal_VOID__STRING_BOOLEAN_BOOLEAN,
			      G_TYPE_NONE, 3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN);

	g_object_class_install_property (object_class,
					 PROP_DAEMON_VERSION,
					 g_param_spec_string ("daemon-version",
							      "Daemon Version",
							      "The version of the running daemon",
							      NULL,
							      G_PARAM_READABLE));

	g_object_class_install_property (object_class,
					 PROP_KEY_CONTROL,
					 g_param_spec_boolean ("key-control",
							       "Key Control",
							       "Whether the key control is enabled",
							       FALSE,
							       G_PARAM_READABLE));

	dbus_g_object_type_install_info (URF_TYPE_DAEMON, &dbus_glib_urf_daemon_object_info);
	dbus_g_error_domain_register (URF_DAEMON_ERROR, NULL, URF_DAEMON_TYPE_ERROR);
}

/**
 * urf_daemon_dispose:
 **/
static void
urf_daemon_dispose (GObject *object)
{
	UrfDaemon *daemon = URF_DAEMON (object);
	UrfDaemonPrivate *priv = daemon->priv;

	if (priv->config) {
		g_object_unref (priv->config);
		priv->config = NULL;
	}

	if (priv->connection) {
		dbus_g_connection_unref (priv->connection);
		priv->connection = NULL;
	}

	if (priv->polkit) {
		g_object_unref (priv->polkit);
		priv->polkit = NULL;
	}

	if (priv->killswitch) {
		g_object_unref (priv->killswitch);
		priv->killswitch = NULL;
	}

	if (priv->consolekit) {
		g_object_unref (priv->consolekit);
		priv->consolekit = NULL;
	}

	G_OBJECT_CLASS (urf_daemon_parent_class)->dispose (object);
}

/**
 * urf_daemon_new:
 **/
UrfDaemon *
urf_daemon_new (UrfConfig *config)
{
	UrfDaemon *daemon;
	daemon = URF_DAEMON (g_object_new (URF_TYPE_DAEMON, NULL));
	daemon->priv->config = g_object_ref (config);
	daemon->priv->key_control = urf_config_get_key_control (config);
	daemon->priv->master_key = urf_config_get_master_key (config);
	return daemon;
}
