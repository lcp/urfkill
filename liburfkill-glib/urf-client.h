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

#if !defined (__URFKILL_H_INSIDE__) && !defined (URF_COMPILATION)
#error "Only <urfkill.h> can be included directly."
#endif

#ifndef __URF_CLIENT_H
#define __URF_CLIENT_H

#include <glib-object.h>
#include <gio/gio.h>

#include "urf-device.h"

G_BEGIN_DECLS

#define URF_TYPE_CLIENT			(urf_client_get_type ())
#define URF_CLIENT(o)			(G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_CLIENT, UrfClient))
#define URF_CLIENT_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), URF_TYPE_CLIENT, UrfClientClass))
#define URF_IS_CLIENT(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_CLIENT))
#define URF_IS_CLIENT_CLASS(k)		(G_TYPE_CHECK_CLASS_TYPE ((k), URF_TYPE_CLIENT))
#define URF_CLIENT_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), URF_TYPE_CLIENT, UrfClientClass))
#define URF_CLIENT_ERROR		(urf_client_error_quark ())
#define URF_CLIENT_TYPE_ERROR		(urf_client_error_get_type ())

typedef struct _UrfClient UrfClient;
typedef struct _UrfClientClass UrfClientClass;
typedef struct _UrfClientPrivate UrfClientPrivate;

/**
 * UrfClient:
 *
 * The UrfClient struct contains only private fields
 * and should not be directly accessed.
 **/
struct _UrfClient
{
	/*< private >*/
	GObject			 parent;
	UrfClientPrivate	*priv;
};

/**
 * UrfClientClass:
 *
 * Class structure for #UrfClient
 **/
struct _UrfClientClass
{
	/*< private >*/
	GObjectClass		 parent_class;
	void			(*device_added)		(UrfClient	*client,
							 UrfDevice	*device);
	void			(*device_removed)	(UrfClient	*client,
							 UrfDevice	*device);
	void			(*device_changed)	(UrfClient	*client,
							 UrfDevice	*device);
};

/* general */
GType		 urf_client_get_type			(void);
UrfClient	*urf_client_new				(void);

/* generic */
GPtrArray	*urf_client_get_devices			(UrfClient	*client);
gboolean	 urf_client_set_block 			(UrfClient	*client,
							 UrfDeviceType	 type,
							 const gboolean	 block,
							 GCancellable	*cancellable,
							 GError		**error);
gboolean	 urf_client_set_block_idx 		(UrfClient	*client,
							 const guint	 index,
							 const gboolean	 block,
							 GCancellable	*cancellable,
							 GError		**error);
gboolean	 urf_client_key_control_enabled		(UrfClient	*client,
							 GError		**error);
guint		 urf_client_inhibit			(UrfClient	*client,
							 GError		**error);
void		 urf_client_uninhibit			(UrfClient	*client,
							 const guint	 cookie);

/* specific type */
gboolean	 urf_client_set_wlan_block		(UrfClient	*client,
							 const gboolean  block);
gboolean	 urf_client_set_bluetooth_block		(UrfClient	*client,
							 const gboolean  block);
gboolean	 urf_client_set_wwan_block		(UrfClient	*client,
							 const gboolean  block);

/* accessors */
const char	*urf_client_get_daemon_version		(UrfClient	*client);

G_END_DECLS

#endif /* __URF_CLIENT_H */
