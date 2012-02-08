/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2010-2011 Gary Ching-Pang Lin <glin@suse.com>
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

/**
 * SECTION:urf-client
 * @short_description: Main client object for accessing the urfkill daemon
 * @title: UrfClient
 * @include: urfkill.h
 * @see_also: #UrfDevice
 *
 * A helper GObject to use for accessing urfkill information, and to be
 * notified when it is changed.
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>

#include "urf-client.h"

static void	urf_client_class_init	(UrfClientClass	*klass);
static void	urf_client_init		(UrfClient	*client);
static void	urf_client_dispose	(GObject	*object);
static void	urf_client_finalize	(GObject	*object);

#define URF_CLIENT_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), URF_TYPE_CLIENT, UrfClientPrivate))

struct _UrfClientPrivate
{
	GDBusProxy	*proxy;
	GList		*devices;
	char		*daemon_version;
	gboolean	 key_control;
	gboolean	 have_properties;
	gboolean	 is_enumerated;
};

enum {
	URF_CLIENT_DEVICE_ADDED,
	URF_CLIENT_DEVICE_REMOVED,
	URF_CLIENT_DEVICE_CHANGED,
	URF_CLIENT_RF_KEY_PRESSED,
	URF_CLIENT_LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_DAEMON_VERSION,
	PROP_KEY_CONTROL,
	PROP_LAST
};

static guint signals [URF_CLIENT_LAST_SIGNAL] = { 0 };
static gpointer urf_client_object = NULL;

G_DEFINE_TYPE (UrfClient, urf_client, G_TYPE_OBJECT)

/**
 * urf_client_find_device:
 **/
static UrfDevice *
urf_client_find_device (UrfClient   *client,
			const char  *object_path)
{
	UrfClientPrivate *priv = client->priv;
	UrfDevice *device = NULL;
	GList *item;

	if (priv->devices == NULL)
		return NULL;

	for (item = priv->devices; item; item = item->next) {
		device = (UrfDevice *) item->data;
		if (g_strcmp0 (urf_device_get_object_path (device), object_path) == 0)
			return device;
	}

	return NULL;
}

/**
 * urf_client_add:
 **/
static UrfDevice *
urf_client_add (UrfClient  *client,
		const char *object_path)
{
	UrfDevice *device;

	device = urf_device_new ();
	urf_device_set_object_path_sync (device, object_path, NULL, NULL);

	client->priv->devices = g_list_append (client->priv->devices, device);

	return device;
}

/**
 * urf_client_device_added:
 **/
static void
urf_client_device_added (UrfClient   *client,
			 const gchar *object_path)
{
	UrfDevice *device;

	device = urf_client_find_device (client, object_path);
	if (device != NULL) {
		g_warning ("already added: %s", object_path);
		return;
	}

	device = urf_client_add (client, object_path);

	g_signal_emit (client, signals [URF_CLIENT_DEVICE_ADDED], 0, device);
}

/**
 * urf_client_device_removed:
 **/
static void
urf_client_device_removed (UrfClient  *client,
			   const char *object_path)
{
	UrfClientPrivate *priv = client->priv;
	UrfDevice *device;

	device = urf_client_find_device (client, object_path);

	if (device == NULL) {
		g_warning ("no such device to be removed: %s", object_path);
		return;
	}

	client->priv->devices = g_list_remove (priv->devices, device);

	g_signal_emit (client, signals [URF_CLIENT_DEVICE_REMOVED], 0, device);

	g_object_unref (device);
}

/**
 * urf_client_device_changed:
 **/
static void
urf_client_device_changed (UrfClient  *client,
			   const char *object_path)
{
	UrfDevice *device;

	device = urf_client_find_device (client, object_path);

	if (device == NULL) {
		g_warning ("no device to be changed: %s", object_path);
		return;
	}

	g_signal_emit (client, signals [URF_CLIENT_DEVICE_CHANGED], 0, device);
}

/**
 * urf_client_get_devices_private:
 **/
