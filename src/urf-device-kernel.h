/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Mathieu Trudel-Lapierre <mathieu.trudel-lapierre@canonical.com>
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

#ifndef __URF_DEVICE_KERNEL_H__
#define __URF_DEVICE_KERNEL_H__

#include <glib-object.h>
#include "urf-device.h"
#include "urf-utils.h"

G_BEGIN_DECLS

#define URF_TYPE_DEVICE_KERNEL (urf_device_kernel_get_type())
#define URF_DEVICE_KERNEL(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_DEVICE, UrfDeviceKernel))
#define URF_DEVICE_KERNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_DEVICE, UrfDeviceKernelClass))
#define URF_IS_DEVICE_KERNEL(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_DEVICE_KERNEL))
#define URF_IS_DEVICE_KERNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					   URF_TYPE_DEVICE_KERNEL))
#define URF_GET_DEVICE_KERNEL_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_DEVICE_KERNEL, UrfDeviceKernelClass))

typedef struct _UrfDeviceKernelPrivate UrfDeviceKernelPrivate;

typedef struct {
	UrfDevice parent;
} UrfDeviceKernel;

typedef struct {
	UrfDeviceClass parent;
} UrfDeviceKernelClass;

GType			 urf_device_kernel_get_type		(void);

UrfDevice		*urf_device_kernel_new			(gint			 index,
								 gint			 type,
								 gboolean		 soft,
								 gboolean		 hard);

G_END_DECLS

#endif /* __URF_DEVICE_KERNEL_H__ */
