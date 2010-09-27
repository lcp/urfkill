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
	URF_CLIENT_RFKILL_ADDED,
	URF_CLIENT_RFKILL_REMOVED,
	URF_CLIENT_RFKILL_CHANGED,
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
 * urf_client_get_all_states:
 **/
GPtrArray *
urf_client_get_all_states (UrfClient *client, GCancellable *cancellable, GError **error)
{
	GError *error_local = NULL;
	GType g_type_gvalue_array;
	GPtrArray *gvalue_ptr_array = NULL;
	GValueArray *gva;
	GValue *gv;
	guint i;
	UrfKillSwitch *killswitch;
	GPtrArray *array = NULL;
	gboolean ret;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	g_type_gvalue_array = dbus_g_type_get_collection ("GPtrArray",
						dbus_g_type_get_struct("GValueArray",
							G_TYPE_UINT,
							G_TYPE_UINT,
							G_TYPE_INT,
							G_TYPE_STRING,
							G_TYPE_INVALID));

	ret = dbus_g_proxy_call (client->priv->proxy, "GetAllStates", &error_local,
				 G_TYPE_INVALID,
				 g_type_gvalue_array, &gvalue_ptr_array,
				 G_TYPE_INVALID);
	if (!ret) {
		/* an actual error */
		g_set_error (error, 1, 0, "%s", error_local->message);
		g_error_free (error_local);
		goto out;
	}

	/* no data */
	if (gvalue_ptr_array->len == 0) {
		g_set_error_literal (error, 1, 0, "no data");
		goto out;
	}

	/* convert */
	array = g_ptr_array_new ();

	for (i=0; i<gvalue_ptr_array->len; i++) {
		gva = (GValueArray *) g_ptr_array_index (gvalue_ptr_array, i);
		killswitch = g_new0 (UrfKillSwitch, 1);

		/* 0: index */
		gv = g_value_array_get_nth (gva, 0);
		killswitch->index = g_value_get_uint (gv);
		g_value_unset (gv);
		/* 1: type */
		gv = g_value_array_get_nth (gva, 1);
		killswitch->type = g_value_get_uint (gv);
		g_value_unset (gv);
		/* 2: state */
		gv = g_value_array_get_nth (gva, 2);
		killswitch->state = g_value_get_int (gv);
		g_value_unset (gv);
		/* 3: name */
		gv = g_value_array_get_nth (gva, 3);
		killswitch->name = g_value_dup_string (gv);
		g_value_unset (gv);

		g_ptr_array_add (array, killswitch);
		g_value_array_free (gva);
	}
out:
	if (gvalue_ptr_array != NULL)
		g_ptr_array_free (gvalue_ptr_array, TRUE);
	return array;
}

/**
 * urf_client_block
 **/
gboolean
urf_client_set_block (UrfClient *client, const char *type, GCancellable *cancellable, GError **error)
{
	gboolean ret, status;
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
        signals[SIGNAL_RFKILL_ADDED] =
                g_signal_new ("rfkill-added",
                              G_OBJECT_CLASS_TYPE (klass),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                              0, NULL, NULL,
                              g_cclosure_marshal_VOID__UINT,
                              G_TYPE_NONE, 1, G_TYPE_UINT);

        signals[SIGNAL_RFKILL_REMOVED] =
                g_signal_new ("rfkill-removed",
                              G_OBJECT_CLASS_TYPE (klass),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                              0, NULL, NULL,
                              g_cclosure_marshal_VOID__UINT,
                              G_TYPE_NONE, 1, G_TYPE_UINT);

        signals[SIGNAL_RFKILL_CHANGED] =
                g_signal_new ("rfkill-changed",
                              G_OBJECT_CLASS_TYPE (klass),
                              G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
                              0, NULL, NULL,
                              g_cclosure_marshal_VOID__UINT,
                              G_TYPE_NONE, 1, G_TYPE_UINT);

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
