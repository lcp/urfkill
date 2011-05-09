/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@novell.com>
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

#include "urf-device.h"

#define URF_DEVICE_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), \
					URF_TYPE_DEVICE, UrfDevicePrivate))

/**
 * UrfDevicePrivate:
 *
 * Private #UrfDevice data
 **/
struct UrfDevicePrivate
{
	guint index;
	guint type;
	guint soft;
	guint hard;
	char *name;
	char *object_path;
};

enum {
	PROP_0,
	PROP_DEVICE_INDEX,
	PROP_DEVICE_TYPE,
	PROP_DEVICE_SOFT,
	PROP_DEVICE_HARD,
	PROP_DEVICE_NAME,
	PROP_LAST
};

G_DEFINE_TYPE (UrfDevice, urf_device, G_TYPE_OBJECT)

/**
 * urf_device_get_rfkill_index:
 **/
guint
urf_device_get_rfkill_index (UrfDevice *device)
{
	/* TODO */
	/* Choose a better return value */
	g_return_val_if_fail (URF_IS_DEVICE (device), 0);

	return device->priv->index;
}

/**
 * urf_device_get_rfkill_type:
 **/
guint
urf_device_get_rfkill_type (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), NUM_RFKILL_TYPES);

	return device->priv->type;
}

/**
 * urf_device_get_rfkill_soft:
 **/
guint
urf_device_get_rfkill_soft (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), 1);

	return device->priv->soft;
}

/**
 * urf_device_get_rfkill_hard:
 **/
guint
urf_device_get_rfkill_hard (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), 1);

	return device->priv->hard;
}

/**
 * urf_device_get_rfkill_name:
 **/
gchar *
urf_device_get_rfkill_name (UrfDevice *device)
{
	g_return_val_if_fail (URF_IS_DEVICE (device), NULL);

	return device->priv->name;
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
		g_value_set_uint (value, priv->soft);
		break;
	case PROP_DEVICE_HARD:
		g_value_set_uint (value, priv->hard);
		break;
	case PROP_DEVICE_NAME:
		g_value_set_string (value, priv->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
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
	UrfDevicePrivate *priv = device->priv;

	switch (prop_id) {
	case PROP_DEVICE_INDEX:
		priv->index = g_value_get_uint (value);
		break;
	case PROP_DEVICE_TYPE:
		priv->type = g_value_get_uint (value);
		break;
	case PROP_DEVICE_SOFT:
		priv->soft = g_value_get_uint (value);
		break;
	case PROP_DEVICE_HARD:
		priv->hard = g_value_get_uint (value);
		break;
	case PROP_DEVICE_NAME:
		g_free (priv->name);
		priv->name = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

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

static void
urf_device_class_init(UrfDeviceClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GParamSpec *pspec;

	g_type_class_add_private(klass, sizeof(UrfDevicePrivate));
	object_class->set_property = urf_device_set_property;
	object_class->get_property = urf_device_get_property;
	object_class->finalize = urf_device_finalize;

	/**
	 * UrfDevice:index:
	 */
	pspec = g_param_spec_uint ("index",
				   "Index", "The index of the killswitch",
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_DEVICE_INDEX, pspec);

	/**
	 * UrfDevice:type:
	 */
	pspec = g_param_spec_uint ("type",
				   "Type", "The type of the killswitch",
				   0, NUM_RFKILL_TYPES-1, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_DEVICE_TYPE, pspec);


	/**
	 * UrfDevice:soft:
	 */
	pspec = g_param_spec_uint ("soft",
				   "Soft Block", "If the soft block is on",
				   0, 1, 1,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_DEVICE_SOFT, pspec);

	/**
	 * UrfDevice:hard:
	 */
	pspec = g_param_spec_uint ("hard",
				   "Hard Block", "If the hard block is on",
				   0, 1, 1,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_DEVICE_HARD, pspec);

	/**
	 * UrfDevice:name:
	 */
	pspec = g_param_spec_string ("name",
				     "Name", "The name of the killswitch",
				     NULL, G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_DEVICE_NAME, pspec);
}

static void
urf_device_init (UrfDevice *device)
{
	device->priv = URF_DEVICE_GET_PRIVATE (device);
	device->priv->name = NULL;
	device->priv->object_path = NULL;
}

/**
 * urf_device_new:
 *
 * Creates a new #UrfDevice object.
 *
 * Return value: a new UrfDevice object.
 *
 **/
UrfDevice *
urf_device_new (const char *object_path)
{
	UrfDevice *device;
	device = URF_DEVICE (g_object_new (URF_TYPE_DEVICE, NULL));
	device->priv->object_path = g_strdup (object_path);

	/* TODO get the dbus object properties */

	return device;
}

