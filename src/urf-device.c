/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@novell.com>
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

#include <linux/rfkill.h>

#include "urf-device.h"
#include "urf-utils.h"

enum
{
	PROP_0,
	PROP_DEVICE_INDEX,
	PROP_DEVICE_TYPE,
	PROP_DEVICE_NAME,
	PROP_DEVICE_SOFT,
	PROP_DEVICE_HARD,
	PROP_LAST
};

#define URF_DEVICE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                URF_TYPE_DEVICE, UrfDevicePrivate))

struct UrfDevicePrivate {
	guint index;
	guint type;
	char *name;
	gboolean soft;
	gboolean hard;
};

G_DEFINE_TYPE(UrfDevice, urf_device, G_TYPE_OBJECT)

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
		if (priv->name)
			g_free (priv->name);
		priv->name = get_rfkill_name_by_index (priv->index);
		break;
	case PROP_DEVICE_TYPE:
		priv->type = g_value_get_uint (value);
		break;
	case PROP_DEVICE_SOFT:
		priv->soft = g_value_get_boolean (value);
		break;
	case PROP_DEVICE_HARD:
		priv->hard = g_value_get_boolean (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_device_get_index
 **/
guint
urf_device_get_index (UrfDevice *device)
{
	return device->priv->index;
}

/**
 * urf_device_get_rf_type
 **/
guint
urf_device_get_rf_type (UrfDevice *device)
{
	return device->priv->type;
}

/**
 * urf_device_get_name
 **/
const char *
urf_device_get_name (UrfDevice *device)
{
	return device->priv->name;
}

/**
 * urf_device_get_soft
 **/
gboolean
urf_device_get_soft (UrfDevice *device)
{
	return device->priv->soft;
}

/**
 * urf_device_get_hard
 **/
gboolean
urf_device_get_hard (UrfDevice *device)
{
	return device->priv->hard;
}

/**
 * urf_device_finalize
 **/
static void
urf_device_finalize (GObject *object)
{
	UrfDevicePrivate *priv = URF_DEVICE_GET_PRIVATE (object);

	if (priv->name)
		g_free (priv->name);

	G_OBJECT_CLASS(urf_device_parent_class)->finalize(object);
}

/**
 * urf_device_init:
 **/
static void
urf_device_init (UrfDevice *device)
{
	UrfDevicePrivate *priv = URF_DEVICE_GET_PRIVATE (device);

	device->priv = priv;
	priv->name = NULL;
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
	object_class->finalize = urf_device_finalize;

	pspec = g_param_spec_uint ("index",
				   "Killswitch Index",
				   "The Index of the killswitch device",
				   0, G_MAXUINT32-1, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_INDEX,
					 pspec);

	pspec = g_param_spec_uint ("type",
				   "Killswitch Type",
				   "The type of the killswitch device",
				   0, NUM_RFKILL_TYPES, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_TYPE,
					 pspec);

	pspec = g_param_spec_boolean ("soft",
				      "Killswitch Soft Block",
				      "The soft block of the killswitch device",
				      FALSE,
				      G_PARAM_READWRITE);
	g_object_class_install_property (object_class,
					 PROP_DEVICE_SOFT,
					 pspec);

	pspec = g_param_spec_boolean ("hard",
				      "Killswitch Hard Block",
				      "The hard block of the killswitch device",
				      FALSE,
				      G_PARAM_READWRITE);
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
}

/**
 * urf_device_new
 */
UrfDevice *
urf_device_new (guint index,
		guint type,
		gboolean soft,
		gboolean hard)
{
	UrfDevice *device = URF_DEVICE(g_object_new (URF_TYPE_DEVICE, NULL));
	device->priv->index = index;
	device->priv->type = type;
	device->priv->soft = soft;
	device->priv->hard = hard;
	device->priv->name = get_rfkill_name_by_index (index);

	return device;
}
