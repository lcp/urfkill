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

#include "urf-device-kernel.h"

#include "urf-utils.h"

#define URF_DEVICE_KERNEL_INTERFACE "org.freedesktop.URfkill.Device.Kernel"

static const char introspection_xml[] =
"<node>"
"  <interface name='org.freedesktop.URfkill.Device.Kernel'>"
"    <signal name='Changed'/>"
"    <property name='index' type='i' access='read'/>"
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

#define URF_DEVICE_KERNEL_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                URF_TYPE_DEVICE_KERNEL, UrfDeviceKernelPrivate))

struct _UrfDeviceKernelPrivate {
	gint		 index;
	guint		 type;
	char		*name;
	gboolean	 soft;
	gboolean	 hard;
	gboolean	 platform;
	char		*object_path;
	GDBusConnection	*connection;
	GDBusNodeInfo	*introspection_data;
};

G_DEFINE_TYPE_WITH_PRIVATE (UrfDeviceKernel, urf_device_kernel, URF_TYPE_DEVICE)


/**
 * emit_properites_changed
 **/
static void
emit_properites_changed (UrfDeviceKernel *device)
{
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);
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
	                                              URF_DEVICE_KERNEL_INTERFACE,
	                                              builder,
	                                              NULL),
	                                &error);
	if (error) {
		g_warning ("Failed to emit PropertiesChanged: %s", error->message);
		g_error_free (error);
	}
}

/**
 * urf_device_kernel_update_states:
 *
 * Return value: #TRUE if the states of the blocks are changed,
 *               otherwise #FALSE
 **/
gboolean
urf_device_kernel_update_states (UrfDeviceKernel      *device,
                                 const gboolean  soft,
                                 const gboolean  hard)
{
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);
	GError *error = NULL;

	if (priv->soft != soft || priv->hard != hard) {
		priv->soft = soft;
		priv->hard = hard;

		g_signal_emit (G_OBJECT (device), signals[SIGNAL_CHANGED], 0);
		emit_properites_changed (device);
		g_dbus_connection_emit_signal (priv->connection,
		                               NULL,
		                               priv->object_path,
		                               URF_DEVICE_KERNEL_INTERFACE,
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
 * get_index:
 **/
static gint
get_index (UrfDevice *device)
{
	return URF_DEVICE_KERNEL_GET_PRIVATE (device)->index;
}

/**
 * get_rf_type:
 **/
static guint
get_rf_type (UrfDevice *device)
{
	return URF_DEVICE_KERNEL_GET_PRIVATE (device)->type;
}

/**
 * get_name:
 **/
static const char *
get_name (UrfDevice *device)
{
	return URF_DEVICE_KERNEL_GET_PRIVATE (device)->name;
}

/**
 * get_soft:
 **/
static gboolean
get_soft (UrfDevice *device)
{
	return URF_DEVICE_KERNEL_GET_PRIVATE (device)->soft;
}

/**
 * get_hard:
 **/
static gboolean
get_hard (UrfDevice *device)
{
	return URF_DEVICE_KERNEL_GET_PRIVATE (device)->hard;
}

/**
 * set_soft:
 **/
static void
set_soft (UrfDevice *device, gboolean blocked)
{
	UrfDeviceKernel *self = URF_DEVICE_KERNEL (device);
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (self);

	priv->soft = blocked;
}

/**
 * set_hard:
 **/
static void
set_hard (UrfDevice *device, gboolean blocked)
{
	UrfDeviceKernel *self = URF_DEVICE_KERNEL (device);
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (self);

	priv->hard = blocked;
}

/**
 * get_state:
 **/
static KillswitchState
get_state (UrfDevice *device)
{
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);

	g_return_val_if_fail (URF_IS_DEVICE_KERNEL (device), -1);

	return event_to_state (priv->soft, priv->hard);
}

/**
 * is_platform:
 */
static gboolean
is_platform (UrfDevice *device)
{
	return URF_DEVICE_KERNEL_GET_PRIVATE (device)->platform;
}

/**
 * get_property:
 **/
static void
get_property (GObject    *object,
	      guint       prop_id,
	      GValue     *value,
	      GParamSpec *pspec)
{
	UrfDeviceKernel *device = URF_DEVICE_KERNEL (object);
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);

	switch (prop_id) {
	case PROP_DEVICE_INDEX:
		g_value_set_int (value, priv->index);
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

static void
set_property (GObject *object,
              guint prop_id,
              const GValue *value,
              GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_DEVICE_INDEX:
	case PROP_DEVICE_TYPE:
	case PROP_DEVICE_NAME:
	case PROP_DEVICE_SOFT:
	case PROP_DEVICE_HARD:
	case PROP_DEVICE_PLATFORM:
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
	}
}

static GObject*
constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
	GObject *object;

	object = G_OBJECT_CLASS (urf_device_kernel_parent_class)->constructor
				 (type,
				  n_construct_params,
				  construct_params);

	if (!object)
		return NULL;

	return object;
}

