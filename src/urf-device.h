/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2011 Gary Ching-Pang Lin <glin@novell.com>
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

#ifndef __URF_DEVICE_H__
#define __URF_DEVICE_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define URF_TYPE_DEVICE (urf_device_get_type())
#define URF_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_DEVICE, UrfDevice))
#define URF_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_DEVICE, UrfDeviceClass))
#define URF_IS_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_DEVICE))
#define URF_IS_DEVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					URF_TYPE_DEVICE))
#define URF_GET_DEVICE_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_DEVICE, UrfDeviceClass))

typedef struct UrfDevicePrivate UrfDevicePrivate;

typedef struct {
	GObject parent;
	UrfDevicePrivate *priv;
} UrfDevice;

typedef struct {
        GObjectClass parent_class;
} UrfDeviceClass;

GType			 urf_device_get_type		(void);

UrfDevice		*urf_device_new			(guint		 index,
							 guint		 type,
							 gboolean	 soft,
							 gboolean	 hard);

guint			 urf_device_get_index		(UrfDevice	*device);
guint			 urf_device_get_rf_type		(UrfDevice	*device);
const char 		*urf_device_get_name		(UrfDevice	*device);
gboolean		 urf_device_get_soft		(UrfDevice	*device);
gboolean		 urf_device_get_hard		(UrfDevice	*device);

G_END_DECLS

#endif /* __URF_DEVICE_H__ */
