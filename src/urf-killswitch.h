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

#ifndef __URF_KILLSWITCH_H__
#define __URF_KILLSWITCH_H__

#include <linux/rfkill.h>
#include <glib-object.h>

#include "urf-device.h"
#include "urf-utils.h"

G_BEGIN_DECLS

#define URF_TYPE_KILLSWITCH (urf_killswitch_get_type())
#define URF_KILLSWITCH(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_KILLSWITCH, UrfKillswitch))
#define URF_KILLSWITCH_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_KILLSWITCH, UrfKillswitchClass))
#define URF_IS_KILLSWITCH(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_KILLSWITCH))
#define URF_IS_KILLSWITCH_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					URF_TYPE_KILLSWITCH))
#define URF_GET_KILLSWITCH_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_KILLSWITCH, UrfKillswitchClass))

typedef struct UrfKillswitchPrivate UrfKillswitchPrivate;

typedef struct {
	GObject parent;
	UrfKillswitchPrivate *priv;
} UrfKillswitch;

typedef struct {
        GObjectClass parent_class;
} UrfKillswitchClass;

GType			 urf_killswitch_get_type		(void);

UrfKillswitch		*urf_killswitch_new			(enum rfkill_type	 type);
void			 urf_killswitch_add_device		(UrfKillswitch		*killswitch,
								 UrfDevice		*device);
void			 urf_killswitch_del_device		(UrfKillswitch		*killswitch,
								 UrfDevice		*device);
KillswitchState		 urf_killswitch_get_state		(UrfKillswitch		*killswitch);
KillswitchState		 urf_killswitch_get_saved_state		(UrfKillswitch		*killswitch);
void			 urf_killswitch_set_saved_state		(UrfKillswitch		*killswitch,
								 KillswitchState         state);
gboolean		 urf_killswitch_set_software_blocked	(UrfKillswitch		*killswitch,
								 gboolean		 blocked);

G_END_DECLS

#endif /* __URF_KILLSWITCH_H__ */
