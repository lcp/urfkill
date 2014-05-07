/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@suse.com>
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
#include <linux/input.h>
#include <linux/rfkill.h>

#include "urf-polkit.h"
#include "urf-daemon.h"
#include "urf-arbitrator.h"
#include "urf-input.h"
#include "urf-utils.h"
#include "urf-config.h"
#include "urf-ofono-manager.h"

#if defined SESSION_TRACKING_CK
#include "urf-session-checker-consolekit.h"
#elif defined SESSION_TRACKING_SYSTEMD
#include "urf-session-checker-logind.h"
#else
#include "urf-session-checker-none.h"
#endif

#define URFKILL_DBUS_INTERFACE "org.freedesktop.URfkill"
#define URFKILL_OBJECT_PATH "/org/freedesktop/URfkill"

static const char introspection_xml[] =
"<node>"
"  <interface name='org.freedesktop.URfkill'>"
"    <method name='Block'>"
"      <arg type='u' name='type' direction='in'/>"
"      <arg type='b' name='block' direction='in'/>"
"      <arg type='b' name='ret' direction='out'/>"
"    </method>"
"    <method name='BlockIdx'>"
"      <arg type='u' name='index' direction='in'/>"
"      <arg type='b' name='block' direction='in'/>"
"      <arg type='b' name='ret' direction='out'/>"
"    </method>"
"    <method name='EnumerateDevices'>"
"      <arg type='ao' name='array' direction='out'/>"
"    </method>"
"    <method name='IsFlightMode'>"
"      <arg type='b' name='is_flight_mode' direction='out'/>"
"    </method>"
"    <method name='FlightMode'>"
"      <arg type='b' name='block' direction='in'/>"
"      <arg type='b' name='ret' direction='out'/>"
"    </method>"
"    <method name='IsInhibited'>"
"      <arg type='b' name='is_inhibited' direction='out'/>"
"    </method>"
"    <method name='Inhibit'>"
"      <arg type='s' name='reason' direction='in'/>"
"      <arg type='u' name='inhibit_cookie' direction='out'/>"
"    </method>"
"    <method name='Uninhibit'>"
"      <arg type='u' name='inhibit_cookie' direction='in'/>"
"    </method>"
"    <signal name='DeviceAdded'>"
"      <arg type='o' name='device' direction='out'/>"
"    </signal>"
"    <signal name='DeviceRemoved'>"
"      <arg type='o' name='device' direction='out'/>"
"    </signal>"
"    <signal name='DeviceChanged'>"
"      <arg type='o' name='device' direction='out'/>"
"    </signal>"
"    <signal name='FlightModeChanged'>"
"      <arg type='b' name='flight_mode' direction='out'/>"
"    </signal>"
"    <signal name='UrfkeyPressed'>"
"      <arg type='i' name='keycode' direction='out'/>"
"    </signal>"
"    <property name='DaemonVersion' type='s' access='read'/>"
"    <property name='KeyControl' type='b' access='read'/>"
"  </interface>"
"</node>";

static const GDBusErrorEntry urf_daemon_error_entries[] =
{
	{URF_DAEMON_ERROR_GENERAL, "org.freedesktop.URfkill.Daemon.Error.General"},
};

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
	SIGNAL_FLIGHT_MODE_CHANGED,
	SIGNAL_URFKEY_PRESSED,
	SIGNAL_LAST,
};

static guint signals[SIGNAL_LAST] = { 0 };

struct UrfDaemonPrivate
{
	UrfConfig		*config;
	UrfPolkit		*polkit;
	UrfArbitrator		*arbitrator;
	UrfInput		*input;
	UrfSessionChecker	*session_checker;
	UrfOfonoManager		*ofono_manager;
	gboolean		 key_control;
	gboolean		 flight_mode;
	gboolean		 master_key;
	GDBusConnection		*connection;
	GDBusNodeInfo		*introspection_data;
};

static void urf_daemon_dispose (GObject *object);

G_DEFINE_TYPE (UrfDaemon, urf_daemon, G_TYPE_OBJECT)

#define URF_DAEMON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
				URF_TYPE_DAEMON, UrfDaemonPrivate))

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
	UrfArbitrator *arbitrator = priv->arbitrator;
	gint type;
	gboolean block = FALSE;
	GError *error = NULL;

	if (urf_session_checker_is_inhibited (priv->session_checker))
		goto out;

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

	switch (urf_arbitrator_get_state (arbitrator, type)) {
	case KILLSWITCH_STATE_UNBLOCKED:
	case KILLSWITCH_STATE_HARD_BLOCKED:
		block = TRUE;
		break;
	case KILLSWITCH_STATE_SOFT_BLOCKED:
		block = FALSE;
		break;
	case KILLSWITCH_STATE_NO_ADAPTER:
	default:
		goto out;
	}

	if (priv->master_key)
		type = RFKILL_TYPE_ALL;

	urf_arbitrator_set_block (arbitrator, type, block);
