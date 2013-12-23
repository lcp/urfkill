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

#ifndef __URF_SEAT_CONSOLEKIT_H__
#define __URF_SEAT_CONSOLEKIT_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define URF_TYPE_SEAT (urf_seat_get_type())
#define URF_SEAT(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
			URF_TYPE_SEAT, UrfSeat))
#define URF_SEAT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
			URF_TYPE_SEAT, UrfSeatClass))
#define URF_IS_SEAT(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
			URF_TYPE_SEAT))
#define URF_IS_SEAT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
			URF_TYPE_SEAT))
#define URF_GET_SEAT_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
			URF_TYPE_SEAT, UrfSeatClass))

typedef struct UrfSeatPrivate UrfSeatPrivate;

typedef struct {
	GObject 	 parent;
	UrfSeatPrivate	*priv;
} UrfSeat;

typedef struct {
        GObjectClass	 parent_class;
	void		(*active_changed)		(UrfSeat	*seat,
							 const char	*session_id);
	void		(*session_removed)		(UrfSeat	*seat,
							 const char	*session_id);
} UrfSeatClass;

GType			 urf_seat_get_type		(void);

UrfSeat			*urf_seat_new			(void);
gboolean		 urf_seat_object_path_sync	(UrfSeat	*seat,
							 const char	*object_path);

const char		*urf_seat_get_object_path	(UrfSeat	*seat);
const char		*urf_seat_get_active		(UrfSeat	*seat);

G_END_DECLS

#endif /* __URF_SEAT_CONSOLEKIT_H__ */