static void
urf_client_get_devices_private (UrfClient *client,
				GError    **error)
{
	GVariant *retval;
	GVariantIter *iter;
	const char *object_path;
	GError *error_local = NULL;

	g_return_if_fail (URF_IS_CLIENT (client));
	g_return_if_fail (client->priv->proxy != NULL);

	retval = g_dbus_proxy_call_sync (client->priv->proxy, "EnumerateDevices",
	                                 NULL,
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error_local);
	if (error_local) {
		g_set_error (error, 1, 0, "%s", error_local->message);
		g_error_free (error_local);
		return;
	}

	g_variant_get (retval, "(ao)", &iter);
	while (g_variant_iter_loop (iter, "o", &object_path))
		urf_client_add (client, object_path);
	g_variant_iter_free (iter);
	g_variant_unref (retval);
}

/**
 * urf_client_enumerate_devices_sync:
 * @client: a #UrfClient instance
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL
 *
 * Enumerate the devices from the daemon.
 *
 * Return value: #TRUE for success, else #FALSE and @error is used
 *
 * Since: 0.3.0
 **/
gboolean
urf_client_enumerate_devices_sync (UrfClient    *client,
				   GCancellable *cancellable,
				   GError       **error)
{
	UrfClientPrivate *priv = client->priv;
	GError *error_local = NULL;
	gboolean ret = FALSE;

	urf_client_get_devices_private (client, &error_local);
	if (error_local) {
		g_warning ("Failed to enumerate devices: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		goto out;
	}

	priv->is_enumerated = TRUE;

	ret = TRUE;
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return ret;
}

/**
 * urf_client_get_devices:
 * @client: a #UrfClient instance
 *
 * Get a list of the device objects.
 * <note>
 *   <para>
 *     You must have called #urf_client_enumerate_devices_sync before
 *     calling this function.
 *   </para>
 * </note>
 *
 * Return value: (element-type UrfDevice) (transfer none): a list of #UrfDevice objects
 *
 * Since: 0.2.0
 **/
GList *
urf_client_get_devices (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), NULL);

	return client->priv->devices;
}

/**
 * urf_client_set_block:
 * @client: a #UrfClient instance
 * @type: the type of the devices
 * @block: %TRUE to block the devices or %FALSE to unblock
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL
 *
 * Block or unblock the devices belonging to the type.
 *
 * <note>
 *   <para>
 *     This function only changes soft block. Hard block is controlled
 *     by BIOS or the hardware and there is no way to change the
 *     state of hard block through kernel functions.
 *   </para>
 * </note>
 *
 * Return value: #TRUE for success, else #FALSE and @error is used
 *
 * Since: 0.2.0
 **/
gboolean
urf_client_set_block (UrfClient      *client,
		      UrfEnumType     type,
		      const gboolean  block,
		      GCancellable   *cancellable,
		      GError         **error)
{
	GVariant *retval;
	gboolean status = FALSE;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);
	g_return_val_if_fail (type < URF_ENUM_TYPE_NUM, FALSE);

	retval = g_dbus_proxy_call_sync (client->priv->proxy, "Block",
	                                 g_variant_new ("(ub)", type, block),
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error_local);
	if (error_local) {
		g_warning ("Couldn't sent BLOCK: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		goto out;
	}

	g_variant_get (retval, "(b)", &status);
	g_variant_unref (retval);
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return status;
}

/**
 * urf_client_set_block_idx:
 * @client: a #UrfClient instance
 * @index: the index of the device
 * @block: %TRUE to block the device or %FALSE to unblock
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL
 *
 * Block or unblock the device by the index.
 *
 * <note>
 *   <para>
 *     This function only changes soft block. Hard block is controlled
 *     by BIOS or the hardware and there is no way to change the
 *     state of hard block through kernel functions.
 *   </para>
 * </note>
 *
 * Return value: #TRUE for success, else #FALSE and @error is used
 *
 * Since: 0.2.0
 **/