out:
	g_signal_emit (daemon, signals[SIGNAL_URFKEY_PRESSED], 0, code);
	g_dbus_connection_emit_signal (priv->connection,
	                               NULL,
	                               URFKILL_OBJECT_PATH,
	                               URFKILL_DBUS_INTERFACE,
	                               "UrfkeyPressed",
	                               g_variant_new ("(i)", code),
	                               &error);
	if (error) {
		g_warning ("Failed to emit UrfkeyPressed: %s", error->message);
		g_error_free (error);
	}
}

/**
 * urf_daemon_block:
 **/
gboolean
urf_daemon_block (UrfDaemon             *daemon,
		  const guint            type,
		  const gboolean         block,
		  GDBusMethodInvocation *invocation)
{
	UrfDaemonPrivate *priv = daemon->priv;
	PolkitSubject *subject = NULL;
	gboolean ret = FALSE;

	if (!urf_arbitrator_has_devices (priv->arbitrator))
		goto out;

	subject = urf_polkit_get_subject (priv->polkit, invocation);
	if (subject == NULL)
		goto out;

	if (!urf_polkit_check_auth (priv->polkit, subject, "org.freedesktop.urfkill.block", invocation))
		goto out;

	ret = urf_arbitrator_set_block (priv->arbitrator, type, block);

	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(b)", ret));
out:
	if (subject != NULL)
		g_object_unref (subject);

	return ret;
}

/**
 * urf_daemon_block_idx:
 **/
gboolean
urf_daemon_block_idx (UrfDaemon             *daemon,
		      const guint            index,
		      const gboolean         block,
		      GDBusMethodInvocation *invocation)
{
	UrfDaemonPrivate *priv = daemon->priv;
	PolkitSubject *subject = NULL;
	gboolean ret = FALSE;

	if (!urf_arbitrator_has_devices (priv->arbitrator))
		goto out;

	subject = urf_polkit_get_subject (priv->polkit, invocation);
	if (subject == NULL)
		goto out;

	if (!urf_polkit_check_auth (priv->polkit, subject, "org.freedesktop.urfkill.blockidx", invocation))
		goto out;

	ret = urf_arbitrator_set_block_idx (priv->arbitrator, index, block);

	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(b)", ret));
out:
	if (subject != NULL)
		g_object_unref (subject);

	return ret;
}

/**
 * urf_daemon_enumerate_devices:
 **/
gboolean
urf_daemon_enumerate_devices (UrfDaemon             *daemon,
			      GDBusMethodInvocation *invocation)
{
	UrfDaemonPrivate *priv = daemon->priv;
	UrfDevice *device;
	GVariantBuilder *builder;
	GVariant *tuple = NULL;
	GVariant *value = NULL;
	GList *devices = NULL, *item = NULL;

	g_return_val_if_fail (URF_IS_DAEMON (daemon), FALSE);

	devices = urf_arbitrator_get_devices (priv->arbitrator);

	builder = g_variant_builder_new (G_VARIANT_TYPE("ao"));

	for (item = devices; item; item = g_list_next (item)) {
		device = (UrfDevice *)item->data;
		value = g_variant_new_object_path (urf_device_get_object_path (device));
		g_variant_builder_add_value (builder, value);
	}

	value = g_variant_builder_end (builder);
	tuple = g_variant_new_tuple (&value, 1);
	g_dbus_method_invocation_return_value (invocation, tuple);
	g_variant_builder_unref (builder);
	return TRUE;
}

/**
 * urf_daemon_is_flight_mode:
 **/
gboolean
urf_daemon_is_flight_mode (UrfDaemon             *daemon,
			   GDBusMethodInvocation *invocation)
{
	UrfDaemonPrivate *priv = daemon->priv;
	GVariant *value;

	value = g_variant_new ("(b)", priv->flight_mode);
	g_dbus_method_invocation_return_value (invocation, value);

	return TRUE;
}

/**
 * urf_daemon_flight_mode:
 **/
