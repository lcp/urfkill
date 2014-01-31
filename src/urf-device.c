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


enum
{
	PROP_0,
	PROP_DEVICE_INDEX,
	PROP_DEVICE_TYPE,
	PROP_DEVICE_NAME,
	PROP_DEVICE_STATE,
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
	char		*object_path;
	GDBusConnection	*connection;
	GDBusNodeInfo	*introspection_data;
};

G_DEFINE_ABSTRACT_TYPE (UrfDevice, urf_device, G_TYPE_OBJECT)

/**
 * urf_device_get_index:
 **/
guint
urf_device_get_index (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), -1);

	if (URF_GET_DEVICE_CLASS (device)->get_index)
		return URF_GET_DEVICE_CLASS (device)->get_index (device);

	return -1;
}

/**
 * urf_device_get_device_type:
 **/
guint
urf_device_get_device_type (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), FALSE);

	if (URF_GET_DEVICE_CLASS (device)->get_device_type)
		return URF_GET_DEVICE_CLASS (device)->get_device_type (device);

	return TRUE;
}

/**
 * urf_device_get_name:
 **/
const char *
urf_device_get_name (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), NULL);

	if (URF_GET_DEVICE_CLASS (device)->get_name)
		return URF_GET_DEVICE_CLASS (device)->get_name (device);

	return NULL;
}

/**
 * urf_device_is_hardware_blocked:
 **/
gboolean
urf_device_is_hardware_blocked (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), FALSE);

	if (URF_GET_DEVICE_CLASS (device)->is_hardware_blocked)
		return URF_GET_DEVICE_CLASS (device)->is_hardware_blocked (device);

	return FALSE;
}

/**
 * urf_device_is_software_blocked:
 **/
gboolean
urf_device_is_software_blocked (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), FALSE);

	if (URF_GET_DEVICE_CLASS (device)->is_software_blocked)
		return URF_GET_DEVICE_CLASS (device)->is_software_blocked (device);

	return FALSE;
}

/**
 * urf_device_set_hardware_blocked:
 **/
void
urf_device_set_hardware_blocked (UrfDevice *device, gboolean blocked)
{
	g_return_if_fail (URF_IS_DEVICE (device));

	if (URF_GET_DEVICE_CLASS (device)->set_hardware_blocked)
		URF_GET_DEVICE_CLASS (device)->set_hardware_blocked (device, blocked);
}

/**
 * urf_device_set_software_blocked:
 **/
void
urf_device_set_software_blocked (UrfDevice *device, gboolean blocked)
{
	g_return_if_fail (URF_IS_DEVICE (device));

	if (URF_GET_DEVICE_CLASS (device)->set_software_blocked)
		URF_GET_DEVICE_CLASS (device)->set_software_blocked (device, blocked);
}

/**
 * urf_device_get_state:
 **/
KillswitchState
urf_device_get_state (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), -1);

	if (URF_GET_DEVICE_CLASS (device)->get_state)
		return URF_GET_DEVICE_CLASS (device)->get_state (device);

	return -1;
}

/**
 * urf_device_set_state:
 **/
void
urf_device_set_state (UrfDevice *device, KillswitchState state)
{
	g_return_if_fail (URF_IS_DEVICE (device));

	if (URF_GET_DEVICE_CLASS (device)->set_state)
		URF_GET_DEVICE_CLASS (device)->set_state (device, state);
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
	g_return_val_if_fail (URF_IS_DEVICE (device), FALSE);

	if (URF_GET_DEVICE_CLASS (device)->is_platform)
		return URF_GET_DEVICE_CLASS (device)->is_platform (device);

	return TRUE;
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

	if (urf_device_is_software_blocked (device) != soft
	    || urf_device_is_hardware_blocked (device) != hard) {
		urf_device_set_software_blocked (device, soft);
		urf_device_set_hardware_blocked (device, hard);
		urf_device_set_state (device, event_to_state (soft, hard));

		g_signal_emit (G_OBJECT (device), signals[SIGNAL_CHANGED], 0);
		//emit_properites_changed (device);
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
 * urf_device_get_property:
 **/
static void
urf_device_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	UrfDevice *device = URF_DEVICE (object);

	switch (prop_id) {
	case PROP_DEVICE_INDEX:
		g_value_set_uint (value, urf_device_get_index (device));
		break;
	case PROP_DEVICE_TYPE:
		g_value_set_uint (value, urf_device_get_device_type (device));
		break;
	case PROP_DEVICE_NAME:
		g_value_set_string (value, urf_device_get_name (device));
		break;
	case PROP_DEVICE_PLATFORM:
		g_value_set_boolean (value, urf_device_is_platform (device));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static GObject*
constructor (GType type,
             guint n_construct_params,
             GObjectConstructParam *construct_params)
{
	GObject *object;
	UrfDevice *self;
	UrfDevicePrivate *priv;

	object = G_OBJECT_CLASS (urf_device_parent_class)->constructor (type,
	                         n_construct_params,
	                         construct_params);

	if (!object)
		return NULL;

	self = URF_DEVICE (object);
	priv = self->priv;

	g_warning ("(%s) device constructor", priv->object_path);

	return object;
}

static void
constructed (GObject *object)
{
	if (G_OBJECT_CLASS (urf_device_parent_class)->constructed)
		G_OBJECT_CLASS (urf_device_parent_class)->constructed (object);
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

	if (priv->object_path)
		g_free (priv->object_path);

	G_OBJECT_CLASS(urf_device_parent_class)->finalize(object);
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
	object_class->constructor = constructor;
	object_class->constructed = constructed;

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

	GVariant *retval = NULL;

	if (g_strcmp0 (property_name, "index") == 0)
		retval = g_variant_new_uint32 (urf_device_get_index (device));
	else if (g_strcmp0 (property_name, "type") == 0)
		retval = g_variant_new_uint32 (urf_device_get_device_type (device));
	else if (g_strcmp0 (property_name, "name") == 0)
		retval = g_variant_new_string (urf_device_get_name (device));
	else if (g_strcmp0 (property_name, "platform") == 0)
		retval = g_variant_new_boolean (urf_device_is_platform (device));

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
	static int index = 0;

	return g_strdup_printf (path_template, index++);
}

/**
 * urf_device_register_device:
 **/
gboolean
urf_device_register_device (UrfDevice *device, const char *introspection_xml)
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
 * urf_device_init:
 **/
static void
urf_device_init (UrfDevice *device)
{
	device->priv = URF_DEVICE_GET_PRIVATE (device);
	device->priv->object_path = NULL;

	const char introspection_xml[] =
		"<node>"
		"  <interface name='org.freedesktop.URfkill.Device'>"
		"    <signal name='Changed'/>"
		"    <property name='index' type='u' access='read'/>"
		"    <property name='type' type='u' access='read'/>"
		"    <property name='name' type='s' access='read'/>"
		"    <property name='state' type='u' access='read'/>"
		"  </interface>"
		"</node>";
	urf_device_register_device (device, introspection_xml);
}