gboolean
urf_client_set_block_idx (UrfClient      *client,
			  const guint     index,
			  const gboolean  block,
			  GCancellable   *cancellable,
			  GError         **error)
{
	GVariant *retval;
	gboolean status = FALSE;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	retval = g_dbus_proxy_call_sync (client->priv->proxy, "BlockIdx",
	                                 g_variant_new ("(ub)", index, block),
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error_local);
	if (error_local) {
		g_warning ("Couldn't sent BLOCKIDX: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		goto out;
	}

	g_variant_get (retval, "(b)", &status);
	g_variant_unref (retval);
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return status;
}

/**
 * urf_client_is_inhibited:
 * @client: a #UrfClient instance
 * @error: a #GError, or %NULL
 *
 * Get whether the key control is inhibited or not,
 *
 * Return value: #TRUE if the key control is inhibited
 *
 * Since: 0.2.0
 **/
gboolean
urf_client_is_inhibited (UrfClient *client,
			 GError    **error)
{
	GVariant *retval;
	gboolean is_inhibited = FALSE;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	retval = g_dbus_proxy_call_sync (client->priv->proxy, "IsInhibited",
	                                 NULL,
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error_local);
	if (error_local) {
		g_warning ("Couldn't sent IsInhibited: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		goto out;
	}

	g_variant_get (retval, "(b)", &is_inhibited);
	g_variant_unref (retval);
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return is_inhibited;

}

/**
 * urf_client_inhibit:
 * @client: a #UrfClient instance
 * @reason: the reason to inhibit the key control
 * @error: a #GError, or %NULL
 *
 * Inhibit the rfkill key handling function for this session.
 *
 * Return value: the cookie and @error is used
 *
 * Since: 0.2.0
 **/
guint
urf_client_inhibit (UrfClient  *client,
		    const char *reason,
		    GError     **error)
{
	GVariant *retval;
	guint cookie = 0;
	GError *error_local = NULL;

	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	g_return_val_if_fail (client->priv->proxy != NULL, FALSE);

	retval = g_dbus_proxy_call_sync (client->priv->proxy, "Inhibit",
	                                 g_variant_new ("(s)", reason),
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, &error_local);
	if (error_local) {
		g_warning ("Couldn't sent INHIBIT: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		goto out;
	}

	g_variant_get (retval, "(u)", &cookie);
	g_variant_unref (retval);
out:
	if (error_local != NULL)
		g_error_free (error_local);
	return cookie;
}

/**
 * urf_client_uninhibit:
 * @client: a #UrfClient instance
 * @cookie: the cookie
 *
 * Cancel a previous call to #urf_client_inhibit identified by the cookie.
 *
 * Since: 0.2.0
 **/
void
urf_client_uninhibit (UrfClient   *client,
		      const guint  cookie)
{
	g_return_if_fail (URF_IS_CLIENT (client));
	g_return_if_fail (client->priv->proxy != NULL);

	g_dbus_proxy_call (client->priv->proxy, "Uninhibit",
	                   g_variant_new ("(u)", cookie),
	                                 G_DBUS_CALL_FLAGS_NONE,
	                                 -1, NULL, NULL, NULL);
}


/**
 * urf_client_set_wlan_block:
 * @client: a #UrfClient instance
 * @block: %TRUE to block the WLAN devices or %FALSE to unblock
 *
 * Block or unblock the WLAN devices. This is a convenient function
 * and the underlying function is #urf_client_set_block.
 *
 * Return value: #TRUE for success, else #FALSE
 *
 * Since: 0.2.0
 **/
gboolean
urf_client_set_wlan_block (UrfClient     *client,
			   const gboolean block)
{
	return urf_client_set_block (client, URF_ENUM_TYPE_WLAN, block, NULL, NULL);
}

/**
 * urf_client_set_bluetooth_block:
 * @client: a #UrfClient instance
 * @block: %TRUE to block the bluetooth devices or %FALSE to unblock
 *
 * Block or unblock the bluetooth devices. This is a convenient function
 * and the underlying function is #urf_client_set_block.
 *
 * Return value: #TRUE for success, else #FALSE
 *
 * Since: 0.2.0
 **/
gboolean
urf_client_set_bluetooth_block (UrfClient     *client,
				const gboolean block)
{
	return urf_client_set_block (client, URF_ENUM_TYPE_BLUETOOTH, block, NULL, NULL);
}

/**
 * urf_client_set_wwan_block:
 * @client: a #UrfClient instance
 * @block: %TRUE to block the WWAN devices or %FALSE to unblock
 *
 * Block or unblock the wireless WAN devices. This is a convenient function
 * and the underlying function is #urf_client_set_block.
 *
 * Return value: #TRUE for success, else #FALSE
 *
 * Since: 0.2.0
 **/
gboolean
urf_client_set_wwan_block (UrfClient     *client,
			   const gboolean block)
{
	return urf_client_set_block (client, URF_ENUM_TYPE_WWAN, block, NULL, NULL);
}

/**
 * urf_client_get_properties_sync:
 **/
static void
urf_client_get_properties_sync (UrfClient *client)
{
	UrfClientPrivate *priv = client->priv;
	GVariant *value;
	gsize length;

	value = g_dbus_proxy_get_cached_property (priv->proxy, "DaemonVersion");
	g_free (priv->daemon_version);
	priv->daemon_version = g_variant_dup_string (value, &length);

	value = g_dbus_proxy_get_cached_property (priv->proxy, "KeyControl");
	priv->key_control = g_variant_get_boolean (value);
}

/**
 * urf_client_get_daemon_version:
 * @client: a #UrfClient instance
 *
 * Get urfkill daemon version
 *
 * Return value: string containing the daemon version, e.g. 0.2.0
 *
 * Since: 0.2.0
 **/
const char *
urf_client_get_daemon_version (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), NULL);
	urf_client_get_properties_sync (client);
	return client->priv->daemon_version;
}

/**
 * urf_client_get_key_control:
 **/
static gboolean
urf_client_get_key_control (UrfClient *client)
{
	g_return_val_if_fail (URF_IS_CLIENT (client), FALSE);
	urf_client_get_properties_sync (client);
	return client->priv->key_control;
}

/**
 * urf_client_get_property:
 **/
static void
urf_client_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	UrfClient *client = URF_CLIENT (object);

	urf_client_get_properties_sync (client);

	switch (prop_id) {
	case PROP_DAEMON_VERSION:
		g_value_set_string (value, urf_client_get_daemon_version (client));
		break;
	case PROP_KEY_CONTROL:
		g_value_set_boolean (value, urf_client_get_key_control (client));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_client_class_init:
 * @klass: The UrfClientClass
 **/
static void
urf_client_class_init (UrfClientClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->get_property = urf_client_get_property;
	object_class->dispose = urf_client_dispose;
	object_class->finalize = urf_client_finalize;

	/**
	 * UrfClient:daemon-version:
	 *
	 * The running daemon version.
	 *
	 * Since: 0.2.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_DAEMON_VERSION,
					 g_param_spec_string ("daemon-version",
							      "Daemon version",
							      "The running daemon version",
							      NULL,
							      G_PARAM_READABLE));

	/**
	 * UrfClient:key-control:
	 *
	 * Whether the key control in the daemon is enabled or not
	 *
	 * Since: 0.2.0
	 **/
	g_object_class_install_property (object_class,
					 PROP_KEY_CONTROL,
					 g_param_spec_boolean ("key-control",
							       "Key Control",
							       "The key control state",
							       FALSE,
							       G_PARAM_READABLE));

	/* install signals */
	/**
	 * UrfClient::device-added:
	 * @client: the #UrfClient instance that emitted the signal
	 * @device: the #UrfDevice that was added.
	 *
	 * The device-added signal is emitted when a rfkill device is added.
	 *
	 * Since 0.2.0
	 **/
        signals[URF_CLIENT_DEVICE_ADDED] =
                g_signal_new ("device-added",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfClientClass, device_added),
			      NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, URF_TYPE_DEVICE);

	/**
	 * UrfClient::device-removed:
	 * @client: the #UrfClient instance that emitted the signal
	 * @device: the #UrfDevice that was removed.
	 *
	 * The device-removed signal is emitted when a rfkill device is removed.
	 *
	 * Since 0.2.0
	 **/
        signals[URF_CLIENT_DEVICE_REMOVED] =
                g_signal_new ("device-removed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfClientClass, device_removed),
			      NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, URF_TYPE_DEVICE);

	/**
	 * UrfClient::device-changed:
	 * @client: the #UrfClient instance that emitted the signal
	 * @device: the #UrfDevice that was changed.
	 *
	 * The device-changed signal is emitted when a rfkill device is changed.
	 *
	 * Since 0.2.0
	 **/
        signals[URF_CLIENT_DEVICE_CHANGED] =
                g_signal_new ("device-changed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfClientClass, device_changed),
			      NULL, NULL, g_cclosure_marshal_VOID__OBJECT,
			      G_TYPE_NONE, 1, URF_TYPE_DEVICE);

	/**
	 * UrfClient::rf-key-pressed:
	 * @client: the #UrfClient instance that emitted the signal
	 * @keycode: the keycode from the input device
	 *
	 * The rf-key-pressed signal is emitted when a rfkill key is pressed.
	 *
	 * Since 0.2.0
	 **/
        signals[URF_CLIENT_RF_KEY_PRESSED] =
                g_signal_new ("rf-key-pressed",
			      G_TYPE_FROM_CLASS (object_class), G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (UrfClientClass, rf_key_pressed),
			      NULL, NULL, g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 1, G_TYPE_INT);

	g_type_class_add_private (klass, sizeof (UrfClientPrivate));
}

/**
 * urf_client_proxy_signal_cb:
 **/
static void
urf_client_proxy_signal_cb (GDBusProxy *proxy,
                            gchar      *sender_name,
                            gchar      *signal_name,
                            GVariant   *parameters,
                            gpointer    user_data)
{
	UrfClient *client = URF_CLIENT (user_data);

	if (g_strcmp0 (signal_name, "UrfkeyPressed") == 0) {
		int keycode;
		g_variant_get (parameters, "(i)", &keycode);
		g_signal_emit (client,
		               signals [URF_CLIENT_RF_KEY_PRESSED],
		               0, keycode);
	}

	if (!client->priv->is_enumerated)
		return;

	if (g_strcmp0 (signal_name, "DeviceAdded") == 0) {
		char *device_path;
		g_variant_get (parameters, "(o)", &device_path);
		urf_client_device_added (client, device_path);
	} else if (g_strcmp0 (signal_name, "DeviceRemoved") == 0) {
		char *device_path;
		g_variant_get (parameters, "(o)", &device_path);
		urf_client_device_removed (client, device_path);
	} else if (g_strcmp0 (signal_name, "DeviceChanged") == 0) {
		char *device_path;
		g_variant_get (parameters, "(o)", &device_path);
		urf_client_device_changed (client, device_path);
	}
}

/**
 * urf_client_init:
 * @client: This class instance
 **/
static void
urf_client_init (UrfClient *client)
{
	GError *error = NULL;

	client->priv = URF_CLIENT_GET_PRIVATE (client);
	client->priv->daemon_version = NULL;
	client->priv->key_control = FALSE;
	client->priv->have_properties = FALSE;
	client->priv->is_enumerated = FALSE;
	client->priv->devices = NULL;

	/* connect to main interface */
	client->priv->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                                     G_DBUS_PROXY_FLAGS_NONE,
	                                                     NULL,
	                                                     "org.freedesktop.URfkill",
	                                                     "/org/freedesktop/URfkill",
	                                                     "org.freedesktop.URfkill",
	                                                     NULL,
	                                                     &error);
	if (error) {
		g_warning ("Couldn't connect to proxy: %s", error->message);
		g_error_free (error);
		return;
	}

	/* callbacks */
	g_signal_connect (client->priv->proxy, "g-signal",
	                  G_CALLBACK (urf_client_proxy_signal_cb), client);
	return;
}

/**
 * urf_client_dispose:
 **/
static void
urf_client_dispose (GObject *object)
{
	UrfClient *client;

	g_return_if_fail (URF_IS_CLIENT (object));

	client = URF_CLIENT (object);

	if (client->priv->proxy) {
		g_object_unref (client->priv->proxy);
		client->priv->proxy = NULL;
	}

	G_OBJECT_CLASS (urf_client_parent_class)->dispose (object);
}

/**
 * urf_client_finalize:
 **/
static void
urf_client_finalize (GObject *object)
{
	UrfClient *client;
	GList *item;

	g_return_if_fail (URF_IS_CLIENT (object));

	client = URF_CLIENT (object);

	g_free (client->priv->daemon_version);

	if (client->priv->devices) {
		for (item = client->priv->devices; item; item = item->next)
			g_object_unref (item->data);
		g_list_free (client->priv->devices);
		client->priv->devices = NULL;
	}

	G_OBJECT_CLASS (urf_client_parent_class)->finalize (object);
}

/**
 * urf_client_new:
 *
 * Creates a new #UrfClient object.
 *
 * Return value: a new #UrfClient object.
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