gboolean
urf_daemon_flight_mode (UrfDaemon             *daemon,
			const gboolean         block,
			GDBusMethodInvocation *invocation)
{
	UrfDaemonPrivate *priv = daemon->priv;
	PolkitSubject *subject = NULL;
	gboolean ret = FALSE;
	GError *error = NULL;

	if (!urf_arbitrator_has_devices (priv->arbitrator))
		goto out;

	subject = urf_polkit_get_subject (priv->polkit, invocation);
	if (subject == NULL)
		goto out;

	if (!urf_polkit_check_auth (priv->polkit, subject, "org.freedesktop.urfkill.flight_mode", invocation))
		goto out;

	ret = urf_arbitrator_set_flight_mode (priv->arbitrator, block);

	if (ret == TRUE) {
		priv->flight_mode = block;
		urf_config_set_persist_state (priv->config, RFKILL_TYPE_ALL,
		                              block
		                              ? KILLSWITCH_STATE_SOFT_BLOCKED
		                              : KILLSWITCH_STATE_UNBLOCKED);

		g_signal_emit (daemon, signals[SIGNAL_FLIGHT_MODE_CHANGED], 0, priv->flight_mode);
		g_dbus_connection_emit_signal (priv->connection,
		                               NULL,
		                               URFKILL_OBJECT_PATH,
		                               URFKILL_DBUS_INTERFACE,
		                               "FlightModeChanged",
		                               g_variant_new ("(b)", priv->flight_mode),
		                               &error);
		if (error) {
			g_warning ("Failed to emit UrfkeyPressed: %s", error->message);
			g_error_free (error);
		}
	}

	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(b)", ret));
out:
	if (subject != NULL)
		g_object_unref (subject);

	return ret;
}

/**
 * urf_daemon_is_inhibited:
 **/
gboolean
urf_daemon_is_inhibited (UrfDaemon             *daemon,
			 GDBusMethodInvocation *invocation)
{
	UrfDaemonPrivate *priv = daemon->priv;
	GVariant *value;

	value = g_variant_new ("(b)", urf_session_checker_is_inhibited (priv->session_checker));
	g_dbus_method_invocation_return_value (invocation, value);

	return TRUE;
}

/**
 * urf_daemon_inhibit:
 **/
gboolean
urf_daemon_inhibit (UrfDaemon             *daemon,
		    const char            *reason,
		    GDBusMethodInvocation *invocation)
{
	UrfDaemonPrivate *priv = daemon->priv;
	const char *bus_name;
	guint cookie = 0;

	bus_name = g_dbus_method_invocation_get_sender (invocation);
	cookie = urf_session_checker_inhibit (priv->session_checker, bus_name, reason);
	g_dbus_method_invocation_return_value (invocation,
	                                       g_variant_new ("(u)", cookie));

	return TRUE;
}

/**
 * urf_daemon_uninhibit:
 **/
void
urf_daemon_uninhibit (UrfDaemon             *daemon,
		      const guint            cookie,
		      GDBusMethodInvocation *invocation)
{
	urf_session_checker_uninhibit (daemon->priv->session_checker, cookie);
}

static void
handle_method_call_main (UrfDaemon             *daemon,
                         const gchar           *method_name,
                         GVariant              *parameters,
                         GDBusMethodInvocation *invocation)
{
	if (g_strcmp0 (method_name, "Block") == 0) {
		guint type;
		gboolean block;
		g_variant_get (parameters, "(ub)", &type, &block);
		urf_daemon_block (daemon, type, block, invocation);
		return;
	} else if (g_strcmp0 (method_name, "BlockIdx") == 0) {
		guint index;
		gboolean block;
		g_variant_get (parameters, "(ub)", &index, &block);
		urf_daemon_block_idx (daemon, index, block, invocation);
		return;
	} else if (g_strcmp0 (method_name, "EnumerateDevices") == 0) {
		urf_daemon_enumerate_devices (daemon, invocation);
		return;
	} else if (g_strcmp0 (method_name, "IsFlightMode") == 0) {
		urf_daemon_is_flight_mode (daemon, invocation);
		return;
	} else if (g_strcmp0 (method_name, "FlightMode") == 0) {
		gboolean block;
		g_variant_get (parameters, "(b)", &block);
		urf_daemon_flight_mode (daemon, block, invocation);
		return;
	} else if (g_strcmp0 (method_name, "IsInhibited") == 0) {
		urf_daemon_is_inhibited (daemon, invocation);
		return;
	} else if (g_strcmp0 (method_name, "Inhibit") == 0) {
		char *reason;
		g_variant_get (parameters, "(s)", &reason);
		urf_daemon_inhibit (daemon, reason, invocation);
		return;
	} else if (g_strcmp0 (method_name, "Uninhibit") == 0) {
		guint cookie;
		g_variant_get (parameters, "(u)", &cookie);
		urf_daemon_uninhibit (daemon, cookie, invocation);
		return;
	}

	g_assert_not_reached ();
}

