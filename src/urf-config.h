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
#ifndef __URF_CONFIG_H__
#define __URF_CONFIG_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define URF_TYPE_CONFIG (urf_config_get_type())
#define URF_CONFIG(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj), \
					URF_TYPE_CONFIG, UrfConfig))
#define URF_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass), \
					URF_TYPE_CONFIG, UrfConfigClass))
#define URF_IS_CONFIG(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj), \
					URF_TYPE_CONFIG))
#define URF_IS_CONFIG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), \
					URF_TYPE_CONFIG))
#define URF_GET_CONFIG_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS((obj), \
					URF_TYPE_CONFIG, UrfConfigClass))

typedef struct UrfConfigPrivate UrfConfigPrivate;

typedef struct {
	GObject parent;
	UrfConfigPrivate *priv;
} UrfConfig;

typedef struct {
        GObjectClass parent_class;

        void (*rf_key_pressed)   (UrfConfig *config, guint code);
} UrfConfigClass;

GType		 urf_config_get_type 		(void);
UrfConfig	*urf_config_new			(void);
void		 urf_config_load_from_file	(UrfConfig	*config,
						 const char	*filename);
const char	*urf_config_get_user		(UrfConfig	*config);
gboolean	 urf_config_get_key_control	(UrfConfig	*config);
gboolean	 urf_config_get_master_key	(UrfConfig	*config);
gboolean	 urf_config_get_force_sync	(UrfConfig	*config);

G_END_DECLS

#endif /* __URF_CONFIG_H__ */
