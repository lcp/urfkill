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

#include <glib.h>
#include "urf-config.h"

#define URF_CONFIG_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), \
                                     URF_TYPE_CONFIG, UrfConfigPrivate))
struct UrfConfigPrivate {
	char *user;
	char **input_devices;
};

G_DEFINE_TYPE(UrfConfig, urf_config, G_TYPE_OBJECT)

/**
 * urf_config_load_from_file:
 **/
void
urf_config_load_from_file (UrfConfig  *config,
			   const char *filename)
{
	UrfConfigPrivate *priv = URF_CONFIG_GET_PRIVATE (config);
	GKeyFile *key_file = g_key_file_new ();
	gsize *length;
	gboolean ret = FALSE;

	ret = g_key_file_load_from_file (key_file, filename, G_KEY_FILE_NONE, NULL);

	if (!ret) {
		g_warning ("Failed to load config file: %s", filename);
		return;
	}

	/* Parse the key file and store to private variables*/
	priv->user = g_key_file_get_value (key_file, "general", "user", NULL);

	priv->input_devices = g_key_file_get_keys (key_file, "input devices", length, NULL);

	g_key_file_free (key_file);
}

/**
 * urf_config_get_user:
 **/
const char *
urf_config_get_user (UrfConfig *config)
{
	UrfConfigPrivate *priv = URF_CONFIG_GET_PRIVATE (config);
	return (const char *)priv->user;
}

/**
 * urf_config_get_device_table:
 **/
const char **
urf_config_get_input_devices (UrfConfig *config)
{
	UrfConfigPrivate *priv = URF_CONFIG_GET_PRIVATE (config);
	return (const char **)priv->input_devices;
}

/**
 * urf_config_init:
 **/
static void
urf_config_init (UrfConfig *config)
{
	UrfConfigPrivate *priv = URF_CONFIG_GET_PRIVATE (config);
	priv->user = NULL;
	priv->input_devices = NULL;
}

/**
 * urf_config_finalize:
 **/
static void
urf_config_finalize (GObject *object)
{
	UrfConfigPrivate *priv = URF_CONFIG_GET_PRIVATE (object);

	g_free (priv->user);
	g_strfreev (priv->input_devices);
}

/**
 * urf_config_class_init:
 **/
static void
urf_config_class_init(UrfConfigClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;

	g_type_class_add_private(klass, sizeof(UrfConfigPrivate));
	object_class->finalize = urf_config_finalize;
}

/**
 * urf_config_new:
 **/
UrfConfig *
urf_config_new (void)
{
	UrfConfig *config;
	config = URF_CONFIG(g_object_new (URF_TYPE_CONFIG, NULL));
	return config;
}