static void
handle_method_call (GDBusConnection       *connection,
                    const gchar           *sender,
                    const gchar           *object_path,
                    const gchar           *interface_name,
                    const gchar           *method_name,
                    GVariant              *parameters,
                    GDBusMethodInvocation *invocation,
                    gpointer               user_data)
{
	UrfDaemon *daemon = URF_DAEMON (user_data);

	if (g_strcmp0 (interface_name, URFKILL_DBUS_INTERFACE) == 0) {
		handle_method_call_main (daemon,
		                         method_name,
		                         parameters,
		                         invocation);
	} else {
		g_warning ("not recognised interface: %s", interface_name);
	}
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
	UrfDaemon *daemon = URF_DAEMON (user_data);
	GVariant *retval = NULL;

	if (g_strcmp0 (property_name, "DaemonVersion") == 0)
		retval = g_variant_new_string (PACKAGE_VERSION);
	else if (g_strcmp0 (property_name, "KeyControl") == 0)
		retval = g_variant_new_boolean (daemon->priv->key_control);

	return retval;
}

static const GDBusInterfaceVTable interface_vtable =
{
	handle_method_call,
	handle_get_property,
	NULL, /* handle set_property */
};

/**
 * urf_daemon_register_rfkill_daemon:
 **/
static gboolean
urf_daemon_register_rfkill_daemon (UrfDaemon *daemon)
{
	UrfDaemonPrivate *priv = daemon->priv;
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

	/* register GObject */
	infos = priv->introspection_data->interfaces;
	reg_id = g_dbus_connection_register_object (priv->connection,
		                                    URFKILL_OBJECT_PATH,
		                                    infos[0],
		                                    &interface_vtable,
		                                    daemon,
		                                    NULL,
		                                    NULL);
	g_assert (reg_id > 0);

	return TRUE;
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

	/* start up the arbitrator */
	ret = urf_arbitrator_startup (priv->arbitrator, priv->config);
	if (!ret) {
		g_warning ("failed to setup arbitrator");
		goto out;
	}

	ret = urf_ofono_manager_startup (priv->ofono_manager, priv->arbitrator);

	if (priv->key_control) {
		/* start up input device monitor */
		ret = urf_input_startup (priv->input);
		if (!ret) {
			g_warning ("failed to setup input device monitor");
			goto out;
		}

		/* start up session checker */
		ret = urf_session_checker_startup (priv->session_checker);
		if (!ret) {
			g_warning ("failed to setup session checker");
			goto out;
		}
	}
out:
	return ret;
}

/**
 * urf_daemon_device_added_cb:
 **/
static void
urf_daemon_device_added_cb (UrfArbitrator *arbitrator,
			    const char    *object_path,
			    UrfDaemon     *daemon)
{
	UrfDaemonPrivate *priv = daemon->priv;
	GError *error = NULL;

	g_return_if_fail (URF_IS_DAEMON (daemon));
	g_return_if_fail (URF_IS_ARBITRATOR (arbitrator));

	if (object_path == NULL) {
		g_warning ("Invalid object path");
		return;
	}
	g_signal_emit (daemon, signals[SIGNAL_DEVICE_ADDED], 0, object_path);
	g_dbus_connection_emit_signal (priv->connection,
	                               NULL,
	                               URFKILL_OBJECT_PATH,
	                               URFKILL_DBUS_INTERFACE,
	                               "DeviceAdded",
	                               g_variant_new ("(o)", object_path),
	                               &error);
	if (error) {
		g_warning ("Failed to emit DeviceAdded: %s", error->message);
		g_error_free (error);
	}
}

/**
 * urf_daemon_device_removed_cb:
 **/
static void
urf_daemon_device_removed_cb (UrfArbitrator *arbitrator,
			      const char    *object_path,
			      UrfDaemon     *daemon)
{
	UrfDaemonPrivate *priv = daemon->priv;
	GError *error = NULL;

	g_return_if_fail (URF_IS_DAEMON (daemon));
	g_return_if_fail (URF_IS_ARBITRATOR (arbitrator));
	if (object_path == NULL) {
		g_warning ("Invalid object path");
		return;
	}
	g_signal_emit (daemon, signals[SIGNAL_DEVICE_REMOVED], 0, object_path);
	g_dbus_connection_emit_signal (priv->connection,
	                               NULL,
	                               URFKILL_OBJECT_PATH,
	                               URFKILL_DBUS_INTERFACE,
	                               "DeviceRemoved",
	                               g_variant_new ("(o)", object_path),
	                               &error);
	if (error) {
		g_warning ("Failed to emit DeviceRemoved: %s", error->message);
		g_error_free (error);
	}
}

