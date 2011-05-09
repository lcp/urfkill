/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@novell.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
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
static void	urf_client_dispose	(GObject	*object);

#define URF_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), URF_TYPE_CLIENT, UrfClientPrivate))

/**
 * UrfClientPrivate:
 *
 * Private #UrfClient data
 **/
struct UrfClientPrivate
{
	DBusGConnection	*bus;
	DBusGProxy	*proxy;
	GPtrArray	*devices;
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
 * urf_client_find_device_by_index
 **/
static UrfDevice *
urf_client_find_device_by_index (UrfClient   *client,
				 const guint  index)
{
	UrfClientPrivate *priv = client->priv;
	UrfDevice *device = NULL;
	guint i, device_index;

	if (priv->devices == NULL)
		return NULL;

	for (i=0; i<priv->devices->len; i++) {
		device = (UrfDevice *) g_ptr_array_index (priv->devices, i);
		g_object_get (device, "index", &device_index, NULL);
		if (index == device_index)
			return device;
	}

	return NULL;
}

/**
 * urf_client_find_device
 **/
static UrfDevice *
urf_client_find_device (UrfClient   *client,
			const char  *object_path)
{
	UrfClientPrivate *priv = client->priv;
	UrfDevice *device = NULL;
	guint i;

	if (priv->devices == NULL)
		return NULL;

	for (i=0; i<priv->devices->len; i++) {
		device = (UrfDevice *) g_ptr_array_index (priv->devices, i);
		if (g_strcmp0 (urf_device_get_object_path (device), object_path) == 0)
			return device;
	}

	return NULL;
}

/**
 * urf_client_get_devices:
 **/
GPtrArray *
urf_client_get_devices (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), NULL);

	return g_ptr_array_ref (client->priv->devices);
}

/**
 * urf_client_block
 **/
