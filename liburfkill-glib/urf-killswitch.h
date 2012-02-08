/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@suse.com>
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

#if !defined (__URFKILL_H_INSIDE__) && !defined (URF_COMPILATION)
#error "Only <urfkill.h> can be included directly."
#endif

#ifndef __URF_KILLSWITCH_H
#define __URF_KILLSWITCH_H

#include <glib-object.h>
#include <gio/gio.h>

#include "urf-enum.h"

G_BEGIN_DECLS

#define URF_TYPE_KILLSWITCH		(urf_killswitch_get_type ())
#define URF_KILLSWITCH(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_KILLSWITCH, UrfKillswitch))
#define URF_KILLSWITCH_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), URF_TYPE_KILLSWITCH, UrfKillswitchClass))
#define URF_IS_KILLSWITCH(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_KILLSWITCH))
#define URF_IS_KILLSWITCH_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), URF_TYPE_KILLSWITCH))
#define URF_KILLSWITCH_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), URF_TYPE_KILLSWITCH, UrfKillswitchClass))

typedef struct _UrfKillswitch UrfKillswitch;
typedef struct _UrfKillswitchClass UrfKillswitchClass;
typedef struct _UrfKillswitchPrivate UrfKillswitchPrivate;

/**
 * UrfKillswitch:
 *
 * The UrfKillswitch struct contains only private fields
 * and should not be directly accessed.
 */
struct _UrfKillswitch
{
	/*< private >*/
	GObject			 parent;
	UrfKillswitchPrivate	*priv;
};

/**
 * UrfKillswitchClass:
 *
 * Class structure for #UrfKillswitch
 **/
struct _UrfKillswitchClass
{
	/*< private>*/
	GObjectClass		 parent_class;
	void			(*state_changed)	(UrfKillswitch	*killswitch,
							 UrfEnumState	 state);
};

/* general */
GType			 urf_killswitch_get_type	(void);
UrfKillswitch		*urf_killswitch_new		(UrfEnumType	 type);
UrfEnumType		 urf_killswitch_get_switch_type	(UrfKillswitch	*killswitch);

G_END_DECLS

#endif /* __URF_KILLSWITCH_H */
