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

#ifndef __URF_KILLSWITCH_H
#define __URF_KILLSWITCH_H

#include <glib-object.h>
#include <gio/gio.h>

typedef enum {
        RFKILL_TYPE_ALL = 0,
        RFKILL_TYPE_WLAN,
        RFKILL_TYPE_BLUETOOTH,
        RFKILL_TYPE_UWB,
        RFKILL_TYPE_WIMAX,
        RFKILL_TYPE_WWAN,
        RFKILL_TYPE_GPS,
        RFKILL_TYPE_FM,
        NUM_RFKILL_TYPES,
} KillswitchType;

typedef enum {
        KILLSWITCH_STATE_NO_ADAPTER = -1,
        KILLSWITCH_STATE_SOFT_BLOCKED = 0,
        KILLSWITCH_STATE_UNBLOCKED,
        KILLSWITCH_STATE_HARD_BLOCKED
} KillswitchState;

G_BEGIN_DECLS

#define URF_TYPE_KILLSWITCH		(urf_killswitch_get_type ())
#define URF_KILLSWITCH(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_KILLSWITCH, UrfKillswitch))
#define URF_KILLSWITCH_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), URF_TYPE_KILLSWITCH, UrfKillswitchClass))
#define URF_IS_KILLSWITCH(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_KILLSWITCH))
#define URF_IS_KILLSWITCH_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), URF_TYPE_KILLSWITCH))
#define URF_KILLSWITCH_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), URF_TYPE_KILLSWITCH, UrfKillswitchClass))
#define URF_KILLSWITCH_ERROR		(urf_killswitch_error_quark ())
#define URF_KILLSWITCH_TYPE_ERROR	(urf_killswitch_error_get_type ())

typedef struct UrfKillswitchPrivate UrfKillswitchPrivate;

typedef struct
{
	 GObject		 parent;
	 UrfKillswitchPrivate	*priv;
} UrfKillswitch;

typedef struct
{
	GObjectClass		 parent_class;
} UrfKillswitchClass;

/* general */
GType			 urf_killswitch_get_type		(void);
UrfKillswitch		*urf_killswitch_new			(void);

void			 urf_killswitch_setup			(UrfKillswitch	*killswitch,
								 const guint	 index,
								 const guint	 type,
								 const gint	 state,
								 const guint	 soft,
								 const guint	 hard,
								 const gchar	*name);
guint			 urf_killswitch_get_rfkill_index	(UrfKillswitch	*killswitch);
KillswitchType		 urf_killswitch_get_rfkill_type		(UrfKillswitch	*killswitch);
KillswitchState		 urf_killswitch_get_rfkill_state	(UrfKillswitch	*killswitch);
guint			 urf_killswitch_get_rfkill_soft		(UrfKillswitch	*killswitch);
guint			 urf_killswitch_get_rfkill_hard		(UrfKillswitch	*killswitch);
gchar			*urf_killswitch_get_rfkill_name		(UrfKillswitch	*killswitch);

G_END_DECLS

#endif /* __URF_KILLSWITCH_H */
