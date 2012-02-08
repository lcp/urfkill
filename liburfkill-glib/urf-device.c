/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@suse.com>
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
 * SECTION:urf-device
 * @short_description: Client object for accessing information about rfkill devices
 * @title: UrfDevice
 * @include: urfkill.h
 * @see_also: #UrfClient
 *
 * A helper GObject for accessing rfkill devices
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <gio/gio.h>

#include "urf-device.h"
#include "urf-enum.h"

#define URF_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					URF_TYPE_DEVICE, UrfDevicePrivate))

struct _UrfDevicePrivate
{
	GDBusProxy *proxy;
	char       *object_path;
	guint       index;
	guint       type;
	gboolean    soft;
	gboolean    hard;
	char       *name;
	gboolean    platform;
	gboolean    is_initialized;
};

enum {
	PROP_0,
	PROP_DEVICE_INDEX,
	PROP_DEVICE_TYPE,
	PROP_DEVICE_SOFT,
	PROP_DEVICE_HARD,
	PROP_DEVICE_NAME,
	PROP_DEVICE_PLATFORM,
	PROP_LAST
};

G_DEFINE_TYPE (UrfDevice, urf_device, G_TYPE_OBJECT)

/**
 * urf_device_refresh_private:
 **/
static void
urf_device_refresh_private (UrfDevice *device,
			    GError    **error)
{
	UrfDevicePrivate *priv = device->priv;
	GVariant *value;
	gsize length;

	value = g_dbus_proxy_get_cached_property (priv->proxy, "soft");
	priv->soft = g_variant_get_boolean (value);

	value = g_dbus_proxy_get_cached_property (priv->proxy, "hard");
	priv->hard = g_variant_get_boolean (value);

	if (priv->is_initialized)
		return;

	value = g_dbus_proxy_get_cached_property (priv->proxy, "index");
	priv->index = g_variant_get_uint32 (value);

	value = g_dbus_proxy_get_cached_property (priv->proxy, "type");
	priv->type = g_variant_get_uint32 (value);

	value = g_dbus_proxy_get_cached_property (priv->proxy, "name");
	priv->name = g_variant_dup_string (value, &length);

	value = g_dbus_proxy_get_cached_property (priv->proxy, "platform");
	priv->platform = g_variant_get_boolean (value);
}

/**
 * urf_device_changed_cb:
 **/
static void
urf_device_changed_cb (GDBusProxy *proxy,
                       GVariant   *changed_properties,
                       GStrv       invalidated_properties,
                       gpointer    user_data)
{
	UrfDevice *device = URF_DEVICE (user_data);

	urf_device_refresh_private (device, NULL);
}

/**
 * urf_device_set_object_path_sync:
 * @device: a #UrfDevice instance
 * @object_path: the #UrfDevice object path
 * @cancellable: a #GCancellable or %NULL
 * @error: a #GError, or %NULL
 *
 * Set the object path of the object and fill up the initial properties.
 *
 * Return value: #TRUE for success, else #FALSE and @error is used
 *
 * Since: 0.2.0
 **/
gboolean
urf_device_set_object_path_sync (UrfDevice    *device,
				 const char   *object_path,
				 GCancellable *cancellable,
				 GError       **error)
{
	UrfDevicePrivate *priv = device->priv;
	GError *error_local = NULL;
	gboolean ret = FALSE;

	g_return_val_if_fail (URF_IS_DEVICE (device), FALSE);

	if (device->priv->object_path != NULL)
		return FALSE;
	if (object_path == NULL)
		return FALSE;

	/* invalid */
	if (object_path == NULL || object_path[0] != '/') {
		g_set_error (error, 1, 0, "Object path %s invalid", object_path);
		goto out;
	}

	priv->proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                             G_DBUS_PROXY_FLAGS_NONE,
	                                             NULL,
	                                             "org.freedesktop.URfkill",
	                                             object_path,
	                                             "org.freedesktop.URfkill.Device",
	                                             NULL,
	                                             &error_local);
	if (error_local) {
		g_warning ("Couldn't connect to proxy: %s", error_local->message);
		g_set_error (error, 1, 0, "%s", error_local->message);
		return FALSE;
	}

        priv->object_path = g_strdup (object_path);

	urf_device_refresh_private (device, &error_local);
	priv->is_initialized = TRUE;

	/* connect signals */
	g_signal_connect (priv->proxy, "g-properties-changed",
	                  G_CALLBACK (urf_device_changed_cb), device);