gboolean
urf_client_set_block (UrfClient      *client,
		      const char     *type,
		      const gboolean  block,
		      GCancellable   *cancellable,
		      GError        **error)
{
	gboolean ret, status;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	ret = dbus_g_proxy_call (client->priv->proxy, "Block", &error_local,
				 G_TYPE_STRING, type,
				 G_TYPE_BOOLEAN, block,
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
 * urf_client_block_idx
 **/
gboolean
urf_client_set_block_idx (UrfClient      *client,
			  const guint     index,
			  const gboolean  block,
			  GCancellable   *cancellable,
			  GError        **error)
{
	gboolean ret, status;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	ret = dbus_g_proxy_call (client->priv->proxy, "BlockIdx", &error_local,
				 G_TYPE_UINT, index,
				 G_TYPE_BOOLEAN, block,
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
		g_warning ("Couldn't sent BLOCKIDX: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		status = FALSE;
	}
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return status;
}

/**
 * urf_client_key_control_enabled
 **/
gboolean
urf_client_key_control_enabled (UrfClient *client,
				GError   **error)
{
	gboolean ret, status;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	ret = dbus_g_proxy_call (client->priv->proxy, "KeyControlEnabled", &error_local,
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
		g_warning ("Couldn't sent KEYCONTROLENABLED: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		status = FALSE;
	}
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return status;
}

/**
 * urf_client_enable_key_control
 **/
gboolean
urf_client_enable_key_control (UrfClient      *client,
			       const gboolean  enable,
			       GError        **error)
{
	gboolean ret, status;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	ret = dbus_g_proxy_call (client->priv->proxy, "EnableKeyControl", &error_local,
				 G_TYPE_BOOLEAN, enable,
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
		g_warning ("Couldn't sent ENABLEKEYCONTROL: %s", error_local->message);
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
urf_client_set_wlan_block (UrfClient     *client,
			   const gboolean block)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_block (client, "WLAN", block, NULL, NULL);
}

/**
 * urf_client_set_bluetooth_block
 **/
gboolean
urf_client_set_bluetooth_block (UrfClient     *client,
				const gboolean block)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_block (client, "BLUETOOTH", block, NULL, NULL);
}

/**
 * urf_client_set_wwan_block
 **/
gboolean
urf_client_set_wwan_block (UrfClient     *client,
			   const gboolean block)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	return urf_client_set_block (client, "WWAN", block, NULL, NULL);
}

/**
 * urf_client_add
 **/
static UrfDevice *
urf_client_add (UrfClient  *client,
		const char *object_path)
{
	UrfDevice *device;

	device = urf_device_new ();
	urf_device_set_object_path_sync (device, object_path, NULL, NULL);

	g_ptr_array_add (client->priv->devices, device);

	return device;
}

/**
 * urf_rfkill_added_cb:
 **/
static void
urf_rfkill_added_cb (DBusGProxy  *proxy,
		     const gchar *object_path,
		     UrfClient   *client)
{
	UrfDevice *device;

	device = urf_client_find_device (client, object_path);
	if (device != NULL) {
		g_warning ("already added: %s", object_path);
		return;
	}

	device = urf_client_add (client, object_path);

	g_signal_emit (client, signals [URF_CLIENT_RFKILL_ADDED], 0, device);
}

/**
 * urf_rfkill_removed_cb:
 **/
static void
urf_rfkill_removed_cb (DBusGProxy *proxy,
		       guint       index,
		       UrfClient  *client)
{
	UrfClientPrivate *priv = client->priv;
	UrfDevice *device;

	device = urf_client_find_device_by_index (client, index);

	if (device == NULL) {
		g_warning ("no such device to be removed: index %u", index);
		return;
	}

	g_signal_emit (client, signals [URF_CLIENT_RFKILL_REMOVED], 0, device);

	g_ptr_array_remove (priv->devices, device);
}

/**
 * urf_rfkill_changed_cb:
 **/
static void
urf_rfkill_changed_cb (DBusGProxy     *proxy,
		       const guint     index,
		       const gboolean  soft,
		       const gboolean  hard,
		       UrfClient      *client)
{
	UrfDevice *device;

	device = urf_client_find_device_by_index (client, index);

	if (device == NULL) {
		g_warning ("no device to be changed: index %u", index);
		return;
	}

	g_object_set (G_OBJECT (device),
		      "soft", soft,
		      "hard", hard,
		      NULL);

	g_signal_emit (client, signals [URF_CLIENT_RFKILL_CHANGED], 0, device);
}

/**
 * urf_client_get_devices_private:
 **/
static void
urf_client_get_devices_private (UrfClient *client,
				GError    **error)
{
	GError *error_local = NULL;
	GType g_type_array;
	GPtrArray *devices = NULL;
	const char *object_path;
	gboolean ret;
	guint i;

	g_return_if_fail (URF_IS_CLIENT (client));
	g_return_if_fail (client->priv->proxy != NULL);

	g_type_array = dbus_g_type_get_collection ("GPtrArray", DBUS_TYPE_G_OBJECT_PATH);
	ret = dbus_g_proxy_call (client->priv->proxy, "EnumerateDevices", &error_local,
				 G_TYPE_INVALID,
				 g_type_array, &devices,
				 G_TYPE_INVALID);
	if (!ret) {
		/* an actual error */
		g_set_error (error, 1, 0, "%s", error_local->message);
		g_error_free (error_local);
		return;
	}

	/* no data */
	if (devices == NULL) {
		g_set_error_literal (error, 1, 0, "no data");
		return;
	}

	/* convert */
	for (i=0; i < devices->len; i++) {
		object_path = (const char *) g_ptr_array_index (devices, i);
		urf_client_add (client, object_path);
	}
}

/*
 * urf_client_class_init:
 * @klass: The UrfClientClass
 */
static void
urf_client_class_init (UrfClientClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->dispose = urf_client_dispose;

	/* install signals */
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
			      NULL, NULL, g_cclosure_marshal_VOID__POINTER,
			      G_TYPE_NONE, 1, G_TYPE_POINTER);

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

	client->priv->devices = g_ptr_array_new_with_free_func ((GDestroyNotify) g_object_unref);

	urf_client_get_devices_private (client, &error);
	if (error) {
		g_warning ("Failed to enumerate devices: %s", error->message);
		g_error_free (error);
		goto out;
	}

	/* connect signals */
	dbus_g_object_register_marshaller (urf_marshal_VOID__UINT_BOOLEAN_BOOLEAN,
					   G_TYPE_NONE,
					   G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
					   G_TYPE_INVALID);

	dbus_g_proxy_add_signal (client->priv->proxy, "RfkillAdded",
				 G_TYPE_STRING,
				 G_TYPE_INVALID);
	dbus_g_proxy_add_signal (client->priv->proxy, "RfkillRemoved",
				 G_TYPE_UINT,
				 G_TYPE_INVALID);
	dbus_g_proxy_add_signal (client->priv->proxy, "RfkillChanged",
				 G_TYPE_UINT, G_TYPE_BOOLEAN, G_TYPE_BOOLEAN,
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
 * urf_client_dispose:
 */
static void
urf_client_dispose (GObject *object)
{
	UrfClient *client;

	g_return_if_fail (URF_IS_CLIENT (object));

	client = URF_CLIENT (object);

	if (client->priv->bus) {
		dbus_g_connection_unref (client->priv->bus);
		client->priv->bus = NULL;
	}

	if (client->priv->proxy) {
		g_object_unref (client->priv->proxy);
		client->priv->proxy = NULL;
	}

	if (client->priv->devices) {
		g_ptr_array_unref (client->priv->devices);
		client->priv->devices = NULL;
	}

	G_OBJECT_CLASS (urf_client_parent_class)->dispose (object);
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

