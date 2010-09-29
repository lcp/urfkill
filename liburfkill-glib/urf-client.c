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
#include "urf-marshal.h"

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
 * urf_client_get_all:
 **/
GList *
urf_client_get_all (UrfClient *client, GCancellable *cancellable, GError **error)
{
	GError *error_local = NULL;
	GType g_type_gvalue_array;
	GPtrArray *gvalue_ptr_array = NULL;
	GValueArray *gva;
	GValue *gv;
	guint i;
	UrfKillswitch *killswitch;
	GList *killswitches = NULL;
	gboolean ret;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	g_type_gvalue_array = dbus_g_type_get_collection ("GPtrArray",
						dbus_g_type_get_struct("GValueArray",
							G_TYPE_UINT,
							G_TYPE_UINT,
							G_TYPE_INT,
							G_TYPE_UINT,
							G_TYPE_UINT,
							G_TYPE_STRING,
							G_TYPE_INVALID));

	ret = dbus_g_proxy_call (client->priv->proxy, "GetAll", &error_local,
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
	for (i=0; i<gvalue_ptr_array->len; i++) {
		gva = (GValueArray *) g_ptr_array_index (gvalue_ptr_array, i);
		killswitch = g_new0 (UrfKillswitch, 1);

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
		/* 3: soft */
		gv = g_value_array_get_nth (gva, 3);
		killswitch->soft = g_value_get_uint (gv);
		g_value_unset (gv);
		/* 4: hard */
		gv = g_value_array_get_nth (gva, 4);
		killswitch->hard = g_value_get_uint (gv);
		g_value_unset (gv);
		/* 5: name */
		gv = g_value_array_get_nth (gva, 5);
		killswitch->name = g_value_dup_string (gv);
		g_value_unset (gv);

		killswitches = g_list_append (killswitches, (gpointer)killswitch);
		g_value_array_free (gva);
	}
out:
	if (gvalue_ptr_array != NULL)
		g_ptr_array_free (gvalue_ptr_array, TRUE);
	return killswitches;
}

/**
 * urf_client_get_killswitch:
 **/
UrfKillswitch *
urf_client_get_killswitch (UrfClient *client, const guint index, GCancellable *cancellable, GError **error)
{
	UrfKillswitch *killswitch = NULL;
	gboolean ret;
	int type, state;
	guint soft, hard;
	char *name = NULL;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	ret = dbus_g_proxy_call (client->priv->proxy, "GetKillswitch", &error_local,
				 G_TYPE_UINT, index,
				 G_TYPE_INVALID,
				 G_TYPE_INT, &type,
				 G_TYPE_INT, &state,
				 G_TYPE_UINT, &soft,
				 G_TYPE_UINT, &hard,
				 G_TYPE_STRING, &name,
				 G_TYPE_INVALID);
	if (!ret) {
		/* DBus might time out, which is okay */
		if (g_error_matches (error_local, DBUS_GERROR, DBUS_GERROR_NO_REPLY)) {
			g_debug ("DBUS timed out, but recovering");
		}

		/* an actual error */
		g_warning ("Couldn't got killswitch: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		goto out;
	}

	if (type < 0) {
		killswitch = NULL;
		goto out;
	}

	killswitch = g_new0 (UrfKillswitch, 1);
	killswitch->index = index;
	killswitch->type  = type;
	killswitch->state = state;
	killswitch->soft  = soft;
	killswitch->hard  = hard;
	killswitch->name  = g_strdup (name);

out:
	if (error_local != NULL)
		g_error_free (error_local);

	return killswitch;
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

/**
 * urf_rfkill_added_cb:
 **/
static void
urf_rfkill_added_cb (DBusGProxy *proxy,
		     const guint index,
		     const guint type,
		     const gint state,
		     const guint soft,
		     const guint hard,
		     const gchar *name,
		     UrfClient *client)
{
	UrfKillswitch *killswitch;

	killswitch = g_new0 (UrfKillswitch, 1);
	killswitch->index = index;
	killswitch->type  = type;
	killswitch->state = state;
	killswitch->soft  = soft;
	killswitch->hard  = hard;
	killswitch->name  = g_strdup (name);

	g_signal_emit (client, signals [URF_CLIENT_RFKILL_ADDED], 0, (gpointer)killswitch);
}

/**
 * urf_rfkill_removed_cb:
 **/
static void
urf_rfkill_removed_cb (DBusGProxy *proxy, guint index, UrfClient *client)
{
	g_signal_emit (client, signals [URF_CLIENT_RFKILL_REMOVED], 0, index);
}

/**
 * urf_rfkill_changed_cb:
 **/
static void
urf_rfkill_changed_cb (DBusGProxy *proxy,
		       const guint index,
		       const guint type,
		       const gint state,
		       const guint soft,
		       const guint hard,
		       const gchar *name,
		       UrfClient *client)
{
	UrfKillswitch *killswitch;

	killswitch = g_new0 (UrfKillswitch, 1);
	killswitch->index = index;
	killswitch->type  = type;
	killswitch->state = state;
	killswitch->soft  = soft;
	killswitch->hard  = hard;
	killswitch->name  = g_strdup (name);

	g_signal_emit (client, signals [URF_CLIENT_RFKILL_CHANGED], 0, (gpointer)killswitch);
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
        signals[URF_CLIENT_RFKILL_ADDED] =
                g_signal_new ("rfkill-added",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfClientClass, rfkill_added),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

        signals[URF_CLIENT_RFKILL_REMOVED] =
                g_signal_new ("rfkill-removed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfClientClass, rfkill_removed),
			      NULL, NULL, g_cclosure_marshal_VOID__UINT,
                              G_TYPE_NONE, 1, G_TYPE_UINT);

        signals[URF_CLIENT_RFKILL_CHANGED] =
                g_signal_new ("rfkill-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfClientClass, rfkill_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

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

	dbus_g_object_register_marshaller (urf_marshal_VOID__UINT_UINT_INT_UINT_UINT_STRING,
					   G_TYPE_NONE,
					   G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING,
					   G_TYPE_INVALID);

	dbus_g_proxy_add_signal (client->priv->proxy, "RfkillAdded",
				 G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING,
				 G_TYPE_INVALID);
	dbus_g_proxy_add_signal (client->priv->proxy, "RfkillRemoved",
				 G_TYPE_UINT,
				 G_TYPE_INVALID);
	dbus_g_proxy_add_signal (client->priv->proxy, "RfkillChanged",
				 G_TYPE_UINT, G_TYPE_UINT, G_TYPE_INT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_STRING,
				 G_TYPE_INVALID);
	/* callbacks */
	dbus_g_proxy_connect_signal (client->priv->proxy, "RfkillAdded",
				     G_CALLBACK (urf_rfkill_added_cb), client, NULL);
	dbus_g_proxy_connect_signal (client->priv->proxy, "RfkillRemoved",
				     G_CALLBACK (urf_rfkill_removed_cb), client, NULL);
	dbus_g_proxy_connect_signal (client->priv->proxy, "RfkillChanged",
				     G_CALLBACK (urf_rfkill_changed_cb), client, NULL);

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

