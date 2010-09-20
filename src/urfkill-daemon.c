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

#include "urfkill-polkit.h"
#include "urfkill-daemon.h"

#include "urfkill-daemon-glue.h"
#include "urfkill-marshal.h"

enum
{
        PROP_0,
        PROP_DAEMON_VERSION,
        PROP_LAST
};

enum
{
        SIGNAL_DEVICE_ADDED,
        SIGNAL_DEVICE_REMOVED,
        SIGNAL_DEVICE_CHANGED,
        SIGNAL_CHANGED,
        SIGNAL_LAST,
};

static guint signals[SIGNAL_LAST] = { 0 };

struct UrfkillDaemonPrivate
{
	DBusGConnection         *connection;
	DBusGProxy              *proxy;
}

static void urfkill_daemon_finalize (GObject *object);

G_DEFINE_TYPE (UrfkillDaemon, urfkill_daemon, G_TYPE_OBJECT)

#define URFKILL_DAEMON_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), URFKILL_TYPE_DAEMON, UrfkillDaemonPrivate))

/**
 * urfkill_daemon_get_property:
 **/
static void
urfkill_daemon_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UrfkillDaemon *daemon = URFKILL_DAEMON (object);
	UrfkillDaemonPrivate *priv = GET_PRIVATE (daemon);

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
 * urfkill_daemon_set_property:
 **/
static void
urfkill_daemon_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	switch (prop_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}
/**
 * urfkill_daemon_class_init:
 **/
static void
urfkill_daemon_class_init (UpDaemonClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = urfkill_daemon_finalize;
	object_class->get_property = urfkill_daemon_get_property;
	object_class->set_property = urfkill_daemon_set_property;

	g_type_class_add_private (klass, sizeof (UrfkillDaemonPrivate));

	/* TODO setup signals for dbus */

	g_object_class_install_property (object_class,
					 PROP_DAEMON_VERSION,
					 g_param_spec_string ("daemon-version",
							      "Daemon Version",
							      "The version of the running daemon",
							      NULL,
							      G_PARAM_READABLE));

	dbus_g_object_type_install_info (URFKILL_TYPE_DAEMON, &dbus_glib_urfkill_daemon_object_info);
	dbus_g_error_domain_register (URFKILL_DAEMON_ERROR, NULL, URFKILL_DAEMON_TYPE_ERROR);
}

/**
 * urfkill_daemon_finalize:
 **/
static void
urfkill_daemon_finalize (GObject *object)
{
	UrfkillDaemon *daemon = URFKILL_DAEMON (object);
	UrfkillDaemonPrivate *priv = GET_PRIVATE (daemon);

	if (priv->proxy != NULL)
		g_object_unref (priv->proxy);
	if (priv->connection != NULL)
		dbus_g_connection_unref (priv->connection);

	G_OBJECT_CLASS (urfkill_daemon_parent_class)->finalize (object);
}

/**
 * urfkill_daemon_new:
 **/
UrfkillDaemon *
urfkill_daemon_new (void)
{
	UrfkillDaemon *daemon;
	daemon = URFKILL_DAEMON (g_object_new (URFKILL_TYPE_DAEMON, NULL));
	return daemon;
}
