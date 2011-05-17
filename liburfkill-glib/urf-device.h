/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@novell.com>
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

#ifndef __URF_DEVICE_H
#define __URF_DEVICE_H

#include <glib-object.h>
#include <gio/gio.h>
#include <linux/rfkill.h>

G_BEGIN_DECLS

#define URF_TYPE_DEVICE		(urf_device_get_type ())
#define URF_DEVICE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_DEVICE, UrfDevice))
#define URF_DEVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_CAST((k), URF_TYPE_DEVICE, UrfDeviceClass))
#define URF_IS_DEVICE(o)	(G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_DEVICE))
#define URF_IS_DEVICE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), URF_TYPE_DEVICE))
#define URF_DEVICE_GET_CLASS(o)	(G_TYPE_INSTANCE_GET_CLASS ((o), URF_TYPE_DEVICE, UrfDeviceClass))
#define URF_DEVICE_ERROR	(urf_device_error_quark ())
#define URF_DEVICE_TYPE_ERROR	(urf_device_error_get_type ())

/**
 * UrfDevice:
 *
 * The UrfDevice struct contains only private fields
 * and should not be directly accessed.
 */
typedef struct _UrfDevice UrfDevice;
typedef struct _UrfDevicePrivate UrfDevicePrivate;

struct _UrfDevice
{
	/*< private >*/
	GObject			 parent;
	UrfDevicePrivate	*priv;
};

/**
 * UrfDeviceClass:
 *
 * Class structure for #UrfDevice
 **/

typedef struct
{
	/*< private>*/
	GObjectClass		 parent_class;
} UrfDeviceClass;

/* general */
GType			 urf_device_get_type			(void);
UrfDevice		*urf_device_new				(void);

/* sync versions */
gboolean		 urf_device_set_object_path_sync	(UrfDevice	*device,
								 const char	*object_path,
								 GCancellable	*cancellable,
								 GError		**error);

/* accessors */
const char		*urf_device_get_object_path		(UrfDevice	*device);

G_END_DECLS

#endif /* __URF_DEVICE_H */
