/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Mathieu Trudel-Lapierre <mathieu-tl@ubuntu.com>
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
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <gio/gio.h>
#include <glib/gi18n-lib.h>

#include <linux/rfkill.h>

#include "urf-device-ofono.h"

#include "urf-utils.h"

#define URF_DEVICE_OFONO_INTERFACE "org.freedesktop.URfkill.Device.Ofono"

static const char introspection_xml[] =
"  <interface name='org.freedesktop.URfkill.Device.Ofono'>"
"    <signal name='Changed'/>"
"    <property name='soft' type='b' access='read'/>"
"  </interface>";

enum
{
	PROP_0,
	PROP_SOFT,
	PROP_LAST
};

enum {
	SIGNAL_CHANGED,
	LAST_SIGNAL
};

static int signals[LAST_SIGNAL] = { 0 };

#define URF_DEVICE_OFONO_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                           URF_TYPE_DEVICE_OFONO, UrfDeviceOfonoPrivate))

struct _UrfDeviceOfonoPrivate {
	gint index;
	char *object_path;
	char *name;

	GHashTable *properties;
	gboolean soft;

	GDBusProxy *proxy;
	GCancellable *cancellable;
};

G_DEFINE_TYPE_WITH_PRIVATE (UrfDeviceOfono, urf_device_ofono, URF_TYPE_DEVICE)


/**
 * urf_device_ofono_update_states:
 *
 * Return value: #TRUE if the states of the blocks are changed,
 *               otherwise #FALSE
 **/
gboolean
urf_device_ofono_update_states (UrfDeviceOfono      *device,
                               const gboolean  soft,
                               const gboolean  hard)
{

	return TRUE;
}

gchar *
urf_device_ofono_get_path (UrfDeviceOfono *ofono)
{
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (ofono);

	g_return_val_if_fail (URF_IS_DEVICE_OFONO (ofono), NULL);

	return g_strdup(priv->object_path);
}

/**
 * get_index:
 **/
static gint
get_index (UrfDevice *device)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (device);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);

	return priv->index;
}

/**
 * get_rf_type:
 **/
static gint
get_rf_type (UrfDevice *device)
{
	/* oFono devices are modems so always of the WWAN type */
	return RFKILL_TYPE_WWAN;
}

static const char *
get_urf_type (UrfDevice *device)
{
	return URF_DEVICE_OFONO_INTERFACE;
}

/**
 * get_name:
 **/
static const char *
get_name (UrfDevice *device)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (device);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);
	GVariant *manufacturer = NULL;
	GVariant *model = NULL;

	if (priv->name)
		g_free (priv->name);

	if (!priv->proxy) {
		g_debug( "have no proxy");
	}

	manufacturer = g_hash_table_lookup (priv->properties, "Manufacturer");
	model = g_hash_table_lookup (priv->properties, "Model");

	priv->name = g_strjoin (" ",
	                        manufacturer
                                    ? g_variant_get_string (manufacturer, NULL)
	                            : _("unknown"),
	                        model
	                            ? g_variant_get_string (model, NULL)
	                            : _("unknown"),
	                        NULL);

	g_debug ("%s: new name: '%s'", __func__, priv->name);

	return priv->name;
}

/**
 * get_soft:
 **/
static gboolean
get_soft (UrfDevice *device)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (device);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);
	GVariant *online;
	gboolean soft = FALSE;

	online = g_hash_table_lookup (priv->properties, "Online");
	if (online)
		soft = !g_variant_get_boolean (online);

	return soft;
}

static void
set_online_cb (GObject *source_object,
               GAsyncResult *res,
               gpointer user_data)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (user_data);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);
	GVariant *result;
	GError *error = NULL;

	result = g_dbus_proxy_call_finish (priv->proxy, res, &error);

	if (!error) {
		g_debug ("online change successful: %s",
		         g_variant_print (result, TRUE));
	} else {
		g_warning ("Could not set Online property in oFono: %s",
		           error ? error->message : "(unknown error)");
	}
}

/**
 * set_soft:
 **/
static gboolean
set_soft (UrfDevice *device, gboolean blocked)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (device);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);

	priv->soft = blocked;
	g_dbus_proxy_call (priv->proxy,
	                   "SetProperty",
	                   g_variant_new ("(sv)",
	                                  "Online",
                                          g_variant_new_boolean (!blocked)),
	                   G_DBUS_CALL_FLAGS_NONE,
	                   -1,
	                   priv->cancellable,
	                   (GAsyncReadyCallback) set_online_cb,
	                   modem);

	/* always succeeds since it's an async call */
	return TRUE;
}

/**
 * get_state:
 **/
static KillswitchState
get_state (UrfDevice *device)
{
	KillswitchState state = KILLSWITCH_STATE_UNBLOCKED;
	gboolean soft;

	soft = get_soft (device);
	if (soft)
		state = KILLSWITCH_STATE_SOFT_BLOCKED;

	return state;
}

static void
modem_signal_cb (GDBusProxy *proxy,
                 gchar *sender_name,
                 gchar *signal_name,
                 GVariant *parameters,
                 gpointer user_data)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (user_data);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);

	if (g_strcmp0 (signal_name, "PropertyChanged") == 0) {
		gchar *prop_name;
		GVariant *prop_value = NULL;

		g_debug ("properties changed for %s: %s",
		         priv->object_path,
		         g_variant_print (parameters, TRUE));

		g_variant_get_child (parameters, 0, "s", &prop_name);
		g_variant_get_child (parameters, 1, "v", &prop_value);

		if (prop_value)
			g_hash_table_replace (priv->properties,
			                      g_strdup (prop_name),
			                      g_variant_ref (prop_value));

		if (g_strcmp0 ("Powered", prop_name) == 0) {
			gboolean powered = FALSE;

			powered = g_variant_get_boolean (prop_value);

			if (powered)
				set_soft (URF_DEVICE (modem), priv->soft);
		}

		g_free (prop_name);
		g_variant_unref (prop_value);
	}
}

