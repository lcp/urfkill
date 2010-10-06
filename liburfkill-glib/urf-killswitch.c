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
	PROP_LAST
};

G_DEFINE_TYPE (UrfKillswitch, urf_killswitch, G_TYPE_OBJECT)

/**
 * urf_killswitch_setup:
 **/
void
urf_killswitch_setup (UrfKillswitch *killswitch,
		      const guint index,
		      const guint type,
		      const gint state,
		      const guint soft,
		      const guint hard,
		      const gchar *name)
{
	UrfKillswitchPrivate *priv;

	g_return_if_fail (URF_IS_KILLSWITCH (killswitch));

	priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	priv->index = index;
	priv->type  = type;
	priv->state = state;
	priv->soft  = soft;
	priv->hard  = hard;
	priv->name  = g_strdup (name);
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
	g_type_class_add_private(klass, sizeof(UrfKillswitchPrivate));
	object_class->finalize = urf_killswitch_finalize;
}

static void
urf_killswitch_init (UrfKillswitch *killswitch)
{
	UrfKillswitchPrivate *priv = URF_KILLSWITCH_GET_PRIVATE (killswitch);

	priv->index = 0;
	priv->type = KILLSWITCH_TYPE_ALL;
	priv->state = KILLSWITCH_STATE_NO_ADAPTER;
	priv->soft = 1;
	priv->hard = 1;
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

	priv->type = KILLSWITCH_TYPE_ALL;
	priv->state = KILLSWITCH_STATE_NO_ADAPTER;
	priv->soft = 1;
	priv->hard = 1;
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

