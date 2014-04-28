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

#ifndef __URF_DEVICE_H__
#define __URF_DEVICE_H__

#include <glib-object.h>
#include <gio/gio.h>
#include "urf-utils.h"

G_BEGIN_DECLS

#define URF_TYPE_DEVICE (urf_device_get_type())
#define URF_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_DEVICE, UrfDevice))
#define URF_DEVICE_CLASS(class) (G_TYPE_CHECK_CLASS_CAST((class), \
					URF_TYPE_DEVICE, UrfDeviceClass))
#define URF_IS_DEVICE(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_DEVICE))
#define URF_IS_DEVICE_CLASS(class) (G_TYPE_CHECK_CLASS_TYPE((class), \
					URF_TYPE_DEVICE))
#define URF_GET_DEVICE_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_DEVICE, UrfDeviceClass))

typedef enum {
	URF_DEVICE_TYPE_UNKNOWN		= -1,
	URF_DEVICE_TYPE_KERNEL		= 0,
	URF_DEVICE_TYPE_OFONO		= 1,
	URF_DEVICE_TYPE_MAX,
} UrfDeviceType;

typedef struct _UrfDevicePrivate UrfDevicePrivate;

typedef struct {
	GObject parent;
} UrfDevice;

typedef struct {
	GObjectClass parent;
	gint			 (*get_index)			(UrfDevice	*device);
	guint			 (*get_device_type)		(UrfDevice	*device);
	const char		*(*get_urf_type)		(UrfDevice	*device);
	const char		*(*get_name)			(UrfDevice	*device);
	KillswitchState		 (*get_state)			(UrfDevice	*device);
	void			 (*set_state)			(UrfDevice	*device,
								 KillswitchState state);
	gboolean		 (*update_states)		(UrfDevice	*device,
                                                                 gboolean soft,
                                                                 gboolean hard);
	gboolean		 (*is_platform)			(UrfDevice	*device);
	gboolean		 (*set_hardware_blocked)	(UrfDevice	*device,
								 gboolean blocked);
	gboolean		 (*is_hardware_blocked)		(UrfDevice	*device);
	gboolean		 (*set_software_blocked)	(UrfDevice	*device,
								 gboolean blocked);
	gboolean		 (*is_software_blocked)		(UrfDevice	*device);
} UrfDeviceClass;

GType			 urf_device_get_type		(void);

UrfDevice		*urf_device_new			(guint		 index,
							 guint		 type,
							 gboolean	 soft,
							 gboolean	 hard);

gboolean		 urf_device_update_states	(UrfDevice	*device,
							 const gboolean	 soft,
							 const gboolean	 hard);

gint			 urf_device_get_index		(UrfDevice	*device);
const char		*urf_device_get_object_path	(UrfDevice	*device);
guint			 urf_device_get_device_type	(UrfDevice	*device);
const char		*urf_device_get_name		(UrfDevice	*device);
KillswitchState		 urf_device_get_state		(UrfDevice	*device);
gboolean		 urf_device_is_platform		(UrfDevice	*device);
gboolean		 urf_device_is_hardware_blocked	(UrfDevice	*device);
gboolean		 urf_device_set_software_blocked (UrfDevice	*device,
                                                          gboolean block);
gboolean		 urf_device_is_software_blocked	(UrfDevice	*device);

gboolean		 urf_device_register_device	(UrfDevice			*device,
							 const GDBusInterfaceVTable	 vtable,
							 const char			*xml);

G_END_DECLS

#endif /* __URF_DEVICE_H__ */