out:
	return ret;
}

/**
 * urf_device_get_object_path:
 * @device: a #UrfDevice instance
 *
 * Get the object path for the device.
 *
 * Return value: the object path, or %NULL
 *
 * Since: 0.2.0
 **/
const char *
urf_device_get_object_path (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), NULL);

	return device->priv->object_path;
}

/**
 * set_block_idx_cb:
 **/
static void
set_block_idx_cb (GDBusProxy     *proxy,
                  GAsyncResult   *res,
                  gpointer        user_data)
{
	GVariant *retval;
	gboolean status;
	GError *error = NULL;

	retval = g_dbus_proxy_call_finish (proxy, res, &error);
	if (retval)
		g_variant_get (retval, "(b)", &status);

	if (error) {
		g_warning ("Failed to set BLOCK: %s", error->message);
		g_error_free (error);
	} else if (!status) {
		g_warning ("Failed to set BLOCK");
	}

	g_object_unref (proxy);
}

/**
 * urf_device_set_block:
 **/
static void
urf_device_set_block (UrfDevice *device,
		      gboolean   block)
{
	GDBusProxy *proxy;
	GError *error = NULL;

	proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
	                                       G_DBUS_PROXY_FLAGS_NONE,
	                                       NULL,
	                                       "org.freedesktop.URfkill",
	                                       "/org/freedesktop/URfkill",
	                                       "org.freedesktop.URfkill",
	                                       NULL,
	                                       &error);
	if (error) {
		g_warning ("Couldn't connect to proxy to set block: %s",
		           error->message);
		g_error_free (error);
		return;
	}

	g_dbus_proxy_call (proxy, "BlockIdx",
	                   g_variant_new ("(ub)",
	                                  device->priv->index,
	                                  block),
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1, NULL,
	                   (GAsyncReadyCallback) set_block_idx_cb,
	                   device);
}

/**
 * urf_device_set_property:
 **/
