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

#include "urf-killswitch.h"

static void	urf_killswitch_class_init	(UrfKillswitchClass	*klass);
static void	urf_killswitch_init		(UrfKillswitch		*killswitch);
static void	urf_killswitch_finalize		(GObject		*object);

#define URF_KILLSWITCH_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), URF_TYPE_KILLSWITCH, UrfKillswitchPrivate))

/**
 * UrfKillswitchPrivate:
 *
 * Private #UrfKillswitch data
 **/
struct UrfKillswitchPrivate
{
	guint index;
	KillswitchType type;
	KillswitchState state;
	guint soft;
	guint hard;
	gchar *name;
};

enum {
	PROP_0,
	PROP_KILLSWITCH_INDEX,
	PROP_KILLSWITCH_TYPE,
	PROP_KILLSWITCH_STATE,
	PROP_KILLSWITCH_SOFT,
	PROP_KILLSWITCH_HARD,
	PROP_KILLSWITCH_NAME,
	PROP_LAST
};

G_DEFINE_TYPE (UrfKillswitch, urf_killswitch, G_TYPE_OBJECT)

/**
 * urf_killswitch_get_property:
 **/
static void
urf_killswitch_get_property (GObject *object, guint prop_id, GValue *value, GParamSpec *pspec)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (object);
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	switch (prop_id) {
	case PROP_KILLSWITCH_INDEX:
		g_value_set_uint (value, priv->index);
		break;
	case PROP_KILLSWITCH_TYPE:
		g_value_set_uint (value, priv->type);
		break;
	case PROP_KILLSWITCH_STATE:
		g_value_set_int (value, priv->state);
		break;
	case PROP_KILLSWITCH_SOFT:
		g_value_set_uint (value, priv->soft);
		break;
	case PROP_KILLSWITCH_HARD:
		g_value_set_uint (value, priv->hard);
		break;
	case PROP_KILLSWITCH_NAME:
		g_value_set_string (value, priv->name);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_killswitch_set_property:
 **/
static void
urf_killswitch_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
	UrfKillswitch *killswitch = URF_KILLSWITCH (object);
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	switch (prop_id) {
	case PROP_KILLSWITCH_INDEX:
		priv->index = g_value_get_uint (value);
		break;
	case PROP_KILLSWITCH_TYPE:
		priv->type = g_value_get_uint (value);
		break;
	case PROP_KILLSWITCH_STATE:
		priv->state = g_value_get_int (value);
		break;
	case PROP_KILLSWITCH_SOFT:
		priv->soft = g_value_get_uint (value);
		break;
	case PROP_KILLSWITCH_HARD:
		priv->hard = g_value_get_uint (value);
		break;
	case PROP_KILLSWITCH_NAME:
		g_free (priv->name);
		priv->name = g_value_dup_string (value);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

/**
 * urf_killswitch_get_rfkill_index:
 **/
guint
urf_killswitch_get_rfkill_index (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv;

	/* TODO */
	/* Choose a better return value */
	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), 0);

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	return priv->index;
}

/**
 * urf_killswitch_get_rfkill_type:
 **/
KillswitchType
urf_killswitch_get_rfkill_type (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), KILLSWITCH_TYPE_ALL);

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	return priv->type;
}

/**
 * urf_killswitch_get_rfkill_state:
 **/
KillswitchState
urf_killswitch_get_rfkill_state (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), KILLSWITCH_STATE_NO_ADAPTER);

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	return priv->state;
}

/**
 * urf_killswitch_get_rfkill_soft:
 **/
guint
urf_killswitch_get_rfkill_soft (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), 1);

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	return priv->soft;
}

/**
 * urf_killswitch_get_rfkill_hard:
 **/
guint
urf_killswitch_get_rfkill_hard (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), 1);

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	return priv->hard;
}

/**
 * urf_killswitch_get_rfkill_name:
 **/
gchar *
urf_killswitch_get_rfkill_name (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv;

	g_return_val_if_fail (URF_IS_KILLSWITCH (killswitch), NULL);

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);
	return priv->name;
}

static void
urf_killswitch_class_init(UrfKillswitchClass *klass)
{
	GObjectClass *object_class = (GObjectClass *) klass;
	GParamSpec *pspec;

	g_type_class_add_private(klass, sizeof(UrfKillswitchPrivate));
	object_class->set_property = urf_killswitch_set_property;
	object_class->get_property = urf_killswitch_get_property;
	object_class->finalize = urf_killswitch_finalize;

	/**
	 * UrfKillswitch:index:
	 */
	pspec = g_param_spec_uint ("index",
				   "Index", "The index of the killswitch",
				   0, G_MAXUINT, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_INDEX, pspec);

	/**
	 * UrfKillswitch:type:
	 */
	pspec = g_param_spec_uint ("type",
				   "Type", "The type of the killswitch",
				   0, NUM_KILLSWITCH_TYPES-1, 0,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_TYPE, pspec);

	/**
	 * UrfKillswitch:state:
	 */
	pspec = g_param_spec_int ("state",
				  "State", "The state of the killswitch",
				  KILLSWITCH_STATE_NO_ADAPTER, KILLSWITCH_STATE_HARD_BLOCKED, KILLSWITCH_STATE_NO_ADAPTER,
				  G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_STATE, pspec);

	/**
	 * UrfKillswitch:soft:
	 */
	pspec = g_param_spec_uint ("soft",
				   "Soft Block", "If the soft block is on",
				   0, 1, 1,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_SOFT, pspec);

	/**
	 * UrfKillswitch:hard:
	 */
	pspec = g_param_spec_uint ("hard",
				   "Hard Block", "If the hard block is on",
				   0, 1, 1,
				   G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_HARD, pspec);

	/**
	 * UrfKillswitch:name:
	 */
	pspec = g_param_spec_string ("name",
				     "Name", "The name of the killswitch",
				     NULL, G_PARAM_READWRITE);
	g_object_class_install_property (object_class, PROP_KILLSWITCH_NAME, pspec);
}

static void
urf_killswitch_init (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	priv->name = NULL;
}

static void
urf_killswitch_finalize (GObject *object)
{
	UrfKillswitch *killswitch;
	UrfKillswitchPrivate *priv;

	g_return_if_fail (URF_IS_KILLSWITCH (object));

	killswitch = URF_KILLSWITCH (object);
	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	g_free (priv->name);

	G_OBJECT_CLASS(urf_killswitch_parent_class)->finalize(object);
}

/**
 * urf_killswitch_new:
 *
 * Creates a new #UrfKillswitch object.
 *
 * Return value: a new UrfKillswitch object.
 *
 **/
UrfKillswitch *
urf_killswitch_new (void)
{
	UrfKillswitch *killswitch;
	killswitch = URF_KILLSWITCH (g_object_new (URF_TYPE_KILLSWITCH, NULL));
	return killswitch;
}

