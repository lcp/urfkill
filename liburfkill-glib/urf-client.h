/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
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

#ifndef __URF_CLIENT_H
#define __URF_CLIENT_H

#include <glib-object.h>
#include <gio/gio.h>

#include "urf-killswitch.h"

G_BEGIN_DECLS

#define URF_TYPE_CLIENT			(urf_client_get_type ())
#define URF_CLIENT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_CLIENT, UrfClient))
#define URF_CLIENT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), URF_TYPE_CLIENT, UrfClientClass))
#define URF_IS_CLIENT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_CLIENT))
#define URF_IS_CLIENT_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), URF_TYPE_CLIENT))
#define URF_CLIENT_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), URF_TYPE_CLIENT, UrfClientClass))
#define URF_CLIENT_ERROR		(urf_client_error_quark ())
#define URF_CLIENT_TYPE_ERROR		(urf_client_error_get_type ())

typedef struct UrfClientPrivate UrfClientPrivate;

typedef struct
{
	 GObject		 parent;
	 UrfClientPrivate	*priv;
} UrfClient;

typedef struct
{
	GObjectClass		 parent_class;
	void			(*rfkill_added)		(UrfClient	*client,
							 UrfKillSwitch	*killswitch);
	void			(*rfkill_removed)	(UrfClient	*client,
							 UrfKillSwitch	*killswitch);
	void			(*rfkill_changed)	(UrfClient	*client,
							 UrfKillSwitch	*killswitch);
} UrfClientClass;

/* general */
GType		 urf_client_get_type			(void);
UrfClient	*urf_client_new				(void);

/* generic */
GPtrArray	*urf_client_get_all_states		(UrfClient	*client,
							 GCancellable	*cancellable,
							 GError		**error);
gboolean	 urf_client_set_block 			(UrfClient	*client,
							 const char	*type,
							 GCancellable	*cancellable,
							 GError		**error);
gboolean	 urf_client_set_unblock			(UrfClient	*client,
							 const char	*type,
							 GCancellable	*cancellable,
							 GError		**error);

/* specific */
gboolean	 urf_client_set_wlan_block		(UrfClient	*client);
gboolean	 urf_client_set_wlan_unblock		(UrfClient	*client);
gboolean	 urf_client_set_bluetooth_block		(UrfClient	*client);
gboolean	 urf_client_set_bluetooth_unblock	(UrfClient	*client);
gboolean	 urf_client_set_wwan_block		(UrfClient	*client);
gboolean	 urf_client_set_wwan_unblock		(UrfClient	*client);

G_END_DECLS

#endif /* __URF_CLIENT_H */