static void
get_properties_cb (GObject *source_object,
                   GAsyncResult *res,
                   gpointer user_data)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (user_data);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);
	GVariant *result, *properties, *variant = NULL;
	GVariantIter iter;
	GError *error = NULL;
	gchar *key;

	result = g_dbus_proxy_call_finish (priv->proxy, res, &error);

	if (!error) {
		properties = g_variant_get_child_value (result, 0);
		g_debug ("%zd properties for %s", g_variant_n_children (properties), priv->object_path);
		g_debug ("%s", g_variant_print (properties, TRUE));

		g_variant_iter_init (&iter, properties);
		while (g_variant_iter_next (&iter, "{sv}", &key, &variant)) {
			if (g_strcmp0 ("Powered", key) == 0 &&
			    g_variant_get_boolean (variant) == TRUE) {
				set_soft (URF_DEVICE (modem), priv->soft);
			}

			g_hash_table_insert (priv->properties, g_strdup (key),
			                     g_variant_ref (variant));
			g_variant_unref (variant);
                        g_free (key);
                }

		g_variant_unref (properties);
		g_variant_unref (result);
	} else {
		g_warning ("Error getting properties: %s",
		           error ? error->message : "(unknown error)");
	}
}

static void
proxy_ready_cb (GObject *source_object,
                GAsyncResult *res,
                gpointer user_data)
{
	UrfDeviceOfono *modem = URF_DEVICE_OFONO (user_data);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (modem);
	GError *error = NULL;

	priv->proxy = g_dbus_proxy_new_finish (res, &error);

	if (!error) {
		g_cancellable_reset (priv->cancellable);
		g_signal_connect (priv->proxy, "g-signal",
		                  G_CALLBACK (modem_signal_cb), modem);
		g_dbus_proxy_call (priv->proxy,
		                   "GetProperties",
		                   NULL,
		                   G_DBUS_CALL_FLAGS_NONE,
		                   -1,
		                   priv->cancellable,
		                   (GAsyncReadyCallback) get_properties_cb,
		                   modem);
	} else {
		g_warning ("Could not get oFono Modem proxy: %s",
		           error ? error->message : "(unknown error)");
	}
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
	UrfDeviceOfono *device = URF_DEVICE_OFONO (object);

	switch (prop_id) {
	case PROP_SOFT:
		g_value_set_boolean (value, get_soft (URF_DEVICE (device)));
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

	object = G_OBJECT_CLASS (urf_device_ofono_parent_class)->constructor
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
	G_OBJECT_CLASS(urf_device_ofono_parent_class)->dispose(object);
}

/**
 * urf_device_ofono_init:
 **/
static void
urf_device_ofono_init (UrfDeviceOfono *device)
{
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (device);

	priv->cancellable = g_cancellable_new ();
	priv->properties = g_hash_table_new_full (g_str_hash, g_str_equal,
	                                          g_free, (GDestroyNotify) g_variant_unref);
	priv->soft = FALSE;
}

/**
 * urf_device_ofono_class_init:
 **/
static void
urf_device_ofono_class_init(UrfDeviceOfonoClass *class)
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
	parent_class->get_urf_type = get_urf_type;
	parent_class->get_device_type = get_rf_type;
	parent_class->is_software_blocked = get_soft;
	parent_class->set_software_blocked = set_soft;

	signals[SIGNAL_CHANGED] =
		g_signal_new ("changed",
			      G_OBJECT_CLASS_TYPE (class),
			      G_SIGNAL_RUN_LAST | G_SIGNAL_DETAILED,
			      0, NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0, G_TYPE_NONE);

	pspec = g_param_spec_boolean ("soft",
				      "Soft Block",
				      "The soft block of the device",
				      FALSE,
				      G_PARAM_READABLE);
	g_object_class_install_property (object_class,
					 PROP_SOFT,
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
	UrfDeviceOfono *device = URF_DEVICE_OFONO (user_data);

	GVariant *retval = NULL;

	if (g_strcmp0 (property_name, "soft") == 0)
		retval = g_variant_new_boolean (get_soft (URF_DEVICE (device)));

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
 * urf_device_ofono_new:
 */
UrfDevice *
urf_device_ofono_new (gint index, const char *object_path)
{
	UrfDeviceOfono *device = g_object_new (URF_TYPE_DEVICE_OFONO, NULL);
	UrfDeviceOfonoPrivate *priv = URF_DEVICE_OFONO_GET_PRIVATE (device);

	priv->index = index;
	priv->object_path = g_strdup (object_path);

	g_debug ("new ofono device: %p for %s", device, priv->object_path);

        if (!urf_device_register_device (URF_DEVICE (device), interface_vtable, introspection_xml)) {
                g_object_unref (device);
                return NULL;
        }

	g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
	                          G_DBUS_PROXY_FLAGS_NONE,
	                          NULL,
	                          "org.ofono",
	                          priv->object_path,
	                          "org.ofono.Modem",
	                          priv->cancellable,
	                          proxy_ready_cb,
	                          device);

	return URF_DEVICE (device);
}

