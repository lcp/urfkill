/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 David Zeuthen <davidz@redhat.com>
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@novell.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>

#include <polkit/polkit.h>
#include <polkit-dbus/polkit-dbus.h>

#include "egg-debug.h"

#include "urf-polkit.h"
#include "urf-daemon.h"

#define URF_POLKIT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), URF_TYPE_POLKIT, UrfPolkitPrivate))

struct UrfPolkitPrivate
{
	PolKitContext           *pk_context;
	DBusConnection		*connection;
};

G_DEFINE_TYPE (UrfPolkit, urf_polkit, G_TYPE_OBJECT)
static gpointer urf_polkit_object = NULL;

/**
 * urf_polkit_caller_new_from_sender:
 **/
PolKitCaller *
urf_polkit_caller_new_from_sender (UrfPolkit *polkit, const gchar *sender)
{
	PolKitCaller *caller;
	DBusError dbus_error;

	g_return_val_if_fail (URF_IS_POLKIT (polkit), NULL);

	/* get the PolKitCaller information */
	dbus_error_init (&dbus_error);
	caller = polkit_caller_new_from_dbus_name (polkit->priv->connection, sender, &dbus_error);
	if (dbus_error_is_set (&dbus_error)) {
		egg_debug ("failed to get caller %s: %s", dbus_error.name, dbus_error.message);
		dbus_error_free (&dbus_error);
	}

	return caller;
}

/**
 * urf_polkit_caller_unref:
 **/
void
urf_polkit_caller_unref (PolKitCaller *caller)
{
	if (caller != NULL)
		polkit_caller_unref (caller);
}

/**
 * urf_polkit_check_auth:
 **/
gboolean
urf_polkit_check_auth (UrfPolkit *polkit, PolKitCaller *caller, const gchar *action_id, DBusGMethodInvocation *context)
{
	gboolean ret = FALSE;
	GError *error;
	GError *error_local = NULL;
	PolKitAction *action;
	PolKitResult result;

	/* set action */
	action = polkit_action_new ();
	if (action == NULL) {
		egg_warning ("error: polkit_action_new failed");
		goto out;
	}
	polkit_action_set_action_id (action, action_id);

	/* check auth */
	result = polkit_context_is_caller_authorized (polkit->priv->pk_context, action, caller, TRUE, NULL);
	if (result == POLKIT_RESULT_YES) {
		ret = TRUE;
	} else {
		error = g_error_new (URF_DAEMON_ERROR, URF_DAEMON_ERROR_GENERAL, "not authorized");
		dbus_g_method_return_error (context, error);
		g_error_free (error);
	}
out:
	return ret;
}

/**
 * urf_polkit_finalize:
 **/
static void
urf_polkit_finalize (GObject *object)
{
	UrfPolkit *polkit;
	g_return_if_fail (URF_IS_POLKIT (object));
	polkit = URF_POLKIT (object);

	polkit_context_unref (polkit->priv->pk_context);

	G_OBJECT_CLASS (urf_polkit_parent_class)->finalize (object);
}

/**
 * urf_polkit_class_init:
 **/
static void
urf_polkit_class_init (UrfPolkitClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = urf_polkit_finalize;
	g_type_class_add_private (klass, sizeof (UrfPolkitPrivate));
}

/**
 * urf_polkit_io_watch_have_data:
 **/
static gboolean
urf_polkit_io_watch_have_data (GIOChannel *channel, GIOCondition condition, gpointer user_data)
{
	int fd;
	PolKitContext *pk_context = user_data;
	fd = g_io_channel_unix_get_fd (channel);
	polkit_context_io_func (pk_context, fd);
	return TRUE;
}

/**
 * urf_polkit_io_add_watch:
 **/
static int
urf_polkit_io_add_watch (PolKitContext *pk_context, int fd)
{
	guint id = 0;
	GIOChannel *channel;
	channel = g_io_channel_unix_new (fd);
	if (channel == NULL)
		return id;
	id = g_io_add_watch (channel, G_IO_IN, urf_polkit_io_watch_have_data, pk_context);
	if (id == 0) {
		g_io_channel_unref (channel);
		return id;
	}
	g_io_channel_unref (channel);
	return id;
}

/**
 * urf_polkit_io_remove_watch:
 **/
static void
urf_polkit_io_remove_watch (PolKitContext *pk_context, int watch_id)
{
        g_source_remove (watch_id);
}

/**
 * urf_polkit_init:
 *
 * initializes the polkit class. NOTE: We expect polkit objects
 * to *NOT* be removed or added during the session.
 * We only control the first polkit object if there are more than one.
 **/
static void
urf_polkit_init (UrfPolkit *polkit)
{
	GError *error = NULL;
	DBusError dbus_error;
	PolKitError *pk_error;
	polkit_bool_t retval;

	polkit->priv = URF_POLKIT_GET_PRIVATE (polkit);

	/* get a connection to the bus */
	dbus_error_init (&dbus_error);
	polkit->priv->connection = dbus_bus_get (DBUS_BUS_SYSTEM, &dbus_error);
	if (polkit->priv->connection == NULL) {
		if (dbus_error_is_set (&dbus_error)) {
			egg_warning ("failed to get system connection %s: %s\n", dbus_error.name, dbus_error.message);
			dbus_error_free (&dbus_error);
		}
	}

	/* get PolicyKit context */
	polkit->priv->pk_context = polkit_context_new ();

	/* watch for changes */
	polkit_context_set_io_watch_functions (polkit->priv->pk_context,
					       urf_polkit_io_add_watch,
					       urf_polkit_io_remove_watch);

	pk_error = NULL;
	retval = polkit_context_init (polkit->priv->pk_context, &pk_error);
	if (retval == FALSE) {
		egg_warning ("Could not init PolicyKit context: %s", polkit_error_get_error_message (pk_error));
		polkit_error_free (pk_error);
	}
}

/**
 * urf_polkit_new:
 * Return value: A new polkit class instance.
 **/
UrfPolkit *
urf_polkit_new (void)
{
	if (urf_polkit_object != NULL) {
		g_object_ref (urf_polkit_object);
	} else {
		urf_polkit_object = g_object_new (URF_TYPE_POLKIT, NULL);
		g_object_add_weak_pointer (urf_polkit_object, &urf_polkit_object);
	}
	return URF_POLKIT (urf_polkit_object);
}

