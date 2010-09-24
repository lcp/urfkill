/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
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

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <dbus/dbus-glib.h>

#include "urf-client.h"

static void	urf_client_class_init	(UrfClientClass	*klass);
static void	urf_client_init		(UrfClient	*client);
static void	urf_client_finalize	(GObject	*object);

#define URF_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), URF_TYPE_CLIENT, UrfClientPrivate))

/**
 * UrfClientPrivate:
 *
 * Private #UrfClient data
 **/
struct UrfClientPrivate
{
	DBusGConnection		*bus;
	DBusGProxy		*proxy;
};

enum {
	URF_CLIENT_CHANGED,
	URF_CLIENT_LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_LAST
};

static guint signals [URF_CLIENT_LAST_SIGNAL] = { 0 };
static gpointer urf_client_object = NULL;

G_DEFINE_TYPE (UrfClient, urf_client, G_TYPE_OBJECT)

/**
 * urf_client_block
 **/
gboolean
urf_client_set_block (UrfClient *client, const char *type, GCancellable *cancellable, GError **error)
{
	gboolean ret;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	ret = dbus_g_proxy_call (client->priv->proxy, "Block", &error_local,
				 G_TYPE_STRING, type,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &status,
				 G_TYPE_INVALID);
	if (!ret) {
		/* DBus might time out, which is okay */
		if (g_error_matches (error_local, DBUS_GERROR, DBUS_GERROR_NO_REPLY)) {
			g_debug ("DBUS timed out, but recovering");
			goto out;
		}

		/* an actual error */
		g_warning ("Couldn't sent BLOCK: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		status = FALSE;
	}
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return status;
}

/**
 * urf_client_unblock
 **/
gboolean
urf_client_set_unblock (UrfClient *client, const char *type, GCancellable *cancellable, GError **error)
{
	gboolean ret, status;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	ret = dbus_g_proxy_call (client->priv->proxy, "Unblock", &error_local,
				 G_TYPE_STRING, type,
				 G_TYPE_INVALID,
				 G_TYPE_BOOLEAN, &status,
				 G_TYPE_INVALID);
	if (!ret) {
		/* DBus might time out, which is okay */
		if (g_error_matches (error_local, DBUS_GERROR, DBUS_GERROR_NO_REPLY)) {
			g_debug ("DBUS timed out, but recovering");
			goto out;
		}

		/* an actual error */
		g_warning ("Couldn't sent UNBLOCK: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		status = FALSE;
	}
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return status;
}

/**
 * urf_client_set_wlan_block
 **/
gboolean
urf_client_set_wlan_block (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_block (client, "WLAN", NULL, NULL);
}

/**
 * urf_client_set_wlan_block
 **/
gboolean
urf_client_set_wlan_unblock (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_unblock (client, "WLAN", NULL, NULL);
}

/**
 * urf_client_set_bluetooth_block
 **/
gboolean
urf_client_set_bluetooth_block (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_block (client, "BLUETOOTH", NULL, NULL);
}

/**
 * urf_client_set_bluetooth_unblock
 **/
gboolean
urf_client_set_bluetooth_unblock (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_unblock (client, "BLUETOOTH", NULL, NULL);
}

/**
 * urf_client_set_wwan_block
 **/
gboolean
urf_client_set_wwan_block (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_block (client, "WWAN", NULL, NULL);
}

/**
 * urf_client_set_wwan_unblock
 **/
gboolean
urf_client_set_wwan_unblock (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_unblock (client, "WWAN", NULL, NULL);
}

/*
 * urf_client_class_init:
 * @klass: The UrfClientClass
 */
static void
urf_client_class_init (UrfClientClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	//object_class->get_property = urf_client_get_property;
	object_class->finalize = urf_client_finalize;

	/* TODO */
	/* install properties and signals */

	g_type_class_add_private (klass, sizeof (UrfClientPrivate));
}

/*
 * urf_client_init:
 * @client: This class instance
 */
static void
urf_client_init (UrfClient *client)
{
	GError *error = NULL;

	client->priv = URF_CLIENT_GET_PRIVATE (client);

	/* get on the bus */
	client->priv->bus = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (client->priv->bus == NULL) {
		g_warning ("Couldn't connect to system bus: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* connect to main interface */
	client->priv->proxy = dbus_g_proxy_new_for_name (client->priv->bus,
							 "org.freedesktop.URfkill",
							 "/org/freedesktop/URfkill",
							 "org.freedesktop.URfkill");
	if (client->priv->proxy == NULL) {
		g_warning ("Couldn't connect to proxy");
		goto out;
	}

	/* TODO */
	/* Take care of dbus signals and callback functions */

out:
	return;
}

/*
 * urf_client_finalize:
 */
static void
urf_client_finalize (GObject *object)
{
	UrfClient *client;

	g_return_if_fail (URF_IS_CLIENT (object));

	client = URF_CLIENT (object);

	if (client->priv->bus)
		dbus_g_connection_unref (client->priv->bus);

	if (client->priv->proxy != NULL)
		g_object_unref (client->priv->proxy);

	G_OBJECT_CLASS (urf_client_parent_class)->finalize (object);
}

/**
 * urf_client_new:
 *
 * Creates a new #UrfClient object.
 *
 * Return value: a new UrfClient object.
 *
 **/
UrfClient *
urf_client_new (void)
{
	if (urf_client_object != NULL) {
		g_object_ref (urf_client_object);
	} else {
		urf_client_object = g_object_new (URF_TYPE_CLIENT, NULL);
		g_object_add_weak_pointer (urf_client_object, &urf_client_object);
	}
	return URF_CLIENT (urf_client_object);
}

