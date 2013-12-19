/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005-2008 Marcel Holtmann <marcel@holtmann.org>
 * Copyright (C) 2006-2009 Bastien Nocera <hadess@hadess.net>
 * Copyright (C) 2010-2011 Gary Ching-Pang Lin <glin@suse.com>
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

#ifndef __URF_ARBITRATOR_H__
#define __URF_ARBITRATOR_H__

#include <glib-object.h>

#include "urf-config.h"
#include "urf-device.h"
#include "urf-utils.h"

G_BEGIN_DECLS

#define URF_TYPE_ARBITRATOR (urf_arbitrator_get_type())
#define URF_ARBITRATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_ARBITRATOR, UrfArbitrator))
#define URF_ARBITRATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_ARBITRATOR, UrfArbitratorClass))
#define URF_IS_ARBITRATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_ARBITRATOR))
#define URF_IS_ARBITRATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					URF_TYPE_ARBITRATOR))
#define URF_GET_ARBITRATOR_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_ARBITRATOR, UrfArbitratorClass))

typedef struct UrfArbitratorPrivate UrfArbitratorPrivate;

typedef struct {
	GObject			 parent;
	UrfArbitratorPrivate	*priv;
} UrfArbitrator;

typedef struct {
        GObjectClass 		 parent_class;

        void 			(*device_added)		(UrfArbitrator	*arbitrator,
							 const char	*object_path);
        void 			(*device_removed)	(UrfArbitrator	*arbitrator,
							 const char	*object_path);
        void 			(*device_changed)	(UrfArbitrator	*arbitrator,
							 const char	*object_path);
} UrfArbitratorClass;

GType			 urf_arbitrator_get_type		(void);
UrfArbitrator		*urf_arbitrator_new			(void);

gboolean		 urf_arbitrator_startup			(UrfArbitrator  *arbitrator,
								 UrfConfig	*config);

gboolean		 urf_arbitrator_has_devices		(UrfArbitrator	*arbitrator);
GList			*urf_arbitrator_get_devices		(UrfArbitrator	*arbitrator);
UrfDevice		*urf_arbitrator_get_device		(UrfArbitrator  *arbitrator,
								 const guint	 index);
gboolean		 urf_arbitrator_set_block		(UrfArbitrator	*arbitrator,
								 const guint	 type,
								 const gboolean	 block);
gboolean		 urf_arbitrator_set_block_idx		(UrfArbitrator	*arbitrator,
								 const guint	 index,
								 const gboolean	 block);
gboolean		 urf_arbitrator_set_flight_mode		(UrfArbitrator	*arbitrator,
								 const gboolean	 block);
KillswitchState		 urf_arbitrator_get_state		(UrfArbitrator	*arbitrator,
								 guint 		 type);
KillswitchState		 urf_arbitrator_get_state_idx		(UrfArbitrator	*arbitrator,
								 guint 		 index);

G_END_DECLS

#endif /* __URF_ARBITRATOR_H__ */
