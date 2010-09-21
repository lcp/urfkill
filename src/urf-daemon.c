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

//#include "urf-polkit.h"
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
}

static void urf_daemon_finalize (GObject *object);

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

	/* TODO */
	/* start to monitor rfkill interfaces */


out:
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
/**
 * urf_daemon_class_init:
 **/
static void
urf_daemon_class_init (UpDaemonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = urf_daemon_finalize;
	object_class->get_property = urf_daemon_get_property;
	object_class->set_property = urf_daemon_set_property;

	g_type_class_add_private (klass, sizeof (UrfDaemonPrivate));

	/* TODO setup signals for dbus */

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