/**
 * dispose:
 **/
static void
dispose (GObject *object)
{
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (object);

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

	G_OBJECT_CLASS(urf_device_kernel_parent_class)->dispose(object);
}

/**
 * urf_device_kernel_init:
 **/
static void
urf_device_kernel_init (UrfDeviceKernel *device)
{
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);

	priv->name = NULL;
	priv->platform = FALSE;
	priv->object_path = NULL;
}

/**
 * urf_device_kernel_class_init:
 **/
static void
urf_device_kernel_class_init(UrfDeviceKernelClass *class)
{
	GObjectClass *object_class = (GObjectClass *) class;
	UrfDeviceClass *parent_class = URF_DEVICE_CLASS (class);
	GParamSpec *pspec;

	object_class->constructor = constructor;
	object_class->get_property = get_property;
	object_class->set_property = set_property;
	object_class->dispose = dispose;

	parent_class->get_index = get_index;
	parent_class->get_state = get_state;
	parent_class->get_name = get_name;
	parent_class->get_device_type = get_rf_type;
	parent_class->is_hardware_blocked = get_hard;
	parent_class->set_hardware_blocked = set_hard;
	parent_class->is_software_blocked = get_soft;
	parent_class->set_software_blocked = set_soft;
	parent_class->is_platform = is_platform;

	signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0, G_TYPE_NONE);

	pspec = g_param_spec_int ("index",
			 	  "Killswitch Index",
				  "The Index of the killswitch device",
				  0, G_MAXINT, 0,
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
	UrfDeviceKernel *device = URF_DEVICE_KERNEL (user_data);
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);

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

static gboolean
handle_set_property (GDBusConnection *connection,
                     const gchar *sender,
                     const gchar *object_path,
                     const gchar *interface_name,
                     const gchar *property_name,
                     GVariant *value,
                     GError **error,
                     gpointer user_data)
{
	return TRUE;
}

static const GDBusInterfaceVTable interface_vtable =
{
	NULL, /* handle method_call */
	handle_get_property,
	handle_set_property,
};

/**
 * get_udev_attrs
 */
static void
get_udev_attrs (UrfDeviceKernel *device)
{
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);
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
 * urf_device_kernel_new:
 */
UrfDevice *
urf_device_kernel_new (gint    index,
                       guint    type,
                       gboolean soft,
                       gboolean hard)
{
	UrfDeviceKernel *device = g_object_new (URF_TYPE_DEVICE_KERNEL, NULL);
	UrfDeviceKernelPrivate *priv = URF_DEVICE_KERNEL_GET_PRIVATE (device);

	priv->index = index;
	priv->type = type;
	priv->soft = soft;
	priv->hard = hard;

	get_udev_attrs (device);

	if (!urf_device_register_device (URF_DEVICE (device), introspection_xml)) {
		g_object_unref (device);
		return NULL;
	}

	return URF_DEVICE (device);
}