/**
 * urf_daemon_device_changed_cb:
 **/
static void
urf_daemon_device_changed_cb (UrfArbitrator *arbitrator,
			      const char    *object_path,
			      UrfDaemon     *daemon)
{
	UrfDaemonPrivate *priv = daemon->priv;
	GError *error = NULL;

	g_return_if_fail (URF_IS_DAEMON (daemon));
	g_return_if_fail (URF_IS_ARBITRATOR (arbitrator));
	if (object_path == NULL) {
		g_warning ("Invalid object path");
		return;
	}
	g_signal_emit (daemon, signals[SIGNAL_DEVICE_CHANGED], 0, object_path);
	g_dbus_connection_emit_signal (priv->connection,
	                               NULL,
	                               URFKILL_OBJECT_PATH,
	                               URFKILL_DBUS_INTERFACE,
	                               "DeviceChanged",
	                               g_variant_new ("(o)", object_path),
	                               &error);
	if (error) {
		g_warning ("Failed to emit DeviceChanged: %s", error->message);
		g_error_free (error);
	}
}

/**
 * urf_daemon_init:
 **/
static void
urf_daemon_init (UrfDaemon *daemon)
{
	daemon->priv = URF_DAEMON_GET_PRIVATE (daemon);
	daemon->priv->polkit = urf_polkit_new ();

	daemon->priv->arbitrator = urf_arbitrator_new ();
	g_signal_connect (daemon->priv->arbitrator, "device-added",
			  G_CALLBACK (urf_daemon_device_added_cb), daemon);
	g_signal_connect (daemon->priv->arbitrator, "device-removed",
			  G_CALLBACK (urf_daemon_device_removed_cb), daemon);
	g_signal_connect (daemon->priv->arbitrator, "device-changed",
			  G_CALLBACK (urf_daemon_device_changed_cb), daemon);

	daemon->priv->ofono_manager = urf_ofono_manager_new ();

	daemon->priv->input = urf_input_new ();
	g_signal_connect (daemon->priv->input, "rf-key-pressed",
			  G_CALLBACK (urf_daemon_input_event_cb), daemon);

	daemon->priv->session_checker = urf_session_checker_new ();
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
	static volatile gsize quark_volatile = 0;
	g_dbus_error_register_error_domain ("urf_daemon_error",
	                                     &quark_volatile,
	                                     urf_daemon_error_entries,
	                                     G_N_ELEMENTS (urf_daemon_error_entries));
	G_STATIC_ASSERT (G_N_ELEMENTS (urf_daemon_error_entries) - 1 == URF_DAEMON_ERROR_GENERAL);
	return (GQuark)quark_volatile;
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
			      g_cclosure_marshal_VOID__STRING,
			      G_TYPE_NONE, 1, G_TYPE_STRING);

	signals[SIGNAL_FLIGHT_MODE_CHANGED] =
		g_signal_new ("flight-mode-changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__BOOLEAN,
			      G_TYPE_NONE, 1, G_TYPE_BOOLEAN);

	signals[SIGNAL_URFKEY_PRESSED] =
		g_signal_new ("urfkey-pressed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

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
}

/**
 * urf_daemon_dispose:
 **/
static void
urf_daemon_dispose (GObject *object)
{
	UrfDaemon *daemon = URF_DAEMON (object);
	UrfDaemonPrivate *priv = daemon->priv;

	if (priv->ofono_manager) {
		g_object_unref (priv->ofono_manager);
		priv->ofono_manager = NULL;
	}

	if (priv->connection) {
		g_object_unref (priv->connection);
		priv->connection = NULL;
	}

	if (priv->polkit) {
		g_object_unref (priv->polkit);
		priv->polkit = NULL;
	}

	if (priv->arbitrator) {
		g_object_unref (priv->arbitrator);
		priv->arbitrator = NULL;
	}

	if (priv->session_checker) {
		g_object_unref (priv->session_checker);
		priv->session_checker = NULL;
	}

	if (priv->config) {
		g_object_unref (priv->config);
		priv->config = NULL;
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
	daemon->priv->flight_mode = urf_config_get_persist_state (config, RFKILL_TYPE_ALL);
	return daemon;
}
