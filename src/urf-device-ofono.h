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

#ifndef __URF_DEVICE_OFONO_H__
#define __URF_DEVICE_OFONO_H__

#include <glib-object.h>
#include "urf-device.h"
#include "urf-utils.h"

G_BEGIN_DECLS

#define URF_TYPE_DEVICE_OFONO (urf_device_ofono_get_type())
#define URF_DEVICE_OFONO(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
                               URF_TYPE_DEVICE, UrfDeviceOfono))
#define URF_DEVICE_OFONO_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
                                       URF_TYPE_DEVICE, UrfDeviceOfonoClass))
#define URF_IS_DEVICE_OFONO(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
                                  URF_TYPE_DEVICE_OFONO))
#define URF_IS_DEVICE_OFONO_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
                                          URF_TYPE_DEVICE_OFONO))
#define URF_GET_DEVICE_OFONO_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
                                         URF_TYPE_DEVICE_OFONO, UrfDeviceOfonoClass))

typedef struct _UrfDeviceOfonoPrivate UrfDeviceOfonoPrivate;

typedef struct {
        UrfDevice parent;
} UrfDeviceOfono;

typedef struct {
        UrfDeviceClass parent;
} UrfDeviceOfonoClass;


GType			 urf_device_ofono_get_type		(void);

UrfDevice		*urf_device_ofono_new			(gint index, const char *object_path);

gchar			*urf_device_ofono_get_path		(UrfDeviceOfono *ofono);

G_END_DECLS

#endif /* __URF_DEVICE_OFONO_H__ */
