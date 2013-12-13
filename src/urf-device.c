/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@suse.com>
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
#include <config.h>
#endif

#include <stdlib.h>
#include <glib.h>
#include <linux/rfkill.h>
#include <gio/gio.h>
#include <libudev.h>

#include "urf-device.h"

#include "urf-utils.h"

#define URF_DEVICE_INTERFACE "org.freedesktop.URfkill.Device"

static const char introspection_xml[] =
"<node>"
"  <interface name='org.freedesktop.URfkill.Device'>"
"    <signal name='Changed'/>"
"    <property name='index' type='u' access='read'/>"
"    <property name='type' type='u' access='read'/>"
"    <property name='name' type='s' access='read'/>"
"    <property name='soft' type='b' access='read'/>"
"    <property name='hard' type='b' access='read'/>"
"    <property name='platform' type='b' access='read'/>"
"  </interface>"
"</node>";

enum
{
	PROP_0,
	PROP_DEVICE_INDEX,
	PROP_DEVICE_TYPE,
	PROP_DEVICE_NAME,
	PROP_DEVICE_SOFT,
	PROP_DEVICE_HARD,
	PROP_DEVICE_PLATFORM,
	PROP_LAST
};

enum {
	SIGNAL_CHANGED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

#define URF_DEVICE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                URF_TYPE_DEVICE, UrfDevicePrivate))

struct UrfDevicePrivate {
	guint		 index;
	guint		 type;
	char		*name;
	gboolean	 soft;
	gboolean	 hard;
	gboolean	 platform;
	KillswitchState	 state;
	char		*object_path;
	GDBusConnection	*connection;
	GDBusNodeInfo	*introspection_data;
};

G_DEFINE_TYPE(UrfDevice, urf_device, G_TYPE_OBJECT)

/**
 * emit_properites_changed
 **/
static void
emit_properites_changed (UrfDevice *device)
{
	UrfDevicePrivate *priv = device->priv;
	GVariantBuilder *builder;
	GError *error = NULL;

	builder = g_variant_builder_new (G_VARIANT_TYPE_ARRAY);
	g_variant_builder_add (builder,
	                       "{sv}",
	                       "soft",
	                       g_variant_new_boolean (priv->soft));
	g_variant_builder_add (builder,
	                       "{sv}",
	                       "hard",
	                       g_variant_new_boolean (priv->hard));

	g_dbus_connection_emit_signal (priv->connection,
	                               NULL,
	                               priv->object_path,
	                               "org.freedesktop.DBus.Properties",
	                               "PropertiesChanged",
	                               g_variant_new ("(sa{sv}as)",
	                                              URF_DEVICE_INTERFACE,
	                                              builder,
	                                              NULL),
	                                &error);
	if (error) {
		g_warning ("Failed to emit PropertiesChanged: %s", error->message);
		g_error_free (error);
	}
}

/**
 * urf_device_update_states:
 *
 * Return value: #TRUE if the states of the blocks are changed,
 *               otherwise #FALSE
 **/
gboolean
urf_device_update_states (UrfDevice      *device,
			  const gboolean  soft,
			  const gboolean  hard)
{
	UrfDevicePrivate *priv = device->priv;
	GError *error = NULL;

	if (priv->soft != soft || priv->hard != hard) {
		priv->soft = soft;
		priv->hard = hard;
		priv->state = event_to_state (priv->soft, priv->hard);

		g_signal_emit (G_OBJECT (device), signals[SIGNAL_CHANGED], 0);
		emit_properites_changed (device);
		g_dbus_connection_emit_signal (priv->connection,
		                               NULL,
		                               priv->object_path,
		                               URF_DEVICE_INTERFACE,
		                               "Changed",
		                               NULL,
		                               &error);
		if (error) {
			g_warning ("Failed to emit Changed: %s", error->message);
			g_error_free (error);
			return FALSE;
		}

		return TRUE;
	}

	return FALSE;
}

/**
 * urf_device_get_index:
 **/
guint
urf_device_get_index (UrfDevice *device)
{
	return device->priv->index;
}

/**
 * urf_device_get_rf_type:
 **/
guint
urf_device_get_rf_type (UrfDevice *device)
{
	return device->priv->type;
}

/**
 * urf_device_get_name:
 **/
const char *
urf_device_get_name (UrfDevice *device)
{
	return device->priv->name;
}

/**
 * urf_device_get_soft:
 **/
gboolean
urf_device_get_soft (UrfDevice *device)
{
	return device->priv->soft;
}

/**
 * urf_device_get_hard:
 **/
gboolean
urf_device_get_hard (UrfDevice *device)
{
	return device->priv->hard;
}

/**
 * urf_device_get_state:
 **/
KillswitchState
urf_device_get_state (UrfDevice *device)
{
	return device->priv->state;
}

/**
 * urf_device_get_object_path:
 **/
const char *
urf_device_get_object_path (UrfDevice *device)
{
	return device->priv->object_path;
}

/**
 * urf_device_is_platform:
 */
gboolean
urf_device_is_platform (UrfDevice *device)
{
	return device->priv->platform;
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
 * urf_device_dispose:
 **/
static void
urf_device_dispose (GObject *object)
{
	UrfDevicePrivate *priv = URF_DEVICE_GET_PRIVATE (object);

	if (priv->introspection_data) {
		g_dbus_node_info_unref (priv->introspection_data);
		priv->introspection_data = NULL;
	}

	if (priv->connection) {
		g_object_unref (priv->connection);
		priv->connection = NULL;
	}

	if (priv->introspection_data) {
		g_dbus_node_info_unref (priv->introspection_data);
		priv->introspection_data = NULL;
	}

	G_OBJECT_CLASS(urf_device_parent_class)->dispose(object);
}

/**
 * urf_device_finalize:
 **/
static void
urf_device_finalize (GObject *object)
{
	UrfDevicePrivate *priv = URF_DEVICE_GET_PRIVATE (object);

	if (priv->name)
		g_free (priv->name);
	if (priv->object_path)
		g_free (priv->object_path);

	G_OBJECT_CLASS(urf_device_parent_class)->finalize(object);
}

/**
 * urf_device_init:
 **/
static void
urf_device_init (UrfDevice *device)
{
	device->priv = URF_DEVICE_GET_PRIVATE (device);
	device->priv->name = NULL;
	device->priv->platform = FALSE;
	device->priv->state = KILLSWITCH_STATE_NO_ADAPTER;
	device->priv->object_path = NULL;
}

/**
 * urf_device_class_init:
 **/
static void
urf_device_class_init(UrfDeviceClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GParamSpec *pspec;

	g_type_class_add_private(klass, sizeof(UrfDevicePrivate));
	object_class->get_property = urf_device_get_property;
	object_class->dispose = urf_device_dispose;
	object_class->finalize = urf_device_finalize;

	signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (klass),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0, G_TYPE_NONE);

	pspec = g_param_spec_uint ("index",
				   "Killswitch Index",
				   "The Index of the killswitch device",
				   0, G_MAXUINT, 0,
				   G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_INDEX,
					 pspec);

	pspec = g_param_spec_uint ("type",
				   "Killswitch Type",
				   "The type of the killswitch device",
				   0, NUM_RFKILL_TYPES, 0,
				   G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_TYPE,
					 pspec);

	pspec = g_param_spec_boolean ("soft",
				      "Killswitch Soft Block",
				      "The soft block of the killswitch device",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_SOFT,
					 pspec);

	pspec = g_param_spec_boolean ("hard",
				      "Killswitch Hard Block",
				      "The hard block of the killswitch device",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_HARD,
					 pspec);

	pspec = g_param_spec_string ("name",
				     "Killswitch Name",
				     "The name of the killswitch device",
				     NULL,
				     G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_NAME,
					 pspec);

	pspec = g_param_spec_boolean ("platform",
				      "Platform Driver",
				      "Whether the device is generated by a platform driver or not",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_PLATFORM,
					 pspec);
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
	UrfDevice *device = URF_DEVICE (user_data);
	UrfDevicePrivate *priv = device->priv;

	GVariant *retval = NULL;

	if (g_strcmp0 (property_name, "index") == 0)
		retval = g_variant_new_uint32 (priv->index);
	else if (g_strcmp0 (property_name, "type") == 0)
		retval = g_variant_new_uint32 (priv->type);
	else if (g_strcmp0 (property_name, "soft") == 0)
		retval = g_variant_new_boolean (priv->soft);
	else if (g_strcmp0 (property_name, "hard") == 0)
		retval = g_variant_new_boolean (priv->hard);
	else if (g_strcmp0 (property_name, "name") == 0)
		retval = g_variant_new_string (priv->name);
	else if (g_strcmp0 (property_name, "platform") == 0)
		retval = g_variant_new_boolean (priv->platform);

	return retval;
}

static const GDBusInterfaceVTable interface_vtable =
{
	NULL, /* handle method_call */
	handle_get_property,
	NULL, /* handle set_property */
};

/**
 * urf_device_compute_object_path:
 **/
static char *
urf_device_compute_object_path (UrfDevice *device)
{
	const char *path_template = "/org/freedesktop/URfkill/devices/%u";
	return g_strdup_printf (path_template, device->priv->index);
}

/**
 * urf_device_register_device:
 **/
static gboolean
urf_device_register_device (UrfDevice *device)
{
	UrfDevicePrivate *priv = device->priv;
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

	priv->object_path = urf_device_compute_object_path (device);
	infos = priv->introspection_data->interfaces;
	reg_id = g_dbus_connection_register_object (priv->connection,
		                                    priv->object_path,
		                                    infos[0],
		                                    &interface_vtable,
		                                    device,
		                                    NULL,
		                                    NULL);
	g_assert (reg_id > 0);

	return TRUE;
}

/**
 * urf_device_get_udev_attrs
 */
static void
urf_device_get_udev_attrs (UrfDevice *device)
{
	UrfDevicePrivate *priv = device->priv;
	struct udev *udev;
	struct udev_device *dev;
	struct udev_device *parent_dev;

	udev = udev_new ();
	if (udev == NULL) {
		g_warning ("udev_new() failed");
		return;
	}
	dev = get_rfkill_device_by_index (udev, priv->index);
	if (!dev) {
		g_warning ("Failed to get udev device for index %u", priv->index);
		udev_unref (udev);
		return;
	}

	priv->name = g_strdup (udev_device_get_sysattr_value (dev, "name"));

	parent_dev = udev_device_get_parent_with_subsystem_devtype (dev, "platform", NULL);
	if (parent_dev)
		priv->platform = TRUE;

	udev_device_unref (dev);
	udev_unref (udev);
}

/**
 * urf_device_new:
 */
UrfDevice *
urf_device_new (guint    index,
		guint    type,
		gboolean soft,
		gboolean hard)
{
	UrfDevice *device = URF_DEVICE(g_object_new (URF_TYPE_DEVICE, NULL));
	UrfDevicePrivate *priv = device->priv;

	priv->index = index;
	priv->type = type;
	priv->soft = soft;
	priv->hard = hard;
	priv->state = event_to_state (soft, hard);

	urf_device_get_udev_attrs (device);

	if (!urf_device_register_device (device)) {
		g_object_unref (device);
		return NULL;
	}

	return device;
}
