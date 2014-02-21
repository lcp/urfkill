/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2014 Mathieu Trudel-Lapierre <mathieu.trudel-lapierre@canonical.com>
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

#ifndef __URF_OFONO_MANAGER_H
#define __URF_OFONO_MANAGER_H

#include <glib-object.h>

#include "urf-arbitrator.h"

G_BEGIN_DECLS

#define URF_TYPE_OFONO_MANAGER (urf_ofono_manager_get_type ())
#define URF_OFONO_MANAGER(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), URF_TYPE_OFONO_MANAGER, UrfOfonoManager))
#define URF_IS_OFONO_MANAGER(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), URF_TYPE_OFONO_MANAGER))

typedef struct _UrfOfonoManager UrfOfonoManager;

GType urf_ofono_manager_get_type (void);

UrfOfonoManager* urf_ofono_manager_new (void);
gboolean urf_ofono_manager_startup (UrfOfonoManager *ofono,
                                    UrfArbitrator *arbitrator);

G_END_DECLS

#endif /* __URF_OFONO_MANAGER_H */

