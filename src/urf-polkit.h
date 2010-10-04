/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Richard Hughes <richard@hughsie.com>
 * Copyright (C) 2010 Gary Ching-Pang Lin <glin@novell.com>
 *
 * Licensed under the GNU General Public License Version 2
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __URF_POLKIT_H
#define __URF_POLKIT_H

#include <glib-object.h>
#include <polkit/polkit.h>

G_BEGIN_DECLS

#define URF_TYPE_POLKIT		(urf_polkit_get_type ())
#define URF_POLKIT(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_POLKIT, UrfPolkit))
#define URF_POLKIT_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), URF_TYPE_POLKIT, UrfPolkitClass))
#define URF_IS_POLKIT(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_POLKIT))
#define URF_IS_POLKIT_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), URF_TYPE_POLKIT))
#define URF_POLKIT_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), URF_TYPE_POLKIT, UrfPolkitClass))

typedef struct UrfPolkitPrivate UrfPolkitPrivate;

typedef struct
{
	GObject parent;
	UrfPolkitPrivate	*priv;
} UrfPolkit;

typedef struct
{
	GObjectClass parent_class;
} UrfPolkitClass;

GType		 urf_polkit_get_type			(void);
UrfPolkit	*urf_polkit_new				(void);
void		 urf_polkit_test			(gpointer		 user_data);

PolKitCaller	*urf_polkit_caller_new_from_sender	(UrfPolkit		*polkit,
							 const gchar		*sender);
void		 urf_polkit_caller_unref		(PolKitCaller		*caller);
gboolean	 urf_polkit_check_auth			(UrfPolkit		*polkit,
							 PolKitCaller		*caller,
							 const gchar		*action_id,
							 DBusGMethodInvocation	*context);


G_END_DECLS

#endif /* __URF_POLKIT_H */