static void
urf_device_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	UrfDevice *device = URF_DEVICE (object);
	gboolean block;

	switch (prop_id) {
	case PROP_DEVICE_SOFT:
		block = g_value_get_boolean (value);
		urf_device_set_block (device, block);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_device_get_property:
 **/
static void
urf_device_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	UrfDevice *device = URF_DEVICE (object);
	UrfDevicePrivate *priv = device->priv;

	switch (prop_id) {
	case PROP_DEVICE_INDEX:
		g_value_set_uint (value, priv->index);
		break;
	case PROP_DEVICE_TYPE:
		g_value_set_uint (value, priv->type);
		break;
	case PROP_DEVICE_SOFT:
		g_value_set_boolean (value, priv->soft);
		break;
	case PROP_DEVICE_HARD:
		g_value_set_boolean (value, priv->hard);
		break;
	case PROP_DEVICE_NAME:
		g_value_set_string (value, priv->name);
		break;
	case PROP_DEVICE_PLATFORM:
		g_value_set_boolean (value, priv->platform);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_device_finalize:
 **/
static void
urf_device_finalize (GObject *object)
{
	UrfDevicePrivate *priv;

	g_return_if_fail (URF_IS_DEVICE (object));

	priv = URF_DEVICE (object)->priv;

	g_free (priv->name);
	g_free (priv->object_path);

	G_OBJECT_CLASS(urf_device_parent_class)->finalize(object);
}

/**
 * urf_device_dispose:
 **/
static void
urf_device_dispose (GObject *object)
{
	UrfDevicePrivate *priv;

	g_return_if_fail (URF_IS_DEVICE (object));

	priv = URF_DEVICE (object)->priv;

	if (priv->proxy) {
		g_object_unref (priv->proxy);
		priv->proxy = NULL;
	}

	G_OBJECT_CLASS(urf_device_parent_class)->dispose(object);
}

/**
 * urf_device_class_init:
 * @klass: The UrfDeviceClass
 **/
static void
urf_device_class_init(UrfDeviceClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GParamSpec *pspec;

	g_type_class_add_private(klass, sizeof(UrfDevicePrivate));
	object_class->set_property = urf_device_set_property;
	object_class->get_property = urf_device_get_property;
	object_class->finalize = urf_device_finalize;
	object_class->dispose = urf_device_dispose;

	/**
	 * UrfDevice:index:
	 *
	 * The index of the rfkill device assigned by the kernel rfkill subsystem
	 *
	 * Since: 0.2.0
	 */
	pspec = g_param_spec_uint ("index",
				   "Index", "The index of the rfkill device",
				   0, G_MAXUINT, 0,
				   G_PARAM_READABLE);
	g_object_class_install_property (object_class, PROP_DEVICE_INDEX, pspec);

	/**
	 * UrfDevice:type:
	 *
	 * The type of the rfkill device. See #UrfEnumType.
	 *
	 * Since: 0.2.0
	 */
	pspec = g_param_spec_uint ("type",
				   "Type", "The type of the rfkill device",
				   0, URF_ENUM_TYPE_NUM-1, 0,
				   G_PARAM_READABLE);
	g_object_class_install_property (object_class, PROP_DEVICE_TYPE, pspec);

	/**
	 * UrfDevice:soft:
	 *
	 * This property indicates whether the soft block of the rfkill device
	 * is on or not.
	 *
	 * Since: 0.3.0
	 */
	pspec = g_param_spec_boolean ("soft",
				      "Soft Block", "If the soft block is on",
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_DEVICE_SOFT, pspec);

	/**
	 * UrfDevice:hard:
	 *
	 * This property indicates whether the hard block of the rfkill device
	 * is on or not.
	 *
	 * Since: 0.2.0
	 */
	pspec = g_param_spec_boolean ("hard",
				      "Hard Block", "If the hard block is on",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class, PROP_DEVICE_HARD, pspec);

	/**
	 * UrfDevice:name:
	 *
	 * The name of the rfkill device defined by the driver
	 *
	 * Since: 0.2.0
	 */
	pspec = g_param_spec_string ("name",
				     "Name", "The name of the rfkill device",
				     NULL, G_PARAM_READABLE);
	g_object_class_install_property (object_class, PROP_DEVICE_NAME, pspec);

	/**
	 * UrfDevice:platform:
	 *
	 * This property indicates whether the rfkill device is generated by
	 * a platform driver or not.
	 *
	 * Since: 0.3.0
	 */
	pspec = g_param_spec_boolean ("platform",
				      "Platform Driver", "If the device is a platform device",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class, PROP_DEVICE_PLATFORM, pspec);
}

/**
 * urf_device_init:
 * @client: This class instance
 **/
static void
urf_device_init (UrfDevice *device)
{
	device->priv = URF_DEVICE_GET_PRIVATE (device);
	device->priv->name = NULL;
	device->priv->object_path = NULL;
	device->priv->is_initialized = FALSE;
}

/**
 * urf_device_new:
 *
 * Creates a new #UrfDevice object.
 *
 * Return value: a new #UrfDevice object.
 *
 * Since: 0.2.0
 **/
UrfDevice *
urf_device_new (void)
{
	UrfDevice *device;
	device = URF_DEVICE (g_object_new (URF_TYPE_DEVICE, NULL));
	return device;
}

